// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant yo Host
// LastCheck: 2023 Nov 16
// AllAuthor: @ArinaMgk since RFR30; @dosconio
// ModuTitle: Console Input and Output
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


#ifndef _INC_CONSIO
#define _INC_CONSIO

#include "stdinc.h"
#include "ustring.h"
#include "ustdbool.h"
#include <stdarg.h>

enum _STD_SRC_t {
	_STD_INN = 0,
	_STD_OUT = 1,
	_STD_ERR = 2,
};

#include "graphic/color-consio.h"

#ifdef _INC_CPP
extern "C" {
#endif


// [HOST-DEF]
void curset(word posi);
word curget(void);
void scrrol(word lines);
void outtxt(const char* str, stduint len);

// [INNER-USE]
#define outs(a) outtxt(a, StrLength(a))
void outc(const char chr);

extern stduint _crt_out_cnt;

#ifdef _MCCA// the pure uint64 input will become 0 in the function
bool outinteger(stduint val, int base, bool sign_show, bool sign_have, byte least_digits, bool zero_padding, byte bytexpo, outbyte_t out);
#else
bool outinteger(uint64 val, int base, bool sign_show, bool sign_have, byte least_digits, bool zero_padding, byte bytexpo, outbyte_t out);
#endif

// User Use
int  outsfmtlst(const char* fmt, para_list lst);
int  outsfmt(const char* fmt, ...);
outbyte_t outredirect(outbyte_t out);
extern Handler_t _serial_callback;

#define printline(...) puts(__VA_ARGS__)


//
void ConClearScreen(void);
#define ConClear ConClearScreen

#if defined(_WinNT) || defined(_Linux)
#include <stdio.h>
#elif defined(_MCCA) // && _MCCA==0x8632 ...
// use \n\r for newline
#endif

// ---- ---- ---- ----

// Return the length of the words excluding terminating zero but "limit" considers it.
size_t ConScanLine(char* buf, size_t limit);

void ConCursorMoveUp(unsigned short dif);
void ConCursorMoveDown(unsigned short dif);
void ConCursorMoveRight(unsigned short dif);
void ConCursorMoveLeft(unsigned short dif);

//
#if defined(_Linux)
static inline void ConCursorReset(void)
{
	printf("\033[H");
}
#endif

void ConCursor(unsigned short col, unsigned short row);

//
#if defined(_Linux)
static inline void ConCursorShow(void)
{
	printf("\033[?25h");
}
#endif


//
#if defined(_Linux)
static inline void ConCursorHide(void)
{
	printf("\033[?25l");
}
#endif

// The style is for the brush
void ConStyleAbnormal(void);
// The style is for the brush
void ConStyleNormal(void);

// ConCurrentWorkingDirectory
#ifdef _WinNT
	#include <direct.h>
	#define ConGetCurrentDirectory _getcwd
	#define ConSetCurrentDirectory _chdir// return 0 if success
#elif defined(_Linux)
	#include <unistd.h>
	#define ConGetCurrentDirectory getcwd
	#define ConSetCurrentDirectory chdir
#endif

#ifdef _AUTO_INCLUDE
	#include "../../lib/c/consio.c"
#endif // _AUTO_INCLUDE


#ifdef _INC_CPP
}

#include "../cpp/stream"
#include "../cpp/string"
#include "../c/graphic.h"

namespace uni {
	// Command Line Interface
	class Console_t : public OstreamTrait, public IstreamTrait
	{
	public:
		// print string with format at cursor
		// C Style printf
		// // virtual int FormatShow(const char* fmt, ...) = 0;
		// [Update] Please Use Console.OutFormat but Console.OutFormat
	};
	
	class HostConsole
	#if 1 // !defined(_MCCA)
		: public Console_t // single instance
	#else
		#define virtual
	#endif
	{
	public:
		virtual int out(const char* str, stduint len);
		virtual int inn();
	public:
		Point getCursor();
		stduint getWidth();
		stduint getHeight();
	#if defined(_STYLE_CSHARP)
		// C#: return void but not here
		int WriteLine(const char* fmt = "", ...);
		int WriteLine(const String& str);
	#endif
	#if 0 && !defined(_MCCA)
		#undef virtual
	#endif
	#if defined(_WinNT) || defined(_Linux)
		void Clear() {
			//{TODO} buffers for
			ConClearScreen();
		}
	#endif
	};
#if defined(_WinNT) || defined(_Linux) || defined(_MCCA)
	extern HostConsole Console;
#endif
}

#endif
#endif
