// ASCII CPL TAB4 CRLF
// Docutitle: [Device] Flexible Data-rate CAN (M_CAN core, H7 only)
// Datecheck: 2026XXXX
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

#ifndef _INC_STANDARD_FDCAN
#define _INC_STANDARD_FDCAN

#include "../stdinc.h"
#include "_predefine/predef.fdcan.hpp"
#include "../../cpp/reference"
#include "../../cpp/Device/GPIO"

/// FDCAN
#if defined(_MCU_STM32H7x)
namespace uni {

	// AKA FDCAN_FrameFormat
	enum class FdcanFrameFormat : stduint {
		Classic = 0x00000000,                 // FDCAN_FRAME_CLASSIC
		FdNoBrs = 0x00000100,                 // FDCAN_FRAME_FD_NO_BRS (CCCR.FDOE)
		FdBrs = 0x00000300,                   // FDCAN_FRAME_FD_BRS (FDOE | BRSE)
	};
	// AKA FDCAN_Mode
	enum class FdcanMode : stduint {
		Normal = 0, RestrictedOperation = 1, BusMonitoring = 2,
		InternalLoopback = 3, ExternalLoopback = 4,
	};
	// AKA FDCAN_IdType (value = bit pattern in element word 1)
	enum class FdcanIdType : stduint {
		Standard = 0x00000000, Extended = 0x40000000,
	};
	// AKA FDCAN_FrameType (value = bit pattern in element word 1)
	enum class FdcanFrameType : stduint {
		Data = 0x00000000, Remote = 0x20000000,
	};
	// AKA FDCAN_DataLengthCode (code; shifted <<16 when packed into element word 2)
	enum class FdcanDlc : byte {
		Bytes0 = 0, Bytes1, Bytes2, Bytes3, Bytes4, Bytes5, Bytes6, Bytes7, Bytes8,
		Bytes12 = 9, Bytes16 = 10, Bytes20 = 11, Bytes24 = 12, Bytes32 = 13, Bytes48 = 14, Bytes64 = 15,
	};
	// AKA FDCAN_DataFieldSize (value used as CvtEltSize index, 1 word = 4 bytes)
	enum class FdcanDataSize : byte {
		Bytes8 = 0x04, Bytes12 = 0x05, Bytes16 = 0x06, Bytes20 = 0x07,
		Bytes24 = 0x08, Bytes32 = 0x0A, Bytes48 = 0x0E, Bytes64 = 0x12,
	};
	// AKA FDCAN_FilterType
	enum class FdcanFilterType : byte {
		Range = 0, Dual = 1, Mask = 2, RangeNoEidm = 3,
	};
	// AKA FDCAN_FilterConfig
	enum class FdcanFilterConfig : byte {
		Disable = 0, ToRxFifo0 = 1, ToRxFifo1 = 2, Reject = 3,
		Hp = 4, ToRxFifo0Hp = 5, ToRxFifo1Hp = 6, ToRxBuffer = 7,
	};
	// AKA FDCAN_TxFifoQueueMode (value = bit pattern of TXBC.TFQM)
	enum class FdcanTxFifoQueueMode : stduint {
		Fifo = 0x00000000, Queue = 0x40000000,
	};
	// AKA FDCAN_RxLocation (HAL: FDCAN_RX_FIFO0=0, FDCAN_RX_FIFO1=1, FDCAN_RX_BUFFER0=32...)
	enum FdcanRxLocation : stduint {
		RxFifo0 = 0, RxFifo1 = 1, RxBuffer0 = 32,
	};
	// AKA HAL_FDCAN_StateTypeDef (simplified)
	enum class FdcanState : byte {
		Reset, Ready, Busy, Error,
	};

