// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant yo MIX bin^16+
// LastCheck: 2023 Dec 25
// AllAuthor: @dosconio
// ModuTitle: Date and Time
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

#include "../../../inc/c/datime.h"

void fromherp(stdint spans, word* year, word* month, word* day)
{
	*year = 2014;
	*month = 1;
	*day = 1;
	if (spans <= 3)
	{
		stdint tmpd = *day, tmpm = *month, tmpy = *year;
		// have the potential error of overflowing
		tmpd += spans - 4;
		while (tmpd < 1)
		{
			tmpm--;
			if (tmpm < 1)
			{
				tmpm = 12;
				tmpy--;
			}
			tmpd += moondays(tmpy, tmpm);
		}
		*day = tmpd;
		*month = tmpm;
		*year = tmpy;
	}
	else
	{
		spans -= 4;
		// 2014-01-01 is spans(0) now
		while (spans >= (isLeapYear(*year) ? 366 : 365))
		{
			spans -= isLeapYear(*year) ? 366 : 365;
			(*year)++;
		}
		while (spans >= moondays(*year, *month))
		{
			spans -= moondays(*year, *month);
			(*month)++;
		}
		*day += spans;
	}
	//{OPT} check with herspan()
}
