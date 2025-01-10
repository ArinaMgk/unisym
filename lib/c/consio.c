// ASCII C99 TAB4 CRLF
// Docutitle: (Module) Console Input and Output
// Codifiers: @ArinaMgk; @dosconio: 20231116 ~ <Last-check> 
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
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

#define _INC_USTDBOOL
#define bool int
#define boolean byte
#define BOOLEAN byte

#include "../../inc/c/consio.h"
#include "../../inc/c/ISO_IEC_STD/stdlib.h"

//{TO-MOVE}
#if defined(_MCCA) && (_MCCA==0x8616||_MCCA==0x8632)
#include "../../inc/c/proctrl/x86/x86.h"
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

#if defined(_WinNT)
	#define byte _byte // avoid warning C4114 and error C2632
	#include <windows.h>
	#undef byte
	static HANDLE ConHandle = { 0 };
#endif

#if 0
#elif defined(_MCCA) && _MCCA==0x8632
#include "../../inc/c/board/IBM.h"

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
#endif


#if defined(_WinNT) || defined(_Linux)
#include <stdio.h>
void outtxt(const char* str, stduint len)
{
	for0(i, len) {
		putchar(*str++);
	}
	_crt_out_cnt += len;
}

#elif defined(_MCCA) && _MCCA==0x8632
void outtxt(const char* str, stduint len) {
	static byte attr = 0;
	static byte attr_enable = 0;
	word posi = curget() * 2;
	byte chr;
	MIN(len, StrLength(str));
	for0(i, len) {
		chr = (byte)*str++;
		switch (chr)
		{
		case (byte)'\xFF':// 20240217-ALICE's FF Method
			attr = *str++;
			attr_enable = (attr != (byte)'\xFF');
			_LIMIT--;
			break;
		case '\r':
			posi -= posi % _BytesPerLine; //= posi / _BytesPerLine * _BytesPerLine;
			break;
		case '\n':// down
			posi += _BytesPerLine;
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
			_VideoBuf[posi++] = chr;
			if (attr_enable)
				_VideoBuf[posi++] = attr;
			else
				posi++;
			break;
		}
		if (!chr) break;
		while (posi >= _ScreenSize) {
			scrrol(1);
			posi -= _BytesPerLine;
		}
	}
	curset(posi / 2);
	_crt_out_cnt += len;
}
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
