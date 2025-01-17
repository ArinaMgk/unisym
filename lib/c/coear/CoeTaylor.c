// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant Host[Allocation]
// LastCheck: RFZ23
// AllAuthor: @ArinaMgk
// ModuTitle: ASCII Powerful Number of Arinae
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

#include "../../../inc/c/coear.h"
#include "../../../inc/c/ustdbool.h"
#if !defined(_MCCA)

coe* CoeTaylor(coe* dest, unsigned char dptor, const coe* period, size_t digcut)
{
	if (dest->divr[1] == '0') return dest;
	if (dest->coff[1] == '0' && !(_CDETayDptr_PoStart(dptor)))// {TEMP} for exp() dpot(0)
	{
		return CoeRst(dest, StrHeap("+1"), StrHeap("+0"), StrHeap("+1"));// coeone
	}
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
	coe* CrtPow = CoeFromInteger(_CDETayDptr_PoStart(dptor));
	boolean sign = (crt->coff[0] == '-');
	boolean signcrt = _CDETayDptr_PowSign(dptor);// ^ sign
	boolean signshock = _CDETayDptr_SignTog(dptor);
	enum { all_ = 0, only_odd, only_even, none_ } powtype = _CDETayDptr_PowType(dptor);
	if (powtype == none_) goto endo;
	enum { one_, n_, nf_, nn_ } divtype = _CDETayDptr_DivType(dptor);
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
			if (StrLength(plus->coff) > malc_limit / 2)
			{
				CoeDig(plus, malc_limit / 2, 2);
			}
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
	if (i >= lup_limit) switch (dptor)
	{
	case _CDETayFunc_Exp:
		CoeDel(result);
		result = CoeFromDouble(exp(CoeToDouble(crt)));
		break;
	case _CDETayFunc_Log:
		CoeDel(result);
		result = CoeFromDouble(log(1 + CoeToDouble(crt)));
		break;
	case _CDETayFunc_Sin:
		CoeDel(result);
		result = CoeFromDouble(sin(CoeToDouble(crt)));
		break;
	case _CDETayFunc_Cos:
		CoeDel(result);
		result = CoeFromDouble(cos(CoeToDouble(crt)));
		break;
	case _CDETayFunc_Atn:
		CoeDel(result);
		result = CoeFromDouble(atan(CoeToDouble(crt)));
		break;
	default: break;
	}
	if (StrLength(result->coff) > show_precise + 1) CoeDig(result, show_precise, 2);// nearest
	CoeDel(diver);
endo:
	CoeDel(CrtPow);
	CoeDel(crt);
	CoeRst(dest, StrHeap(result->coff), StrHeap(result->expo), StrHeap(result->divr));
	CoeDel(result);
	CoeCtz(dest);
	return dest;
}
#endif
