// GBK RFR30+
// Open-source ArinaMgk
/*
	Copyright 2023 ArinaMgk

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/

#ifndef ModConIO
#define ModConIO
#include <stdio.h>

// Return the length of the words excluding terminating zero but "limit" considers it.
size_t ConScanLine(char* buf, size_t limit);

#if defined(_Linux)
//
static inline void ConCursorMoveUp(unsigned short dif)
{
	if (dif)printf("\033[%hdA", dif);
}

//
static inline void ConCursorMoveDown(unsigned short dif)
{
	if (dif)printf("\033[%hdB", dif);
}

//
static inline void ConCursorMoveRight(unsigned short dif)
{
	if (dif)printf("\033[%hdC", dif);
}

//
static inline void ConCursorMoveLeft(unsigned short dif)
{
	if (dif)printf("\033[%hdD", dif);
}
#elif defined(_WinNT)
void ConCursorMoveRight(unsigned short dif);
#endif


//
static inline void ConCursorReset(void)
{
	printf("\033[H");
}


#if defined(_WinNT)
void ConCursor(unsigned short col, unsigned short row);
#else// defined(_Linux)
//
static inline void ConCursor(unsigned short col, unsigned short row)
{
	printf("\033[%hd;%hdH", col, row);
}
#endif


//
static inline void ConCursorShow(void)
{
	printf("\033[?25h");
}

//
#if defined(_WinNT)
void ConClear(void);
#else// defined(_Linux)
//
static inline void ConClear(void)
{
	printf("\033[2J");
}
#endif


//
static inline void ConCursorHide(void)
{
	printf("\033[?25l");
}

#if defined(_Linux)
// The style is for the brush
static inline void ConStyleAbnormal(void)
{
	printf("\033[7m");
}
// The style is for the brush
static inline void ConStyleNormal(void)
{
	printf("\033[27m");
}
#elif defined(_WinNT)
// The style is for the brush
void ConStyleAbnormal(void);
// The style is for the brush
void ConStyleNormal(void);
#endif

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

#endif