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

#ifndef _INC_DEVICE_SPI_QUAD
#define _INC_DEVICE_SPI_QUAD

#include "../../c/stdinc.h"
#include "../../c/driver/_predefine/predef.qspi.hpp"
#include "../reference"
#include "../interrupt"
#include "MDMA"
#include "RCC/RCCAddress"
#include "SysTick"

#if defined(_MCU_STM32H7x)

#define _QSPI_ADDR 0x52005000 // D1_AHB1PERIPH_BASE + 0x5000

#undef QSPI

// global ISR entry (extern "C"; defined in interrupt_qspi.hpp), friended below
extern "C" void QUADSPI_IRQHandler();

namespace uni {

	// AKA QSPI_InitTypeDef
	struct QSPI_Init {
		stduint clock_prescaler = 1;           // AHB clock divider, 0..255 (CR.PRESCALER)
		stduint fifo_threshold = 4;            // FIFO threshold bytes, 1..32 (CR.FTHRES)
		QSPISampleShift sample_shifting = QSPISampleShift::None;
		stduint flash_size = 23;               // address bits - 1, 0..31 (DCR.FSIZE)
		QSPICSHighTime chip_select_high_time = QSPICSHighTime::Cycle1;
		QSPIClockMode clock_mode = QSPIClockMode::Mode0;
		QSPIFlashSelect flash_select = QSPIFlashSelect::Flash1;
		QSPIDualFlash dual_flash = QSPIDualFlash::Disable;
	};

	// AKA QSPI_CommandTypeDef
	struct QSPI_Command {
		stduint instruction = 0;               // 8-bit instruction (CCR.INSTRUCTION)
		stduint address = 0;                   // address to send (AR)
		stduint alternate_bytes = 0;           // alternate bytes to send (ABR)
		QSPIAddressSize address_size = QSPIAddressSize::_24;
		QSPIAlternateBytesSize alternate_bytes_size = QSPIAlternateBytesSize::_8;
		stduint dummy_cycles = 0;              // 0..31 (CCR.DCYC)
		QSPIInstrMode instruction_mode = QSPIInstrMode::Line1;
		QSPIAddrMode address_mode = QSPIAddrMode::Line1;
		QSPIAlternateMode alternate_byte_mode = QSPIAlternateMode::None;
		QSPIDataMode data_mode = QSPIDataMode::Line1;
		stduint nb_data = 0;                   // data count, 0 = undefined (DLR)
		QSPIDdrMode ddr_mode = QSPIDdrMode::Disable;
		QSPIDdrHold ddr_hold_half_cycle = QSPIDdrHold::AnalogDelay;
		QSPISiooMode sioo_mode = QSPISiooMode::EveryCmd;
	};

	// AKA QSPI_AutoPollingTypeDef
	struct QSPI_AutoPolling {
		stduint match = 0;                     // value to match (PSMAR)
		stduint mask = 0;                      // mask applied to status (PSMKR)
		stduint interval = 0;                  // clock cycles between reads (PIR)
		stduint status_bytes_size = 1;         // 1..4
		QSPIMatchMode match_mode = QSPIMatchMode::And;
		QSPIAutoStop automatic_stop = QSPIAutoStop::Disable;
	};

	// AKA QSPI_MemoryMappedTypeDef
	struct QSPI_MemoryMapped {
		stduint timeout_period = 0;            // 0..0xFFFF (LPTR)
		QSPITimeoutActivation timeout_activation = QSPITimeoutActivation::Disable;
	};

	// QSPI hardware driver (H7 only; AKA QSPI_HandleTypeDef)
	class QSPI : public RuptTrait {
	public:
		// init config (assign before setMode())
		QSPI_Init init;

		// MDMA handle for IOMethod::DMA (AKA hmdma); assign before DMA transfer
		const MDMAChannel* hmdma = 0;

