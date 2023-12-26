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
