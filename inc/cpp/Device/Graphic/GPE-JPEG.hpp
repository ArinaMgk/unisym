// UTF-8 CPP-ISO11 TAB4 CRLF
// Docutitle: [Device.GrpahicEngine] JPEG
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

#ifndef _INC_DEVICE_GRAPHIC_GPE_JPEG
#define _INC_DEVICE_GRAPHIC_GPE_JPEG

#include "../../../c/stdinc.h"
#include "../../../c/driver/_predefine/predef.jpeg.hpp"
#include "../../../c/format/picture/JPEG.h"
#include "../../reference"
#include "../../interrupt"
#include "../../string"
#include "../MDMA"
#include "../RCC/RCCAddress"
#include "../SysTick"

#if defined(_MCU_STM32H7x)

#define _JPEG_ADDR 0x52003000 // D1_AHB1PERIPH_BASE + 0x3000

#undef JPEG

// global ISR entry (extern "C"; defined in interrupt_jpeg.hpp), friended below
extern "C" void JPEG_IRQHandler();

namespace uni {

	// JPEG hardware codec (H7 only; AKA JPEG_HandleTypeDef + JPEG_ConfTypeDef)
	class JPEG_HARD : public RuptTrait {
	public:
		// ---- encoding configuration fields (AKA JPEG_ConfTypeDef) ----
		// assign before calling setMode()
		JPEGColorSpace color_space = JPEGColorSpace::YCbCr;
		JPEGSubsampling subsampling = JPEGSubsampling::_444;
		stduint image_width = 0;
		stduint image_height = 0;
		byte image_quality = 90; // 1..100

		// custom quantization tables; 0 = use built-in defaults (AKA HAL_JPEG_SetUserQuantTables)
		const byte* quantTable0 = 0;
		const byte* quantTable1 = 0;
		const byte* quantTable2 = 0;
		const byte* quantTable3 = 0;

		// MDMA handles for IOMethod::DMA (AKA hdmain / hdmaout); assign before DMA transfer
		const MDMAChannel* hdmain = 0;
		const MDMAChannel* hdmaout = 0;

		// in/out data buffers (AKA HAL_JPEG_ConfigInputBuffer / ConfigOutputBuffer);
		// assign address/length before Encode/Decode
		Slice inn_buffer = { 0, 0 };
		Slice out_buffer = { 0, 0 };

		// callbacks (AKA HAL weak callbacks; assign directly)
		Handler_t InfoReadyHandler = 0;
		Handler_t EncodeCpltHandler = 0;
		Handler_t DecodeCpltHandler = 0;
		Handler_t ErrorHandler = 0;

	protected:
		// internal processing state (AKA tail of JPEG_HandleTypeDef)
		stduint JpegInCount = 0;
		stduint JpegOutCount = 0;
		volatile JPEGState State = JPEGState::Reset;
		stduint ErrorCode = 0;
		volatile stduint Context = 0;

	public:
		JPEG_HARD() = default;

		// AKA HAL_JPEG_Init: enClock, core reset, default quant/Huffman tables, header parsing
		bool setMode();
		// AKA HAL_JPEG_ConfigEncoding: apply encoding config fields (color_space/subsampling/size/quality)
		bool ConfigEncoding();
		// AKA HAL_JPEG_DeInit
		void canMode();
		// AKA __HAL_JPEG_ENABLE / DISABLE (CR.JCEN)
		void enAble(bool ena = true);
		// RCC AHB3ENR.JPGDECEN
		void enClock(bool ena = true);
		// AKA HAL_JPEG_EnableHeaderParsing / DisableHeaderParsing
		void setHeaderParsing(bool ena);
		// AKA HAL_JPEG_GetInfo; fills JPEG_INFO parsed from the decode header
		bool getInfo(JPEG_INFO& info);
		// AKA HAL_JPEG_SetUserQuantTables (assign quantTable0..3 then call)
		bool setQuantTables();

		// AKA HAL_JPEG_Encode / _IT / _DMA (in: MCU pixel blocks, out: JPEG stream)
		bool Encode(const byte* pDataInMCU, stduint inLen, byte* pDataOut, stduint outLen, IOMethod method = IOMethod::Loop);
		// AKA HAL_JPEG_Decode / _IT / _DMA (in: JPEG stream, out: MCU pixel blocks)
		bool Decode(const byte* pDataIn, stduint inLen, byte* pDataOut, stduint outLen, IOMethod method = IOMethod::Loop);
		// AKA HAL_JPEG_Pause / Resume (sel: JPEG_PAUSE_INPUT / JPEG_PAUSE_OUTPUT / JPEG_PAUSE_INPUT_OUTPUT)
		bool Pause(stduint sel);
		bool Resume(stduint sel);
		// AKA HAL_JPEG_Abort
		bool Abort();

		// AKA HAL_JPEG_GetState / GetError
		JPEGState getState() const { return State; }
		stduint getError() const { return ErrorCode; }

		// register access
		Reference operator[](JPEGReg reg) const { return Reference(_JPEG_ADDR + _IMMx4(reg)); }

		// RuptTrait (NVIC + IRQ_JPEG)
		_COM_DEF_Interrupt_Interface();

	protected:
		// AKA JPEG_Init_Process: set DE bit, flush FIFOs, clear flags, start core
		void InitializeProcess();
		// AKA JPEG_Process: pump in/out FIFO words until EOC; true when done
		bool ProcessPump();
		// AKA JPEG_ReadInputData / JPEG_StoreOutputData
		void ReadInput(stduint nbWords);
		void StoreOutput(stduint nbWords);

		// ---- DMA path (AKA JPEG_DMA_* / JPEG_MDMA*Callback; invoked from IOMethod::DMA) ----
		// static members so they can touch the protected state above
		static bool DMAStartProcess(JPEG_HARD& jpeg);
		static void DMAContinueProcess(JPEG_HARD& jpeg);
		static void DMAEndProcess(JPEG_HARD& jpeg);
		static void DMAPollResidual(JPEG_HARD& jpeg);
		static void MDMAInCplt();
		static void MDMAOutCplt();
		static void MDMAError();
		static void MDMAOutAbort();

		// ISR entry (global, extern "C", defined in interrupt_jpeg.hpp)
		friend void ::JPEG_IRQHandler();
	};

	extern JPEG_HARD JPEG;

}

#endif // _MCU_STM32H7x

#endif // _INC_DEVICE_GRAPHIC_GPE_JPEG
