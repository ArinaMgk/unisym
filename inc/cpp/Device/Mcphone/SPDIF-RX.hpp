// ASCII CPP-ISO11 TAB4 LF
// Docutitle: [Device] Sony/Philips Digital Interface Receiver
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

#ifndef _INC_DEVICE_MCPHONE_SPDIF
#define _INC_DEVICE_MCPHONE_SPDIF

#include "../../../c/stdinc.h"

#if defined(_MCU_STM32H7x)
#include "../../../c/driver/_predefine/predef.spdifrx.hpp"
#include "../../atomic"
#include "../../interrupt"
#include "../../reference"
#include "../../System/Audiosys/AudioCapture.hpp"

#undef SPDIFRX
extern "C" void SPDIF_RX_IRQHandler();

namespace uni {
	class DMAStream;

	enum class SPDIFRXFlow : byte { Data, Control };
	enum class SPDIFRXInput : byte { Input0 = 0, Input1 = 1, Input2 = 2, Input3 = 3 };
	enum class SPDIFRXRetries : byte { None = 0, Three = 1, Fifteen = 2, SixtyThree = 3 };
	enum class SPDIFRXPacking : byte { LSB = 0, MSB = 1, Packed16 = 2 };
	enum class SPDIFRXChannel : byte { A = 0, B = 1 };

	enum class SPDIFRXState : byte {
		Reset = 0, Ready = 1, Busy = 2, BusyData = 3, BusyControl = 4, Error = 7
	};

	enum class SPDIFRXResult : byte { OK, Error, Busy, Timeout, InvalidArgument };

	enum class SPDIFRXError : uint32 {
		None = 0, Timeout = 1U << 0, Overrun = 1U << 1, Parity = 1U << 2,
		DMA = 1U << 3, Unknown = 1U << 4, Frame = 1U << 5,
		Synchronization = 1U << 6, TransitionTimeout = 1U << 7
	};

	struct SPDIFRXDataFormat {
		SPDIFRXPacking packing = SPDIFRXPacking::LSB;
		bool stereo = true;
		bool mask_parity_error = false, mask_validity = false;
		bool mask_channel_status = false, mask_preamble = false;
	};

	struct SPDIFRXConfig_t {
		SPDIFRXDataFormat format;
		SPDIFRXInput input = SPDIFRXInput::Input0;
		SPDIFRXRetries retries = SPDIFRXRetries::None;
		SPDIFRXChannel control_channel = SPDIFRXChannel::A;
		bool wait_for_activity = false;
		bool symbol_clock = false, backup_symbol_clock = false;
	};

	class SPDIFRX_t : public RuptTrait {
	public:
		// Board code owns input pins, the kernel clock and DMA stream selection.
		const DMAStream* data_dma = nullptr;
		const DMAStream* control_dma = nullptr;
		mutable Handler_t DataHandler = nullptr;
		Handler_t DataHalfHandler = nullptr, ControlHandler = nullptr, ControlHalfHandler = nullptr;
		Handler_t ErrorHandler = nullptr;

		bool setMode(const SPDIFRXConfig_t& config);
		bool canMode();
		bool setDataFormat(const SPDIFRXDataFormat& format);
		// Foreground only. Each word is a 32-bit DR or CSR read. Async modes wait only for link acquisition.
		SPDIFRXResult Receive(SPDIFRXFlow flow, uint32* dest, stduint word_count,
			IOMethod method = IOMethod::Loop, stduint timeout_ms = 10);
		// Foreground only. On failure keep DMA buffers reserved until a later successful Stop.
		bool Stop(stduint timeout_ms = 10);
		SPDIFRXState getState() const;
		uint32 getError() const;
		uint32 getError(SPDIFRXFlow flow) const;
		Reference operator[](SPDIFRXReg reg) const { return Reference(_SPDIFRX_BASE + _IMMx4(reg)); }
		using RuptTrait::enInterrupt;
		_COM_DEF_Interrupt_Interface();

	private:
		struct Transfer {
			uint32* dest = nullptr;
			stduint remaining = 0;
			uint32 error = 0;
			IOMethod method = IOMethod::Loop;
			bool active = false, circular = false;
			const DMAStream* dma = nullptr;
			Handler_t saved_half = nullptr, saved_complete = nullptr, saved_error = nullptr;
		};

		Atomic<bool> operation{false};
		Transfer data, control;
		uint32 link_error = 0;
		bool configured = false, link_failed = false;

		Transfer& SelectTransfer(SPDIFRXFlow flow) { return flow == SPDIFRXFlow::Data ? data : control; }
		SPDIFRXResult StartTransfer(SPDIFRXFlow flow, uint32* dest, stduint word_count, IOMethod method, bool start_link);
		bool StartDMA(SPDIFRXFlow flow, Transfer& transfer);
		bool ReleaseDMA(Transfer& transfer, stduint timeout_ms);
		void RequestStop(SPDIFRXFlow flow);
		bool StopTransfers(stduint timeout_ms);
		void UpdateInterruptMask();
		void SetIdle();
		SPDIFRXResult WaitForSync(stduint timeout_ms);
		SPDIFRXResult Poll(SPDIFRXFlow flow, stduint timeout_ms);
		void HandleRupt();
		void HandleDMAComplete(SPDIFRXFlow flow, bool half);
		void HandleDMAError(SPDIFRXFlow flow);
		static void DataDMAHalf();
		static void DataDMAComplete();
		static void DataDMAError();
		static void ControlDMAHalf();
		static void ControlDMAComplete();
		static void ControlDMAError();
		friend void ::SPDIF_RX_IRQHandler();
	};


	// The caller establishes an LPCM source and its nominal rate before starting.
	class SPDIFRXCapture : public AudioDMACapture {
		SPDIFRX_t& receiver;
		uint32 sample_rate;
		SPDIFRXDataFormat saved_format;
		Handler_t saved_half = nullptr, saved_complete = nullptr, saved_error = nullptr;
		bool bound = false, saved_circular = false;
		static SPDIFRXCapture* active;
		static void HalfComplete();
		static void Complete();
		static void Failed();
	protected:
		AudioResult BeginSource() override;
		AudioResult EndSource() override;
		bool IsHalfReadable(bool second) const override;
		bool SourceFailed() const override;
		void ConvertSamples(const byte* source, byte* dest, uint32 bytes) override;
	public:
		// Allocate staging in DMA1/2-accessible memory; clocks and pins are board-owned.
		SPDIFRXCapture(SPDIFRX_t& receiver, uint32 sample_rate, trait::Malloc& allocator)
			: AudioDMACapture(allocator), receiver(receiver), sample_rate(sample_rate) {}
		~SPDIFRXCapture() override;
	};

	extern SPDIFRX_t SPDIFRX;
}
#endif
#endif
