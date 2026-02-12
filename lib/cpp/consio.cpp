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

#define _INC_USTDBOOL

#include "../../inc/c/consio.h"
#if !defined(_MCCA)
#include <stdio.h>
#endif

#if defined(_WinNT) || defined(_Linux) ||  defined(_MCCA)
namespace uni {
	HostConsole Console;

	int HostConsole::out(const char* str, stduint len) {
		if (!count_mode) outtxt(str, len);
		out_count += len;
		return len;
	}
	int HostConsole::inn() {
	#if defined(_WinNT) || defined(_Linux)
		return _TEMP getchar();
	#else
		return _TEMP 0;
	#endif
	}

}
//{TEMP} only understands %d, %x, %p, %s with-no modification.
int outsfmt(const char* fmt, ...)
{
	Letpara(args, fmt);
	return outsfmtlst(fmt, args);
	para_endo(args);
}
int outsfmtlst(const char* fmt, para_list paras) { return uni::Console.OutFormatPack(fmt, paras); }
void outc(const char chr) { uni::Console.out(&chr, 1); }
#if defined(_BIT_SUPPORT_64)
#if defined(_MCCA) && ((_MCCA & 0xFF00) == 0x8600)
__attribute__((target("sse2")))
#endif
bool outinteger(uint64 val, int base, bool sign_show, bool sign_have, byte least_digits, bool zero_padding, byte bytexpo, outbyte_t out) { (void)out; return uni::Console.OutInteger(val, base, sign_show, sign_have, least_digits, zero_padding, bytexpo); }
#else
#endif
#if defined(_MCCA) && ((_MCCA & 0xFF00) == 0x8600)
__attribute__((target("sse2")))
#endif
static void outfloat(float val, outbyte_t out) { (void)out; return uni::Console.OutFloating(val); }

#endif


#if defined(_WinNT)
	#define byte _byte // avoid warning C4114 and error C2632
	#include <windows.h>
	#undef byte
	static HANDLE ConHandle = { 0 };
#endif

#if defined(_WinNT) || defined(_Linux)
#include <stdio.h>
void outtxt(const char* str, stduint len)
{
	for0(i, len) {
		putchar(*str++);
	}
	//_crt_out_cnt += len;
}

#elif defined(_MCCA) && _MCCA==0x8632// in consio.cpp
#endif

void ConStyleAbnormal(void)
{
#if defined(_WinNT)
	if (!ConHandle) ConHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(ConHandle, 0x0 + 0xF0);
#elif defined(_Linux)
	printf("\033[7m");
#endif
}

void ConStyleNormal(void)
{
#if defined(_WinNT)
	if (!ConHandle) ConHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(ConHandle, 0xF + 0x00);
#elif defined(_Linux)
	printf("\033[27m");
#endif
}

size_t ConScanLine(char* buf, size_t limit)
{
#if defined(_WinNT) || defined(_Linux)
	size_t slen = 0;
	char* tmp = fgets(buf, (int)limit, stdin);// tmp: cheat compiler
	if (!*buf) return 0;
	while (buf[slen + 1])slen++;
	while ((buf[slen] == '\n' || buf[slen] == '\r'))
	{
		buf[slen] = 0;
		if (!slen) break; else slen--;
	}
	return 1 + slen;
#else
	return _TODO 0;
#endif
}

void ConClearScreen(void)
{
	int i =
#ifdef _WinNT
		system("cls");
#elif defined(_Linux)
		system("clear");// <=> printf("\033[2J");
#else
		0; i++;
#endif
	// cheat compiler
}


// CRT VGA
#if defined(_MCCA) && ((_MCCA & 0xFF00) == 0x8600)
#include "../../inc/c/proctrl/x86/x86.h"
#include "../../inc/c/board/IBM.h"
#include "../../inc/cpp/Device/Buzzer.hpp"

void curset(word posi)
{
	outpb(PORT_CRT_CRAR, CRT_CDR_CursorLocationHigh);
	outpb(PORT_CRT_DRAR, posi >> 8);
	outpb(PORT_CRT_CRAR, CRT_CDR_CursorLocationLow);
	outpb(PORT_CRT_DRAR, posi & 0xFF);
}

word curget(void)
{
	word ret;
	outpb(PORT_CRT_CRAR, CRT_CDR_CursorLocationHigh);
	ret = innpb(PORT_CRT_DRAR) << 8;
	outpb(PORT_CRT_CRAR, CRT_CDR_CursorLocationLow);
	ret |= innpb(PORT_CRT_DRAR);
	return ret;
}

static byte* const _VideoBuf = (byte*)_VIDEO_ADDR_BUFFER; //{TODO} + 0x80000000;
const static word _CharsPerLine = 80;
const static word _BytesPerLine = _CharsPerLine * 2;
const static word _LinesPerScreen = 25;
const static word _ScreenSize = _BytesPerLine * _LinesPerScreen;

