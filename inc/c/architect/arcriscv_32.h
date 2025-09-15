// ASCII CPL-GCC-C99 TAB4 CRLF
// LastCheck: 20240504
// AllAuthor: @dosconio
// ModuTitle: Architecture-specific definitions
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

// for GCC compiler

typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed int int32_t;
typedef unsigned int uint32_t;
// long is 64-bit on Linux-GCC-Devk and RISCV64
typedef signed long long int64_t;
typedef unsigned long long uint64_t;

// limits.h
#define MB_LEN_MAX    5 //{TEMP} AyanoChan did know, is it right?

#define INT_MIN     INT32_MIN
#define INT_MAX     INT32_MAX
#define UINT_MAX    UINT32_MAX

#define LONG_MIN    INT64_MIN
#define LONG_MAX    INT64_MAX
#define ULONG_MAX   UINT64_MAX

#define LLONG_MAX   INT64_MIN
#define LLONG_MIN   INT64_MAX
#define ULLONG_MAX  UINT64_MAX

// stddef.h
typedef int32_t ptrdiff_t;
typedef uint32_t size_t;
//{TODO} typedef unsigned long wchar_t;//{TEMP}

