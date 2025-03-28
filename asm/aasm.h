// ASCII DJGPP (Debug MSVC2010) TAB4 CRLF
// Attribute: ArnCovenant Host bin^16+ 386+
// LastCheck: RFX19
// AllAuthor: @dosconio, @ArinaMgk
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

#ifndef _AASM_HEAD
#define _AASM_HEAD

#define __PROJNAME__ "Ain Assembler of Arinae UNISYM"
#define __PROJVERS__ "0101H"
#define __AUTHOR__ "@dosconio"
#define __LICENSE__ "Apache License 2"

#include <c/stdinc.h>

typedef struct {
	//: Called at the start of a pass
	//{TODO} void (*reset) (char*, int, efunc, evalfunc, ListGen*, StrList**);
	//: Called to fetch a line of preprocessed source.
	_Need_free char* (*getline) (void);
	//: Called at the end of a pass.
	void (*cleanup) (int);
} Prepro;
extern Prepro _pp;

typedef const unsigned char macros_t;

// Single Processor Logging
extern int log_pass1;
extern int log_pass2;
extern int log_0file;
extern int log_0severity;
extern int want_usage;// bool

enum warn_t {
	ERR_WARN_NONE,
	ERR_WARN_MNP, // macro-num-parameters warning
	ERR_WARN_MSR,
	ERR_WARN_MDP,
	ERR_WARN_OL,
	ERR_WARN_NOV,
	ERR_WARN_GNUELF,
	ERR_WARN_FL_OVERFLOW,
	ERR_WARN_FL_DENORM,
	ERR_WARN_FL_UNDERFLOW,
	ERR_WARN_FL_TOOLONG,
	ERR_WARN_USER
};
#define ERR_WARN_MAX		11
extern enum warn_t log_warnt;

void printinfo(void);
void printl(loglevel_t level, const char* fmt, ...);
int* handlog(void* _serious, ...);

// ---- FILE ----
struct dbgffmt;
struct outffmt;

// DebugFileFormat
struct dbgffmt {

};// AKA dfmt

// OutFileFormat
#define OFMT_TEXT 1 /* Text file format */
struct outffmt {
	//
	rostr    fullname;
	//
	rostr    shortname;
	//
	unsigned flags;
	// to the first element of the debug information
	struct dbgffmt** debug_formats;
	// to the element that is being used
	/*
		this is set to the default at compile time and changed if the -F option is selected.
		If developing a set of new debug formats for an output format, be sure to set this to whatever default you want.
	*/
	struct dbgffmt* current_dfmt;
	//








	
};// AKA ofmt





#endif
