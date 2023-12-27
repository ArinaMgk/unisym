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
