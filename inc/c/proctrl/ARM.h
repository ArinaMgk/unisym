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

#include "../stdinc.h"
#ifndef _INC_ARM
#define _INC_ARM


#define _PCUMODE_USR 0x10 // User
#define _PCUMODE_FIQ 0x11 // Fast Interrupt Request
#define _PCUMODE_IRQ 0x12 // Interrupt Request
#define _PCUMODE_SVC 0x13 // Supervisor

#define _PCUMODE_MON 0x16 // Monitor
#define _PCUMODE_ABT 0x17 // Abort
#define _PCUMODE_HYP 0x1A // Hypervisor
#define _PCUMODE_UND 0x1B // Undefined Instruction
#define _PCUMODE_SYS 0x1F // System

// wait for interrupt
static inline void HALT()
{
	_ASM volatile("wfi");
}
// wait for event
static inline void HALT_EVENT()
{
	_ASM volatile("wfe");
}


#endif
