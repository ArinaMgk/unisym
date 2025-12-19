// UTF-8 
// @ArinaMgk
// Command Line Parameter Parsing and Processing

#include "inc/aasm.h"
#include "../inc/c/stdinc.h"
#include <stdlib.h>

// outform.h
_ESYM_C{
	struct outffmt* ofmt_find(char*);
	struct dbgffmt* dfmt_find(struct outffmt*, char*);
	void ofmt_list(struct outffmt*, FILE*);
	void dfmt_list(struct outffmt* ofmt, FILE* fp);
	struct outffmt* ofmt_register(void);
	extern struct dbgffmt null_debug_form;
}

// ver.c
_ESYM_C{
	extern const char aasm_version[];
	extern const char aasm_date[];
	extern const char aasm_compile_options[];
	extern const char aasm_comment[];
	extern const char aasm_signature[];
}

// preproc.h
_ESYM_C{
	enum preproc_token pp_token_hash(const char* token);
}

// label.h
_ESYM_C{
	extern char lprefix[PREFIX_MAX];
	extern char lpostfix[PREFIX_MAX];
}






_ESYM_C
void aasm_log(loglevel_t level, const char* fmt, ...);

static void copy_filename(char* dst, const char* src)
{
	size_t len = StrLength(src);
	if (len >= _IMM(FILENAME_MAX)) {
		//--auto clean flag
		log_0file = 1;
		aasm_log(_LOG_FATAL, "file name too long");
	}
	else StrCopyN(dst, src, FILENAME_MAX);
}


// Transform a string to be safely used as a Makefile target or prerequisite
static char* quote_for_make(const char* str)
{
	const char* p;
	char* os, * q;

	size_t n = 1;		/* Terminating zero */
	size_t nbs = 0;

	if (!str)
		return NULL;

	for (p = str; *p; p++) {
		switch (*p) {
		case ' ':
		case '\t':
			/* Convert N backslashes + ws -> 2N+1 backslashes + ws */
			n += nbs + 2;
			nbs = 0;
			break;
		case '$':
		case '#':
			nbs = 0;
			n += 2;
			break;
		case '\\':
			nbs++;
			n++;
			break;
		default:
			nbs = 0;
			n++;
			break;
		}
	}

	/* Convert N backslashes at the end of filename to 2N backslashes */
	if (nbs)
		n += nbs;

	os = q = (char*)malc(n);

	nbs = 0;
	for (p = str; *p; p++) {
		switch (*p) {
		case ' ':
		case '\t':
			while (nbs--)
				*q++ = '\\';
			*q++ = '\\';
			*q++ = *p;
			break;
		case '$':
			*q++ = *p;
			*q++ = *p;
			nbs = 0;
			break;
		case '#':
			*q++ = '\\';
			*q++ = *p;
			nbs = 0;
			break;
		case '\\':
			*q++ = *p;
			nbs++;
			break;
		default:
			*q++ = *p;
			nbs = 0;
			break;
		}
	}
	while (nbs--)
		*q++ = '\\';

	*q = '\0';

	return os;
}

// Get a parameter for a command line option.
// First arg must be in the form of e.g. -f...
static char* get_param(char* p, char* q, bool* advance)
{
	*advance = false;
	if (p[2]) {                 /* the parameter's in the option */
		p += 2;
		while (ascii_isspace(*p))
			p++;
		return p;
	}
	if (q && q[0]) {
		*advance = true;
		return q;
	}
	//--auto clean flag
	log_0file = 1;
	want_usage = 1;
	aasm_log(_LOG_ERROR, "option `-%c' requires an argument", p[1]);
	return NULL;
}


_ESYM_C{
void no_pp_reset(char* file, int pass, evalfunc eval, ListGen* listgen, StrList** deplist);
char* no_pp_getline(void);
void no_pp_cleanup(int pass);
}
static Preproc no_pp = {
	no_pp_reset,
	no_pp_getline,
	no_pp_cleanup
};

_ESYM_C int opt_verbose_info = nil;
_ESYM_C int using_debug_info;
enum op_type {
	op_normal,                  /* Preprocess and assemble */
	op_preprocess,              /* Preprocess only */
	op_depend,                  /* Generate dependencies */
};//{dupl}
_ESYM_C enum op_type operating_mode = op_normal;


