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

#define _LIB_STRING_HEAP
#include <stdlib.h>
#include <setjmp.h>
#include <limits.h>
#include <float.h>
#include <math.h>
#include "../aldbg.h"
#include "../ustring.h"
#include "../cdear.h"

#define show_precise LIB_CDE_PRECISE_SHOW

#define boolean int
#define coe_const(iden,co,ex) static const coe iden = {.coff=(co), .divr="+1", .expo=(ex)};

size_t show_precise = _CDE_PRECISE_SHOW_DEFAULT;
static size_t lup_times = _CDE_PRECISE_LOOPTIMES_LEAST_DEFAULT;
static size_t lup_limit = _CDE_PRECISE_LOOPTIMES_LIMIT_DEFAULT;
static size_t lup_last;// [OUT]

//TODO. temporary use
//pi=3.1415926535 8979323846 2643383279 5028841971 6939937510
//     5820974944 5923078164 0628620899 8628034825 3421170679
static const coe coeinf = { .coff = "+1", .divr = "+0", .expo = "+0" };
static const coe coenan = { .coff = "+0", .divr = "+0", .expo = "+0" };
coe_const(coepi, "+3141592653589793238462643383279502884197169399375105820974944592", "-63")
coe_const(coe2pi, "+6283185307179586476925286766559005768394338798750211641949889185", "-63")
coe_const(coepi_half, "+1570796326794896619231321691639751442098584699687552910487472296", "-63")
coe_const(coepi_quarter, "+785398163397448309615660845819875721049292349843776455243736148", "-63")
coe_const(coeln2, "+6931471806094223", "-16")
coe_const(coeone, "+1", "+0")
coe_const(coenegone, "-1", "+0")
coe_const(coezero, "+0", "+0")
coe_const(coetwo, "+2", "+0")
coe_const(coefour, "+4", "+0")
static const coe coehalf = { .coff = "+1", .divr = "+2", .expo = "+0" };

static size_t _DIG_CUT = 4;

void CoeInit()
{
	MemSet(&aflag, 0, sizeof aflag);
	aflag.Signed = 1;
}

void CoeDig(coe* obj, size_t digits, int direction)
{
	if (obj->divr[1] == '0') return;
	if (!digits) digits = show_precise;
	size_t CrtPrecise = 0;
	while (obj->coff[CrtPrecise + 1]) CrtPrecise++;
	if (digits == CrtPrecise) return;
	else if (CrtPrecise > digits)// Cut. CrtPrecise is at least 2, which greater than 9.
	{
		//+123 CrtPrecise=3
		//0123
		// Appended RFW22:16:40
		int rest_al_zero = 1;
		{
			size_t tmplen = 1;
			char tmpc;
			while (tmpc = obj->coff[digits + tmplen])
			{
				if (tmpc != '0')
				{
					rest_al_zero = 0;
					break;
				}
				tmplen++;
			}
		}
		// diection:0: need not anything
		if (direction == 2 // Nearest, e.g. 1.5 to 2
			&& obj->coff[digits + 1] >= '5') DigInc('1', obj->coff + digits);
		else if (!rest_al_zero)
		{
			size_t coflen = 0;
			while (obj->coff[coflen])coflen++;
			if (direction == 1)// to +inf
				if (*obj->coff == '+')
					DigInc('1', obj->coff + digits);
				else;// '-'
			else if (direction == 3)// to -inf
				if (*obj->coff == '+')
					;
				else DigInc('1', obj->coff + digits);
			else if (direction == 4)// to out
				DigInc('1', obj->coff + digits);
		}
		obj->coff[digits + 1] = 0;
		char* internum;
		srs(obj->expo, ChrAdd(obj->expo, internum = instoa(CrtPrecise - digits)));
		memfree(internum);
	}
	// Here are RFW21:11:35 appended for adding suffixed zero.
	else
	{
		// the diff zo (digits-CrtPrecise)
		char* tmp = obj->coff;
		size_t tmplen = StrLength(tmp);
		obj->coff = (char*)malloc(1 + tmplen + (digits - CrtPrecise));
		StrCopy(obj->coff, tmp);
		obj->coff[tmplen + (digits - CrtPrecise)] = 0;
		MemSet(obj->coff + tmplen, '0', digits - CrtPrecise);
		free(tmp);
		tmp = instoa(digits - CrtPrecise);
		srs(obj->expo, ChrSub(obj->expo, tmp));
		memfree(tmp);
	}
}

coe* CoeCtz(coe* dest)
{
	if (dest->divr[1] == '0') return dest;
	char* tmpptr;
	srs(dest->expo, ChrAdd(dest->expo, tmpptr = instoa(ChrCtz(dest->coff))));
	memfree(tmpptr);
	if (dest->coff[1] == '0')
	{
		dest->coff[0] = '+';
		srs(dest->expo, StrHeap("+0"));
		srs(dest->divr, StrHeap("+1"));
	}
	else if (*dest->divr == '-')
	{
		*dest->divr = '+';
		*dest->coff = (*dest->coff == '-') ? '+' : '-';
	}
	return dest;
}

