// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: Multi-byte Char Set
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

#ifndef _INC_CHAR_MULTIBYTE
#define _INC_CHAR_MULTIBYTE

#include "charset/charset.h"

#if defined(_OPT_CHARSET_UTF8)
#include "charset/multichar/UTF-8.h"
#endif
#if defined(_OPT_CHARSET_UTF16)
typedef uint16 unit_utf16_t;
typedef struct {
	unit_utf16_t unit[2];
} char_utf16_t;
#endif

#ifdef _INC_CPP
extern "C" {
#endif
	
	stduint CscUTF(byte from, byte to, const pureptr_t src, stduint slen, pureptr_t* des);

	// return the length of the next UTF-8 char
	stduint ScanUTF8(byte* src);

#ifdef _INC_CPP
}
#endif
#endif
