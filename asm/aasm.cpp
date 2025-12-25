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
#define _HIS_TIME_H
#include <c/stdinc.h>
#include <c/ustring.h>
#include <cpp/string>

#include <c/datime.h>
#include <time.h>
#include <c/strpage.h>
#include <c/strbuff.h>
#include "inc/aasm.h"


_ESYM_C{
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


// ---- maybe suppressible warnings ----
// Entry zero isn't an actual warning, but it used for -w+error/-Werror.
//TODO: <Bitmap>
bool warning_on[ERR_WARN_MAX + 1]; /* Current state */
bool warning_on_global[ERR_WARN_MAX + 1]; /* Command-line state */

extern const struct warning warnings[ERR_WARN_MAX + 1];
extern int opt_verbose_info;
bool stopoptions = false;
struct location location;//{TODO} static
Preproc* preproc;
static int get_bits(char* value);
static uint32_t get_cpu(char* cpu_str);
static void assemble_file(char*, StrList**);
bool tasm_compatible_mode = false;
int pass0, passn;
int maxbits = 0;
int globalrel = 0;
static int globallineno;        /* for forward-reference tracking */
int optimizing = -1;            /* number of optimization passes to take */
static int sb, cmd_sb = 16;     /* by default */
static uint32_t cmd_cpu = IF_PLEVEL;       /* highest level by default */
static uint32_t cpu = IF_PLEVEL;   /* passed to insn_size & assemble.c */
int64_t global_offset_changed;      /* referenced in labels.c */
int64_t prev_offset_changed;
int32_t stall_count;
int in_abs_seg;                 /* Flag we are in ABSOLUTE seg */
int32_t abs_seg;                   /* ABSOLUTE segment basis */
int32_t abs_offset;                /* ABSOLUTE offset */
static const forwrefinfo* forwref;
//bool want_usage;
int user_nolist = 0;            /* fbk 9/2/00 */

}

/*
 * get/set current offset...
 */
#define GET_CURR_OFFS (in_abs_seg?abs_offset:StrpageGet(offsets,location.segment))
#define SET_CURR_OFFS(x) (in_abs_seg?(void)(abs_offset=(x)):\
	(void)(offsets=StrpageSet(offsets,location.segment,(x))))




_ESYM_C void getcrt(_Need_free char** const pname, stduint* const plineno)
{
	*plineno = globallineno;
	*pname = inname;
}

enum op_type {
	op_normal,     // Preprocess and assemble
	op_preprocess, // Preprocess only
	op_depend,     // Generate dependencies
};
_ESYM_C enum op_type operating_mode;


inline static void aasm_puts(const char* line, FILE* outfile)
{
	if (outfile) {
		fputs(line, outfile);
		putc('\n', outfile);
	}
	else
		puts(line);
}
static void emit_dependencies(StrList* list)
{
	FILE* deps;
	int linepos, len;
	StrList* l, * nl;

	if (depend_file && StrCompare(depend_file, "-")) {
		deps = fopen(depend_file, "w");
		if (!deps) {
			//--auto clean flag
			log_0file = 1;
			want_usage = 1;
			aasm_log(_LOG_ERROR, "unable to write dependency file `%s'", depend_file);
			return;
		}
	}
	else {
		deps = stdout;
	}

	linepos = fprintf(deps, "%s:", depend_target);
	for (l = list; l; l = l->next) {
		len = StrLength(l->str);
		if (linepos + len > 62) {
			fprintf(deps, " \\\n ");
			linepos = 1;
		}
		fprintf(deps, " %s", l->str);
		linepos += len + 1;
	}
	fprintf(deps, "\n\n");

	for (l = list; l; l = nl) {
		if (depend_emit_phony)
			fprintf(deps, "%s:\n\n", l->str);

		nl = l->next;
		memf(l);
	}

	if (deps != stdout)
		fclose(deps);
}