	// AKA FDCAN_TxHeaderTypeDef
	struct FdcanTxHeader {
		FdcanIdType IdType = FdcanIdType::Standard;
		stduint Identifier = 0;
		FdcanFrameType TxFrameType = FdcanFrameType::Data;
		FdcanDlc DataLength = FdcanDlc::Bytes0;
		bool ErrorStateIndicator = false;   // ESI: false = error active, true = error passive
		bool BitRateSwitch = false;
		bool FDFormat = false;              // false = classic CAN, true = FD
		bool TxEventFifoControl = false;    // false = no Tx event, true = store Tx event
		byte MessageMarker = 0;
	};
	// AKA FDCAN_RxHeaderTypeDef
	struct FdcanRxHeader {
		FdcanIdType IdType = FdcanIdType::Standard;
		stduint Identifier = 0;
		FdcanFrameType RxFrameType = FdcanFrameType::Data;
		bool ErrorStateIndicator = false;
		stduint RxTimestamp = 0;
		FdcanDlc DataLength = FdcanDlc::Bytes0;
		bool BitRateSwitch = false;
		bool FDFormat = false;
		stduint FilterIndex = 0;
		bool IsFilterMatchingFrame = false;
	};
	// AKA FDCAN_FilterTypeDef
	struct FdcanFilter {
		FdcanIdType IdType = FdcanIdType::Standard;
		stduint FilterIndex = 0;
		FdcanFilterType FilterType = FdcanFilterType::Range;
		FdcanFilterConfig FilterConfig = FdcanFilterConfig::Disable;
		stduint FilterID1 = 0;
		stduint FilterID2 = 0;
		stduint RxBufferIndex = 0;
		bool IsCalibrationMsg = false;
	};
	// AKA FDCAN_ClkCalUnitTypeDef
	struct FdcanClkCalConfig {
		bool ClockCalibration = false;
		stduint ClockDivider = 0;            // FDCAN_CLOCK_DIV1..30 (value = CDIV field)
		stduint MinOscClkPeriods = 0;
		stduint CalFieldLength = 0;          // 0 = 32 bits, CFL bit = 64 bits
		stduint TimeQuantaPerBitTime = 0;    // 4..25
		stduint WatchdogStartValue = 0;
	};
	// AKA FDCAN_TriggerTypeDef
	struct FdcanTriggerConfig {
		stduint TriggerIndex = 0;
		stduint TimeMark = 0;
		stduint RepeatFactor = 0;
		stduint StartCycle = 0;
		bool TmEventInt = false;
		bool TmEventExt = false;
		stduint TriggerType = 0;
		FdcanIdType FilterType = FdcanIdType::Standard;
		stduint TxBufferIndex = 0;
		stduint FilterIndex = 0;
	};
	// AKA FDCAN_TTOperationStatusTypeDef
	struct FdcanTTOpStatus {
		stduint ErrorLevel = 0;
		stduint MasterState = 0;
		stduint SyncState = 0;
		stduint GTimeQuality = 0;
		stduint ClockQuality = 0;
		stduint RefTrigOffset = 0;
		stduint GTimeDiscPending = 0;
		stduint GapFinished = 0;
		stduint MasterPriority = 0;
		stduint GapStarted = 0;
		stduint WaitForEvt = 0;
		stduint AppWdgEvt = 0;
		stduint ECSPending = 0;
		stduint PhaseLock = 0;
	};
	// AKA FDCAN_ErrorCountersTypeDef
	struct FdcanErrorCounters {
		stduint TxErrorCounter = 0;
		stduint RxErrorCounter = 0;
		bool ErrorPassive = false;
		stduint ErrorLoggingCounter = 0;
	};
	// AKA FDCAN_ProtocolStatusTypeDef
	struct FdcanProtocolStatus {
		stduint LastErrorCode = 0;
		stduint DataLastErrorCode = 0;
		stduint Activity = 0;
		bool ErrorPassive = false;
		bool ErrorWarning = false;
		bool BusOff = false;
		bool Rxesi = false;
		bool Rxbrs = false;
		bool Rxdlc = false;
		bool ProtocolException = false;
		stduint Tdcv = 0;
	};
	// AKA FDCAN_HpMsgStatusTypeDef
	struct FdcanHpMsgStatus {
		stduint BufferIndex = 0;
		stduint MessageStorage = 0;
		stduint FilterIndex = 0;
		bool FilterList = false;
	};
	// AKA FDCAN_TxEventFifoTypeDef
	struct FdcanTxEvent {
		FdcanIdType IdType = FdcanIdType::Standard;
		stduint Identifier = 0;
		FdcanFrameType TxFrameType = FdcanFrameType::Data;
		FdcanDlc DataLength = FdcanDlc::Bytes0;
		bool ErrorStateIndicator = false;
		bool BitRateSwitch = false;
		bool FDFormat = false;
		byte MessageMarker = 0;
		stduint EventType = 0;
		stduint TxTimestamp = 0;
	};
	// AKA FDCAN_TT_ConfigTypeDef
	struct FdcanTTConfig {
		stduint TURNumerator = 0x10000;
		stduint TURDenominator = 1;
		stduint OperationMode = 0;          // FDCAN_TT_COMMUNICATION_LEVEL0/1/2
		stduint TimeMaster = 0;             // FDCAN_TT_POTENTIAL_MASTER / NON_MASTER
		stduint SyncDevLimit = 0;
		stduint InitRefTrigOffset = 0;
		stduint TriggerMemoryNbr = 0;
		stduint CycleStartSync = 0;
		stduint StopWatchTrigSel = 0;
		stduint EventTrigSel = 0;
		stduint BasicCyclesNbr = 0;
		stduint GapEnable = 0;
		stduint AppWdgLimit = 0;
		stduint EvtTrigPolarity = 0;
		stduint TxEnableWindow = 0;
		stduint ExpTxTrigNbr = 0;
		stduint ExternalClkSync = 0;
		stduint GlobalTimeFilter = 0;
		stduint ClockCalibration = 0;
	};

