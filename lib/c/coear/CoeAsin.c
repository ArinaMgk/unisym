/// ASCII C99 TAB4 CRLF
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
		erro("ASin or ArcX Over definition");
		CoeRst(dest, StrHeap(coenan.coff), StrHeap(coenan.expo), StrHeap(coenan.divr));
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
	CoeRst(dest, StrHeap(result->coff), StrHeap(result->expo), StrHeap(result->divr));
	CoeDel(result);
	return dest;
}
