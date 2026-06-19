// ASCII C99 TAB4 CRLF
// LastCheck: 2026JUN19
// AllAuthor: @dosconio
// ModuTitle: Architecture-specific definitions for Mecocoa x64
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
// FOR Mecocoa x64 on x86_64 ONLY
// DO NOT INC THIS - ONLY FOR INNER MECHANISM, which is true for parallels.

#ifndef _INC_ARCHITECT_MCCA_X64_
#define _INC_ARCHITECT_MCCA_X64_

#ifndef _INC_DEPEND_STDINT
// ---- ---- ISO/IEC stdint.h ---- ----

	#if defined(__INT8_TYPE__)
		typedef __INT8_TYPE__ int8_t;
	#else
		typedef signed char int8_t;
	#endif
	#if defined(__UINT8_TYPE__)
		typedef __UINT8_TYPE__ uint8_t;
	#else
		typedef unsigned char uint8_t;
	#endif
	#if defined(__INT16_TYPE__)
		typedef __INT16_TYPE__ int16_t;
	#else
		typedef short int16_t;
	#endif
	#if defined(__UINT16_TYPE__)
		typedef __UINT16_TYPE__ uint16_t;
	#else
		typedef unsigned short uint16_t;
	#endif
	#if defined(__INT32_TYPE__)
		typedef __INT32_TYPE__ int32_t;
	#else
		typedef int int32_t;
	#endif
	#if defined(__UINT32_TYPE__)
		typedef __UINT32_TYPE__ uint32_t;
	#else
		typedef unsigned uint32_t;
	#endif
	#if defined(__INT64_TYPE__)
		typedef __INT64_TYPE__ int64_t;
	#else
		typedef long long int64_t;
	#endif
	#if defined(__UINT64_TYPE__)
		typedef __UINT64_TYPE__ uint64_t;
	#else
		typedef unsigned long long uint64_t;
	#endif

	#if defined(__INT_LEAST8_TYPE__)
		typedef __INT_LEAST8_TYPE__ int_least8_t;
	#else
		typedef int8_t int_least8_t;
	#endif
	#if defined(__INT_FAST8_TYPE__)
		typedef __INT_FAST8_TYPE__ int_fast8_t;
	#else
		typedef int8_t int_fast8_t;
	#endif
	#if defined(__UINT_LEAST8_TYPE__)
		typedef __UINT_LEAST8_TYPE__ uint_least8_t;
	#else
		typedef uint8_t uint_least8_t;
	#endif
	#if defined(__UINT_FAST8_TYPE__)
		typedef __UINT_FAST8_TYPE__ uint_fast8_t;
	#else
		typedef uint8_t uint_fast8_t;
	#endif

	#if defined(__INT_LEAST16_TYPE__)
		typedef __INT_LEAST16_TYPE__ int_least16_t;
	#else
		typedef int16_t int_least16_t;
	#endif
	#if defined(__INT_FAST16_TYPE__)
		typedef __INT_FAST16_TYPE__ int_fast16_t;
	#elif defined(__GNUC__) || defined(__clang__)
		typedef short int_fast16_t;
	#else
		typedef int int_fast16_t;
	#endif
	#if defined(__UINT_LEAST16_TYPE__)
		typedef __UINT_LEAST16_TYPE__ uint_least16_t;
	#else
		typedef uint16_t uint_least16_t;
	#endif
	#if defined(__UINT_FAST16_TYPE__)
		typedef __UINT_FAST16_TYPE__ uint_fast16_t;
	#elif defined(__GNUC__) || defined(__clang__)
		typedef unsigned short uint_fast16_t;
	#else
		typedef unsigned uint_fast16_t;
	#endif

	#if defined(__INT_LEAST32_TYPE__)
		typedef __INT_LEAST32_TYPE__ int_least32_t;
	#else
		typedef int32_t int_least32_t;
	#endif
	#if defined(__INT_FAST32_TYPE__)
		typedef __INT_FAST32_TYPE__ int_fast32_t;
	#else
		typedef int32_t int_fast32_t;
	#endif
	#if defined(__UINT_LEAST32_TYPE__)
		typedef __UINT_LEAST32_TYPE__ uint_least32_t;
	#else
		typedef uint32_t uint_least32_t;
	#endif
	#if defined(__UINT_FAST32_TYPE__)
		typedef __UINT_FAST32_TYPE__ uint_fast32_t;
	#else
		typedef uint32_t uint_fast32_t;
	#endif

	#if defined(__INT_LEAST64_TYPE__)
		typedef __INT_LEAST64_TYPE__ int_least64_t;
	#else
		typedef int64_t int_least64_t;
	#endif
	#if defined(__INT_FAST64_TYPE__)
		typedef __INT_FAST64_TYPE__ int_fast64_t;
	#else
		typedef int64_t int_fast64_t;
	#endif
	#if defined(__UINT_LEAST64_TYPE__)
		typedef __UINT_LEAST64_TYPE__ uint_least64_t;
	#else
		typedef uint64_t uint_least64_t;
	#endif
	#if defined(__UINT_FAST64_TYPE__)
		typedef __UINT_FAST64_TYPE__ uint_fast64_t;
	#else
		typedef uint64_t uint_fast64_t;
	#endif

	#if defined(__INTMAX_TYPE__)
		typedef __INTMAX_TYPE__ intmax_t;
	#else
		typedef int64_t intmax_t;
	#endif
	#if defined(__UINTMAX_TYPE__)
		typedef __UINTMAX_TYPE__ uintmax_t;
	#else
		typedef uint64_t uintmax_t;
	#endif
	#if defined(__INTPTR_TYPE__)
		typedef __INTPTR_TYPE__ intptr_t;
	#else
		typedef int64_t intptr_t;
	#endif
	#if defined(__UINTPTR_TYPE__)
		typedef __UINTPTR_TYPE__ uintptr_t;
	#else
		typedef uint64_t uintptr_t;
	#endif

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

	#define INT_LEAST64_MIN INT64_MIN
	#define INT_LEAST64_MAX INT64_MAX
	#define UINT_LEAST64_MAX UINT64_MAX
	#define INT_FAST64_MIN INT64_MIN
	#define INT_FAST64_MAX INT64_MAX
	#define UINT_FAST64_MAX UINT64_MAX

	#define INTPTR_MIN INT64_MIN
	#define INTPTR_MAX INT64_MAX
	#define UINTPTR_MAX UINT64_MAX

	#define INTMAX_MIN INT64_MIN
	#define INTMAX_MAX INT64_MAX
	#define UINTMAX_MAX UINT64_MAX

	#define SIG_ATOMIC_MIN INT32_MIN
	#define SIG_ATOMIC_MAX INT32_MAX
