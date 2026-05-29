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

static inline int isalnum(int c) { return ascii_isalnum(c); }

static inline int isalpha(int c) { return ascii_isalpha(c); }

static inline int isblank(int c) { return ascii_isblank(c); }

static inline int iscntrl(int c) { return ascii_iscontrol(c); }

static inline int isdigit(int c) { return ascii_isdigit(c); }

static inline int isgraph(int c) { return ascii_isgraph(c); }

static inline int islower(int c) { return ascii_islower(c); }

static inline int isprint(int c) { return ascii_isprint(c); }

static inline int ispunct(int c) { return ascii_ispunct(c); }

static inline int isspace(int c) { return ascii_isspace(c); }

static inline int isupper(int c) { return ascii_isupper(c); }

static inline int isxdigit(int c) { return ascii_isxdigit(c); }

static inline int tolower(int c) { return ascii_tolower(c); }

static inline int toupper(int c) { return ascii_toupper(c); }

#endif
