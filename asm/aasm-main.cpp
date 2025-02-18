#define _HIS_TIME_H
#include <c/stdinc.h>
#include <c/datime.h>
#include <c/consio.h>

#include <time.h>

_ESYM_C{
	void pp_pre_define(char* definition);
}

//{TODO} UNI DATIME
extern "C" void define_macros_early(time_t* startup_time)
{
	char temp[128];
	tm lt, * lt_p, gm, * gm_p;
	int64_t posix_time = 0;

	if (lt_p = localtime(startup_time)) {
		lt = *lt_p;

		strftime(temp, sizeof temp, "__DATE__=\"%Y-%m-%d\"", &lt);
		pp_pre_define(temp);
		strftime(temp, sizeof temp, "__DATE_NUM__=%Y%m%d", &lt);
		pp_pre_define(temp);
		strftime(temp, sizeof temp, "__TIME__=\"%H:%M:%S\"", &lt);
		pp_pre_define(temp);
		strftime(temp, sizeof temp, "__TIME_NUM__=%H%M%S", &lt);
		pp_pre_define(temp);
	}

	if (gm_p = gmtime(startup_time)) {
		gm = *gm_p;

		strftime(temp, sizeof temp, "__UTC_DATE__=\"%Y-%m-%d\"", &gm);
		pp_pre_define(temp);
		strftime(temp, sizeof temp, "__UTC_DATE_NUM__=%Y%m%d", &gm);
		pp_pre_define(temp);
		strftime(temp, sizeof temp, "__UTC_TIME__=\"%H:%M:%S\"", &gm);
		pp_pre_define(temp);
		strftime(temp, sizeof temp, "__UTC_TIME_NUM__=%H%M%S", &gm);
		pp_pre_define(temp);
	}

	if (gm_p)
		posix_time = POSIXGetSeconds(&gm);
	else if (lt_p)
		posix_time = POSIXGetSeconds(&lt);

	if (posix_time) {
		pp_pre_define(uni::String::newFormat("__POSIX_TIME__=%[64I]", posix_time).reflect());
	}
}

extern "C" void define_macros_late(rostr ofmt_shortname)
{
	pp_pre_define(uni::String::newFormat("__OUTPUT_FORMAT__=%s\n", ofmt_shortname).reflect());
}


