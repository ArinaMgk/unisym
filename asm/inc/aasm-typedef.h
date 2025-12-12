
#ifndef _AASM_TYPEDEF
#define _AASM_TYPEDEF

#include "../inc/c/ustdbool.h"
#include "../inc/c/ustring.h"

#include "stdio.h"

// [x86]
struct location {
	int64_t offset;
	int32_t segment;
	int known;
};

// Linked list of strings
typedef struct string_list {
    struct string_list *next;
    char str[1];
} StrList;

// Token types returned by the scanner, in addition to ordinary ASCII character values, and zero for end-of-string
enum token_type {		/* token types, other than chars */
	TOKEN_INVALID = -1,         /* a placeholder value */
	TOKEN_EOS = 0,              /* end of string */
	TOKEN_EQ = '=', TOKEN_GT = '>', TOKEN_LT = '<',     /* aliases */
	TOKEN_ID = 256,		/* identifier */
	TOKEN_NUM = tok_number,			/* numeric constant */
	TOKEN_ERRNUM,		/* malformed numeric constant */
	TOKEN_STR = tok_string,			/* string constant */
	TOKEN_ERRSTR,               /* unterminated string constant */
	TOKEN_FLOAT,                /* floating-point constant */
	TOKEN_REG,			/* register name */
	TOKEN_INSN,			/* instruction name */
	TOKEN_HERE, TOKEN_BASE,     /* $ and $$ */
	TOKEN_SPECIAL,              /* BYTE, WORD, DWORD, QWORD, FAR, NEAR, etc */
	TOKEN_PREFIX,               /* A32, O16, LOCK, REPNZ, TIMES, etc */
	TOKEN_SHL, TOKEN_SHR,       /* << and >> */
	TOKEN_SDIV, TOKEN_SMOD,     /* // and %% */
	TOKEN_GE, TOKEN_LE, TOKEN_NE,       /* >=, <= and <> (!= is same as <>) */
	TOKEN_DBL_AND, TOKEN_DBL_OR, TOKEN_DBL_XOR, /* &&, || and ^^ */
	TOKEN_SEG, TOKEN_WRT,       /* SEG and WRT */
	TOKEN_FLOATIZE,		/* __floatX__ */
	TOKEN_STRFUNC,		/* __utf16__, __utf32__ */
};

// The expression evaluator must be passed a scanner function
//{} a standard scanner is provided as part of nasmlib.c.
// Preprocessor will use a different one. Scanners, and the token-value structures they return, look like this.
// return value from the scanner is always a copy of the `t_type' field in the structure.
struct tokenval {
	enum token_type t_type;
	char *t_charptr;
	int64_t t_integer, t_inttwo;
};
typedef int (*scanner) (void *private_data, struct tokenval * tv);

// Expression-evaluator datatype. Expressions, within the evaluator, are stored as an array of these beasts, terminated by a record with type==0. Mostly, it's a vector type: each type denotes some kind of a component, and the value denotes the multiple of that component present in the expression. The exception is the WRT type, whose `value' field denotes the segment to which the expression is relative. These segments will be segment-base types, i.e. either odd segment values or SEG_ABS types. So it is still valid to assume that anything with a `value' field of zero is insignificant.
typedef struct {
	int32_t type; // a register, or EXPR_xxx
	int64_t value; // must be >= 32 bits
} expr;

//{} ?: give an example?
// The evaluator can also return hints about which of two registers used in an expression should be the base register. See also the `operand' structure.
struct eval_hints {
	int64_t base;
	int type;
};


/// ---- ---- File ---- ----
struct dbgffmt;
struct outffmt;

typedef const unsigned char macros_t;
typedef void (*ldfunc)
(char* label, int32_t segment, int64_t offset, char* special, bool is_norm, bool isextrn, struct outffmt* ofmt);
typedef expr* (*evalfunc)
(scanner sc, void* scprivate, struct tokenval* tv, int* fwref, int critical, struct eval_hints* hints);


