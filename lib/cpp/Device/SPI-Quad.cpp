// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Device) Quad Serial Peripheral Interface, QSPI
// Codifiers: @ArinaMgk
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

#include "../../../inc/cpp/Device/SPI-Quad.hpp"
#include "../../../inc/cpp/Device/SysTick"
#include "../../../inc/cpp/Device/NVIC"
#include "../../../inc/cpp/Device/Interrupt/interrupt_tab.h"
#include "Interrupt/interrupt_qspi.hpp"

namespace uni {
#if defined(_MCU_STM32H7x)

	#define _QSPI_FMODE_INDIRECT_WRITE 0x00000000U
	#define _QSPI_FMODE_INDIRECT_READ  0x00000001U  // CCR.FMODE_0
	#define _QSPI_FMODE_AUTO_POLLING   0x00000002U  // CCR.FMODE_1
	#define _QSPI_FMODE_MEMORY_MAPPED  0x00000003U  // CCR.FMODE

	QSPI QSPI1;

	// AKA QSPI_WaitFlagStateUntilTimeout
	bool QSPI::waitFlag(stduint flag_pos, bool expect, uint64 tickstart) {
		while (self[QSPIReg::SR].bitof(flag_pos) != expect) {
			if ((SysTick::getTick() - tickstart) > timeout) {
				error_code |= _QSPI_ERROR_TIMEOUT;
				return false;
			}
		}
		return true;
	}

	// AKA QSPI_Config: build CCR from command + functional mode (8 branches)
	static void _QSPI_Config(QSPI& q, const QSPI_Command& cmd, stduint fmode) {
		if (cmd.data_mode != QSPIDataMode::None && fmode != _QSPI_FMODE_MEMORY_MAPPED) {
			q[QSPIReg::DLR] = cmd.nb_data - 1;
		}
		stduint instr = (stduint)cmd.instruction;
		stduint imode = (stduint)cmd.instruction_mode << _QSPI_CCR_POS_IMODE;
		stduint admode = (stduint)cmd.address_mode << _QSPI_CCR_POS_ADMODE;
		stduint adsize = (stduint)cmd.address_size << _QSPI_CCR_POS_ADSIZE;
		stduint abmode = (stduint)cmd.alternate_byte_mode << _QSPI_CCR_POS_ABMODE;
		stduint absize = (stduint)cmd.alternate_bytes_size << _QSPI_CCR_POS_ABSIZE;
		stduint dcyc = cmd.dummy_cycles << _QSPI_CCR_POS_DCYC;
		stduint dmode = (stduint)cmd.data_mode << _QSPI_CCR_POS_DMODE;
		stduint ddr = (stduint)cmd.ddr_mode << _QSPI_CCR_POS_DDRM;
		stduint dhhc = (stduint)cmd.ddr_hold_half_cycle << _QSPI_CCR_POS_DHHC;
		stduint sioo = (stduint)cmd.sioo_mode << _QSPI_CCR_POS_SIOO;
		stduint base = ddr | dhhc | sioo | dmode | dcyc;

		if (cmd.instruction_mode != QSPIInstrMode::None) {
			if (cmd.alternate_byte_mode != QSPIAlternateMode::None) {
				q[QSPIReg::ABR] = cmd.alternate_bytes;
				if (cmd.address_mode != QSPIAddrMode::None) {
					q[QSPIReg::CCR] = base | absize | abmode | adsize | admode | imode | instr | fmode;
					if (fmode != _QSPI_FMODE_MEMORY_MAPPED) q[QSPIReg::AR] = cmd.address;
				} else {
					q[QSPIReg::CCR] = base | absize | abmode | admode | imode | instr | fmode;
				}
			} else {
				if (cmd.address_mode != QSPIAddrMode::None) {
					q[QSPIReg::CCR] = base | abmode | adsize | admode | imode | instr | fmode;
					if (fmode != _QSPI_FMODE_MEMORY_MAPPED) q[QSPIReg::AR] = cmd.address;
				} else {
					q[QSPIReg::CCR] = base | abmode | admode | imode | instr | fmode;
				}
			}
		} else {
			if (cmd.alternate_byte_mode != QSPIAlternateMode::None) {
				q[QSPIReg::ABR] = cmd.alternate_bytes;
				if (cmd.address_mode != QSPIAddrMode::None) {
					q[QSPIReg::CCR] = base | absize | abmode | adsize | admode | imode | fmode;
					if (fmode != _QSPI_FMODE_MEMORY_MAPPED) q[QSPIReg::AR] = cmd.address;
				} else {
					q[QSPIReg::CCR] = base | absize | abmode | admode | imode | fmode;
				}
			} else {
				if (cmd.address_mode != QSPIAddrMode::None) {
					q[QSPIReg::CCR] = base | abmode | adsize | admode | imode | fmode;
					if (fmode != _QSPI_FMODE_MEMORY_MAPPED) q[QSPIReg::AR] = cmd.address;
				} else {
					if (cmd.data_mode != QSPIDataMode::None) {
						q[QSPIReg::CCR] = base | abmode | admode | imode | fmode;
					}
				}
			}
		}
	}

