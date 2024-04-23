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


// If no host-environment will be used, you can define __BITS__ by yourself.
#if defined(__BITS__)
	//
#elif defined(_Win32)
	#undef _WinNT
	#define _WinNT 32
	#define __BITS__ 32
#elif defined(_Win64)
	#undef _WinNT
	#define _WinNT 64
	#define __BITS__ 64
#elif defined(_Win16)
	#undef _WinNT
	#define _WinNT 16
	#define __BITS__ 16
//
#elif defined(_Linux32)
	#undef _Linux
	#define _Linux 32
	#define __BITS__ 32
#elif defined(_Linux64)
	#undef _Linux
	#define _Linux 64
	#define __BITS__ 64
//
#elif defined(_MCCAx86) // Architect + Bitmode
	#undef _MCCA
	#define _MCCA 0x8632 // default flat-segment mode
	#define __BITS__ 32
#elif defined(_MCCAx86Real)
	#undef _MCCA
	#define _MCCA 0x8616
	#define __BITS__ 16
// 
#else
	#include "proctrl.h"
	#define _INC_DEPEND_STDINT
	#include <stdint.h> // try
	#if SIZE_MAX==0xFFFF
		#define __BITS__ 16 
	#elif SIZE_MAX==0xFFFFFFFF
		#define __BITS__ 32
	#elif SIZE_MAX==0xFFFFFFFFFFFFFFFF
		#define __BITS__ 64
	#else 
		#define __BITS__ 8
	#endif
#endif

#if __BITS__ == 16
	#define __BIT_STR__ "16"
#elif __BITS__ == 32
	#define __BIT_STR__ "32"
#elif __BITS__ == 64
	#define __BIT_STR__ "64"
#elif __BITS__ == 8
	#define __BIT_STR__ "8"
#endif

#include "alice.h"
#include "floating.h"

enum Architecture_t // for instruction sets
{
	Architecture_x86,
	Architecture_RISCV64,
	
	Architecture_x64,
	Architecture_ARM,
	Architecture_ARM64,
	Architecture_MIPS,
	Architecture_MIPS64,
	Architecture_RISCV32,
	Architecture_POWERPC64,
	Architecture_POWERPC32,
	Architecture_SPARC64,
	Architecture_SPARC32,
	Architecture_Unknown
};

#include "call.h"
#include "aflag.h"

#endif
