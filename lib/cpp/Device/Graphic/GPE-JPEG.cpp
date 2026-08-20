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

#include "../../../../inc/cpp/Device/Graphic/GPE-JPEG.hpp"
#include "../../../../inc/cpp/Device/SysTick"
#include "../../../../inc/cpp/Device/NVIC"
#include "../../../../inc/cpp/Device/Interrupt/interrupt_tab.h"
#include "../Interrupt/interrupt_jpeg.hpp"

namespace uni {
#if defined(_MCU_STM32H7x)

	#define _JPEG_TIMEOUT_VALUE   1000U   // 1s
	#define _JPEG_AC_HUFF_SIZE    162U
	#define _JPEG_DC_HUFF_SIZE    12U
	#define _JPEG_FIFO_SIZE       16U     // words
	#define _JPEG_FIFO_TH_SIZE    8U      // words
	#define _JPEG_INTERRUPT_MASK  0x0000007EU

	// ---- default quantization tables (JPEG spec ISO/IEC 10918-1, K.1) ----
	static const byte _JPEG_LUM_QuantTable[64] = {
		16,  11,  10,  16,  24,  40,  51,  61,
		12,  12,  14,  19,  26,  58,  60,  55,
		14,  13,  16,  24,  40,  57,  69,  56,
		14,  17,  22,  29,  51,  87,  80,  62,
		18,  22,  37,  56,  68, 109, 103,  77,
		24,  35,  55,  64,  81, 104, 113,  92,
		49,  64,  78,  87, 103, 121, 120, 101,
		72,  92,  95,  98, 112, 100, 103,  99,
	};
	static const byte _JPEG_CHROM_QuantTable[64] = {
		17,  18,  24,  47,  99,  99,  99,  99,
		18,  21,  26,  66,  99,  99,  99,  99,
		24,  26,  56,  99,  99,  99,  99,  99,
		47,  66,  99,  99,  99,  99,  99,  99,
		99,  99,  99,  99,  99,  99,  99,  99,
		99,  99,  99,  99,  99,  99,  99,  99,
		99,  99,  99,  99,  99,  99,  99,  99,
		99,  99,  99,  99,  99,  99,  99,  99,
	};
	static const byte _JPEG_ZIGZAG_ORDER[64] = {
		 0,   1,   8,  16,   9,   2,   3,  10,
		17,  24,  32,  25,  18,  11,   4,   5,
		12,  19,  26,  33,  40,  48,  41,  34,
		27,  20,  13,   6,   7,  14,  21,  28,
		35,  42,  49,  56,  57,  50,  43,  36,
		29,  22,  15,  23,  30,  37,  44,  51,
		58,  59,  52,  45,  38,  31,  39,  46,
		53,  60,  61,  54,  47,  55,  62,  63,
	};

	// ---- default Huffman tables (JPEG spec, K.3/K.4) ----
	struct _JPEG_DCHuffTable { byte Bits[16]; byte HuffVal[12]; };
	struct _JPEG_ACHuffTable { byte Bits[16]; byte HuffVal[162]; };

	static const _JPEG_DCHuffTable _JPEG_DCLUM_HuffTable = {
		{ 0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xa, 0xb },
	};
	static const _JPEG_DCHuffTable _JPEG_DCCHROM_HuffTable = {
		{ 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 },
		{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xa, 0xb },
	};
	static const _JPEG_ACHuffTable _JPEG_ACLUM_HuffTable = {
		{ 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d },
		{
			0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
			0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
			0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
			0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
			0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
			0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
			0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
			0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
			0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
			0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
			0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
			0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
			0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
			0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
			0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
			0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
			0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
			0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
			0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
			0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
			0xf9, 0xfa,
		},
	};
	static const _JPEG_ACHuffTable _JPEG_ACCHROM_HuffTable = {
		{ 0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77 },
		{
			0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
			0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
			0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
			0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
			0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
			0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
			0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
			0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
			0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
			0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
			0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
			0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
			0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
			0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
			0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
			0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
			0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
			0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
			0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
			0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
			0xf9, 0xfa,
		},
	};

	JPEG_HARD JPEG;

	// ---- private helpers ----

	// AKA JPEG_Bits_To_SizeCodes (Figure C.1/C.2 of the JPEG spec)
	static bool _JPEG_Bits_To_SizeCodes(const byte* Bits, byte* Huffsize, stduint* Huffcode, stduint* LastK) {
		stduint i, p, l, code, si;
		p = 0;
		for (l = 0; l < 16; l++) {
			i = Bits[l];
			if ((p + i) > 256) return false;
			while (i != 0) {
				Huffsize[p++] = (byte)(l + 1);
				i--;
			}
		}
		Huffsize[p] = 0;
		*LastK = p;
		code = 0;
		si = Huffsize[0];
		p = 0;
		while (Huffsize[p] != 0) {
			while (Huffsize[p] == si) {
				Huffcode[p++] = code;
				code++;
			}
			if (code >= (_IMM1S(si))) return false;
			code <<= 1;
			si++;
		}
		return true;
	}

	// AKA JPEG_DCHuff_BitsVals_To_SizeCodes
	static bool _JPEG_DCHuff_BitsVals_To_SizeCodes(const _JPEG_DCHuffTable* src, byte* outCodeLength, stduint* outHuffmanCode) {
		byte huffsize[257];
		stduint huffcode[257];
		stduint lastK;
		if (!_JPEG_Bits_To_SizeCodes(src->Bits, huffsize, huffcode, &lastK)) return false;
		for (stduint k = 0; k < lastK; k++) {
			stduint l = src->HuffVal[k];
			if (l >= _JPEG_DC_HUFF_SIZE) return false;
			outHuffmanCode[l] = huffcode[k];
			outCodeLength[l] = (byte)(huffsize[k] - 1);
		}
		return true;
	}

	// AKA JPEG_ACHuff_BitsVals_To_SizeCodes
	static bool _JPEG_ACHuff_BitsVals_To_SizeCodes(const _JPEG_ACHuffTable* src, byte* outCodeLength, stduint* outHuffmanCode) {
		byte huffsize[257];
		stduint huffcode[257];
		stduint lastK;
		if (!_JPEG_Bits_To_SizeCodes(src->Bits, huffsize, huffcode, &lastK)) return false;
		for (stduint k = 0; k < lastK; k++) {
			stduint l = src->HuffVal[k];
			if (l == 0) l = 160;       // EOB
			else if (l == 0xF0) l = 161; // ZRL
			else l = ((l & 0xF0) >> 4) * 10 + (l & 0x0F) - 1;
			if (l >= _JPEG_AC_HUFF_SIZE) return false;
			outHuffmanCode[l] = huffcode[k];
			outCodeLength[l] = (byte)(huffsize[k] - 1);
		}
		return true;
	}

