// UTF-8 
// @ArinaMgk
// Command Line Parameter Parsing and Processing

#include "inc/aasm.h"
#include "../inc/cpp/Format/Code.hpp"
#include "../inc/cpp/Lango/Archend.hpp"
#include "../inc/cpp/string"

// #include <stdlib.h>
_ESYM_C char* getenv(const char* name);

// preproc.h
_ESYM_C{
	enum preproc_token pp_token_hash(const char* token);
}


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

_ESYM_C FILE* error_file;
_ESYM_C const struct warning warnings[ERR_WARN_MAX + 1];
_ESYM_C int drop(); _ESYM_C void exit(int);

static void SelectArchitecture(AssemblerOptions& options, const char* name)
{
	uni::lango::ArchendInterface* archend = uni::lango::FindArchend(name);
	if (!archend) {
		log_0file = 1;
		want_usage = 1;
		aasm_log(_LOG_FATAL,
			"unrecognised architecture `%s' - use -ha for a list", name);
		return;
	}
	options.Target.SelectArchend(*archend);
		globalarch = archend->DumpArchend().shortname;
}

bool ProcessArgument(AssemblerOptions& options, char* p, char* q)
{
	char* param;
	int i;
	int advance = false;
	//{}bool advance = false;
	bool do_warn;

	if (!p || !p[0])
		return false;

	if (p[0] == '-' && !options.StopOptionParsing) {
		if (!StrCompareInsensitive(p, "-mrelax") ||
			!StrCompareInsensitive(p, "--mrelax")) {
			options.EnableRISCVRelax = true;
			return false;
		}
		bool architecture_option = false;
		if (!StrCompareInsensitive(p, "-march") ||
			!StrCompareInsensitive(p, "--arch")) {
			architecture_option = true;
			param = q;
			advance = true;
		}
		else if (!StrCompareNInsensitive(p, "-march=", 7) ||
			!StrCompareNInsensitive(p, "--arch=", 7)) {
			architecture_option = true;
			param = p + 7;
		}
		if (architecture_option) {
			if (!param || !param[0]) {
				log_0file = 1;
				want_usage = 1;
				aasm_log(_LOG_ERROR,
					"option `%s' requires an argument", p);
				return advance;
			}
			SelectArchitecture(options, param);
			return advance;
		}
	}

	if (p[0] == '-' && !options.StopOptionParsing) {
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
			copy_filename(options.OutputName, param);
			break;

		case 'f':		/* output format */
		{
			uni::CodeendInterface* codeend = uni::FindCodeend(param);
			if (!codeend) {
				//--auto clean flag
				log_0file = 1;
				want_usage = 1;
				aasm_log(_LOG_FATAL, "unrecognised output format `%s' - use -hf for a list", param);
			}
			options.Target.SelectCodeend(*codeend);
			uni::SelectCodeend(options.Target.Codeend());
			break;
		}

		case 'O':		/* Optimization level */
		{
			int opt;

			if (!*param) {
				/* Naked -O == -Ox */
				options.OptimizationLevel = INT_MAX >> 1; /* Almost unlimited */
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
							options.OptimizationLevel = opt - 1;
						else
							options.OptimizationLevel = opt;
						break;

					case 'v':
					case '+':
						param++;
						opt_verbose_info = true;
						break;

					case 'x':
						param++;
						options.OptimizationLevel = INT_MAX >> 1; /* Almost unlimited */
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
			copy_filename(options.ListName, param);
			break;

		case 'Z':			/* error messages file */
			StrCopy(options.ErrorName, param);
			break;

		case 'F':			/* specify debug format */
			if (!options.Target.Codeend().SelectDebugFormat(param)) {
				//--auto clean flag
				log_0file = 1;
				want_usage = 1;
				aasm_log(_LOG_FATAL, "unrecognized debug format `%s' for output format `%s'", param, options.Target.Codeend().DumpCodeend().shortname);
			}
			options.EnableDebugInfo = true;
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
			options.EnableDebugInfo = true;
			break;

		case 'h':
			printf
			("usage: aasm [-@ response file] [-o outfile] [-f format] "
				"[-l listfile]\n"
				"            [options...] [--] filename\n"
				"    or aasm -v   for version info\n\n"
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
				"    -mrelax     emit RISC-V linker-relaxation hints\n"
				"    -march=name select the instruction architecture\n"
				"    --arch name select the instruction architecture\n"
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
			if (p[2] == 'a') {
				printf("\nvalid architectures for -march are"
					" (`*' denotes default):\n");
				uni::lango::ListArchends(
					options.Target.Archend(), stdout);
			}
			else if (p[2] == 'f') {
				printf("\nvalid output formats for -f are"
					" (`*' denotes default):\n");
				uni::ListCodeends(options.Target.Codeend(), stdout);
			}
			else {
				printf("\nFor a list of valid output formats, use -hf.\n");
				printf("For a list of debug formats, use -f <form> -y.\n");
			}
			drop();	exit(0); // never need usage message here
			break;

		case 'y':
			printf("\nvalid debug formats for '%s' output format are"
				" ('*' denotes default):\n",
				options.Target.Codeend().DumpCodeend().shortname);
			options.Target.Codeend().ListDebugFormats(stdout);
			drop(); exit(0);// exit(drop());
			break;

		case 't':
			options.TasmCompatible = true;
			break;

		case 'v':
			printf("AASM version %s, compiled on %s%s\n",
				aasm_version, aasm_date, aasm_compile_options);
			drop(); exit(0);        /* never need usage message here */
			break;

		case 'e':              /* preprocess only */
		case 'E':
			options.Mode = AssemblyMode::Preprocess;
			break;

		case 'a':              /* assemble only - don't preprocess */
			options.Preprocessor = &no_pp;
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
				options.WarningEnabled[i] = do_warn;
			else if (!StrCompareInsensitive(param, "all"))
				for (i = 1; i <= ERR_WARN_MAX; i++)
					options.WarningEnabled[i] = do_warn;
			else if (!StrCompareInsensitive(param, "none"))
				for (i = 1; i <= ERR_WARN_MAX; i++)
					options.WarningEnabled[i] = !do_warn;
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
				options.Mode = AssemblyMode::Dependency;
				break;
			case 'G':
				options.Mode = AssemblyMode::Dependency;
				options.AllowMissingDependencies = true;
				break;
			case 'P':
				options.EmitPhonyDependencies = true;
				break;
			case 'D':
				options.DependencyFile = q;
				advance = true;
				break;
			case 'T':
				options.DependencyTarget = q;
				advance = true;
				break;
			case 'Q':
				options.DependencyTarget = quote_for_make(q);
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
				options.StopOptionParsing = true;
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
			if (!options.Target.Codeend().ProcessOption(&p)) {
				//--auto clean flag
				log_0file = 1;
				want_usage = 1;
				aasm_log(_LOG_ERROR, "unrecognised option `-%c'", p[1]);
			}
			break;
		}
	}
	else {
		if (*options.InputName) {
			log_0file = 1;
			want_usage = 1;
			aasm_log(_LOG_ERROR, "more than one input file specified");
		}
		else {
			copy_filename(options.InputName, p);
		}
	}

	return advance;
}

// Process args from a string of args, rather than the argv array. Used by the environment variable and response file processing.
void ProcessArguments(AssemblerOptions& options, char* args)
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
		if (ProcessArgument(options, prevarg, arg))
			arg = NULL;
	}
	if (arg)
		ProcessArgument(options, arg, NULL);
}

