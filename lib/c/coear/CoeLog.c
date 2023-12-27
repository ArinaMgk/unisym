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
		CoeRst(dest, StrHeap(coenan.coff), StrHeap(coenan.expo), StrHeap(coenan.divr));
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
		CoeRst(dest, StrHeap(coeln2.coff), StrHeap(coeln2.expo), StrHeap(coeln2.divr));
	}
	else
	{
		dest = CoeSub(dest, &coeone);
		CoeTaylor(dest, _CDETayFunc_Log, 0, _DIG_CUT);
	}
	while (subtimes--)
	{
		CoeAdd(dest, &coeln2);
	}
	return dest;
}