	// AKA JPEG_Set_HuffDC_Mem (HUFFENC_DC0/DC1, 6 words each, written backwards)
	static bool _JPEG_Set_HuffDC_Mem(JPEG_HARD& jpeg, const _JPEG_DCHuffTable* table, JPEGReg base) {
		byte codeLength[_JPEG_DC_HUFF_SIZE] = { 0 };
		stduint huffmanCode[_JPEG_DC_HUFF_SIZE] = { 0 };
		if (!_JPEG_DCHuff_BitsVals_To_SizeCodes(table, codeLength, huffmanCode)) return false;
		stduint addr = _JPEG_ADDR + (_IMM(base) + _JPEG_DC_HUFF_SIZE / 2) * 4;
		Reference(addr + 0) = 0x0FFF0FFF;
		Reference(addr + 4) = 0x0FFF0FFF;
		for (stduint i = _JPEG_DC_HUFF_SIZE; i > 0; ) {
			i--;
			stduint msb = ((stduint)(codeLength[i] & 0xF) << 8) | (huffmanCode[i] & 0xFF);
			i--;
			stduint lsb = ((stduint)(codeLength[i] & 0xF) << 8) | (huffmanCode[i] & 0xFF);
			addr -= 4;
			Reference(addr + 0) = lsb | (msb << 16);
		}
		return true;
	}

	// AKA JPEG_Set_HuffAC_Mem (HUFFENC_AC0/AC1, 81 words each, written backwards)
	static bool _JPEG_Set_HuffAC_Mem(JPEG_HARD& jpeg, const _JPEG_ACHuffTable* table, JPEGReg base) {
		byte codeLength[_JPEG_AC_HUFF_SIZE] = { 0 };
		stduint huffmanCode[_JPEG_AC_HUFF_SIZE] = { 0 };
		if (!_JPEG_ACHuff_BitsVals_To_SizeCodes(table, codeLength, huffmanCode)) return false;
		stduint addr = _JPEG_ADDR + (_IMM(base) + _JPEG_AC_HUFF_SIZE / 2) * 4;
		// locations 162..175 of each AC table hold internal core info
		Reference(addr + 0) = 0x0FFF0FFF; addr += 4;
		Reference(addr + 0) = 0x0FFF0FFF; addr += 4;
		Reference(addr + 0) = 0x0FFF0FFF; addr += 4;
		Reference(addr + 0) = 0x0FD10FD0; addr += 4;
		Reference(addr + 0) = 0x0FD30FD2; addr += 4;
		Reference(addr + 0) = 0x0FD50FD4; addr += 4;
		Reference(addr + 0) = 0x0FD70FD6;
		for (stduint i = _JPEG_AC_HUFF_SIZE; i > 0; ) {
			i--;
			stduint msb = ((stduint)(codeLength[i] & 0xF) << 8) | (huffmanCode[i] & 0xFF);
			i--;
			stduint lsb = ((stduint)(codeLength[i] & 0xF) << 8) | (huffmanCode[i] & 0xFF);
			addr -= 4;
			Reference(addr + 0) = lsb | (msb << 16);
		}
		return true;
	}

	// AKA JPEG_Set_Huff_DHTMem: write the DHT segment payload (Bits+Vals) into DHTMEM
	static void _JPEG_Set_Huff_DHTMem(JPEG_HARD& jpeg, const _JPEG_ACHuffTable* AC0, const _JPEG_DCHuffTable* DC0,
		const _JPEG_ACHuffTable* AC1, const _JPEG_DCHuffTable* DC1) {
		stduint base = _JPEG_ADDR + 0x360 * 1; // DHTMEM byte offset 0x360
		if (DC0) {
			// DC0 BITS: 4 words at DHTMEM+0..3 (written from word 3 down)
			stduint addr = base + 3 * 4;
			for (stduint index = 16; index > 0; ) {
				Reference(addr + 0) = (stduint(DC0->Bits[index - 1]) << 24) | (stduint(DC0->Bits[index - 2]) << 16)
					| (stduint(DC0->Bits[index - 3]) << 8) | stduint(DC0->Bits[index - 4]);
				addr -= 4;
				index -= 4;
			}
			// DC0 VALS: 3 words at DHTMEM+4..6
			addr = base + 6 * 4;
			for (stduint index = 12; index > 0; ) {
				Reference(addr + 0) = (stduint(DC0->HuffVal[index - 1]) << 24) | (stduint(DC0->HuffVal[index - 2]) << 16)
					| (stduint(DC0->HuffVal[index - 3]) << 8) | stduint(DC0->HuffVal[index - 4]);
				addr -= 4;
				index -= 4;
			}
		}
		if (AC0) {
			// AC0 BITS: 4 words at DHTMEM+7..10
			stduint addr = base + 10 * 4;
			for (stduint index = 16; index > 0; ) {
				Reference(addr + 0) = (stduint(AC0->Bits[index - 1]) << 24) | (stduint(AC0->Bits[index - 2]) << 16)
					| (stduint(AC0->Bits[index - 3]) << 8) | stduint(AC0->Bits[index - 4]);
				addr -= 4;
				index -= 4;
			}
			// AC0 VALS: 162 bytes; last word at DHTMEM+51 keeps high 16 bits
			addr = base + 51 * 4;
			stduint value = Reference(addr + 0) & 0xFFFF0000U;
			value |= (stduint(AC0->HuffVal[161]) << 8) | stduint(AC0->HuffVal[160]);
			Reference(addr + 0) = value;
			addr -= 4;
			for (stduint index = 160; index > 0; ) {
				Reference(addr + 0) = (stduint(AC0->HuffVal[index - 1]) << 24) | (stduint(AC0->HuffVal[index - 2]) << 16)
					| (stduint(AC0->HuffVal[index - 3]) << 8) | stduint(AC0->HuffVal[index - 4]);
				addr -= 4;
				index -= 4;
			}
		}
		if (DC1) {
			// DC1 BITS: bytes at DHTMEM+51 (hi), +52..54, +55 (lo)
			stduint addr = base + 51 * 4;
			stduint value = Reference(addr + 0) & 0x0000FFFFU;
			value |= (stduint(DC1->Bits[1]) << 24) | (stduint(DC1->Bits[0]) << 16);
			Reference(addr + 0) = value;
			addr = base + 55 * 4;
			value = Reference(addr + 0) & 0xFFFF0000U;
			value |= (stduint(DC1->Bits[15]) << 8) | stduint(DC1->Bits[14]);
			Reference(addr + 0) = value;
			addr -= 4;
			for (stduint index = 12; index > 0; ) {
				Reference(addr + 0) = (stduint(DC1->Bits[index + 1]) << 24) | (stduint(DC1->Bits[index]) << 16)
					| (stduint(DC1->Bits[index - 1]) << 8) | stduint(DC1->Bits[index - 2]);
				addr -= 4;
				index -= 4;
			}
			// DC1 VALS: bytes at DHTMEM+55 (hi), +56..57, +58 (lo)
			addr = base + 55 * 4;
			value = Reference(addr + 0) & 0x0000FFFFU;
			value |= (stduint(DC1->HuffVal[1]) << 24) | (stduint(DC1->HuffVal[0]) << 16);
			Reference(addr + 0) = value;
			addr = base + 58 * 4;
			value = Reference(addr + 0) & 0xFFFF0000U;
			value |= (stduint(DC1->HuffVal[11]) << 8) | stduint(DC1->HuffVal[10]);
			Reference(addr + 0) = value;
			addr -= 4;
			for (stduint index = 8; index > 0; ) {
				Reference(addr + 0) = (stduint(DC1->HuffVal[index + 1]) << 24) | (stduint(DC1->HuffVal[index]) << 16)
					| (stduint(DC1->HuffVal[index - 1]) << 8) | stduint(DC1->HuffVal[index - 2]);
				addr -= 4;
				index -= 4;
			}
		}
		if (AC1) {
			// AC1 BITS: bytes at DHTMEM+58 (hi), +59..61, +62 (lo)
			stduint addr = base + 58 * 4;
			stduint value = Reference(addr + 0) & 0x0000FFFFU;
			value |= (stduint(AC1->Bits[1]) << 24) | (stduint(AC1->Bits[0]) << 16);
			Reference(addr + 0) = value;
			addr = base + 62 * 4;
			value = Reference(addr + 0) & 0xFFFF0000U;
			value |= (stduint(AC1->Bits[15]) << 8) | stduint(AC1->Bits[14]);
			Reference(addr + 0) = value;
			addr -= 4;
			for (stduint index = 12; index > 0; ) {
				Reference(addr + 0) = (stduint(AC1->Bits[index + 1]) << 24) | (stduint(AC1->Bits[index]) << 16)
					| (stduint(AC1->Bits[index - 1]) << 8) | stduint(AC1->Bits[index - 2]);
				addr -= 4;
				index -= 4;
			}
			// AC1 VALS: bytes at DHTMEM+62 (hi 16b) then +63..102
			addr = base + 62 * 4;
			value = Reference(addr + 0) & 0x0000FFFFU;
			value |= (stduint(AC1->HuffVal[1]) << 24) | (stduint(AC1->HuffVal[0]) << 16);
			Reference(addr + 0) = value;
			addr = base + 102 * 4;
			for (stduint index = 160; index > 0; ) {
				Reference(addr + 0) = (stduint(AC1->HuffVal[index + 1]) << 24) | (stduint(AC1->HuffVal[index]) << 16)
					| (stduint(AC1->HuffVal[index - 1]) << 8) | stduint(AC1->HuffVal[index - 2]);
				addr -= 4;
				index -= 4;
			}
		}
	}

