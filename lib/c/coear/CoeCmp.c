/// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant 
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

int CoeCmp(const coe* o1, const coe* o2)
{
	if (o1->divr[1] == '0' || o2->divr[1] == '0')
	{
		return 27;// ((int)'PHI'+'NA' + 'REN');// !!!
	}
	coe* dest = CoeCpy(o1),
		* sors = CoeCpy(o2);
	int state;
	CoeDivrAlign(dest, sors);
	state = ChrCmp(dest->coff, sors->coff);
	CoeDel(dest); CoeDel(sors);
	return state;
}
