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
// MSVC Style {filename(lineno) : error: ...}
// GCC  Style {filename:lineno: error: ...}
// This process `error` only. User should print `filename` and `lineno` before calling.

const char* _pref_fata = "[FATAL]";
const char* _pref_pani = "[PANIC]";
const char* _pref_erro = "[ERROR]";
const char* _pref_warn = "[WARNS]";
const char* _pref_info = "[INFOR]";
const char* _pref_dbug = "[DEBUG]";
const char* _pref_trac = "[TRACE]";
_tocall_ft _call_serious;
_tocall_ft _befo_logging;

static void printpref(loglevel_t level) {
#if defined(_Linux) || 1
	switch (level)
	{
	case _LOG_FATAL:// better into STDERR
		outsfmt("\x1b[%dm%s", CON_FORE_RED, _logstyle == _LOG_STYLE_NONE ? _pref_fata : "fatal: ");
		break;
	case _LOG_PANIC:// better into STDERR
		outsfmt("\x1b[%dm%s", CON_FORE_RED, _logstyle == _LOG_STYLE_NONE ? _pref_pani : "panic: ");
		break;
	case _LOG_ERROR:// better into STDERR
		outsfmt("\x1b[%dm%s", CON_FORE_RED, _logstyle == _LOG_STYLE_NONE ? _pref_erro : "error: ");
		break;
	case _LOG_WARN:
		outsfmt("\x1b[%dm%s", CON_FORE_YELLOW, _logstyle == _LOG_STYLE_NONE ? _pref_warn : "warning: "); break;
	case _LOG_INFO:
		outsfmt("\x1b[%dm%s", CON_FORE_BLUE, _logstyle == _LOG_STYLE_NONE ? _pref_info : "info: "); break;
	case _LOG_DEBUG:
		outsfmt("\x1b[%dm%s", CON_FORE_GREEN, _logstyle == _LOG_STYLE_NONE ? _pref_dbug : "debug: "); break;
	case _LOG_TRACE:
		outsfmt("\x1b[%dm%s", CON_FORE_GRAY, _logstyle == _LOG_STYLE_NONE ? _pref_trac : "trace: "); break;
	case _LOG_STDOUT:
	default:
		break;
	}
#endif
}

static void printsuff() {
#if defined(_Linux) || 1
	outs("\x1b[0m\n");
#endif
}

void printlogx(loglevel_t level, const char* fmt, para_list paras)
{
#if defined(_Linux) || 1
	printpref(level);
	asserv(_befo_logging) ((void*)level);
	outsfmtlst(fmt, paras);
	printsuff();
#endif
	if (level <= _LOG_WARN) // assume low level is more serious
		asserv(_call_serious)((void*)level);
}

void printlog(loglevel_t level, const char* fmt, ...)
{
	Letpara(paras, fmt);
	printlogx(level, fmt, paras);
}

#endif
