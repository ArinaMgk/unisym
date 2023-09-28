// ASCII
// Date and Time
/*
	Copyright 2023 ArinaMgk

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/

#include "../datime.h"

unsigned GetMoexDayIdentity(word year, word month, byte* weekday, byte* moondays)// RFV26
{
	unsigned PastDays = 4;
	byte MoonDays[13] = { 0,0X1F,0x1C,0x1F,0x1E,0x1F,0x1E,0x1F,0x1F,0x1E,0x1F,0x1E,0x1F };
	word crtyear = 2014;
	word crtmoon = 1;
	if (year < 2014 || !month || month > 12)return 0;
	while (crtyear < year)// until crtyear == month
	{
		PastDays += isLeapYear(crtyear) ? 366 : 365;
		crtyear++;
	}
	if (isLeapYear(year)) MoonDays[2] = 29;
	while (crtmoon < month)
		PastDays += MoonDays[crtmoon++];
	if (weekday) *weekday = (PastDays - 2 + 1) % 7;// increase for the first day of the month
	// 20131229 is Sunday(0)
	if (moondays) *moondays = MoonDays[month];
	return PastDays;// how many days between the first day of the month 00:00 from hday 23:59
}

