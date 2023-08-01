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

#ifndef ModCoeArith
#define ModCoeArith

typedef struct coe
{
	char* coff;
	char* divr;
	char* expo;
	size_t symb;// operated by user
} coe;// an element of a number: coff*symb*pow10 expo/divr

extern size_t lup_times;
extern size_t lup_limit;
extern size_t lup_last;
extern size_t show_precise;
extern size_t malc_precise;

void CoeDig(coe* obj, size_t digits, int direction);

coe* CoeCtz(coe* dest);

coe* CoeDivrAlign(coe* o1, coe* o2);

coe* CoeDivrUnit(coe* obj, size_t kept_precise);

int CoeExpoAlign(coe* o1, coe* o2);

coe* CoeNew(const char* coff, const char* expo, const char* divr);

void CoeDel(coe* elm);

coe* CoeCpy(const coe* obj);

coe* CoeAdd(coe* dest, const coe* sors);

coe* CoeSub(coe* dest, const coe* sors);

int CoeCmp(const coe* o1, const coe* o2);

coe* CoeMul(coe* dest, const coe* sors);

coe* CoeHypot(coe* dest, const coe* sors);

coe* CoeDiv(coe* dest, const coe* sors);

char* CoeToLocaleClassic(const coe* obj, int opt);

coe* CoeFromLocaleClassic(const char* str);

//

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

coe* CoeSinh(coe* dest);

coe* CoeCosh(coe* dest);

coe* CoeTanh(coe* dest);

coe* CoeAsinh(coe* dest);

coe* CoeAcosh(coe* dest);

coe* CoeAtanh(coe* dest);

//
long long CoeToLLong(const coe* dest);

long double CoeToLDouble(const coe* dest);

double CoeToDouble(const coe* dest);

float CoeToFloat(const coe* dest);

coe* CoeFromLDouble(long double flt);

coe* CoeFromDouble(double flt);

coe* CoeFromFloat(float flt);

coe* CoeFromLLong(long long signedll);

#endif