#define ARG_BUF_DELTA 128
void process_respfile(FILE* rfile)
{
	char* buffer, * p, * q, * prevarg;
	int bufsize, prevargsize;

	bufsize = prevargsize = ARG_BUF_DELTA;
	buffer = (char*)malc(ARG_BUF_DELTA);
	prevarg = (char*)malc(ARG_BUF_DELTA);
	prevarg[0] = '\0';

	while (1) {                 /* Loop to handle all lines in file */
		p = buffer;
		while (1) {             /* Loop to handle long lines */
			q = fgets(p, bufsize - (p - buffer), rfile);
			if (!q)
				break;
			p += StrLength(p);
			if (p > buffer && p[-1] == '\n')
				break;
			if (p - buffer > bufsize - 10) {
				int offset;
				offset = p - buffer;
				bufsize += ARG_BUF_DELTA;
				buffer = (char*)aasm_realloc(buffer, bufsize);
				p = buffer + offset;
			}
		}

		if (!q && p == buffer) {
			if (prevarg[0])
				process_arg(prevarg, NULL);
			memf(buffer);
			memf(prevarg);
			return;
		}

		/*
		 * Play safe: remove CRs, LFs and any spurious ^Zs, if any of
		 * them are present at the end of the line.
		 */
		*(p = &buffer[StrSpanExclude(buffer, "\r\n\032")]) = '\0';

		while (p > buffer && ascii_isspace(p[-1]))
			*--p = '\0';

		p = buffer;
		while (ascii_isspace(*p))
			p++;

		if (process_arg(prevarg, p))
			*p = '\0';

		if ((int)StrLength(p) > prevargsize - 10) {
			prevargsize += ARG_BUF_DELTA;
			prevarg = (char*)aasm_realloc(prevarg, prevargsize);
		}
		StrCopyN(prevarg, p, prevargsize);
	}
}
_ESYM_C void exit(int);
void process_response_file(const char* file)
{
	char str[2048];
	FILE* f = fopen(file, "r");
	if (!f) {
		perror(file);
		exit(-1);
	}
	while (fgets(str, sizeof str, f)) {
		process_args(str);
	}
	fclose(f);
}

_ESYM_C struct outffmt** ofmt_f();
void mainx();

//{TODO} UNI DATIME
void define_macros_early(time_t* startup_time)
{
	char temp[128];
	tm lt, * lt_p, gm, * gm_p;
	int64_t posix_time = 0;

	if (lt_p = localtime(startup_time)) {
		lt = *lt_p;

		strftime(temp, sizeof temp, "__DATE__=\"%Y-%m-%d\"", &lt);
		Preprocessor::pre_define(temp);
		strftime(temp, sizeof temp, "__DATE_NUM__=%Y%m%d", &lt);
		Preprocessor::pre_define(temp);
		strftime(temp, sizeof temp, "__TIME__=\"%H:%M:%S\"", &lt);
		Preprocessor::pre_define(temp);
		strftime(temp, sizeof temp, "__TIME_NUM__=%H%M%S", &lt);
		Preprocessor::pre_define(temp);
	}

	if (gm_p = gmtime(startup_time)) {
		gm = *gm_p;

		strftime(temp, sizeof temp, "__UTC_DATE__=\"%Y-%m-%d\"", &gm);
		Preprocessor::pre_define(temp);
		strftime(temp, sizeof temp, "__UTC_DATE_NUM__=%Y%m%d", &gm);
		Preprocessor::pre_define(temp);
		strftime(temp, sizeof temp, "__UTC_TIME__=\"%H:%M:%S\"", &gm);
		Preprocessor::pre_define(temp);
		strftime(temp, sizeof temp, "__UTC_TIME_NUM__=%H%M%S", &gm);
		Preprocessor::pre_define(temp);
	}

	if (gm_p)
		posix_time = POSIXGetSeconds(&gm);
	else if (lt_p)
		posix_time = POSIXGetSeconds(&lt);

	if (posix_time) {
		Preprocessor::pre_define(uni::String::newFormat("__POSIX_TIME__=%[64I]", posix_time).reflect());
	}
}

void define_macros_late(rostr ofmt_shortname)
{
	Preprocessor::pre_define(uni::String::newFormat("__OUTPUT_FORMAT__=%s\n", ofmt_shortname).reflect());
}

