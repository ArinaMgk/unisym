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

#include "../../../inc/c/consio.h"
#include "../../../inc/c/ISO_IEC_STD/stdlib.h"

#if defined(_WinNT)
	#define byte _byte // avoid warning C4114 and error C2632
	#include <windows.h>
	#undef byte
	static HANDLE ConHandle = { 0 };
#endif

void ConCursorMoveUp(unsigned short dif)
{
#if defined(_Linux)
	if (dif) printf("\033[%hdA", dif);
#endif
}

void ConCursorMoveDown(unsigned short dif)
{
#if defined(_Linux)
	if (dif) printf("\033[%hdB", dif);
#endif
}

void ConCursorMoveRight(unsigned short dif)
{
#if defined(_WinNT)
	if (!ConHandle) ConHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO ConInfo;
	GetConsoleScreenBufferInfo(ConHandle, &ConInfo);
	ConCursor(ConInfo.dwCursorPosition.X + dif, ConInfo.dwCursorPosition.Y);
#elif defined(_Linux)
	if (dif) printf("\033[%hdC", dif);
#endif
}

void ConCursorMoveLeft(unsigned short dif)
{
#if defined(_Linux)
	if (dif) printf("\033[%hdD", dif);
#endif
}

void ConCursor(unsigned short col, unsigned short row)
{
#if defined(_WinNT)
	if (!ConHandle) ConHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	COORD pos = { col,row };
	SetConsoleCursorPosition(ConHandle, pos);
#elif defined(_Linux)
	printf("\033[%hd;%hdH", col, row);
#endif
}


	