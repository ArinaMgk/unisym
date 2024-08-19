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


void*   memallocate(stduint siz);

//{} stduint memshrink(...);

void    memrelease();

// : International Standard Interface [user - def]
_CALL_C void* calloc(size_t nmemb, size_t size);
_CALL_C void free(void* ptr);
_CALL_C void* malloc(size_t size);
_CALL_C void* realloc(void* ptr, size_t size);

#endif
