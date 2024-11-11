#include <stdinc.h>
#include <ustdbool.h>
#include <../../demo/template/version/version.h>
#include <time.h>
#include <stdio.h>

static time_t temp_time;
time_t startup_time;

extern bool terminate_after_phase;
extern bool treat_warn_as_erro;
extern bool enable_warning;

extern FILE* outfile;
extern char  outname;

// NASM-style warning
const struct warning {
	rostr name;
	rostr help;
	bool enabled;
} warnings[] = {
	{"error",           "treat warnings as errors", false},
	{"macro-params",    "macro calls with wrong parameter count", true},
	{"macro-selfref",   "cyclic macro references", false},
	{"macro-defaults",  "macros with more default than optional parameters", true},
	{"orphan-labels",   "labels alone on lines without trailing `:'", true},
	{"number-overflow", "numeric constant does not fit", true},
	{"float-overflow",  "floating point overflow", true},
	{"float-denorm",    "floating point denormal", false},
	{"float-underflow", "floating point underflow", false},
	{"float-toolong",   "too many digits in floating-point number", true},
	{"user",            "%warning directives", true},
	{"gnu-elf-extensions", "using 8- or 16-bit relocation in ELF32, a GNU extension", false},
};

void getcrt(_Need_free char** const pname, stduint* const plineno);

void printinfo(void) {
	time(&temp_time);
	printf("%s", __PROJ_INFO__);
	printf("Timspan: %" PRIuSTD "\n", temp_time - startup_time);
}

void printl(loglevel_t level, const char* fmt, ...)
{
	Letpara(paras, fmt);
	if (_LOG_WARN == level && !enable_warning)
		return;
	char* crtfile = NULL;
	stduint lineno = 0;
	getcrt(&crtfile, &lineno);
	switch (_logstyle) {
	case _LOG_STYLE_GCC:
		printf("%s(%" PRIuSTD ") : ", crtfile, lineno);
		break;
	case _LOG_STYLE_MSVC:
		printf("%s:%" PRIuSTD ": ", crtfile, lineno);
		break;
	default: return;
	}
	printlogx(level, fmt, paras);
	memf(crtfile);
}

int drop();
int* handlog(void* _serious, ...)
{
	Letvar(serious, loglevel_t, _serious);
	switch (serious)
	{
	case _LOG_WARN:
		if (treat_warn_as_erro)
	case _LOG_ERROR:
		terminate_after_phase = true;
	default: break;
	case _LOG_FATAL:
		if (outfile) {
			fclose(outfile);
			remove(outname);
		}
	case _LOG_PANIC:
		puts("----\t----\tAASM panics!\t----\t----");
		fflush(NULL);
		exit(drop());
	}
	return NULL;
}