	// AKA HAL_MDMA_Init for QSPI DMA: fixed QSPI request/trigger, direction-flipped increments.
	// bufferLen == FIFO threshold (ST: buffer transfer length == bytes in FIFO).
	static bool _QSPI_MDMAConfig(const MDMAChannel* ch, stduint fifo_threshold, bool is_tx) {
		return ch->setMode(
			MDMA_REQUEST_QUADSPI_FIFO_TH, MDMATrigger::Buffer,
			is_tx ? MDMAInc::IncByte : MDMAInc::Fixed,
			is_tx ? MDMAInc::Fixed : MDMAInc::IncByte,
			MDMASize::Byte, MDMASize::Byte,
			MDMAAlign::Right, MDMABurst::Single, MDMABurst::Single,
			fifo_threshold);
	}

	// AKA QSPI_DMATxCplt: MDMA pushed all bytes into DR; finish on QSPI TC interrupt.
	void QSPI::MDMATxCplt() {
		QSPI1.tx_count = 0;
		QSPI1[QSPIReg::CR].setof(_QSPI_CR_POS_TCIE);
	}

	// AKA QSPI_DMARxCplt: MDMA drained all bytes from DR; finish on QSPI TC interrupt.
	void QSPI::MDMARxCplt() {
		QSPI1.rx_count = 0;
		QSPI1[QSPIReg::CR].setof(_QSPI_CR_POS_TCIE);
	}

	// AKA QSPI_DMAError: clear DMAEN then abort (AbortRupt takes the non-DMA branch).
	void QSPI::MDMAError() {
		QSPI1.tx_count = 0;
		QSPI1.rx_count = 0;
		QSPI1.error_code |= _QSPI_ERROR_DMA;
		QSPI1[QSPIReg::CR].rstof(_QSPI_CR_POS_DMAEN);
		QSPI1.AbortRupt();
	}

	// AKA QSPI_DMAAbortCplt: MDMA abort finished; trigger QSPI abort, or error if not aborting.
	void QSPI::MDMAAbortCplt() {
		QSPI1.tx_count = 0;
		QSPI1.rx_count = 0;
		if (QSPI1.state == QSPIState::Abort) {
			QSPI1[QSPIReg::FCR].setof(_QSPI_FCR_POS_CTCF);
			QSPI1[QSPIReg::CR].setof(_QSPI_CR_POS_TCIE);
			QSPI1[QSPIReg::CR].setof(_QSPI_CR_POS_ABORT);
		} else {
			QSPI1.state = QSPIState::Ready;
			if (QSPI1.ErrorHandler) QSPI1.ErrorHandler();
		}
	}

