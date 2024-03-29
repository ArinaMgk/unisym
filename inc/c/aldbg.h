// ASCII TAB4 C99 ArnCovenant TAB CRLF
/*
	Copyright 2023 ArinaMgk

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/

//{MAY} Rename to debug.h

#pragma warning(disable:6011)// for MSVC

#ifndef _LIB_DEBUG// Add this can cheat the compiler
#define _LIB_DEBUG// Her Convenient odd style. Maybe a bad habit for formal project.

#include <stddef.h>
#include "alice.h"

#define printb(x) printf("%s: " #x "\n",(x)?"True":"False")
#define malc_count _MALCOUNT
#define malc_limit _MALLIMIT

#define _MALLIMIT_DEFAULT 0x1000
#define unchecked
#define toheap

extern void erro(char* erromsg);
extern void warn(char* warnmsg);
extern size_t _MALCOUNT;// <OUT>
extern size_t _MALLIMIT;// <IN>
extern size_t call_state;// <OUT>
// call_state :
// 00: nullptr
// 01: over the limit for size
// 02: invaild input but not null pointer
extern size_t malc_occupy;
extern size_t arna_precise;

#if defined(_dbg) || defined(_DEBUG)
	#define memalloc(dest,size)\
		(*(char**)&dest=(char*)malloc(size))?((void)_MALCOUNT++):(erro("MEMORY RUN OUT!"),(void)0)
	#define memfree(x) if(x){free((char*)(x));_MALCOUNT--;}// RFW21 version
	#define srs(x,y) {void*ebx=(void*)(y);if(x)free((void*)x);_MALCOUNT--;*(void**)&(x)=ebx;}
	#define malc(size) (void*)(_MALCOUNT++,malloc(size))
	#define zalc(size) (void*)(_MALCOUNT++,calloc(size,1))
	
#else
	#define memalloc(dest,size)\
		(*(char**)&dest=(char*)malloc(size))?((void)0):(erro("MEMORY RUN OUT!"),(void)0)
	#define memfree(x) {if(x)free((char*)(x));}
	#define srs(x,y) {void*ebx=(void*)(y);if(x)free((char*)x);*(void**)&(x)=ebx;}
	#define malc(size) (void*)(malloc(size))
	#define zalc(size) (void*)(calloc(size,1))

#endif

#define zalcof(x) (x*)zalc(sizeof(x))
#define malcof(x) (x*)malc(sizeof(x))
#define memf(x)   memfree(x)
#define mfree(x) {memfree(x);(x)=0;}

#include <stdlib.h>
inline static void _memf(void* x)
{
	memfree(x);
}
inline static char* salc(size_t size)
{
	if (!size) return 0;
	char* r = (char*)malc(size);
	r[size - 1] = 0;
	return r;
}

// Quickly set the necessary configuration
#define ulibsym(limit)\
	size_t _MALCOUNT, malc_limit=(limit), call_state;


#endif
