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

#include "../stdinc.h"

#define DEF_COLOR(v,iden) iden=v

#ifdef _INC_CPP
namespace uni {
#endif

	enum
		#ifdef _INC_CPP
		class
		#endif
		PixelFormat {
		ARGB8888 = 0x00,// DB B,G,R,A <=> DD 0xAARRGGBB
		ABGR8888 = 0x01,// DB R,G,B,A <=> DD 0xAABBGGRR
		RGBA8888 = 0x02,
		BGRA8888 = 0x03,
		//
		RGB565 = 0x04,
		BGR565 = 0x05,
		RGB888 = 0x06,
		ARGB1555 = 0x07,
		ARGB4444 = 0x08,
		L8 = 0x09,
		AL44 = 0x0A,
		AL88 = 0x0B,
		//
		UYVY = 0x0C,
		VYUY = 0x0D,
		YUYV = 0x0E,
		YVYU = 0x0F,
		//
		NV12 = 0x10,
		NV21 = 0x11,
		YUV420 = 0x12,
		YVU420 = 0x13
	};
	
	//[ATTR] little-endian, argb
	struct alignas(byteof(uint32)) Color {
		union {
			struct { byte b, g, r, a; }; // union {x y z i}
			uint32 val;
		};
		#ifdef _INC_CPP
		enum ColorIdentifier : uint32 {
			// ❤
			DEF_COLOR(0xFFF0F8FF, AliceBlue),

			// classic
			DEF_COLOR(0xFFFFFFFF, White),
			DEF_COLOR(0xFF000000, Black),
			DEF_COLOR(0xFFFF0000, Red),
			DEF_COLOR(0xFFFFFF00, Yellow),
			DEF_COLOR(0xFF00FF00, Green),
			DEF_COLOR(0xFF0000FF, Blue),
			
			// light
			DEF_COLOR(0xFFC0C0C0, Silver),
			DEF_COLOR(0xFFFFE4E1, MistyRose),
			
			// dark
			DEF_COLOR(0xFF800000, Maroon),
			DEF_COLOR(0xFF6A5ACD, SlateBlue),
		};
		//

		Color(uint32 i = 0) { *(uint32*)this = i; }

		static Color FromRGB888(uint32 argb);
		static Color FromBGR565(uint16 col);

		//{TODO} static HSLA

		operator uint32() const { return *(uint32*)this; }

		uint16 ToRGB565() const {
			// R5[11] G6[5] B5[0]
			uint16 _r = r >> (8 - 5), _g = g >> (8 - 6);
			uint16 res = (uint16)(b >> (8 - 5));
			res |= _g << 5;
			res |= _r << 11;
			return res;
		}

		uint32 ToBGR888() const {
			union {
				uint32 ret32;
				struct { uint16 lows, highs; };
			} ret;
			ret.ret32 = uint32(self) & 0x00FFFFFF;
			ret.lows ^= ret.highs;
			ret.highs ^= ret.lows;
			ret.highs &= 0x00FF;
			ret.lows ^= ret.highs;
			return ret.ret32;
		}
		#endif
	};




#ifdef _INC_CPP
}
#endif




#endif