coe* CoeDivrAlign(coe* o1, coe* o2)
{
	if (!o1 && o1->divr[1]=='0') return 0;
	if (!o2)// Reduction
	{
		if (o1->coff[1] == '0')
		{
			if (o1->divr[1] != '0')
				srs(o1->divr, StrHeap("+1"));
			return o1;
		}
		char* GCD = ChrComDiv(o1->coff, o1->divr);
		char* GCDCpy = StrHeap(GCD);
		if (StrCompare(GCD + 1, "1"))
		{
			ChrDiv(o1->coff, GCD);
			ChrDiv(o1->divr, GCDCpy);
		}
		memfree(GCD);
		memfree(GCDCpy);
		// then we check if there are +2 and +5 element
		while (StrCompare((GCD = ChrComDiv(o1->divr, "+2")) + 1, "1"))
		{
			char* tmpptr;
			ChrDiv(o1->divr, tmpptr = StrHeap("+2"));
			memfree(tmpptr);
			srs(o1->expo, ChrSub(o1->expo, "+1"));

			tmpptr = StrHeapAppend(o1->coff, "0");
			memfree(o1->coff);
			o1->coff = tmpptr;

			ChrDiv(o1->coff, tmpptr = StrHeap("+2"));
			memfree(GCD); memfree(tmpptr);
		}
		memfree(GCD);
		while (StrCompare((GCD = ChrComDiv(o1->divr, "+5")) + 1, "1"))
		{
			char* tmpptr;
			ChrDiv(o1->divr, tmpptr = StrHeap("+5"));
			memfree(tmpptr);
			srs(o1->expo, ChrSub(o1->expo, "+1"));

			tmpptr = StrHeapAppend(o1->coff, "0");
			memfree(o1->coff);
			o1->coff = tmpptr;

			ChrDiv(o1->coff, tmpptr = StrHeap("+5"));
			memfree(GCD); memfree(tmpptr);
		}
		memfree(GCD);
		return o1;
	}
	// Align expo
	if (o2->divr[1] == '0') return 0;
	CoeExpoAlign(o1, o2);
	char* GCM = ChrComMul(o1->divr, o2->divr);
	char* GCMCpy = StrHeap(GCM);
	char* ori1 = StrHeap(o1->divr),
		* ori2 = StrHeap(o2->divr);
	srs(o1->divr, StrHeap(GCM));
	srs(o2->divr, StrHeap(GCMCpy));
	ChrDiv(GCM, ori1); srs(o1->coff, ChrMul(o1->coff, GCM));
	ChrDiv(GCMCpy, ori2); srs(o2->coff, ChrMul(o2->coff, GCMCpy));
	memfree(GCM); memfree(GCMCpy);
	memfree(ori1); memfree(ori2);
	return o1;
}

// Is bound to lost precise! @RFW25
// DIVR HAS NOT A REAL SIGN!!!
coe* CoeDivrUnit(coe* obj, size_t kept_precise)
{
	if (obj->divr[1] == '0') return obj;
	if (!StrCompare(obj->divr, "+1"))
		return obj;
	size_t sorlen = 0;
	while (obj->divr[sorlen])sorlen++;
	if (!kept_precise || kept_precise + sorlen + 2 > malc_limit)
		return 0;
	char* ptor = salc(kept_precise + 2);
	*ptor = '+'; MemSet(ptor + 1, '9', kept_precise);// test how long for result.

	char* refer = ChrMul(ptor, obj->divr);
	size_t refer_len = 0;// excluding sign
	for (; refer[refer_len + 1]; refer_len++);
	memfree(refer); memfree(ptor);

	size_t coff_len = 0;// excluding sign
	for (; obj->coff[coff_len + 1]; coff_len++);

	char* quo, * tmppp;
	if (coff_len < refer_len)// compensate 0
	{
		memalloc(tmppp, refer_len + 2);
		quo = StrCopy(tmppp, obj->coff);// trans pointer
		MemSet(quo + 1 + coff_len, '0', refer_len - coff_len);
		quo[refer_len + 1] = 0;
		// borrow === refer_len - coff_len
	}
	else 
	{
		memalloc(tmppp, coff_len + 2);
		quo = StrCopy(tmppp, obj->coff);
	}
	char* rem = malc(sorlen + 2);
	StrCopy(rem, obj->divr);
	ChrDiv(quo, rem);
	if (atoins(rem) * 2 > atoins(obj->divr))
		srs(quo, ChrAdd(quo, "+1"));// bound
	size_t quo_len = 0;// excluding sign
	for (; quo[quo_len + 1]; quo_len++);
	if (quo_len < kept_precise)
		erro("Precise Exception yo CoeDivrUnit()");
	quo[kept_precise + 1] = 0;
	if (quo[kept_precise + 1] >= '5')// round
		srs(quo, ChrAdd(quo, (*quo == '+') ? "+1" : "-1"));
	srs(obj->coff, quo);
	srs(obj->divr, StrHeap("+1"));
	//expo
	char* tmp_expo_inter;
	if (coff_len < refer_len)
	{
		srs(obj->expo, ChrSub(obj->expo, tmp_expo_inter = instoa(((refer_len - coff_len) - (quo_len - kept_precise)))));
	}
	else srs(obj->expo, ChrAdd(obj->expo, tmp_expo_inter = instoa(quo_len - kept_precise)));
	memfree(tmp_expo_inter);
	memfree(rem);
	return obj;
}

int CoeExpoAlign(coe* o1, coe* o2)
{
	if (o1->divr[1] == '0' || o2->divr[1] == '0') return 0;
	int state = ChrCmp(o1->expo, o2->expo);
	if (state == 0) return 1;
	char* limit = instoa(malc_limit);
	if (state < 0) xchg(*(size_t*)&o1, *(size_t*)&o2);
	// now expo: o1 > o2
	char* expdif = ChrSub(o1->expo, o2->expo);
	if (ChrCmp(expdif, limit) >= 0 || *expdif != '+')
	{
		memfree(limit); memfree(expdif);
		// erro("CoeExpoAlign-OverLimit or -SystematicError.");
		return 0;
	}
	size_t expdif_num = atoins(expdif);
	srs(o1->expo, StrHeap(o2->expo));
	srs(o1->coff, StrHeapAppendChars(o1->coff, '0', expdif_num));
	memfree(limit); memfree(expdif);
	return 1;
}

