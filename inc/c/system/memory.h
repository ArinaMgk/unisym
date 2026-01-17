// ASCII CPL TAB4 CRLF
// Docutitle: (Module) Memory
// Codifiers: @dosconio: 20240505 ~ <Last-check> 
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

#ifndef _INC_MEMORY
#define _INC_MEMORY

#include "../stdinc.h"

// #ifndef _STDLIB_H
// 	#define _STDLIB_H
// 	#endif
//#define __stdlib_h
// #define _BITS_ERRNO_H

// ---- above ---- stdlib.h

#if !defined(_DEBUG) && !defined(_dbg)
	#define memalloc(dest,size)\
			((*(char**)&dest=(char*)malloc(size))? "":"! MEMORY RUN OUT!")
	#define memfree(x) {if(x)free((char*)(x));}
	#define srs(x,y) {void*ebx=(void*)(y);if(x)free((char*)x);*(void**)&(x)=ebx;}
	#define malc(size) (void*)(malloc(size))
	#define zalc(size) (void*)(calloc(size,1))
#else
	#define memalloc(dest,size)\
		(*(char**)&dest=(char*)malloc(size))?((void)_MALCOUNT++):(erro("MEMORY RUN OUT!"),(void)0)
	#define memfree(x) do if(x){free((char*)(x));_MALCOUNT--;} while(0)// RFW21 version
	#define srs(x,y) {void*ebx=(void*)(y);if(x)free((void*)x);_MALCOUNT--;*(void**)&(x)=ebx;}
	#define malc(size) (void*)(_MALCOUNT++,malloc(size))
	#define zalc(size) (void*)(_MALCOUNT++,calloc(size,1))
#endif



_ESYM_C void memf(void* m);// non-side-effect version, with null-check
#define mfree(x) do{memfree(x);(x)=0;}while(0)

#ifdef _INC_CPP

#if defined(_DEV_GCC) && defined(_MCCA) && !defined(_NEW)

// ---- std.new
#if !((_MCCA & 0xFF00) == 0x1000)
_ESYM_CPP{
#include <new>
}
#else
_ESYM_CPP
inline void* operator new(size_t, void* p) { return p; }// #include <new>
namespace std {
	struct nothrow_t {
		#if __cplusplus >= 201103L
		explicit nothrow_t() = default;
		#endif
	};
	extern const nothrow_t nothrow;
}
_ESYM_CPP
void* operator new(size_t size, const std::nothrow_t&) noexcept;
#define _NEW// GCC Header Guard
#endif
#endif


extern "C" {
#endif
	
	// inline static void _memf(void* x) { memfree(x); }

	// size includes null-terminator
	char* salc(size_t size);

#ifdef _INC_CPP
}

	// `new(buf)type;` won't call this but `new type;`.
	//void* operator new(size_t size);
	//
	//void operator delete(void* p);

#endif


_ESYM_C void* memallocate(stduint siz);

//{} stduint memshrink(...);

_ESYM_C void    memrelease();

// : International Standard Interface [user - def]
#if !defined(_MCCA) || defined(_DEV_GCC)// avoid env-EDK2

#ifdef _DEV_MSVC
// #define _memory_midfix __cdecl
// #define _memory_prefix
#include "corecrt_malloc.h"
#elif defined(__stdlib_h)// ARMGCC
// #elif defined(_STDLIB_H)
#elif defined(_Linux)
#include <stdlib.h>
#else 
#if defined(_MCCA) && ((_MCCA & 0xFF00) != 0x1000)
#include <stdlib.h>
#endif

#define _memory_midfix
#define _memory_prefix _ESYM_C
_memory_prefix void*  calloc(size_t nmemb, size_t size);
_memory_prefix void   free(void* ptr);
_memory_prefix void*  malloc(size_t size);
_memory_prefix void*  realloc(void* ptr, size_t size);
#undef _memory_midfix
#undef _memory_prefix
#endif

#endif// !_MCCA

#endif
