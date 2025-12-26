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


#include "../../inc/c/stdinc.h"
#include "aasm-main.h"
#include "aasm-typedef.h"
#include "aasm-limits.h"

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

#if 1

#define WRITECHAR(p,v)				\
	do {					\
	*(uint8_t *)(p) = (v);			\
	(p) += 1;				\
	} while (0)

#define WRITESHORT(p,v)				\
	do {					\
	*(uint16_t *)(p) = (v);			\
	(p) += 2;				\
	} while (0)

#define WRITELONG(p,v)				\
	do {					\
	*(uint32_t *)(p) = (v);			\
	(p) += 4;				\
	} while (0)

#define WRITEDLONG(p,v)				\
	do {					\
	*(uint64_t *)(p) = (v);			\
	(p) += 8;				\
	} while (0)

#define WRITEADDR(p,v,s)			\
	do {					\
	uint64_t _wa_v = (v);			\
	MemCopyN((p), &_wa_v, (s));		\
	(p) += (s);				\
	} while (0)

#else /* !X86_MEMORY */

#define WRITECHAR(p,v)				\
	do {					\
	uint8_t *_wc_p = (uint8_t *)(p);	\
	uint8_t _wc_v = (v);			\
	_wc_p[0] = _wc_v;			\
	(p) = (void *)(_wc_p + 1);		\
	} while (0)

#define WRITESHORT(p,v)				\
	do {					\
	uint8_t *_ws_p = (uint8_t *)(p);	\
	uint16_t _ws_v = (v);			\
	_ws_p[0] = _ws_v;			\
	_ws_p[1] = _ws_v >> 8;			\
	(p) = (void *)(_ws_p + 2);		\
	} while (0)

#define WRITELONG(p,v)				\
	do {					\
	uint8_t *_wl_p = (uint8_t *)(p);	\
	uint32_t _wl_v = (v);			\
	_wl_p[0] = _wl_v;			\
	_wl_p[1] = _wl_v >> 8;			\
	_wl_p[2] = _wl_v >> 16;			\
	_wl_p[3] = _wl_v >> 24;			\
	(p) = (void *)(_wl_p + 4);		\
	} while (0)

#define WRITEDLONG(p,v)				\
	do {					\
	uint8_t *_wq_p = (uint8_t *)(p);	\
	uint64_t _wq_v = (v);			\
	_wq_p[0] = _wq_v;			\
	_wq_p[1] = _wq_v >> 8;			\
	_wq_p[2] = _wq_v >> 16;			\
	_wq_p[3] = _wq_v >> 24;			\
	_wq_p[4] = _wq_v >> 32;			\
	_wq_p[5] = _wq_v >> 40;			\
	_wq_p[6] = _wq_v >> 48;			\
	_wq_p[7] = _wq_v >> 56;			\
	(p) = (void *)(_wq_p + 8);		\
	} while (0)

#define WRITEADDR(p,v,s)			\
	do {					\
	int _wa_s = (s);			\
	uint64_t _wa_v = (v);			\
	while (_wa_s--) {			\
		WRITECHAR(p,_wa_v);			\
		_wa_v >>= 8;			\
	}					\
	} while(0)

#endif

#if defined(__GNUC__) && __GNUC__ >= 3
# define likely(x)	__builtin_expect(!!(x), 1)
# define unlikely(x)	__builtin_expect(!!(x), 0)
#else
# define likely(x)	(!!(x))
# define unlikely(x)	(!!(x))
#endif

/* The container_of construct: if p is a pointer to member m of
   container class c, then return a pointer to the container of which
   *p is a member. */
#ifndef container_of
#define container_of(p, c, m) ((c *)((char *)(p) - offsetof(c,m)))
#endif


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
	ERR_WARN_FL_OVERFLOW,
	ERR_WARN_FL_DENORM,
	ERR_WARN_FL_UNDERFLOW,
	ERR_WARN_FL_TOOLONG,
	ERR_WARN_USER,
	ERR_WARN_GNUELF
};
#define ERR_WARN_MAX		11
_ESYM_C enum warn_t log_warnt;


#include "./inst.h"

#define CRITICAL 0x100
// The actual expression evaluator function looks like this. When called, it expects the first token of its expression to already be in `*tv'; if it is not, set tv->t_type to TOKEN_INVALID and it will start by calling the scanner.
// If a forward reference happens during evaluation, the evaluator must set `*fwref' to true if `fwref' is non-NULL.
// `critical' is non-zero if the expression may not contain forward references. The evaluator will report its own error if this occurs; if `critical' is 1, the error will be "symbol not defined before use", whereas if `critical' is 2, the error will be "symbol undefined".
// If `critical' has bit 8 set (in addition to its main value: 0x101 and 0x102 correspond to 1 and 2) then an extended expression syntax is recognised, in which relational operators such as =, < and >= are accepted, as well as low-precedence logical operators &&, ^^ and ||.
// If `hints' is non-NULL, it gets filled in with some hints as to the base register in complex effective addresses.
typedef expr *(*evalfunc) (scanner sc, void *scprivate, struct tokenval * tv, int *fwref, int critical, struct eval_hints * hints);

