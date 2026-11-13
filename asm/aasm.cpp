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

#include <c/stdinc.h>
#include <c/ustring.h>
#include <cpp/string>

#include <time.h>
#include <c/datime.h>
#include "inc/aasm.h"
#include "../inc/cpp/Format/Code.hpp"
#include "../inc/cpp/Lango/Archend.hpp"

void AssemblyTarget::Initialize(uni::lango::ArchendInterface& archend,
	uni::CodeendInterface& codeend)
{
	SelectArchend(archend);
	SelectCodeend(codeend);
}

void AssemblyTarget::SelectArchend(uni::lango::ArchendInterface& archend)
{
	ArchendDriver = &archend;
	DefaultBitWidth = archend.DumpArchend().default_bits;
	CpuLevel = archend.DefaultCpu();
}

void AssemblyTarget::SelectCodeend(uni::CodeendInterface& codeend)
{
	CodeendDriver = &codeend;
}

void AssemblyTarget::SelectCpu(uint32_t cpu)
{
	CpuLevel = cpu;
}

uni::lango::ArchendInterface& AssemblyTarget::Archend() const
{
	return *ArchendDriver;
}

uni::CodeendInterface& AssemblyTarget::Codeend() const
{
	return *CodeendDriver;
}

int AssemblyTarget::DefaultBits() const
{
	return DefaultBitWidth;
}

uint32_t AssemblyTarget::Cpu() const
{
	return CpuLevel;
}

bool AssemblyTarget::IsValid() const
{
	return ArchendDriver && CodeendDriver &&
		ArchendDriver->ValidateBits(DefaultBitWidth, CpuLevel);
}

bool AssemblyTarget::IsCodeendCompatible() const
{
	return ArchendDriver && CodeendDriver &&
		CodeendDriver->SupportsArchitecture(
			ArchendDriver->DumpArchend().architecture);
}


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

FILE* outfile = NULL;
time_t official_compile_time;


// ---- maybe suppressible warnings ----
// Entry zero isn't an actual warning, but it used for -w+error/-Werror.
//TODO: <Bitmap>
bool warning_on[ERR_WARN_MAX + 1]; /* Current state */
bool warning_on_global[ERR_WARN_MAX + 1]; /* Command-line state */

extern const struct warning warnings[ERR_WARN_MAX + 1];
extern int opt_verbose_info;
bool stopoptions = false;
Preproc* preproc;
bool tasm_compatible_mode = false;
int pass0, passn;
int maxbits = 0;
int globalrel = 0;
int optimizing = -1;            /* number of optimization passes to take */
int64_t global_offset_changed;      /* referenced in labels.c */
int in_abs_seg;                 /* Flag we are in ABSOLUTE seg */
int32_t abs_seg;                   /* ABSOLUTE segment basis */
int32_t abs_offset;                /* ABSOLUTE offset */
//bool want_usage;
int user_nolist = 0;            /* fbk 9/2/00 */

static AssemblerOptions assembler_options;
static AssemblySession assembly_session;

}

_ESYM_C void getcrt(_Need_free char** const pname, stduint* const plineno)
{
	*plineno = assembly_session.GlobalLineNumber;
	*pname = assembler_options.InputName;
}

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
void ProcessResponseFile(AssemblerOptions& options, FILE* rfile)
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
				ProcessArgument(options, prevarg, NULL);
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

		if (ProcessArgument(options, prevarg, p))
			*p = '\0';

		if ((int)StrLength(p) > prevargsize - 10) {
			prevargsize += ARG_BUF_DELTA;
			prevarg = (char*)aasm_realloc(prevarg, prevargsize);
		}
		StrCopyN(prevarg, p, prevargsize);
	}
}
_ESYM_C void exit(int);
void ProcessResponsePath(AssemblerOptions& options, const char* file)
{
	char str[2048];
	FILE* f = fopen(file, "r");
	if (!f) {
		perror(file);
		exit(-1);
	}
	while (fgets(str, sizeof str, f)) {
		ProcessArguments(options, str);
	}
	fclose(f);
}

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

static void InitializeAssemblerOptions(AssemblerOptions& options)
{
	options = AssemblerOptions{};
	options.Mode = AssemblyMode::Assemble;
	options.OptimizationLevel = -1;
	options.Preprocessor = &nasmpp;
	options.Target.Initialize(
		uni::lango::DefaultArchend(), uni::DefaultCodeend());
}

static void SyncLegacyOptions(const AssemblerOptions& options)
{
	StrCopyN(inname, options.InputName, FILENAME_MAX);
	StrCopyN(outname, options.OutputName, FILENAME_MAX);
	StrCopyN(listname, options.ListName, FILENAME_MAX);
	StrCopyN(errname, options.ErrorName, FILENAME_MAX);
	optimizing = options.OptimizationLevel;
	using_debug_info = options.EnableDebugInfo;
	tasm_compatible_mode = options.TasmCompatible;
	stopoptions = options.StopOptionParsing;
	depend_emit_phony = options.EmitPhonyDependencies;
	depend_missing_ok = options.AllowMissingDependencies;
	depend_target = options.DependencyTarget;
	depend_file = options.DependencyFile;
	preproc = options.Preprocessor;
	uni::SelectCodeend(options.Target.Codeend());
	MemCopyN(warning_on_global, options.WarningEnabled,
		(ERR_WARN_MAX + 1) * sizeof(bool));
}

