// ASCII CPP TAB4 CRLF
// Docutitle: (Device) HDMI Consumer Electronics Control (CEC)
// Codifiers: @ArinaMgk: 20260731
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
/*
	Copyright 2023 ArinaMgk

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0
	http://unisym.org/license.html

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/

#include "../../../../inc/cpp/Device/Video/HDMI-CEC.hpp"

#if defined(_MCU_STM32H7x)

#include "../../../../inc/cpp/Device/NVIC"
#include "../../../../inc/cpp/Device/RCC/RCCAddress"

namespace uni {

	// CEC_BASE = D2_APB1PERIPH_BASE + 0x6C00 (D2 APB1 at 0x40000000)
	#define _CEC_ADDR 0x40006C00

	// CEC_CR bit positions
	#define _CEC_CR_POSI_CECEN 0
	#define _CEC_CR_POSI_TXSOM 1
	#define _CEC_CR_POSI_TXEOM 2

	// CEC_CFGR field positions
	#define _CEC_CFGR_POSI_SFT 0
	#define _CEC_CFGR_POSI_RXTOL 3
	#define _CEC_CFGR_POSI_BRESTP 4
	#define _CEC_CFGR_POSI_BREGEN 5
	#define _CEC_CFGR_POSI_LBPEGEN 6
	#define _CEC_CFGR_POSI_BRDNOGEN 7
	#define _CEC_CFGR_POSI_SFTOPT 8
	#define _CEC_CFGR_POSI_OAR 16
	#define _CEC_CFGR_LENI_OAR 15
	#define _CEC_CFGR_POSI_LSTN 31

	// CEC_ISR bit positions (W1C flags)
	#define _CEC_ISR_POSI_RXBR 0
	#define _CEC_ISR_POSI_RXEND 1
	#define _CEC_ISR_POSI_TXBR 8
	#define _CEC_ISR_POSI_TXEND 9

	// all IER/ISR bits 0..12 (RXBR|RXEND|RXOVR|BRE|SBPE|LBPE|RXACKE|ARBLST|TXBR|TXEND|TXUDR|TXERR|TXACKE)
	#define _CEC_ALL_IT_FLAGS 0x1FFF

	// RCC D2 APB1LENR.CECEN bit (H7)
	#define _RCC_APB1LENR_CECEN_Pos 27

	HDMI_CEC_t::HDMI_CEC_t() {}

	Reference HDMI_CEC_t::operator[](CECReg idx) const {
		return Reference(_CEC_ADDR + _IMMx4(idx));
	}

	bool HDMI_CEC_t::enClock(bool ena) const {
		Reference(_RCC_APB1LENR_ADDR).setof(_RCC_APB1LENR_CECEN_Pos, ena);
		return true;
	}

	bool HDMI_CEC_t::enAble(bool ena) const {
		self[CECReg::CR].setof(_CEC_CR_POSI_CECEN, ena);
		return true;
	}

	// AKA HAL_CEC_Init: one-shot CFGR write + enable all RX/TX/error interrupts + CECEN
	bool HDMI_CEC_t::setMode(CECSignalFreeTime sft, CECTolerance tolerance, stduint ownAddress, CECListenMode listen) {
		if (!rx_buffer) return false;   // HAL: Init.RxBuffer == NULL -> HAL_ERROR
		enClock();
		self[CECReg::CR].setof(_CEC_CR_POSI_CECEN, false); // disable the peripheral (HAL order)
		// combined value mirrors HAL Init's OR of all CEC_InitTypeDef fields
		self[CECReg::CFGR] = ((stduint)sft & 0x7)
			| ((stduint)tolerance << _CEC_CFGR_POSI_RXTOL)
			| ((stduint)rx_stop_bre << _CEC_CFGR_POSI_BRESTP)
			| ((stduint)bre_err_gen << _CEC_CFGR_POSI_BREGEN)
			| ((stduint)lbpe_err_gen << _CEC_CFGR_POSI_LBPEGEN)
			| ((stduint)brd_no_gen << _CEC_CFGR_POSI_BRDNOGEN)
			| ((stduint)soft_opt << _CEC_CFGR_POSI_SFTOPT)
			| (ownAddress << _CEC_CFGR_POSI_OAR)
			| ((stduint)listen << _CEC_CFGR_POSI_LSTN);
		self[CECReg::IER] |= _CEC_ALL_IT_FLAGS; // enable all RX/TX/error interrupts (HAL set)
		enAble(true);
		errcode = ERR_CEC_NONE;
		lock_tx = false;
		lock_rx = false;
		return true;
	}

	// AKA HAL_CEC_DeInit
	bool HDMI_CEC_t::canMode() {
		self[CECReg::CR].setof(_CEC_CR_POSI_CECEN, false); // disable the peripheral
		self[CECReg::ISR] |= _CEC_ALL_IT_FLAGS;            // clear all flags (W1C)
		self[CECReg::IER] = 0;                             // disable all interrupts
		errcode = ERR_CEC_NONE;
		lock_tx = false;
		lock_rx = false;
		tx_count = 0;
		rx_size = 0;
		enClock(false);                                    // gate the peripheral clock
		return true;
	}

	// AKA HAL_CEC_SetDeviceAddress (multi-address mode: OR keeps previously set addresses)
	bool HDMI_CEC_t::setOwnAddress(stduint ownAddress) {
		if (lock_tx || lock_rx) return false;  // HAL: not READY -> HAL_BUSY
		enAble(false);
		if (ownAddress) self[CECReg::CFGR] |= (ownAddress << _CEC_CFGR_POSI_OAR);
		else self[CECReg::CFGR].maset(_CEC_CFGR_POSI_OAR, _CEC_CFGR_LENI_OAR, 0);
		errcode = ERR_CEC_NONE;
		enAble(true);
		return true;
	}

	// AKA HAL_CEC_Transmit_IT
	bool HDMI_CEC_t::Transmit(byte initiator, byte destination, const byte* data, stduint size) {
		if (lock_tx) return false;               // HAL: gState != READY -> HAL_BUSY
		if (!data && size > 0) return false;     // HAL: (pData == NULL && Size > 0) -> HAL_ERROR
		if (size > 15) return false;             // IS_CEC_MSGSIZE (1 opcode + up to 14 operands)
		if ((initiator | destination) > 0x0F) return false; // IS_CEC_ADDRESS (4-bit logical addresses)
		tx_ptr = data;
		tx_count = size;
		errcode = ERR_CEC_NONE;
		lock_tx = true;                          // AKA gState = BUSY_TX
		if (size == 0) self[CECReg::CR].setof(_CEC_CR_POSI_TXEOM); // ping: header is also the last byte
		self[CECReg::TXDR] = (byte)((initiator << 4) | destination); // header block
		self[CECReg::CR].setof(_CEC_CR_POSI_TXSOM); // TXSOM: start of message
		return true;
	}

	// AKA HAL_CEC_GetState (simplified)
	CECState HDMI_CEC_t::getState() const {
		if (errcode != ERR_CEC_NONE) return CECState::Error;
		if (!self[CECReg::CR].bitof(_CEC_CR_POSI_CECEN)) return CECState::Reset;
		if (lock_tx) return CECState::BusyTX;
		if (lock_rx) return CECState::BusyRX;
		return CECState::Ready;
	}

	// AKA HAL_CEC_IRQHandler
	void HDMI_CEC_t::IRQHandler() {
		stduint reg = self[CECReg::ISR]; // snapshot

		// Arbitration Lost: record only, no callback, no state reset (hardware retries)
		if (reg & ERR_CEC_ARBLST) {
			errcode = ERR_CEC_ARBLST;
			self[CECReg::ISR].setof(7); // W1C clear ARBLST
		}

		// RX: byte received
		if (reg & _IMM1S(_CEC_ISR_POSI_RXBR)) {
			lock_rx = true;
			rx_size++;
			if (rx_buffer) *rx_buffer++ = (byte)self[CECReg::RXDR];
			self[CECReg::ISR].setof(_CEC_ISR_POSI_RXBR); // W1C clear RXBR
		}

		// RX: end of reception
		if (reg & _IMM1S(_CEC_ISR_POSI_RXEND)) {
			self[CECReg::ISR].setof(_CEC_ISR_POSI_RXEND); // W1C clear RXEND
			lock_rx = false;
			errcode = ERR_CEC_NONE;
			if (rx_buffer) rx_buffer -= rx_size; // rewind to frame start
			asserv(RxCpltHandler)();
			rx_size = 0;
		}

		// TX: byte request
		// NOTE: fixed vs HAL bug (ST community): the HAL writes an extra byte into TXDR
		// when TxXferCount reaches 0, so the frame is always one byte longer than Size.
		// Correct flow: the last data byte (count==1) is written with TXEOM set; a
		// subsequent TXBR with count==0 does nothing (message ends, TXEND follows).
		if (reg & _IMM1S(_CEC_ISR_POSI_TXBR)) {
			if (tx_count == 0) {
				// nothing to send; wait for TXEND
			} else {
				if (tx_count == 1) self[CECReg::CR].setof(_CEC_CR_POSI_TXEOM); // last byte
				if (tx_ptr) self[CECReg::TXDR] = *tx_ptr++;
				tx_count--;
			}
			self[CECReg::ISR].setof(_CEC_ISR_POSI_TXBR); // W1C clear TXBR
		}

		// TX: end of transmission
		if (reg & _IMM1S(_CEC_ISR_POSI_TXEND)) {
			self[CECReg::ISR].setof(_CEC_ISR_POSI_TXEND); // W1C clear TXEND
			lock_tx = false;
			errcode = ERR_CEC_NONE;
			asserv(TxCpltHandler)();
		}

		// RX/TX errors (ARBLST excluded here; handled above)
		if (reg & (ERR_CEC_RXOVR | ERR_CEC_BRE | ERR_CEC_SBPE | ERR_CEC_LBPE | ERR_CEC_RXACKE
			| ERR_CEC_TXUDR | ERR_CEC_TXERR | ERR_CEC_TXACKE)) {
			errcode = reg;
			self[CECReg::ISR] |= (ERR_CEC_RXOVR | ERR_CEC_BRE | ERR_CEC_SBPE | ERR_CEC_LBPE | ERR_CEC_RXACKE
				| ERR_CEC_TXUDR | ERR_CEC_TXERR | ERR_CEC_TXACKE); // W1C clear all error flags
			if (reg & (ERR_CEC_RXOVR | ERR_CEC_BRE | ERR_CEC_SBPE | ERR_CEC_LBPE | ERR_CEC_RXACKE)) {
				// RX-side error: rewind buffer
				if (rx_buffer) rx_buffer -= rx_size;
				rx_size = 0;
				lock_rx = false;
			}
			else if ((reg & (ERR_CEC_TXUDR | ERR_CEC_TXERR | ERR_CEC_TXACKE)) && !(reg & ERR_CEC_ARBLST)) {
				// TX-side error (without arbitration lost): release TX lock
				lock_tx = false;
			}
			asserv(ErrorHandler)();
		}
	}

	// RuptTrait: the CEC IRQ vector is hardwired to HDMI_CEC.IRQHandler(), which
	// dispatches to TxCpltHandler / RxCpltHandler / ErrorHandler; there is no
	// per-instance handler table, so setInterrupt is intentionally a no-op.
	void HDMI_CEC_t::setInterrupt(Handler_t f) const { (void)f; }
	void HDMI_CEC_t::setInterruptPriority(byte preempt, byte sub_priority) const {
		NVIC.setPriority(IRQ_CEC, preempt, sub_priority);
	}
	void HDMI_CEC_t::enInterrupt(bool enable) const {
		NVIC.setAble(IRQ_CEC, enable);
	}

	HDMI_CEC_t HDMI_CEC;

}

// vector table refers to this symbol; IRQ handler dispatches to the single CEC instance
// NOTE: must keep C linkage — the startup assembly vector table references the
//       unmangled symbol `CEC_IRQHandler`, otherwise the weak default (B .) is used.
extern "C" void CEC_IRQHandler(void) {
	uni::HDMI_CEC.IRQHandler();
}

#endif
