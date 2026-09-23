// ASCII C TAB4 CRLF
// Docutitle: (Module) Color
// Codifiers: @dosconio: 20240513
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

#include "../../inc/c/graphic/color.h"

namespace uni {
	static inline byte ColorClipByte(int value) {
		if (value < 0) return 0;
		if (value > 255) return 255;
		return (byte)value;
	}

	Color Color::FromRGB888(uint32 argb) {
		Color color = *(Color*)&argb;
		if (argb && !(argb & 0xFF000000)) argb |= 0xFF000000u;
		return color;
	}

	Color Color::FromBGR565(uint16 col) {
		Color color;
		color.b = (col) & 0x1F;
		color.g = (col >> 5) & 0x3F;
		color.r = (col >> 11) & 0x1F;
		return color;
	}

	Color Color::FromYCbCr(byte y, byte cb, byte cr, YCbCrMatrix matrix, YCbCrRange range) {
		int cr_r, cb_g, cr_g, cb_b;
		if (range == YCbCrRange::Full) {
			if (matrix == YCbCrMatrix::BT709) {
				cr_r = 403; cb_g = 48; cr_g = 120; cb_b = 475;
			} else if (matrix == YCbCrMatrix::BT2020_NCL) {
				cr_r = 378; cb_g = 42; cr_g = 146; cb_b = 482;
			} else {
				cr_r = 359; cb_g = 88; cr_g = 183; cb_b = 454;
			}
		} else {
			if (matrix == YCbCrMatrix::BT709) {
				cr_r = 459; cb_g = 55; cr_g = 136; cb_b = 541;
			} else if (matrix == YCbCrMatrix::BT2020_NCL) {
				cr_r = 430; cb_g = 48; cr_g = 167; cb_b = 548;
			} else {
				cr_r = 409; cb_g = 100; cr_g = 208; cb_b = 516;
			}
		}

		const int y_scale = (range == YCbCrRange::Full) ? 256 : 298;
		int c = (int)y - ((range == YCbCrRange::Full) ? 0 : 16);
		if (c < 0) c = 0;
		int d = (int)cb - 128;
		int e = (int)cr - 128;

		Color color;
		color.r = ColorClipByte((y_scale * c + cr_r * e + 128) >> 8);
		color.g = ColorClipByte((y_scale * c - cb_g * d - cr_g * e + 128) >> 8);
		color.b = ColorClipByte((y_scale * c + cb_b * d + 128) >> 8);
		color.a = 0xFF;
		return color;
	}
}
