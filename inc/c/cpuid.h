// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant CPU(80586+)
// LastCheck: RFZ03
// AllAuthor: @dosconio
// ModuTitle: Instruction CPUID
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

#ifndef _INC_CPUID
#define _INC_CPUID

#include "stdinc.h"

#define _CPU_BRAND_SIZE 48

#ifdef _INC_CPP
extern "C" {
#endif

// Get CPU Brand String
void _CALL_FAST CpuBrand(char*);

#ifdef _INC_CPP
}
#endif

#ifdef _AUTO_INCLUDE
	// ../../lib/asm/x86/cpuid.asm
#endif // _AUTO_INCLUDE
#endif // !_INC_CPUID