_ESYM_C void pp_include_path(char*);
void printlog0(loglevel_t level, const char* fmt, ...);
int main(int argc, char** argv) {
	time(&startup_time);
	error_file = stderr;
	_call_serious = handlog;
	_logstyle = _LOG_STYLE_GCC;
	_f_printlog = printlog0;
	//
	SegInit();
	pass0 = 0;
	preproc = &nasmpp;
	operating_mode = op_normal;
	offsets = StrpageNew();
	forwrefs = StrbuffNew(sizeof(forwrefinfo));

	mainx();
	OutfileFormat::setMode();
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

	if (!using_debug_info)// If debugging info is disabled, suppress any debug calls
		(*ofmt_f())->current_dfmt = &null_debug_form;

	if ((*ofmt_f())->stdmac)
		Preprocessor::extra_stdmac((*ofmt_f())->stdmac);
	Parser::parser_global_info((*ofmt_f()), &location);
	eval_global_info((*ofmt_f()), lookup_label, &location);

	define_macros_late((*ofmt_f())->shortname);

	StrList* depend_list = NULL, ** depend_ptr;
	depend_ptr = (depend_file || (operating_mode == op_depend))
		? &depend_list : NULL;
	if (!depend_target)
		depend_target = outname;

	switch (operating_mode) {
	case op_depend:
	{
		char* line;

		if (depend_missing_ok)
			pp_include_path(NULL);	/* "assume generated" */

		preproc->reset(inname, 0, eval_evaluate, &aasmlist,
			depend_ptr);
		if (outname[0] == '\0')
			(*ofmt_f())->filename(inname, outname);
		outfile = NULL;
		while ((line = preproc->getline()))
			memf(line);
		preproc->cleanup(0);
	}
	break;

	case op_preprocess:
	{
		char* line;
		char* file_name = NULL;
		int32_t prior_linnum = 0;
		int lineinc = 0;

		if (*outname) {
			outfile = fopen(outname, "w");
			if (!outfile) {
				//--auto clean flag
				log_0file = 1;
				aasm_log(_LOG_FATAL, "unable to open output file `%s'", outname);
			}
		}
		else
			outfile = NULL;

		location.known = false;

		/* pass = 1; */
		preproc->reset(inname, 3, eval_evaluate, &aasmlist,
			depend_ptr);

		while ((line = preproc->getline())) {
			/*
			 * We generate %line directives if needed for later programs
			 */
			int32_t linnum = prior_linnum += lineinc;
			int altline = src_get(&linnum, &file_name);
			if (altline) {
				if (altline == 1 && lineinc == 1)
					aasm_puts("", outfile);
				else {
					lineinc = (altline != -1 || lineinc != 1);
					fprintf(outfile ? outfile : stdout,
						"%%line %" PRId32 "+%d %s\n", linnum, lineinc,
						file_name);
				}
				prior_linnum = linnum;
			}
			aasm_puts(line, outfile);
			memf(line);
		}
		memf(file_name);
		preproc->cleanup(0);
		if (outfile)
			fclose(outfile);
		if (outfile && terminate_after_phase)
			remove(outname);
	}
	break;

	case op_normal:
	{
		/*
		 * We must call (*ofmt_f())->filename _anyway_, even if the user
		 * has specified their own output file, because some
		 * formats (eg OBJ and COFF) use (*ofmt_f())->filename to find out
		 * the name of the input file and then put that inside the
		 * file.
		 */
		(*ofmt_f())->filename(inname, outname);

		outfile = fopen(outname, ((*ofmt_f())->flags & OFMT_TEXT) ? "w" : "wb");
		if (!outfile) {
			//--auto clean flag
			log_0file = 1;
			aasm_log(_LOG_FATAL, "unable to open output file `%s'", outname);
		}

		/*
		 * We must call init_labels() before (*ofmt_f())->init() since
		 * some object formats will want to define labels in their
		 * init routines. (eg OS/2 defines the FLAT group)
		 */
		init_labels();

		(*ofmt_f())->init(outfile, define_label, eval_evaluate);
		(*ofmt_f())->current_dfmt->init((*ofmt_f()), NULL, outfile);

		assemble_file(inname, depend_ptr);

		if (!terminate_after_phase) {
			(*ofmt_f())->cleanup(using_debug_info);
			cleanup_labels();
		}
		else {
			/*
			 * Despite earlier comments, we need this fclose.
			 * The object output drivers only fclose on cleanup,
			 * and we just skipped that.
			 */
			fclose(outfile);

			remove(outname);
		}
	}
	break;
	}

	if (depend_list && !terminate_after_phase)
		emit_dependencies(depend_list);
	eval_cleanup();
	stdscan_cleanup();
	//{} {} {} return terminate_after_phase;
	//return drop() _TEMP - 5 - 3;
	return drop() & 0;
}

enum directives {
	D_NONE, D_ABSOLUTE, D_BITS, D_COMMON, D_CPU, D_DEBUG, D_DEFAULT,
	D_EXTERN, D_FLOAT, D_GLOBAL, D_LIST, D_SECTION, D_SEGMENT, D_WARNING
};
static const char* directives[] = {
	"", "absolute", "bits", "common", "cpu", "debug", "default",
	"extern", "float", "global", "list", "section", "segment", "warning"
};
static enum directives getkw(char** directive, char** value);