	class FDCAN_t : public RuptTrait {
	public:
		// ---- AKA FDCAN_InitTypeDef (member fields, applied by setMode()) ----
		FdcanFrameFormat FrameFormat = FdcanFrameFormat::Classic;
		FdcanMode Mode = FdcanMode::Normal;
		bool AutoRetransmission = true;   // DAR = 0
		bool TransmitPause = false;       // TXP
		bool ProtocolException = true;    // PXHD = 0
		stduint NominalPrescaler = 1, NominalSyncJumpWidth = 1, NominalTimeSeg1 = 1, NominalTimeSeg2 = 1;
		stduint DataPrescaler = 1, DataSyncJumpWidth = 1, DataTimeSeg1 = 1, DataTimeSeg2 = 1;
		stduint StdFiltersNbr = 0, ExtFiltersNbr = 0;
		stduint RxFifo0ElmtsNbr = 0; FdcanDataSize RxFifo0ElmtSize = FdcanDataSize::Bytes8;
		stduint RxFifo1ElmtsNbr = 0; FdcanDataSize RxFifo1ElmtSize = FdcanDataSize::Bytes8;
		stduint RxBuffersNbr = 0; FdcanDataSize RxBufferSize = FdcanDataSize::Bytes8;
		stduint TxEventsNbr = 0;
		stduint TxBuffersNbr = 0;
		stduint TxFifoQueueElmtsNbr = 0; FdcanTxFifoQueueMode TxFifoQueueMode = FdcanTxFifoQueueMode::Fifo;
		FdcanDataSize TxElmtSize = FdcanDataSize::Bytes8;
		stduint MessageRAMOffset = 0;

		// ---- AKA FDCAN_MsgRamAddressTypeDef ----
		stduint StandardFilterSA = 0, ExtendedFilterSA = 0, RxFIFO0SA = 0, RxFIFO1SA = 0;
		stduint RxBufferSA = 0, TxEventFIFOSA = 0, TxBufferSA = 0, TxFIFOQSA = 0;
		stduint TTMemorySA = 0, EndAddress = 0;