#endif

#ifndef _INC_DEPEND_LIMITS
// limits.h
	#ifndef MB_LEN_MAX
		#define MB_LEN_MAX 5
	#endif

	#ifndef INT_MIN
		#define INT_MIN INT32_MIN
	#endif
	#ifndef INT_MAX
		#define INT_MAX INT32_MAX
	#endif
	#ifndef UINT_MAX
		#define UINT_MAX UINT32_MAX
	#endif

	#ifndef LONG_MIN
		#define LONG_MIN (-__LONG_MAX__ - 1L)
	#endif
	#ifndef LONG_MAX
		#define LONG_MAX __LONG_MAX__
	#endif
	#ifndef ULONG_MAX
		#define ULONG_MAX (2UL * __LONG_MAX__ + 1UL)
	#endif

	#ifndef LLONG_MAX
		#define LLONG_MAX INT64_MAX
	#endif
	#ifndef LLONG_MIN
		#define LLONG_MIN INT64_MIN
	#endif
	#ifndef ULLONG_MAX
		#define ULLONG_MAX UINT64_MAX
	#endif
#endif

#ifndef _INC_DEPEND_STDDEF
// stddef.h
	#ifndef SIZE_MAX
		#define SIZE_MAX UINT64_MAX
		#define PTRDIFF_MIN INT64_MIN
		#define PTRDIFF_MAX INT64_MAX
	#endif
	#if !defined(__PTRDIFF_TYPE__)
		typedef int64_t ptrdiff_t;
	#else
	typedef __PTRDIFF_TYPE__ ptrdiff_t;
	#endif
	#if !defined(__SIZE_TYPE__)
		typedef uint64_t size_t;
	#else
	typedef __SIZE_TYPE__ size_t;
	#endif
#endif

#endif
