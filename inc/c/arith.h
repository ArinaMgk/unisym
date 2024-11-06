// UTF-8 C99 TAB4 CRLF
// LastCheck: 2023 Nov 16
// AllAuthor: @ArinaMgk; @dosconio
// ModuTitle: Processor inside Math / Arithmetic
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

#ifndef _INC_ARITHMETIC
#define _INC_ARITHMETIC

#include "stdinc.h"
#include "coear.h"
#include "number.h"

// π
#define _VAL_PI 3.14159265358979323846264338327950288419716939937510582097494459
// ln2
#define _VAL_L2 0.69314718055994530941723212145817656807550013436025525412068
// e
#define _VAL_E  2.7182818284590452353602874713526624977572470937


/*//{TODO}
type
- float_t
- double_t

macro
- HUGE_VAL
- HUGE_VALF
- HUGE_VALL
- INFINITY
- NAN
- FP_INFINITE
- FP_NAN
- FP_NORMAL
- FP_SUBNORMAL
- FP_ZERO
- FP_FAST_FMA
- FP_FAST_FMAF
- FP_FAST_FMAL
- FP_ILOGB0
- FP_ILOGBNAN
- MATH_ERRNO     1
- MATH_ERREXCEPT 2
- math_errhandling

macro-style function
- int fpclassify(*real-floating* x);
- int isfinite(*real-floating* x);
- int isinf(*real-floating* x);
- int isnormal(*real-floating* x);
- int signbit(*real-floating* x);
*/
#ifdef _INC_CPP
extern "C" {
#endif
/*
function
- version for `float` and `long double` of all belows.
- atan2
- aXXXh
- exp2 (by bits logical)
- expml
- frexp
- ilogb
- ldexp
- log10
- log1p
- log2
- logb
- modf
- scalbn
- scalbln
- cbrt
- hypot
- erf
- erfc
- lgamma
- tgamma
- ceil
- floor
- nearbyint
- rint
- lrint
- llrint
- round
- lround
- llround
- trunc
- fmod
- remainder
- remquo
- copysign
- nan
- nextafter
- nexttoward
- fdim
- fmax
- fmin
- fma
- isgreater
- isgreaterequal
- isless
- islessequal
- islessgreater
- isunordered

pragma
- FP_CONTRACT
*/

extern stduint _EFDIGS;// how many effective digits
void ariprecise(stduint prec);

// ---- { stdlib.h } ----
//#define abs(i) intabs(i) 
int intabs(int j);
int abs(int j);
long int labs(long int j);
long long int llabs(long long int j);


// ---- END

stduint intFibonacci(stduint idx);

// Floor method
inline static stduint intlog2_iexpo(stduint v) {
	stduint crt = 0;
	if (!v) return 0;
	while (v >>= 1) crt++;
	return crt;
}

inline static stduint intpow2_iexpo(stduint expo)
{
	return _IMM1 << expo;
}

double dblexp(double expo);
// based on `e`
double dbllog(double power);

double dblpow_iexpo(double bas, stdint exp);

double dblpow_fexpo(double bas, double exp);

//{TODO} and there will cpxpow_cexpo for Complex arithmetics

// fabs
double dblabs(double inp);

double dblsqrt(double inp);

// ---- angle ----

double dblsin(double rad);

double dblcos(double rad);

double dbltan(double rad);

double dblasin(double val);

double dblacos(double val);

double dblatan(double val);

double dblsinh(double rad);

double dblcosh(double rad);

double dbltanh(double rad);

// Dep(pow_f)
double dblfactorial(double inp);

#define _CDETayFunc_Exp 0b10000000
#define _CDETayFunc_Sin 0b10010101
#define _CDETayFunc_Cos 0b10100001// cos yo (0,2]
#define _CDETayFunc_Log 0b01000101// ln(x+1)
#define _CDETayFunc_Atn 0b01010101// arctan [-1,+1]
#define _CDETayDptr_DivType(x) ((0b11000000&(x))>>6)
#define _CDETayDptr_PowType(x) ((0b00110000&(x))>>4)
#define _CDETayDptr_PoStart(x) ((0b00001100&(x))>>2)
#define _CDETayDptr_PowSign(x) ((0b00000010&(x))>>1)// [bit]
#define _CDETayDptr_SignTog(x) (0b00000001&(x))// [bit]
enum TaylorType {
	TaylorExp = _CDETayFunc_Exp,
	TaylorSin = _CDETayFunc_Sin,
	TaylorCos = _CDETayFunc_Cos,
	TaylorLog = _CDETayFunc_Log,
	TaylorAtn = _CDETayFunc_Atn,
};
// Taylors is writen by Haruno RFR.
//MSB[Divr 00:1 01:n 10:n! 11:n*n]
//   [00:All 01:OnlyOdd 10: OnlyEven]
//   [StartPower 0~3]
//   [StartSign 0:+ 1:-]
//LSB[SignFlap 0:N 1:Y]
double dbltaylor(double inp, enum TaylorType dptor, double period);

#ifdef _INC_CPP
}
#endif

#endif
