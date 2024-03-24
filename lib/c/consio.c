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
#include <stdlib.h>
#if defined(_WinNT)
#include <windows.h>

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

void ConClear(void)
{
	system("cls");
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
	system("clear");
#endif
	// cheat compiler
}
