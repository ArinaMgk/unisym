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

#define _STDLIB_H// GCC
#define _CSTDLIB_// MSVC
#define _GLIBCXX_CSTDLIB


#include "../stdinc.h"// {archit.h}
#include "../widechar.h"
//- GBK
//- UTF-32 (till now)
#include "../multichar.h"
//- UTF-8
//- UTF-16

#include "../random.h"// Pseudo-random sequence generation
#include "../memory.h"// Memory management 
#include "../algorithm/sort.h"// Searching and sorting utilities

#if defined(_INC_CPP)
namespace std {
	int rand(void);
	double strtod(const char* inp, char** endptr);
	float strtof(const char* inp, char** endptr);
	long double strtold(const char* inp, char** endptr);
	long int strtol(const char* inp, char** endptr, int base);
	unsigned long int strtoul(const char* inp, char** endptr, int base);
	long long int strtoll(const char* inp, char** endptr, int base);
	unsigned long long int strtoull(const char* inp, char** endptr, int base);
}

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

//{TODO}:below ===========================================

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
//{TODO} into "ustring.h"

// #include "../ustring.h" // contain "atoins" and "instoa"
// To adapt for GNU chain, do not use #define

#define restrict //{TEMP}

//#define atoi _atoidefa
int _atoidefa(const char* inp);

//#define atof atoflt
double atoflt(const char* astr);

//#define atol atolong
long int atolong(const char* inp);
//#define strtol StrToken_long
long int StrToken_long(const char* restrict inp, char** restrict endptr, int base);

//{TODO}{HERE}
double strtod(const char* restrict inp, char** restrict endptr);
float strtof(const char* restrict inp, char** restrict endptr);
long double strtold(const char* restrict inp, char** restrict endptr);
unsigned long int strtoul(const char* restrict inp, char** restrict endptr, int base);

#ifdef _BIT_SUPPORT_64
long long int atoll(const char* inp);
long long int strtoll(const char* restrict inp, char** restrict endptr, int base);
unsigned long long int strtoull(const char* restrict inp, char** restrict endptr, int base);
#endif





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

// Integer arithmetic functions よ arith.h
//#define abs(i) intabs(i) 
int intabs(int j);
int abs(int j);
long int labs(long int j);
long long int llabs(long long int j);

//{TODO} implementation by ASMx86(Hard) and (Soft) <- Decide by ARCH
struct div_t div(int numer, int denom);
struct ldiv_t ldiv(long int numer, long int denom);
struct lldiv_t lldiv(long long int numer, long long int denom);

// Multibyte/wide character conversion functions

int mblen(const char* s, size_t n);
int mbtowc(wchar_t* restrict pwc, const char* restrict s, size_t n);
int wctomb(char* s, wchar_t wc);

// Multibyte/wide string conversion functions

size_t mbstowcs(wchar_t* restrict pwcs, const char* restrict s, size_t n);
size_t wcstombs(char* restrict s, const wchar_t* restrict pwcs, size_t n);
#undef restrict //{TEMP}, this make error for MSVC2022(C11) at 20240819

#if defined(__cplusplus) || defined(_INC_CPP)
} //END C++ Area

#else//: C Area
#endif




#endif
