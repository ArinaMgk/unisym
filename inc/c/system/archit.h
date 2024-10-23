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

//{TEMP} Do not change the order for Magice making use of it.
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
//{TODO} in uoption: Architecture_ARMv7

// If no host-environment will be used, you can define __BITS__ by yourself.
#if defined(__BITS__) && defined(__ARCH__)
	//
#elif defined(_Win32) // "_WIN32" is for many Windows Series but just 32-bit
	#undef _WinNT // if this cause warnings, add `#ifdef` more.
	#define _WinNT 32
	#define _Intelx86_Windows_32
	#define __ARCH__ Architecture_x86
	#ifndef _ARC_x86
		#define _ARC_x86 0
		#endif
	#define __BITS__ 32
#elif defined(_Win64)
	#undef _WinNT
	#define _WinNT 64
	#define _Intelx86o64_Windows_64
	#define __ARCH__ Architecture_x86
	#ifndef _ARC_x86
		#define _ARC_x86 0
		#endif
	#define __BITS__ 64
#elif defined(_Win16)
	#undef _WinNT
	#define _WinNT 16
	#define _Intelx86_Windows_16
	#define __ARCH__ Architecture_x86
	#ifndef _ARC_x86
		#define _ARC_x86 0
		#endif
	#define __BITS__ 16
//
#elif defined(_Linux32)
	#undef _Linux
	#define _Linux 32
	#define _Intelx86_Linux_32
	#define __ARCH__ Architecture_x86
	#ifndef _ARC_x86
		#define _ARC_x86 0
		#endif
	#define __BITS__ 32
#elif defined(_Linux64)
	#undef _Linux
	#define _Linux 64
	#define _Intelx86o64_Linux_64
	#define __ARCH__ Architecture_x86_64
	#ifndef _ARC_x64
		#define _ARC_x64
		#endif
	#define __BITS__ 64
// [Special for Mecocoa]
#elif defined(_MCCA) && _MCCA==0x8632// defined(_ARC_x86)
	#define _Intelx86_Mcca_32
	#define __ARCH__ Architecture_x86
	#ifndef _ARC_x86
		#define _ARC_x86 5
		#endif
	#define __BITS__ 32
#elif defined(_MCCA) && _MCCA==0x8616// defined(_ARC_x86)
	#define _Intelx86_Mcca_16
	#define __ARCH__ Architecture_x86
	#ifndef _ARC_x86
		#define _ARC_x86 1
		#endif
	#define __BITS__ 16
#elif defined(_OPT_RISCV64)
	#define __ARCH__ Architecture_RISCV64
	#define _ARC_RISCV_64
	#define __BITS__ 64
#endif
// ---- ---- ---- ---- stdint.h fixed ---- ---- ---- ----
#if defined(_STDINT) || defined(_STDINT_H_) || defined(_STDINT_H)
	#define _INC_DEPEND_STDINT
#endif
// ---- ---- ---- ---- limits.h fixed ---- ---- ---- ----
#if defined(_INC_LIMITS) || defined(_LIMITS_H___)
	#define _INC_DEPEND_LIMITS
#endif
// ---- ---- ---- ---- stddef.h fixed ---- ---- ---- ----
#if defined(_INC_STDDEF) || defined(_STDDEF_H) || defined(_STDDEF_H_) || defined(__stddef_h)
	#define _INC_DEPEND_STDDEF
#endif
// ---- ---- ---- ---- specific ---- ---- ---- ----

// `long int` measures sometimes 32 and sometimes 64, as example, so it is necessary to add host for compilers except Magice, which is also a customizable linear language.

#if defined(_INC_DEPEND_STDINT) || defined(_INC_DEPEND_LIMITS) || defined(_INC_DEPEND_STDDEF)
	// do nothing. We would better include UNISYM at the beginning of the document.
	#ifndef _INC_DEPEND_STDINT
	#define _INC_DEPEND_STDINT
	#include <stdint.h>
	#endif
	#ifndef _INC_DEPEND_LIMITS
	#define _INC_DEPEND_LIMITS
	#include <limits.h>
	#endif
	#ifndef _INC_DEPEND_STDDEF
	#define _INC_DEPEND_STDDEF
	#include <stddef.h>
	#endif		