_ESYM_CPP{
void ProcessResponseFile(AssemblerOptions& options, FILE* rfile);
void ProcessResponsePath(AssemblerOptions& options, const char* file);
}

// Parse paths of necessary files and options
void ParseCommandLine(AssemblerOptions& options, int argc, char** argv)
{
	FILE* rfile;
	char* envreal, * envcopy = NULL, * p, * arg;
	int i;

	*options.InputName = *options.OutputName = *options.ListName = *options.ErrorName = '\0';
	for0(i, numsof(options.WarningEnabled)) options.WarningEnabled[i] = warnings[i].enabled;// init

	// AASM adapt for NASM
	arg = NULL;
	if (envreal = getenv("NASMENV")) {
		uni::String nasmenv(envreal);
		ProcessArguments(options, nasmenv.reflect());
	}

	// Default using $uincpath/Kasha/n_ and $uincpath/naasm/n_ if
	// - uincpath is defined
	if (envreal = getenv("uincpath")) {
		uni::String uinc = "-I ";
		uinc += envreal;
		uinc += "/Kasha/n_";
		ProcessArguments(options, uinc.reflect());
	}
	if (envreal = getenv("uincpath")) {
		uni::String uinc = "-I ";
		uinc += envreal;
		uinc += "/naasm/n_";
		ProcessArguments(options, uinc.reflect());
	}

	// process the actual command line
	while (--argc) {
		bool advance;
		argv++;
		// response file: use the text of a textfile as part of command
		// @resp
		if (argv[0][0] == '@') {
			// allow multiple arguments on a single line
			ProcessResponsePath(options, argv[0] + 1);
			//{} why advance here: ...
			argc--; argv++;
		}
		// -@resp
		if (!options.StopOptionParsing && argv[0][0] == '-' && argv[0][1] == '@') {
			p = get_param(argv[0], argc > 1 ? argv[1] : NULL, &advance);
			if (p) {
				rfile = fopen(p, "r");
				if (rfile) {
					ProcessResponseFile(options, rfile);
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
			advance = ProcessArgument(options, argv[0], argc > 1 ? argv[1] : NULL);
		argv += advance, argc -= advance;
	}

	/* Look for basic command line typos.  This definitely doesn't
	   catch all errors, but it might help cases of fumbled fingers. */
	if (!*options.InputName) {
		//--auto clean flag
		log_0file = 1;
		want_usage = 1;
		aasm_log(_LOG_ERROR, "no input file specified");
	}
	else if (
		!StrCompare(options.InputName, options.ErrorName) ||
		!StrCompare(options.InputName, options.OutputName) ||
		!StrCompare(options.InputName, options.ListName) ||
		(options.DependencyFile && !StrCompare(options.InputName, options.DependencyFile))) {
		log_0file = 1;
		want_usage = 1;
		aasm_log(_LOG_FATAL, "file `%s' is both input and output file", options.InputName);
	}

	if (*options.ErrorName) {
		error_file = fopen(options.ErrorName, "w");
		if (!error_file) {
			error_file = stderr;        /* Revert to default! */
			log_0file = 1;
			want_usage = 1;
			aasm_log(_LOG_FATAL, "cannot open file `%s' for error messages", options.ErrorName);
		}
	}
}



