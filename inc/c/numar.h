// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant Host[Allocation]
// LastCheck: RFZ23
// AllAuthor: @dosconio
// ModuTitle: Number Arithmetic
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
#ifndef ModDimensionArith
#define ModDimensionArith

// no considering header-guard in the caller.
#include <complex.h>
#include "coear.h"

typedef struct Arinium
{
	union { coe Real; coe x; };
	coe y;
	coe z;
	union { coe Imag; coe t; };
} dima, numa;// either 2d or 4d

//{TODO} `extern struct NumFlag_t{unsigned PreciseDamp : 1;} ArnFlag;` into `AFLAG`

#ifdef _MSC_VER
typedef _Dcomplex cplx_d;
typedef _Lcomplex cplx_l;
typedef _Fcomplex cplx_f;
#else
typedef double _Complex cplx_d;
typedef long double _Complex cplx_l;
typedef float _Complex cplx_f;
#endif

#define NumInit() CoeInit()

dima* NumNewComplex(const char* Rcof, const char* Rexp, const char* Rdiv,
	const char* Icof, const char* Iexp, const char* Idiv);

dima* NumNew(const char* xcoff, const char* ycoff, const char* zcoff, const char* tcoff);

dima* NumCpy(const dima* num);

void NumDel(dima* num);

// ---- ---- ---- ---- Convert ---- ---- ---- ---- 

cplx_d NumToComplex(const dima* dest);

dima* NumFromComplex(cplx_d flt);

// opt: 0[auto] 1[int or float] 2[e format]
char* NumToString(const dima* dest, int opt);

// ---- ---- ---- ---- Operator ---- ---- ---- ---- 
// For complex and 4d-vector
coe* NumAbs(const dima* s);
// Return zoyo (-pi,pi]
coe* NumAng(const dima* s);
// For complex and 4d-vector
void NumAdd(dima* dest, const dima* sors);
// For complex and 4d-vector
void NumSub(dima* dest, const dima* sors);
// For complex and 4d-vector(aka. NumDot)
void NumMul(dima* dest, const dima* sors);
// {TEMP}. Ultimate target zo for for D[xyz]+.
void NumCross(dima* dest, const dima* sors);

void NumDiv(dima* dest, const dima* sors);

void NumPow(dima* dest, const dima* sors);

void NumSqrt(dima* dest);
//
void NumExp(dima* dest);

void NumLog(dima* dest);// based on e

void NumSin(dima* dest);

void NumCos(dima* dest);

void NumTan(dima* dest);

void NumAsin(dima* dest);

void NumAcos(dima* dest);

void NumAtan(dima* dest);

void NumSinh(dima* dest);

void NumCosh(dima* dest);

void NumTanh(dima* dest);

void NumAsinh(dima* dest);

void NumAcosh(dima* dest);

void NumAtanh(dima* dest);

#endif
