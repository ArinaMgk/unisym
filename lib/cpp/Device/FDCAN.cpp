// ASCII CPL TAB4 CRLF
// Docutitle: [Device] Flexible Data-Rate CAN
// Developer: @dosconio
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

#include "../../../inc/c/driver/FDCAN.h"
#include "../../../inc/cpp/Device/SysTick"
#if defined(_MCU_STM32H7x)
namespace uni {

	FDCAN_t FDCAN1(FDCAN1_BASE), FDCAN2(FDCAN2_BASE);

	// AKA HAL FDCAN_TIMEOUT_VALUE
	static constexpr uint32 _FDCAN_TIMEOUT_VALUE = 10;
	// AKA HAL CvtEltSize[]: data field size code -> number of 32-bit words
	static const byte _FdcanCvtEltSize[19] = { 0, 0, 0, 0, 0, 1, 2, 3, 4, 0, 5, 0, 0, 0, 6, 0, 0, 0, 7 };
	// DLC table _FdcanDlcToBytes is provided by predef.fdcan.hpp

	// wait for a flag bit in a register, returns true if matched, false on timeout
	// AKA HAL "WaitOnFlagUntilTimeout" loop
	static bool fdcan_waitBit(Reference reg, byte bitpos, bool expected, uint64 tickstart) {
		while (reg.bitof(bitpos) != expected) {
			if ((SysTick::getTick() - tickstart) > _FDCAN_TIMEOUT_VALUE)
				return false;
		}
		return true;
	}
	// position value of a set bit (AKA POSITION_VAL)
	static stduint fdcan_posval(stduint x) {
		stduint r = 0;
		if (!x) return 0;
		while (x >>= 1) r++;
		return r;
	}

	// ---- A. lifecycle ----

	// AKA HAL_FDCAN_Init
	bool FDCAN_t::setMode() {
		// init low level hardware (AKA MspInit): enable clock
		if (state == FdcanState::Reset) {
			if (!enClock(true)) { status |= ERR_FDCAN_PARAM; state = FdcanState::Error; return false; }
		}
		// Exit from sleep mode
		self[FdcanReg::CCCR].setof(_FDCAN_CCCR_POSI_CSR, false);
		{
			uint64 tickstart = SysTick::getTick();
			if (!fdcan_waitBit(self[FdcanReg::CCCR], _FDCAN_CCCR_POSI_CSA, false, tickstart)) {
				status |= ERR_FDCAN_TIMEOUT; state = FdcanState::Error; return false;
			}
		}
		// Request initialisation
		self[FdcanReg::CCCR].setof(_FDCAN_CCCR_POSI_INIT, true);
		{
			uint64 tickstart = SysTick::getTick();
			if (!fdcan_waitBit(self[FdcanReg::CCCR], _FDCAN_CCCR_POSI_INIT, true, tickstart)) {
				status |= ERR_FDCAN_TIMEOUT; state = FdcanState::Error; return false;
			}
		}
		// Enable configuration change
		self[FdcanReg::CCCR].setof(_FDCAN_CCCR_POSI_CCE, true);

		// Auto retransmission (DAR)
		self[FdcanReg::CCCR].setof(_FDCAN_CCCR_POSI_DAR, !AutoRetransmission);
		// Transmit pause (TXP)
		self[FdcanReg::CCCR].setof(_FDCAN_CCCR_POSI_TXP, TransmitPause);
		// Protocol exception handling (PXHD)
		self[FdcanReg::CCCR].setof(_FDCAN_CCCR_POSI_PXHD, !ProtocolException);
		// Frame format (FDOE / BRSE)
		self[FdcanReg::CCCR].maset(_FDCAN_CCCR_POSI_FDOE, 2, (stduint)FrameFormat >> _FDCAN_CCCR_POSI_FDOE);

		// Operating mode
		if (Mode == FdcanMode::RestrictedOperation) {
			self[FdcanReg::CCCR].setof(_FDCAN_CCCR_POSI_ASM, true);
		}
		else if (Mode != FdcanMode::Normal) {
			if (Mode != FdcanMode::BusMonitoring) {
				self[FdcanReg::CCCR].setof(_FDCAN_CCCR_POSI_TEST, true);
				self[FdcanReg::TEST].setof(_FDCAN_TEST_POSI_LBCK, true);
				if (Mode == FdcanMode::InternalLoopback)
					self[FdcanReg::CCCR].setof(_FDCAN_CCCR_POSI_MON, true);
			}
			else {
				self[FdcanReg::CCCR].setof(_FDCAN_CCCR_POSI_MON, true);
			}
		}

		// Nominal bit timing
		self[FdcanReg::NBTP] = (((NominalSyncJumpWidth - 1) << _FDCAN_NBTP_POSI_NSJW) |
			((NominalTimeSeg1 - 1) << _FDCAN_NBTP_POSI_NTSEG1) |
			(NominalTimeSeg2 - 1) |
			((NominalPrescaler - 1) << _FDCAN_NBTP_POSI_NBRP));
		// Data bit timing (FD with BRS)
		if (FrameFormat == FdcanFrameFormat::FdBrs) {
			self[FdcanReg::DBTP] = ((DataSyncJumpWidth - 1) |
				((DataTimeSeg1 - 1) << _FDCAN_DBTP_POSI_DTSEG1) |
				((DataTimeSeg2 - 1) << _FDCAN_DBTP_POSI_DTSEG2) |
				((DataPrescaler - 1) << _FDCAN_DBTP_POSI_DBRP));
		}

		// Tx FIFO/Queue mode
		if (TxFifoQueueElmtsNbr > 0) {
			self[FdcanReg::TXBC].setof(_FDCAN_TXBC_POSI_TFQM, (stduint)TxFifoQueueMode != 0);
		}
		// Tx element size
		if ((TxBuffersNbr + TxFifoQueueElmtsNbr) > 0) {
			self[FdcanReg::TXESC].maset(_FDCAN_TXESC_POSI_TBDS, 3, _FdcanCvtEltSize[(byte)TxElmtSize]);
		}
		// Rx FIFO 0 element size
		if (RxFifo0ElmtsNbr > 0) {
			self[FdcanReg::RXESC].maset(_FDCAN_RXESC_POSI_F0DS, 3, _FdcanCvtEltSize[(byte)RxFifo0ElmtSize]);
		}
		// Rx FIFO 1 element size
		if (RxFifo1ElmtsNbr > 0) {
			self[FdcanReg::RXESC].maset(_FDCAN_RXESC_POSI_F1DS, 3, _FdcanCvtEltSize[(byte)RxFifo1ElmtSize]);
		}
		// Rx buffer element size
		if (RxBuffersNbr > 0) {
			self[FdcanReg::RXESC].maset(_FDCAN_RXESC_POSI_RBDS, 3, _FdcanCvtEltSize[(byte)RxBufferSize]);
		}
		// By default event-driven communication (TT disabled); TT_ConfigOperation switches to TT
		if (baseaddr == FDCAN1_BASE) {
			self[FdcanReg::TTOCF].setof(_FDCAN_TTOCF_POSI_OM, false);
		}

		// Message RAM block addresses
		if (!calcRamBlockAddresses()) return false;

		status = ERR_FDCAN_NONE;
		state = FdcanState::Ready;
		return true;
	}

	// AKA HAL_FDCAN_DeInit
	bool FDCAN_t::canMode() {
		stop();
		enClock(false);
		status = ERR_FDCAN_NONE;
		state = FdcanState::Reset;
		return true;
	}

	// AKA HAL_FDCAN_Start
	bool FDCAN_t::start() {
		if (state == FdcanState::Ready) {
			state = FdcanState::Busy;
			self[FdcanReg::CCCR].setof(_FDCAN_CCCR_POSI_INIT, false); // request leave initialisation
			status = ERR_FDCAN_NONE;
			return true;
		}
		status |= ERR_FDCAN_NOT_READY;
		return false;
	}

	// AKA HAL_FDCAN_Stop
	bool FDCAN_t::stop() {
		if (state == FdcanState::Busy) {
			self[FdcanReg::CCCR].setof(_FDCAN_CCCR_POSI_INIT, true); // request initialisation
			{
				uint64 tickstart = SysTick::getTick();
				if (!fdcan_waitBit(self[FdcanReg::CCCR], _FDCAN_CCCR_POSI_INIT, true, tickstart)) {
					status |= ERR_FDCAN_TIMEOUT; state = FdcanState::Error; return false;
				}
			}
			self[FdcanReg::CCCR].setof(_FDCAN_CCCR_POSI_CCE, true);
			state = FdcanState::Ready;
			return true;
		}
		status |= ERR_FDCAN_NOT_STARTED;
		return false;
	}

	// AKA HAL_FDCAN_EnterPowerDownMode / ExitPowerDownMode
	bool FDCAN_t::enSleep(bool ena) {
		uint64 tickstart = SysTick::getTick();
		if (ena) {
			self[FdcanReg::CCCR].setof(_FDCAN_CCCR_POSI_CSR, true);
			if (!fdcan_waitBit(self[FdcanReg::CCCR], _FDCAN_CCCR_POSI_CSA, true, tickstart)) {
				status |= ERR_FDCAN_TIMEOUT; state = FdcanState::Error; return false;
			}
		}
		else {
			self[FdcanReg::CCCR].setof(_FDCAN_CCCR_POSI_CSR, false);
			if (!fdcan_waitBit(self[FdcanReg::CCCR], _FDCAN_CCCR_POSI_CSA, false, tickstart)) {
				status |= ERR_FDCAN_TIMEOUT; state = FdcanState::Error; return false;
			}
			self[FdcanReg::CCCR].setof(_FDCAN_CCCR_POSI_INIT, false);
		}
		return true;
	}

	// FDCAN kernel clock enable (H7: APB1HENR.FDCANEN, shared by FDCAN1/2)
	bool FDCAN_t::enClock(bool ena) {
		RCC[RCCReg::APB1HENR].setof(_RCC_APB1HENR_POSI_FDCANEN, ena);
		return RCC[RCCReg::APB1HENR].bitof(_RCC_APB1HENR_POSI_FDCANEN) == ena;
	}