	// AKA HAL_QSPI_Init
	bool QSPI::setMode() {
		if (state != QSPIState::Reset) return false;
		enClock();
		// FIFO threshold (CR.FTHRES = threshold - 1)
		self[QSPIReg::CR].maset(_QSPI_CR_POS_FTHRES, 4, init.fifo_threshold - 1);
		uint64 tickstart = SysTick::getTick();
		if (!waitFlag(_QSPI_SR_POS_BUSY, false, tickstart)) return false;
		// clock prescaler + sample shift + flash select + dual flash
		self[QSPIReg::CR].maset(_QSPI_CR_POS_PRESCALER, 8, init.clock_prescaler);
		self[QSPIReg::CR].setof(_QSPI_CR_POS_SSHIFT, (stduint)init.sample_shifting);
		self[QSPIReg::CR].setof(_QSPI_CR_POS_FSEL, (stduint)init.flash_select);
		self[QSPIReg::CR].setof(_QSPI_CR_POS_DFM, (stduint)init.dual_flash);
		// flash size + CS high time + clock mode
		self[QSPIReg::DCR].maset(_QSPI_DCR_POS_FSIZE, 5, init.flash_size);
		self[QSPIReg::DCR].maset(_QSPI_DCR_POS_CSHT, 3, (stduint)init.chip_select_high_time);
		self[QSPIReg::DCR].setof(_QSPI_DCR_POS_CKMODE, (stduint)init.clock_mode);
		enAble(true);
		error_code = _QSPI_ERROR_NONE;
		state = QSPIState::Ready;
		return true;
	}

	// AKA HAL_QSPI_DeInit
	void QSPI::canMode() {
		enAble(false);
		enClock(false);
		error_code = _QSPI_ERROR_NONE;
		state = QSPIState::Reset;
	}

	// AKA __HAL_QSPI_ENABLE / DISABLE (CR.EN)
	void QSPI::enAble(bool ena) {
		self[QSPIReg::CR].setof(_QSPI_CR_POS_EN, ena);
	}

	// RCC AHB3ENR.QSPIEN (bit 14)
	void QSPI::enClock(bool ena) {
		Reference(_RCC_AHB3ENR_ADDR).setof(14, ena);
	}

	// AKA HAL_QSPI_Command / Command_IT
	bool QSPI::Command(const QSPI_Command& cmd, IOMethod method) {
		if (state != QSPIState::Ready) return false;
		error_code = _QSPI_ERROR_NONE;
		state = QSPIState::Busy;
		uint64 tickstart = SysTick::getTick();
		if (!waitFlag(_QSPI_SR_POS_BUSY, false, tickstart)) {
			state = QSPIState::Error;
			return false;
		}
		_QSPI_Config(*this, cmd, _QSPI_FMODE_INDIRECT_WRITE);
		if (cmd.data_mode == QSPIDataMode::None) {
			if (method == IOMethod::Rupt) {
				// clear TE/TC, arm TC+TE IT
				self[QSPIReg::FCR].setof(_QSPI_FCR_POS_CTEF);
				self[QSPIReg::FCR].setof(_QSPI_FCR_POS_CTCF);
				self[QSPIReg::CR].setof(_QSPI_CR_POS_TCIE);
				self[QSPIReg::CR].setof(_QSPI_CR_POS_TEIE);
				return true;
			}
			// blocking: wait TC then clear
			if (!waitFlag(_QSPI_SR_POS_TCF, true, tickstart)) {
				state = QSPIState::Error;
				return false;
			}
			self[QSPIReg::FCR].setof(_QSPI_FCR_POS_CTCF);
			state = QSPIState::Ready;
		} else {
			state = QSPIState::Ready;
		}
		return true;
	}