	// AKA JPEG_Set_HuffEnc_Mem: encoder Huffman tables (HUFFENC_*) + DHTMEM
	static bool _JPEG_Set_HuffEnc_Mem(JPEG_HARD& jpeg) {
		_JPEG_Set_Huff_DHTMem(jpeg, &_JPEG_ACLUM_HuffTable, &_JPEG_DCLUM_HuffTable,
			&_JPEG_ACCHROM_HuffTable, &_JPEG_DCCHROM_HuffTable);
		if (!_JPEG_Set_HuffAC_Mem(jpeg, &_JPEG_ACLUM_HuffTable, JPEGReg::HUFFENC_AC0)) return false;
		if (!_JPEG_Set_HuffAC_Mem(jpeg, &_JPEG_ACCHROM_HuffTable, JPEGReg::HUFFENC_AC1)) return false;
		if (!_JPEG_Set_HuffDC_Mem(jpeg, &_JPEG_DCLUM_HuffTable, JPEGReg::HUFFENC_DC0)) return false;
		if (!_JPEG_Set_HuffDC_Mem(jpeg, &_JPEG_DCCHROM_HuffTable, JPEGReg::HUFFENC_DC1)) return false;
		return true;
	}

	// AKA JPEG_Set_Quantization_Mem: scale by quality, zigzag order, write QMEMx
	static bool _JPEG_Set_Quantization_Mem(JPEG_HARD& jpeg, const byte* QTable, JPEGReg qmem, byte quality) {
		stduint scale;
		if (quality >= 50 && quality <= 100) scale = 200 - quality * 2;
		else if (quality > 0) scale = 5000 / quality;
		else return false;
		stduint addr = _JPEG_ADDR + _IMMx4(qmem);
		for (stduint i = 0; i < 64; i += 4) {
			stduint row = 0;
			for (stduint j = 0; j < 4; j++) {
				stduint v = (stduint(QTable[_JPEG_ZIGZAG_ORDER[i + j]]) * scale + 50) / 100;
				if (v == 0) v = 1;
				else if (v > 255) v = 255;
				row |= (v & 0xFF) << (8 * j);
			}
			Reference(addr + i) = row;
		}
		return true;
	}

	// AKA JPEG_SetColorYCBCR / GrayScale / CMYK (CONFR1 NF/NS + CONFR4-7 sampling)
	static void _JPEG_SetColor(JPEG_HARD& jpeg, JPEGColorSpace color, JPEGSubsampling sampling) {
		using namespace uni;
		stduint ySamplingH = 0, ySamplingV = 0, yblockNb = 0;
		if (sampling == JPEGSubsampling::_420) { ySamplingH = 0x2000; ySamplingV = 0x200; yblockNb = 0x30; }
		else if (sampling == JPEGSubsampling::_422) { ySamplingH = 0x2000; ySamplingV = 0x100; yblockNb = 0x10; }
		else { /* _444 */ }
		Reference confr1 = jpeg[JPEGReg::CONFR1];
		if (color == JPEGColorSpace::Gray) {
			confr1.rstof(_JPEG_CONFR1_POS_NF + 1);
			confr1.rstof(_JPEG_CONFR1_POS_NF);
			confr1.rstof(_JPEG_CONFR1_POS_NS + 1);
			confr1.rstof(_JPEG_CONFR1_POS_NS);
			jpeg[JPEGReg::CONFR4] = 0;
			jpeg[JPEGReg::CONFR4] |= (0x1U << _JPEG_CONFRx_POS_HSF) | (0x1U << _JPEG_CONFRx_POS_VSF);
		}
		else if (color == JPEGColorSpace::YCbCr) {
			confr1.rstof(_JPEG_CONFR1_POS_NF + 1);
			confr1.setof(_JPEG_CONFR1_POS_NF);
			confr1.rstof(_JPEG_CONFR1_POS_NS + 1);
			confr1.setof(_JPEG_CONFR1_POS_NS);
			jpeg[JPEGReg::CONFR4] = ySamplingH | ySamplingV | (yblockNb & 0xF0);
			jpeg[JPEGReg::CONFR5] = (0x1U << _JPEG_CONFRx_POS_HSF) | (0x1U << _JPEG_CONFRx_POS_VSF) | (0x1U << _JPEG_CONFRx_POS_QT) | (0x1U << _JPEG_CONFRx_POS_HA) | (0x1U << _JPEG_CONFRx_POS_HD);
			jpeg[JPEGReg::CONFR6] = (0x1U << _JPEG_CONFRx_POS_HSF) | (0x1U << _JPEG_CONFRx_POS_VSF) | (0x1U << _JPEG_CONFRx_POS_QT) | (0x1U << _JPEG_CONFRx_POS_HA) | (0x1U << _JPEG_CONFRx_POS_HD);
		}
		else { // CMYK
			confr1.setof(_JPEG_CONFR1_POS_NF + 1);
			confr1.setof(_JPEG_CONFR1_POS_NF);
			confr1.setof(_JPEG_CONFR1_POS_NS + 1);
			confr1.setof(_JPEG_CONFR1_POS_NS);
			jpeg[JPEGReg::CONFR4] = ySamplingH | ySamplingV | (yblockNb & 0xF0);
			jpeg[JPEGReg::CONFR5] = (0x1U << _JPEG_CONFRx_POS_HSF) | (0x1U << _JPEG_CONFRx_POS_VSF);
			jpeg[JPEGReg::CONFR6] = (0x1U << _JPEG_CONFRx_POS_HSF) | (0x1U << _JPEG_CONFRx_POS_VSF);
			jpeg[JPEGReg::CONFR7] = (0x1U << _JPEG_CONFRx_POS_HSF) | (0x1U << _JPEG_CONFRx_POS_VSF);
		}
	}