	// ---- B. CCU clock calibration (FDCAN_CCU is a global single unit, config via FDCAN1 only) ----

	// AKA HAL_FDCAN_ConfigClockCalibration
	bool FDCAN_t::setClockCalibration(const FdcanClkCalConfig& cfg) {
		if (baseaddr != FDCAN1_BASE) {
			status |= ERR_FDCAN_NOT_INITIALIZED;
			return false;
		}
		if (state != FdcanState::Ready) {
			status |= ERR_FDCAN_NOT_READY;
			return false;
		}
		if (!cfg.ClockCalibration) {
			// Bypass clock calibration: set BCC, configure divider
			ccu(FdcanCcuReg::CCFG).setof(_FDCANCCU_CCFG_POSI_BCC, true);
			ccu(FdcanCcuReg::CCFG).maset(_FDCANCCU_CCFG_POSI_CDIV, 4, cfg.ClockDivider);
		}
		else {
			ccu(FdcanCcuReg::CCFG).setof(_FDCANCCU_CCFG_POSI_BCC, false);
			ccu(FdcanCcuReg::CCFG).maset(_FDCANCCU_CCFG_POSI_TQBT, 6, cfg.TimeQuantaPerBitTime);
			ccu(FdcanCcuReg::CCFG).setof(_FDCANCCU_CCFG_POSI_CFL, cfg.CalFieldLength != 0);
			ccu(FdcanCcuReg::CCFG).maset(_FDCANCCU_CCFG_POSI_OCPM, 8, cfg.MinOscClkPeriods);
			ccu(FdcanCcuReg::CWD).maset(_FDCANCCU_CWD_POSI_WDC, 16, cfg.WatchdogStartValue);
		}
		return true;
	}

	// AKA HAL_FDCAN_GetClockCalibrationState
	stduint FDCAN_t::getClockCalibrationState() const {
		return ccu(FdcanCcuReg::CSTAT).mask(_FDCANCCU_CSTAT_POSI_CALS, 2);
	}

	// AKA HAL_FDCAN_ResetClockCalibrationState
	bool FDCAN_t::resetClockCalibration() {
		ccu(FdcanCcuReg::CCFG).setof(_FDCANCCU_CCFG_POSI_SWR, true);
		return true;
	}

	// AKA HAL_FDCAN_GetClockCalibrationCounter
	// counter: 0 = time quanta, 1 = clock period, 2 = watchdog
	stduint FDCAN_t::getClockCalibrationCounter(stduint counter) const {
		if (counter == 0) return ccu(FdcanCcuReg::CSTAT).masof(_FDCANCCU_CSTAT_POSI_TQC, 12);
		if (counter == 1) return ccu(FdcanCcuReg::CSTAT).masof(_FDCANCCU_CSTAT_POSI_OCPC, 18);
		return ccu(FdcanCcuReg::CWD).masof(_FDCANCCU_CWD_POSI_WDV, 16);
	}

	// CCU interrupt (IRQ_FDCAN_CAL), AKA NVIC enable for the calibration line
	bool FDCAN_t::enCalibrationRupt(bool ena) {
		NVIC.setAble(IRQ_FDCAN_CAL, ena);
		return true;
	}

	// ---- C. filters & message RAM config ----

	// AKA HAL_FDCAN_ConfigFilter
	bool FDCAN_t::setFilter(const FdcanFilter& cfg) {
		if ((state != FdcanState::Ready) && (state != FdcanState::Busy)) {
			status |= ERR_FDCAN_NOT_INITIALIZED;
			return false;
		}
		stduint w1, w2;
		if (cfg.IdType == FdcanIdType::Standard) {
			if (cfg.FilterConfig == FdcanFilterConfig::ToRxBuffer) {
				w1 = ((stduint)FdcanFilterConfig::ToRxBuffer << 27) |
					(cfg.FilterID1 << 16) |
					((stduint)cfg.IsCalibrationMsg << 8) |
					cfg.RxBufferIndex;
			}
			else {
				w1 = ((stduint)cfg.FilterType << 30) |
					((stduint)cfg.FilterConfig << 27) |
					(cfg.FilterID1 << 16) |
					cfg.FilterID2;
			}
			*(stduint*)(StandardFilterSA + (cfg.FilterIndex * 4)) = w1;
		}
		else { // Extended
			w1 = ((stduint)cfg.FilterConfig << 29) | cfg.FilterID1;
			w2 = (cfg.FilterConfig == FdcanFilterConfig::ToRxBuffer) ?
				cfg.RxBufferIndex :
				(((stduint)cfg.FilterType << 30) | cfg.FilterID2);
			stduint* addr = (stduint*)(ExtendedFilterSA + (cfg.FilterIndex * 8));
			addr[0] = w1;
			addr[1] = w2;
		}
		return true;
	}

	// AKA HAL_FDCAN_ConfigGlobalFilter
	bool FDCAN_t::setGlobalFilter(stduint non_matching_std, stduint non_matching_ext, bool reject_remote_std, bool reject_remote_ext) {
		if (state != FdcanState::Ready) { status |= ERR_FDCAN_NOT_READY; return false; }
		self[FdcanReg::GFC] = (non_matching_std << 4) | (non_matching_ext << 2) |
			((stduint)reject_remote_std << 1) | (stduint)reject_remote_ext;
		return true;
	}

	// AKA HAL_FDCAN_ConfigExtendedIdMask
	bool FDCAN_t::setExtIdMask(stduint mask) {
		if (state != FdcanState::Ready) { status |= ERR_FDCAN_NOT_READY; return false; }
		self[FdcanReg::XIDAM] = mask;
		return true;
	}

	// AKA HAL_FDCAN_ConfigRxFifoOverwrite
	bool FDCAN_t::setRxFifoOverwrite(stduint rxfifo, stduint operation_mode) {
		if (state != FdcanState::Ready) { status |= ERR_FDCAN_NOT_READY; return false; }
		if (rxfifo == 0) self[FdcanReg::RXF0C].setof(_FDCAN_RXF0C_POSI_F0OM, operation_mode != 0);
		else self[FdcanReg::RXF1C].setof(_FDCAN_RXF1C_POSI_F1OM, operation_mode != 0);
		return true;
	}

	// AKA HAL_FDCAN_ConfigFifoWatermark
	// fifo: 0 = Rx FIFO 0, 1 = Rx FIFO 1, 2 = Tx Event FIFO
	bool FDCAN_t::setFifoWatermark(stduint fifo, stduint watermark) {
		if (state != FdcanState::Ready) { status |= ERR_FDCAN_NOT_READY; return false; }
		if (fifo == 2) self[FdcanReg::TXEFC].maset(_FDCAN_TXEFC_POSI_EFWM, 5, watermark);
		else if (fifo == 0) self[FdcanReg::RXF0C].maset(_FDCAN_RXF0C_POSI_F0WM, 6, watermark);
		else self[FdcanReg::RXF1C].maset(_FDCAN_RXF1C_POSI_F1WM, 6, watermark);
		return true;
	}

	// AKA HAL_FDCAN_ConfigRamWatchdog
	bool FDCAN_t::setRamWatchdog(stduint counter_start_value) {
		if (state != FdcanState::Ready) { status |= ERR_FDCAN_NOT_READY; return false; }
		self[FdcanReg::RWD].maset(_FDCAN_RWD_POSI_WDC, 8, counter_start_value);
		return true;
	}

	// ---- D. timestamp / timeout / Tx delay compensation ----

	// AKA HAL_FDCAN_ConfigTimestampCounter
	bool FDCAN_t::setTimestampCounter(stduint prescaler) {
		if (state != FdcanState::Ready) { status |= ERR_FDCAN_NOT_READY; return false; }
		self[FdcanReg::TSCC].maset(_FDCAN_TSCC_POSI_TCP, 16, prescaler);
		return true;
	}
	// AKA HAL_FDCAN_EnableTimestampCounter / DisableTimestampCounter
	bool FDCAN_t::enTimestampCounter(bool ena, stduint operation) {
		if (state != FdcanState::Ready) { status |= ERR_FDCAN_NOT_READY; return false; }
		if (ena) self[FdcanReg::TSCC].maset(_FDCAN_TSCC_POSI_TSS, 2, operation);
		else self[FdcanReg::TSCC].setof(_FDCAN_TSCC_POSI_TSS, false);
		return true;
	}
	// AKA HAL_FDCAN_ResetTimestampCounter
	bool FDCAN_t::resetTimestampCounter() {
		if (self[FdcanReg::TSCC].masof(_FDCAN_TSCC_POSI_TSS, 2) == 2 /* external timestamp */) {
			status |= ERR_FDCAN_NOT_SUPPORTED;
			return false;
		}
		self[FdcanReg::TSCV] = 0;
		return true;
	}
	// AKA HAL_FDCAN_GetTimestampCounter
	stduint FDCAN_t::getTimestampCounter() const {
		return self[FdcanReg::TSCV] & 0xFFFF;
	}
	// AKA HAL_FDCAN_ConfigTimeoutCounter
	bool FDCAN_t::setTimeoutCounter(stduint operation, stduint period) {
		if (state != FdcanState::Ready) { status |= ERR_FDCAN_NOT_READY; return false; }
		self[FdcanReg::TOCC].maset(_FDCAN_TOCC_POSI_TOS, 2, operation);
		self[FdcanReg::TOCC].maset(_FDCAN_TOCC_POSI_TOP, 16, period);
		return true;
	}
	// AKA HAL_FDCAN_EnableTimeoutCounter / DisableTimeoutCounter
	bool FDCAN_t::enTimeoutCounter(bool ena) {
		if (state != FdcanState::Ready) { status |= ERR_FDCAN_NOT_READY; return false; }
		self[FdcanReg::TOCC].setof(_FDCAN_TOCC_POSI_ETOC, ena);
		return true;
	}
	// AKA HAL_FDCAN_ResetTimeoutCounter
	bool FDCAN_t::resetTimeoutCounter() {
		if (self[FdcanReg::TOCC].masof(_FDCAN_TOCC_POSI_TOS, 2) == 0 /* continuous mode */) {
			status |= ERR_FDCAN_NOT_SUPPORTED;
			return false;
		}
		self[FdcanReg::TOCV] = 0;
		return true;
	}
	// AKA HAL_FDCAN_GetTimeoutCounter
	stduint FDCAN_t::getTimeoutCounter() const {
		return self[FdcanReg::TOCV] & 0xFFFF;
	}
	// AKA HAL_FDCAN_ConfigTxDelayCompensation
	bool FDCAN_t::setTxDelayCompensation(stduint tdc_offset, stduint tdc_filter) {
		if (state != FdcanState::Ready) { status |= ERR_FDCAN_NOT_READY; return false; }
		self[FdcanReg::TDCR] = tdc_filter | (tdc_offset << 8);
		return true;
	}
	// AKA HAL_FDCAN_EnableTxDelayCompensation / DisableTxDelayCompensation
	bool FDCAN_t::enTxDelayCompensation(bool ena) {
		if (state != FdcanState::Ready) { status |= ERR_FDCAN_NOT_READY; return false; }
		self[FdcanReg::DBTP].setof(_FDCAN_DBTP_POSI_TDC, ena);
		return true;
	}

