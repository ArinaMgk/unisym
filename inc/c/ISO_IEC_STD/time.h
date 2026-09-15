// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant
// LastCheck: 2026 Sep 15
// AllAuthor: @dosconio
// ModuTitle: Alias for ISO IEC Standard CPL time.h
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

#ifndef _HER_TIME_H
#define _HER_TIME_H

#include "../datime.h"

#define CLOCKS_PER_SEC 1000

#ifndef _CLOCK_T_DEFINED
#define _CLOCK_T_DEFINED
typedef stduint clock_t;
#endif

#ifndef _TIME_T_DEFINED
#define _TIME_T_DEFINED
typedef sint64 time_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

clock_t clock(void);
double difftime(time_t time1, time_t time0);
time_t mktime(struct tm* timeptr);
time_t time(time_t* timer);
char* asctime(const struct tm* timeptr);
char* ctime(const time_t* timer);
struct tm* gmtime(const time_t* timer);
struct tm* localtime(const time_t* timer);
size_t strftime(char* s, size_t maxsize, const char* format, const struct tm* timeptr);

#ifdef __cplusplus
}
#endif

#endif

