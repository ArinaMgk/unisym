// UTF-8 C/C++11 TAB4 CRLF
// Docutitle: (Protocol) Serial Peripheral interface, SPI
// Codifiers: @dosconio: 20240730~;
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
// Dependens: GPIO
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

#if !defined(_INC_Standard_SPI)
#define _INC_Standard_SPI
#include "../stdinc.h"
#if defined(_MCU_Intel8051)


#elif defined(_INC_CPP) // Below are C++ Area
#include "../../cpp/Device/GPIO"
#include "_predefine/predef.spi.hpp"

#define DA_PP 1

namespace uni {

#if defined(_SUPPORT_GPIO)
#undef SPI
	class SPI_t {
	// Dynamic for software, Static for hardware
	protected:
		// bool CPOL; when SCLK==CPOL, Trans is idle
		// bool CPHA; sample at first edge and send secondly if zero, or second edge
		virtual byte Transceive(byte data) { return data & nil; }
	public:
		Handler_t func_delay;

		virtual stduint Transceivex(stduint data, byte blen) { return nil; }
		virtual void SendStart() {}
		virtual void SendStop() {}
		//
		void Send(byte data) { (void)Transceive(data); }
		// 0xFF is a dummy data
		byte Read(void) { return Transceive(0xFF); }
		SPI_t& operator<<(byte txt) { Send(txt); return self; }
		void operator>>(byte& txt) { txt = Read(); }
	};

	class SPI_SOFT : public SPI_t {
	protected:
	protected:
		GPIO_Pin& SCLK,
			& MOSI,
			& MISO,
			& CSEL;// Chip select

		virtual byte Transceive(byte data) override;
	public:
		SPI_SOFT(GPIO_Pin& SCLK, GPIO_Pin& MOSI, GPIO_Pin& MISO, GPIO_Pin& CSEL);
		virtual stduint Transceivex(stduint data, byte blen) override;
		virtual void SendStart() override;
		virtual void SendStop() override;
	};
#if defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)
	#include "../../cpp/Device/_inner/SPI-STM32H7.hpp"

#ifndef _INC_UNI_IOMethod
#define _INC_UNI_IOMethod
	enum class IOMethod : byte {
		Loop,   // polling
		Rupt,   // interrupt
		DMA     // direct memory access
	};
#endif
	class DMAStream;// forward declaration for SPI DMA integration
	// AKA HAL_SPI_StateTypeDef
	enum class SPIXState : byte {
		Reset, Ready, BusyTX, BusyRX, BusyTxRx, Error, Abort
	};

	class SPI_HARD : public SPI_t {
	protected:
		byte id;// 1..6 for H7, 1..5 for MP13
		// AKA SPI_InitTypeDef
		SPIMode_E mode = SPIMode_E::Master;
		SPIDirection_E direction = SPIDirection_E::TwoLine;
		SPIDataSize_E datasize = SPIDataSize_E::Bits8;
		SPIClockPolarity_E clk_polarity = SPIClockPolarity_E::Low;
		SPIClockPhase_E clk_phase = SPIClockPhase_E::Edge1;
		SPINss_E nss = SPINss_E::Soft;
		SPIBaudRate_E baudrate = SPIBaudRate_E::Div2;
		SPIFirstBit_E first_bit = SPIFirstBit_E::MSB;
		SPITIMode_E ti_mode = SPITIMode_E::Disable;
		SPICrc_E crc_calc = SPICrc_E::Disable;
		SPICrcLength_E crc_length = SPICrcLength_E::DataSize;
		stduint crc_polynomial = 0x0007;
		SPICrcInitPattern_E crc_tx_init = SPICrcInitPattern_E::AllZero;
		SPICrcInitPattern_E crc_rx_init = SPICrcInitPattern_E::AllZero;
		SPINssPulse_E nss_pulse = SPINssPulse_E::Disable;
		SPINssPolarity_E nss_polarity = SPINssPolarity_E::Low;
		SPIFifoThreshold_E fifo_threshold = SPIFifoThreshold_E::Data1;
		SPIMasterSSIdle_E ss_idle = SPIMasterSSIdle_E::Cycle0;
		SPIMasterInterDataIdle_E interdata_idle = SPIMasterInterDataIdle_E::Cycle0;
		SPIMasterRxAutoSusp_E rx_autosusp = SPIMasterRxAutoSusp_E::Disable;
		SPIMasterKeepIO_E keep_io = SPIMasterKeepIO_E::Disable;
		SPIIOSwap_E ioswap = SPIIOSwap_E::Disable;
		// AKA SPI_HandleTypeDef transfer fields
		bool lock = false;// HAL Lock
		SPIXState state = SPIXState::Reset;
		stduint error_code = 0;
		const byte* tx_buff = nullptr;
		stduint tx_size = 0, tx_count = 0;
		byte* rx_buff = nullptr;
		stduint rx_size = 0, rx_count = 0;
		// DMA handles (set by user before DMA transfer)
		const DMAStream* hdmatx = nullptr;
		const DMAStream* hdmarx = nullptr;
	#if defined(_MCU_STM32H7x)
		// SPI6 DMA uses BDMA channels (DMAMUX2); SPI1~5 use DMA1/DMA2 streams
		byte bdma_tx_channel = 0xFF;
		byte bdma_rx_channel = 0xFF;
	#endif
	#if defined(_MPU_STM32MP13)
		// AKA SPI_ReloadTypeDef (MP13 continuous transfer)
		const byte* reload_tx = nullptr;
		stduint reload_tx_size = 0;
		byte* reload_rx = nullptr;
		stduint reload_rx_size = 0;
		bool reload_requested = false;
	#endif
	public:
		SPI_HARD(byte _id) : id(_id) {}

