// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant
// LastCheck: RFZ26
// AllAuthor: @dosconio
// ModuTitle: Alias for ISO IEC Standard CPL ctype.h
// Compatibl: ISO IEC Standard CPL ctype.h
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

//{TODO} Check(by Exhaustive Table), make experiment export, replace ISO-ctype.h

#ifndef _INC_CTYPE // same with MSVC and GCC
#define _INC_CTYPE

#include "../uctype.h"

#define isalnum ascii_isalnum

#define isalpha ascii_isalpha

#define isblank ascii_isblank

#define iscntrl ascii_iscontrol

#define isdigit ascii_isdigit

#define isgraph ascii_isalpha

#define islower ascii_islower

#define isprint ascii_isprint

#define ispunct ascii_ispunct

#define isspace ascii_isspace

#define isupper ascii_isupper

#define isxdigit ascii_isxdigit

#define tolower ascii_tolower

#define toupper ascii_toupper

#endif