	// ---- E. transmit ----

	// AKA HAL_FDCAN_AddMessageToTxFifoQ
	bool FDCAN_t::Transmit(const FdcanTxHeader& header, const byte* data) {
		if (state != FdcanState::Busy) { status |= ERR_FDCAN_NOT_STARTED; return false; }
		// Tx FIFO/Queue must have an allocated area in the RAM (TXBC.TFQS != 0)
		if (self[FdcanReg::TXBC].masof(_FDCAN_TXBC_POSI_TFQS, 8) == 0) {
			status |= ERR_FDCAN_PARAM;
			return false;
		}
		// Tx FIFO/Queue must not be full
		if (self[FdcanReg::TXFQS].bitof(_FDCAN_TXFQS_POSI_TFQF)) {
			status |= ERR_FDCAN_PARAM;
			return false;
		}
		stduint put_index = self[FdcanReg::TXFQS].masof(_FDCAN_TXFQS_POSI_TFQPI, 5);
		if (!copyMessageToRAM(header, data, put_index)) return false;
		self[FdcanReg::TXBAR] = (1 << put_index);
		return true;
	}

	// AKA HAL_FDCAN_AddMessageToTxBuffer
	bool FDCAN_t::Transmit(const FdcanTxHeader& header, const byte* data, stduint buffer_index) {
		if ((state != FdcanState::Ready) && (state != FdcanState::Busy)) {
			status |= ERR_FDCAN_NOT_INITIALIZED;
			return false;
		}
		stduint pos = fdcan_posval(buffer_index);
		// buffer must have an allocated area in the RAM
		if (pos >= self[FdcanReg::TXBC].masof(_FDCAN_TXBC_POSI_NDTB, 8)) {
			status |= ERR_FDCAN_PARAM;
			return false;
		}
		// no transmission request pending for the selected buffer
		if ((self[FdcanReg::TXBRP] & buffer_index) != 0) {
			status |= ERR_FDCAN_PENDING;
			return false;
		}
		return copyMessageToRAM(header, data, pos);
	}

	// AKA HAL_FDCAN_EnableTxBufferRequest
	bool FDCAN_t::requestTxBuffer(stduint buffer_index) {
		if (state != FdcanState::Busy) { status |= ERR_FDCAN_NOT_STARTED; return false; }
		self[FdcanReg::TXBAR] = buffer_index;
		return true;
	}

	// AKA HAL_FDCAN_AbortTxRequest
	bool FDCAN_t::abortTxRequest(stduint buffer_index) {
		if (state != FdcanState::Busy) { status |= ERR_FDCAN_NOT_STARTED; return false; }
		self[FdcanReg::TXBCR] = buffer_index;
		return true;
	}

	// ---- F. receive & status ----

	// AKA HAL_FDCAN_GetRxMessage
	bool FDCAN_t::Receive(FdcanRxLocation location, FdcanRxHeader& header, byte* data) {
		if ((state != FdcanState::Ready) && (state != FdcanState::Busy)) {
			status |= ERR_FDCAN_NOT_INITIALIZED;
			return false;
		}
		stduint* addr;
		stduint get_index = 0;
		if (location == RxFifo0) {
			if (self[FdcanReg::RXF0C].masof(_FDCAN_RXF0C_POSI_F0S, 8) == 0) { status |= ERR_FDCAN_PARAM; return false; }
			if (self[FdcanReg::RXF0S].masof(_FDCAN_RXF0S_POSI_F0FL, 7) == 0) { status |= ERR_FDCAN_PARAM; return false; }
			get_index = self[FdcanReg::RXF0S].masof(_FDCAN_RXF0S_POSI_F0GI, 6);
			addr = (stduint*)(RxFIFO0SA + (get_index * (byte)RxFifo0ElmtSize * 4));
		}
		else if (location == RxFifo1) {
			if (self[FdcanReg::RXF1C].masof(_FDCAN_RXF1C_POSI_F1S, 8) == 0) { status |= ERR_FDCAN_PARAM; return false; }
			if (self[FdcanReg::RXF1S].masof(_FDCAN_RXF1S_POSI_F1FL, 7) == 0) { status |= ERR_FDCAN_PARAM; return false; }
			get_index = self[FdcanReg::RXF1S].masof(_FDCAN_RXF1S_POSI_F1GI, 6);
			addr = (stduint*)(RxFIFO1SA + (get_index * (byte)RxFifo1ElmtSize * 4));
		}
		else { // dedicated Rx buffer
			if ((stduint)location >= RxBuffersNbr) { status |= ERR_FDCAN_PARAM; return false; }
			addr = (stduint*)(RxBufferSA + ((stduint)location - RxBuffer0) * (byte)RxBufferSize * 4);
		}
		// element word 1
		header.IdType = (addr[0] & _FDCAN_ELM_MASK_XTD) ? FdcanIdType::Extended : FdcanIdType::Standard;
		header.Identifier = (header.IdType == FdcanIdType::Standard) ?
			((addr[0] & _FDCAN_ELM_MASK_STDID) >> 18) : (addr[0] & _FDCAN_ELM_MASK_EXTID);
		header.RxFrameType = (addr[0] & _FDCAN_ELM_MASK_RTR) ? FdcanFrameType::Remote : FdcanFrameType::Data;
		header.ErrorStateIndicator = (addr[0] & _FDCAN_ELM_MASK_ESI) != 0;
		// element word 2
		header.RxTimestamp = addr[1] & _FDCAN_ELM_MASK_TS;
		header.DataLength = (FdcanDlc)(byte)((addr[1] & _FDCAN_ELM_MASK_DLC) >> 16);
		header.BitRateSwitch = (addr[1] & _FDCAN_ELM_MASK_BRS) != 0;
		header.FDFormat = (addr[1] & _FDCAN_ELM_MASK_FDF) != 0;
		header.FilterIndex = (addr[1] & _FDCAN_ELM_MASK_FIDX) >> 24;
		header.IsFilterMatchingFrame = (addr[1] & _FDCAN_ELM_MASK_ANMF) != 0;
		// payload (little-endian words after the two header words)
		{
			const byte* src = (const byte*)(addr + 2);
			stduint nbytes = _FdcanDlcToBytes[(byte)header.DataLength];
			for (stduint i = 0; i < nbytes; i++) data[i] = src[i];
		}
		// acknowledge
		if (location == RxFifo0) self[FdcanReg::RXF0A] = get_index;
		else if (location == RxFifo1) self[FdcanReg::RXF1A] = get_index;
		else {
			stduint idx = (stduint)location - RxBuffer0;
			if (idx < 32) self[FdcanReg::NDAT1] = (1 << idx);
			else self[FdcanReg::NDAT2] = (1 << (idx - 32));
		}
		return true;
	}

	// AKA HAL_FDCAN_GetTxEvent
	bool FDCAN_t::getTxEvent(FdcanTxEvent& ev) {
		if ((state != FdcanState::Ready) && (state != FdcanState::Busy)) {
			status |= ERR_FDCAN_NOT_INITIALIZED;
			return false;
		}
		stduint get_index = self[FdcanReg::TXEFS].masof(_FDCAN_TXEFS_POSI_EFGI, 5);
		stduint* addr = (stduint*)(TxEventFIFOSA + (get_index * 2 * 4));
		ev.IdType = (addr[0] & _FDCAN_ELM_MASK_XTD) ? FdcanIdType::Extended : FdcanIdType::Standard;
		ev.Identifier = (ev.IdType == FdcanIdType::Standard) ?
			((addr[0] & _FDCAN_ELM_MASK_STDID) >> 18) : (addr[0] & _FDCAN_ELM_MASK_EXTID);
		ev.TxFrameType = (addr[0] & _FDCAN_ELM_MASK_RTR) ? FdcanFrameType::Remote : FdcanFrameType::Data;
		ev.ErrorStateIndicator = (addr[0] & _FDCAN_ELM_MASK_ESI) != 0;
		ev.TxTimestamp = addr[1] & _FDCAN_ELM_MASK_TS;
		ev.DataLength = (FdcanDlc)(byte)((addr[1] & _FDCAN_ELM_MASK_DLC) >> 16);
		ev.BitRateSwitch = (addr[1] & _FDCAN_ELM_MASK_BRS) != 0;
		ev.FDFormat = (addr[1] & _FDCAN_ELM_MASK_FDF) != 0;
		ev.EventType = (addr[1] & _FDCAN_ELM_MASK_ET) >> 22;
		ev.MessageMarker = (byte)((addr[1] & _FDCAN_ELM_MASK_MM) >> 24);
		self[FdcanReg::TXEFA] = get_index;
		return true;
	}