static void InitializeAssemblySession(AssemblySession& session,
	AssemblerOptions& options)
{
	session = AssemblySession{};
	session.Options = &options;
	session.ActiveArchend = &options.Target.Archend();
	session.Listing = &aasmlist;
	session.CurrentBits = options.Target.DefaultBits();
	session.SectionBits = options.Target.DefaultBits();
	session.CurrentCpu = options.Target.Cpu();
	session.CurrentAbi = session.ActiveArchend->DefaultAbi(
		session.CurrentBits, session.CurrentCpu);
	session.AbiExplicit = false;
	session.PassPhase = 0;
	session.PassNumber = 0;
	session.Location.known = false;
}

int main(int argc, char** argv) {
	time(&startup_time);
	error_file = stderr;
	_call_serious = handlog;
	_logstyle = _LOG_STYLE_GCC;
	_f_printlog = printlog0;
	//
	SegInit();
	pass0 = 0;
	mainx();
	OutfileFormat::setMode();
	InitializeAssemblerOptions(assembler_options);
	// return drop();
	// : contain 5 leaks
	define_macros_early(&startup_time);


	ParseCommandLine(assembler_options, argc, argv);
	if (!assembler_options.Target.IsValid()) {
		aasm_log(_LOG_FATAL,
			"selected architecture has an invalid default target");
	}
	if (!assembler_options.Target.IsCodeendCompatible()) {
		aasm_log(_LOG_FATAL,
			"output format `%s' does not support architecture `%s'",
			assembler_options.Target.Codeend().DumpCodeend().shortname,
			assembler_options.Target.Archend().DumpArchend().shortname);
	}
	SyncLegacyOptions(assembler_options);
	InitializeAssemblySession(assembly_session, assembler_options);
	Preprocessor::SetAssemblySession(&assembly_session);
	if (terminate_after_phase) {
		if (want_usage)
			usage();
		want_usage = 0;
		drop();
		return 1;
	}

	uni::CodeendInterface& codeend = assembler_options.Target.Codeend();
	if (!using_debug_info)// If debugging info is disabled, suppress any debug calls
		codeend.DisableDebug();

	if (codeend.StandardMacros())
		Preprocessor::extra_stdmac(codeend.StandardMacros());
	EvalGlobalInfo(codeend, Labelsys::Lookup, &assembly_session.Location);

	define_macros_late(codeend.DumpCodeend().shortname);

	StrList** depend_ptr;
	depend_ptr = (assembler_options.DependencyFile ||
		(assembler_options.Mode == AssemblyMode::Dependency))
		? &assembly_session.Dependencies : NULL;
	if (!depend_target)
		depend_target = outname;

	switch (assembler_options.Mode) {
	case AssemblyMode::Dependency:
	{
		char* line;

		if (depend_missing_ok)
			pp_include_path(NULL);	/* "assume generated" */

		preproc->reset(inname, 0, eval_evaluate, &aasmlist.Legacy(),
			depend_ptr);
		if (outname[0] == '\0')
			codeend.PrepareFilename(inname, outname);
		outfile = NULL;
		while ((line = preproc->getline()))
			memf(line);
		preproc->cleanup(0);
	}
	break;

	case AssemblyMode::Preprocess:
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

		assembly_session.Location.known = false;

		/* pass = 1; */
		preproc->reset(inname, 3, eval_evaluate, &aasmlist.Legacy(),
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

	case AssemblyMode::Assemble:
	{
		codeend.PrepareFilename(inname, outname);

		bool text_output =
			(codeend.DumpCodeend().flags & uni::CodeendFlag::Text) !=
			uni::CodeendFlag::None;
		outfile = fopen(outname, text_output ? "w" : "wb");
		assembly_session.OutputFile = outfile;
		if (!outfile) {
			//--auto clean flag
			log_0file = 1;
			aasm_log(_LOG_FATAL, "unable to open output file `%s'", outname);
		}

		// Some formats define labels during initialization.
		Labelsys::Initialize();

		codeend.Initialize(outfile,
			assembler_options.Target.Archend().DumpArchend().architecture);
		codeend.InitializeDebug(outfile);

		Assembler assembler(assembly_session);
		assembler.AssembleFile(inname, depend_ptr);

		if (!terminate_after_phase) {
			codeend.Cleanup(using_debug_info);
			Labelsys::Cleanup();
		}
		else {
			// Codeend cleanup normally owns the file closure.
			fclose(outfile);

			remove(outname);
		}
	}
	break;
	}

	if (assembly_session.Dependencies && !terminate_after_phase)
		emit_dependencies(assembly_session.Dependencies);
	assembly_session.TerminateAfterPhase = terminate_after_phase;
	eval_cleanup();
	stdscan_cleanup();
	//{} {} {} return terminate_after_phase;
	//return drop() _TEMP - 5 - 3;
	// return drop() & 0;
	return terminate_after_phase;
}


#define BUF_DELTA 512

static FILE* no_pp_fp;
// static efunc no_pp_err;
static ListGen* no_pp_list;
static int32_t no_pp_lineinc;



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
	assembly_session.Offsets.Clear();
	//{TODO} eval_cleanup();
	//{TODO} stdscan_cleanup();
	return malc_count;
}
