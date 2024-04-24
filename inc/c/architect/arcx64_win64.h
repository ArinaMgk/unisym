// ASCII C99 TAB4 CRLF
// LastCheck: RFZ23
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

#ifndef _INC_ARCHITECT_WIN64_
#define _INC_ARCHITECT_WIN64_

// ---- ---- ISO/IEC stdint.h ---- ----

	typedef signed char int8_t;
	typedef unsigned char   uint8_t;
	typedef short  int16_t;
	typedef unsigned short  uint16_t;
	typedef int  int32_t;
	typedef unsigned   uint32_t;
	typedef long long  int64_t;
	typedef unsigned long long   uint64_t;

	typedef signed char int_least8_t;
	typedef signed char int_fast8_t;
	typedef unsigned char   uint_least8_t;
	typedef unsigned char uint_fast8_t;

	typedef short  int_least16_t;
	#if defined(__GNUC__)
		typedef short  int_fast16_t;
	#else
		typedef int    int_fast16_t;
	#endif
	typedef unsigned short  uint_least16_t;
	#if defined(__GNUC__)
		typedef unsigned short  uint_fast16_t;
	#else
		typedef unsigned  uint_fast16_t;
	#endif
	
	typedef int  int_least32_t;
	typedef int  int_fast32_t;
	typedef unsigned   uint_least32_t;
	typedef unsigned   uint_fast32_t;

	typedef long long  int_least64_t;
	typedef long long  int_fast64_t;
	typedef unsigned long long   uint_least64_t;
	typedef unsigned long long   uint_fast64_t;

	typedef long long  intmax_t;
	typedef unsigned long long   uintmax_t;

	#define INT8_MIN (-128)
	#define INT8_MAX 127
	#define UINT8_MAX 255

	#define INT16_MIN (-32768)
	#define INT16_MAX 32767
	#define UINT16_MAX 65535

	#define INT32_MIN (-2147483647 - 1)
	#define INT32_MAX 2147483647
	#define UINT32_MAX 0xffffffffU

	#define INT64_MIN  (-9223372036854775807LL - 1)
	#define INT64_MAX 9223372036854775807LL
	#define UINT64_MAX 0xffffffffffffffffULL

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
	//

	#define INTPTR_MIN INT64_MIN
	#define INTPTR_MAX INT64_MAX
	#define UINTPTR_MAX UINT64_MAX

	#define INTMAX_MIN INT64_MIN
	#define INTMAX_MAX INT64_MAX
	#define UINTMAX_MAX UINT64_MAX

	#define SIG_ATOMIC_MIN INT32_MIN
	#define SIG_ATOMIC_MAX INT32_MAX

	#ifndef SIZE_MAX
		#define SIZE_MAX UINT64_MAX
		#define PTRDIFF_MIN INT64_MIN
		#define PTRDIFF_MAX INT64_MAX
	#endif
#endif // !_HOST_WIN64_