// DebugFileFormat
struct dbgffmt {
	//
	rostr    fullname;
	//
	rostr    shortname;
	// init - called initially to set up local pointer to object format, void pointer to implementation defined data, file pointer (which probably won't be used, but who knows?), and error function.
	void (*init) (struct outffmt * of, void *id, FILE * fp);
	// linenum - called any time there is output with a change of line number or file.
	void (*linenum) (const char *filename, int32_t linenumber, int32_t segto);
	// debug_deflabel - called whenever a label is defined. Parameters are the same as to 'symdef()' in the output format. This function would be called before the output format version.
	void (*debug_deflabel) (char *name, int32_t segment, int64_t offset, int is_global, char *special);
	// debug_directive - called whenever a DEBUG directive other than 'LINE' is encountered. 'directive' contains the first parameter to the DEBUG directive, and params contains the rest.
	// For example, 'DEBUG VAR _somevar:int' would translate to a call to this function with 'directive' equal to "VAR" and 'params' equal to "_somevar:int".
	void (*debug_directive) (const char* directive, const char* params);
	// typevalue - called whenever the assembler wishes to register a type for the last defined label.  This routine MUST detect if a type was already registered and not re-register it.
	void (*debug_typevalue) (int32_t type);
	// debug_output - called whenever output is required 'type' is the type of info required, and this is format-specific
	void (*debug_output) (int type, void *param);
	// cleanup - called after processing of file is complete
	void (*cleanup) (void);
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
	// (set by -F)
	struct dbgffmt* current_dfmt;
	// if non-NULL, is a NULL-terminated list of `char *'s pointing to extra standard macros supplied by the object format
	// e.g. a sensible initial default value of __SECT__, and user-level equivalents for any format-specific directives
	//{TODO} Dchain
	macros_t* stdmac;
	// called at the start of an output session
	// This tells the output format
	// - what file it will be writing to
	// - what routine to report errors through
	// - how to interface to the label manager and expression evaluator if necessary
	// - gives it a chance to do other initialisation
	void (*init) (FILE * fp, ldfunc ldef, evalfunc eval);
	// called to pass generic information to the object file
	// 1st parameter gives the information type (currently only command line switches)
	// 2nd parameter gives the value.
	// returns 1 if recognized, 0 if unrecognized
	int (*setinfo) (enum geninfo type, char** string);
	// called by assemble() to write actual generated code or data to the object file
	// Typically it doesn't have to actually _write_ it, just store it for later
	// The `type' argument specifies the type of output data, and usually the size as well: its contents are described below.
	//{TODO} OstreamTrait
	void (*output) (
		int32_t segto,
		const void* data,
		enum out_type type,
		uint64_t size,
		int32_t segment,
		int32_t wrt
		);
	// called once for every symbol defined in the module being assembled.
	// It gives the name and value of the symbol, and indicates whether it has been declared to be global.
	// Note: param "name", when passed, will point to a piece of static storage allocated inside the label manager - it's safe to keep using that pointer, because the label manager doesn't clean up until after the output driver has.
	// Note: `is_global' are: 0 means the symbol is local; 1 means the symbol is global; 2 means the symbol is common (in which case `offset' holds the _size_ of the variable). Anything else is available for the output driver to use internally.
	// This routine explicitly _is_ allowed to call the label manager to define further symbols, if it wants to, even though it's been called _from_ the label manager.  That much re-entrancy is guaranteed in the label manager. However, the label manager will in turn call this routine, so it should be prepared to be re-entrant itself.
	// The `special' parameter contains special information passed through from the command that defined the label: it may have been an EXTERN, a COMMON or a GLOBAL. The distinction should be obvious to the output format from the other parameters.
	void (*symdef) (char *name, int32_t segment, int64_t offset, int is_global, char *special);
	// This procedure is called when the source code requests a segment change. It should return the corresponding segment _number_ for the name, or NO_SEG if the name is not a valid segment name.
	// It may also be called with NULL, in which case it is to return the _default_ section number for starting assembly in.
	// It is allowed to modify the string it is given a pointer to.
	// It is also allowed to specify a default instruction size for the segment, by setting `*bits' to 16 or 32. Or, if it doesn't wish to define a default, it can leave `bits' alone.
	int32_t (*section) (char *name, int pass, int *bits);
	// called to modify the segment base values
	// returned from the SEG operator. It is given a segment base value (i.e. a segment value with the low bit set), and is required to produce in return a segment value which may be different. It can map segment bases to absolute numbers by means of returning SEG_ABS types.
	// It should return NO_SEG if the segment base cannot be determined; the evaluator (which calls this routine) is responsible for throwing an error condition if that occurs in pass two or in a critical expression.
	int32_t(*segbase) (int32_t segment);
	// called to allow the output driver to process its own specific directives. When called, it has the directive word in `directive' and the parameter string in `value'. It is called in both assembly passes, and `pass' will be either 1 or 2.
	// This procedure should return zero if it does not _recognise_ the directive, so that the main program can report an error.
	// If it recognises the directive but then has its own errors, it should report them itself and then return non-zero. It should also return non-zero if it correctly processes the directive.
	int (*directive) (char* directive, char* value, int pass);
	// called before anything else - even before the "init" routine - and is passed the name of the input file from which this output file is being generated.
	// return its preferred name for the output file in `outname', if outname[0] is not '\0', and do nothing to `outname' otherwise. Since it is called before the driver is properly initialized, it has to be passed its error handler separately.
	// This procedure may also take its own copy of the input file name for use in writing the output file: it is _guaranteed_ that it will be called before the "init" routine.
	// The parameter `outname' points to an area of storage
	void (*filename) (char *inname, char *outname);
	// This procedure is called after assembly finishes, to allow the output driver to clean itself up and free its memory. Typically, it will also be the point at which the object file actually gets _written_.
	// One thing the cleanup routine should always do is to close the output file pointer.
	void (*cleanup) (int debuginfo);

	
};// AKA ofmt