	// AKA HAL_FDCAN_GetHighPriorityMessageStatus
	bool FDCAN_t::getHpMsgStatus(FdcanHpMsgStatus& st) {
		st.FilterList = self[FdcanReg::HPMS].bitof(_FDCAN_HPMS_POSI_FLST);
		st.FilterIndex = self[FdcanReg::HPMS].masof(_FDCAN_HPMS_POSI_FIDX, 7);
		st.MessageStorage = self[FdcanReg::HPMS].masof(_FDCAN_HPMS_POSI_MSI, 2);
		st.BufferIndex = self[FdcanReg::HPMS].masof(_FDCAN_HPMS_POSI_BIDX, 6);
		return true;
	}

	// AKA HAL_FDCAN_GetProtocolStatus
	bool FDCAN_t::getProtocolStatus(FdcanProtocolStatus& st) {
		stduint reg = self[FdcanReg::PSR];
		st.LastErrorCode = reg & 0x7;
		st.DataLastErrorCode = (reg >> _FDCAN_PSR_POSI_DLEC) & 0x7;
		st.Activity = (reg >> _FDCAN_PSR_POSI_ACT) & 0x3;
		st.ErrorPassive = (reg >> _FDCAN_PSR_POSI_EP) & 1;
		st.ErrorWarning = (reg >> _FDCAN_PSR_POSI_EW) & 1;
		st.BusOff = (reg >> _FDCAN_PSR_POSI_BO) & 1;
		st.Rxesi = (reg >> _FDCAN_PSR_POSI_RESI) & 1;
		st.Rxbrs = (reg >> _FDCAN_PSR_POSI_RBRS) & 1;
		st.Rxdlc = (reg >> _FDCAN_PSR_POSI_REDL) & 1;
		st.ProtocolException = (reg >> _FDCAN_PSR_POSI_PXE) & 1;
		st.Tdcv = (reg >> _FDCAN_PSR_POSI_TDCV) & 0x7F;
		return true;
	}

	// AKA HAL_FDCAN_GetErrorCounters
	bool FDCAN_t::getErrorCounters(FdcanErrorCounters& ec) {
		stduint reg = self[FdcanReg::ECR];
		ec.TxErrorCounter = reg & 0xFF;
		ec.RxErrorCounter = (reg >> _FDCAN_ECR_POSI_REC) & 0xFF;
		ec.ErrorPassive = (reg >> _FDCAN_ECR_POSI_RP) & 1;
		ec.ErrorLoggingCounter = (reg >> _FDCAN_ECR_POSI_CEL) & 0xFF;
		return true;
	}

	// AKA HAL_FDCAN_IsRxBufferMessageAvailable (also clears the new-data flag)
	bool FDCAN_t::isRxBufferAvailable(stduint rx_buffer_index) const {
		FDCAN_t* self_p = const_cast<FDCAN_t*>(this);
		if (rx_buffer_index < 32) {
			if ((self[FdcanReg::NDAT1] & (1 << rx_buffer_index)) == 0) return false;
			self_p->operator[](FdcanReg::NDAT1) = (1 << rx_buffer_index);
		}
		else {
			stduint idx = rx_buffer_index - 32;
			if ((self[FdcanReg::NDAT2] & (1 << idx)) == 0) return false;
			self_p->operator[](FdcanReg::NDAT2) = (1 << idx);
		}
		return true;
	}

	// AKA HAL_FDCAN_IsTxBufferMessagePending
	bool FDCAN_t::isTxBufferPending(stduint tx_buffer_index) const {
		return (self[FdcanReg::TXBRP] & tx_buffer_index) != 0;
	}

	// AKA HAL_FDCAN_GetRxFifoFillLevel
	stduint FDCAN_t::getRxFifoFillLevel(stduint rxfifo) const {
		if (rxfifo == 0) return self[FdcanReg::RXF0S].masof(_FDCAN_RXF0S_POSI_F0FL, 7);
		return self[FdcanReg::RXF1S].masof(_FDCAN_RXF1S_POSI_F1FL, 7);
	}

	// AKA HAL_FDCAN_GetTxFifoFreeLevel
	stduint FDCAN_t::getTxFifoFreeLevel() const {
		return self[FdcanReg::TXFQS].masof(_FDCAN_TXFQS_POSI_TFFL, 6);
	}

	// AKA HAL_FDCAN_IsRestrictedOperationMode
	bool FDCAN_t::isRestrictedMode() const {
		return self[FdcanReg::CCCR].bitof(_FDCAN_CCCR_POSI_ASM);
	}

	// AKA HAL_FDCAN_ExitRestrictedOperationMode
	bool FDCAN_t::exitRestrictedMode() {
		if ((state != FdcanState::Ready) && (state != FdcanState::Busy)) {
			status |= ERR_FDCAN_NOT_INITIALIZED;
			return false;
		}
		self[FdcanReg::CCCR].setof(_FDCAN_CCCR_POSI_ASM, false);
		return true;
	}

	// ---- G. interrupts ----

	// RuptTrait: register the Rx FIFO 0 callback (most common receive path)
	void FDCAN_t::setInterrupt(Handler_t f) const {
		const_cast<FDCAN_t*>(this)->rx_fifo0_handler = f;
	}
	void FDCAN_t::setInterruptPriority(byte preempt, byte sub_priority) const {
		Request_t req0 = (baseaddr == FDCAN1_BASE) ? IRQ_FDCAN1_IT0 : IRQ_FDCAN2_IT0;
		Request_t req1 = (baseaddr == FDCAN1_BASE) ? IRQ_FDCAN1_IT1 : IRQ_FDCAN2_IT1;
		NVIC.setPriority(req0, preempt, sub_priority);
		NVIC.setPriority(req1, preempt, sub_priority);
	}
	void FDCAN_t::enInterrupt(bool enable) const {
		Request_t req0 = (baseaddr == FDCAN1_BASE) ? IRQ_FDCAN1_IT0 : IRQ_FDCAN2_IT0;
		Request_t req1 = (baseaddr == FDCAN1_BASE) ? IRQ_FDCAN1_IT1 : IRQ_FDCAN2_IT1;
		NVIC.setAble(req0, enable);
		NVIC.setAble(req1, enable);
	}

	// AKA HAL_FDCAN_ConfigInterruptLines (select IT0 or IT1 for a set of interrupt sources)
	bool FDCAN_t::setInterruptLine(stduint it_list, stduint line) {
		if ((state != FdcanState::Ready) && (state != FdcanState::Busy)) {
			status |= ERR_FDCAN_NOT_INITIALIZED;
			return false;
		}
		if (line == 0) self[FdcanReg::ILS] &= ~it_list;
		else self[FdcanReg::ILS] |= it_list;
		return true;
	}

	// AKA HAL_FDCAN_TT_ConfigInterruptLines (TTILS, FDCAN1 only)
	bool FDCAN_t::TT_setInterruptLine(stduint it_list, stduint line) {
		if (baseaddr != FDCAN1_BASE) { status |= ERR_FDCAN_NOT_SUPPORTED; return false; }
		if ((state != FdcanState::Ready) && (state != FdcanState::Busy)) {
			status |= ERR_FDCAN_NOT_INITIALIZED;
			return false;
		}
		if (line == 0) self[FdcanReg::TTILS] &= ~it_list;
		else self[FdcanReg::TTILS] |= it_list;
		return true;
	}

	// AKA HAL_FDCAN_ActivateNotification / DeactivateNotification
	bool FDCAN_t::enNotification(stduint its, stduint buffer_indexes, bool ena) {
		if ((state != FdcanState::Ready) && (state != FdcanState::Busy)) {
			status |= ERR_FDCAN_NOT_INITIALIZED;
			return false;
		}
		if (ena) {
			// enable the selected interrupt sources
			self[FdcanReg::IE] |= its;
			// enable the corresponding interrupt line(s): all on line 0 / all on line 1 / mixed
			if ((its & self[FdcanReg::ILS]) == 0)
				self[FdcanReg::ILE].setof(_FDCAN_ILE_POSI_EINT0, true);
			else if ((its & self[FdcanReg::ILS]) == its)
				self[FdcanReg::ILE].setof(_FDCAN_ILE_POSI_EINT1, true);
			else
				self[FdcanReg::ILE] = (1u << _FDCAN_ILE_POSI_EINT0) | (1u << _FDCAN_ILE_POSI_EINT1);
			// Tx buffer transmission complete / cancellation finished notification
			if (buffer_indexes) {
				self[FdcanReg::TXBTIE] |= buffer_indexes;
				self[FdcanReg::TXBCIE] |= buffer_indexes;
			}
		}
		else {
			self[FdcanReg::IE] &= ~its;
		}
		return true;
	}

	// AKA HAL_FDCAN_TT_ActivateNotification / DeactivateNotification (TTIE, FDCAN1 only)
	bool FDCAN_t::TT_enNotification(stduint its, bool ena) {
		if (baseaddr != FDCAN1_BASE) { status |= ERR_FDCAN_NOT_SUPPORTED; return false; }
		if ((state != FdcanState::Ready) && (state != FdcanState::Busy)) {
			status |= ERR_FDCAN_NOT_INITIALIZED;
			return false;
		}
		if (ena) {
			if ((its & self[FdcanReg::TTILS]) == 0)
				self[FdcanReg::ILE].setof(_FDCAN_ILE_POSI_EINT0, true);
			else if ((its & self[FdcanReg::TTILS]) == its)
				self[FdcanReg::ILE].setof(_FDCAN_ILE_POSI_EINT1, true);
			else
				self[FdcanReg::ILE] = (1u << _FDCAN_ILE_POSI_EINT0) | (1u << _FDCAN_ILE_POSI_EINT1);
			self[FdcanReg::TTIE] |= its;
		}
		else {
			self[FdcanReg::TTIE] &= ~its;
		}
		return true;
	}

