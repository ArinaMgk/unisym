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
#include "../multichar.h"

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
// - EXIT_FAILURE
// - EXIT_SUCCESS
// - RAND_MAX
// - MB_CUR_MAX

// # function

// Numeric conversion

#include "../ustring.h" // contain "atoins" and "instoa"

double atof(const char* nptr);

#define atoi(inp) _atoidefa(inp)
int _atoidefa(const char* inp);

long int atol(const char* nptr);
long long int atoll(const char* nptr);
double strtod(const char* restrict nptr, char** restrict endptr);
float strtof(const char* restrict nptr, char** restrict endptr);
long double strtold(const char* restrict nptr, char** restrict endptr);
long int strtol(const char* restrict nptr, char** restrict endptr, int base);
long long int strtoll(const char* restrict nptr, char** restrict endptr, int base);
unsigned long int strtoul(const char* restrict nptr, char** restrict endptr, int base);
unsigned long long int strtoull(const char* restrict nptr, char** restrict endptr, int base);

// Pseudo-random sequence generation

int rand(void);
void srand(unsigned int seed);

// Memory management

void* calloc(size_t nmemb, size_t size);
void free(void* ptr);
void* malloc(size_t size);
void* realloc(void* ptr, size_t size);

// Communication with the environment]

void abort(void);
int atexit(void (*func)(void));
void exit(int status);
void _Exit(int status);
char* getenv(const char* name);
int system(const char* string);

// Searching and sorting utilities

//{} incl...
void* bsearch(const void* key, const void* base, size_t nmemb, size_t size, int (*compar)(const void*, const void*));

#include "../algorithm/sort.h"
void qsort(void* base, size_t nmemb, size_t size, int (*compar)(const void*, const void*));

// Integer arithmetic functions

#define _ABS_IMM(a) ((a)<0?-(a):(a))
#define abs(i) intabs(i)
static inline int intabs(int j) { return _ABS_IMM(j); }
static inline long int labs(long int j) { return _ABS_IMM(j); }
static inline long long int llabs(long long int j) { return _ABS_IMM(j); }

//{TODO} implementation by ASMx86(Hard) and (Soft) <- Decide by ARCH
div_t div(int numer, int denom);
ldiv_t ldiv(long int numer, long int denom);
lldiv_t lldiv(long long int numer, long long int denom);

// Multibyte/wide character conversion functions

int mblen(const char* s, size_t n);
int mbtowc(wchar_t* restrict pwc, const char* restrict s, size_t n);
int wctomb(char* s, wchar_t wc);

// Multibyte/wide string conversion functions

size_t mbstowcs(wchar_t* restrict pwcs, const char* restrict s, size_t n);
size_t wcstombs(char* restrict s, const wchar_t* restrict pwcs, size_t n);

#endif
