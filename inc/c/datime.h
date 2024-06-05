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

#ifndef _LIB_DATETIME
#define _LIB_DATETIME

#include "stdinc.h"
#include <time.h>

//{TODO} utime.h

#ifdef _INC_CPP
	namespace uni {
#endif

struct datime_t {
	// time
	byte second;// [0 60], heap-second accepted
	byte minute;// [0 60]
	byte hour;  // [0 23]
	// date
	byte mday;  // [0 30]
	byte month; // [0 11]
	stdint year;  //[CUSTOMED] usually 0 since 1900 or 0
	stduint property;
};

struct datimex_t {
	struct datime_t datime;
	byte weekday;
	uint16 yearday;
	byte isDaylightSavingTime;
};

		
// `[MACRO]`
#define isLeapYear(year) (!((year)&3)&&((year)%100)||!((year)%400)) // RFQ27

// `[MACRO]` y: Yesus based
#define getHerDaySpanYear(y) ((y>2013)?(4 + (y-1900-114)*365 + (y-1900-117+4)/4 - (y-1900-101)/100 + (y-1900-101)/400):-((2014-(y))*365 + (2016-(y))/4 - ((2100-(y))/100) + (2400-(y))/400 - (31-27)))

// `[MACRO]`
#define getHerDaySpanMonth(y,m) (getHerDaySpanYear(y) + (m-1)*31 - (m-1)/2 - (m>2)*(30-28-isLeapYear(y)) + ((m>8)&&(m&1)))

// `[MACRO]`
#define herspan(y,m,d) (getHerDaySpanMonth(y,m)+d-1)

// `[MACRO]`
#define getHerWeekNumber(y,m,d) ((herspan(y,m,d)+6)/7-(herspan(y,m,d)<0))

// What is the day{`0~6`} of the day{`01~31`} in the month{`01~12`} of the year{`0000~9999`}
unsigned weekday(word year, word month, word day);

// `[MACRO]` How many days in the month{01~12}
#define moondays(year, month) (30 + (((month)&1) ^ ((month)>7)) - ((month)==2?2-isLeapYear(year):0))

//
uint64 POSIXGetSeconds(struct tm* tm);

// Reverse function of herspan()
void fromherp(stdint herspans, word* year, word* month, word* day);


#ifdef _INC_CPP
	}
#endif

#endif