	// AKA HAL_FDCAN_IRQHandler: dispatch IR/IE flags to registered callbacks
	void FDCAN_t::irqHandler() {
		// Clock calibration unit interrupts (global CCU, shared by both instances)
		{
			stduint cc_its = (ccu(FdcanCcuReg::IR) << 30) & (ccu(FdcanCcuReg::IE) << 30);
			if (cc_its) {
				ccu(FdcanCcuReg::IE) = ccu(FdcanCcuReg::IE) & ~(cc_its >> 30);
				ccu(FdcanCcuReg::IR) = cc_its >> 30; // write 1 to clear
				irq_flags = cc_its;
				if (cc_calibration_handler) cc_calibration_handler();
			}
		}
		const stduint rf0_mask = (1u << _FDCAN_IR_POSI_RF0N) | (1u << _FDCAN_IR_POSI_RF0W) |
			(1u << _FDCAN_IR_POSI_RF0F) | (1u << _FDCAN_IR_POSI_RF0L);
		const stduint rf1_mask = (1u << _FDCAN_IR_POSI_RF1N) | (1u << _FDCAN_IR_POSI_RF1W) |
			(1u << _FDCAN_IR_POSI_RF1F) | (1u << _FDCAN_IR_POSI_RF1L);
		const stduint tef_mask = (1u << _FDCAN_IR_POSI_TEFN) | (1u << _FDCAN_IR_POSI_TEFW) |
			(1u << _FDCAN_IR_POSI_TEFF) | (1u << _FDCAN_IR_POSI_TEFL);
		const stduint err_mask = (1u << _FDCAN_IR_POSI_ELO) | (1u << _FDCAN_IR_POSI_EP) |
			(1u << _FDCAN_IR_POSI_EW) | (1u << _FDCAN_IR_POSI_BO) | (1u << _FDCAN_IR_POSI_WDI) |
			(1u << _FDCAN_IR_POSI_PEA) | (1u << _FDCAN_IR_POSI_PED) | (1u << _FDCAN_IR_POSI_ARA);

		// High priority message
		if ((self[FdcanReg::IR].bitof(_FDCAN_IR_POSI_HPM)) && (self[FdcanReg::IE].bitof(_FDCAN_IR_POSI_HPM))) {
			self[FdcanReg::IE].setof(_FDCAN_IR_POSI_HPM, false);
			self[FdcanReg::IR] = (1u << _FDCAN_IR_POSI_HPM);
			irq_flags = (1u << _FDCAN_IR_POSI_HPM);
			if (hp_msg_handler) hp_msg_handler();
		}
		// Tx abort complete
		if ((self[FdcanReg::IR].bitof(_FDCAN_IR_POSI_TCF)) && (self[FdcanReg::IE].bitof(_FDCAN_IR_POSI_TCF))) {
			stduint aborted = self[FdcanReg::TXBCF] & self[FdcanReg::TXBCIE];
			self[FdcanReg::TXBCIE] = self[FdcanReg::TXBCIE] & ~aborted;
			self[FdcanReg::IR] = (1u << _FDCAN_IR_POSI_TCF);
			irq_flags = aborted;
			if (tx_buffer_abort_handler) tx_buffer_abort_handler();
		}
		// Tx event FIFO
		{
			stduint its = self[FdcanReg::IR] & tef_mask & self[FdcanReg::IE];
			if (its) {
				self[FdcanReg::IE] = self[FdcanReg::IE] & ~its;
				self[FdcanReg::IR] = its;
				irq_flags = its;
				if (tx_event_handler) tx_event_handler();
			}
		}
		// Rx FIFO 0
		{
			stduint its = self[FdcanReg::IR] & rf0_mask & self[FdcanReg::IE];
			if (its) {
				self[FdcanReg::IE] = self[FdcanReg::IE] & ~its;
				self[FdcanReg::IR] = its;
				irq_flags = its;
				if (rx_fifo0_handler) rx_fifo0_handler();
			}
		}
		// Rx FIFO 1
		{
			stduint its = self[FdcanReg::IR] & rf1_mask & self[FdcanReg::IE];
			if (its) {
				self[FdcanReg::IE] = self[FdcanReg::IE] & ~its;
				self[FdcanReg::IR] = its;
				irq_flags = its;
				if (rx_fifo1_handler) rx_fifo1_handler();
			}
		}
		// Tx FIFO empty
		if ((self[FdcanReg::IR].bitof(_FDCAN_IR_POSI_TFE)) && (self[FdcanReg::IE].bitof(_FDCAN_IR_POSI_TFE))) {
			self[FdcanReg::IE].setof(_FDCAN_IR_POSI_TFE, false);
			self[FdcanReg::IR] = (1u << _FDCAN_IR_POSI_TFE);
			irq_flags = (1u << _FDCAN_IR_POSI_TFE);
			if (tx_fifo_empty_handler) tx_fifo_empty_handler();
		}
		// Tx complete
		if ((self[FdcanReg::IR].bitof(_FDCAN_IR_POSI_TC)) && (self[FdcanReg::IE].bitof(_FDCAN_IR_POSI_TC))) {
			stduint transmitted = self[FdcanReg::TXBTO] & self[FdcanReg::TXBTIE];
			self[FdcanReg::TXBTIE] = self[FdcanReg::TXBTIE] & ~transmitted;
			self[FdcanReg::IR] = (1u << _FDCAN_IR_POSI_TC);
			irq_flags = transmitted;
			if (tx_buffer_complete_handler) tx_buffer_complete_handler();
		}
		// Rx buffer new message
		if ((self[FdcanReg::IR].bitof(_FDCAN_IR_POSI_DRX)) && (self[FdcanReg::IE].bitof(_FDCAN_IR_POSI_DRX))) {
			self[FdcanReg::IE].setof(_FDCAN_IR_POSI_DRX, false);
			self[FdcanReg::IR] = (1u << _FDCAN_IR_POSI_DRX);
			irq_flags = (1u << _FDCAN_IR_POSI_DRX);
			if (rx_buffer_new_handler) rx_buffer_new_handler();
		}
		// Timestamp wraparound
		if ((self[FdcanReg::IR].bitof(_FDCAN_IR_POSI_TSW)) && (self[FdcanReg::IE].bitof(_FDCAN_IR_POSI_TSW))) {
			self[FdcanReg::IE].setof(_FDCAN_IR_POSI_TSW, false);
			self[FdcanReg::IR] = (1u << _FDCAN_IR_POSI_TSW);
			irq_flags = (1u << _FDCAN_IR_POSI_TSW);
			if (timestamp_wrap_handler) timestamp_wrap_handler();
		}
		// Timeout occurred
		if ((self[FdcanReg::IR].bitof(_FDCAN_IR_POSI_TOO)) && (self[FdcanReg::IE].bitof(_FDCAN_IR_POSI_TOO))) {
			self[FdcanReg::IE].setof(_FDCAN_IR_POSI_TOO, false);
			self[FdcanReg::IR] = (1u << _FDCAN_IR_POSI_TOO);
			irq_flags = (1u << _FDCAN_IR_POSI_TOO);
			if (timeout_handler) timeout_handler();
		}
		// RAM access failure
		if ((self[FdcanReg::IR].bitof(_FDCAN_IR_POSI_MRAF)) && (self[FdcanReg::IE].bitof(_FDCAN_IR_POSI_MRAF))) {
			self[FdcanReg::IE].setof(_FDCAN_IR_POSI_MRAF, false);
			self[FdcanReg::IR] = (1u << _FDCAN_IR_POSI_MRAF);
			status |= ERR_FDCAN_RAM_ACCESS;
		}
		// Protocol / counters errors
		{
			stduint its = self[FdcanReg::IR] & err_mask & self[FdcanReg::IE];
			if (its) {
				self[FdcanReg::IE] = self[FdcanReg::IE] & ~its;
				self[FdcanReg::IR] = its;
				status |= its;
			}
		}
		// TT interrupts (FDCAN1 only)
		if (baseaddr == FDCAN1_BASE) {
			const stduint sched_mask = (1u << _FDCAN_TTIR_POSI_SBC) | (1u << _FDCAN_TTIR_POSI_SMC) |
				(1u << _FDCAN_TTIR_POSI_CSM) | (1u << _FDCAN_TTIR_POSI_SOG);
			const stduint tmark_mask = (1u << _FDCAN_TTIR_POSI_RTMI) | (1u << _FDCAN_TTIR_POSI_TTMI);
			const stduint gtime_mask = (1u << _FDCAN_TTIR_POSI_GTW) | (1u << _FDCAN_TTIR_POSI_GTD);
			const stduint dist_mask = (1u << _FDCAN_TTIR_POSI_GTE) | (1u << _FDCAN_TTIR_POSI_TXU) |
				(1u << _FDCAN_TTIR_POSI_TXO) | (1u << _FDCAN_TTIR_POSI_SE1) |
				(1u << _FDCAN_TTIR_POSI_SE2) | (1u << _FDCAN_TTIR_POSI_ELC);
			const stduint fatal_mask = (1u << _FDCAN_TTIR_POSI_IWT) | (1u << _FDCAN_TTIR_POSI_WT) |
				(1u << _FDCAN_TTIR_POSI_AW) | (1u << _FDCAN_TTIR_POSI_CER);
			stduint ttir = self[FdcanReg::TTIR] & self[FdcanReg::TTIE];
			if (ttir & sched_mask) {
				stduint its = ttir & sched_mask;
				self[FdcanReg::TTIE] = self[FdcanReg::TTIE] & ~its;
				self[FdcanReg::TTIR] = its;
				tt_irq_flags = its;
				if (tt_sched_sync_handler) tt_sched_sync_handler();
			}
			if (ttir & tmark_mask) {
				stduint its = ttir & tmark_mask;
				self[FdcanReg::TTIE] = self[FdcanReg::TTIE] & ~its;
				self[FdcanReg::TTIR] = its;
				tt_irq_flags = its;
				if (tt_time_mark_handler) tt_time_mark_handler();
			}
			if ((ttir & (1u << _FDCAN_TTIR_POSI_SWE)) && (self[FdcanReg::TTIE].bitof(_FDCAN_TTIR_POSI_SWE))) {
				self[FdcanReg::TTIE].setof(_FDCAN_TTIR_POSI_SWE, false);
				self[FdcanReg::TTIR] = (1u << _FDCAN_TTIR_POSI_SWE);
				tt_irq_flags = (1u << _FDCAN_TTIR_POSI_SWE);
				if (tt_stop_watch_handler) tt_stop_watch_handler();
			}
			if (ttir & gtime_mask) {
				stduint its = ttir & gtime_mask;
				self[FdcanReg::TTIE] = self[FdcanReg::TTIE] & ~its;
				self[FdcanReg::TTIR] = its;
				tt_irq_flags = its;
				if (tt_global_time_handler) tt_global_time_handler();
			}
			if (ttir & dist_mask) {
				stduint its = ttir & dist_mask;
				self[FdcanReg::TTIE] = self[FdcanReg::TTIE] & ~its;
				self[FdcanReg::TTIR] = its;
				status |= its;
			}
			if (ttir & fatal_mask) {
				stduint its = ttir & fatal_mask;
				self[FdcanReg::TTIE] = self[FdcanReg::TTIE] & ~its;
				self[FdcanReg::TTIR] = its;
				status |= its;
			}
		}
		// Error callback if any error bit was latched
		if (status != ERR_FDCAN_NONE) {
			if (error_handler) error_handler();
		}
	}