coe* CoeNew(const char* coff, const char* expo, const char* divr)
{
	coe* elm; memalloc(elm, sizeof(coe));
	elm->coff = StrHeap(coff);
	elm->expo = StrHeap(expo);
	elm->divr = StrHeap(divr);
	return elm;
}

void CoeDel(coe* elm)
{
	memfree(elm->coff);
	memfree(elm->expo);
	memfree(elm->divr);
	memfree(elm);
}

coe* CoeCpy(const coe* obj)
{
	coe* ret = malcof(coe);
	ret->coff = StrHeap(obj->coff);
	ret->expo = StrHeap(obj->expo);
	ret->divr = StrHeap(obj->divr);
	ret->symb = obj->symb;
	return ret;
}

coe* CoeAdd(coe* dest, const coe* sors)
{
	if (dest->divr[1] == '0' || sors->divr[1] == '0')
	{
		if (dest->divr[1] != '0')srs(dest->coff, StrHeap(sors->coff));
		return dest;
	}
	sors = CoeCpy(sors);
	CoeDivrAlign(dest, (coe*)sors);
	srs(dest->coff, ChrAdd(dest->coff, sors->coff));
	CoeDivrAlign(dest, 0);// prior to Ctz
	CoeCtz(dest);
	CoeDel((coe*)sors);
	return dest;
}

coe* CoeSub(coe* dest, const coe* sors)
{
	if (dest->divr[1] == '0' || sors->divr[1] == '0')
	{
		if (dest->divr[1] != '0')srs(dest->coff, StrHeap(sors->coff));
		return dest;
	}
	sors = CoeCpy(sors);
	CoeDivrAlign(dest, (coe*)sors);
	srs(dest->coff, ChrSub(dest->coff, sors->coff));
	CoeDivrAlign(dest, 0);// prior to Ctz
	CoeCtz(dest);
	CoeDel((coe*)sors);
	return dest;
}

int CoeCmp(const coe* o1, const coe* o2)
{
	if (o1->divr[1] == '0' || o2->divr[1] == '0')
	{
		return 27;// ((int)'PHI'+'NA' + 'REN');// !!!
	}
	coe* dest = CoeCpy(o1),
		* sors = CoeCpy(o2);
	int state;
	CoeDivrAlign(dest, sors);
	state = ChrCmp(dest->coff, sors->coff);
	CoeDel(dest); CoeDel(sors);
	return state;
}

coe* CoeMul(coe* dest, const coe* sors)
{
	if (dest->divr[1] == '0' || sors->divr[1] == '0')
	{
		if (dest->divr[1] != '0')srs(dest->coff, StrHeap(sors->coff));
		return dest;
	}
	if (dest->coff[1] == '0' || sors->coff[1] == '0')// faster
	{
		srs(dest->coff, StrHeap("+0"));
		srs(dest->expo, StrHeap("+0"));
		srs(dest->divr, StrHeap("+1"));
		return dest;
	}
	srs(dest->coff, ChrMul(dest->coff, sors->coff));
	srs(dest->divr, ChrMul(dest->divr, sors->divr));
	srs(dest->expo, ChrAdd(dest->expo, sors->expo));
	CoeDivrAlign(dest, 0);// prior to Ctz
	CoeCtz(dest);
	return dest;
}

coe* CoeHypot(coe* dest, const coe* sors)
{
	if (dest->divr[1] == '0' || sors->divr[1] == '0')
	{
		if (dest->divr[1] != '0')srs(dest->coff, StrHeap(sors->coff));
		return dest;
	}
	coe* tmpd = CoeCpy(dest);
	coe* tmps = CoeCpy(sors);
	CoeMul(dest, tmpd);
	CoeMul(tmps, sors);
	CoeAdd(dest, tmps);
	// too many check will make this slow
	CoePow(dest, &coehalf);
	CoeDel(tmpd); CoeDel(tmps);
	return dest;
}

coe* CoeDiv(coe* dest, const coe* sors)
{
	if (dest->divr[1] == '0' || sors->divr[1] == '0')
	{
		if (dest->divr[1] != '0')srs(dest->coff, StrHeap(sors->coff));
		return dest;
	}
	if (sors->coff[1] == '0')
	{
		dest->divr[1] = '0';
		if (sors->coff[0] == '-')dest->coff[0] = (dest->coff[0] == '+') ? '-' : '+';//{Future} ChrSignTog
		return dest;
	}
	if (dest->coff[1] == '0') return dest;
	srs(dest->coff, ChrMul(dest->coff, sors->divr));
	srs(dest->divr, ChrMul(dest->divr, sors->coff));
	srs(dest->expo, ChrSub(dest->expo, sors->expo));
	CoeDivrAlign(dest, 0);// prior to Ctz
	CoeCtz(dest);
	return dest;
}

coe* CoeInt(coe* dest)
{
	if (dest->divr[1] == '0') return dest;
	CoeDivrUnit(dest, show_precise);
	if (dest->expo[1] == '0') return dest;
	ptrdiff_t expdif_num = atoins(dest->expo);// {Potential}
	if (expdif_num < 0)
	{
		size_t coflen = StrLength(dest->coff);
		if ((ptrdiff_t)(coflen - 1) > -expdif_num)// E.g. -12 LEN=3 
		{
			dest->coff[coflen + expdif_num] = 0;
		}
		else
			srs(dest->coff, StrHeap("+0"));
	}
	else
	{
		srs(dest->coff, StrHeapAppendChars(dest->coff, '0', expdif_num));
	}
	srs(dest->expo, StrHeap("+0"));
	return dest;
}


