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

#define MAX_OPERANDS 5


#include "../inc/c/stdinc.h"
#include "inc/aasm-main.h"
#include "inc/aasm-typedef.h"
#include "inc/aasm-limits.h"

typedef uint32_t opflags_t;

#define aasm_isalpha(x)  ascii_isalpha((unsigned char)(x))
#define aasm_isdigit(x)  ascii_isdigit((unsigned char)(x))
#define aasm_isalnum(x)  ascii_isalnum((unsigned char)(x))
#define aasm_isxdigit(x) ascii_isxdigit((unsigned char)(x))

/*
 * isidstart matches any character that may start an identifier, and isidchar
 * matches any character that may appear at places other than the start of an
 * identifier. E.g. a period may only appear at the start of an identifier
 * (for local labels), whereas a number may appear anywhere *but* at the
 * start.
 */

#define isidstart(c) ( aasm_isalpha(c) || (c)=='_' || (c)=='.' || (c)=='?' \
								  || (c)=='@' )
#define isidchar(c)  ( isidstart(c) || aasm_isdigit(c) || \
			   (c)=='$' || (c)=='#' || (c)=='~' )

/* Ditto for numeric constants. */

#define isnumstart(c)  ( aasm_isdigit(c) || (c)=='$' )
#define isnumchar(c)   ( aasm_isalnum(c) || (c)=='_' )

/* This returns the numeric value of a given 'digit'. */

#define numvalue(c)  ((c)>='a' ? (c)-'a'+10 : (c)>='A' ? (c)-'A'+10 : (c)-'0')

// typedef struct {
// 	//: Called at the start of a pass
// 	//{TODO} void (*reset) (char*, int, efunc, evalfunc, ListGen*, StrList**);
// 	//: Called to fetch a line of preprocessed source.
// 	_Need_free char* (*getline) (void);
// 	//: Called at the end of a pass.
// 	void (*cleanup) (int);
// } Prepro;
// extern Prepro _pp;

typedef const unsigned char macros_t;


/*
 * This declaration passes the "pass" number to all other modules
 * "pass0" assumes the values: 0, 0, ..., 0, 1, 2
 * where 0 = optimizing pass
 *       1 = pass 1
 *       2 = pass 2
 */
# ifdef _INC_CPP
extern "C" {
# endif

extern int pass0;
extern int passn;		/* Actual pass number */
extern bool tasm_compatible_mode;
extern int optimizing;
extern int globalbits;          /* 16, 32 or 64-bit mode */
extern int globalrel;		/* default to relative addressing? */
extern int maxbits;		/* max bits supported by output */

// version-relative
extern const char aasm_version[];
extern const char aasm_date[];
extern const char aasm_compile_options[];
extern const char aasm_comment[];
extern const char aasm_signature[];

# ifdef _INC_CPP
}
# endif

// Single Processor Logging
# ifdef _INC_CPP
extern "C" {
# endif
	extern int log_pass1;
	extern int log_pass2;
	extern int log_0file;
	extern int log_0severity;
	extern int want_usage;// bool
# ifdef _INC_CPP
}
# endif

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
_ESYM_C enum warn_t log_warnt;


#include "insns.h"

#define CRITICAL 0x100
// The actual expression evaluator function looks like this. When called, it expects the first token of its expression to already be in `*tv'; if it is not, set tv->t_type to TOKEN_INVALID and it will start by calling the scanner.
// If a forward reference happens during evaluation, the evaluator must set `*fwref' to true if `fwref' is non-NULL.
// `critical' is non-zero if the expression may not contain forward references. The evaluator will report its own error if this occurs; if `critical' is 1, the error will be "symbol not defined before use", whereas if `critical' is 2, the error will be "symbol undefined".
// If `critical' has bit 8 set (in addition to its main value: 0x101 and 0x102 correspond to 1 and 2) then an extended expression syntax is recognised, in which relational operators such as =, < and >= are accepted, as well as low-precedence logical operators &&, ^^ and ||.
// If `hints' is non-NULL, it gets filled in with some hints as to the base register in complex effective addresses.
typedef expr *(*evalfunc) (scanner sc, void *scprivate, struct tokenval * tv, int *fwref, int critical, struct eval_hints * hints);

#ifdef _INC_CPP

// Outfile Format
struct OutfileFormat {
	static outffmt* crt_format;
	static void setMode();
};

struct Preprocessor {
	static void include_path(char*);
	static void pre_include(char*);
	static void pre_define(char*);
	static void pre_undefine(char*);
	static void runtime(char*);
	static void extra_stdmac(macros_t*);
};


extern "C" {
#endif

	// ---- main ---- //
	int drop();
	void usage(void);
	void printinfo(void);
	void printl(loglevel_t level, const char* fmt, ...);
	int* handlog(void* _serious, ...);
	void aasm_log(loglevel_t level, const char* fmt, ...);
	
	int is_reloc(expr*);
	int is_simple(expr *);
	int is_really_simple(expr *);
	int is_unknown(expr *);
	int is_just_unknown(expr *);
	int64_t reloc_value(expr *);
	int32_t reloc_seg(expr *);
	int32_t reloc_wrt(expr*);

	// Convert a string into a number
	int64_t readnum(char *str, bool *error);

	// Convert a character constant into a number. Sets `*warn' to true if an overflow occurs. str points to and length covers the middle of the string, without the quotes.
	int64_t readstrnum(char *str, int length, bool *warn);

	// ---- pptok.c ---- //
	extern const char* const pp_directives[];
	extern const uint8_t pp_directives_len[];
#include "pptok.h"

	// ---- cmdparam ---- //
	bool process_arg(char* p, char* q);
	void process_args(char* args);
	void parse_cmdline(int argc, char** argv);
	
	// ---- list ---- //
	extern ListGen nasmlist;
	extern int user_nolist; // fbk - 9/1/00

	
#ifdef _INC_CPP
}
#endif
#endif