	// ---- internal helpers ----

	// AKA FDCAN_CalcultateRamBlockAddresses
	bool FDCAN_t::calcRamBlockAddresses() {
		StandardFilterSA = MessageRAMOffset;
		self[FdcanReg::SIDFC].maset(_FDCAN_SIDFC_POSI_FLSSA, 14, StandardFilterSA);
		self[FdcanReg::SIDFC].maset(_FDCAN_SIDFC_POSI_LSS, 8, StdFiltersNbr);
		ExtendedFilterSA = StandardFilterSA + StdFiltersNbr;
		self[FdcanReg::XIDFC].maset(_FDCAN_XIDFC_POSI_FLESA, 14, ExtendedFilterSA);
		self[FdcanReg::XIDFC].maset(_FDCAN_XIDFC_POSI_LSE, 8, ExtFiltersNbr);
		RxFIFO0SA = ExtendedFilterSA + (ExtFiltersNbr * 2);
		self[FdcanReg::RXF0C].maset(_FDCAN_RXF0C_POSI_F0SA, 14, RxFIFO0SA);
		self[FdcanReg::RXF0C].maset(_FDCAN_RXF0C_POSI_F0S, 8, RxFifo0ElmtsNbr);
		RxFIFO1SA = RxFIFO0SA + (RxFifo0ElmtsNbr * (byte)RxFifo0ElmtSize);
		self[FdcanReg::RXF1C].maset(_FDCAN_RXF1C_POSI_F1SA, 14, RxFIFO1SA);
		self[FdcanReg::RXF1C].maset(_FDCAN_RXF1C_POSI_F1S, 8, RxFifo1ElmtsNbr);
		RxBufferSA = RxFIFO1SA + (RxFifo1ElmtsNbr * (byte)RxFifo1ElmtSize);
		self[FdcanReg::RXBC].maset(_FDCAN_RXBC_POSI_RBSA, 14, RxBufferSA);
		TxEventFIFOSA = RxBufferSA + (RxBuffersNbr * (byte)RxBufferSize);
		self[FdcanReg::TXEFC].maset(_FDCAN_TXEFC_POSI_EFSA, 14, TxEventFIFOSA);
		self[FdcanReg::TXEFC].maset(_FDCAN_TXEFC_POSI_EFS, 5, TxEventsNbr);
		TxBufferSA = TxEventFIFOSA + (TxEventsNbr * 2);
		self[FdcanReg::TXBC].maset(_FDCAN_TXBC_POSI_TBSA, 14, TxBufferSA);
		self[FdcanReg::TXBC].maset(_FDCAN_TXBC_POSI_NDTB, 8, TxBuffersNbr);
		TxFIFOQSA = TxBufferSA + (TxBuffersNbr * (byte)TxElmtSize);
		self[FdcanReg::TXBC].maset(_FDCAN_TXBC_POSI_TFQS, 8, TxFifoQueueElmtsNbr);

		// convert offsets to absolute addresses (SRAMCAN base)
		StandardFilterSA = SRAMCAN_BASE + (MessageRAMOffset * 4);
		ExtendedFilterSA = StandardFilterSA + (StdFiltersNbr * 4);
		RxFIFO0SA = ExtendedFilterSA + (ExtFiltersNbr * 2 * 4);
		RxFIFO1SA = RxFIFO0SA + (RxFifo0ElmtsNbr * (byte)RxFifo0ElmtSize * 4);
		RxBufferSA = RxFIFO1SA + (RxFifo1ElmtsNbr * (byte)RxFifo1ElmtSize * 4);
		TxEventFIFOSA = RxBufferSA + (RxBuffersNbr * (byte)RxBufferSize * 4);
		TxBufferSA = TxEventFIFOSA + (TxEventsNbr * 2 * 4);
		TxFIFOQSA = TxBufferSA + (TxBuffersNbr * (byte)TxElmtSize * 4);
		EndAddress = TxFIFOQSA + (TxFifoQueueElmtsNbr * (byte)TxElmtSize * 4);

		if (EndAddress > FDCAN_RAM_END) {
			status |= ERR_FDCAN_PARAM;
			return false;
		}
		// flush the allocated Message RAM area
		for (stduint a = StandardFilterSA; a < EndAddress; a += 4)
			*(stduint*)a = 0;
		return true;
	}

	// AKA FDCAN_CopyMessageToRAM
	bool FDCAN_t::copyMessageToRAM(const FdcanTxHeader& header, const byte* data, stduint buffer_index) {
		stduint w1, w2;
		if (header.IdType == FdcanIdType::Standard) {
			w1 = ((stduint)header.ErrorStateIndicator << 31) |
				(stduint)FdcanIdType::Standard |
				(stduint)header.TxFrameType |
				(header.Identifier << 18);
		}
		else {
			w1 = ((stduint)header.ErrorStateIndicator << 31) |
				(stduint)FdcanIdType::Extended |
				(stduint)header.TxFrameType |
				header.Identifier;
		}
		w2 = ((stduint)header.MessageMarker << 24) |
			((stduint)header.TxEventFifoControl << 23) |
			((stduint)header.FDFormat << 21) |
			((stduint)header.BitRateSwitch << 20) |
			((stduint)header.DataLength << 16);
		stduint* addr = (stduint*)(TxBufferSA + (buffer_index * (byte)TxElmtSize * 4));
		addr[0] = w1;
		addr[1] = w2;
		stduint nbytes = _FdcanDlcToBytes[(byte)header.DataLength];
		byte* dst = (byte*)(addr + 2);
		for (stduint i = 0; i < nbytes; i++) dst[i] = data[i];
		return true;
	}

	// ---- H. TT time-triggered (FDCAN1 only) ----

	// AKA HAL_FDCAN_TT_ConfigOperation
	bool FDCAN_t::TT_ConfigOperation(const FdcanTTConfig& cfg) {
		if (baseaddr != FDCAN1_BASE) { status |= ERR_FDCAN_NOT_SUPPORTED; return false; }
		if (state != FdcanState::Ready) { status |= ERR_FDCAN_NOT_READY; return false; }
		// stop local time to enable write access to TURCF other bits
		self[FdcanReg::TURCF].setof(_FDCAN_TURCF_POSI_ELT, false);
		{
			uint64 tickstart = SysTick::getTick();
			if (!fdcan_waitBit(self[FdcanReg::TURCF], _FDCAN_TURCF_POSI_ELT, false, tickstart)) {
				status |= ERR_FDCAN_TIMEOUT; state = FdcanState::Error; return false;
			}
		}
		// Configure TUR (Time Unit Ratio)
		self[FdcanReg::TURCF].maset(_FDCAN_TURCF_POSI_NCL, 16, cfg.TURNumerator - 0x10000);
		self[FdcanReg::TURCF].maset(_FDCAN_TURCF_POSI_DC, 15, cfg.TURDenominator);
		self[FdcanReg::TURCF].setof(_FDCAN_TURCF_POSI_ELT, true);
		// TT operation (OM/TM/LDSDL/IRTO)
		self[FdcanReg::TTOCF].maset(_FDCAN_TTOCF_POSI_OM, 2, cfg.OperationMode);
		self[FdcanReg::TTOCF].setof(_FDCAN_TTOCF_POSI_TM, cfg.TimeMaster != 0);
		self[FdcanReg::TTOCF].maset(_FDCAN_TTOCF_POSI_LDSDL, 3, cfg.SyncDevLimit);
		self[FdcanReg::TTOCF].maset(_FDCAN_TTOCF_POSI_IRTO, 8, cfg.InitRefTrigOffset);
		if (cfg.OperationMode != 0) { // not Level 0
			self[FdcanReg::TTOCF].setof(_FDCAN_TTOCF_POSI_GEN, cfg.GapEnable != 0);
			self[FdcanReg::TTOCF].maset(_FDCAN_TTOCF_POSI_AWL, 8, cfg.AppWdgLimit);
			self[FdcanReg::TTOCF].setof(_FDCAN_TTOCF_POSI_EVTP, cfg.EvtTrigPolarity != 0);
		}
		if (cfg.OperationMode != 1) { // not Level 1
			self[FdcanReg::TTOCF].setof(_FDCAN_TTOCF_POSI_EECS, cfg.ExternalClkSync != 0);
			self[FdcanReg::TTOCF].setof(_FDCAN_TTOCF_POSI_EGTF, cfg.GlobalTimeFilter != 0);
			self[FdcanReg::TTOCF].setof(_FDCAN_TTOCF_POSI_ECC, cfg.ClockCalibration != 0);
		}
		// System matrix limits
		self[FdcanReg::TTMLM].maset(_FDCAN_TTMLM_POSI_CSS, 2, cfg.CycleStartSync);
		if (cfg.OperationMode != 0) {
			self[FdcanReg::TTMLM].maset(_FDCAN_TTMLM_POSI_TXEW, 8, cfg.TxEnableWindow - 1);
			self[FdcanReg::TTMLM].maset(_FDCAN_TTMLM_POSI_ENTT, 12, cfg.ExpTxTrigNbr);
		}
		if (cfg.TimeMaster != 0) { // potential master
			self[FdcanReg::TTMLM].maset(_FDCAN_TTMLM_POSI_CCM, 6, cfg.BasicCyclesNbr);
		}
		// Input triggers: stop watch and event
		self[FdcanReg::TTTS].maset(_FDCAN_TTTS_POSI_SWTSEL, 4, cfg.StopWatchTrigSel);
		self[FdcanReg::TTTS].maset(_FDCAN_TTTS_POSI_EVTSEL, 3, cfg.EventTrigSel);
		// Trigger memory start address
		TTMemorySA = (EndAddress - SRAMCAN_BASE) / 4;
		self[FdcanReg::TTTMC].maset(_FDCAN_TTTMC_POSI_TMSA, 14, TTMemorySA);
		self[FdcanReg::TTTMC].maset(_FDCAN_TTTMC_POSI_TME, 7, cfg.TriggerMemoryNbr);
		// Recalculate end address
		TTMemorySA = SRAMCAN_BASE + (TTMemorySA * 4);
		EndAddress = TTMemorySA + (cfg.TriggerMemoryNbr * 2 * 4);
		if (EndAddress > FDCAN_RAM_END) {
			status |= ERR_FDCAN_PARAM;
			return false;
		}
		for (stduint a = TTMemorySA; a < EndAddress; a += 4) *(stduint*)a = 0;
		return true;
	}