static const unsigned char TaylorDptr[] =
{
	0b10000000,// 0 exp
	0b10010101,// 1 sin
	0b10100001,// 2 cos yo (0,2]
	0b01000101,// 3 ln(x+1)
	0b01010101 // 4 arctan [-1,+1]
};
// Taylors is writen by Haruno RFR.
//MSB[Divr 00:1 01:n 10:n! 11:n*n]
//   [00:All 01:OnlyOdd 10: OnlyEven]
//   [StartPower 0~3]
//   [StartSign 0:+ 1:-]
//LSB[SignFlap 0:N 1:Y]
static coe* CoeTaylor(coe* dest, unsigned char dptor, const coe* period, size_t digcut)
{
	if (dest->divr[1] == '0') return dest;
	char* tmp;
	// [0~period{>0}). id period is not NULL
	// recommand digcut 4
	if (show_precise >= malc_limit)
	{
		erro("SHOW >= MALC.");
		return dest;
	}
	if (digcut > malc_count) digcut = 0;
	coe* crt = CoeCpy(dest);
	CoeDivrUnit(crt, digcut);// then we can omit .divr
	if (period)
	{
		while (CoeCmp(crt, &coezero) < 0)
			CoeAdd(crt, period);
		while (CoeCmp(crt, period) >= 0)
			CoeSub(crt, period);
	}
	coe* CrtPow = CoeNew(tmp = instoa((0b00001100 & dptor) >> 2), "+0", "+1");
	memfree(tmp);
	boolean sign = (crt->coff[0] == '-');
	boolean signcrt = ((0b00000010 & dptor) >> 1);// ^ sign
	boolean signshock = (0b00000001 & dptor);
	enum { all_ = 0, only_odd, only_even, none_ } powtype = (0b00110000 & dptor) >> 4;
	if (powtype == none_) goto endo;
	enum { one_, n_, nf_, nn_ } divtype = (0b11000000 & dptor) >> 6;
	if (divtype == nn_) goto endo;// do not support NOW.
	coe* result;
	coe* plus;
	coe* diver = divtype == one_ ? CoeCpy(&coeone) :
		divtype == n_ ? CoeCpy(CrtPow) :
		divtype == nf_ ? CoeFac(CoeCpy(CrtPow)) : 0;

	// Endo Condition: "5+9<5", no rounding increase, when expodif==8;
	//  or expodif > 8
	//  (assume show_precise=8)
	//    123456789 <res>  123456789 +0
	//    000000005 <plus> 5 -8
	// first loop
	{
		result = CoeCpy(&coezero);
	}
	// += sgn* powx/divcof
	// more loop
	boolean conti = 1;
	size_t i = 0;
	lup_last = 0;
	do
	{
		lup_last++;
		plus = CoeCpy(crt);
		CoePow(plus, CrtPow);
		CoeDiv(plus, diver);
		// conti = () . NOW do not care, because we need consider the change rate of a and b of a/b
		conti = 0;
		if (plus->coff[1] != '0')
		{
			char* ori = StrHeapN(result->coff, show_precise + 1);
			// coe* test_precise = CoeCpy(result);
			// CoeDiv(test_precise, plus);
			if (signcrt) plus->coff[0] = (plus->coff[0] == '-' ? '+' : '-');
			CoeAdd(result, plus);
			CoeDivrUnit(result, show_precise + 1);
			conti = ((++i < lup_times) || StrCompareN(result->coff, ori, show_precise + 1)) && (i < lup_limit);
			// [Old condi] ChrCmp(test_precise->expo, "+0") <= 0 && ...
			// [Old condi] (ptrdiff_t)StrLength(result->coff) - atoins(result->expo) <= show_precise) ...
			// CoeDel(test_precise);
			if (signshock) signcrt = !signcrt;
			CoeAdd(CrtPow, powtype == all_ ? &coeone : &coetwo);
			CoeDel(diver);
			diver = divtype == one_ ? CoeCpy(&coeone) :
				divtype == n_ ? CoeCpy(CrtPow) :
				divtype == nf_ ? CoeFac(CoeCpy(CrtPow)) : 0;
			memfree(ori);
		}
		CoeDel(plus);
	} while (conti);
	if (StrLength(result->coff) > show_precise + 1) CoeDig(result, show_precise, 2);// nearest
	CoeDel(diver);
endo:
	CoeDel(CrtPow);
	CoeDel(crt);
	srs(dest->coff, StrHeap(result->coff));
	srs(dest->expo, StrHeap(result->expo));
	srs(dest->divr, StrHeap(result->divr));
	CoeDel(result);
	return dest;
}

coe* CoePow(coe* dest, const coe* sors)
{
	if (dest->divr[1] == '0' || sors->divr[1] == '0')
	{
		if (dest->divr[1] != '0')srs(dest->coff, StrHeap(sors->coff));
		return dest;
	}
	if (*sors->coff == '+' && *sors->expo == '+' && !StrCompare(sors->divr, "+1"))
	{
		if (sors->coff[1] == '0')
		{
			srs(dest->coff, StrHeap("+1"));
			srs(dest->expo, StrHeap("+0"));
			srs(dest->divr, StrHeap("+1"));
			return dest;
		}
		coe* CrtExp = CoeCpy(sors);
		coe* ret = CoeCpy(dest);// attention! the char* will be free, so use this but "coe ret;" in stack
		CoeSub(CrtExp, &coeone);
		while (CoeCmp(CrtExp, &coezero) > 0)
		{
			CoeMul(ret, dest);
			CoeSub(CrtExp, &coeone);
			// void CoePrint(coe * co); CoePrint(ret); CoePrint(dest);
			// srs(CrtExp, ChrSub(CrtExp, "+1"));
		}
		CoeDel(CrtExp);
		srs(dest->coff, StrHeap(ret->coff));
		srs(dest->expo, StrHeap(ret->expo));
		srs(dest->divr, StrHeap(ret->divr));
		CoeDel(ret);
	}
	else
	{
		// Pow(a,b) == Exp( mul( log(a) , b) )
		int t_sign = (*dest->coff == '-');
		ptrdiff_t t_expo = atoins(dest->expo);
		if (!(t_expo & 1)) t_sign = 0;
		dest = CoeLog(dest);
		CoeMul(dest, sors);
		CoeExp(dest);
		*dest->coff = t_sign ? '-' : '+';
	}
	return dest;
}