	// AKA HAL_QSPI_Transmit / Transmit_IT / Transmit_DMA
	bool QSPI::Transmit(byte* data, stduint size, IOMethod method) {
		if (state != QSPIState::Ready) return false;
		if (!data) { error_code |= _QSPI_ERROR_INVALID_PARAM; return false; }
		error_code = _QSPI_ERROR_NONE;
		state = QSPIState::BusyIndirectTx;
		tx_buff = data;
		tx_count = self[QSPIReg::DLR] + 1;
		tx_size = tx_count;
		// functional mode = indirect write
		self[QSPIReg::CCR].maset(_QSPI_CCR_POS_FMODE, 2, _QSPI_FMODE_INDIRECT_WRITE);
		self[QSPIReg::DLR] = tx_count - 1;// re-assert data length right before the transfer
		uint64 tickstart = SysTick::getTick();
		if (method == IOMethod::Rupt) {
			// arm FT + TC + TE interrupts; data pumped in ISR
			self[QSPIReg::CR].setof(_QSPI_CR_POS_FTIE);
			self[QSPIReg::CR].setof(_QSPI_CR_POS_TCIE);
			self[QSPIReg::CR].setof(_QSPI_CR_POS_TEIE);
			return true;
		}
		if (method == IOMethod::DMA) {
			// AKA HAL_QSPI_Transmit_DMA: MDMA memory -> DR, triggered by FIFO threshold.
			if (!hmdma) { error_code |= _QSPI_ERROR_DMA; state = QSPIState::Error; return false; }
			self[QSPIReg::FCR].setof(_QSPI_FCR_POS_CTEF);
			self[QSPIReg::FCR].setof(_QSPI_FCR_POS_CTCF);
			if (!_QSPI_MDMAConfig(hmdma, init.fifo_threshold, true)) {
				error_code |= _QSPI_ERROR_DMA; state = QSPIState::Error; return false;
			}
			byte id = hmdma->getID();
			MDMA.XferCpltCallback[id] = &QSPI::MDMATxCplt;
			MDMA.XferErrorCallback[id] = &QSPI::MDMAError;
			MDMA.XferAbortCallback[id] = 0;
			if (!hmdma->Transfer((pureptr_t)tx_buff, (pureptr_t)(_QSPI_ADDR + _IMMx4(QSPIReg::DR)), tx_size, 1, IOMethod::Rupt)) {
				error_code |= _QSPI_ERROR_DMA; state = QSPIState::Error; return false;
			}
			self[QSPIReg::CR].setof(_QSPI_CR_POS_TEIE);
			return true;
		}
		// blocking: poll FT (threshold reached = room available), write DR byte-wise
		Reference_T<byte> qdr(_QSPI_ADDR + _IMMx4(QSPIReg::DR));
		while (tx_count > 0) {
			if (!waitFlag(_QSPI_SR_POS_FTF, true, tickstart)) {
				state = QSPIState::Error;
				return false;
			}
			qdr = *tx_buff++;
			tx_count--;
		}
		if (!waitFlag(_QSPI_SR_POS_TCF, true, tickstart)) {
			state = QSPIState::Error;
			return false;
		}
		self[QSPIReg::FCR].setof(_QSPI_FCR_POS_CTCF);
		state = QSPIState::Ready;
		return true;
	}

