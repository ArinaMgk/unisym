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
#if !defined(_MCCA)
coe* CoeAcosh(coe* dest)
{
	if (dest->divr[1] == '0') return dest;
	double d = CoeToDouble(dest);
	// A domain error occurs if x < 1.
	// [1, +)->[0, +oo]
	if (d < 1.0)
	{
		erro("Acosh dom err");
		CoeRst(dest, StrHeap(coenan.coff), StrHeap(coenan.expo), StrHeap(coenan.divr));
		return dest;
	}
	coe* res = CoeFromDouble(acosh(d));
	CoeRst(dest, StrHeap(res->coff), StrHeap(res->expo), StrHeap(res->divr));
	CoeDel(res);
	return dest;
}
#endif
