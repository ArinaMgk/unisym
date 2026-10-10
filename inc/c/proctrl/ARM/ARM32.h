// ASCII C99 TAB4 CRLF
// Attribute: Little-Endian(Byte, Bit)
// AllAuthor: @ArinaMgk
// ModuTitle: General Header for ARM CPU
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

// called by ARM.h

#include "../../stdinc.h"
#ifndef _INC_ARM32
#define _INC_ARM32


// en/disable device interrupts
static inline void enInterrupt(int enable) {
	if (enable)
		_ASM volatile("cpsie i" : : : "memory");
	else
		_ASM volatile("cpsid i" : : : "memory");
}

static inline int getInterrupt() {
#if defined(_MCCA) && ((_MCCA >> 24) == 0x1A)// Cortex-M: PRIMASK, not the A-profile CPSR I bit
	stduint primask;
	_ASM volatile("mrs %0, primask" : "=r"(primask));
	return primask == 0;
#else
	stduint cpsr;
	_ASM volatile("mrs %0, cpsr" : "=r"(cpsr));
	return (cpsr & 0x80) == 0;
#endif
}

#endif // _INC_ARM32
