// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant CPU(80586+)
// LastCheck: RFZ26
// AllAuthor: @dosconio
// ModuTitle: Debug Aide
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

#include "../../inc/c/stdinc.h"
#include "../../inc/c/supple.h"
#include "../../inc/c/consio.h"

char* salc(size_t size)
{
	if (!size) return 0;
	char* r = (char*)malc(size);
	r[size - 1] = 0;
	return r;
}

#ifdef _DEBUG

logstyle_t _logstyle = _LOG_STYLE_NONE;

const char* _pref_fata = "[FATAL]";
const char* _pref_pani = "[PANIC]";
const char* _pref_erro = "[ERROR]";
const char* _pref_warn = "[WARNS]";
const char* _pref_info = "[INFOR]";
const char* _pref_dbug = "[DEBUG]";
const char* _pref_trac = "[TRACE]";

void printlogx(loglevel_t level, const char* fmt, para_list paras)
{
#if defined(_Linux) || 1
	//{TODO} _logstyle
	//{TODEL} Letpara(paras, fmt);
	switch (level)
	{
	case _LOG_STDOUT:
		outsfmtlst(fmt, paras); outs("\n"); return;
	case _LOG_FATAL:// better into STDERR
		outsfmt("\x1b[%dm%s", CON_FORE_RED, _pref_fata); break;
	case _LOG_PANIC:// better into STDERR
		outsfmt("\x1b[%dm%s", CON_FORE_RED, _pref_pani); break;
	case _LOG_ERROR:// better into STDERR
		outsfmt("\x1b[%dm%s", CON_FORE_RED, _pref_erro); break;
	case _LOG_WARN:
		outsfmt("\x1b[%dm%s", CON_FORE_YELLOW, _pref_warn); break;
	case _LOG_INFO:
		outsfmt("\x1b[%dm%s", CON_FORE_BLUE, _pref_info); break;
	case _LOG_DEBUG:
		outsfmt("\x1b[%dm%s", CON_FORE_GREEN, _pref_dbug); break;
	case _LOG_TRACE:
		outsfmt("\x1b[%dm%s", CON_FORE_GRAY, _pref_trac); break;
	default:
		return;
	}
	outsfmtlst(fmt, paras);
	outs("\x1b[0m\n");
#endif
}

void printlog(loglevel_t level, const char* fmt, ...)
{
	Letpara(paras, fmt);
	printlogx(level, fmt, paras);
}

#endif
