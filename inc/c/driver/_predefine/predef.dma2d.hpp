// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Predefine) DMA2D
// Codifiers: @dosconio: 20241201 ~
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

#ifndef _INC_DEV_PREDEF_DMA2D
#define _INC_DEV_PREDEF_DMA2D

#if defined(_MCU_STM32H7x)

// DMA2D (Chrom-Art Accelerator) — STM32H7 only
// Base: D1_AHB1PERIPH_BASE + 0x1000
#define _DMA2D_ADDR 0x52001000

// ---- CR bit positions ----
#define _DMA2D_CR_START    0
#define _DMA2D_CR_SUSP     1
#define _DMA2D_CR_ABORT    2
#define _DMA2D_CR_TEIE     8
#define _DMA2D_CR_TCIE     9
#define _DMA2D_CR_TWIE     10
#define _DMA2D_CR_CAEIE    11
#define _DMA2D_CR_CTCIE    12
#define _DMA2D_CR_CEIE     13
#define _DMA2D_CR_MODE     16

// ---- ISR / IFCR flag bit positions ----
#define _DMA2D_FLAG_TE     0
#define _DMA2D_FLAG_TC     1
#define _DMA2D_FLAG_TW     2
#define _DMA2D_FLAG_CAE    3
#define _DMA2D_FLAG_CTC    4
#define _DMA2D_FLAG_CE     5

// ---- (FG/BG)PFCCR bit positions ----
#define _DMA2D_PFCCR_CM    0   // input color mode, 4b
#define _DMA2D_PFCCR_CCM   4   // CLUT color mode, 1b
#define _DMA2D_PFCCR_START 5   // CLUT load start
#define _DMA2D_PFCCR_CS    8   // CLUT size, 8b
#define _DMA2D_PFCCR_AM    16  // alpha mode, 2b
#define _DMA2D_PFCCR_CSS   18  // chroma sub-sampling, 2b (foreground only)
#define _DMA2D_PFCCR_AI    20  // alpha inverted
#define _DMA2D_PFCCR_RBS   21  // red/blue swap
#define _DMA2D_PFCCR_ALPHA 24  // alpha value, 8b

// ---- OPFCCR bit positions ----
#define _DMA2D_OPFCCR_CM   0   // output color mode, 3b
#define _DMA2D_OPFCCR_AI   20
#define _DMA2D_OPFCCR_RBS  21

// ---- NLR bit positions ----
#define _DMA2D_NLR_NL      0   // number of lines, 16b
#define _DMA2D_NLR_PL      16  // pixels per line, 14b

// ---- AMTCR bit positions ----
#define _DMA2D_AMTCR_EN    0
#define _DMA2D_AMTCR_DT    8   // dead time, 8b

namespace uni {

	// AKA DMA2D_TypeDef — register word indices (32-bit stride)
	enum class DMA2DReg {
		CR,      // 0x00 control
		ISR,     // 0x04 interrupt status
		IFCR,    // 0x08 interrupt flag clear
		FGMAR,   // 0x0C foreground memory address
		FGOR,    // 0x10 foreground offset
		BGMAR,   // 0x14 background memory address
		BGOR,    // 0x18 background offset
		FGPFCCR, // 0x1C foreground PFC control
		FGCOLR,  // 0x20 foreground color
		BGPFCCR, // 0x24 background PFC control
		BGCOLR,  // 0x28 background color
		FGCMAR,  // 0x2C foreground CLUT memory address
		BGCMAR,  // 0x30 background CLUT memory address
		OPFCCR,  // 0x34 output PFC control
		OCOLR,   // 0x38 output color
		OMAR,    // 0x3C output memory address
		OOR,     // 0x40 output offset
		NLR,     // 0x44 number of line
		LWR,     // 0x48 line watermark
		AMTCR    // 0x4C AHB master timer
	};

}

#endif // _MCU_STM32H7x

#endif
