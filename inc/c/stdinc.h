// ASCII C99 TAB4 CRLF
// Docutitle: (Common) Standard Include File and Basic Types and Characters of the Machine
// Codifiers: @dosconio: 20240423 ~ 20240423
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
// Descripts: This file is supplementary for C/C++, while Magice builds these in.
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

#ifndef _INC_MACHINE
#define _INC_MACHINE
#if defined(_INC_CPP) || defined(__cplusplus)
extern "C" {
#endif
#include "proctrl/proctrl.h"

#if defined(_INC_CPP) || defined(__cplusplus)
}
#endif

#include "system/host.h"

#if defined(_INC_CPP) || defined(__cplusplus)
extern "C" {
#endif

#include "system/supple.h"
#if defined(_dbg) || defined(_DEBUG)
#include "system/debug.h"
#endif
	extern char _tab_HEXA[16];
	extern char _tab_hexa[16];
//	
#if defined(_INC_CPP) || defined(__cplusplus)
}
#endif
#ifdef _STYLE_ABBR
#include "datype/prefabbr.h"
#endif
#ifdef _STYLE_RUST
#include "datype/ruststyle.h"
#endif
#ifdef _STYLE_CSHARP
#include "datype/csharpstyle.h"
#endif
#endif