	// AKA JPEG_GetQuality: estimate quality from QMEM0 vs the standard LUM table
	static stduint _JPEG_GetQuality(JPEG_HARD& jpeg) {
		stduint quality = 0;
		stduint addr = _JPEG_ADDR + _IMMx4(JPEGReg::QMEM0);
		for (stduint i = 0; i < 64; i += 4) {
			stduint row = Reference(addr + i);
			for (stduint j = 0; j < 4; j++) {
				stduint v = (row >> (8 * j)) & 0xFF;
				if (v == 1) quality += 100;
				else {
					stduint scale = v * 100 / _JPEG_LUM_QuantTable[_JPEG_ZIGZAG_ORDER[i + j]];
					if (scale <= 100) quality += (200 - scale) / 2;
					else quality += 5000 / scale;
				}
			}
		}
		return quality / 64;
	}

	// ---- DMA path helpers (AKA JPEG_DMA_* / JPEG_MDMA*Callback; called from IOMethod::DMA) ----

	// AKA JPEG_DMA_StartProcess: bind MDMA callbacks, align lengths, start out/in transfers
	bool JPEG_HARD::DMAStartProcess(JPEG_HARD& jpeg) {
		// in/out buffer transfer lengths (user must setMode MDMA channels with
		// MDMA_REQUEST_JPEG_INFIFO_TH / JPEG_OUTFIFO_TH, MDMATrigger::Buffer, bufferLen = 32)
		const MDMAChannel* in = jpeg.hdmain;
		const MDMAChannel* out = jpeg.hdmaout;
		if (!in || !out) return false;
		MDMA_t& m = MDMA;
		m.XferCpltCallback[in->getID()] = &JPEG_HARD::MDMAInCplt;
		m.XferErrorCallback[in->getID()] = &JPEG_HARD::MDMAError;
		m.XferCpltCallback[out->getID()] = &JPEG_HARD::MDMAOutCplt;
		m.XferErrorCallback[out->getID()] = &JPEG_HARD::MDMAError;
		m.XferAbortCallback[out->getID()] = &JPEG_HARD::MDMAOutAbort;
		// MDMA transfer size must be a multiple of the MDMA buffer size (TLEN)
		jpeg.inn_buffer.length -= jpeg.inn_buffer.length % 32;
		jpeg.out_buffer.length -= jpeg.out_buffer.length % 32;
		// start MDMA FIFO out transfer: DOR (fixed) -> out buffer
		if (!out->Transfer((pureptr_t)(_JPEG_ADDR + 0x44), (pureptr_t)jpeg.out_buffer.address, jpeg.out_buffer.length, 1, IOMethod::Rupt)) return false;
		// start MDMA FIFO in transfer: in buffer -> DIR (fixed)
		if (!in->Transfer((pureptr_t)jpeg.inn_buffer.address, (pureptr_t)(_JPEG_ADDR + 0x40), jpeg.inn_buffer.length, 1, IOMethod::Rupt)) return false;
		return true;
	}

	// AKA JPEG_DMA_ContinueProcess: EOC/HPD handling from the JPEG IRQ (DMA mode)
	void JPEG_HARD::DMAContinueProcess(JPEG_HARD& jpeg) {
		// header parsing done (decode)
		if (((jpeg.Context & _JPEG_CONTEXT_OPERATION_MASK) == _JPEG_CONTEXT_DECODE) && jpeg[JPEGReg::SR].bitof(_JPEG_SR_POS_HPDF)) {
			JPEG_INFO info;
			if (jpeg.getInfo(info) && jpeg.InfoReadyHandler) jpeg.InfoReadyHandler();
			jpeg[JPEGReg::CR].rstof(_JPEG_CR_POS_HPDIE);
			jpeg[JPEGReg::CFR].setof(_JPEG_CFR_POS_CHPDF);
		}
		// end of conversion
		if (jpeg[JPEGReg::SR].bitof(_JPEG_SR_POS_EOCF)) {
			jpeg.Context |= _JPEG_CONTEXT_ENDING_DMA;
			jpeg[JPEGReg::CONFR0].rstof(0);// STOP
			jpeg[JPEGReg::CR] &= ~_JPEG_INTERRUPT_MASK;
			jpeg[JPEGReg::CFR] = 0x30;
			if (jpeg.hdmain && jpeg.hdmain->getState() == _MDMA_STATE_BUSY)
				jpeg.hdmain->AbortRupt();
			if (jpeg.hdmaout && jpeg.hdmaout->getState() == _MDMA_STATE_BUSY)
				jpeg.hdmaout->AbortRupt();
			else
				DMAEndProcess(jpeg);
		}
	}

	// AKA JPEG_DMA_EndProcess: compute residual output, fire DataReady/completion callbacks
	void JPEG_HARD::DMAEndProcess(JPEG_HARD& jpeg) {
		stduint tmpContext;
		jpeg.JpegOutCount = jpeg.out_buffer.length;
		if (jpeg.hdmaout)
			jpeg.JpegOutCount = jpeg.out_buffer.length - ((*jpeg.hdmaout)[MDMAReg::Chan::CBNDTR].mask(0, 17));
		// output FIFO drained?
		if (!jpeg[JPEGReg::SR].bitof(_JPEG_SR_POS_OFNEF)) {
			if (jpeg.JpegOutCount > 0) jpeg.JpegOutCount = 0;
			jpeg[JPEGReg::CONFR0].rstof(0);// STOP
			tmpContext = jpeg.Context;
			jpeg.Context &= (_JPEG_CONTEXT_CONF_ENCODING | _JPEG_CONTEXT_CUSTOM_TABLES);
			jpeg.State = JPEGState::Ready;
			if ((tmpContext & _JPEG_CONTEXT_OPERATION_MASK) == _JPEG_CONTEXT_DECODE) {
				if (jpeg.DecodeCpltHandler) jpeg.DecodeCpltHandler();
			}
			else {
				if (jpeg.EncodeCpltHandler) jpeg.EncodeCpltHandler();
			}
		}
		else if (!(jpeg.Context & _JPEG_CONTEXT_PAUSE_OUTPUT)) {
			DMAPollResidual(jpeg);
		}
	}

