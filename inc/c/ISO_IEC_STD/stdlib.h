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

#include "../stdinc.h"// {archit.h}
#include "../widechar.h"
//- GBK
//- UTF-32 (till now)
#include "../multichar.h"
//- UTF-8
//- UTF-16



// # type
// - size_t よ archit.h
// - wchar_t よ widechar.h
// - div_t
struct div_t {
	int quot; // EAX
	int rem;  // EDX
};
// - ldiv_t
struct ldiv_t {
	long int quot; // EAX
	long int rem;  // EDX
};
// - lldiv_t
struct lldiv_t {
	long long int quot; // RAX
	long long int rem;  // RDX
};

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

#include "../ustring.h" // contain "atoins" and "instoa"

#define atoi _atoidefa
_CALL_C int _atoidefa(const char* inp);

#define atof atoflt
_CALL_C double atoflt(const char* astr);

#define atol atolong
_CALL_C long int atolong(const char* inp);

#define restrict //{TEMP}
//{TODO}{HERE}
_CALL_C double strtod(const char* restrict inp, char** restrict endptr);
_CALL_C float strtof(const char* restrict inp, char** restrict endptr);
_CALL_C long double strtold(const char* restrict inp, char** restrict endptr);
_CALL_C long int strtol(const char* restrict inp, char** restrict endptr, int base);
_CALL_C unsigned long int strtoul(const char* restrict inp, char** restrict endptr, int base);

#ifdef _BIT_SUPPORT_64
_CALL_C long long int atoll(const char* inp);
_CALL_C long long int strtoll(const char* restrict inp, char** restrict endptr, int base);
_CALL_C unsigned long long int strtoull(const char* restrict inp, char** restrict endptr, int base);
#endif

// Pseudo-random sequence generation

_CALL_C int rand(void);
_CALL_C void srand(unsigned int seed);

// Memory management 

#include "../memory.h"

// Communication with the environment]

_CALL_C void abort(void);
_CALL_C int atexit(void (*func)(void));
_CALL_C void exit(int status);
_CALL_C void _Exit(int status);
_CALL_C char* getenv(const char* name);
_CALL_C int system(const char* string);

// Searching and sorting utilities

//{} incl...
_CALL_C void* bsearch(const void* key, const void* base, size_t nmemb, size_t size, int (*compar)(const void*, const void*));

#include "../algorithm/sort.h"
_CALL_C void qsort(void* base, size_t nmemb, size_t size, int (*compar)(const void*, const void*));

// Integer arithmetic functions

#define _ABS_IMM(a) ((a)<0?-(a):(a))
#define abs(i) intabs(i)
static inline int intabs(int j) { return _ABS_IMM(j); }
static inline long int labs(long int j) { return _ABS_IMM(j); }
static inline long long int llabs(long long int j) { return _ABS_IMM(j); }

//{TODO} implementation by ASMx86(Hard) and (Soft) <- Decide by ARCH
_CALL_C struct div_t div(int numer, int denom);
_CALL_C struct ldiv_t ldiv(long int numer, long int denom);
_CALL_C struct lldiv_t lldiv(long long int numer, long long int denom);

// Multibyte/wide character conversion functions

_CALL_C int mblen(const char* s, size_t n);
_CALL_C int mbtowc(wchar_t* restrict pwc, const char* restrict s, size_t n);
_CALL_C int wctomb(char* s, wchar_t wc);

// Multibyte/wide string conversion functions

_CALL_C size_t mbstowcs(wchar_t* restrict pwcs, const char* restrict s, size_t n);
_CALL_C size_t wcstombs(char* restrict s, const wchar_t* restrict pwcs, size_t n);

#undef restrict //{TEMP}, this make error for MSVC2022(C11) at 20240819

#endif
