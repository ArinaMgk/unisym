// ASCII C99 TAB4 CRLF
// Docutitle: (Common) Basic Types and Characters of the Machine
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

#ifndef _INC_ARCHITECTURE
#define _INC_ARCHITECTURE

enum Architecture_t // for instruction sets
{
	Architecture_x86,
	Architecture_RISCV64,
	
	Architecture_x86_64,

	Architecture_ARM,
	Architecture_ARM64,
	Architecture_MIPS,
	Architecture_MIPS64,
	Architecture_RISCV32,
	Architecture_POWERPC64,
	Architecture_POWERPC32,
	Architecture_SPARC64,
	Architecture_SPARC32,
	//
	Architecture_Unknown
};

// If no host-environment will be used, you can define __BITS__ by yourself.
#if defined(__BITS__)
	//
#elif defined(_Win32) // "_WIN32" is for many Windows Series but just 32-bit
	#undef _WinNT // if this cause warnings, add `#ifdef` more.
	#define _WinNT 32
	#define _Intelx86_Windows_32
	#define __ARCH__ Architecture_x86
	#define __BITS__ 32
#elif defined(_Win64)
	#undef _WinNT
	#define _WinNT 64
	#define _Intelx86o64_Windows_64
	#define __ARCH__ Architecture_x86
	#define __BITS__ 64
#elif defined(_Win16)
	#undef _WinNT
	#define _WinNT 16
	#define _Intelx86_Windows_16
	#define __ARCH__ Architecture_x86	
	#define __BITS__ 16
//
#elif defined(_Linux32)
	#undef _Linux
	#define _Linux 32
	#define _Intelx86_Linux_32
	#define __ARCH__ Architecture_x86
	#define __BITS__ 32
#elif defined(_Linux64)
	#undef _Linux
	#define _Linux 64
	#define _Intelx86o64_Linux_64
	#define __ARCH__ Architecture_x86_64
	#define __BITS__ 64
// [Special for Mecocoa]
#elif defined(_MCCAx86)
	#undef _MCCA
	#define _MCCA 0x8632 // default flat-segment mode
	#define _Intelx86_Mcca_32
	#define __ARCH__ Architecture_x86
	#define __BITS__ 32
#elif defined(_MCCAx86Real)
	#undef _MCCA
	#define _MCCA 0x8616
	#define _Intelx86_Mcca_16
	#define __ARCH__ Architecture_x86
	#define __BITS__ 16
#endif
// ---- ---- ---- ---- stdint.h [partial] ---- ---- ---- ----

// `long int` measures sometimes 32 and sometimes 64, as example, so it is necessary to add host for compilers except Magice, which is also a customizable linear language.
#if defined(_STDINT) || defined(_STDINT_H_) || defined(_STDINT_H) // stdint.h
	#define _INC_DEPEND_STDINT
#elif defined(_Intelx86o64_Windows_64)
	#include "architect/arcx64_win64.h"
#else
	#define _INC_DEPEND_STDINT
	#include <stdint.h> // try
	#ifndef __ARCH__
		#define __ARCH__ Architecture_Unknown
	#endif
#endif
#ifndef __BITS__
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

#if !defined(_STDINT)
	#define _STDINT // for MSVC
#endif
#if !defined(_STDINT_H_)
	#define _STDINT_H_ // for GCC
#endif
#if !defined(_STDINT_H)
	#define _STDINT_H // for GCC
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
#include "call.h" // part of attributes of others' compilers
#include "flag.h" // may be used for Operating System Development

#endif
