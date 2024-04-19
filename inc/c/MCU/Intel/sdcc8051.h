// ASCII C-SDCC TAB4 CRLF
// Attribute: <ArnCovenant> <Env> <bin^%> <CPU()> [Allocation]
// LastCheck: 2024Mar01
// AllAuthor: @dosconio
// ModuTitle: 8051 MCU Header for SDCC
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

#ifndef _INC_MCU_8051_SDCC
#define _INC_MCU_8051_SDCC

#define defbyt(idn,adr) __sfr  __at (adr) idn;
#define defbit(idn,adr) __sbit __at (adr) idn;
#include "i8051.h"

#endif
