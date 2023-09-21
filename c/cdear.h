// ASCII Powerful Number of Arinae
// The first generation is outdated, this 2-gen is the first open-sourced version.
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
// ASCII C99 Generation-2nd:CoeAr. Ultimate Version
// Based on ustring heap version

// State Number
// * INF: COF=NON0 DIV=0
// * NAN: COF=0    DIV=0

// Rule: 
// * a sign prefix is a must.
// * aflag will be applied
// * the sign of divr is omitted and should always be positive
// * State Number, but also with erro() call
// * Arn is going to setup dnode-malc-table and warn-table (from old generation)

#pragma warning(disable:4068)
#pragma source ./source/cdear.c

#ifndef ModCoeArith
#define ModCoeArith

typedef struct PhinaeCDEAr
{
	char* coff;
	char* divr;
	char* expo;
	size_t symb;
} coe, cde;// Current, seemingly since SGA-Generaion II

struct PhinaeCDEArOld
{
	char* coff, * divr, * expo;
};// since SGA-Generation I


#define _CDE_PRECISE_SHOW_DEFAULT 63
#define _CDE_PRECISE_ARITHMETIC_DEFAULT 79
#define _CDE_PRECISE_SHOW_LOCALE_EXPONENT 8
#define _CDE_PRECISE_LOOPTIMES_LEAST_DEFAULT 16
#define _CDE_PRECISE_LOOPTIMES_LIMIT_DEFAULT 32

// {TODO} coe ---> cde
// {TODO} cut the dig for each loop of Taylor family

// Initial and set the necessary system structure.
void CoeInit();

// Reset the length of the digits. If the real precise is less, zero will be moved at the end of coff from expo; if more, will cut.
void CoeDig(coe* obj, size_t digits, int direction);

// Cut trailing zeros of coff and append to expo. This is usually used at the end of the operation, so some adaptation is done here. 
coe* CoeCtz(coe* dest);

coe* CoeDivrAlign(coe* o1, coe* o2);

coe* CoeDivrUnit(coe* obj, size_t kept_precise);

int CoeExpoAlign(coe* o1, coe* o2);

coe* CoeNew(const char* coff, const char* expo, const char* divr);

void CoeDel(coe* elm);

coe* CoeCpy(const coe* obj);

#define CdeAdd CoeAdd
coe* CoeAdd(coe* dest, const coe* sors);

#define CdeSub CoeSub
coe* CoeSub(coe* dest, const coe* sors);

int CoeCmp(const coe* o1, const coe* o2);

#define CdeMul CoeMul
coe* CoeMul(coe* dest, const coe* sors);

coe* CoeHypot(coe* dest, const coe* sors);

#define CdeDiv CoeDiv
coe* CoeDiv(coe* dest, const coe* sors);

//

// CoeRem: make dest < unit_period

// Clear the mantissa part. Based on CoeExpoAlign().
coe* CoeInt(coe* dest);

coe* CoePow(coe* dest, const coe* sors);

// CoeSqrt

// CoeCbrt

coe* CoeSin(coe* dest);

coe* CoeCos(coe* dest);

coe* CoeTan(coe* dest);

coe* CoeAsin(coe* dest);

coe* CoeAcos(coe* dest);

coe* CoeAtan(coe* dest);

// Log`10(x) == Log`(default e)(x)/Log`e(10)
coe* CoeLog(coe* dest);

int CoeSgn(const coe* dest);

coe* CoeExp(coe* dest);//TEST-STAGE

coe* CoeFac(coe* dest);

coe* CoePi();

coe* CoeE();

coe* CoeSinh(coe* dest);

coe* CoeCosh(coe* dest);

coe* CoeTanh(coe* dest);

coe* CoeAsinh(coe* dest);

coe* CoeAcosh(coe* dest);

coe* CoeAtanh(coe* dest);

// ---- ---- ---- ---- Conversion ---- ---- ---- ----

#define CdeToLocale CoeToLocale
// opt: 0[auto] 1[int or float] 2[e format]
char* CoeToLocale(const coe* obj, int opt);

#define CdeFromLocale CoeFromLocale
// +12.27e+3 12.27 [+A].[B]e[+C] -> COE (fail then 0)
coe* CoeFromLocale(const char* str);

#define CdeToDouble CoeToDouble
double CoeToDouble(const coe* dest);

#define CdeFromDouble CoeFromDouble
coe* CoeFromDouble(double flt);


#endif
