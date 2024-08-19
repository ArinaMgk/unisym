// ASCII CPL TAB4 CRLF
// Docutitle: (Module) Debug
// Codifiers: @dosconio: 20240424 ~ <Last-check> 
// Attribute: <ArnCovenant> Any-Architect <Environment> <Reference/Dependence>
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


#ifdef _BUILD_MSVC
#pragma warning(disable:6011)// for MSVC
#endif

#if !defined(_LIB_DEBUG) && defined(_DEBUG)// Add this can cheat the compiler
#define _LIB_DEBUG// Her Convenient odd style. Maybe a bad habit for formal project.

#include "proctrl.h"

#define malc_count _MALCOUNT
#define malc_limit _MALLIMIT

// #define jump goto // to see asm-inst
#define idle() {_jump: goto _jump;}

#define chars_stack(x) &(char[]){#x "\0"}// chars_stack(123)
#define chstk chars_stack

#define _MALLIMIT_DEFAULT 0x1000

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

//{TOIN} memory.h
#if defined(_dbg) || defined(_DEBUG)
	#define memalloc(dest,size)\
		(*(char**)&dest=(char*)malloc(size))?((void)_MALCOUNT++):(erro("MEMORY RUN OUT!"),(void)0)
	#define memfree(x) if(x){free((char*)(x));_MALCOUNT--;}// RFW21 version
	#define srs(x,y) {void*ebx=(void*)(y);if(x)free((void*)x);_MALCOUNT--;*(void**)&(x)=ebx;}
	#define malc(size) (void*)(_MALCOUNT++,malloc(size))
	#define zalc(size) (void*)(_MALCOUNT++,calloc(size,1))
#endif

// Quickly set the necessary configuration
#define ulibsym(limit)\
	size_t _MALCOUNT, malc_limit=(limit), call_state;

#endif