static void assemble_file(char* fname, StrList** depend_ptr)
{
	char* directive, * value, * p, * q, * special, * line, debugid[80];
	insn output_ins;
	int i, validid;
	bool rn_error;
	int32_t seg;
	int64_t offs;
	struct tokenval tokval;
	expr* e;
	int pass_max;

	if (cmd_sb == 32 && cmd_cpu < IF_386) {
		//--auto clean flag
		aasm_log(_LOG_FATAL, "command line: 32-bit segment size requires a higher cpu");
	}

	pass_max = prev_offset_changed = (INT_MAX >> 1) + 2; /* Almost unlimited */
	for (passn = 1; pass0 <= 2; passn++) {
		int pass1, pass2;
		ldfunc def_label;

		pass1 = pass0 == 2 ? 2 : 1;	/* 1, 1, 1, ..., 1, 2 */
		pass2 = passn > 1 ? 2 : 1;     /* 1, 2, 2, ..., 2, 2 */
		/* pass0                           0, 0, 0, ..., 1, 2 */

		def_label = passn > 1 ? redefine_label : define_label;

		globalbits = sb = cmd_sb;   /* set 'bits' to command line default */
		cpu = cmd_cpu;
		if (pass0 == 2) {
			if (*listname)
				aasmlist.init(listname);
		}
		in_abs_seg = false;
		global_offset_changed = 0;  /* set by redefine_label */
		location.segment = (*ofmt_f())->section(NULL, pass2, &sb);
		globalbits = sb;
		if (passn > 1) {
			StrbuffRewind(forwrefs);
			forwref = (forwrefinfo*)StrbuffEalloc(forwrefs);
			StrpageFree(offsets);
			offsets = StrpageNew();
		}
		preproc->reset(fname, pass1, eval_evaluate, &aasmlist,
			pass1 == 2 ? depend_ptr : NULL);
		MemCopyN(warning_on, warning_on_global, (ERR_WARN_MAX + 1) * sizeof(bool));

		globallineno = 0;
		if (passn == 1)
			location.known = true;
		location.offset = offs = GET_CURR_OFFS;

		while ((line = preproc->getline())) {
			enum directives d;
			globallineno++;

			/*
		 * Here we parse our directives; this is not handled by the
		 * 'real' parser.  This really should be a separate function.
		 */
			directive = line;
			d = getkw(&directive, &value);
			if (d) {
				int err = 0;

				switch (d) {
				case D_SEGMENT:		/* [SEGMENT n] */
				case D_SECTION:
					seg = (*ofmt_f())->section(value, pass2, &sb);
					if (seg == NO_SEG) {
						//--auto clean flag
						aasm_log(pass1 == 1 ? _LOG_ERROR : _LOG_PANIC, "segment name `%s' not recognized", value);
					}
					else {
						in_abs_seg = false;
						location.segment = seg;
					}
					break;
				case D_EXTERN:		/* [EXTERN label:special] */
					if (*value == '$')
						value++;        /* skip initial $ if present */
					if (pass0 == 2) {
						q = value;
						while (*q && *q != ':')
							q++;
						if (*q == ':') {
							*q++ = '\0';
							(*ofmt_f())->symdef(value, 0L, 0L, 3, q);
						}
					}
					else if (passn == 1) {
						q = value;
						validid = true;
						if (!isidstart(*q))
							validid = false;
						while (*q && *q != ':') {
							if (!isidchar(*q))
								validid = false;
							q++;
						}
						if (!validid) {
							//--auto clean flag
							aasm_log(_LOG_ERROR, "identifier expected after EXTERN");
							break;
						}
						if (*q == ':') {
							*q++ = '\0';
							special = q;
						}
						else
							special = NULL;
						if (!is_extern(value)) {        /* allow re-EXTERN to be ignored */
							int temp = pass0;
							pass0 = 1;  /* fake pass 1 in labels.c */
							declare_as_global(value, special
							);
							define_label(value, SegAlloc(), 0L, NULL,
								false, true, (*ofmt_f()));
							pass0 = temp;
						}
					}           /* else  pass0 == 1 */
					break;
				case D_BITS:		/* [BITS bits] */
					globalbits = sb = get_bits(value);
					break;
				case D_GLOBAL:		/* [GLOBAL symbol:special] */
					if (*value == '$')
						value++;        /* skip initial $ if present */
					if (pass0 == 2) {   /* pass 2 */
						q = value;
						while (*q && *q != ':')
							q++;
						if (*q == ':') {
							*q++ = '\0';
							(*ofmt_f())->symdef(value, 0L, 0L, 3, q);
						}
					}
					else if (pass2 == 1) {    /* pass == 1 */
						q = value;
						validid = true;
						if (!isidstart(*q))
							validid = false;
						while (*q && *q != ':') {
							if (!isidchar(*q))
								validid = false;
							q++;
						}
						if (!validid) {
							//--auto clean flag
							aasm_log(_LOG_ERROR, "identifier expected after GLOBAL");
							break;
						}
						if (*q == ':') {
							*q++ = '\0';
							special = q;
						}
						else
							special = NULL;
						declare_as_global(value, special);
					}           /* pass == 1 */
					break;
				case D_COMMON:		/* [COMMON symbol size:special] */
				{
					int64_t size;

					if (*value == '$')
						value++;        /* skip initial $ if present */
					p = value;
					validid = true;
					if (!isidstart(*p))
						validid = false;
					while (*p && !ascii_isspace(*p)) {
						if (!isidchar(*p))
							validid = false;
						p++;
					}
					if (!validid) {
						//--auto clean flag
						aasm_log(_LOG_ERROR, "identifier expected after COMMON");
						break;
					}
					if (*p) {
						while (*p && ascii_isspace(*p))
							*p++ = '\0';
						q = p;
						while (*q && *q != ':')
							q++;
						if (*q == ':') {
							*q++ = '\0';
							special = q;
						}
						else {
							special = NULL;
						}
						size = readnum(p, &rn_error);
						if (rn_error) {
							//--auto clean flag
							aasm_log(_LOG_ERROR, "invalid size specified in COMMON declaration");
							break;
						}
					}
					else {
						//--auto clean flag
						aasm_log(_LOG_ERROR, "no size specified in COMMON declaration");
						break;
					}

					if (pass0 < 2) {
						define_common(value, SegAlloc(), size,
							special, (*ofmt_f()));
					}
					else if (pass0 == 2) {
						if (special)
							(*ofmt_f())->symdef(value, 0L, 0L, 3, special);
					}
					break;
				}
				case D_ABSOLUTE:		/* [ABSOLUTE address] */
					stdscan_reset();
					stdscan_bufptr = value;
					tokval.t_type = TOKEN_INVALID;
					e = eval_evaluate(stdscan, NULL, &tokval, NULL, pass2
						, NULL);
					if (e) {
						if (!is_reloc(e)) {
							//--auto clean flag
							aasm_log(pass0 == 1 ? _LOG_ERROR : _LOG_PANIC, "cannot use non-relocatable expression as ABSOLUTE address");
						}
						else {
							abs_seg = reloc_seg(e);
							abs_offset = reloc_value(e);
						}
					}
					else if (passn == 1)
						abs_offset = 0x100;     /* don't go near zero in case of / */
					else {
						//--auto clean flag
						aasm_log(_LOG_PANIC, "invalid ABSOLUTE address in pass two");
					}
					in_abs_seg = true;
					location.segment = NO_SEG;
					break;
				case D_DEBUG:		/* [DEBUG] */
					p = value;
					q = debugid;
					validid = true;
					if (!isidstart(*p))
						validid = false;
					while (*p && !ascii_isspace(*p)) {
						if (!isidchar(*p))
							validid = false;
						*q++ = *p++;
					}
					*q++ = 0;
					if (!validid) {
						//--auto clean flag
						aasm_log(passn == 1 ? _LOG_ERROR : _LOG_PANIC, "identifier expected after DEBUG");
						break;
					}
					while (*p && ascii_isspace(*p))
						p++;
					if (pass0 == 2)
						(*ofmt_f())->current_dfmt->debug_directive(debugid, p);
					break;
				case D_WARNING:		/* [WARNING {+|-|*}warn-name] */
					while (*value && ascii_isspace(*value))
						value++;

					switch (*value) {
					case '-': validid = 0; value++; break;
					case '+': validid = 1; value++; break;
					case '*': validid = 2; value++; break;
					default:  validid = 1; break;
					}

					for (i = 1; i <= ERR_WARN_MAX; i++)
						if (!StrCompareInsensitive(value, warnings[i].name))
							break;
					if (i <= ERR_WARN_MAX) {
						switch (validid) {
						case 0:
							warning_on[i] = false;
							break;
						case 1:
							warning_on[i] = true;
							break;
						case 2:
							warning_on[i] = warning_on_global[i];
							break;
						}
					}
					else {
						//--auto clean flag
						aasm_log(_LOG_ERROR, "invalid warning id in WARNING directive");
					}
					break;
				case D_CPU:		/* [CPU] */
					cpu = get_cpu(value);
					break;
				case D_LIST:		/* [LIST {+|-}] */
					while (*value && ascii_isspace(*value))
						value++;

					if (*value == '+') {
						user_nolist = 0;
					}
					else {
						if (*value == '-') {
							user_nolist = 1;
						}
						else {
							err = 1;
						}
					}
					break;
				case D_DEFAULT:		/* [DEFAULT] */
					stdscan_reset();
					stdscan_bufptr = value;
					tokval.t_type = TOKEN_INVALID;
					if (stdscan(NULL, &tokval) == TOKEN_SPECIAL) {
						switch ((int)tokval.t_integer) {
						case S_REL:
							globalrel = 1;
							break;
						case S_ABS:
							globalrel = 0;
							break;
						default:
							err = 1;
							break;
						}
					}
					else {
						err = 1;
					}
					break;
				case D_FLOAT:
					if (float_option(value)) {
						//--auto clean flag
						aasm_log(pass1 == 1 ? _LOG_ERROR : _LOG_PANIC, "unknown 'float' directive: %s", value);
					}
					break;
				default:
					if (!(*ofmt_f())->directive(directive, value, pass2)) {
						//--auto clean flag
						aasm_log(pass1 == 1 ? _LOG_ERROR : _LOG_PANIC, "unrecognised directive [%s]", directive);
					}
				}
				if (err) {
					//--auto clean flag
					aasm_log(_LOG_ERROR, "invalid parameter to [%s] directive", directive);
				}
			}
			else {            /* it isn't a directive */
				Parser::parse_line(pass1, line, &output_ins , eval_evaluate, def_label);

				if (optimizing > 0) {
					if (forwref != NULL && globallineno == forwref->lineno) {
						output_ins.forw_ref = true;
						do {
							output_ins.oprs[forwref->operand].opflags |=
								OPFLAG_FORWARD;
							forwref = (forwrefinfo*)StrbuffEread(forwrefs);
						} while (forwref != NULL
							&& forwref->lineno == globallineno);
					}
					else
						output_ins.forw_ref = false;

					if (output_ins.forw_ref) {
						if (passn == 1) {
							for (i = 0; i < output_ins.operands; i++) {
								if (output_ins.oprs[i].
									opflags & OPFLAG_FORWARD) {
									forwrefinfo* fwinf = (forwrefinfo*)StrbuffEalloc(forwrefs);
									fwinf->lineno = globallineno;
									fwinf->operand = i;
								}
							}
						}
					}
				}

				/*  forw_ref */
				if (output_ins.opcode == I_EQU) {
					if (pass1 == 1) {
						/*
						 * Special `..' EQUs get processed in pass two,
						 * except `..@' macro-processor EQUs which are done
						 * in the normal place.
						 */
						if (!output_ins.label) {
							//--auto clean flag
							aasm_log(_LOG_ERROR, "EQU not preceded by label");
						}

						else if (output_ins.label[0] != '.' ||
							output_ins.label[1] != '.' ||
							output_ins.label[2] == '@') {
							if (output_ins.operands == 1 &&
								(output_ins.oprs[0].type & IMMEDIATE) &&
								output_ins.oprs[0].wrt == NO_SEG) {
								bool isext = !!(output_ins.oprs[0].opflags
									& OPFLAG_EXTERN);
								def_label(output_ins.label,
									output_ins.oprs[0].segment,
									output_ins.oprs[0].offset, NULL,
									false, isext, (*ofmt_f()));
							}
							else if (output_ins.operands == 2
								&& (output_ins.oprs[0].type & IMMEDIATE)
								&& (output_ins.oprs[0].type & COLON)
								&& output_ins.oprs[0].segment == NO_SEG
								&& output_ins.oprs[0].wrt == NO_SEG
								&& (output_ins.oprs[1].type & IMMEDIATE)
								&& output_ins.oprs[1].segment == NO_SEG
								&& output_ins.oprs[1].wrt == NO_SEG) {
								def_label(output_ins.label,
									output_ins.oprs[0].offset | SEG_ABS,
									output_ins.oprs[1].offset,
									NULL, false, false, (*ofmt_f()));
							}
							else {
								//--auto clean flag
								aasm_log(_LOG_ERROR, "bad syntax for EQU");
							}
						}
					}
					else {
						/*
						 * Special `..' EQUs get processed here, except
						 * `..@' macro processor EQUs which are done above.
						 */
						if (output_ins.label[0] == '.' &&
							output_ins.label[1] == '.' &&
							output_ins.label[2] != '@') {
							if (output_ins.operands == 1 &&
								(output_ins.oprs[0].type & IMMEDIATE)) {
								define_label(output_ins.label,
									output_ins.oprs[0].segment,
									output_ins.oprs[0].offset,
									NULL, false, false, (*ofmt_f())
								);
							}
							else if (output_ins.operands == 2
								&& (output_ins.oprs[0].
									type & IMMEDIATE)
								&& (output_ins.oprs[0].type & COLON)
								&& output_ins.oprs[0].segment ==
								NO_SEG
								&& (output_ins.oprs[1].
									type & IMMEDIATE)
								&& output_ins.oprs[1].segment ==
								NO_SEG) {
								define_label(output_ins.label,
									output_ins.oprs[0].
									offset | SEG_ABS,
									output_ins.oprs[1].offset,
									NULL, false, false, (*ofmt_f())
								);
							}
							else {
								//--auto clean flag
								aasm_log(_LOG_ERROR, "bad syntax for EQU");
							}
						}
					}
				}
				else {        /* instruction isn't an EQU */

					if (pass1 == 1) {

						int64_t l = insn_size(location.segment, offs, sb, cpu,
							&output_ins);

						/* if (using_debug_info)  && output_ins.opcode != -1) */
						if (using_debug_info)
						{       /* fbk 03/25/01 */
							/* this is done here so we can do debug type info */
							int32_t typeinfo =
								TYS_ELEMENTS(output_ins.operands);
							switch (output_ins.opcode) {
							case I_RESB:
								typeinfo =
									TYS_ELEMENTS(output_ins.oprs[0].
										offset) | TY_BYTE;
								break;
							case I_RESW:
								typeinfo =
									TYS_ELEMENTS(output_ins.oprs[0].
										offset) | TY_WORD;
								break;
							case I_RESD:
								typeinfo =
									TYS_ELEMENTS(output_ins.oprs[0].
										offset) | TY_DWORD;
								break;
							case I_RESQ:
								typeinfo =
									TYS_ELEMENTS(output_ins.oprs[0].
										offset) | TY_QWORD;
								break;
							case I_REST:
								typeinfo =
									TYS_ELEMENTS(output_ins.oprs[0].
										offset) | TY_TBYTE;
								break;
							case I_RESO:
								typeinfo =
									TYS_ELEMENTS(output_ins.oprs[0].
										offset) | TY_OWORD;
								break;
							case I_RESY:
								typeinfo =
									TYS_ELEMENTS(output_ins.oprs[0].
										offset) | TY_YWORD;
								break;
							case I_DB:
								typeinfo |= TY_BYTE;
								break;
							case I_DW:
								typeinfo |= TY_WORD;
								break;
							case I_DD:
								if (output_ins.eops_float)
									typeinfo |= TY_FLOAT;
								else
									typeinfo |= TY_DWORD;
								break;
							case I_DQ:
								typeinfo |= TY_QWORD;
								break;
							case I_DT:
								typeinfo |= TY_TBYTE;
								break;
							case I_DO:
								typeinfo |= TY_OWORD;
								break;
							case I_DY:
								typeinfo |= TY_YWORD;
								break;
							default:
								typeinfo = TY_LABEL;

							}

							(*ofmt_f())->current_dfmt->debug_typevalue(typeinfo);

						}
						if (l != -1) {
							offs += l;
							SET_CURR_OFFS(offs);
						}
						/*
						 * else l == -1 => invalid instruction, which will be
						 * flagged as an error on pass 2
						 */

					}
					else {
						offs += assemble(location.segment, offs, sb, cpu,
							&output_ins, (*ofmt_f()),
							&aasmlist);
						SET_CURR_OFFS(offs);

					}
				} // not an EQU
				Parser::cleanup_insn(&output_ins);
			}
			memf(line);
			location.offset = offs = GET_CURR_OFFS;
		}                       /* end while (line = preproc->getline... */

		if (pass0 == 2 && global_offset_changed && !terminate_after_phase) {
			//--auto clean flag
			aasm_log(_LOG_ERROR, "phase error detected at end of assembly.");
		}

		if (pass1 == 1)
			preproc->cleanup(1);

		if ((passn > 1 && !global_offset_changed) || pass0 == 2) {
			pass0++;
		}
		else if (global_offset_changed &&
			global_offset_changed < prev_offset_changed) {
			prev_offset_changed = global_offset_changed;
			stall_count = 0;
		}
		else {
			stall_count++;
		}

		if (terminate_after_phase)
			break;

		if ((stall_count > 997) || (passn >= pass_max)) {
			/* We get here if the labels don't converge
			 * Example: FOO equ FOO + 1
			 */
			 //--auto clean flag
			aasm_log(_LOG_ERROR, "Can't find valid values for all labels after %d passes, giving up.", passn);
			aasm_log(_LOG_ERROR, "Possible causes: recursive EQUs, macro abuse.");
			break;
		}
	}

	preproc->cleanup(0);
	aasmlist.cleanup();
	if (!terminate_after_phase && opt_verbose_info) {
		/*  -On and -Ov switches */
		fprintf(stdout, "info: assembly required 1+%d+1 passes\n", passn - 3);
	}
}