	// AKA JPEG_DMA_PollResidualData: drain leftover output FIFO words after EOC
	void JPEG_HARD::DMAPollResidual(JPEG_HARD& jpeg) {
		stduint tmpContext, count = _JPEG_FIFO_SIZE;
		while (jpeg[JPEGReg::SR].bitof(_JPEG_SR_POS_OFNEF) && (count > 0) && !(jpeg.Context & _JPEG_CONTEXT_PAUSE_OUTPUT)) {
			count--;
			if (jpeg.out_buffer.length >= (jpeg.JpegOutCount + 4)) {
				stduint d = jpeg[JPEGReg::DOR];
				byte* outptr = (byte*)jpeg.out_buffer.address;
				outptr[jpeg.JpegOutCount] = (byte)(d & 0xFF);
				outptr[jpeg.JpegOutCount + 1] = (byte)((d >> 8) & 0xFF);
				outptr[jpeg.JpegOutCount + 2] = (byte)((d >> 16) & 0xFF);
				outptr[jpeg.JpegOutCount + 3] = (byte)((d >> 24) & 0xFF);
				jpeg.JpegOutCount += 4;
			}
			else break;
		}
		if (!(jpeg.Context & _JPEG_CONTEXT_PAUSE_OUTPUT)) {
			jpeg[JPEGReg::CONFR0].rstof(0);// STOP
			if (jpeg.JpegOutCount > 0) jpeg.JpegOutCount = 0;
			tmpContext = jpeg.Context;
			jpeg.Context &= (_JPEG_CONTEXT_CONF_ENCODING | _JPEG_CONTEXT_CUSTOM_TABLES);
			jpeg.State = JPEGState::Ready;
			if ((tmpContext & _JPEG_CONTEXT_OPERATION_MASK) == _JPEG_CONTEXT_DECODE) {
				if (jpeg.DecodeCpltHandler) jpeg.DecodeCpltHandler();
			}
			else {
				if (jpeg.EncodeCpltHandler) jpeg.EncodeCpltHandler();
			}
		}
	}

	// AKA JPEG_MDMAInCpltCallback: input transfer done; restart if more input remains
	void JPEG_HARD::MDMAInCplt() {
		JPEG_HARD& jpeg = JPEG;
		jpeg[JPEGReg::CR] &= ~_JPEG_INTERRUPT_MASK;
		if (((jpeg.Context & _JPEG_CONTEXT_METHOD_MASK) == _JPEG_CONTEXT_DMA) && !(jpeg.Context & _JPEG_CONTEXT_ENDING_DMA)) {
			jpeg.JpegInCount = jpeg.inn_buffer.length;
			// all input consumed; JPEG keeps running, EOC will come via IRQ
			jpeg[JPEGReg::CR].setof(_JPEG_CR_POS_EOCIE);
			jpeg[JPEGReg::CR].setof(_JPEG_CR_POS_HPDIE);
		}
	}

	// AKA JPEG_MDMAOutCpltCallback: output transfer done; restart until EOC
	void JPEG_HARD::MDMAOutCplt() {
		JPEG_HARD& jpeg = JPEG;
		jpeg[JPEGReg::CR] &= ~_JPEG_INTERRUPT_MASK;
		if (((jpeg.Context & _JPEG_CONTEXT_METHOD_MASK) == _JPEG_CONTEXT_DMA) && !(jpeg.Context & _JPEG_CONTEXT_ENDING_DMA)) {
			if (!jpeg[JPEGReg::SR].bitof(_JPEG_SR_POS_EOCF)) {
				jpeg.JpegOutCount = jpeg.out_buffer.length;
				if (jpeg.hdmaout && !(jpeg.Context & _JPEG_CONTEXT_PAUSE_OUTPUT))
					jpeg.hdmaout->Transfer((pureptr_t)(_JPEG_ADDR + 0x44), (pureptr_t)jpeg.out_buffer.address, jpeg.out_buffer.length, 1, IOMethod::Rupt);
			}
			jpeg[JPEGReg::CR].setof(_JPEG_CR_POS_EOCIE);
			jpeg[JPEGReg::CR].setof(_JPEG_CR_POS_HPDIE);
		}
	}

	// AKA JPEG_MDMAErrorCallback: stop, mark error, fire ErrorHandler
	void JPEG_HARD::MDMAError() {
		JPEG_HARD& jpeg = JPEG;
		jpeg[JPEGReg::CONFR0].rstof(0);// STOP
		jpeg[JPEGReg::CR] &= ~_JPEG_INTERRUPT_MASK;
		jpeg.State = JPEGState::Ready;
		jpeg.ErrorCode |= _JPEG_ERROR_DMA;
		if (jpeg.ErrorHandler) jpeg.ErrorHandler();
	}

	// AKA JPEG_MDMAOutAbortCallback: when ending DMA, finalize
	void JPEG_HARD::MDMAOutAbort() {
		if (JPEG.Context & _JPEG_CONTEXT_ENDING_DMA)
			DMAEndProcess(JPEG);
	}

	// ---- public API ----

	void JPEG_HARD::enClock(bool ena) {
		Reference(_RCC_AHB3ENR_ADDR).setof(5, ena);// JPGDECEN
	}

	void JPEG_HARD::enAble(bool ena) {
		self[JPEGReg::CR].setof(_JPEG_CR_POS_JCEN, ena);
	}

	// AKA HAL_JPEG_Init
	bool JPEG_HARD::setMode() {
		if (State == JPEGState::Reset) {
			enClock();
			// AKA HAL_JPEG_Init: enable core, stop process, disable IT, flush FIFOs, clear flags
			enAble(true);
			self[JPEGReg::CONFR0].rstof(0);// STOP
			self[JPEGReg::CR] &= ~_JPEG_INTERRUPT_MASK;
			self[JPEGReg::CR].setof(_JPEG_CR_POS_IFF);
			self[JPEGReg::CR].setof(_JPEG_CR_POS_OFF);
			self[JPEGReg::CFR] = 0x30;// clear EOCF|HPDF (CFR bits 4/5)
			if (!_JPEG_Set_HuffEnc_Mem(self)) {
				ErrorCode |= _JPEG_ERROR_HUFF_TABLE;
				State = JPEGState::Error;
				return false;
			}
			// enable header parsing (decode side)
			self[JPEGReg::CONFR1].setof(_JPEG_CONFR1_POS_HDR);
			JpegInCount = 0;
			JpegOutCount = 0;
			ErrorCode = _JPEG_ERROR_NONE;
			Context = 0;
			State = JPEGState::Ready;
			return true;
		}
		return false;
	}

