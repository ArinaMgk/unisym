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
//extern "C" {
#include "tmp.h"
//}

#define _INC_DNODE//{}
#include <c/stdinc.h>
//{} #include <ustdbool.h>
#include <c/ustring.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <c/strpage.h>
#include <c/strbuff.h>
//#include "aasm.h"


//_ESYM_C{
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
// Dependency:
bool depend_emit_phony = false;
bool depend_missing_ok = false;
const char* depend_target = NULL;
const char* depend_file = NULL;

Strpage* offsets;//{} static
Strbuff* forwrefs;//{} static

FILE* outfile = NULL;
time_t official_compile_time;

typedef struct ForewardReference {
	int lineno;// line number
	_TEMP int operand;
} forwrefinfo;
//}

// ---- maybe suppressible warnings ----
// Entry zero isn't an actual warning, but it used for -w+error/-Werror.
//TODO: <Bitmap>
bool warning_on[ERR_WARN_MAX + 1]; /* Current state */
bool warning_on_global[ERR_WARN_MAX + 1]; /* Command-line state */

extern const struct warning warnings[ERR_WARN_MAX + 1];


_ESYM_C void getcrt(_Need_free char** const pname, stduint* const plineno)
{
	//{TODO}
	//*plineno = 12;
	//*pname = StrHeap("nihao.asm");
}


bool stopoptions = false;


// static subfunc:
bool process_arg(char* p, char* q);
void process_respfile(FILE* rfile);
void process_args(char* args);
void process_response_file(const char* file);
char* get_param(char* p, char* q, bool* advance);
// Parse paths of necessary files and options
static void parse_cmdline(int argc, char** argv)
{
	FILE* rfile;
	char* envreal, * envcopy = NULL, * p, * arg;
	int i;

	*inname = *outname = *listname = *errname = '\0';
	for0(i, numsof(warning_on_global)) warning_on_global[i] = warnings[i].enabled;// init

	// AASM adapt for NASM
	envreal = getenv("NASMENV");
	arg = NULL;
	if (envreal) {
		envcopy = StrHeap(envreal);
		process_args(envcopy);
		memf(envcopy);
	}

	// process the actual command line
	while (--argc) {
		bool advance;
		argv++;
		// response file: use the text of a textfile as part of command
		// @resp
		if (argv[0][0] == '@') {
			// allow multiple arguments on a single line
			process_response_file(argv[0] + 1);
			//{} why advance here: ...
			argc--; argv++;
		}
		// -@resp
		if (!stopoptions && argv[0][0] == '-' && argv[0][1] == '@') {
			p = get_param(argv[0], argc > 1 ? argv[1] : NULL, &advance);
			if (p) {
				rfile = fopen(p, "r");
				if (rfile) {
					process_respfile(rfile);
					fclose(rfile);
				}
				else {
					log_0file = 1;
					want_usage = 1;
					aasm_log(_LOG_ERROR, "unable to open response file `%s'", p);
				}
			}
		}
		// other args
		else
			advance = process_arg(argv[0], argc > 1 ? argv[1] : NULL);
		argv += advance, argc -= advance;
	}

	/* Look for basic command line typos.  This definitely doesn't
	   catch all errors, but it might help cases of fumbled fingers. */
	if (!*inname) {
		//--auto clean flag
		log_0file = 1;
		want_usage = 1;
		aasm_log(_LOG_ERROR, "no input file specified");
	}
	else if (
		!StrCompare(inname, errname) ||
		!StrCompare(inname, outname) ||
		!StrCompare(inname, listname) ||
		(depend_file && !StrCompare(inname, depend_file))) {
		log_0file = 1;
		want_usage = 1;
		aasm_log(_LOG_FATAL, "file `%s' is both input and output file", inname);
	}

	if (*errname) {
		error_file = fopen(errname, "w");
		if (!error_file) {
			error_file = stderr;        /* Revert to default! */
			log_0file = 1;
			want_usage = 1;
			aasm_log(_LOG_FATAL, "cannot open file `%s' for error messages", errname);
		}
	}
}


void mainx();
_ESYM_C void usage();
int drop();
int* handlog(void* _serious, ...);
void define_macros_early(time_t* startup_time);
int main(int argc, char** argv) {
	time(&startup_time);
	error_file = stderr;
	_call_serious = handlog;
	_logstyle = _LOG_STYLE_GCC;
	SegInit();
	main0();
	offsets = StrpageNew();
	forwrefs = StrbuffNew(sizeof(forwrefinfo));

	mainx();
	// return drop();
	// : contain 5 leaks
	define_macros_early(&startup_time);


	parse_cmdline(argc, argv);
	if (terminate_after_phase) {
		if (want_usage)
			usage();
		want_usage = 0;
		drop();
		return 1;
	}

	main2(argc, argv);





	//{} {} {}
	return drop() - _TEMP 5;
}



int drop() {
	if (want_usage) usage();
#ifdef _DEBUG
	// printinfo();
#endif
	StrpageFree(offsets);
	StrbuffFree(forwrefs);
	//{TODO} eval_cleanup();
	//{TODO} stdscan_cleanup();
	return malc_count;
}