typedef struct {
    /*
     * Called to initialize the listing file generator. Before this
     * is called, the other routines will silently do nothing when
     * called. The `char *' parameter is the file name to write the
     * listing to.
     */
    void (*init) (char *);

    /*
     * Called to clear stuff up and close the listing file.
     */
    void (*cleanup) (void);

    /*
     * Called to output binary data. Parameters are: the offset;
     * the data; the data type. Data types are similar to the
     * output-format interface, only OUT_ADDRESS will _always_ be
     * displayed as if it's relocatable, so ensure that any non-
     * relocatable address has been converted to OUT_RAWDATA by
     * then. Note that OUT_RAWDATA,0 is a valid data type, and is a
     * dummy call used to give the listing generator an offset to
     * work with when doing things like uplevel(LIST_TIMES) or
     * uplevel(LIST_INCBIN).
     */
    void (*output) (int32_t, const void *, enum out_type, uint64_t);

    /*
     * Called to send a text line to the listing generator. The
     * `int' parameter is LIST_READ or LIST_MACRO depending on
     * whether the line came directly from an input file or is the
     * result of a multi-line macro expansion.
     */
    void (*line) (int, char *);

    /*
     * Called to change one of the various levelled mechanisms in
     * the listing generator. LIST_INCLUDE and LIST_MACRO can be
     * used to increase the nesting level of include files and
     * macro expansions; LIST_TIMES and LIST_INCBIN switch on the
     * two binary-output-suppression mechanisms for large-scale
     * pseudo-instructions.
     *
     * LIST_MACRO_NOLIST is synonymous with LIST_MACRO except that
     * it indicates the beginning of the expansion of a `nolist'
     * macro, so anything under that level won't be expanded unless
     * it includes another file.
     */
    void (*uplevel) (int);

    /*
     * Reverse the effects of uplevel.
     */
    void (*downlevel) (int);

    /*
     * Called on a warning or error, with the error message.
     */
    void (*error)(int severity, const char *pfx, const char *msg);
} ListGen;


/// ---- ---- ? ---- ----
enum geninfo { GI_SWITCH };

/// ---- ---- Preproc ---- ----
typedef struct preproc_ops {
	// Called at the start of a pass
	void (*reset) (char* filename, int nof_pass, evalfunc, ListGen* evaluator_fn, StrList** listing_generator);

	/*
	 * Called to fetch a line of preprocessed source. The line
	 * returned has been malloc'ed, and so should be freed after
	 * use.
	 */
	char *(*getline) (void);

	/*
	 * Called at the end of a pass.
	 */
	void (*cleanup) (int);
} Preproc;
extern Preproc nasmpp;

#endif /* _AASM_TYPEDEF */
