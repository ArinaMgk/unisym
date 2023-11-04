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
	if (weekday) *weekday = (PastDays - 1) % 7;// increase for the first day of the month
	// 20131229 is Sunday(0) PastDays(1)
	if (moondays) *moondays = MoonDays[month];
	return PastDays;// [excluded hday] how many days between the first day of the month 00:00 from hday 23:59
}

unsigned DatimeCalendar(word year, word month, byte* weekday, byte* moondays)// Haruno RFC01.00:00: Decide to not merge with GetMoexDayIdentity(), and this function can be compatile with any case.
{
	if (year >= 2014)return GetMoexDayIdentity(year, month, weekday, moondays);
	byte MoonDays[13] = { 0,0X1F,0x1C,0x1F,0x1E,0x1F,0x1E,0x1F,0x1F,0x1E,0x1F,0x1E,27 };// 27 can for 2013 and earlier
	// E.g. 2023 Jan: Jan~Nov + 27
	// E.g. 2022 Dec: 2022.Dec + Jan~Nov + 27 == Jan~Dec + 27 
	unsigned SpanDays = 0;// [included hday] how many days between the first day of the month 00:00 until hday 23:59
	word crtyear = year;
	if (!month || month > 12)return 0;
	if (isLeapYear(year) && month <= 2) MoonDays[2] = 29;
	while (crtyear++ < 2013)
		SpanDays += isLeapYear(crtyear) ? 366 : 365;
	for (unsigned i = month; i <= 12; i++)
		SpanDays += MoonDays[i];
	if (weekday) *weekday = 6 - ((SpanDays) % 7);
	if (moondays) *moondays = month == 12 ? 31 : MoonDays[month];
	return SpanDays;
}

sll POSIXGetSeconds(struct tm *tm)
{
    sll t;
    sll y = tm->tm_year;// year number relative from 1900
    // Detail IEEE 1003.1:2004, section 4.14, POSIX-style time constant
    // Application:
    //- NASM 0207 [main source of the function]
    t = (y - 70) * 365 + // year number relative from 1970
        (y - 69) / 4 - // 1973 just passed a heap year 1972 (first 4-heap year after, 69=73-4)
        (y - 1) / 100 + // 2001 just passed next 100-heap after 1970
        (y + 299) / 400;// 2001 (101 after 1900) just passed the first after 1970 (299=400-101)
    // another choice `getHerDayDif(y)`
    t += tm->tm_yday;
    t *= 24;
    t += tm->tm_hour;
    t *= 60;
    t += tm->tm_min;
    t *= 60;
    t += tm->tm_sec;
    return t;
}
