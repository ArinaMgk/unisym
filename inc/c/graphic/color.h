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

#ifndef _INC_Color
#define _INC_Color

#ifdef _INC_CPP
namespace uni {
#endif
	//[ATTR] little-endian
	struct Color {
		byte b, g, r, a; // union {x y z i} 
		enum ColorIdentifier {
			// ❤
			AliceBlue = 0x00F0F8FF,

			// classic
			Red = 0x00FF0000,

			// kinderness
			MistyRose = 0x00FFE4E1,

			// light
			Yellow = 0x00FFFF00,

			// dark
			Black = 0x00000000,
			Maroon = 0x00800000,
			Silver = 0x00C0C0C0,
			SlateBlue = 0x006A5ACD,
		};
		//

		Color(uint32 i = 0) {
			*(uint32*)this = i;
		}
		Color(ColorIdentifier ci) {
			*(uint32*)this = (uint32)ci;
		}

		static Color From32(uint32 argb) {
			Color color = *(Color*)&argb;
			return color;
		}

		static Color FromBGR565(uint16 col) {
			Color color;
			color.b = (col) & 0x1F;
			color.g = (col >> 5) & 0x3F;
			color.r = (col >> 11) & 0x1F;
			return color;
		}

		operator uint32() {
			return *(uint32*)this;
		}

		uint16 ToRGB565() const {
			// R5[11] G6[5] B5[0]
			uint16 _r = r >> (8 - 5), _g = g >> (8 - 6);
			uint16 res = (uint16)(b >> (8 - 5));
			res |= _g << 5;
			res |= _r << 11;
			return res;
		}
	};




#ifdef _INC_CPP
}
#endif




#endif
