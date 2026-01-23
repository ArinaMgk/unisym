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

extern stduint _crt_out_cnt, _crt_out_lim;

bool outinteger(uint64 val, int base, bool sign_show, bool sign_have, byte least_digits, bool zero_padding, byte bytexpo, outbyte_t out);

// User Use
int  outsfmtlst(const char* fmt, para_list lst);
int  outsfmt(const char* fmt, ...);
outbyte_t outredirect(outbyte_t out);

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

#include "../cpp/queue"
#include "../cpp/stream"
#include "../cpp/string"
#include "../cpp/trait/SheetTrait.hpp"
#include "../c/graphic.h"

namespace uni {
	// Command Line Interface
	class Console_t : public OstreamTrait, public IstreamTrait
	{
	public:
		//virtual int out(const char* str, stduint len) { _TEMP return 0; }
		//virtual int inn() { _TEMP return 0; }
		
		// print string with format at cursor
		// C Style printf
		// // virtual int FormatShow(const char* fmt, ...) = 0;
		// [Update] Please Use Console.OutFormat but Console.OutFormat
	};
	
	class HostConsole : public Console_t // single instance
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
	#if defined(_WinNT) || defined(_Linux)//{} remove the condition
		void Clear() {
			//{TODO} buffers for
			ConClearScreen();
		}
	#endif
	};

#if defined(_MCCA)

	enum class TTY_Mode {
		Console,
		Graphic,
		None
	};
	struct TTY {
		TTY_Mode mode;
		void* body;// BareConsole or ...
		stduint fore_id;// foreid < nums
		stduint nums;// of tasks this have been binded
	};// May need a array for multiple tasks. bind_task[nums]

	// |----------|
	// |area|area | area_show  ( left:0 , top, width =columns , height , filled=bgcolor_enable, (bg)color )
	// |show|total| area_total ( columns , lines_total )
	// |----|     |
	// |          |
	// |----------|
	class BareConsole
		: public Console_t, public SheetTrait
	{
	public:
		virtual int out(const char* str, stduint len);
		virtual int inn();
		virtual void doshow(void*) override;
		virtual void onrupt(SheetEvent event, Point rel_p, ...) override {}
	public:
		Rectangle area_show;// relatived to Screen
		Size2 area_total;// relatived to Memory
		pureptr_t vga_loc_addr; // Locale VGA buffer
		byte unit = 2;// 2 for a WORD
		stduint crtline = 0;// should be in [0, area_total.y - area_show.height]
		stduint topline;// 
		stduint last_curposi;// user set, should be in [width*topline, width*(topline+area_show.height)-1]
		// - out
		byte attr = 0;
		bool attr_enable = 0;
		bool auto_incbegaddr = 1;
		stduint stat_lines = 0;// inc by scroll
		#ifdef _MCCA
		//[TEMP] no output buffer, user library can make it in their level.
		QueueLimited input_queue = (Slice){0, 0};
		#endif
		//
		// - use global cursor position function
		//
		BareConsole(stduint columns, stduint lines_total, stduint address, stduint topline = 0)// e.g. BareConsole(80,25,_VIDEO_ADDR_BUFFER)
			: area_show(0, 0, columns, lines_total), area_total(columns, lines_total), vga_loc_addr((pureptr_t)address), topline(topline)
		{
			last_curposi = topline * columns;// next char will be written at vga_loc_addr[unit*crt_curposi]
		}

		void setShowY(stduint top, stduint height) {
			area_show.y = top;
			area_show.height = height;
		}// e.g. (0,24), (1,24), ...

		void Scroll(stdsint dif, word& posi);// positive for up, negative for down


		static void setStartPosition(word pos);
		void setStartLine(word lineno) {
			setStartPosition(lineno * area_total.x);
		}
	};
	// Compatible:
	// - TEXT 80x25
	// Current:
	// - Flush Immediately
	// - Range: [width*topline, width*(topline+area_show.height)-1]
	//{unchk} when left!=0 or width!=columns
	//{todo} bgcolor
	//{todo} scroll down
	//{todo} for area_show.height < screen.height



#endif


#if defined(_WinNT) || defined(_Linux) || defined(_MCCA)
	extern HostConsole Console;
#endif

	class HostConsoleLine :
		public IstreamTrait // return -1 for LineEnd
	{
		_TODO
	};

	
}

#endif
#endif