// ---- out ---- //

_ESYM_C struct outffmt of_bin;
_ESYM_C struct outffmt of_ith;
_ESYM_C struct outffmt of_srec;
_ESYM_C struct outffmt of_aout;
_ESYM_C struct outffmt of_aoutb;
_ESYM_C struct outffmt of_coff;
_ESYM_C struct outffmt of_elf32;
_ESYM_C struct outffmt of_elf;
_ESYM_C struct outffmt of_elf64;
_ESYM_C struct outffmt of_as86;
_ESYM_C struct outffmt of_obj;
_ESYM_C struct outffmt of_win32;
_ESYM_C struct outffmt of_win64;
_ESYM_C struct outffmt of_rdf2;// NASM RDF
_ESYM_C struct outffmt of_ieee;
_ESYM_C struct outffmt of_macho32;
_ESYM_C struct outffmt of_macho;
_ESYM_C struct outffmt of_macho64;
_ESYM_C struct outffmt of_dbg;

/* --- From standard.mac via macros.pl: --- */

/* macros.c */
_ESYM_C const unsigned char aasm_stdmac[];
_ESYM_C const unsigned char* const aasm_stdmac_after_tasm;
_ESYM_C const unsigned char* aasm_stdmac_find_package(const char*);

/* --- From insns.dat via insns.pl: --- */

/* insnsn.c */
_ESYM_C const char* const _asm_inst_literal_list[];

/* --- From regs.dat via regs.pl: --- */

/* regs.c */
_ESYM_C const char* const aasm_reg_names[];
/* regflags.c */
_ESYM_C const int32_t aasm_reg_flags[];
/* regvals.c */
_ESYM_C const int aasm_regvals[];

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

struct Parser {
	static void parser_global_info(outffmt *output, struct location * locp);
	static insn *parse_line(int pass, char *buffer, insn * result, evalfunc evaluate, ldfunc ldef);
	static void cleanup_insn(insn * instruction);
};


