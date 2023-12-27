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
#include <float.h>
#include <math.h>

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
