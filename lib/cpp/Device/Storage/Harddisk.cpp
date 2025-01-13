// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Stroage) Harddisk
// Codifiers: @dosconio: 20250107
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

#include "../../../../inc/c/storage/harddisk.h"

namespace uni {
#if defined(_DEV_GCC) && defined(_MCCA) && _MCCA == 0x8632
	bool Harddisk_t::Read(stduint BlockIden, void* Dest) {
		stduint C, B;
		__asm volatile("mov %%ecx, %0" : "=r" (C));// will break GNU stack judge: __asm ("push %ecx");
		__asm volatile("mov %%ebx, %0" : "=r" (B));// will break GNU stack judge: __asm ("push %ebx");
		__asm volatile("mov %0, %%ebx" : : "r" _IMM(Dest));// gcc use mov %eax->%ebx to assign
		__asm volatile("mov %0, %%eax": : "r" (BlockIden));
		__asm volatile("mov $1, %ecx");
		__asm volatile("call HdiskLBA28Load");
		__asm volatile("mov %0, %%ebx" : : "r" (B));// rather __asm ("pop %ebx");
		__asm volatile("mov %0, %%ecx" : : "r" (C));// rather __asm ("pop %ecx");
		return true;
	}
#endif
}
