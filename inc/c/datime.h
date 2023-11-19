// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant yo MIX bin^16+
// LastCheck: 2023 Nov 16
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

#ifndef _LIB_DATETIME
#define _LIB_DATETIME

#include "alice.h"
#include <time.h>

//{TODO} utime.h

// origin: unisym/kasha/n_timer.a : %imacro GetMoexDayIdentity 2
// year>=2014, month>0
// Return pastdays and weekday(0~6)
llong GetMoexDayIdentity(word year, word month);

// extern of GetMoexDayIdentity, can show the months before 2014.
llong DatimeCalendar(word year, word month);

llong POSIXGetSeconds(struct tm* tm);

#define isLeapYear(year) (!((year)&3)&&((year)%100)||!((year)%400)) // RFQ27

// y: Yesus based
#define getHerDaySpanYear(y) ((y>2013)?(4 + (y-1900-114)*365 + (y-1900-117+4)/4 - (y-1900-101)/100 + (y-1900-101)/400):-((2014-(y))*365 + (2016-(y))/4 - ((2100-(y))/100) + (2400-(y))/400 - (31-27)))

#define getHerDaySpanMonth(y,m) (getHerDaySpanYear(y) + (m-1)*31 - (m-1)/2 - (m>2)*(30-28-isLeapYear(y)) + ((m>8)&&(m&1)))

#define herspan(y,m,d) (getHerDaySpanMonth(y,m)+d-1)

#define getHerWeekNumber(y,m,d) ((herspan(y,m,d)+6)/7-(herspan(y,m,d)<0))

unsigned weekday(word year, word month, word day);

unsigned moondays(word year, word month);

#ifdef _AUTO_INCLUDE
	#include "../../lib/c/datime.c"
#endif // _AUTO_INCLUDE


#endif
