// ASCII C TAB4 CRLF
// Docutitle: (Interface) STDLIB
// Codifiers: @dosconio: 20240602
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

#ifndef _INC_STDLIB
#define _INC_STDLIB
#include <stdlib.h>
#endif

#ifndef _INC_STDLIB
#define _INC_STDLIB

// #define _STDLIB_H// GCC
// #define _CSTDLIB_// MSVC
// #define _GLIBCXX_CSTDLIB

#include "../stdinc.h"// {archit.h}
#include "../widechar.h"
//- GBK
//- UTF-32 (till now)
#include "../multichar.h"
//- UTF-8
//- UTF-16

#include "../random.h"// Pseudo-random sequence generation
//#include "../memory.h"// Memory management (stdinc->host->memory)
#include "../algorithm/sort.h"// Searching and sorting utilities
#include "../arith.h"// // Integer arithmetic functions


#if defined(_INC_CPP)
/*
namespace std {
	//declare in random.h
	int rand(void);

	double strtod(const char* inp, char** endptr);
	float strtof(const char* inp, char** endptr);
	long double strtold(const char* inp, char** endptr);
	long int strtol(const char* inp, char** endptr, int base);
	unsigned long int strtoul(const char* inp, char** endptr, int base);
	long long int strtoll(const char* inp, char** endptr, int base);
	unsigned long long int strtoull(const char* inp, char** endptr, int base);
}
*/
//namespace uni {
extern "C" {
#endif

// # type
// - size_t よ archit.h
// - wchar_t よ widechar.h
// - div_t

typedef struct div_t {
	int quot; // EAX
	int rem;  // EDX
} div_t;
// - ldiv_t
typedef struct ldiv_t {
	long int quot; // EAX
	long int rem;  // EDX
} ldiv_t;
// - lldiv_t
typedef struct lldiv_t {
	long long int quot; // RAX
	long long int rem;  // RDX
} lldiv_t;

// # macro
// - NULL よ archit.h
// - EXIT_SUCCESS
#define EXIT_SUCCESS 0
// - EXIT_FAILURE
#define EXIT_FAILURE 1
// - RAND_MAX
#define RAND_MAX 0x7FFF //{TEMP}
//{TODO} - MB_CUR_MAX 

// # function

// Numeric conversion

#include "../ustring.h" // contain "atoins" and "instoa", and { Multibyte/wide character/string conversion functions }
// To adapt for GNU chain, do not use #define

#define restrict //{TEMP}

// Communication with the environment : International Standard Interface [user - def]

#ifdef _DEV_MSVC
//{TODO} error C2381: “abort”: redef and dif in "noreturn"
#else
void abort(void);
#endif
int atexit(void (*func)(void));
void exit(int status);
void _Exit(int status);
char* getenv(const char* name);
int system(const char* string);

struct div_t div(int numer, int denom);
struct ldiv_t ldiv(long int numer, long int denom);
struct lldiv_t lldiv(long long int numer, long long int denom);

#undef restrict //{TEMP}, this make error for MSVC2022(C11) at 20240819

#if defined(_INC_CPP)
} //END C++ Area

#else//: C Area
#endif




#endif
