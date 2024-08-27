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

#include "stdinc.h"

#define _STDLIB_H

#ifdef _INC_CPP
extern "C++" {
#endif
	// inline static void _memf(void* x) { memfree(x); }
	char* salc(size_t size);

	// `new(buf)type;` won't call this but `new type;`.
	//void* operator new(size_t size);
	//
	//void operator delete(void* p);
#ifdef _INC_CPP
}
#endif


_CALL_C void* memallocate(stduint siz);

//{} stduint memshrink(...);

_CALL_C void    memrelease();

// : International Standard Interface [user - def]
#ifdef _DEV_MSVC
// #define _memory_midfix __cdecl
// #define _memory_prefix
#include "corecrt_malloc.h"
#else
#define _memory_midfix
#define _memory_prefix _CALL_C
_memory_prefix void*  calloc(size_t nmemb, size_t size);
_memory_prefix void   free(void* ptr);
_memory_prefix void*  malloc(size_t size);
_memory_prefix void*  realloc(void* ptr, size_t size);
#undef _memory_midfix
#undef _memory_prefix
#endif


#endif
