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

coe* CoePi()
{
	return CoeCpy(&coepi);

	// take use of ARCTAN
	coe* conum = CoeCpy(&coeone);
	CoeAtan(conum);// CoeTaylor(conum, TaylorDptr[4], 0, _DIG_CUT);
	CoeMul(conum, &coetwo);
	CoeMul(conum, &coetwo);
	CoeDig(conum, show_precise, 2);
	return conum;
}
