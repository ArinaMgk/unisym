// ASCII C TAB4 CRLF
// Attribute: ArnCovenant Host bins(16+) 386+
// LastCheck: RFX19
// AllAuthor: @dosconio
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

#include <stdinc.h>
#include <ustdbool.h>
#include <stdio.h>
#include <../../demo/template/version/version.h>
#include <time.h>
#include <strpage.h>
#include <strbuff.h>

static time_t startup_time;
static time_t temp_time;
static byte rest_parse_phase = 2;
static bool terminate_after_phase = false;

Strpage* _offsets;
Strbuff* _forwrefs;

typedef struct ForewardReference {
	int lineno;// line number
	_TEMP int operand;
} forwrefinfo;

static printinfo(void) {
	time(&temp_time);
	printf("%s", __PROJ_INFO__);
	printf("Timspan: %" PRIuSTD "\n", temp_time - startup_time);
}

int main(int argc, char** argv, char** envv)
{
	time(&startup_time);
	//{TODO} report_error = report_error_gnu;
	//{TODO} error_file = stderr;
	//{TODO} _set_malloc_error(report_error);
	_offsets = StrpageNew();
	_forwrefs = StrbuffNew(byteof(forwrefinfo));


	

	printinfo();
	StrpageFree(_offsets);
	StrbuffFree(_forwrefs);
	//{} ...
	return malc_count;
}