coe* CoeSin(coe* dest)
{
	return CoeTaylor(dest, TaylorDptr[1], &coe2pi, _DIG_CUT);
}

coe* CoeCos(coe* dest)
{
	return CoeTaylor(dest, TaylorDptr[2], &coe2pi, _DIG_CUT);
}

// NOW use indirectly method
coe* CoeTan(coe* dest)
{
	if (dest->divr[1] == '0') return dest;
	coe* val_sin, * val_cos;
	val_sin = CoeCpy(dest);
	val_cos = CoeCpy(dest);
	val_sin = CoeSin(val_sin);
	val_cos = CoeCos(val_cos);
	if (CoeCmp(val_cos, &coezero) == 0)
	{
		erro("Tan ?/0");
		CoeDel(val_sin);
		CoeDel(val_cos);
		return dest;
	}
	val_sin = CoeDiv(val_sin, val_cos);
	srs(dest->coff, StrHeap(val_sin->coff));
	srs(dest->expo, StrHeap(val_sin->expo));
	srs(dest->divr, StrHeap(val_sin->divr));
	CoeDel(val_sin);
	CoeDel(val_cos);
	return dest;
}


// Below: not recommend to be used. It is better to use the math.h for calculation as the replacement now. --RFR12
//{
//	CoeDivrUnit(dest, show_precise + 1);
//	double r = asin();
//}
// Not recommend
coe* CoeAsin(coe* dest)
{
	if (dest->divr[1] == '0') return dest;
	// This is based on CoeTaylor()
	// def chk [-1,1]
	if (CoeCmp(dest, &coeone) > 0 || CoeCmp(dest, &coenegone) < 0)
	{
		erro("ASin Over definition");
		return dest;
	}
	if (show_precise >= malc_limit)
	{
		erro("SHOW >= MALC.");
		return dest;
	}
	if(_DIG_CUT) CoeDivrUnit(dest, _DIG_CUT);// then we can omit .divr
	coe* result;
	coe* plus;// = muler_fac2n/diver_powfacn/diver_powfour*(muler_main)/CrtPow
	coe* muler_fac2n = CoeCpy(&coeone);
	coe* diver_powfacn = CoeCpy(&coeone);
	coe* diver_powfour = CoeCpy(&coeone);
	coe* muler_main = CoeCpy(dest);
	coe* diver_main = CoeCpy(&coeone);

	coe* crtpow = CoeCpy(&coezero);
	coe* crtpow2 = CoeCpy(&coezero);

	result = CoeCpy(&coezero);
	boolean conti = 1;
	size_t i = 0;
	lup_last = 0;
	do
	{
		lup_last++;
		plus = CoeCpy(muler_main);
		CoeMul(plus, muler_fac2n);
		CoeDiv(plus, diver_powfour);
		CoeDiv(plus, diver_powfacn);
		CoeDiv(plus, diver_main);
		if (plus->coff[1] != '0')
		{
			char* ori = StrHeapN(result->coff, show_precise + 1);
			CoeAdd(result, plus);
			CoeDivrUnit(result, show_precise + 1);
			conti = ((++i < lup_times) || StrCompareN(result->coff, ori, show_precise + 1)) && (i < lup_limit);
			//
			CoeAdd(crtpow, &coeone); CoeAdd(diver_main, &coetwo);
			CoeMul(diver_powfacn, crtpow);
			CoeMul(diver_powfacn, crtpow);
			CoeMul(diver_powfour, &coefour);
			CoeAdd(crtpow2, &coeone);
			CoeMul(muler_fac2n, crtpow2);
			CoeAdd(crtpow2, &coeone);
			CoeMul(muler_fac2n, crtpow2);
			CoeMul(muler_main, dest);
			CoeMul(muler_main, dest);
			memfree(ori);
		}
		CoeDel(plus);
	} while (conti);
	if (StrLength(result->coff) > show_precise + 1) CoeDig(result, show_precise, 2);// nearest
	CoeDel(crtpow2);
	CoeDel(crtpow);
	CoeDel(diver_main);
	CoeDel(muler_main);
	CoeDel(diver_powfour);
	CoeDel(diver_powfacn);
	CoeDel(muler_fac2n);
	srs(dest->coff, StrHeap(result->coff));
	srs(dest->expo, StrHeap(result->expo));
	srs(dest->divr, StrHeap(result->divr));
	CoeDel(result);
	return dest;
}

// Not recommend
coe* CoeAcos(coe* dest)
{
	if (dest->divr[1] == '0') return dest;
	// pi/2 - CoeAsin this
	coe* _coepi_half = CoeCpy(&coepi_half);
	dest = CoeAsin(dest);
	CoeDivrUnit(dest, show_precise + 1);
	CoeDivrUnit(_coepi_half, show_precise + 1);
	_coepi_half = CoeSub(_coepi_half, dest);
	srs(dest->coff, StrHeap(_coepi_half->coff));
	srs(dest->expo, StrHeap(_coepi_half->expo));
	srs(dest->divr, StrHeap(_coepi_half->divr));
	CoeDel(_coepi_half);
	return dest;
}

