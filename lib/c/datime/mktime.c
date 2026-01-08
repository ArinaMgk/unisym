// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant yo MIX bin^16+
// LastCheck: RFZ18
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
#define _HIS_TIME_H
#include <time.h>
#include "../../../inc/c/datime.h"

#if 0
time_t mktime(struct tm* time)
{
	time_t res = 0;
	stduint days = herspan(time->tm_year + 1900, time->tm_mon, time->tm_mday) - herspan(1970, 1, 1);
	res += 60 * 60 * 24 * days;
	res += 60 * 60 * time->tm_hour;
	res += 60 * time->tm_min;
	res += time->tm_sec;
	return res;
}

#endif
