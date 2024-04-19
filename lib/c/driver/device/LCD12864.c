// ASCII C-SDCC TAB4 CRLF
// Attribute: depend(HD44780.c)
// LastCheck: 20240413
// AllAuthor: @dosconio
// ModuTitle: LCD1602
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

#if defined(_LCD12864_PIN_PSB)

#include "../../../../inc/c/driver/LCD12864.h"

#ifndef _HD44780_BIT4
#define _HD44780_BIT4 0// default 8 parallel
#endif

void LCD12864_Initialize(void)
{
	outpi(_LCD12864_PIN_PSB, 1);// {TEMP}parallel mode
	HD44780_Out(0x30, be_cmd);// {TEMP}8 bit(accept-only)
	HD44780_Out(0x0c, be_cmd);// Display on, cursor off, blink off
	HD44780_Out(0x06, be_cmd);// Entry mode: cursor increment, no shift scroll
	HD44780_Out(0x01, be_cmd);// Clear display
}

void LCD12864_Clear(void)
{
	HD44780_Out(0x01, be_cmd);
}

void LCD12864_CursorSet(byte col, byte row)
{
	static const byte _tab_row[] = { 0x80, 0x90, 0x88, 0x98 };
	HD44780_Out((_tab_row[row]) | col, be_cmd);
}

// ---- just for single screen ----
//{TODO} combinated by consio 

static char _tab_dec2hex[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

void LCD12864_Position(word posi)
{
	// HD44780_Out(0x80 + pos, be_cmd);
	if (posi <= 16 * 2)// col * row
		LCD12864_CursorSet(posi & 7, posi >> 3);// ? useful ?
}

void LCD12864_OutText(const char* str, word len)//void outtxt(const char *str, dword len);
{
	while (len-- && *str)
		HD44780_Out(*str++, be_dat);
}

//#define LCD12864_OutString(a) LCD12864_OutText(a, ~(dword)0)//outs

void LCD12864_OutChar(const char chr)//void outc(const char chr);
{
	HD44780_Out(chr, be_dat);
	//<=> LCD12864_OutText(&chr, 1);
}

// ----

void LCD12864_Outi8hex(const byte inp)//void outi8hex(const byte inp);
{
	byte val = inp;
	char buf[2];
	byte i;// Keil requested but SDCC
	for (i = 0; i < 2; i++)
	{
		buf[1 - i] = _tab_dec2hex[val & 0xF];
		val >>= 4;
	}
	LCD12864_OutText(buf, 2);
}

void LCD12864_Outi16hex(const word inp)//void outi16hex(const word inp);
{
	word val = inp;
	char buf[4];
	byte i;
	for (i = 0; i < 4; i++)
	{
		buf[3 - i] = _tab_dec2hex[val & 0xF];
		val >>= 4;
	}
	LCD12864_OutText(buf, 4);
}

void LCD12864_Outi32hex(const dword inp)//void outi32hex(const dword inp);
{
	dword val = inp;
	char buf[8];
	byte i;
	for (i = 0; i < 8; i++)
	{
		buf[7 - i] = _tab_dec2hex[val & 0xF];
		val >>= 4;
	}
	LCD12864_OutText(buf, 8);
}

//{TODO} combinated with - JUST till here - until consio contain these.

#endif