struct textargs {
	const char* label;
	int value;
};

#define OPT_PREFIX 0
#define OPT_POSTFIX 1
static struct textargs textopts[] = {
	{"prefix", OPT_PREFIX},
	{"postfix", OPT_POSTFIX},
	{NULL, 0}
};

_ESYM_C bool stopoptions;
_ESYM_C FILE* error_file;
_ESYM_C char inname[FILENAME_MAX];
_ESYM_C char outname[FILENAME_MAX];
_ESYM_C char listname[FILENAME_MAX];
_ESYM_C char errname[FILENAME_MAX];
_ESYM_C const struct warning warnings[ERR_WARN_MAX + 1];
_ESYM_C bool warning_on[ERR_WARN_MAX + 1];
_ESYM_C bool warning_on_global[ERR_WARN_MAX + 1];
_ESYM_C outffmt** ofmt_f();
_ESYM_C int optimizing;
_ESYM_C bool tasm_compatible_mode;
_ESYM_C bool depend_emit_phony;
_ESYM_C bool depend_missing_ok;
_ESYM_C const char* depend_target;
_ESYM_C const char* depend_file;
//
_ESYM_C Preproc* preproc;
_ESYM_C int drop(); _ESYM_C void exit(int);
bool process_arg(char* p, char* q)
{
	char* param;
	int i;
	int advance = false;
	//{}bool advance = false;
	bool do_warn;

	if (!p || !p[0])
		return false;

	if (p[0] == '-' && !stopoptions) {
		if (StrIndexChar("oOfpPdDiIlFXuUZwW", p[1])) {
			/* These parameters take values */
			if (!(param = get_param(p, q, (bool*) & advance)))
				return advance;
		}

		switch (p[1]) {
		case 's':
			error_file = stdout;
			break;

		case 'o':		/* output file */
			copy_filename(outname, param);
			break;

		case 'f':		/* output format */
			*ofmt_f() = ofmt_find(param);
			if (!(*ofmt_f())) {
				//--auto clean flag
				log_0file = 1;
				want_usage = 1;
				aasm_log(_LOG_FATAL, "unrecognised output format `%s' - use -hf for a list", param);
			}
			break;

		case 'O':		/* Optimization level */
		{
			int opt;

			if (!*param) {
				/* Naked -O == -Ox */
				optimizing = INT_MAX >> 1; /* Almost unlimited */
			}
			else {
				while (*param) {
					switch (*param) {
					case '0': case '1': case '2': case '3': case '4':
					case '5': case '6': case '7': case '8': case '9':
						opt = StrTokenULong(param, &param, 10);

						/* -O0 -> optimizing == -1, 0.98 behaviour */
						/* -O1 -> optimizing == 0, 0.98.09 behaviour */
						if (opt < 2)
							optimizing = opt - 1;
						else
							optimizing = opt;
						break;

					case 'v':
					case '+':
						param++;
						opt_verbose_info = true;
						break;

					case 'x':
						param++;
						optimizing = INT_MAX >> 1; /* Almost unlimited */
						break;

					default:
						//--auto clean flag
						aasm_log(_LOG_FATAL, "unknown optimization option -O%c\n", *param);
						break;
					}
				}
			}
			break;
		}

		case 'p': case 'P':
			Preprocessor::pre_include(param); break;
		case 'd': case 'D':
			Preprocessor::pre_define(param); break;
		case 'u': case 'U':
			Preprocessor::pre_undefine(param); break;
		case 'i': case 'I':
			Preprocessor::include_path(param); break;

		case 'l':			/* listing file */
			copy_filename(listname, param);
			break;

		case 'Z':			/* error messages file */
			StrCopy(errname, param);
			break;

		case 'F':			/* specify debug format */
			(*ofmt_f())->current_dfmt = dfmt_find((*ofmt_f()), param);
			if (!(*ofmt_f())->current_dfmt) {
				//--auto clean flag
				log_0file = 1;
				want_usage = 1;
				aasm_log(_LOG_FATAL, "unrecognized debug format `%s' for output format `%s'", param, (*ofmt_f())->shortname);
			}
			using_debug_info = true;
			break;

		case 'X':		/* specify error reporting format */
			if (StrCompareInsensitive("vc", param) == 0)
				_logstyle = _LOG_STYLE_MSVC;
			else if (StrCompareInsensitive("gnu", param) == 0)
				_logstyle = _LOG_STYLE_GCC;
			else {
				//--auto clean flag
				log_0file = 1;
				want_usage = 1;
				aasm_log(_LOG_FATAL, "unrecognized error reporting format `%s'", param);
			}
			break;

		case 'g':
			using_debug_info = true;
			break;

		case 'h':
			printf
			("usage: nasm [-@ response file] [-o outfile] [-f format] "
				"[-l listfile]\n"
				"            [options...] [--] filename\n"
				"    or nasm -v   for version info\n\n"
				"    -t          assemble in SciTech TASM compatible mode\n"
				"    -g          generate debug information in selected format.\n");
			printf
			("    -E (or -e)  preprocess only (writes output to stdout by default)\n"
				"    -a          don't preprocess (assemble only)\n"
				"    -M          generate Makefile dependencies on stdout\n"
				"    -MG         d:o, missing files assumed generated\n\n"
				"    -Z<file>    redirect error messages to file\n"
				"    -s          redirect error messages to stdout\n\n"
				"    -F format   select a debugging format\n\n"
				"    -I<path>    adds a pathname to the include file path\n");
			printf
			("    -O<digit>   optimize branch offsets (-O0 disables, default)\n"
				"    -P<file>    pre-includes a file\n"
				"    -D<macro>[=<value>] pre-defines a macro\n"
				"    -U<macro>   undefines a macro\n"
				"    -X<format>  specifies error reporting format (gnu or vc)\n"
				"    -w+foo      enables warning foo (equiv. -Wfoo)\n"
				"    -w-foo      disable warning foo (equiv. -Wno-foo)\n"
				"Warnings:\n");
			for (i = 0; i <= ERR_WARN_MAX; i++)
				printf("    %-23s %s (default %s)\n",
					warnings[i].name, warnings[i].help,
					warnings[i].enabled ? "on" : "off");
			printf
			("\nresponse files should contain command line parameters"
				", one per line.\n");
			if (p[2] == 'f') {
				printf("\nvalid output formats for -f are"
					" (`*' denotes default):\n");
				ofmt_list((*ofmt_f()), stdout);
			}
			else {
				printf("\nFor a list of valid output formats, use -hf.\n");
				printf("For a list of debug formats, use -f <form> -y.\n");
			}
			drop();	exit(0); // never need usage message here
			break;

		case 'y':
			printf("\nvalid debug formats for '%s' output format are"
				" ('*' denotes default):\n", (*ofmt_f())->shortname);
			dfmt_list((*ofmt_f()), stdout);
			drop(); exit(0);// exit(drop());
			break;

		case 't':
			tasm_compatible_mode = true;
			break;

		case 'v':
			printf("NASMANL version %s compiled on %s%s\n",
				aasm_version, aasm_date, aasm_compile_options);
			drop(); exit(0);        /* never need usage message here */
			break;

		case 'e':              /* preprocess only */
		case 'E':
			operating_mode = op_preprocess;
			break;

		case 'a':              /* assemble only - don't preprocess */
			preproc = &no_pp;
			break;

		case 'W':
			if (param[0] == 'n' && param[1] == 'o' && param[2] == '-') {
				do_warn = false;
				param += 3;
			}
			else {
				do_warn = true;
			}
			goto set_warning;

		case 'w':
			if (param[0] != '+' && param[0] != '-') {
				//--auto clean flag
				log_0file = 1;
				want_usage = 1;
				aasm_log(_LOG_ERROR, "invalid option to `-w'");
				break;
			}
			do_warn = (param[0] == '+');
			param++;
			goto set_warning;
		set_warning:
			for (i = 0; i <= ERR_WARN_MAX; i++)
				if (!StrCompareInsensitive(param, warnings[i].name))
					break;
			if (i <= ERR_WARN_MAX)
				warning_on_global[i] = do_warn;
			else if (!StrCompareInsensitive(param, "all"))
				for (i = 1; i <= ERR_WARN_MAX; i++)
					warning_on_global[i] = do_warn;
			else if (!StrCompareInsensitive(param, "none"))
				for (i = 1; i <= ERR_WARN_MAX; i++)
					warning_on_global[i] = !do_warn;
			else {
				//--auto clean flag
				log_0file = 1;
				want_usage = 1;
				aasm_log(_LOG_ERROR, "invalid warning `%s'", param);
			}
			break;

		case 'M':
			switch (p[2]) {
			case 0:
				operating_mode = op_depend;
				break;
			case 'G':
				operating_mode = op_depend;
				depend_missing_ok = true;
				break;
			case 'P':
				depend_emit_phony = true;
				break;
			case 'D':
				depend_file = q;
				advance = true;
				break;
			case 'T':
				depend_target = q;
				advance = true;
				break;
			case 'Q':
				depend_target = quote_for_make(q);
				advance = true;
				break;
			default:
				//--auto clean flag
				log_0file = 1;
				want_usage = 1;
				aasm_log(_LOG_ERROR, "unknown dependency option `-M%c'", p[2]);
				break;
			}
			if (advance && (!q || !q[0])) {
				//--auto clean flag
				log_0file = 1;
				want_usage = 1;
				aasm_log(_LOG_ERROR, "option `-M%c' requires a parameter", p[2]);
				break;
			}
			break;

		case '-':
		{
			int s;

			if (p[2] == 0) {        /* -- => stop processing options */
				stopoptions = 1;
				break;
			}
			for (s = 0; textopts[s].label; s++) {
				if (!StrCompareInsensitive(p + 2, textopts[s].label)) {
					break;
				}
			}

			switch (s) {

			case OPT_PREFIX:
			case OPT_POSTFIX:
			{
				if (!q) {
					//--auto clean flag
					log_0file = 1;
					want_usage = 1;
					aasm_log(_LOG_ERROR, "option `--%s' requires an argument", p + 2);
					break;
				}
				else {
					advance = 1, param = q;
				}

				if (s == OPT_PREFIX) {
					StrCopyN(lprefix, param, PREFIX_MAX - 1);
					lprefix[PREFIX_MAX - 1] = 0;
					break;
				}
				if (s == OPT_POSTFIX) {
					StrCopyN(lpostfix, param, POSTFIX_MAX - 1);
					lpostfix[POSTFIX_MAX - 1] = 0;
					break;
				}
				break;
			}
			default:
			{
				//--auto clean flag
				log_0file = 1;
				want_usage = 1;
				aasm_log(_LOG_ERROR, "unrecognised option `--%s'", p + 2);
				break;
			}
			}
			break;
		}

		default:
			if (!(*ofmt_f())->setinfo(GI_SWITCH, &p)) {
				//--auto clean flag
				log_0file = 1;
				want_usage = 1;
				aasm_log(_LOG_ERROR, "unrecognised option `-%c'", p[1]);
			}
			break;
		}
	}
	else {
		if (*inname) {
			log_0file = 1;
			want_usage = 1;
			aasm_log(_LOG_ERROR, "more than one input file specified");
		}
		else {
			copy_filename(inname, p);
		}
	}

	return advance;
}

// Process args from a string of args, rather than the argv array. Used by the environment variable and response file processing.
void process_args(char* args)
{
	char* p, * q, * arg, * prevarg;
	char separator = ' ';

	p = args;
	if (*p && *p != '-')
		separator = *p++;
	arg = NULL;
	while (*p) {
		q = p;
		while (*p && *p != separator)
			p++;
		while (*p == separator)
			*p++ = '\0';
		prevarg = arg;
		arg = q;
		if (process_arg(prevarg, arg))
			arg = NULL;
	}
	if (arg)
		process_arg(arg, NULL);
}

_ESYM_C{
void process_respfile(FILE* rfile);
void process_response_file(const char* file);

}
// Parse paths of necessary files and options
void parse_cmdline(int argc, char** argv)
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



