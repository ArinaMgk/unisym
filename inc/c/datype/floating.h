// ASCII CPL TAB4 CRLF
// Docutitle: Floating
// Datecheck: 20240421 ~ <Last-check>
// Developer: @dosconio
// Attribute: <ArnCovenant> <Environment> <Platform>
// Reference: <Reference>
// Dependens: <Dependence>
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

//{Option} {TODO!} software implementation or hardware implementation (by instruction by driving Co-processor)

// Compatible with:
//{TODO} - ISO float.h
//{TODO} - ISO fenv.h


#ifndef _INC_FLOATING
#define _INC_FLOATING

#include "integer.h"

#define isZeroMantissa(flt) ((int)(flt)==(flt)) 
#define isNaN(f) ((f) != (f))
#define getNearInteger(flt) (flt+0.5)

typedef float float32;
typedef double float64;

#ifdef _BIT_SUPPORT_DFLOAT
inline static double CastF64FromU0(stduint i) {
	//{TODO} Soft Implementation, while this is hard implementation
	return (double)i;
}
#endif

#if defined(_DEV_GCC) && defined(_Intelx86o64_Windows_64) && \
	((defined (__STDC_VERSION__) && __STDC_VERSION__ >= 201112L) || (defined(__cplusplus) && __cplusplus >= 201103L)) \
	// && !def(_CRT_ERRNO_DEFINED)
typedef struct {
	long long __max_align_ll __attribute__((__aligned__(__alignof__(long long))));
	long double __max_align_ld __attribute__((__aligned__(__alignof__(long double))));
	#ifdef __i386__
	__float128 __max_align_f128 __attribute__((__aligned__(__alignof(__float128))));
	#endif
} max_align_t;
#endif // C11 or C++11
// #define _CRT_ERRNO_DEFINED


//{for GCC: STDDEF}
#ifdef _Intelx86o64_Windows_64
#include "../msgface.h"
#endif

// borrow from NASM

#ifdef _INC_CPP
namespace uni {
#endif

	enum float_round_t {
		FLOAT_RC_NEAR,
		FLOAT_RC_ZERO,
		FLOAT_RC_DOWN,
		FLOAT_RC_UP,
	};

#ifdef _INC_CPP
}
extern "C" {
#endif

int float_const(const char* string, int sign, uint8_t* result, int bytes);
int float_option(const char* option);

extern void (*fp_normal)(rostr);
extern Handler_t fp_over_digits;
extern Handler_t fp_fn1;
extern Handler_t fp_fn_underflow, fp_fn_overflow, fp_fn_denormal;

#ifdef _INC_CPP
}
#endif


/* AASM
 test   i via rax self
 JNS {
 pxor   xmm0 self
 cvtsi2sd xmm0,rax
} else {
 mov    rdx,rax
 rdx <<= 1
 eax  &= 1
 rdx  |= rax
 pxor   xmm0 self
 cvtsi2sd xmm0,rdx
 addsd  xmm0 self
}
*/

#endif
