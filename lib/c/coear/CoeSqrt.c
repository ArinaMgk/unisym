/// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant Host[Allocation]
// LastCheck: RFC02
// AllAuthor: @dosconio
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

coe* CoeSqrt(coe* dest)
{
	int sign = 0;
	if (dest->divr[1] == '0') return dest;
	if (dest->coff[1] == '0') return dest;
	if (dest->coff[0] == '-')
	{
		sign = 1;
		dest->coff[0] = '+';
	}
	CoePow(dest, &coehalf);
	if (sign) dest->coff[0] = '-';
	return dest;
}
