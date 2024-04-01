// ASCII C++ TAB4 CRLF
// Attribute: 
// LastCheck: 20240331
// AllAuthor: @dosconio
// ModuTitle: Arinae Flag
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

#ifndef _INC_FLAG
#define _INC_FLAG

#include "integer.h"

// ==== ==== <mcore.c> ==== ====

extern struct _aflag_t// for some unisym functions, stored in stack for calling other functions. Method of application: register the current address of aflag globally before calling corresponding functions.
{
	byte carry : 1;                    // CF
	byte autosort : 1;                 ///1 or ASF (Auto Sort Flag) 
	byte parity : 1;                   // PF
	byte signsym : 1;                  ///0 or SEF (Signed Operand Flag)
	byte auxiliary : 1;                // AF
	byte one : 1;                      ///0 or ONF (One Flag)
	byte zero : 1;                     // ZF
	byte sign : 1;                     // SF
	byte debug : 1;                    // TF (Trap Flag)
	byte interrupt : 1;                // IF
	byte direction : 1;                // DF (0 for increasing, 1 for decreasing)
	byte overflow : 1;                 // OF or precies loss
	byte io_privilege_level : 2;       // IOPL
	byte nested_task : 1;              // NT
	byte fail : 1;                     ///0 or FF (Fail Flag)
	//
	byte resume : 1;                   // RF
	byte virtual_8086 : 1;             // VM
	byte alignment_check : 1;          // AC
	byte virtual_interrupt : 1;        // VIF
	byte virtual_interrupt_pending : 1;// VIP
	byte identification : 1;           // ID
	byte : 2;
	byte : 8;
} aflaga;

#define _aflaga_init() (MemSet(&aflaga, 0, sizeof aflaga), aflaga.autosort = 1)

#endif