	// AKA HAL_JPEG_ConfigEncoding
	bool JPEG_HARD::ConfigEncoding() {
		if (State != JPEGState::Ready) return false;
		if (!image_width || !image_height || !image_quality) return false;
		State = JPEGState::Busy;
		Reference confr1 = self[JPEGReg::CONFR1];
		JPEGColorSpace cs = color_space;
		confr1.maset(_JPEG_CONFR1_POS_COLORSPACE, 2, (stduint)cs);
		// sampling
		JPEGSubsampling ss = subsampling;
		if (cs == JPEGColorSpace::Gray) ss = JPEGSubsampling::_444;
		_JPEG_SetColor(self, cs, ss);
		// quantization tables
		const byte* qt0 = quantTable0 ? quantTable0 : _JPEG_LUM_QuantTable;
		const byte* qt1 = quantTable1 ? quantTable1 : _JPEG_CHROM_QuantTable;
		const byte* qt2 = quantTable2;
		const byte* qt3 = quantTable3;
		if (quantTable0 || quantTable1 || quantTable2 || quantTable3) Context |= _JPEG_CONTEXT_CUSTOM_TABLES;
		if (!_JPEG_Set_Quantization_Mem(self, qt0, JPEGReg::QMEM0, image_quality)) {
			ErrorCode |= _JPEG_ERROR_QUANT_TABLE;
			State = JPEGState::Error;
			return false;
		}
		if (cs == JPEGColorSpace::YCbCr) {
			if (!_JPEG_Set_Quantization_Mem(self, qt1, JPEGReg::QMEM1, image_quality)) {
				ErrorCode |= _JPEG_ERROR_QUANT_TABLE;
				State = JPEGState::Error;
				return false;
			}
			if (Context & _JPEG_CONTEXT_CUSTOM_TABLES) {
				confr1.setof(_JPEG_CONFR1_POS_COLORSPACE + 1);
				if (qt2 && !_JPEG_Set_Quantization_Mem(self, qt2, JPEGReg::QMEM2, image_quality)) {
					ErrorCode |= _JPEG_ERROR_QUANT_TABLE;
					State = JPEGState::Error;
					return false;
				}
				self[JPEGReg::CONFR5].maset(_JPEG_CONFRx_POS_QT, 2, 1);
				self[JPEGReg::CONFR6].maset(_JPEG_CONFRx_POS_QT, 2, 2);
			}
		}
		else if (cs == JPEGColorSpace::CMYK) {
			if (Context & _JPEG_CONTEXT_CUSTOM_TABLES) {
				confr1.setof(_JPEG_CONFR1_POS_COLORSPACE + 1);
				if (qt1 && !_JPEG_Set_Quantization_Mem(self, qt1, JPEGReg::QMEM1, image_quality)) {
					ErrorCode |= _JPEG_ERROR_QUANT_TABLE;
					State = JPEGState::Error;
					return false;
				}
				if (qt2 && !_JPEG_Set_Quantization_Mem(self, qt2, JPEGReg::QMEM2, image_quality)) {
					ErrorCode |= _JPEG_ERROR_QUANT_TABLE;
					State = JPEGState::Error;
					return false;
				}
				if (qt3 && !_JPEG_Set_Quantization_Mem(self, qt3, JPEGReg::QMEM3, image_quality)) {
					ErrorCode |= _JPEG_ERROR_QUANT_TABLE;
					State = JPEGState::Error;
					return false;
				}
				self[JPEGReg::CONFR5].maset(_JPEG_CONFRx_POS_QT, 2, 1);
				self[JPEGReg::CONFR6].maset(_JPEG_CONFRx_POS_QT, 2, 2);
				self[JPEGReg::CONFR7].maset(_JPEG_CONFRx_POS_QT, 2, 3);
			}
		}
		// image size
		confr1.maset(_JPEG_CONFR1_POS_YSIZE, 16, image_height & 0xFFFF);
		self[JPEGReg::CONFR3].maset(16, 16, image_width & 0xFFFF);
		// MCU count
		stduint hfactor = 8, vfactor = 8;
		if (ss == JPEGSubsampling::_420) { hfactor = 16; vfactor = 16; }
		else if (ss == JPEGSubsampling::_422) { hfactor = 16; vfactor = 8; }
		stduint hMCU = image_width / hfactor + ((image_width % hfactor) ? 1 : 0);
		stduint vMCU = image_height / vfactor + ((image_height % vfactor) ? 1 : 0);
		self[JPEGReg::CONFR2] = (hMCU * vMCU - 1) & 0x3FFFFFFU;
		Context |= _JPEG_CONTEXT_CONF_ENCODING;
		State = JPEGState::Ready;
		return true;
	}

	// AKA HAL_JPEG_DeInit
	void JPEG_HARD::canMode() {
		enAble(false);
		enClock(false);
		JpegInCount = 0;
		JpegOutCount = 0;
		inn_buffer = { 0, 0 };
		out_buffer = { 0, 0 };
		ErrorCode = _JPEG_ERROR_NONE;
		Context = 0;
		State = JPEGState::Reset;
	}

	// AKA HAL_JPEG_EnableHeaderParsing / DisableHeaderParsing
	void JPEG_HARD::setHeaderParsing(bool ena) {
		self[JPEGReg::CONFR1].setof(_JPEG_CONFR1_POS_HDR, ena);
	}

	// AKA HAL_JPEG_GetInfo
	bool JPEG_HARD::getInfo(JPEG_INFO& info) {
		if (State != JPEGState::Ready && State != JPEGState::Busy) return false;
		Reference confr1 = self[JPEGReg::CONFR1];
		stduint nf = confr1.masof(_JPEG_CONFR1_POS_NF, 2);
		info.width = self[JPEGReg::CONFR3].masof(16, 16);
		info.height = confr1.masof(_JPEG_CONFR1_POS_YSIZE, 16);
		info.precision = 8;
		info.ncomp = (byte)nf;
		if (nf == 1) {
			info.colorSpace = uni::ColorSpace::GRAY;
			info.subsampling = JPEGSubsampling::_444;
		}
		else if (nf == 3) {
			info.colorSpace = uni::ColorSpace::SRGB;
			stduint yb = self[JPEGReg::CONFR4].masof(_JPEG_CONFRx_POS_NB, 4);
			if (yb == 0) info.subsampling = JPEGSubsampling::_444;
			else if (yb == 1) info.subsampling = JPEGSubsampling::_422;
			else info.subsampling = JPEGSubsampling::_420;
		}
		else {
			info.colorSpace = uni::ColorSpace::CMYK;
			info.subsampling = JPEGSubsampling::_444;
		}
		info.quality = 0;// only valid at end of decode; see HAL note
		return true;
	}

	// AKA HAL_JPEG_SetUserQuantTables (fields already assigned; mark custom and re-apply)
	bool JPEG_HARD::setQuantTables() {
		Context |= _JPEG_CONTEXT_CUSTOM_TABLES;
		return true;
	}

	// AKA JPEG_Init_Process
	void JPEG_HARD::InitializeProcess() {
		Context &= ~(_JPEG_CONTEXT_PAUSE_INPUT | _JPEG_CONTEXT_PAUSE_OUTPUT);
		if ((Context & _JPEG_CONTEXT_OPERATION_MASK) == _JPEG_CONTEXT_DECODE)
			self[JPEGReg::CONFR1].setof(_JPEG_CONFR1_POS_DE);
		else
			self[JPEGReg::CONFR1].rstof(_JPEG_CONFR1_POS_DE);
		self[JPEGReg::CONFR0].rstof(0);// STOP
		self[JPEGReg::CR] &= ~_JPEG_INTERRUPT_MASK;
		self[JPEGReg::CR].setof(_JPEG_CR_POS_IFF);
		self[JPEGReg::CR].setof(_JPEG_CR_POS_OFF);
		self[JPEGReg::CFR] = 0x30;// clear EOCF|HPDF
		self[JPEGReg::CONFR0].setof(0);// START
		if ((Context & _JPEG_CONTEXT_METHOD_MASK) == _JPEG_CONTEXT_IT) {
			self[JPEGReg::CR].setof(_JPEG_CR_POS_IFTIE);
			self[JPEGReg::CR].setof(_JPEG_CR_POS_IFNFIE);
			self[JPEGReg::CR].setof(_JPEG_CR_POS_OFTIE);
			self[JPEGReg::CR].setof(_JPEG_CR_POS_OFNEIE);
			self[JPEGReg::CR].setof(_JPEG_CR_POS_EOCIE);
			self[JPEGReg::CR].setof(_JPEG_CR_POS_HPDIE);
		}
		else if ((Context & _JPEG_CONTEXT_METHOD_MASK) == _JPEG_CONTEXT_DMA) {
			self[JPEGReg::CR].setof(_JPEG_CR_POS_EOCIE);
			self[JPEGReg::CR].setof(_JPEG_CR_POS_HPDIE);
		}
	}

