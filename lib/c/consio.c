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

#include "../../inc/c/consio.h"

#if defined(_MCCA)// x86 or riscv64

#if defined(_MCCA) && (_MCCA==0x8616||_MCCA==0x8632)
#include "proctrl/x86/x86.h" //{TEMP}
void curset(word posi)
{
	outpb(0x03D4, 0x0E);
	outpb(0x03D5, posi >> 8);
	outpb(0x03D4, 0x0F);
	outpb(0x03D5, posi & 0xFF);
}

word curget(void)
{
	word ret;
	outpb(0x03D4, 0x0E);
	ret = innpb(0x03D5) << 8;
	outpb(0x03D4, 0x0F);
	ret |= innpb(0x03D5);
	return ret;
}

#endif

static char _tab_dec2hex[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

void outc(const char chr)
{
	outtxt(&chr, 1);
}

//{TODO} use template to simplify:

//{TEMP} always align to right
void outi8hex(const byte inp)
{
	byte val = inp;
	char buf[2];
	for (stduint i = 0; i < 2; i++)
	{
		buf[1 - i] = _tab_dec2hex[val & 0xF];
		val >>= 4;
	}
	outtxt(buf, 2);
}

// Replacement of DbgEcho16
void outi16hex(const word inp)
{
	word val = inp;
	char buf[4];
	for (stduint i = 0; i < 4; i++)
	{
		buf[3 - i] = _tab_dec2hex[val & 0xF];
		val >>= 4;
	}
	outtxt(buf, 4);
}

// Replacement of DbgEcho32
void outi32hex(const dword inp)
{
	dword val = inp;
	char buf[8];
	for (stduint i = 0; i < 8; i++)
	{
		buf[7 - i] = _tab_dec2hex[val & 0xF];
		val >>= 4;
	}
	outtxt(buf, 8);
}

void outi64hex(const uint64 inp)
{
	uint64 val = inp;
	char buf[16];
	for (stduint i = 0; i < numsof(buf); i++)
	{
		buf[numsof(buf) - 1 - i] = _tab_dec2hex[val & 0xF];
		val >>= 4;
	}
	outtxt(buf, numsof(buf));
}

void outidec(int xx, int base, int sign)
{
	char buf[16];
	int i;
	unsigned x;

	if (sign && (sign = xx < 0))
		x = -xx;
	else
		x = xx;

	i = 0;
	do {
		buf[i++] = _tab_dec2hex[x % base];
	} while ((x /= base) != 0);

	if (sign)
		buf[i++] = '-';

	while (--i >= 0)
		outc(buf[i]);
}

//{UNCHK}
void outi(stdint val, int base, int sign_show)
{
	if (base < 2) return;
	char buf[bitsof(stdint)] = { 0 };
	int i = 0;
	if (val < 0) val = -val;
	do buf[i++] = _tab_dec2hex[val % base]; while (val /= base);
	if (sign_show) buf[i++] = val < 0 ? '-' : '+';
	outtxt(buf, numsof(buf));
}

void outsfmtlst(const char* fmt, va_list lst)
{
	int i;
	byte c;
	char* s;
	va_list paras = lst;
	if (fmt == 0) return;

	for (i = 0; (c = fmt[i]); i++) {
		if (c != '%') {
			if (c == (byte)'\xFF')
			{
				outtxt(&fmt[i], 2);
				i++;
			}
			else outtxt(&fmt[i], 1);
			continue;
		}
		c = fmt[++i];
		if (c == 0)
			break;
		switch (c) {
		case 'd':
			outidec(va_arg(paras, int), 10, 1);
			break;
		case 'x':
			outidec(va_arg(paras, int), 16, 1);
			break;
		case 'p':
			outtxt("0x", 2);
			if (bitsof(stduint) == 64)
				outi64hex(va_arg(paras, stduint));
			else if (bitsof(stduint) == 32)
				outi32hex(va_arg(paras, stduint));
			else if (bitsof(stduint) == 16)
				outi16hex(va_arg(paras, stduint));
			else
				outi8hex(va_arg(paras, stduint));
			break;
		case 's':
			if ((s = va_arg(paras, char*)) == 0)
				s = "(null)";
			outtxt(s, -1);
			break;
		case '%':
			c = '%';
			outtxt(&fmt[i], 1);
			break;
		default:
			// no-care
			break;
		}
	}
}

//{TEMP} only understands %d, %x, %p, %s.
void outsfmt(const char* fmt, ...)
{
	va_list paras;

	va_start(paras, fmt);
	outsfmtlst(fmt, paras);
	va_end(paras);
}

#else

#if defined(_WinNT)
#define byte _byte // avoid warning C4114 and error C2632
#include <windows.h>
#undef byte

static HANDLE ConHandle = { 0 };

void ConCursorMoveRight(unsigned short dif)
{
	if (!ConHandle) ConHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO ConInfo;
	GetConsoleScreenBufferInfo(ConHandle, &ConInfo);
	ConCursor(ConInfo.dwCursorPosition.X + dif, ConInfo.dwCursorPosition.Y);
}

void ConCursor(unsigned short col, unsigned short row)
{
	if (!ConHandle) ConHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	COORD pos = { col,row };
	SetConsoleCursorPosition(ConHandle, pos);
}

void ConStyleAbnormal(void)
{
	if (!ConHandle) ConHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(ConHandle, 0x0 + 0xF0);
}

void ConStyleNormal(void)
{
	if (!ConHandle) ConHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(ConHandle, 0xF + 0x00);
}

#endif

size_t ConScanLine(char* buf, size_t limit)
{
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
}

void ConClearScreen(void)
{
	int i =
	#ifdef _WinNT
		system("cls");
	#else // elif defined(_Linux)
		system("clear");// <=> printf("\033[2J");
	#endif
	// cheat compiler
}

#endif
