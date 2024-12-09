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
// end in aasm.c
#define _CRT_SECURE_NO_WARNINGS

#include "tmp.h"

#define _INC_DNODE//{}
#include <stdinc.h>
//{} #include <ustdbool.h>
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
bool endian_little = true;
bool using_debug_info = false;

#define FILENAME_MAX 260
char inname[FILENAME_MAX];
char outname[FILENAME_MAX];
char listname[FILENAME_MAX];
char errname[FILENAME_MAX];

Strpage* offsets;//{} static
Strbuff* forwrefs;//{} static

FILE* outfile = NULL;
time_t official_compile_time;

typedef struct ForewardReference {
	int lineno;// line number
	_TEMP int operand;
} forwrefinfo;


void getcrt(_Need_free char** const pname, stduint* const plineno)
{
	//{TODO}
	//*plineno = 12;
	//*pname = StrHeap("nihao.asm");
}


int drop();
int* handlog(void* _serious, ...);
int main(int argc, char** argv) {
	time(&startup_time);
	error_file = stderr;
	_call_serious = handlog;
	SegInit();
	main0();
	offsets = StrpageNew();
	forwrefs = StrbuffNew(sizeof(forwrefinfo));
	main2(argc, argv);
	


	//{} {} {}
	return drop();
}

int drop() {
	if (want_usage) usage();
#ifdef _DEBUG
	printinfo();
#endif
	StrpageFree(offsets);
	StrbuffFree(forwrefs);
	//{TODO} eval_cleanup();
	//{TODO} stdscan_cleanup();
	return malc_count;
}
