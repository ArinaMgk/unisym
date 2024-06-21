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

#include "../../../inc/c/datime.h"
#include <time.h>

uint64 POSIXGetSeconds(struct tm* tm)
{
    uint64 t;
    uint64 y = tm->tm_year;// year number relative from 1900
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
