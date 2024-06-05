// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: Fixed-Multi-byte Char Set
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

#ifndef _INC_CHAR_WIDE
#define _INC_CHAR_WIDE
// `locale.h` may define relative macros.

#if defined(_OPT_CHARSET_GBK)
#include "charset/widechar/GBK.h"
#elif defined(__cplusplus) || defined(_INC_CPP)
// wchar_t is a keyword
#else // C
typedef short int wchar_t;
#endif

#endif
