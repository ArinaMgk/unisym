//
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

#ifndef ArnHabitDbg// Add this can cheat the compiler
#define ArnHabitDbg// Her Convenient odd style. Maybe a bad habit for formal project.

#include <stddef.h>
#include "alice.h"

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

#ifdef _dbg
	#define memalloc(dest,size)\
		(*(char**)&dest=(char*)malloc(size))?((void)malc_count++):(erro("MEMORY RUN OUT!"),(void)0)
	#define memfree(x) if(x){free((char*)(x));malc_count--;}// RFW21 version
	#define srs(x,y) {void*ebx=(void*)(y);if(x)free((void*)x);malc_count--;*(void**)&(x)=ebx;}
	#define malc(size) (void*)(malc_count++,malloc(size))
	#define zalc(size) (void*)(malc_count++,calloc(size,1))
	#define memf(x) memfree(x)
	
#else
	#define memalloc(dest,size)\
		(*(char**)&dest=(char*)malloc(size))?((void)0):(erro("MEMORY RUN OUT!"),(void)0)
	#define memfree(x) {if(x)free((char*)(x));}
	#define srs(x,y) {void*ebx=(void*)(y);if(x)free((char*)x);*(void**)&(x)=ebx;}
	#define malc(size) (void*)(malloc(size))
	#define zalc(size) (void*)(calloc(size,1))
	#define memf(x) memfree(x)
	
#endif

#include <stdlib.h>
inline static char* salc(size_t size)
{
	if (!size) return 0;
	char* r = malc(size);
	r[size - 1] = 0;
	return r;
}


#define init_total_errmech() \
	if (setjmp(errjb))\
	{\
		if (SGAErroMsg && *SGAErroMsg)\
			fprintf(stderr, "!Err %s\n", SGAErroMsg);\
		if(malc_count) fprintf(stderr, "malc_count = %"PRIXPTR"\n", malc_count);\
		if(mode=='c')system("pause");\
		return 1;\
	}

#define assert(expression) ((expression)?(char*)(expression):(exit(1),(char*)0))

// Quickly set the necessary configuration
#define ulibsym(limit)\
	size_t malc_count, malc_limit=(limit), call_state;


#endif
