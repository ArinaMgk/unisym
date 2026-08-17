// UTF-8 C/C++11 TAB4 CRLF
// Docutitle: (Protocol) Inter-IC Sound, I2S
// Codifiers: @dosconio: 2025xxxx~;
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
// Dependens: RCC
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

#if !defined(_INC_Standard_I2S)
#define _INC_Standard_I2S
#include "../stdinc.h"
#if defined(_MCU_Intel8051)


#elif defined(_INC_CPP) // Below are C++ Area
#include "../../cpp/reference"
#include "_predefine/predef.spi.hpp"
#include "_predefine/predef.i2s.hpp"

#define DA_PP 1

namespace uni {

#ifndef _INC_UNI_IOMethod
#define _INC_UNI_IOMethod
	enum class IOMethod : byte {
		Loop,   // polling
		Rupt,   // interrupt
		DMA     // direct memory access
	};
#endif

#if defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)
#include "../../cpp/Device/_inner/I2S-STM32H7.hpp"

	class DMAStream;// forward declaration for I2S DMA integration

	// AKA HAL_I2S_StateTypeDef
	enum class I2SXState : byte {
		Reset, Ready, BusyTX, BusyRX, BusyTxRx, Error, Abort
	};

	class I2S_HARD {
	protected:
		byte id;// 1..3 (SPI1/SPI2/SPI3 with I2S), identical on H7 & MP13
		// AKA I2S_InitTypeDef
		I2SMode_E mode = I2SMode_E::Slave;
		I2SStandard_E standard = I2SStandard_E::Philips;
		I2SDataFormat_E datafmt = I2SDataFormat_E::Bits16;
		I2SClockPolarity_E cpol = I2SClockPolarity_E::Low;
		I2SMCLKOutput_E mclk = I2SMCLKOutput_E::Disable;
		I2SFirstBit_E first_bit = I2SFirstBit_E::MSB;
		I2SWSInversion_E wsinv = I2SWSInversion_E::Disable;
		I2SData24BitAlignment_E align24 = I2SData24BitAlignment_E::Right;
		I2SMasterKeepIO_E keep_io = I2SMasterKeepIO_E::Disable;
	#if defined(_MCU_STM32H7x)
		I2SIOSwap_E ioswap = I2SIOSwap_E::Disable;
		I2SFifoThreshold_E fifo_thr = I2SFifoThreshold_E::Data1;
		I2SSlaveExtendFREDetection_E fixch = I2SSlaveExtendFREDetection_E::Disable;
	#endif
		stduint audio_freq = 0;// 0 = AKA I2S_AUDIOFREQ_DEFAULT (divider 2/0)
		// AKA I2S_HandleTypeDef transfer fields
		bool lock = false;// HAL Lock
		I2SXState state = I2SXState::Reset;
		stduint error_code = 0;
		const uint16* tx_buff = nullptr;
		stduint tx_size = 0, tx_count = 0;
		uint16* rx_buff = nullptr;
		stduint rx_size = 0, rx_count = 0;
		// I2SCFG[2:0] direction encoding currently written (0..5); avoids rewrites
		// on same-direction streaming transfers (I2SCFG must be edited with SPE off)
		byte i2scfg_cur = 0xFF;
		// DMA handles (set by user before DMA transfer)
		const DMAStream* hdmatx = nullptr;
		const DMAStream* hdmarx = nullptr;
	public:
		I2S_HARD(byte _id) : id(_id) {}

		Reference operator[](SPIReg reg);
		bool enClock(bool ena = true);
		bool enAble(bool ena = true);
		bool canMode();

		I2S_HARD& setMode(I2SMode_E val) { mode = val; return self; }
		I2S_HARD& setStandard(I2SStandard_E val) { standard = val; return self; }
		I2S_HARD& setDataFormat(I2SDataFormat_E val) { datafmt = val; return self; }
		I2S_HARD& setClockPolarity(I2SClockPolarity_E val) { cpol = val; return self; }
		I2S_HARD& setMCLKOutput(I2SMCLKOutput_E val) { mclk = val; return self; }
		I2S_HARD& setFirstBit(I2SFirstBit_E val) { first_bit = val; return self; }
		I2S_HARD& setWSInversion(I2SWSInversion_E val) { wsinv = val; return self; }
		I2S_HARD& setData24BitAlignment(I2SData24BitAlignment_E val) { align24 = val; return self; }
		I2S_HARD& setMasterKeepIO(I2SMasterKeepIO_E val) { keep_io = val; return self; }
	#if defined(_MCU_STM32H7x)
		I2S_HARD& setIOSwap(I2SIOSwap_E val) { ioswap = val; return self; }
		I2S_HARD& setFifoThreshold(I2SFifoThreshold_E val) { fifo_thr = val; return self; }
		I2S_HARD& setSlaveExtendFREDetection(I2SSlaveExtendFREDetection_E val) { fixch = val; return self; }
	#endif
		I2S_HARD& setAudioFreq(stduint hz) { audio_freq = hz; return self; }

		// AKA HAL_I2S_Transmit / _IT / _DMA (blocking + interrupt + DMA)
		stduint Transmit(const uint16* tx, stduint size, IOMethod method = IOMethod::Loop);
		// AKA HAL_I2S_Receive / _IT / _DMA
		stduint Receive(uint16* rx, stduint size, IOMethod method = IOMethod::Loop);
		// AKA HAL_I2SEx_TransmitReceive / _IT / _DMA (full-duplex, H7 _ex & MP13 native)
		stduint Transceive(const uint16* tx, uint16* rx, stduint size, IOMethod method = IOMethod::Loop);
		// AKA HAL_I2S_DMAPause / DMAResume / DMAStop (audio stream pause/resume/stop)
		bool PauseDMA();
		bool ResumeDMA();
		bool StopDMA();
		// AKA HAL_I2S_GetState / GetError
		I2SXState getState() const { return state; }
		stduint getError() const { return error_code; }
		void setState(I2SXState s) { state = s; }
		void setError(stduint e) { error_code |= e; }
		// callbacks (Handler_t, AKA HAL weak callbacks)
		Handler_t TxCpltHandler, RxCpltHandler, TxRxCpltHandler, ErrorHandler;
		// ISR helpers (called from interrupt table once wired; AKA HAL_I2S_IRQHandler split)
		void innHandlerByInterrupt();
		void outHandlerByInterrupt();
		// internal transfer helpers (AKA I2S_CloseRx_ISR / I2S_CloseTx_ISR / WaitFlagStateUntilTimeout)
		void closeTxISR();
		void closeRxISR();
		bool waitFlag(stduint pos, bool val, uint64 tickstart, stduint timeout);
		// direction mapping: dir 0=Tx 1=Rx 2=FD -> I2SCFG[2:0] per (mode, dir); no-op if unchanged
		void setDirection(byte dir);
		// DMA start helpers (route DMA1/2 stream; I2S only on SPI1/2/3, no BDMA)
		bool dmaTxStart(pureptr_t mem, pureptr_t peri, stduint ndtr);
		bool dmaRxStart(pureptr_t peri, pureptr_t mem, stduint ndtr);
		// AKA I2S_DMATxCplt / I2S_DMARxCplt (DMA completion, dispatched from DMA layer)
		void dmaCpltISR();
	};
	extern I2S_HARD I2S1, I2S2, I2S3;
#endif

}

#endif

#endif