static enum directives getkw(char** directive, char** value)
{
	char* p, * q, * buf;

	buf = *directive;

	/*  allow leading spaces or tabs */
	while (*buf == ' ' || *buf == '\t')
		buf++;

	if (*buf != '[')
		return D_NONE;

	p = buf;

	while (*p && *p != ']')
		p++;

	if (!*p)
		return D_NONE;

	q = p++;

	while (*p && *p != ';') {
		if (!ascii_isspace(*p))
			return D_NONE;
		p++;
	}
	q[1] = '\0';

	*directive = p = buf + 1;
	while (*buf && *buf != ' ' && *buf != ']' && *buf != '\t')
		buf++;
	if (*buf == ']') {
		*buf = '\0';
		*value = buf;
	}
	else {
		*buf++ = '\0';
		while (ascii_isspace(*buf))
			buf++;              /* beppu - skip leading whitespace */
		*value = buf;
		while (*buf != ']')
			buf++;
		*buf++ = '\0';
	}

	return (enum directives)bsii(*directive, directives, numsof(directives));
}



#define BUF_DELTA 512

static FILE* no_pp_fp;
// static efunc no_pp_err;
static ListGen* no_pp_list;
static int32_t no_pp_lineinc;



uint32 get_cpu_id(rostr iden);
static uint32_t get_cpu(char* value)
{
	uint32 tmp = get_cpu_id(value);
	if (tmp == IF_PLEVEL) {
		//--auto clean flag
		aasm_log(pass0 < 2 ? _LOG_ERROR : _LOG_FATAL, "unknown 'cpu' type");
	}
	return tmp;
}

