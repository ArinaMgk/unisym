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
	CoeRst(dest, StrHeap(val_sin->coff), StrHeap(val_sin->expo), StrHeap(val_sin->divr));
	CoeDel(val_sin);
	CoeDel(val_cos);
	return dest;
}
