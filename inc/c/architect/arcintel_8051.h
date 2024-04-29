// ASCII C99 TAB4 CRLF
// LastCheck: 20240428
// AllAuthor: @dosconio
// ModuTitle: General Header
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
// FOR Win64 on x86_64 ONLY

#ifndef _INC_ARCHITECT_i8051
#define _INC_ARCHITECT_i8051

// ---- ---- ISO/IEC stdint.h ---- ----

// int measures 2-bytes
typedef signed char int8_t;
typedef unsigned char   uint8_t;
typedef short  int16_t;
typedef unsigned short  uint16_t;
typedef long  int32_t;
typedef unsigned long  uint32_t;

typedef signed char int_least8_t;
typedef signed char int_fast8_t;
typedef unsigned char   uint_least8_t;
typedef unsigned char uint_fast8_t;

typedef short  int_least16_t;
typedef int  int_fast16_t;
typedef unsigned short  uint_least16_t;
typedef int  uint_fast16_t;

typedef   signed long   int_least32_t;
typedef   signed long   int_fast32_t;
typedef unsigned long  uint_least32_t;
typedef unsigned long  uint_fast32_t;

typedef long  intmax_t;
typedef unsigned long   uintmax_t;

#define INT_LEAST8_MIN INT8_MIN
#define INT_LEAST8_MAX INT8_MAX
#define UINT_LEAST8_MAX UINT8_MAX
#define INT_FAST8_MIN INT8_MIN
#define INT_FAST8_MAX INT8_MAX
#define UINT_FAST8_MAX UINT8_MAX

#define INT_LEAST16_MIN INT16_MIN
#define INT_LEAST16_MAX INT16_MAX
#define UINT_LEAST16_MAX UINT16_MAX
#define INT_FAST16_MIN INT16_MIN
#define INT_FAST16_MAX INT16_MAX
#define UINT_FAST16_MAX UINT16_MAX

#define INT_LEAST32_MIN INT32_MIN
#define INT_LEAST32_MAX INT32_MAX
#define UINT_LEAST32_MAX UINT32_MAX
#define INT_FAST32_MIN INT32_MIN
#define INT_FAST32_MAX INT32_MAX
#define UINT_FAST32_MAX UINT32_MAX

#define SIG_ATOMIC_MIN INT32_MIN
#define SIG_ATOMIC_MAX INT32_MAX

#ifndef SIZE_MAX
#define SIZE_MAX UINT32_MAX
#define PTRDIFF_MIN INT32_MIN
#define PTRDIFF_MAX INT32_MAX
#endif
#endif // !_HOST_WIN64_
