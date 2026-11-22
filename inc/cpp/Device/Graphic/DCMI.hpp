// UTF-8 CPP-ISO11 TAB4 LF
// Docutitle: [Device] Digital Camera Interface
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

#ifndef _INC_DEVICE_GRAPHIC_DCMI
#define _INC_DEVICE_GRAPHIC_DCMI

#include "../../../c/stdinc.h"

#if defined(_MCU_STM32H7x)
#include "../../../c/graphic.h"
#include "../../../c/driver/_predefine/predef.dcmi.hpp"
#include "../../atomic"
#include "../../reference"
#include "../../interrupt"
#include "../DMA"

#undef DCMI
extern "C" void DCMI_IRQHandler();
extern "C" void DCMI_PSSI_IRQHandler();

namespace uni {
	struct ImageBuffer;

	enum class DCMIState : byte {
		Reset = 0x00, Ready = 0x01, Busy = 0x02,
		Timeout = 0x03, Error = 0x04, Suspended = 0x05
	};
	enum class DCMICaptureMode : byte { Continuous = 0, Snapshot = 1 };
	enum class DCMISyncMode : byte { Hardware = 0, Embedded = 1 };
	enum class DCMICaptureRate : byte { All = 0, Alternate2 = 1, Alternate4 = 2 };
	enum class DCMIDataWidth : byte { Bits8 = 0, Bits10 = 1, Bits12 = 2, Bits14 = 3 };
	enum class DCMIByteSelect : byte { All = 0, Other = 1, Alternate4 = 2, Alternate2 = 3 };
	enum class DCMILineSelect : byte { All = 0, Alternate2 = 1 };
	enum class DCMISelectStart : byte { First = 0, Second = 1 };

	struct DCMISyncCodes {
		byte frame_start = 0, line_start = 0, line_end = 0, frame_end = 0;
	};

	struct DCMIConfig_t {
		DCMISyncMode sync_mode = DCMISyncMode::Hardware;
		bool pixel_clock_rising = false;
		bool vsync_high = false, hsync_high = false;
		DCMICaptureRate capture_rate = DCMICaptureRate::All;
		DCMIDataWidth data_width = DCMIDataWidth::Bits8;
		DCMISyncCodes sync_codes;
		bool jpeg_mode = false;
		DCMIByteSelect byte_select = DCMIByteSelect::All;
		DCMISelectStart byte_select_start = DCMISelectStart::First;
		DCMILineSelect line_select = DCMILineSelect::All;
		DCMISelectStart line_select_start = DCMISelectStart::First;
	};

	class DCMI_t : public RuptTrait {
	public:
		const DMAStream* hdmarx = nullptr;
		Handler_t FrameHandler = nullptr, LineHandler = nullptr;
		Handler_t VsyncHandler = nullptr, ErrorHandler = nullptr;

		bool setMode(const DCMIConfig_t& config);
		bool canMode();
		// DMA length is in 32-bit words; dest must be word-aligned. Retry nested IRQ restarts from the main loop.
		bool Start(DCMICaptureMode mode, pureptr_t dest, stduint word_count);
		// Rows must be tightly packed; capacity includes word padding. No conversion or ownership change.
		bool Start(DCMICaptureMode mode, ImageBuffer& image);
		// Snapshot capacity is word-aligned and must exceed the received frame size.
		bool StartVariable(pureptr_t dest, stduint capacity_bytes);
		// Continuous capture may overwrite a completed frame before it is consumed.
		bool getFrameReady() const { return frame_ready.load(MemoryOrder_Acquire); }
		// Includes any final word padding; inspect the JPEG stream for its exact length.
		stduint getReceivedSize() const { return received_size.load(MemoryOrder_Acquire); }
		// Nested IRQ cleanup/restart can fail while DMA callbacks are retained; retry from the main loop.
		bool Stop();
		bool Suspend();
		bool Resume();
		// RM0433 DCMI_CWSIZE: x/width count pixel clocks; y/height count lines.
		bool ConfigCrop(const Rectangle& rect);
		bool enCrop(bool ena = true);
		DCMIState getState() const { return state; }
		uint32 getError() const { return error_code; }
		void enClock(bool ena = true);
		void enAble(bool ena = true);
		Reference operator[](DCMIReg reg) const { return Reference(_DCMI_BASE + _IMMx4(reg)); }
		_COM_DEF_Interrupt_Interface();

	protected:
		Atomic<bool> lock{false};
		Atomic<bool> frame_ready{false};
		Atomic<stduint> received_size{0};
		volatile bool variable_capture = false, finishing_frame = false;
		volatile bool capacity_reached = false;
		volatile bool deferred_release = false;
		bool saved_circular = false;
		bool staged_capture = false;
		stduint staged_written = 0;
		stduint capture_capacity = 0;
		volatile DCMIState state = DCMIState::Reset;
		volatile uint32 error_code = _DCMI_ERROR_NONE;
		volatile stduint transfer_count = 0;
		stduint transfer_size = 0, transfer_number = 0, buffer_address = 0;
		stduint frame_blocks = 0, blocks_left = 0;
		const DMAStream* active_dma = nullptr;
		Handler_t saved_callbacks[6] = {};
		pureptr_t saved_bind = nullptr;
		static DCMI_t* volatile active_device;

		bool BindDMA(const DMAStream& stream);
		bool Start(DCMICaptureMode mode, pureptr_t dest, stduint word_count, bool variable, stduint row_words = 0);
		bool FinishFrame();
		void ReleaseDMA();
		bool StopTransfer();
		bool WaitCaptureStopped();
		bool CopyStagedBuffer(bool memory1, stduint bytes);
		void IRQHandler();
		static void DMATransferComplete();
		static void DMAError();
		friend void ::DCMI_IRQHandler();
		friend void ::DCMI_PSSI_IRQHandler();
	};

	extern DCMI_t DCMI;
}
#endif
#endif