	// AKA HAL_FDCAN_TT_ConfigReferenceMessage
	bool FDCAN_t::TT_ConfigReferenceMessage(FdcanIdType id_type, stduint identifier, stduint payload) {
		if (baseaddr != FDCAN1_BASE) { status |= ERR_FDCAN_NOT_SUPPORTED; return false; }
		if (state != FdcanState::Ready) { status |= ERR_FDCAN_NOT_READY; return false; }
		// RID[28:0] | XTD[30] | RMPS[31]
		stduint val = payload | (stduint)id_type;
		if (id_type == FdcanIdType::Standard) val |= (identifier << 18);
		else val |= identifier;
		self[FdcanReg::TTRMC] = (self[FdcanReg::TTRMC] & ~(0x1FFFFFFF | (1u << 30) | (1u << 31))) | val;
		return true;
	}

	// AKA HAL_FDCAN_TT_ConfigTrigger
	bool FDCAN_t::TT_ConfigTrigger(const FdcanTriggerConfig& trig) {
		if (baseaddr != FDCAN1_BASE) { status |= ERR_FDCAN_NOT_SUPPORTED; return false; }
		if (state != FdcanState::Ready) { status |= ERR_FDCAN_NOT_READY; return false; }
		stduint cycle_code = (trig.RepeatFactor == 0) ? 0 : (trig.RepeatFactor + trig.StartCycle);
		stduint w1 = (trig.TimeMark << 16) | (cycle_code << 8) |
			((stduint)trig.TmEventInt << 5) | ((stduint)trig.TmEventExt << 6) |
			trig.TriggerType;
		// select message number depending on trigger type
		stduint msg_no;
		if (trig.TriggerType == 8 /* RX_TRIGGER */) msg_no = trig.FilterIndex;
		else if ((trig.TriggerType >= 2) && (trig.TriggerType <= 5)) msg_no = fdcan_posval(trig.TxBufferIndex);
		else msg_no = 0;
		stduint w2 = ((stduint)trig.FilterType >> 7) | (msg_no << 16);
		stduint* addr = (stduint*)(TTMemorySA + (trig.TriggerIndex * 8));
		addr[0] = w1;
		addr[1] = w2;
		return true;
	}

	// AKA HAL_FDCAN_TT_SetGlobalTime
	bool FDCAN_t::TT_SetGlobalTime(stduint time_preset) {
		if (baseaddr != FDCAN1_BASE) { status |= ERR_FDCAN_NOT_SUPPORTED; return false; }
		if ((state != FdcanState::Ready) && (state != FdcanState::Busy)) { status |= ERR_FDCAN_NOT_INITIALIZED; return false; }
		if (!self[FdcanReg::TTOCF].bitof(_FDCAN_TTOCF_POSI_EECS)) { status |= ERR_FDCAN_NOT_SUPPORTED; return false; }
		if (self[FdcanReg::TTOCN].bitof(_FDCAN_TTOCN_POSI_SGT)) { status |= ERR_FDCAN_PENDING; return false; }
		self[FdcanReg::TTGTP].maset(_FDCAN_TTGTP_POSI_TP, 16, time_preset);
		{
			uint64 tickstart = SysTick::getTick();
			if (!fdcan_waitBit(self[FdcanReg::TTOCN], _FDCAN_TTOCN_POSI_LCKC, false, tickstart)) {
				status |= ERR_FDCAN_TIMEOUT; state = FdcanState::Error; return false;
			}
		}
		self[FdcanReg::TTOCN].setof(_FDCAN_TTOCN_POSI_SGT, true);
		return true;
	}

	// AKA HAL_FDCAN_TT_SetClockSynchronization
	bool FDCAN_t::TT_SetClockSync(stduint new_tur_numerator) {
		if (baseaddr != FDCAN1_BASE) { status |= ERR_FDCAN_NOT_SUPPORTED; return false; }
		if ((state != FdcanState::Ready) && (state != FdcanState::Busy)) { status |= ERR_FDCAN_NOT_INITIALIZED; return false; }
		if (!self[FdcanReg::TTOCF].bitof(_FDCAN_TTOCF_POSI_EECS)) { status |= ERR_FDCAN_NOT_SUPPORTED; return false; }
		if (self[FdcanReg::TTOCN].bitof(_FDCAN_TTOCN_POSI_ECS)) { status |= ERR_FDCAN_PENDING; return false; }
		self[FdcanReg::TURCF].maset(_FDCAN_TURCF_POSI_NCL, 16, new_tur_numerator - 0x10000);
		{
			uint64 tickstart = SysTick::getTick();
			if (!fdcan_waitBit(self[FdcanReg::TTOCN], _FDCAN_TTOCN_POSI_LCKC, false, tickstart)) {
				status |= ERR_FDCAN_TIMEOUT; state = FdcanState::Error; return false;
			}
		}
		self[FdcanReg::TTOCN].setof(_FDCAN_TTOCN_POSI_ECS, true);
		return true;
	}

	// AKA HAL_FDCAN_TT_ConfigStopWatch
	bool FDCAN_t::TT_ConfigStopWatch(stduint source, stduint polarity) {
		if (baseaddr != FDCAN1_BASE) { status |= ERR_FDCAN_NOT_SUPPORTED; return false; }
		if ((state != FdcanState::Ready) && (state != FdcanState::Busy)) { status |= ERR_FDCAN_NOT_INITIALIZED; return false; }
		{
			uint64 tickstart = SysTick::getTick();
			if (!fdcan_waitBit(self[FdcanReg::TTOCN], _FDCAN_TTOCN_POSI_LCKC, false, tickstart)) {
				status |= ERR_FDCAN_TIMEOUT; state = FdcanState::Error; return false;
			}
		}
		self[FdcanReg::TTOCN].maset(_FDCAN_TTOCN_POSI_SWS, 1, source);
		self[FdcanReg::TTOCN].setof(_FDCAN_TTOCN_POSI_SWP, polarity != 0);
		return true;
	}

	// AKA HAL_FDCAN_TT_ConfigRegisterTimeMark
	bool FDCAN_t::TT_ConfigRegisterTimeMark(stduint source, stduint value, stduint repeat_factor, stduint start_cycle) {
		if (baseaddr != FDCAN1_BASE) { status |= ERR_FDCAN_NOT_SUPPORTED; return false; }
		if ((state != FdcanState::Ready) && (state != FdcanState::Busy)) { status |= ERR_FDCAN_NOT_INITIALIZED; return false; }
		{
			uint64 tickstart = SysTick::getTick();
			if (!fdcan_waitBit(self[FdcanReg::TTOCN], _FDCAN_TTOCN_POSI_LCKC, false, tickstart)) {
				status |= ERR_FDCAN_TIMEOUT; state = FdcanState::Error; return false;
			}
		}
		self[FdcanReg::TTOCN].setof(_FDCAN_TTOCN_POSI_TMC, false); // disable time mark compare
		if (source != 0) { // not disabled
			stduint cycle_code = (repeat_factor == 0) ? 0 : (repeat_factor + start_cycle);
			{
				uint64 tickstart = SysTick::getTick();
				if (!fdcan_waitBit(self[FdcanReg::TTTMK], _FDCAN_TTTMK_POSI_LCKM, false, tickstart)) {
					status |= ERR_FDCAN_TIMEOUT; state = FdcanState::Error; return false;
				}
			}
			self[FdcanReg::TTTMK] = value | (cycle_code << _FDCAN_TTTMK_POSI_TICC);
			{
				uint64 tickstart = SysTick::getTick();
				if (!fdcan_waitBit(self[FdcanReg::TTOCN], _FDCAN_TTOCN_POSI_LCKC, false, tickstart)) {
					status |= ERR_FDCAN_TIMEOUT; state = FdcanState::Error; return false;
				}
			}
			self[FdcanReg::TTOCN].maset(_FDCAN_TTOCN_POSI_TMC, 2, source);
		}
		return true;
	}