// Not recommend
coe* CoeAtan(coe* dest)
{
	if (dest->divr[1] == '0') return dest;
	// Haruno RFR11
	// Considering the Taylor is true for [-1,1].
	// E.g. atan(3.14 = 1+1+1+0.14), 1 for recursion and 0.14 for Taylor.
	// let a =atan A, b=atan B
	// tan(a+b)=(tan a+tan b)/(1-tan a*tan b)
	// tan(a-b)=(tan a-tan b)/(1+tan a*tan b)
	// atan(A)-atan(B)=atan(A-B/(1+A*B))
	// so atan(1+0.14)=atan 0.14+atan(/(1+1.14*0.14))=pi/4 + atan(0.14/2.14) (B=0.14 and B=1) -- [checked]
	// [checked] pi/4 == atan(1.14/0.86)-atan 0.14 == atan 0.14+atan(0.86/1.14)
	int state = 0;
	if (dest->coff[0] == '-')
	{
		dest->coff[0] = '+';
		dest = CoeAtan(dest);
		*dest->coff = (*dest->coff == '-' ? '+' : '-');
	}
	else if ((state = CoeCmp(dest, &coeone)) == 0)
	{
		// *dest = coepi_quarter;
		srs(dest->coff, StrHeap(coepi_quarter.coff));
		srs(dest->expo, StrHeap(coepi_quarter.expo));
		srs(dest->divr, StrHeap(coepi_quarter.divr));
		CoeDig(dest, show_precise, 2);
	}
	else if (state == 1)
	{
		// atan(A)=atan( div(A-1,A+1) )+pi/4
		coe* a = CoeCpy(dest); CoeSub(a, &coeone);
		coe* b = CoeCpy(dest); CoeAdd(b, &coeone);
		CoeDiv(a, b);
		CoeAtan(a);
		CoeAdd(a, &coepi_quarter);
		srs(dest->coff, StrHeap(a->coff));
		srs(dest->expo, StrHeap(a->expo));
		srs(dest->divr, StrHeap(a->divr));
		CoeDel(a); CoeDel(b);
		// atan(A)=atan(1/(1+A*(A-1)))+atan(A-1)
	}
	else
	{
		// CoeDivrUnit(dest, show_precise + 4);
		//if (CoeCmp(dest, &coehalf) == 1)
		//{
		//	// tan(a) = 2*tan(a/2)/(1-tan(a/2)*tan(a/2))
		//	// atan atan tan a = ...
		//	CoeDiv(dest, &coetwo);
		//	CoeAtan(dest);
		//	coe* a = CoeCpy(dest); CoeMul(a, &coetwo);
		//	coe* b = CoeCpy(dest); CoeMul(b, dest);
		//	*b->coff = *b->coff == '+' ? '-' : '+';
		//	CoeAdd(b, &coeone);
		//	CoeDiv(a, b);
		//	CoeAtan(a);
		//	CoeAtan(a);
		//	srs(dest->coff, StrHeap(a->coff));
		//	srs(dest->expo, StrHeap(a->expo));
		//	srs(dest->divr, StrHeap(a->divr));
		//	CoeDel(a); CoeDel(b);
		//}
		//else
		//CoeDivrUnit(dest, show_precise + 4);
		//CoeDig(dest, show_precise + 4, 2);
		CoeTaylor(dest, TaylorDptr[4], 0, _DIG_CUT);
	}
	return dest;
}

// Not recommend
coe* CoeLog(coe* dest)// ln, log`e()
{
	if (dest->divr[1] == '0') return dest;
	// ln(pow(2,n)*x)=n*ln 2+ln x
	lup_last = 0;
	size_t subtimes = 0;
	if (CoeCmp(dest, &coezero) <= 0)
	{
		erro("Log`e opt <= 0 overt definition.");
		return dest;
	}
	while (CoeCmp(dest, &coetwo) > 0)
	{
		subtimes++;
		CoeDiv(dest, &coetwo);
		// CoeDivrUnit(dest, show_precise + 1);
	}
	if (CoeCmp(dest, &coetwo) == 0)
	{
		srs(dest->coff, StrHeap(coeln2.coff));
		srs(dest->expo, StrHeap(coeln2.expo));
		srs(dest->divr, StrHeap(coeln2.divr));
	}
	else
	{
		dest = CoeSub(dest, &coeone);
		CoeTaylor(dest, TaylorDptr[3], 0, _DIG_CUT);
	}
	while (subtimes--)
	{
		CoeAdd(dest, &coeln2);
	}
	return dest;
}

int CoeSgn(const coe* dest)
{
	return CoeCmp(dest, &coezero);
}

coe* CoeExp(coe* dest)// e
{
	CoeTaylor(dest, TaylorDptr[0], 0, _DIG_CUT);
	return dest;
}

coe* CoeFac(coe* dest)
{
	if (dest->divr[1] == '0') return dest;
	if (StrCompare(dest->divr, "+1") || dest->expo[0] == '-')
	{
		erro("CoeFac");
		return dest;// do nothing if error
	}
	coe* AnotherOne = CoeCpy(&coeone);
	if (StrCompare(dest->expo, "+0"))
		CoeExpoAlign(dest, AnotherOne);// make dest->expo be "+0"
	srs(dest->coff, ChrFactorial(dest->coff));
	CoeDel(AnotherOne);
	return dest;
}

coe* CoePi()
{
	return CoeCpy(&coepi);

	// take use of ARCTAN
	coe* conum = CoeCpy(&coeone);
	CoeAtan(conum);
	// CoeTaylor(conum, TaylorDptr[4], 0, _DIG_CUT);
	CoeMul(conum, &coetwo);
	CoeMul(conum, &coetwo);
	// CoeDig(conum, show_precise, 2);
	return conum;

}

coe* CoeE()
{
	coe* co = CoeCpy(&coeone);
	return CoeExp(co);
}

coe* CoeSinh(coe* dest)
{
	if (dest->divr[1] == '0') return dest;
	//=average(exp(x)-exp(neg(x)));
	coe* c_negexp = CoeCpy(dest);
	*c_negexp->coff = *c_negexp->coff == '+' ? '-' : '+';
	CoeExp(dest); CoeExp(c_negexp);
	CoeSub(dest, c_negexp);
	CoeDiv(dest, &coetwo);
	CoeDel(c_negexp);
	return dest;
}