#elif defined(_Intelx86o64_Windows_64)
	#include "../architect/arcx64_win64.h"
#elif defined(_MCU_Intel8051)
	#include "../architect/arcintel_8051.h"
	#define __BITS__ 8
	#define __ARCH__ Architecture_Unknown
#elif defined(_OPT_RISCV64)
	#include "../architect/arcriscv_64.h"
#else // if unknown architecture, UNISYM will not define any
	#define _INC_DEPEND_STDINT
	#include <stdint.h> // try
	#define _INC_DEPEND_LIMITS
	#include <limits.h> // try
	#define _INC_DEPEND_STDDEF
	#include <stddef.h> // try
	#ifndef __ARCH__
		#define __ARCH__ Architecture_Unknown
	#endif
#endif


#ifndef _INC_DEPEND_STDINT
#ifndef INT8_MIN
	#define INT8_MIN (-128)
	#define INT8_MAX 127
	#define UINT8_MAX 255
#endif
#ifndef INT16_MIN
	#define INT16_MIN (-32768)
	#define INT16_MAX 32767
	#define UINT16_MAX 65535
#endif
#ifndef INT32_MIN
	#define INT32_MIN (-2147483647 - 1)
	#define INT32_MAX 2147483647
	#define UINT32_MAX 0xffffffffU
#endif
#ifndef INT64_MIN
	#define INT64_MIN  (-9223372036854775807LL - 1)
	#define INT64_MAX 9223372036854775807LL
	#define UINT64_MAX 0xffffffffffffffffULL
#endif
#endif

#ifndef _INC_DEPEND_LIMITS
#define CHAR_BIT 8 // Arina Covenant
#define SCHAR_MIN   (-127) // -128 is a trap representation so it does not appear here. --Alice said.
#define SCHAR_MAX +127
#define UCHAR_MAX 0xff
#if 1 //{TEMP}
#define CHAR_MIN    SCHAR_MIN
#define CHAR_MAX    SCHAR_MAX
#else
#define CHAR_MIN    0
#define CHAR_MAX    UCHAR_MAX
#endif
// Think short or word is uint16_t
#define SHRT_MIN      (-SHRT_MAX) // aka (-32767) // (-32768)
#define SHRT_MAX      32767
#define USHRT_MAX     0xffff
#endif

#ifndef _INC_DEPEND_STDDEF
#ifndef NULL
	#if defined(_INC_CPP) || defined(__cplusplus)
	#define NULL 0
	#else
	#define NULL ((void*)0)
	#endif
#endif
#define offsetof(s,m) ((size_t)&(((s*)0)->m))
#endif

#if !defined(__BITS__) && !defined(SIZE_MAX)
	#if SIZE_MAX==0xFFFF
		#define __BITS__ 16 
	#elif SIZE_MAX==0xFFFFFFFF
		#define __BITS__ 32
	#elif SIZE_MAX==0xFFFFFFFFFFFFFFFF
		#define __BITS__ 64
	#elif SIZE_MAX==0xFF
		#define __BITS__ 8
	#endif
	#endif

// STDINT.H
	#if !defined(_STDINT)
		#define _STDINT // for MSVC
	#endif
	#if !defined(_STDINT_H_)
		#define _STDINT_H_ // for GCC
	#endif
	#if !defined(_STDINT_H)
		#define _STDINT_H // for GCC
	#endif
// LIMITS.H
	#if !defined(_INC_LIMITS)
		#define _INC_LIMITS // for MSVC
	#endif
	#if !defined(_LIMITS_H___)
		#define _LIMITS_H___ // for GCC
	#endif
// STDDEF.H
	#if !defined(_INC_STDDEF)
		#define _INC_STDDEF // for MSVC
	#endif
	#if !defined(_STDDEF_H)
		#define _STDDEF_H // for GCC
	#endif
	#if !defined(_STDDEF_H_)
		#define _STDDEF_H_ // for GCC
	#endif
	#if !defined(__stddef_h)
		#define __stddef_h // for Keil MDK
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

//: general headers for devkit
#include "alice.h"
#include "call.h" // part of attributes of others' compilers
#include "flag.h" // may be used for Operating System Development

#endif
