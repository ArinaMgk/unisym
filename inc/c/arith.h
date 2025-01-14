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
- hypot
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
// 0->0 1->0 2->1 3->1 4->2 5->2 ...
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

// Ceiling method
// 0->0 1->1 2->1 3->2 4->2 5->3 ...
inline static stduint intlog2__iexpo(stduint v) {
	stduint floor = intlog2_iexpo(v);
	stduint cmpr = intpow2_iexpo(floor);
	return (cmpr == v) ? floor : floor + 1;
}

double dblexp(double expo);
#if defined(_USE_INNER_MATH)
#define expf dblexp // TEMP no float ver
#else
float expf(float val);
double exp(double val);
long double expl(long double val);
#endif

//{} exp2 (by bits logical)
#if defined(_USE_INNER_MATH)
#define exp2f dblexp2 // TEMP no float ver
#else
float exp2f(float val);
double exp2(double val);
long double exp2l(long double val);
#endif
//{} expml ? m1 ?
#if defined(_USE_INNER_MATH)
#define expm1f dblexpm1 // TEMP no float ver
#else
float expm1f(float val);
double expm1(double val);
long double expm1l(long double val);
#endif



// based on `e`
double dbllog(double power);
#if defined(_USE_INNER_MATH)
#define logf dbllog // TEMP no float ver
#else
float logf(float val);
double log(double val);
long double logl(long double val);
#endif
//{} log10
#if defined(_USE_INNER_MATH)
#define log10f dbllog10 // TEMP no float ver
#else
float log10f(float val);
double log10(double val);
long double log10l(long double val);
#endif
//{} log2
#if defined(_USE_INNER_MATH)
#define log2f dbllog2 // TEMP no float ver
#else
float log2f(float val);
double log2(double val);
long double log2l(long double val);
#endif
//{} log1p
#if defined(_USE_INNER_MATH)
#define log1pf dbllog1p // TEMP no float ver
#else
float log1pf(float val);
double log1p(double val);
long double log1pl(long double val);
#endif
//{} logb
#if defined(_USE_INNER_MATH)
#define logbf dbllogb // TEMP no float ver
#else
float logbf(float val);
double logb(double val);
long double logbl(long double val);
#endif

double dblpow_iexpo(double bas, stdint exp);

double dblpow_fexpo(double bas, double exp);

//{TODO} and there will cpxpow_cexpo for Complex arithmetics

// fmin
#if defined(_USE_INNER_MATH)
#define fminf dblmin // TEMP no float ver
#else
float fminf(float x, float y);
double fmin(double x, double y);
long double fminl(long double x, long double y);
#endif

// fmax
#if defined(_USE_INNER_MATH)
#define fmaxf dblmax // TEMP no float ver
#else
float fmaxf(float x, float y);
double fmax(double x, double y);
long double fmaxl(long double x, long double y);
#endif


// fabs
double dblabs(double inp);
#if defined(_USE_INNER_MATH)
#define fabsf dblabs // TEMP no float ver
#else
float fabsf(float val);
double fabs(double val);
long double fabsl(long double val);
#endif

//{} fdim
#if defined(_USE_INNER_MATH)
#define fdimf dblfdim // TEMP no float ver
#else
float fdimf(float x, float y);
double fdim(double x, double y);
long double fdiml(long double x, long double y);
#endif

double dblsqrt(double inp);
#if defined(_USE_INNER_MATH)
#define sqrtf dblsqrt // TEMP no float ver
#else
float sqrtf(float val);
double sqrt(double val);
long double sqrtl(long double val);
#endif

// ---- angle ----

double dblsin(double rad);
#if defined(_USE_INNER_MATH)
#define sinf dblsin // TEMP no float ver
#else
float sinf(float val);
double sin(double val);
long double sinl(long double val);
#endif

double dblcos(double rad);
#if defined(_USE_INNER_MATH)
#define cosf dblcos // TEMP no float ver
#else
float cosf(float val);
double cos(double val);
long double cosl(long double val);
#endif

double dbltan(double rad);
#if defined(_USE_INNER_MATH)
#define tanf dbltan // TEMP no float ver
#else
float tanf(float val);
double tan(double val);
long double tanl(long double val);
#endif

double dblasin(double val);
#if defined(_USE_INNER_MATH)
#define asinf dblasin // TEMP no float ver : fltasin
#else
float asinf(float val);
double asin(double val);
long double asinl(long double val);
#endif

//{} dblasinh
#if defined(_USE_INNER_MATH)
#define asinhf dblasinh // TEMP no float ver
#else
float asinhf(float val);
double asinh(double val);
long double asinhl(long double val);
#endif

double dblacos(double val);
#if defined(_USE_INNER_MATH)
#define acosf dblacos // TEMP no float ver
#else
float acosf(float val);
double acos(double val);
long double acosl(long double val);
#endif

//{} dblacosh
#if defined(_USE_INNER_MATH)
#define acoshf dblacosh // TEMP no float ver
#else
float acoshf(float val);
double acosh(double val);
long double acoshl(long double val);
#endif

double dblatan(double val);
#if defined(_USE_INNER_MATH)
#define atanf dblatan // TEMP no float ver
#else
float atanf(float val);
double atan(double val);
long double atanl(long double val);
#endif

//{} dblatan2
#if defined(_USE_INNER_MATH)
#define atan2f dblatan2 // TEMP no float ver
#else
float atan2f(float y, float x);
double atan2(double y, double x);
long double atan2l(long double y, long double x);
#endif

//{} dblatanh
#if defined(_USE_INNER_MATH)
#define atanhf dblatanh // TEMP no float ver
#else
float atanhf(float val);
double atanh(double val);
long double atanhl(long double val);
#endif

double dblsinh(double rad);
#if defined(_USE_INNER_MATH)
#define sinhf dblsinh // TEMP no float ver
#else
float sinhf(float val);
double sinh(double val);
long double sinhl(long double val);
#endif

double dblcosh(double rad);
#if defined(_USE_INNER_MATH)
#define coshf dblcosh // TEMP no float ver
#else
float coshf(float val);
double cosh(double val);
long double coshl(long double val);
#endif

double dbltanh(double rad);
#if defined(_USE_INNER_MATH)
#define tanhf dbltanh // TEMP no float ver
#else
float tanhf(float val);
double tanh(double val);
long double tanhl(long double val);
#endif

//{} cbrt
#if defined(_USE_INNER_MATH)
#define cbrtf dblcbrt // TEMP no float ver
#else
float cbrtf(float val);
double cbrt(double val);
long double cbrtl(long double val);
#endif

//{} erf and erfc
#if defined(_USE_INNER_MATH)
#define erff dblerf // TEMP no float ver
#define erfcf dblerfc // TEMP no float ver
#else
float erff(float val);
double erf(double val);
long double erfl(long double val);
float erfcf(float val);
double erfc(double val);
long double erfcl(long double val);
#endif

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