coe* CoeCosh(coe* dest)
{
	if (dest->divr[1] == '0') return dest;
	//=average(exp(x)+exp(neg(x)));
	coe* c_negexp = CoeCpy(dest);
	*c_negexp->coff = *c_negexp->coff == '+' ? '-' : '+';
	CoeExp(dest); CoeExp(c_negexp);
	CoeAdd(dest, c_negexp);
	CoeDiv(dest, &coetwo);
	CoeDel(c_negexp);
	return dest;
}

coe* CoeTanh(coe* dest)
{
	if (dest->divr[1] == '0') return dest;
	coe* diver = CoeCpy(dest);
	CoeCosh(diver);
	CoeSinh(dest);
	CoeDiv(dest, diver);
	CoeDel(diver);
	return dest;
}

coe* CoeAsinh(coe* dest)
{
	if (dest->divr[1] == '0') return dest;
	double d = CoeToDouble(dest);
	coe* res = CoeFromDouble(asinh(d));
	srs(dest->coff, StrHeap(res->coff));
	srs(dest->expo, StrHeap(res->expo));
	srs(dest->divr, StrHeap(res->divr));
	CoeDel(res);
	return dest;
}

coe* CoeAcosh(coe* dest)
{
	if (dest->divr[1] == '0') return dest;
	double d = CoeToDouble(dest);
	// A domain error occurs if x < 1.
	// [1, +)->[0, +oo]
	if (d < 1.0)
	{
		erro("Acosh dom err");
		return dest;
	}
	coe* res = CoeFromDouble(acosh(d));
	srs(dest->coff, StrHeap(res->coff));
	srs(dest->expo, StrHeap(res->expo));
	srs(dest->divr, StrHeap(res->divr));
	CoeDel(res);
	return dest;
}

coe* CoeAtanh(coe* dest)
{
	if (dest->divr[1] == '0') return dest;
	double d = CoeToDouble(dest);
	if (fabs(d) > 1.0 || acosh(d) == 0.0)
	{
		erro("ATANH DEF ERR");
		return dest;
	}
	coe* res = CoeFromDouble(atanh(d));
	srs(dest->coff, StrHeap(res->coff));
	srs(dest->expo, StrHeap(res->expo));
	srs(dest->divr, StrHeap(res->divr));
	CoeDel(res);
	return dest;
}

// ---- ---- ---- ---- Conversion ---- ---- ---- ----

// LDouble is cancelled for "powl" and others cannot be linked by GCC for AR-Library.
// Float does not have the problem, but also cancelled.
// -- Arina RFA

char* CoeToLocale(const coe* obj, int opt)
{
	// 0.0001 : "1" and -4, real_exp + len === 5
	// 100000 : "1" and 5, real_exp + len === 6
	if (obj->divr[1] == '0')
	{
		return StrHeap(obj->coff[1] == '0' ? "NaN" : obj->coff[0] == '+' ? "+Inf" : "-Inf");
	}
	coe* objj = CoeCpy(obj);
	CoeDivrUnit(objj, show_precise);// RFR16 Appended.
	ptrdiff_t real_exp_sgn = atoins(objj->expo);// limit {TODO}
	size_t real_exp = real_exp_sgn < 0 ? -real_exp_sgn : real_exp_sgn;
	size_t len = StrLength(objj->coff), numlen = len - 1;
	//
	if (!opt) opt = (real_exp + len + (real_exp_sgn < 0) >
		_CDE_PRECISE_SHOW_LOCALE_EXPONENT) ? 2 : 1;
	if (opt == 1)
	{
		if (*objj->expo == '-') len++;
		//+2e-5 0.00002 (len 7)
		len += real_exp;// I am too lazy -- Arina RFW25
		char* const tmpptr = malc(len + 1);
		{size_t ecx = len + 1; while (ecx--) tmpptr[ecx] = 0;}
		// I am too lazy to think the detail
		if (*objj->expo == '+')
		{
			char* subp = tmpptr;
			for (size_t i = 0; i < numlen + 1; i++)
				*subp++ = objj->coff[i];
			for (size_t i = 0; i < real_exp; i++)
				*subp++ = '0';
		}
		else if (real_exp >= numlen)//0.xxx
		{
			char* subp = tmpptr;
			*subp++ = *objj->coff;// sign
			*subp++ = '0';
			*subp++ = '.';
			for (size_t i = 0; i < real_exp - numlen;i++)
				*subp++ = '0';
			StrCopy(subp, objj->coff + 1);
		}
		else if (numlen == 1 || real_exp == 0)
		{
			StrCopy(tmpptr, objj->coff);
		}
		else
		{
			char* subp = tmpptr;
			*subp++ = *objj->coff;// sign
			for (size_t i = 1;i <= numlen - real_exp;i++)
				*subp++ = objj->coff[i];
			*subp++ = '.';
			StrCopy(subp, objj->coff + numlen - real_exp + 1);
		}
		CoeDel(objj);// RFR16
		return tmpptr;
	}
	else if (opt == 2)// +16 +23 +1.6e+23  +2 +4 +2e+4
	{
		char* tmp_expo = StrHeap(objj->expo);
		char* tmp_expo_dif = instoa(numlen - 1);
		srs(tmp_expo, ChrAdd(tmp_expo, tmp_expo_dif));
		len += StrLength(tmp_expo);
		real_exp = atoins(tmp_expo);

		char* tmpptr = malc(len + 3);
		{size_t ecx = len + 1; while (ecx--) tmpptr[ecx] = 0;}
		// I am too lazy to think the detail
		char* subp = tmpptr;
		*subp++ = *objj->coff;// sign
		*subp++ = objj->coff[1];
		if (numlen > 1)
		{
			*subp++ = '.';
			for (size_t i = 2; i < numlen + 1; i++)
				*subp++ = objj->coff[i];
		}
		if (real_exp)
		{
			*subp++ = 'e';
			StrCopy(subp, tmp_expo);
		}

		memfree(tmp_expo_dif);
		memfree(tmp_expo);
		CoeDel(objj);// RFR16
		return tmpptr;
	}
	CoeDel(objj);// RFR16
	return 0;
}

