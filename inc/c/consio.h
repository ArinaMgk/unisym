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
#include <stdarg.h>

enum _STD_SRC_t{
	_STD_INN = 0,
	_STD_OUT = 1,
	_STD_ERR = 2,
};

/*//{TODO}
struct ConsoleControlBlock {
	word posi;
	word width, height;
};
*/

typedef struct {
	struct {
		byte Fore_B : 1;// blue
		byte Fore_G : 1;// green
		byte Fore_R : 1;// red
		byte Highlight : 1;
		byte Back_B : 1;// blue
		byte Back_G : 1;// green
		byte Back_R : 1;// red
		byte Kirablink : 1;
	};
	byte Bold;
	byte Underline;
} ConsoleCharPropertyColor8;

enum CON_FORECOLOR {
#if 0
#elif defined(_Linux) || 1 // VT100
	CON_FORE_DEFAULT = 39,
	CON_FORE_DARK = 30,
	CON_FORE_RED = 31,
	CON_FORE_GREEN = 32,
	CON_FORE_YELLOW = 33,
	CON_FORE_BLUE = 34,
	CON_FORE_MAGENTA = 35,
	CON_FORE_CYAN = 36,
	CON_FORE_GRAY = 37,
	CON_FORE_GRAY_LIGHT = 90,
	CON_FORE_RED_LIGHT = 91,
	CON_FORE_GREEN_LIGHT = 92,
	CON_FORE_YELLOW_LIGHT = 93,
	CON_FORE_BLUE_LIGHT = 94,
	CON_FORE_MAGENTA_LIGHT = 95,
	CON_FORE_CYAN_LIGHT = 96,
	CON_FORE_WHITE = 97,
#endif
};
enum CON_BACKCOLOR {
#if 0
#elif defined(_Linux) || 1 // VT100
	CON_BACK_DEFAULT = CON_FORE_DEFAULT + 10,
	CON_BACK_DARK    = CON_FORE_DARK    + 10,
	CON_BACK_RED     = CON_FORE_RED     + 10,
	CON_BACK_GREEN   = CON_FORE_GREEN   + 10,
	CON_BACK_YELLOW  = CON_FORE_YELLOW  + 10,
	CON_BACK_BLUE    = CON_FORE_BLUE    + 10,
	CON_BACK_MAGENTA = CON_FORE_MAGENTA + 10,
	CON_BACK_CYAN    = CON_FORE_CYAN    + 10,
	CON_BACK_GRAY    = CON_FORE_GRAY    + 10,
	CON_BACK_GRAY_LIGHT    = CON_FORE_GRAY_LIGHT    + 10,
	CON_BACK_RED_LIGHT     = CON_FORE_RED_LIGHT     + 10,
	CON_BACK_GREEN_LIGHT   = CON_FORE_GREEN_LIGHT   + 10,
	CON_BACK_YELLOW_LIGHT  = CON_FORE_YELLOW_LIGHT  + 10,
	CON_BACK_BLUE_LIGHT    = CON_FORE_BLUE_LIGHT    + 10,
	CON_BACK_MAGENTA_LIGHT = CON_FORE_MAGENTA_LIGHT + 10,
	CON_BACK_CYAN_LIGHT    = CON_FORE_CYAN_LIGHT    + 10,
	CON_BACK_WHITE         = CON_FORE_WHITE         + 10,
#endif
};


void curset(word posi);
word curget(void);
void scrrol(word lines);
void outtxt(const char *str, dword len);
#define outs(a) outtxt(a, ~(dword)0)
void outc(const char chr);

void outi8hex(const byte inp);
void outi16hex(const word inp);
void outi32hex(const dword inp);
void outi64hex(const uint64 inp);
void outidec(int xx, int base, int sign);
void outi(stdint val, int base, int sign_show);
void outu(stduint val, int base);
int  outsfmtlst(const char* fmt, para_list lst);
int  outsfmt(const char* fmt, ...);

#define printline(...) puts(__VA_ARGS__)


//
void ConClearScreen(void);
#define ConClear ConClearScreen

#if defined(_WinNT) || defined(_Linux)
#include <stdio.h>
#elif defined(_MCCA) // && _MCCA==0x8632 ...
//
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

#endif