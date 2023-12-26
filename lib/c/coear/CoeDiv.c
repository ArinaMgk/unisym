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
	CoeRst(dest, ChrMul(dest->coff, sors->divr), 
		ChrSub(dest->expo, sors->expo), 
		ChrMul(dest->divr, sors->coff));
	CoeDivrAlign(dest, 0);// prior to Ctz
	CoeCtz(dest);
	return dest;
}
