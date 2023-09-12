// ASCII
/*
	Copyright 2023 ArinaMgk

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/
#ifndef ModComplexternArith
#define ModComplexternArith

// no considering header-guard in the caller.
#include <complex.h>
#include "cdear.h"

typedef struct Complextern
{
	coe Real, Imag;
} Scalar, scalar;// no using CoeDel

typedef struct Arinium// Arina time and space descriptor position.
{
	scalar x;
	scalar y;
	scalar z;
	scalar t;
} Arnum;// do not using ArnDel

struct NumFlag_t
{
	unsigned PreciseDamp : 1;
};
extern struct NumFlag_t ArnFlag;

#ifdef _MSVC
typedef _Dcomplex cplx_d;
typedef _Lcomplex cplx_l;
typedef _Fcomplex cplx_f;
#else
typedef double _Complex cplx_d;
typedef long double _Complex cplx_l;
typedef float _Complex cplx_f;
#endif

scalar* NumNew(const char* Rcof, const char* Rexp, const char* Rdiv,
	const char* Icof, const char* Iexp, const char* Idiv);

scalar* NumCpy(const scalar* num);

void NumDel(scalar* num);

// Convert

cplx_l NumToLDComplex(const scalar* dest);

cplx_d NumToDComplex(const scalar* dest);

cplx_f NumToFComplex(const scalar* dest);

scalar* NumFromLDComplex(cplx_l flt);

scalar* NumFromDComplex(cplx_d flt);

scalar* NumFromFComplex(cplx_f flt);

scalar* NumFromLLong(long long signedll);

// Operator

void NumAdd(scalar* dest, const scalar* sors);

void NumSub(scalar* dest, const scalar* sors);

void NumMul(scalar* dest, const scalar* sors);

void NumDiv(scalar* dest, const scalar* sors);

void NumFactorial(scalar* dest);

void NumPow(scalar* dest, const scalar* sors);

void NumSin(scalar* dest);

void NumCos(scalar* dest);

void NumTan(scalar* dest);

// No recommendation, below.
void NumAsin(scalar* dest);

void NumAcos(scalar* dest);

void NumAtan(scalar* dest);

void NumLog(scalar* dest);// Ln

void NumExp(scalar* dest);

void NumSinh(scalar* dest);

void NumCosh(scalar* dest);

void NumTanh(scalar* dest);

void NumAsinh(scalar* dest);

void NumAcosh(scalar* dest);

void NumAtanh(scalar* dest);

// opt: 0[auto] 1[int or float] 2[e format]
size_t NumToString(const scalar* dest, int opt, char* buf, size_t buflen);

char* NumToStringAuto(const scalar* dest, int opt);

#endif
