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

extern void erro(char*);
extern void warn(char*);
extern size_t malc_count;// <IN>
extern size_t malc_limit;// <IN>
extern size_t call_state;// <OUT>
// call_state :
// 00: nullptr
// 01: over the limit for size
// 02: invaild input but not null pointer
extern size_t malc_occupy;
extern size_t arna_precise;

#if defined(_dbg) || defined(_DEBUG)
	#define memalloc(dest,size)\
		(*(char**)&dest=(char*)malloc(size))?((void)malc_count++):(erro("MEMORY RUN OUT!"),(void)0)
	#define memfree(x) if(x){free((char*)(x));malc_count--;}// RFW21 version
	#define srs(x,y) {void*ebx=(void*)(y);if(x)free((void*)x);malc_count--;*(void**)&(x)=ebx;}
	#define malc(size) (void*)(malc_count++,malloc(size))
	#define zalc(size) (void*)(malc_count++,calloc(size,1))
	
#else
	#define memalloc(dest,size)\
		(*(char**)&dest=(char*)malloc(size))?((void)0):(erro("MEMORY RUN OUT!"),(void)0)
	#define memfree(x) {if(x)free((char*)(x));}
	#define srs(x,y) {void*ebx=(void*)(y);if(x)free((char*)x);*(void**)&(x)=ebx;}
	#define malc(size) (void*)(malloc(size))
	#define zalc(size) (void*)(calloc(size,1))

#endif

#define zalcof(x) zalc(sizeof(x))
#define malcof(x) malc(sizeof(x))
#define memf(x) memfree(x)

#include <stdlib.h>
inline static char* salc(size_t size)
{
	if (!size) return 0;
	char* r = (char*)malc(size);
	r[size - 1] = 0;
	return r;
}


#define init_total_errmech(exitcode) \
	if (setjmp(errjb))\
	{\
		if (SGAErroMsg && *SGAErroMsg)\
			fprintf(stderr, "!Err %s\n", SGAErroMsg);\
		if(exitcode&&malc_count) fprintf(stderr, "MEMORY LEAK %"PRIuPTR" TIMES.\n", malc_count);\
		return (exitcode);\
	}

// Quickly set the necessary configuration
#define ulibsym(limit)\
	size_t malc_count, malc_limit=(limit), call_state;


#endif