		// ---- callbacks (AKA HAL_FDCAN_*Callback, assigned directly, see migrate.md §3.4) ----
		Handler_t rx_fifo0_handler = nil;
		Handler_t rx_fifo1_handler = nil;
		Handler_t tx_event_handler = nil;
		Handler_t tx_fifo_empty_handler = nil;
		Handler_t tx_buffer_complete_handler = nil;
		Handler_t tx_buffer_abort_handler = nil;
		Handler_t rx_buffer_new_handler = nil;
		Handler_t hp_msg_handler = nil;
		Handler_t timestamp_wrap_handler = nil;
		Handler_t timeout_handler = nil;
		Handler_t error_handler = nil;
		Handler_t cc_calibration_handler = nil;
		Handler_t tt_sched_sync_handler = nil;
		Handler_t tt_time_mark_handler = nil;
		Handler_t tt_stop_watch_handler = nil;
		Handler_t tt_global_time_handler = nil;
		// AKA HAL callback ITs argument: interrupt bits of the last dispatch, readable inside callbacks
		stduint irq_flags = 0;    // core IR bits (see _FDCAN_IR_POSI_*)
		stduint tt_irq_flags = 0; // TT TTIR bits (see _FDCAN_TTIR_POSI_*)

	protected:
		stduint baseaddr = nil;
		FdcanState state = FdcanState::Reset;
		stduint status = ERR_FDCAN_NONE;  // AKA HAL ErrorCode, read via getError()

	public:
		FDCAN_t(stduint _baseaddr) : baseaddr(_baseaddr) {}

		// ---- register access ----
		stduint getAddress() const { return baseaddr; }
		Reference operator[](FdcanReg::FdcanRegType idx) const { return baseaddr + idx; } // idx is byte offset
		Reference ccu(FdcanCcuReg::FdcanCcuRegType idx) const { return FDCAN_CCU_BASE + idx; }

		// ---- A. lifecycle (AKA HAL_FDCAN_Init / DeInit / Start / Stop / PowerDown) ----
		bool setMode();
		bool canMode();
		bool start();
		bool stop();
		bool enSleep(bool ena = true);
		bool enClock(bool ena = true);

		// ---- B. CCU clock calibration (AKA HAL_FDCAN_ConfigClockCalibration etc.) ----
		bool setClockCalibration(const FdcanClkCalConfig& cfg);
		stduint getClockCalibrationState() const;
		bool resetClockCalibration();
		stduint getClockCalibrationCounter(stduint counter) const;
		bool enCalibrationRupt(bool ena = true); // AKA CCU interrupt (IRQ_FDCAN_CAL)

		// ---- C. filters & message RAM config ----
		bool setFilter(const FdcanFilter& cfg);
		bool setGlobalFilter(stduint non_matching_std, stduint non_matching_ext, bool reject_remote_std, bool reject_remote_ext);
		bool setExtIdMask(stduint mask);
		bool setRxFifoOverwrite(stduint rxfifo, stduint operation_mode);
		bool setFifoWatermark(stduint fifo, stduint watermark);
		bool setRamWatchdog(stduint counter_start_value);

		// ---- D. timestamp / timeout / Tx delay compensation ----
		bool setTimestampCounter(stduint prescaler);
		bool enTimestampCounter(bool ena = true, stduint operation = 0);
		bool resetTimestampCounter();
		stduint getTimestampCounter() const;      // AKA HAL_FDCAN_GetTimestampCounter (TSCV)
		bool setTimeoutCounter(stduint operation, stduint period);
		bool enTimeoutCounter(bool ena = true);
		bool resetTimeoutCounter();
		stduint getTimeoutCounter() const;        // AKA HAL_FDCAN_GetTimeoutCounter (TOCV)
		bool setTxDelayCompensation(stduint tdc_offset, stduint tdc_filter);
		bool enTxDelayCompensation(bool ena = true);