	// AKA HAL_QSPI_Receive / Receive_IT / Receive_DMA
	bool QSPI::Receive(byte* data, stduint size, IOMethod method) {
		if (state != QSPIState::Ready) return false;
		if (!data) { error_code |= _QSPI_ERROR_INVALID_PARAM; return false; }
		error_code = _QSPI_ERROR_NONE;
		state = QSPIState::BusyIndirectRx;
		rx_buff = data;
		rx_count = self[QSPIReg::DLR] + 1;
		rx_size = rx_count;
		stduint addr = self[QSPIReg::AR];
		// functional mode = indirect read
		self[QSPIReg::CCR].maset(_QSPI_CCR_POS_FMODE, 2, _QSPI_FMODE_INDIRECT_READ);
		self[QSPIReg::DLR] = rx_count - 1;// re-assert data length right before the transfer
		self[QSPIReg::AR] = addr;// restart transfer
		// DR must be accessed byte-wise (8-bit): the 32-bit-wide FIFO packs up to 4
		// received bytes per entry, so a 32-bit read would pop them all at once.
		Reference_T<byte> qdr(_QSPI_ADDR + _IMMx4(QSPIReg::DR));
		uint64 tickstart = SysTick::getTick();
		if (method == IOMethod::Rupt) {
			self[QSPIReg::CR].setof(_QSPI_CR_POS_FTIE);
			self[QSPIReg::CR].setof(_QSPI_CR_POS_TCIE);
			self[QSPIReg::CR].setof(_QSPI_CR_POS_TEIE);
			return true;
		}
		if (method == IOMethod::DMA) {
			// AKA HAL_QSPI_Receive_DMA: MDMA DR -> memory; DMAEN gates the receive FIFO request.
			if (!hmdma) { error_code |= _QSPI_ERROR_DMA; state = QSPIState::Error; return false; }
			self[QSPIReg::FCR].setof(_QSPI_FCR_POS_CTEF);
			self[QSPIReg::FCR].setof(_QSPI_FCR_POS_CTCF);
			if (!_QSPI_MDMAConfig(hmdma, init.fifo_threshold, false)) {
				error_code |= _QSPI_ERROR_DMA; state = QSPIState::Error; return false;
			}
			byte id = hmdma->getID();
			MDMA.XferCpltCallback[id] = &QSPI::MDMARxCplt;
			MDMA.XferErrorCallback[id] = &QSPI::MDMAError;
			MDMA.XferAbortCallback[id] = 0;
			if (!hmdma->Transfer((pureptr_t)(_QSPI_ADDR + _IMMx4(QSPIReg::DR)), (pureptr_t)rx_buff, rx_size, 1, IOMethod::Rupt)) {
				error_code |= _QSPI_ERROR_DMA; state = QSPIState::Error; return false;
			}
			self[QSPIReg::CR].setof(_QSPI_CR_POS_TEIE);
			self[QSPIReg::CR].setof(_QSPI_CR_POS_DMAEN);
			return true;
		}
		while (rx_count > 0) {
			// wait until FT (threshold reached = data available) OR TC (transfer done)
			while (!self[QSPIReg::SR].bitof(_QSPI_SR_POS_FTF)
				&& !self[QSPIReg::SR].bitof(_QSPI_SR_POS_TCF)) {
				if ((SysTick::getTick() - tickstart) > timeout) {
					error_code |= _QSPI_ERROR_TIMEOUT;
					state = QSPIState::Error;
					return false;
				}
			}
			*rx_buff++ = (byte)qdr;
			rx_count--;
		}
		if (!waitFlag(_QSPI_SR_POS_TCF, true, tickstart)) {
			state = QSPIState::Error;
			return false;
		}
		self[QSPIReg::FCR].setof(_QSPI_FCR_POS_CTCF);
		state = QSPIState::Ready;
		return true;
	}

	// AKA HAL_QSPI_AutoPolling / AutoPolling_IT
	bool QSPI::AutoPolling(const QSPI_Command& cmd, const QSPI_AutoPolling& cfg, IOMethod method) {
		if (state != QSPIState::Ready) return false;
		error_code = _QSPI_ERROR_NONE;
		state = QSPIState::BusyAutoPolling;
		uint64 tickstart = SysTick::getTick();
		if (!waitFlag(_QSPI_SR_POS_BUSY, false, tickstart)) {
			state = QSPIState::Error;
			return false;
		}
		self[QSPIReg::PSMAR] = cfg.match;
		self[QSPIReg::PSMKR] = cfg.mask;
		self[QSPIReg::PIR] = cfg.interval;
		// PMM + APMS (automatic stop always enabled in blocking mode)
		self[QSPIReg::CR].setof(_QSPI_CR_POS_PMM, (stduint)cfg.match_mode);
		self[QSPIReg::CR].setof(_QSPI_CR_POS_APMS, true);
		QSPI_Command cmd2 = cmd;
		cmd2.nb_data = cfg.status_bytes_size;
		_QSPI_Config(*this, cmd2, _QSPI_FMODE_AUTO_POLLING);
		if (method == IOMethod::Rupt) {
			// arm SM + TE interrupts
			self[QSPIReg::CR].setof(_QSPI_CR_POS_SMIE);
			self[QSPIReg::CR].setof(_QSPI_CR_POS_TEIE);
			return true;
		}
		if (!waitFlag(_QSPI_SR_POS_SMF, true, tickstart)) {
			state = QSPIState::Error;
			return false;
		}
		self[QSPIReg::FCR].setof(_QSPI_FCR_POS_CSMF);
		state = QSPIState::Ready;
		return true;
	}