extern "C" {
#endif

	// ---- main ---- //
	int drop();
	void usage(void);
	void printinfo(void);
	void printl(loglevel_t level, const char* fmt, ...);
	int* handlog(void* _serious, ...);
	#define aasm_log _f_printlog// void aasm_log(loglevel_t level, const char* fmt, ...);
	extern FILE* error_file;
	void SegInit();
	stdint SegAlloc();

	/*
	* Special values for expr->type.  These come after EXPR_REG_END
	* as defined in regs.h.
	*/

	#define EXPR_UNKNOWN	(EXPR_REG_END+1) /* forward references */
	#define EXPR_SIMPLE	(EXPR_REG_END+2)
	#define EXPR_WRT	(EXPR_REG_END+3)
	#define EXPR_SEGBASE	(EXPR_REG_END+4)
	
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
	int64_t readstrnum(char* str, int length, bool* warn);

	enum strfunc {
		STRFUNC_UTF16,
		STRFUNC_UTF32,
	};

	size_t string_transform(char *, size_t, char **, enum strfunc);

	// ---- asm ---- //

	int64_t insn_size
	(int32_t segment, int64_t offset, int bits, uint32_t cp, insn* instruction);

	int64_t assemble
	(int32_t segment, int64_t offset, int bits, uint32_t cp, insn* instruction, struct outffmt* output, ListGen* listgen);

	// ---- eval ---- //
	
	// Called once to tell the evaluator what output format is providing segment-base details, and what function can be used to look labels up.
	void eval_global_info(struct outffmt *output, lfunc lookup_label, struct location * locp);

	// The evaluator itself.
	expr *eval_evaluate(scanner sc, void *scprivate, struct tokenval *tv, int *fwref, int critical, struct eval_hints *hints);
	
	void eval_cleanup(void);
		
	// ---- pptok.c ---- //
	extern const char* const pp_directives[];
	extern const uint8_t pp_directives_len[];
#include "../data/pptok.h"

	// ---- cmdparam ---- //
	bool process_arg(char* p, char* q);
	void process_args(char* args);
	void parse_cmdline(int argc, char** argv);
	
	// ---- list ---- //
	extern ListGen aasmlist;
	extern int user_nolist; // fbk - 9/1/00
	
	// ---- label ---- //

	extern char lprefix[PREFIX_MAX];
	extern char lpostfix[PREFIX_MAX];
	
	bool lookup_label(char* label, int32_t* segment, int64_t* offset);
	bool is_extern(char *label);
	void define_label(rostr label, int32_t segment, int64_t offset, char *special, bool is_norm, bool isextrn, struct outffmt *ofmt);
	void redefine_label(rostr label, int32_t segment, int64_t offset, char *special, bool is_norm, bool isextrn, struct outffmt *ofmt);
	void define_common(char *label, int32_t segment, int32_t size, char *special, struct outffmt *ofmt);
	void declare_as_global(char *label, char *special);
	int init_labels(void);
	void cleanup_labels(void);
	char *local_scope(char *label);

	// ---- quote ---- //

	char *aasm_quote(char *str, size_t len);
	size_t aasm_unquote(char *str, char **endptr);
	char *aasm_skip_string(char *str);

	// ---- stdscan ---- //

	extern char* stdscan_bufptr;
	void stdscan_reset(void);
	int stdscan(void* private_data, struct tokenval* tv);
	int aasm_token_hash(const char* token, struct tokenval* tv);
	void stdscan_cleanup(void);

	// ---- lib ---- //

	void* aasm_realloc(void*, size_t);

	const char* prefix_name(int);

	void assert_failed(const char*, int, const char*);
	#define aasm_assert(x)						\
		do {							\
		if (unlikely(!(x)))					\
			assert_failed(__FILE__,__LINE__,#x);		\
		} while (0)

	char* aasm_strsep(char** stringp, const char* delim);

	#define fwriteint8_t(d,f) putc(d,f)
	void fwriteint16_t(uint16_t data, FILE * fp);
	void fwriteint32_t(uint32_t data, FILE * fp);
	void fwriteint64_t(uint64_t data, FILE * fp);
	void fwriteaddr(uint64_t data, int size, FILE * fp);

	#define ZERO_BUF_SIZE	4096
	extern const uint8_t zero_buffer[ZERO_BUF_SIZE];
	size_t fwritezero(size_t bytes, FILE *fp);

	void standard_extension(char *inname, char *outname, rostr extension);
	
	/*
	* Binary search routine. Returns index into `array' of an entry
	* matching `string', or <0 if no match. `array' is taken to
	* contain `size' elements.
	*/
	int bsi(const char *string, const char **array, int size);
	int bsii(const char *string, const char **array, int size);// insensitive

	char *src_set_fname(char *newname);
	int32_t src_set_linnum(int32_t newline);
	int32_t src_get_linnum(void);
	/*
	* src_get may be used if you simply want to know the source file and line.
	* It is also used if you maintain private status about the source location
	* It return 0 if the information was the same as the last time you
	* checked, -1 if the name changed and (new-old) if just the line changed.
	*/
	int src_get(int32_t *xline, char **xname);


	// ---- out ---- //

	#ifndef OF_ONLY
	#ifndef OF_ALL
	#define OF_ALL                  /* default is to have all formats */
	#endif
	#endif

	#ifdef OF_ALL
	#define OF_BIN
	#define OF_OBJ
	#define OF_ELF32
	#define OF_ELF64
	#define OF_COFF
	#define OF_AOUT
	#define OF_AOUTB
	#define OF_WIN32
	#define OF_WIN64
	#define OF_AS86
	#define OF_RDF2
	#define OF_IEEE
	#define OF_MACHO32
	#define OF_MACHO64
	#define OF_DBG
	#endif

	uint64_t realsize(enum out_type type, uint64_t size);

	/* Do-nothing versions of all the debug routines */
	struct outffmt;
	void null_debug_init(struct outffmt *of, void *id, FILE * fp);
	void null_debug_linenum(const char *filename, int32_t linenumber, int32_t segto);
	void null_debug_deflabel(char *name, int32_t segment, int64_t offset,
							int is_global, char *special);
	void null_debug_routine(const char *directive, const char *params);
	void null_debug_typevalue(int32_t type);
	void null_debug_output(int type, void *param);
	void null_debug_cleanup(void);
	extern struct dbgffmt* null_debug_arr[2];

	struct outffmt *ofmt_find(char *);
	struct dbgffmt *dfmt_find(struct outffmt *, char *);
	void ofmt_list(struct outffmt *, FILE *);
	void dfmt_list(struct outffmt *ofmt, FILE * fp);
	struct outffmt *ofmt_register(void);
	extern struct dbgffmt null_debug_form;

	// ---- endo ---- //
	
#ifdef _INC_CPP
}
#endif
#endif
