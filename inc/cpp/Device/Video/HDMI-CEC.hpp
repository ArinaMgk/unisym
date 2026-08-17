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

#ifndef _INC_DEVICE_HDMI_CEC
#define _INC_DEVICE_HDMI_CEC

#include "../../unisym"
#if defined(_MCU_STM32H7x)

#include "../../reference"
#include "../../interrupt"

namespace uni {

#undef CEC

	// AKA CEC register offsets (new-generation CEC: CR/CFGR/TXDR/RXDR/ISR/IER)
	enum class CECReg {
		CR, CFGR, TXDR, RXDR, ISR, IER
	};

	// AKA CEC_Signal_Free_Time (CFGR.SFT[2:0]); value n (1..7) = 0.5 + (n-1) bit periods
	enum class CECSignalFreeTime : byte {
		Default = 0,   // CEC_DEFAULT_SFT
		Bit0_5 = 1,    // CEC_0_5_BITPERIOD_SFT
		Bit1_5 = 2,    // CEC_1_5_BITPERIOD_SFT
		Bit2_5 = 3,
		Bit3_5 = 4,
		Bit4_5 = 5,
		Bit5_5 = 6,
		Bit6_5 = 7
	};

	// AKA CEC_Tolerance (CFGR.RXTOL)
	enum class CECTolerance : byte {
		Standard = 0, Extended = 1
	};

	// AKA CEC_Listening_Mode (CFGR.LSTN)
	enum class CECListenMode : byte {
		Reduced = 0, Full = 1
	};

	// AKA CEC_SFT_Option (CFGR.SFTOPT)
	enum class CECSftOption : byte {
		OnTxSom = 0, OnTxRxEnd = 1
	};

	// AKA HAL_CEC_StateTypeDef (simplified; gState|RxState collapsed)
	enum class CECState : byte {
		Reset, Ready, BusyTX, BusyRX, Error
	};

	// AKA HAL_CEC_ERROR_* (ISR bitmask)
	#define ERR_CEC_NONE    0x0000
	#define ERR_CEC_RXOVR   (1u << 2)   // ISR.RXOVR
	#define ERR_CEC_BRE     (1u << 3)   // ISR.BRE
	#define ERR_CEC_SBPE    (1u << 4)   // ISR.SBPE
	#define ERR_CEC_LBPE    (1u << 5)   // ISR.LBPE
	#define ERR_CEC_RXACKE  (1u << 6)   // ISR.RXACKE
	#define ERR_CEC_ARBLST  (1u << 7)   // ISR.ARBLST
	#define ERR_CEC_TXUDR   (1u << 10)  // ISR.TXUDR
	#define ERR_CEC_TXERR   (1u << 11)  // ISR.TXERR
	#define ERR_CEC_TXACKE  (1u << 12)  // ISR.TXACKE

	class HDMI_CEC_t : public RuptTrait {
	public:
		byte* rx_buffer = nullptr;       // AKA HAL Init.RxBuffer (RX 目标缓冲)
		Handler_t TxCpltHandler = 0;     // AKA HAL_CEC_TxCpltCallback
		Handler_t RxCpltHandler = 0;     // AKA HAL_CEC_RxCpltCallback
		Handler_t ErrorHandler = 0;      // AKA HAL_CEC_ErrorCallback
	protected:
		bool rx_stop_bre = false;                          // AKA Init.BRERxStop (CFGR.BRESTP)
		bool bre_err_gen = false;                          // AKA Init.BREErrorBitGen (CFGR.BREGEN)
		bool lbpe_err_gen = false;                         // AKA Init.LBPEErrorBitGen (CFGR.LBPEGEN)
		bool brd_no_gen = false;                           // AKA Init.BroadcastMsgNoErrorBitGen (CFGR.BRDNOGEN)
		CECSftOption sft_opt = CECSftOption::OnTxSom;      // AKA Init.SignalFreeTimeOption (CFGR.SFTOPT)
		const byte* tx_ptr = nullptr;                      // AKA pTxBuffPtr
		stduint tx_count = 0;                              // AKA TxXferCount
		stduint rx_size = 0;                               // AKA RxXferSize
		stduint errcode = ERR_CEC_NONE;                    // AKA ErrorCode
		bool lock_tx = false;                              // AKA gState == BUSY_TX
		bool lock_rx = false;                              // AKA RxState == BUSY_RX
	public:
		HDMI_CEC_t();
		Reference operator[](CECReg idx) const;
		bool enClock(bool ena = true) const;
		bool enAble(bool ena = true) const;
		// AKA HAL_CEC_Init（常用参数；错误生成标志经下方 setter 缓存后一并写入 CFGR）
		bool setMode(CECSignalFreeTime sft = CECSignalFreeTime::Default,
			CECTolerance tolerance = CECTolerance::Standard,
			byte ownAddress = 0,
			CECListenMode listen = CECListenMode::Reduced);
		// AKA HAL_CEC_DeInit
		bool canMode();
		// AKA HAL_CEC_SetDeviceAddress
		bool setOwnAddress(byte ownAddress);
		// AKA Init.BRERxStop
		HDMI_CEC_t& setRxStopOnBRE(bool ena = true) { rx_stop_bre = ena; return self; }
		// AKA Init.BREErrorBitGen
		HDMI_CEC_t& setBREErrorBitGen(bool ena = true) { bre_err_gen = ena; return self; }
		// AKA Init.LBPEErrorBitGen
		HDMI_CEC_t& setLBPEErrorBitGen(bool ena = true) { lbpe_err_gen = ena; return self; }
		// AKA Init.BroadcastMsgNoErrorBitGen
		HDMI_CEC_t& setBroadcastNoErrorBitGen(bool ena = true) { brd_no_gen = ena; return self; }
		// AKA Init.SignalFreeTimeOption
		HDMI_CEC_t& setSftOption(CECSftOption opt = CECSftOption::OnTxSom) { sft_opt = opt; return self; }
		// AKA HAL_CEC_Transmit_IT
		bool Transmit(byte initiator, byte destination, const byte* data, stduint size);
		// AKA HAL_CEC_GetLastReceivedFrameSize
		stduint getLastReceivedFrameSize() const { return rx_size; }
		// AKA HAL_CEC_ChangeRxBuffer
		void changeRxBuffer(byte* buffer) { rx_buffer = buffer; }
		// AKA HAL_CEC_GetState
		CECState getState() const;
		// AKA HAL_CEC_GetError
		stduint getError() const { return errcode; }
		// AKA HAL_CEC_IRQHandler
		void IRQHandler();
		_COM_DEF_Interrupt_Interface();
	};

	extern HDMI_CEC_t HDMI_CEC;

}

#endif // _MCU_STM32H7x
#endif // _INC_DEVICE_HDMI_CEC