	// AKA HAL_QSPI_MemoryMapped
	bool QSPI::MemoryMapped(const QSPI_Command& cmd, const QSPI_MemoryMapped& cfg) {
		if (state != QSPIState::Ready) return false;
		error_code = _QSPI_ERROR_NONE;
		state = QSPIState::BusyMemMapped;
		uint64 tickstart = SysTick::getTick();
		if (!waitFlag(_QSPI_SR_POS_BUSY, false, tickstart)) {
			state = QSPIState::Error;
			return false;
		}
		self[QSPIReg::CR].setof(_QSPI_CR_POS_TCEN, (stduint)cfg.timeout_activation);
		if (cfg.timeout_activation == QSPITimeoutActivation::Enable) {
			self[QSPIReg::LPTR] = cfg.timeout_period;
			self[QSPIReg::FCR].setof(_QSPI_FCR_POS_CTOF);
			self[QSPIReg::CR].setof(_QSPI_CR_POS_TOIE);
		}
		_QSPI_Config(*this, cmd, _QSPI_FMODE_MEMORY_MAPPED);
		return true;
	}

	// AKA HAL_QSPI_Abort
	bool QSPI::Abort() {
		if (!((stduint)state & 0x2)) return true;// not busy
		uint64 tickstart = SysTick::getTick();
		if (self[QSPIReg::CR].bitof(_QSPI_CR_POS_DMAEN)) {
			self[QSPIReg::CR].rstof(_QSPI_CR_POS_DMAEN);
			if (hmdma) hmdma->Abort();
		}
		self[QSPIReg::CR].setof(_QSPI_CR_POS_ABORT);
		if (!waitFlag(_QSPI_SR_POS_TCF, true, tickstart)) {
			state = QSPIState::Error;
			return false;
		}
		self[QSPIReg::FCR].setof(_QSPI_FCR_POS_CTCF);
		if (!waitFlag(_QSPI_SR_POS_BUSY, false, tickstart)) {
			state = QSPIState::Error;
			return false;
		}
		state = QSPIState::Ready;
		return true;
	}

	// AKA HAL_QSPI_Abort_IT
	bool QSPI::AbortRupt() {
		if (!((stduint)state & 0x2)) return true;
		state = QSPIState::Abort;
		// disable all interrupts
		self[QSPIReg::CR].rstof(_QSPI_CR_POS_TOIE);
		self[QSPIReg::CR].rstof(_QSPI_CR_POS_SMIE);
		self[QSPIReg::CR].rstof(_QSPI_CR_POS_FTIE);
		self[QSPIReg::CR].rstof(_QSPI_CR_POS_TCIE);
		self[QSPIReg::CR].rstof(_QSPI_CR_POS_TEIE);
		if (self[QSPIReg::CR].bitof(_QSPI_CR_POS_DMAEN)) {
			self[QSPIReg::CR].rstof(_QSPI_CR_POS_DMAEN);
			if (hmdma) {
				MDMA.XferAbortCallback[hmdma->getID()] = &QSPI::MDMAAbortCplt;
				hmdma->AbortRupt();
			}
		} else {
			self[QSPIReg::FCR].setof(_QSPI_FCR_POS_CTCF);
			self[QSPIReg::CR].setof(_QSPI_CR_POS_TCIE);
			self[QSPIReg::CR].setof(_QSPI_CR_POS_ABORT);
		}
		return true;
	}

	// AKA HAL_QSPI_SetFifoThreshold
	bool QSPI::setFifoThreshold(stduint threshold) {
		if (state != QSPIState::Ready) return false;
		if (threshold < 1 || threshold > _QSPI_FIFO_THRESHOLD_MAX) return false;
		init.fifo_threshold = threshold;
		self[QSPIReg::CR].maset(_QSPI_CR_POS_FTHRES, 4, threshold - 1);
		return true;
	}

	// AKA HAL_QSPI_GetFifoThreshold
	stduint QSPI::getFifoThreshold() {
		return self[QSPIReg::CR].masof(_QSPI_CR_POS_FTHRES, 4) + 1;
	}

	// ---- RuptTrait (NVIC + IRQ_QUADSPI) ----
	void QSPI::setInterrupt(Handler_t f) const {
		FUNC_QSPI[0] = f;
	}
	void QSPI::setInterruptPriority(byte preempt, byte sub_priority) const {
		NVIC.setPriority(IRQ_QUADSPI, preempt, sub_priority);
	}
	void QSPI::enInterrupt(bool enable) const {
		NVIC.setAble(IRQ_QUADSPI, enable);
	}

#endif // _MCU_STM32H7x
}
