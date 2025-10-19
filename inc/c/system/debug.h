// ASCII CPL TAB4 CRLF
// Docutitle: (Module) Debug
// Codifiers: @dosconio: 20240424 ~ <Last-check> 
// Attribute: <ArnCovenant> Any-Architect <Environment> <Reference/Dependence>
// Copyright: UNISYM, under Apache License 2.0
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


#ifdef _DEV_MSVC
#pragma warning(disable:6011)// for MSVC
#endif

#if !defined(_LIB_DEBUG) && defined(_DEBUG)// Add this can cheat the compiler
#define _LIB_DEBUG// Her Convenient odd style. Maybe a bad habit for formal project.

// ---- { memory } ----

#define malc_count _MALCOUNT
#define malc_limit _MALLIMIT
extern size_t _MALCOUNT;// <OUT>
extern size_t _MALLIMIT;// <IN>
#define _MALLIMIT_DEFAULT 0x1000

// #define jump goto // to see asm-inst
#define idle() {_jump: goto _jump;}

#define chars_stack(x) &(char[]){#x "\0"}// chars_stack(123)
#define chstk chars_stack

// ---- { logging } ----

extern void erro(char* erromsg
#ifdef _INC_CPP
	= NULL
#endif
);
extern void warn(char* warnmsg);
extern size_t malc_occupy;
extern size_t arna_precise;

typedef enum {
	_LOG_STDOUT = 0,
	_LOG_FATAL,// instantly fatal: exit with error code
	_LOG_PANIC,// internal error: panic instantly and dump core for reference
	_LOG_ERROR,// usually terminate after current phase
	_LOG_WARN,
	_LOG_INFO,
	_LOG_DEBUG,
	_LOG_TRACE,
} loglevel_t;

typedef enum {
	_LOG_STYLE_MAGICE = 0,
	_LOG_STYLE_NONE,
	_LOG_STYLE_GCC,
	_LOG_STYLE_MSVC,
} logstyle_t;

extern logstyle_t _logstyle;

extern const char* _pref_fata;
extern const char* _pref_pani;
extern const char* _pref_erro;
extern const char* _pref_warn;
extern const char* _pref_info;
extern const char* _pref_dbug;
extern const char* _pref_trac;
extern const char** _tab_pref[];
extern _tocall_ft _call_serious;// if error, panic or fatal
extern _tocall_ft _befo_logging;// after prefix, before context

void printlogx(loglevel_t level, const char* fmt, para_list paras);
typedef void (*_printlog_t)(loglevel_t level, const char* fmt, ...);
void printlog(loglevel_t level, const char* fmt, ...);
#define printlnf(a, ...) printlog(_LOG_STDOUT, a, ##__VA_ARGS__)

#define ploginfo(...) printlog(_LOG_INFO, __VA_ARGS__)
#define plogwarn(...) printlog(_LOG_WARN, __VA_ARGS__)
#define plogerro(...) printlog(_LOG_ERROR, __VA_ARGS__)
#define plogtrac(...) printlog(_LOG_TRACE, __VA_ARGS__)

#endif