		// callbacks (AKA HAL_QSPI_*Callback; assign directly)
		Handler_t ErrorHandler = 0;
		Handler_t AbortCpltHandler = 0;
		Handler_t FifoThresholdHandler = 0;
		Handler_t CmdCpltHandler = 0;
		Handler_t RxCpltHandler = 0;
		Handler_t TxCpltHandler = 0;
		Handler_t RxHalfCpltHandler = 0;
		Handler_t TxHalfCpltHandler = 0;
		Handler_t StatusMatchHandler = 0;
		Handler_t TimeOutHandler = 0;

	protected:
		// internal transfer state (AKA tail of QSPI_HandleTypeDef)
		byte* tx_buff = 0;
		stduint tx_size = 0, tx_count = 0;
		byte* rx_buff = 0;
		stduint rx_size = 0, rx_count = 0;
		volatile QSPIState state = QSPIState::Reset;
		stduint error_code = 0;
		stduint timeout = 5000; // AKA HAL_QPSI_TIMEOUT_DEFAULT_VALUE (5s)

	public:
		// AKA HAL_QSPI_Init
		bool setMode();
		// AKA HAL_QSPI_DeInit
		void canMode();
		// AKA __HAL_QSPI_ENABLE / DISABLE (CR.EN)
		void enAble(bool ena = true);
		// RCC AHB3ENR.QSPIEN (bit 14)
		void enClock(bool ena = true);

		// ---- indirect mode ----
		// AKA HAL_QSPI_Command / Command_IT
		bool Command(const QSPI_Command& cmd, IOMethod method = IOMethod::Loop);
		// AKA HAL_QSPI_Transmit / Transmit_IT / Transmit_DMA
		bool Transmit(byte* data, stduint size, IOMethod method = IOMethod::Loop);
		// AKA HAL_QSPI_Receive / Receive_IT / Receive_DMA
		bool Receive(byte* data, stduint size, IOMethod method = IOMethod::Loop);

		// ---- status polling mode ----
		// AKA HAL_QSPI_AutoPolling / AutoPolling_IT
		bool AutoPolling(const QSPI_Command& cmd, const QSPI_AutoPolling& cfg, IOMethod method = IOMethod::Loop);

		// ---- memory-mapped mode ----
		// AKA HAL_QSPI_MemoryMapped
		bool MemoryMapped(const QSPI_Command& cmd, const QSPI_MemoryMapped& cfg);

		// ---- control / state ----
		// AKA HAL_QSPI_Abort / Abort_IT
		bool Abort();
		bool AbortRupt();
		// AKA HAL_QSPI_SetFifoThreshold / GetFifoThreshold
		bool setFifoThreshold(stduint threshold);
		stduint getFifoThreshold();
		// AKA HAL_QSPI_SetTimeout
		void setTimeout(stduint timeout_ms) { timeout = timeout_ms; }
		// AKA HAL_QSPI_GetState / GetError
		QSPIState getState() const { return state; }
		stduint getError() const { return error_code; }

		// register access
		Reference operator[](QSPIReg reg) const { return Reference(_QSPI_ADDR + _IMMx4(reg)); }

		// RuptTrait (NVIC + IRQ_QUADSPI)
		_COM_DEF_Interrupt_Interface();

	protected:
		// ---- DMA path (AKA QSPI_DMA*Callback; static members, bound to MDMA channel slots) ----
		static void MDMATxCplt();
		static void MDMARxCplt();
		static void MDMAError();
		static void MDMAAbortCplt();

		// ISR entry (global, extern "C", defined in interrupt_qspi.hpp)
		friend void ::QUADSPI_IRQHandler();

		// internal helper (AKA QSPI_WaitFlagStateUntilTimeout)
		bool waitFlag(stduint pos, bool val, uint64 tickstart);
	};

	extern QSPI QSPI1;

}

#endif // _MCU_STM32H7x

#endif // _INC_DEVICE_SPI_QUAD