		// ---- E. transmit ----
		bool Transmit(const FdcanTxHeader& header, const byte* data);
		bool Transmit(const FdcanTxHeader& header, const byte* data, stduint buffer_index);
		bool requestTxBuffer(stduint buffer_index);
		bool abortTxRequest(stduint buffer_index);

		// ---- F. receive & status ----
		bool Receive(FdcanRxLocation location, FdcanRxHeader& header, byte* data);
		bool getTxEvent(FdcanTxEvent& ev);
		bool getHpMsgStatus(FdcanHpMsgStatus& st);
		bool getProtocolStatus(FdcanProtocolStatus& st);
		bool getErrorCounters(FdcanErrorCounters& ec);
		bool isRxBufferAvailable(stduint rx_buffer_index) const;
		bool isTxBufferPending(stduint tx_buffer_index) const;
		stduint getRxFifoFillLevel(stduint rxfifo) const;
		stduint getTxFifoFreeLevel() const;
		bool isRestrictedMode() const;
		bool exitRestrictedMode();
		stduint getError() const { return status; }
		FdcanState getState() const { return state; }

		// ---- G. interrupts (RuptTrait) ----
		void setInterrupt(Handler_t f) const override;          // sets rx_fifo0_handler
		void setInterruptPriority(byte preempt, byte sub_priority) const override;
		void enInterrupt(bool enable = true) const override;     // NVIC IT0 + IT1 lines
		bool setInterruptLine(stduint it_list, stduint line);   // AKA HAL_FDCAN_ConfigInterruptLines
		// AKA HAL_FDCAN_ActivateNotification / DeactivateNotification: enable/disable a set of
		// interrupt sources (IE bits) + interrupt line (ILE) + Tx buffer notification (TXBTIE/TXBCIE)
		bool enNotification(stduint its, stduint buffer_indexes = 0, bool ena = true);
		// AKA HAL_FDCAN_TT_ActivateNotification / DeactivateNotification: TT interrupt sources (TTIE)
		bool TT_enNotification(stduint its, bool ena = true);
		// AKA HAL_FDCAN_TT_ConfigInterruptLines: TT interrupt line selection (TTILS)
		bool TT_setInterruptLine(stduint it_list, stduint line);

		// ---- H. TT time-triggered (FDCAN1 only) ----
		bool TT_ConfigOperation(const FdcanTTConfig& cfg);
		bool TT_ConfigReferenceMessage(FdcanIdType id_type, stduint identifier, stduint payload);
		bool TT_ConfigTrigger(const FdcanTriggerConfig& trig);
		bool TT_SetGlobalTime(stduint time_preset);
		bool TT_SetClockSync(stduint new_tur_numerator);
		bool TT_ConfigStopWatch(stduint source, stduint polarity);
		bool TT_ConfigRegisterTimeMark(stduint source, stduint value, stduint repeat_factor, stduint start_cycle);
		bool TT_enRegisterTimeMarkPulse(bool ena = true);
		bool TT_enTriggerTimeMarkPulse(bool ena = true);
		bool TT_enHardwareGapControl(bool ena = true);
		bool TT_enTimeMarkGapControl(bool ena = true);
		bool TT_SetNextIsGap();
		bool TT_SetEndOfGap();
		bool TT_ConfigExternalSyncPhase(stduint target_phase);
		bool TT_enExternalSync(bool ena = true);
		bool TT_getOperationStatus(FdcanTTOpStatus& st);

		// AKA HAL_FDCAN_IRQHandler, dispatched by FDCAN1_IT0/IT1_IRQHandler / FDCAN_CAL_IRQHandler
		void irqHandler();

	protected:
		// internal helpers (AKA FDCAN_CalcultateRamBlockAddresses / FDCAN_CopyMessageToRAM)
		bool calcRamBlockAddresses();
		bool copyMessageToRAM(const FdcanTxHeader& header, const byte* data, stduint buffer_index);
	};
	extern FDCAN_t FDCAN1, FDCAN2; // global instances (H7)
} // namespace uni
#endif // _MCU_STM32H7x
#endif // _INC_STANDARD_FDCAN
