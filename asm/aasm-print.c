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
		fflush(NULL);
		exit(serious);
	}
	return NULL;
}