void scrrol(word lines)
{
	if (!lines) return;
	MIN(lines, _LinesPerScreen);
	word* sors = (word*)_VideoBuf + _CharsPerLine * lines;// Add for Pointer!
	word* dest = (word*)_VideoBuf;
	forp(dest, _CharsPerLine * (_LinesPerScreen - lines))* dest = *sors++;
	forp(dest, _CharsPerLine * lines)
		* dest = 0x0720;//{TEMP} the new lines are of 'white on black' color
}

namespace uni {
	//[sync(outtxt)]
	int BareConsole::out(const char* str, stduint len) {
		const stduint _BytesPerLine = unit * area_total.x;
		const stduint _Offset = unit * area_show.y * area_total.x;// Top Area
		volatile byte* _VideoBuf0 = (volatile byte*)vga_loc_addr;
		if (!count_mode)// outtxt(str, len);
		{
			word posi = last_curposi * unit;
			byte chr;
			// MIN(len, StrLength(str));
			for0(i, len) {
				chr = (byte)*str++;
				switch (chr)
				{
				case 0:
					break;
				case (byte)'\xFF':// 20240217-ALICE's FF Method
					attr = *(byte*)str++;
					attr_enable = (attr != (byte)'\xFF');
					_LIMIT--;
					break;
				case '\r':
					posi -= posi % _BytesPerLine; //= posi / _BytesPerLine * _BytesPerLine;
					break;
				case '\n':// down
					posi += _BytesPerLine;
					break;
				case '\a':
					#ifndef _UEFI
					Buzzer::Buzz(true);
					for (size_t i = 0, _LIMIT = (100);i < (_LIMIT);i++) {
						_ASM volatile("nop"::: "memory");// "pause"
					}
					Buzzer::Buzz(false);
					#endif
					break;
				case '\b':// left
					posi -= 2;
					break;
				case '\x01':// next
					posi += 2;
					break;
				case '\x02':// up
					posi -= _BytesPerLine;
					break;
				default:
					_VideoBuf0[posi++] = chr;//{}
					if (attr_enable)
						_VideoBuf0[posi++] = attr;//{}
					else
						posi++;
					break;
				}
				if (!chr) break;
				// posi includes the crtline (aka. begin-addr)
				//while (posi - unit * area_total.x * crtline >= _Offset + _BytesPerLine * area_show.height)
				while (posi >= _BytesPerLine * (topline + area_total.y))
				{
					Scroll(+1, posi);
				}
				if (auto_incbegaddr && posi / _BytesPerLine == topline + crtline + area_show.height) {
					setStartLine(++crtline);
				}
			}
			last_curposi = posi / 2;
			auto po = curget();
			if (Ranglin(po, topline * area_total.x, area_total.x * area_total.y))
				curset(last_curposi);
			// _crt_out_cnt += len;
		}
		out_count += len;
		return len;
	}
	int BareConsole::inn() {

		return _TODO - 1;
	}
	_WEAK void BareConsole::doshow(void* _) { }
	void BareConsole::Scroll(stdsint lines, word& posi) {
		//{} The fact may be ContinPage + MemAbsolute
		const stduint _BytesPerLine = unit * area_total.x;
		volatile byte* _VideoBuf = (volatile byte*)vga_loc_addr + topline * _BytesPerLine;
		//__asm("cli");
		//{TODO} 
		if (!lines) return;
		// lines > 0 :
		//MIN(lines, _LinesPerScreen);
		word* sors = (word*)(_VideoBuf + _BytesPerLine * lines);// Add for Pointer!
		word* dest = (word*)_VideoBuf;
		forp(dest, area_total.x * (area_total.y - lines))* dest = *sors++;
		forp(dest, area_total.x * lines)
			* dest = 0x0720;//{TEMP} the new lines are of 'white on black' color
		posi -= _BytesPerLine * lines;//{TEMP}! assume lines always equals 1
		stat_lines += lines;
	}

	void BareConsole::setStartPosition(word begposi) {
		outpb(PORT_CRT_CRAR, CRT_CDR_StartAddressHigh);
		outpb(PORT_CRT_DRAR, begposi >> 8);
		outpb(PORT_CRT_CRAR, CRT_CDR_StartAddressLow);
		outpb(PORT_CRT_DRAR, begposi & 0xFF);
	}

}

extern uni::OstreamTrait* con0_out;
void outtxt(const char* str, stduint len) {
	asserv (con0_out)->out(str, len);
}

#endif
#if 0

size_t ConScanLine(char* innbuf, size_t limit) {
	char ch;
	while (i < limit && std::cin.get(ch) && ch != '\n')
		innbuf[i++] = ch;
	innbuf[i] = 0;
}

#endif