static int get_bits(char* value)
{
	int i;

	if ((i = atoins(value)) == 16)
		return i;               /* set for a 16-bit segment */
	else if (i == 32) {
		if (cpu < IF_386) {
			//--auto clean flag
			aasm_log(_LOG_ERROR, "cannot specify 32-bit segment on processor below a 386");
			i = 16;
		}
	}
	else if (i == 64) {
		if (cpu < IF_X86_64) {
			//--auto clean flag
			aasm_log(_LOG_ERROR, "cannot specify 64-bit segment on processor below an x86-64");
			i = 16;
		}
		if (i != maxbits) {
			//--auto clean flag
			aasm_log(_LOG_ERROR, "%s output format does not support 64-bit code", (*ofmt_f())->shortname);
			i = 16;
		}
	}
	else {
		//--auto clean flag
		aasm_log(pass0 < 2 ? _LOG_ERROR : _LOG_FATAL, "`%s' is not a valid segment size; must be 16, 32 or 64", value);
		i = 16;
	}
	return i;
}

/* end of nasm.c */
_ESYM_C
void no_pp_reset(char* file, int pass, evalfunc eval, ListGen* listgen, StrList** deplist)
{
	src_set_fname(StrHeap(file));
	src_set_linnum(0);
	no_pp_lineinc = 1;
	no_pp_fp = fopen(file, "r");
	if (!no_pp_fp) {
		//--auto clean flag
		log_0file = 1;
		aasm_log(_LOG_FATAL, "unable to open input file `%s'", file);//:no_pp_err
	}
	no_pp_list = listgen;
	(void)pass;                 /* placate compilers */
	(void)eval;                 /* placate compilers */

	if (deplist) {
		StrList* sl = (StrList*)malc(StrLength(file) + 1 + sizeof sl->next);
		sl->next = NULL;
		StrCopy(sl->str, file);
		*deplist = sl;
	}
}
_ESYM_C
char* no_pp_getline(void)
{
	char* buffer, * p, * q;
	int bufsize;

	bufsize = BUF_DELTA;
	buffer = (char*)malc(BUF_DELTA);
	src_set_linnum(src_get_linnum() + no_pp_lineinc);

	while (1) {                 /* Loop to handle %line */

		p = buffer;
		while (1) {             /* Loop to handle long lines */
			q = fgets(p, bufsize - (p - buffer), no_pp_fp);
			if (!q)
				break;
			p += StrLength(p);
			if (p > buffer && p[-1] == '\n')
				break;
			if (p - buffer > bufsize - 10) {
				int offset;
				offset = p - buffer;
				bufsize += BUF_DELTA;
				buffer = (char*)aasm_realloc(buffer, bufsize);
				p = buffer + offset;
			}
		}

		if (!q && p == buffer) {
			memf(buffer);
			return NULL;
		}

		/*
		 * Play safe: remove CRs, LFs and any spurious ^Zs, if any of
		 * them are present at the end of the line.
		 */
		buffer[StrSpanExclude(buffer, "\r\n\032")] = '\0';

		if (!StrCompareNInsensitive(buffer, "%line", 5)) {
			int32_t ln;
			int li;
			char* nm = (char*)malc(StrLength(buffer));
			if (sscanf(buffer + 5, "%" PRId32 "+%d %s", &ln, &li, nm) == 3) {
				memf(src_set_fname(nm));
				src_set_linnum(ln);
				no_pp_lineinc = li;
				continue;
			}
			memf(nm);
		}
		break;
	}

	no_pp_list->line(LIST_READ, buffer);

	return buffer;
}
_ESYM_C
void no_pp_cleanup(int pass)
{
	(void)pass;// placate GCC
	fclose(no_pp_fp);
}



int drop() {
	if (want_usage) usage();
#ifdef _DEBUG
	 //printinfo();
#endif
	StrpageFree(offsets);
	StrbuffFree(forwrefs);
	//{TODO} eval_cleanup();
	//{TODO} stdscan_cleanup();
	return malc_count;
}
