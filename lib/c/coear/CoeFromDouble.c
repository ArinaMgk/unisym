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

#undef isnan
#define isnan(x) ((x) != (x))

#if !defined(_MCCA)
#include <float.h>

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
#endif