	// AKA JPEG_ReadInputData
	void JPEG_HARD::ReadInput(stduint nbRequestWords) {
		byte* inptr = (byte*)inn_buffer.address;
		if (inn_buffer.length == 0 || nbRequestWords == 0) {
			Context |= _JPEG_CONTEXT_PAUSE_INPUT;
			return;
		}
		stduint nbBytes;
		if (inn_buffer.length > JpegInCount) nbBytes = inn_buffer.length - JpegInCount;
		else if (inn_buffer.length == JpegInCount) {
			// no more data in current buffer; pause (unisym: no GetData chaining)
			Context |= _JPEG_CONTEXT_PAUSE_INPUT;
			return;
		}
		else return;
		if ((nbBytes > 0) && !(Context & _JPEG_CONTEXT_PAUSE_INPUT)) {
			stduint nBwords = nbBytes / 4;
			if (nBwords >= nbRequestWords) {
				for (stduint index = 0; index < nbRequestWords; index++) {
					stduint ic = JpegInCount;
					self[JPEGReg::DIR] = stduint(inptr[ic]) | (stduint(inptr[ic + 1]) << 8)
						| (stduint(inptr[ic + 2]) << 16) | (stduint(inptr[ic + 3]) << 24);
					JpegInCount += 4;
				}
			}
			else if (nBwords > 0) {
				for (stduint index = 0; index < nBwords; index++) {
					stduint ic = JpegInCount;
					self[JPEGReg::DIR] = stduint(inptr[ic]) | (stduint(inptr[ic + 1]) << 8)
						| (stduint(inptr[ic + 2]) << 16) | (stduint(inptr[ic + 3]) << 24);
					JpegInCount += 4;
				}
			}
			else {
				// end of file: pack remaining bytes
				stduint dataword = 0;
				for (stduint index = 0; index < nbBytes; index++) {
					dataword |= stduint(inptr[JpegInCount]) << (8 * index);
					JpegInCount++;
				}
				self[JPEGReg::DIR] = dataword;
			}
		}
	}

	// AKA JPEG_StoreOutputData
	void JPEG_HARD::StoreOutput(stduint nbOutputWords) {
		byte* outptr = (byte*)out_buffer.address;
		for (stduint index = 0; index < nbOutputWords; index++) {
			if (out_buffer.length >= (JpegOutCount + 4)) {
				stduint d = self[JPEGReg::DOR];
				outptr[JpegOutCount] = (byte)(d & 0xFF);
				outptr[JpegOutCount + 1] = (byte)((d >> 8) & 0xFF);
				outptr[JpegOutCount + 2] = (byte)((d >> 16) & 0xFF);
				outptr[JpegOutCount + 3] = (byte)((d >> 24) & 0xFF);
				JpegOutCount += 4;
			}
			else break;
		}
	}

	// AKA JPEG_Process (polling/IT FIFO pump). Returns true when EOC reached.
	bool JPEG_HARD::ProcessPump() {
		// header parsing done (decode)
		if (((Context & _JPEG_CONTEXT_OPERATION_MASK) == _JPEG_CONTEXT_DECODE) && self[JPEGReg::SR].bitof(_JPEG_SR_POS_HPDF)) {
			JPEG_INFO info;
			if (getInfo(info) && InfoReadyHandler) InfoReadyHandler();
			self[JPEGReg::CR].rstof(_JPEG_CR_POS_HPDIE);
			self[JPEGReg::CFR].setof(_JPEG_CFR_POS_CHPDF);
		}
		// input FIFO
		if (!(Context & _JPEG_CONTEXT_PAUSE_INPUT)) {
			if (self[JPEGReg::SR].bitof(_JPEG_SR_POS_IFTF)) ReadInput(_JPEG_FIFO_TH_SIZE);
			else if (self[JPEGReg::SR].bitof(_JPEG_SR_POS_IFNFF)) ReadInput(1);
		}
		// output FIFO
		if (!(Context & _JPEG_CONTEXT_PAUSE_OUTPUT)) {
			if (self[JPEGReg::SR].bitof(_JPEG_SR_POS_OFTF)) StoreOutput(_JPEG_FIFO_TH_SIZE);
			else if (self[JPEGReg::SR].bitof(_JPEG_SR_POS_OFNEF)) StoreOutput(1);
		}
		// end of conversion: EOCF set and both output flags clear
		stduint sr = self[JPEGReg::SR];
		if ((sr & ((1U << _JPEG_SR_POS_EOCF) | (1U << _JPEG_SR_POS_OFTF) | (1U << _JPEG_SR_POS_OFNEF))) == (1U << _JPEG_SR_POS_EOCF)) {
			self[JPEGReg::CONFR0].rstof(0);// STOP
			if ((Context & _JPEG_CONTEXT_METHOD_MASK) == _JPEG_CONTEXT_IT)
				self[JPEGReg::CR] &= ~_JPEG_INTERRUPT_MASK;
			self[JPEGReg::CFR] = 0x30;
			// DataReady callback for trailing bytes
			if (JpegOutCount > 0) JpegOutCount = 0;
			stduint tmpContext = Context;
			Context &= (_JPEG_CONTEXT_CONF_ENCODING | _JPEG_CONTEXT_CUSTOM_TABLES);
			State = JPEGState::Ready;
			if ((tmpContext & _JPEG_CONTEXT_OPERATION_MASK) == _JPEG_CONTEXT_DECODE) {
				if (DecodeCpltHandler) DecodeCpltHandler();
			}
			else {
				if (EncodeCpltHandler) EncodeCpltHandler();
			}
			return true;
		}
		return false;
	}