coe* CoeFromLocale(const char* str)
{
	const char* ParA = str, * ParB = 0, * ParC = 0;
	size_t LenA = 0, LenB = 0, LenC = 0;
	char c;
	coe* co = 0;
	size_t coflen = 0;
loop: switch (c = *str++)
	{
	case 0:
		// Chk and gen co.
		memalloc(co, sizeof(coe));
		co->symb = 0;
		co->divr = StrHeap("+1");
		if (ParC) LenC = str - ParC - 1;// e2$$
		else if (ParB) LenB = str - ParB - 1;
		else LenA = str - ParA;

		if (ParC && LenC && ParC[LenC - 1] != '+' && ParC[LenC - 1] != '-')
		{
			co->expo = StrHeapN(ParC, LenC);
			if (*co->expo != '+' && *co->expo != '-')
				srs(co->expo, StrHeapAppend(("+"), co->expo));
		}
		else co->expo = StrHeap("+0");
		if (!LenA && !LenB)
		{
			co->coff = StrHeap("+1");
			goto loopend;
		}

		if (!ParB || ParA == ParB || !LenB)// only interger
		{
			co->coff = StrHeapN(ParA, LenA);
			if (*co->coff != '+' && *co->coff != '-')
				srs(co->coff, StrHeapAppend(("+"), co->coff));
			if (!co->coff[1])
				srs(co->coff, StrHeapAppend(co->coff, "1"));
			goto loopend;
		}
		else
		{
			memalloc(co->coff, LenA + LenB + 1);
			if (LenA)
				StrCopyN(co->coff, ParA, LenA);// 0.x case
			StrCopyN(co->coff + LenA, ParB, LenB);
			co->coff[LenA + LenB] = 0;
			char* tmpptr = instoa(LenB);
			srs(co->expo, ChrSub(co->expo, tmpptr));
			memfree(tmpptr);
			if (*co->coff != '+' && *co->coff != '-')
				srs(co->coff, StrHeapAppend(("+"), co->coff));
			if (!co->coff[1])
				srs(co->coff, StrHeapAppend(co->coff, "1"));
			goto loopend;
		}
		break;
	default:
		if (c >= '0' && c <= '9');
		else goto loopend;
		break;
	case '+':
	case '-':
		break;
	case 'e':
		if (ParC) goto loopend;
		ParC = str;
		if (ParA && ParB)//1.2e3
		{
			LenB = str - ParB - 1;
			ParC = str;
		}
		else if (ParA && !ParB)//1e2
		{
			LenA = str - ParA - 1;
			ParB = ParA;
			ParC = str;
		}
		else goto loopend;
		break;
	case '.':
		if (ParB || ParC) goto loopend;
		ParB = str;
		LenA = str - ParA - 1;//1.1
		break;
	} goto loop; 
	loopend:
	if (!co) return co;
	ChrCpz(co->coff);// +002$ 
	ChrCpz(co->expo);
	return co;
}

double CoeToDouble(const coe* dest)
{
	if (dest->divr[1] == '0')
	{
		return (dest->coff[1] == '0' ? NAN : INFINITY);
	}
	coe* ddd = CoeCpy(dest);
	CoeDivrUnit(ddd, show_precise);
	double ll = 0.0;
	ptrdiff_t CrtPow = atoins(ddd->expo);
	if (CrtPow > DBL_MAX_10_EXP) return INFINITY;
	for (ptrdiff_t i = StrLength(ddd->coff) - 1; i > 0; i--)
	{
		if (CrtPow >= DBL_MIN_10_EXP)
			ll += (ddd->coff[i] - '0') * pow(10.0, (double)CrtPow);
		CrtPow++;
		if (CrtPow > DBL_MAX_10_EXP) break;
	}
	if (*ddd->coff == '-')ll *= -1;
	CoeDel(ddd);
	return ll;
}

coe* CoeFromDouble(double flt)
{
	// based on above
	int sign = flt < 0;
	if (sign) flt = -flt;
	if (flt == 0.0) return CoeNew("+0", "+0", "+1");
	coe* res;
	if (isnan(flt)) 
		return CoeNew("+1", "+0", "+0");
	ptrdiff_t crtpow = (ptrdiff_t)(flt < 0. ? -log10(-flt) : log10(flt));
	size_t luptimes = 0;
	char* buf = salc(show_precise + 2);
	char* ptr = buf + 1;
	char* tmpptr;
	char c;
	while (crtpow > DBL_MIN_10_EXP && luptimes++ < show_precise)
	{
		double tmp0 = pow(10, (double)crtpow);
		ptrdiff_t tmp1 = (ptrdiff_t)(flt / tmp0);
		c = (tmp1 % 10) + 0x30;
		flt -= tmp1 * tmp0;
		*ptr++ = c;
		crtpow--;
		if (flt == 0.0) break;
		// RFV12 fixed.
	}
	if (ptr == buf || ptr == buf + 1)
	{
		ptr = buf; ptr++;
		*ptr++ = '0';
	}
	*ptr = 0;
	buf[0] = sign ? '-' : '+';
	res = CoeNew(buf, tmpptr = instoa(crtpow+1), "+1");
	memfree(tmpptr);
	memfree(buf);
	ChrCpz(res->coff);
	CoeCtz(res);
	ChrCpz(res->coff);
	return res;
}

