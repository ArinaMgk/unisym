// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant Host[Allocation]
// LastCheck: RFZ23
// AllAuthor: @dosconio
// ModuTitle: Register-united Arithmetic
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

// - includes single and arina-formed and 4-dimension 3 styles
// - process-unit: size_t ptrdiff_t
//    need to be times of `int`, at least twice of `char`.
// - assume the number input is reducted.
// - when expo == 0, expsign must be 0.
// - when coff == 0, cofsign must be 0.
// [Limit]
// Use malc_limit as the limit of default 3 tempors.

#ifndef ModRegisterBasedFloating
#define ModRegisterBasedFloating
// Also-known-as Regular Arith.

#include <stddef.h>
#include <limits.h>
#include "alice.h"

// [LIMIT] Length is times of size_t or ptrdiff_t, but byte.
extern size_t regcof,// for common result
	regexp,
	regdiv;

//---- ---- ---- ---- Single Style ---- ---- ---- ---- 
// [Suggested Arina RFB20] better a-k-a BcdAr.
// This is unsigned and in 256-system(Arina-Covenant Cliche^).
// At least 2 times of byte.

size_t RsgDiv8(size_t* sstr, size_t slen, unsigned char divr);

signed RsgCmp(const size_t* a, const size_t* b, size_t alen, size_t blen);

int RsgMul8(size_t* sstr, size_t slen, unsigned char mult);

int RsgAdd8(size_t* sstr, size_t slen, unsigned char plus);

int RsgSub8(size_t* sstr, size_t slen, unsigned char subr);

size_t* _Need_free RsgResize(const size_t* sstr, size_t slen, size_t dlen);

char* _Need_free RsgToString(const size_t* sstr, size_t slen, signed syst);

int RsgAdd(size_t* dstr, const size_t* sstr, size_t comlen);

int RsgSub(size_t* dstr, const size_t* sstr, size_t comlen);

void RsgSubComple(size_t* dstr, const size_t* sstr, size_t comlen);

int _Heap RsgMul(size_t* dstr, const size_t* sstr, size_t comlen);

int _Heap_tmpher RsgDiv(size_t* dstr, size_t* sstr, size_t comlen);

size_t* _Need_free RsgNew(const size_t* sors, size_t len);

size_t* _Need_free RsgImm(size_t sors, size_t len);

int _Heap_tmpher RsgPow(size_t* base, const size_t* expo, size_t comlen);

int _Heap RsgArrange(size_t* total, const size_t* items, size_t comlen);

int _Heap RsgFactorial(size_t* base, size_t comlen);

int _Heap RsgCombinate(size_t* total, const size_t* items, size_t comlen);

int _Heap RsgComDiv(size_t* d, const size_t* s, size_t comlen);

int _Heap RsgComMul(size_t* d, const size_t* s, size_t comlen);

//---- ---- ---- ---- Phinae Style ---- ---- ---- ---- 
// coff * 10^{expo} / divr = number (ArinaMgk style)
// coff * 0x10^{expo} / divr = number (Haruno style)
#define PHIANE_REGAR_LENLEN (sizeof(size_t)*CHAR_BIT-3)
#define TAYLOR_EXP 0b10000000
#define TAYLOR_SIN 0b10010101
#define TAYLOR_COS 0b10100001
#define TAYLOR_LN  0b01000101
#define TAYLOR_ATAN 0b01010101

typedef struct RegP_t
{
	size_t* coff;
	size_t* expo;
	size_t* divr;
	struct 
	{
		unsigned cofsign : 1;// 1 for neg
		unsigned expsign : 1;
		unsigned preloss : 1;// precise loss
		size_t defalen : PHIANE_REGAR_LENLEN;
	};
} Rfnar_t;

Rfnar_t* _Need_free RedNew(size_t* coff, size_t* expo, size_t* divr, size_t len);

Rfnar_t* _Need_free RedNewImm(size_t coff, size_t expo, size_t divr, size_t len);

// all _Heap:

void RedDel(Rfnar_t* elm);

Rfnar_t* RedHeap(const Rfnar_t* obj);

void RedDig(Rfnar_t* dest, size_t prec);

void RedZip(Rfnar_t* dest);

void RedAlignExpo(Rfnar_t* d, Rfnar_t* s);

void RedExpoZero(Rfnar_t* d);// RFV2 ArnMgk

void RedDivrUnit(Rfnar_t* d);

void RedReduct(Rfnar_t* d);

void RedAlign(Rfnar_t* d, Rfnar_t* s);

int RedAdd(Rfnar_t* d, const Rfnar_t* s);

int RedSub(Rfnar_t* d, const Rfnar_t* s);

int RedCmp(const Rfnar_t* d, const Rfnar_t* s);

int RedMul(Rfnar_t* dest, const Rfnar_t* sors);

int RedDiv(Rfnar_t* dest, const Rfnar_t* sors);

double _Heap RedToDouble(const Rfnar_t* dest);

Rfnar_t* _Need_free RedFromDouble(double flt, size_t rfnumlen, size_t FetchDigits);

//---- ---- ---- ---- Haruno Style ---- ---- ---- ---- 
// [Suggested Arina RFB19] common used by Complex and the universal vector:
typedef struct RegH_t
{
	Rfnar_t x, y, z, t;
} Hrnar_t, posi_t;

Hrnar_t* HrnImm(size_t xcoff, size_t xexpo, size_t xdivr,\
	size_t ycoff, size_t yexpo, size_t ydivr,\
	size_t zcoff, size_t zexpo, size_t zdivr,\
	size_t tcoff, size_t texpo, size_t tdivr,\
	size_t comlen);

Hrnar_t* HrnCpy(const Hrnar_t* obj);

void HrnClr(Hrnar_t* d);

void HrnAdd(Hrnar_t* d, const Hrnar_t* s);

void HrnSub(Hrnar_t* d, const Hrnar_t* s);

Hrnar_t* HrnCross(const Hrnar_t* d, const Hrnar_t* s);

Rfnar_t* HrnDot(const Hrnar_t* d, const Hrnar_t* s);

Rfnar_t* _Need_free HrnAbs(const Hrnar_t* d);

#endif // !ModRegisterBasedFloating

// ISSUE VER FIRST OF FB~17
// > Simulate Heap
// > Explicit heap and buffer
// > buffer mode specific Default-3-buffers
// > Have not test in dbg mode
// > Hrn-part has not been tested.
// > -Xxx8 unusual order of parameters. {too Lazy to change in this ver.}
// > RsgDig, unreasonable, should be like Phinae.
// > The symbols are very confused and not systematic, these are going to be more systematic in the next version.
// > expo can just use the least one size_t.
// > just tested x86.
// > going to use "half-register" as arithmetic unit.
// > e.g. "1+0.000001", if 0.000001 can not be expressed by the fixed register, nothing will be done.
// EOF