	bool JPEG_HARD::Encode(const byte* pDataInMCU, stduint inLen, byte* pDataOut, stduint outLen, IOMethod method) {
		if (!pDataInMCU || !pDataOut || inLen < 4 || outLen < 4) return false;
		if (State != JPEGState::Ready) return false;
		if (!(Context & _JPEG_CONTEXT_CONF_ENCODING)) return false;
		State = JPEGState::BusyEncoding;
		Context &= ~(_JPEG_CONTEXT_OPERATION_MASK | _JPEG_CONTEXT_METHOD_MASK);
		Context |= _JPEG_CONTEXT_ENCODE | (method == IOMethod::Rupt ? _JPEG_CONTEXT_IT : method == IOMethod::DMA ? _JPEG_CONTEXT_DMA : _JPEG_CONTEXT_POLLING);
		inLen -= inLen % 4;
		outLen -= outLen % 4;
		inn_buffer = { (stduint)pDataInMCU, inLen };
		out_buffer = { (stduint)pDataOut, outLen };
		JpegInCount = 0;
		JpegOutCount = 0;
		InitializeProcess();
		if (method == IOMethod::DMA) {
			// JPEG in/out FIFOs driven by MDMA hardware requests.
			// Requires hdmain/hdmaout assigned to MDMA channels configured for
			// MDMA_REQUEST_JPEG_INFIFO_TH / JPEG_OUTFIFO_TH (MDMATrigger::Buffer, bufferLen = 32).
			// The JPEG EOC/HPD interrupts drive the end-of-transfer sequence in HandleIRQ().
			if (!JPEG_HARD::DMAStartProcess(*this)) {
				State = JPEGState::Error;
				ErrorCode |= _JPEG_ERROR_DMA;
				return false;
			}
			return true;
		}
		if (method == IOMethod::Loop) {
			uint64 tickstart = SysTick::getTick();
			while (!ProcessPump()) {
				if ((SysTick::getTick() - tickstart) > _JPEG_TIMEOUT_VALUE) {
					ErrorCode |= _JPEG_ERROR_TIMEOUT;
					State = JPEGState::Ready;
					return false;
				}
			}
			return true;
		}
		// Rupt: continue in HandleIRQ()
		return true;
	}

	bool JPEG_HARD::Decode(const byte* pDataIn, stduint inLen, byte* pDataOut, stduint outLen, IOMethod method) {
		if (!pDataIn || !pDataOut || inLen < 4 || outLen < 4) return false;
		if (State != JPEGState::Ready) return false;
		State = JPEGState::BusyDecoding;
		Context &= ~(_JPEG_CONTEXT_OPERATION_MASK | _JPEG_CONTEXT_METHOD_MASK);
		Context |= _JPEG_CONTEXT_DECODE | (method == IOMethod::Rupt ? _JPEG_CONTEXT_IT : method == IOMethod::DMA ? _JPEG_CONTEXT_DMA : _JPEG_CONTEXT_POLLING);
		inLen -= inLen % 4;
		outLen -= outLen % 4;
		inn_buffer = { (stduint)pDataIn, inLen };
		out_buffer = { (stduint)pDataOut, outLen };
		JpegInCount = 0;
		JpegOutCount = 0;
		InitializeProcess();
		if (method == IOMethod::DMA) {
			if (!JPEG_HARD::DMAStartProcess(*this)) {
				State = JPEGState::Error;
				ErrorCode |= _JPEG_ERROR_DMA;
				return false;
			}
			return true;
		}
		if (method == IOMethod::Loop) {
			uint64 tickstart = SysTick::getTick();
			while (!ProcessPump()) {
				if ((SysTick::getTick() - tickstart) > _JPEG_TIMEOUT_VALUE) {
					ErrorCode |= _JPEG_ERROR_TIMEOUT;
					State = JPEGState::Ready;
					return false;
				}
			}
			return true;
		}
		return true;
	}

	// AKA HAL_JPEG_Pause
	bool JPEG_HARD::Pause(stduint sel) {
		stduint mask = 0;
		if ((Context & _JPEG_CONTEXT_METHOD_MASK) == _JPEG_CONTEXT_DMA) {
			if (sel & JPEG_PAUSE_INPUT) { Context |= _JPEG_CONTEXT_PAUSE_INPUT; }
			if (sel & JPEG_PAUSE_OUTPUT) { Context |= _JPEG_CONTEXT_PAUSE_OUTPUT; }
			return true;
		}
		if ((Context & _JPEG_CONTEXT_METHOD_MASK) == _JPEG_CONTEXT_IT) {
			if (sel & JPEG_PAUSE_INPUT) { Context |= _JPEG_CONTEXT_PAUSE_INPUT; mask |= (1U << _JPEG_CR_POS_IFTIE) | (1U << _JPEG_CR_POS_IFNFIE); }
			if (sel & JPEG_PAUSE_OUTPUT) { Context |= _JPEG_CONTEXT_PAUSE_OUTPUT; mask |= (1U << _JPEG_CR_POS_OFTIE) | (1U << _JPEG_CR_POS_OFNEIE) | (1U << _JPEG_CR_POS_EOCIE); }
			self[JPEGReg::CR] &= ~mask;
		}
		return true;
	}

	// AKA HAL_JPEG_Resume
	bool JPEG_HARD::Resume(stduint sel) {
		stduint mask = 0;
		if ((Context & _JPEG_CONTEXT_METHOD_MASK) == _JPEG_CONTEXT_IT) {
			if (sel & JPEG_PAUSE_INPUT) { Context &= ~_JPEG_CONTEXT_PAUSE_INPUT; mask |= (1U << _JPEG_CR_POS_IFTIE) | (1U << _JPEG_CR_POS_IFNFIE); }
			if (sel & JPEG_PAUSE_OUTPUT) { Context &= ~_JPEG_CONTEXT_PAUSE_OUTPUT; mask |= (1U << _JPEG_CR_POS_OFTIE) | (1U << _JPEG_CR_POS_OFNEIE) | (1U << _JPEG_CR_POS_EOCIE); }
			self[JPEGReg::CR] |= mask;
		}
		return true;
	}

	// AKA HAL_JPEG_Abort
	bool JPEG_HARD::Abort() {
		stduint tmpContext = Context;
		Context &= ~(_JPEG_CONTEXT_OPERATION_MASK | _JPEG_CONTEXT_METHOD_MASK | _JPEG_CONTEXT_ENDING_DMA);
		if ((tmpContext & _JPEG_CONTEXT_METHOD_MASK) == _JPEG_CONTEXT_DMA) {
			if (hdmaout) hdmaout->AbortRupt();
			if (hdmain) hdmain->AbortRupt();
		}
		self[JPEGReg::CONFR0].rstof(0);// STOP
		uint64 tickstart = SysTick::getTick();
		while (self[JPEGReg::SR].bitof(_JPEG_SR_POS_COF)) {
			if ((SysTick::getTick() - tickstart) > _JPEG_TIMEOUT_VALUE) {
				ErrorCode |= _JPEG_ERROR_TIMEOUT;
				State = JPEGState::Timeout;
				return false;
			}
		}
		self[JPEGReg::CR] &= ~_JPEG_INTERRUPT_MASK;
		self[JPEGReg::CR].setof(_JPEG_CR_POS_IFF);
		self[JPEGReg::CR].setof(_JPEG_CR_POS_OFF);
		self[JPEGReg::CFR] = 0x30;
		JpegInCount = 0;
		JpegOutCount = 0;
		Context &= ~(_JPEG_CONTEXT_PAUSE_INPUT | _JPEG_CONTEXT_PAUSE_OUTPUT);
		State = JPEGState::Ready;
		return true;
	}

	// ---- RuptTrait (NVIC + IRQ_JPEG) ----
	void JPEG_HARD::setInterrupt(Handler_t f) const {
		FUNC_JPEG[0] = f;
	}
	void JPEG_HARD::setInterruptPriority(byte preempt, byte sub_priority) const {
		NVIC.setPriority(IRQ_JPEG, preempt, sub_priority);
	}
	void JPEG_HARD::enInterrupt(bool enable) const {
		NVIC.setAble(IRQ_JPEG, enable);
	}

#endif // _MCU_STM32H7x
}
