// ASCII CPL TAB4 CRLF
// Docutitle: (Module) Video
// Codifiers: @dosconio: 20240502 ~ 20240502
// Attribute: Arn-Covenant Any-Architect Bit-32mode Non-Dependence
// Copyright: UNISYM, under Apache License 2.0; Dosconio Mecocoa, BSD 3-Clause License
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

#ifndef _INC_DEVICE_Video
#define _INC_DEVICE_Video

// see `Video`

// Driver for Screen
// - ILI9341
// - ST7789V

// Driver for Touch
// - XPT2046
#include "../../stdinc.h"

#ifdef _INC_CPP
namespace uni {
	#endif

	struct ModeInfoBlock {
		/* Mandatory information for all VBE versions */
		uint16_t ModeAttributes;        // offset 0x00 : mode attributes flags
		uint8_t  WinAAttributes;        // offset 0x02 : Window A attributes
		uint8_t  WinBAttributes;        // offset 0x03 : Window B attributes
		uint16_t WinGranularity;        // offset 0x04 : granularity of window (in KB units)
		uint16_t WinSize;               // offset 0x06 : size of window (in KB)
		uint16_t WinASegment;           // offset 0x08 : segment for window A
		uint16_t WinBSegment;           // offset 0x0A : segment for window B
		uint32_t WinFuncPtr;            // offset 0x0C : far pointer (segment:offset) for window function (bank switching)
		uint16_t BytesPerScanLine;      // offset 0x10 : number of bytes per scan line (pitch / stride)

		/* VBE 1.2+ fields */
		uint16_t XResolution;            // offset 0x12 : horizontal resolution (in pixels)
		uint16_t YResolution;            // offset 0x14 : vertical resolution (in pixels)
		uint8_t  XCharSize;              // offset 0x16 : character cell width in pixels (text mode)
		uint8_t  YCharSize;              // offset 0x17 : character cell height in pixels (text mode)
		uint8_t  NumberOfPlanes;         // offset 0x18 : number of memory planes
		uint8_t  BitsPerPixel;           // offset 0x19 : bits per pixel (color depth)
		uint8_t  NumberOfBanks;          // offset 0x1A : number of banks (if banked mode)
		uint8_t  MemoryModel;             // offset 0x1B : memory model (e.g. direct color, packed pixel, etc.)
		uint8_t  BankSize;                // offset 0x1C : bank size in KB
		uint8_t  NumberOfImagePages;      // offset 0x1D : number of image pages
		uint8_t  Reserved1;               // offset 0x1E : reserved for alignment / future use

		/* Only valid for “packed / direct color / YUV” memory model */
		uint8_t  RedMaskSize;            // offset 0x1F : number of bits in red mask
		uint8_t  RedFieldPosition;       // offset 0x20 : bit position of LSB of red
		uint8_t  GreenMaskSize;          // offset 0x21 : number of bits in green mask
		uint8_t  GreenFieldPosition;     // offset 0x22 : bit position of LSB of green
		uint8_t  BlueMaskSize;           // offset 0x23 : number of bits in blue mask
		uint8_t  BlueFieldPosition;      // offset 0x24 : bit position of LSB of blue
		uint8_t  RsvdMaskSize;            // offset 0x25 : number of bits in reserved / padding mask (alpha, etc.)
		uint8_t  RsvdFieldPosition;       // offset 0x26 : bit position of LSB of reserved mask
		uint8_t  DirectColorModeInfo;     // offset 0x27 : direct color mode attributes

		/* VBE 2.0+ extended / optional fields */
		uint32_t PhysBasePtr;             // offset 0x28 : physical address of the linear frame buffer (if LFB supported)
		uint32_t OffScreenMemOffset;      // offset 0x2C : offset (in bytes) from start of frame buffer to offscreen memory
		uint16_t OffScreenMemSize;        // offset 0x30 : size of offscreen memory in KB

		/* Additional fields (VBE 3.0 / linear fields) */
		uint16_t LinBytesPerScanLine;     // offset 0x32 : bytes per scan line for linear modes
		uint8_t  BnkNumberOfPages;         // offset 0x34 : number of bank pages
		uint8_t  LinNumberOfPages;         // offset 0x35 : number of linear pages
		uint8_t  LinRedMaskSize;           // offset 0x36 : red mask size for linear mode
		uint8_t  LinRedFieldPosition;      // offset 0x37 : red field pos for linear mode
		uint8_t  LinGreenMaskSize;         // offset 0x38 : green mask size
		uint8_t  LinGreenFieldPosition;    // offset 0x39 : green field pos
		uint8_t  LinBlueMaskSize;          // offset 0x3A : blue mask size
		uint8_t  LinBlueFieldPosition;     // offset 0x3B : blue field pos
		uint8_t  LinRsvdMaskSize;           // offset 0x3C : reserved mask bits for linear mode
		uint8_t  LinRsvdFieldPosition;      // offset 0x3D : reserved field position for linear mode
		uint32_t MaxPixelClock;             // offset 0x3E : maximum pixel clock (in Hz or kHz) for this mode

		uint8_t  ReservedTail[189];           // offset 0x42 ~ 0xFF : reserved / padding to make total size = 256 bytes
	};
	// first 64 bytes are enough

	#ifdef _INC_CPP
}
#endif

#endif