		Reference operator[](SPIReg reg);
		bool enClock(bool ena = true);
		bool enAble(bool ena = true);

		SPI_HARD& setMode(SPIMode_E val) { mode = val; return self; }
		SPI_HARD& setDirection(SPIDirection_E val) { direction = val; return self; }
		SPI_HARD& setDataSize(SPIDataSize_E val) { datasize = val; return self; }
		SPI_HARD& setClockPolarity(SPIClockPolarity_E val) { clk_polarity = val; return self; }
		SPI_HARD& setClockPhase(SPIClockPhase_E val) { clk_phase = val; return self; }
		SPI_HARD& setNss(SPINss_E val) { nss = val; return self; }
		SPI_HARD& setBaudRate(SPIBaudRate_E val) { baudrate = val; return self; }
		SPI_HARD& setFirstBit(SPIFirstBit_E val) { first_bit = val; return self; }
		SPI_HARD& setTIMode(SPITIMode_E val) { ti_mode = val; return self; }
		SPI_HARD& setCRCCalculation(SPICrc_E val) { crc_calc = val; return self; }
		SPI_HARD& setCRCLength(SPICrcLength_E val) { crc_length = val; return self; }
		SPI_HARD& setCRCPolynomial(stduint val) { crc_polynomial = val; return self; }
		SPI_HARD& setCRCInitPattern(bool tx, SPICrcInitPattern_E val) { (tx ? crc_tx_init : crc_rx_init) = val; return self; }
		SPI_HARD& setNssPulse(SPINssPulse_E val) { nss_pulse = val; return self; }
		SPI_HARD& setNssPolarity(SPINssPolarity_E val) { nss_polarity = val; return self; }
		SPI_HARD& setFifoThreshold(SPIFifoThreshold_E val) { fifo_threshold = val; return self; }
		SPI_HARD& setMasterSSIdle(SPIMasterSSIdle_E val) { ss_idle = val; return self; }
		SPI_HARD& setMasterInterDataIdle(SPIMasterInterDataIdle_E val) { interdata_idle = val; return self; }
		SPI_HARD& setMasterRxAutoSusp(SPIMasterRxAutoSusp_E val) { rx_autosusp = val; return self; }
		SPI_HARD& setMasterKeepIO(SPIMasterKeepIO_E val) { keep_io = val; return self; }
		SPI_HARD& setIOSwap(SPIIOSwap_E val) { ioswap = val; return self; }

		bool canMode();
		//
		virtual byte Transceive(byte data) override;
		virtual stduint Transceivex(stduint data, byte blen) override;
		virtual void SendStart() override;
		virtual void SendStop() override;
		// AKA HAL_SPI_Transmit/Receive/TransmitReceive (blocking + IT + DMA)
		stduint Transmit(const byte* tx, stduint size, IOMethod method = IOMethod::Loop);
		stduint Receive(byte* rx, stduint size, IOMethod method = IOMethod::Loop);
		stduint Transceive(const byte* tx, byte* rx, stduint size, IOMethod method = IOMethod::Loop);
	#if defined(_MPU_STM32MP13)
		// AKA HAL_SPI_Reload_Transmit/Receive/TransmitReceive_IT
		stduint ReloadTransmit(const byte* tx, stduint size);
		stduint ReloadReceive(byte* rx, stduint size);
		stduint ReloadTransceive(const byte* tx, byte* rx, stduint size);
	#endif
		// AKA HAL_SPI_Abort / Abort_IT
		bool Abort();
		bool AbortRupt();
		// AKA HAL_SPI_DMAPause / DMAResume / DMAStop
		bool PauseDMA();
		bool ResumeDMA();
		bool StopDMA();
		// AKA HAL_SPI_GetState / GetError
		SPIXState getState() const { return state; }
		stduint getError() const { return error_code; }
		void setState(SPIXState s) { state = s; }
		void setError(stduint e) { error_code |= e; }
		// AKA HAL_SPIEx_FlushRxFifo / ConfigureUnderrun / EnableLockConfiguration
		bool FlushRxFifo();
		bool ConfigureUnderrun(SPIUnderrunDetect_E det, SPIUnderrunBehav_E beh);
		bool EnableLockConfiguration();
		// callbacks (Handler_t, AKA HAL weak callbacks)
		Handler_t TxCpltHandler, RxCpltHandler, TxRxCpltHandler, ErrorHandler, AbortCpltHandler;
		// ISR helpers (called from interrupt_spi.hpp)
		void innHandlerByInterrupt();
		void outHandlerByInterrupt();
		// internal transfer helpers (AKA SPI_CloseTransfer / SPI_WaitOnFlagUntilTimeout)
		void closeTransfer();
		bool waitFlag(stduint pos, bool val, uint64 tickstart, stduint timeout);
		// DMA start helpers (route DMA1/2 stream or BDMA channel for SPI6)
		bool dmaTxStart(pureptr_t mem, pureptr_t peri, stduint ndtr);
		bool dmaRxStart(pureptr_t peri, pureptr_t mem, stduint ndtr);
	};
	#if defined(_MCU_STM32H7x)
	extern SPI_HARD SPI1, SPI2, SPI3, SPI4, SPI5, SPI6;
	#elif defined(_MPU_STM32MP13)
	extern SPI_HARD SPI1, SPI2, SPI3, SPI4, SPI5;
	#endif
#endif

#endif

}

#endif
#endif