	// AKA HAL_FDCAN_TT_EnableRegisterTimeMarkPulse / DisableRegisterTimeMarkPulse
	bool FDCAN_t::TT_enRegisterTimeMarkPulse(bool ena) {
		if (baseaddr != FDCAN1_BASE) { status |= ERR_FDCAN_NOT_SUPPORTED; return false; }
		if ((state != FdcanState::Ready) && (state != FdcanState::Busy)) { status |= ERR_FDCAN_NOT_INITIALIZED; return false; }
		{
			uint64 tickstart = SysTick::getTick();
			if (!fdcan_waitBit(self[FdcanReg::TTOCN], _FDCAN_TTOCN_POSI_LCKC, false, tickstart)) {
				status |= ERR_FDCAN_TIMEOUT; state = FdcanState::Error; return false;
			}
		}
		self[FdcanReg::TTOCN].setof(_FDCAN_TTOCN_POSI_RTIE, ena);
		return true;
	}

	// AKA HAL_FDCAN_TT_EnableTriggerTimeMarkPulse / DisableTriggerTimeMarkPulse
	bool FDCAN_t::TT_enTriggerTimeMarkPulse(bool ena) {
		if (baseaddr != FDCAN1_BASE) { status |= ERR_FDCAN_NOT_SUPPORTED; return false; }
		if ((state != FdcanState::Ready) && (state != FdcanState::Busy)) { status |= ERR_FDCAN_NOT_INITIALIZED; return false; }
		if (self[FdcanReg::TTOCF].masof(_FDCAN_TTOCF_POSI_OM, 2) == 0) { status |= ERR_FDCAN_NOT_SUPPORTED; return false; }
		{
			uint64 tickstart = SysTick::getTick();
			if (!fdcan_waitBit(self[FdcanReg::TTOCN], _FDCAN_TTOCN_POSI_LCKC, false, tickstart)) {
				status |= ERR_FDCAN_TIMEOUT; state = FdcanState::Error; return false;
			}
		}
		self[FdcanReg::TTOCN].setof(_FDCAN_TTOCN_POSI_TTIE, ena);
		return true;
	}

	// AKA HAL_FDCAN_TT_EnableHardwareGapControl / DisableHardwareGapControl
	bool FDCAN_t::TT_enHardwareGapControl(bool ena) {
		if (baseaddr != FDCAN1_BASE) { status |= ERR_FDCAN_NOT_SUPPORTED; return false; }
		if ((state != FdcanState::Ready) && (state != FdcanState::Busy)) { status |= ERR_FDCAN_NOT_INITIALIZED; return false; }
		if (self[FdcanReg::TTOCF].masof(_FDCAN_TTOCF_POSI_OM, 2) == 0) { status |= ERR_FDCAN_NOT_SUPPORTED; return false; }
		{
			uint64 tickstart = SysTick::getTick();
			if (!fdcan_waitBit(self[FdcanReg::TTOCN], _FDCAN_TTOCN_POSI_LCKC, false, tickstart)) {
				status |= ERR_FDCAN_TIMEOUT; state = FdcanState::Error; return false;
			}
		}
		self[FdcanReg::TTOCN].setof(_FDCAN_TTOCN_POSI_GCS, ena);
		return true;
	}

	// AKA HAL_FDCAN_TT_EnableTimeMarkGapControl / DisableTimeMarkGapControl
	bool FDCAN_t::TT_enTimeMarkGapControl(bool ena) {
		if (baseaddr != FDCAN1_BASE) { status |= ERR_FDCAN_NOT_SUPPORTED; return false; }
		if ((state != FdcanState::Ready) && (state != FdcanState::Busy)) { status |= ERR_FDCAN_NOT_INITIALIZED; return false; }
		if (self[FdcanReg::TTOCF].masof(_FDCAN_TTOCF_POSI_OM, 2) == 0) { status |= ERR_FDCAN_NOT_SUPPORTED; return false; }
		{
			uint64 tickstart = SysTick::getTick();
			if (!fdcan_waitBit(self[FdcanReg::TTOCN], _FDCAN_TTOCN_POSI_LCKC, false, tickstart)) {
				status |= ERR_FDCAN_TIMEOUT; state = FdcanState::Error; return false;
			}
		}
		self[FdcanReg::TTOCN].setof(_FDCAN_TTOCN_POSI_TMG, ena);
		return true;
	}

	// AKA HAL_FDCAN_TT_SetNextIsGap
	bool FDCAN_t::TT_SetNextIsGap() {
		if (baseaddr != FDCAN1_BASE) { status |= ERR_FDCAN_NOT_SUPPORTED; return false; }
		if ((state != FdcanState::Ready) && (state != FdcanState::Busy)) { status |= ERR_FDCAN_NOT_INITIALIZED; return false; }
		if (!self[FdcanReg::TTOCF].bitof(_FDCAN_TTOCF_POSI_GEN)) { status |= ERR_FDCAN_NOT_SUPPORTED; return false; }
		if (self[FdcanReg::TTOCF].masof(_FDCAN_TTOCF_POSI_OM, 2) == 0) { status |= ERR_FDCAN_NOT_SUPPORTED; return false; }
		{
			uint64 tickstart = SysTick::getTick();
			if (!fdcan_waitBit(self[FdcanReg::TTOCN], _FDCAN_TTOCN_POSI_LCKC, false, tickstart)) {
				status |= ERR_FDCAN_TIMEOUT; state = FdcanState::Error; return false;
			}
		}
		self[FdcanReg::TTOCN].setof(_FDCAN_TTOCN_POSI_NIG, true);
		return true;
	}

	// AKA HAL_FDCAN_TT_SetEndOfGap
	bool FDCAN_t::TT_SetEndOfGap() {
		if (baseaddr != FDCAN1_BASE) { status |= ERR_FDCAN_NOT_SUPPORTED; return false; }
		if ((state != FdcanState::Ready) && (state != FdcanState::Busy)) { status |= ERR_FDCAN_NOT_INITIALIZED; return false; }
		if (!self[FdcanReg::TTOCF].bitof(_FDCAN_TTOCF_POSI_GEN)) { status |= ERR_FDCAN_NOT_SUPPORTED; return false; }
		if (self[FdcanReg::TTOCF].masof(_FDCAN_TTOCF_POSI_OM, 2) == 0) { status |= ERR_FDCAN_NOT_SUPPORTED; return false; }
		{
			uint64 tickstart = SysTick::getTick();
			if (!fdcan_waitBit(self[FdcanReg::TTOCN], _FDCAN_TTOCN_POSI_LCKC, false, tickstart)) {
				status |= ERR_FDCAN_TIMEOUT; state = FdcanState::Error; return false;
			}
		}
		self[FdcanReg::TTOCN].setof(_FDCAN_TTOCN_POSI_FGP, true);
		return true;
	}

	// AKA HAL_FDCAN_TT_ConfigExternalSyncPhase
	bool FDCAN_t::TT_ConfigExternalSyncPhase(stduint target_phase) {
		if (baseaddr != FDCAN1_BASE) { status |= ERR_FDCAN_NOT_SUPPORTED; return false; }
		if ((state != FdcanState::Ready) && (state != FdcanState::Busy)) { status |= ERR_FDCAN_NOT_INITIALIZED; return false; }
		if (self[FdcanReg::TTOCN].bitof(_FDCAN_TTOCN_POSI_ESCN)) { status |= ERR_FDCAN_PENDING; return false; }
		self[FdcanReg::TTGTP].maset(_FDCAN_TTGTP_POSI_CTP, 16, target_phase);
		return true;
	}

	// AKA HAL_FDCAN_TT_EnableExternalSynchronization / DisableExternalSynchronization
	bool FDCAN_t::TT_enExternalSync(bool ena) {
		if (baseaddr != FDCAN1_BASE) { status |= ERR_FDCAN_NOT_SUPPORTED; return false; }
		if ((state != FdcanState::Ready) && (state != FdcanState::Busy)) { status |= ERR_FDCAN_NOT_INITIALIZED; return false; }
		{
			uint64 tickstart = SysTick::getTick();
			if (!fdcan_waitBit(self[FdcanReg::TTOCN], _FDCAN_TTOCN_POSI_LCKC, false, tickstart)) {
				status |= ERR_FDCAN_TIMEOUT; state = FdcanState::Error; return false;
			}
		}
		self[FdcanReg::TTOCN].setof(_FDCAN_TTOCN_POSI_ESCN, ena);
		return true;
	}

	// AKA HAL_FDCAN_TT_GetOperationStatus
	bool FDCAN_t::TT_getOperationStatus(FdcanTTOpStatus& st) {
		if (baseaddr != FDCAN1_BASE) { status |= ERR_FDCAN_NOT_SUPPORTED; return false; }
		stduint reg = self[FdcanReg::TTOST];
		st.ErrorLevel = (reg >> _FDCAN_TTOST_POSI_EL) & 0x3;
		st.MasterState = (reg >> _FDCAN_TTOST_POSI_MS) & 0x3;
		st.SyncState = (reg >> _FDCAN_TTOST_POSI_SYS) & 0x3;
		st.GTimeQuality = (reg >> _FDCAN_TTOST_POSI_QGTP) & 1;
		st.ClockQuality = (reg >> _FDCAN_TTOST_POSI_QCS) & 1;
		st.RefTrigOffset = (reg >> _FDCAN_TTOST_POSI_RTO) & 0xFF;
		st.GTimeDiscPending = (reg >> _FDCAN_TTOST_POSI_WGTD) & 1;
		st.GapFinished = (reg >> _FDCAN_TTOST_POSI_GFI) & 1;
		st.MasterPriority = (reg >> _FDCAN_TTOST_POSI_TMP) & 0x7;
		st.GapStarted = (reg >> _FDCAN_TTOST_POSI_GSI) & 1;
		st.WaitForEvt = (reg >> _FDCAN_TTOST_POSI_WFE) & 1;
		st.AppWdgEvt = (reg >> _FDCAN_TTOST_POSI_AWE) & 1;
		st.ECSPending = (reg >> _FDCAN_TTOST_POSI_WECS) & 1;
		st.PhaseLock = (reg >> _FDCAN_TTOST_POSI_SPL) & 1;
		return true;
	}
} // namespace uni
#endif
