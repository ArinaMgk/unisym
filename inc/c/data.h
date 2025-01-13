// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Data) Inline Data Set in UNISYM
// Codifiers: @dosconio: 20240422 ~ <Last-check> 
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

#ifndef _INC_HeaderTemplate_X
#define _INC_HeaderTemplate_X

#include "stdinc.h"

#if defined(_INC_CPP)
namespace uni {
	extern "C" {
	#else

#endif

// 0x20 .. 0x7F
extern byte _BITFONT_ASCII_8x5[95][5];
extern byte _BITFONT_ASCII_16x8[95][16];


#if defined(_INC_CPP)
	} //: C++ Area

} //END C++ Area
#else//: C Area

#endif
#endif
