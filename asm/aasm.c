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
#include <ustring.h>
#include <stdio.h>
#include <time.h>
#include <strpage.h>
#include <strbuff.h>
#include "aasm.h"

extern time_t startup_time;

static byte rest_parse_phase = 2;
bool terminate_after_phase = false;
bool treat_warn_as_erro = false;
bool enable_warning = true;

#define FILENAME_MAX 260
static char inname[FILENAME_MAX];
char outname[FILENAME_MAX];
static char listname[FILENAME_MAX];
static char errname[FILENAME_MAX];

Strpage* _offsets;
Strbuff* _forwrefs;

FILE* outfile = NULL;


typedef struct ForewardReference {
	int lineno;// line number
	_TEMP int operand;
} forwrefinfo;

void getcrt(_Need_free char** const pname, stduint* const plineno)
{
	//{TODO}
	*plineno = 12;
	*pname = StrHeap("nihao.asm");
}

int main(int argc, char** argv, char** envv)
{
	time(&startup_time);
	//{TODO} report_error = report_error_gnu;
	//{TODO} error_file = stderr;
	//{TODO} _set_malloc_error(report_error);
	_offsets = StrpageNew();
	_forwrefs = StrbuffNew(byteof(forwrefinfo));
	_logstyle = _LOG_STYLE_GCC;
	_call_serious = handlog;
	//{TODO} preproc = &nasmpp;
	//{TODO} operating_mode = op_normal;


	//{TODO} 
	printl(_LOG_WARN, "QAQ...");
	//{TODO} 

	printinfo();
	StrpageFree(_offsets);
	StrbuffFree(_forwrefs);
	//{TODO} eval_cleanup();
	//{TODO} stdscan_cleanup();
	return malc_count;
}


