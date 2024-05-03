// ASCII C-SDCC TAB4 CRLF
// Attribute: depend(HD44780.c)
// LastCheck: 2024Mar02
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

#include "../../../../inc/c/driver/LCD1602.h"


void LCD1602_Initialize(void)
{
	HD44780_Out(
		#if !defined(_HD44780_BIT4) || _HD44780_BIT4!=1
		0x38// 8-bit, 2-line, 5x7
		#else
		0x28// 4-bit, 2-line, 5x7
		#endif
		, be_cmd);
	HD44780_Out(0x0c, be_cmd);// Display on, cursor off, blink off
	HD44780_Out(0x06, be_cmd);// Entry mode: cursor increment, no shift scroll
	HD44780_Out(0x01, be_cmd);// Clear display
}

void LCD1602_Clear(void)
{
	HD44780_Out(0x01, be_cmd);
}

void LCD1602_CursorSet(byte col, byte row)
{
	HD44780_Out(0x80 + (row * 0x40) + col, be_cmd);
}

// ---- just for single screen ----
//{TODO} combinated by consio 

static char _tab_dec2hex[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

void LCD1602_Position(word posi)
{
	// HD44780_Out(0x80 + pos, be_cmd);
	if (posi <= 16 * 2)// col * row
		LCD1602_CursorSet(posi & 0xF, posi >> 4);// (posi%16, posi/16)
}

void LCD1602_OutText(const char* str, word len)//void outtxt(const char *str, dword len);
{
	while (len-- && *str)
		HD44780_Out(*str++, be_dat);
}

//#define LCD1602_OutString(a) LCD1602_OutText(a, ~(dword)0)//outs

void LCD1602_OutChar(const char chr)//void outc(const char chr);
{
	HD44780_Out(chr, be_dat);
	//<=> LCD1602_OutText(&chr, 1);
}

// ----

void LCD1602_Outi8hex(const byte inp)//void outi8hex(const byte inp);
{
	byte val = inp;
	char buf[2];
	byte i;// Keil requested but SDCC
	for (i = 0; i < 2; i++)
	{
		buf[1 - i] = _tab_dec2hex[val & 0xF];
		val >>= 4;
	}
	LCD1602_OutText(buf, 2);
}

void LCD1602_Outi16hex(const word inp)//void outi16hex(const word inp);
{
	word val = inp;
	char buf[4];
	byte i;
	for (i = 0; i < 4; i++)
	{
		buf[3 - i] = _tab_dec2hex[val & 0xF];
		val >>= 4;
	}
	LCD1602_OutText(buf, 4);
}

void LCD1602_Outi32hex(const dword inp)//void outi32hex(const dword inp);
{
	dword val = inp;
	char buf[8];
	byte i;
	for (i = 0; i < 8; i++)
	{
		buf[7 - i] = _tab_dec2hex[val & 0xF];
		val >>= 4;
	}
	LCD1602_OutText(buf, 8);
}

void LCD1602_Outu8dec(const sbyte inp, byte len)
{
	// 0 ~ 255
	char buf[3];
	sbyte val = inp;
	byte i;
	if (len > 3) len = 3;
	for (i = 0; i < len; i++)
	{
		buf[len - 1 - i] = '0' + val % 10;
		val /= 10;
	}
	LCD1602_OutText(buf + len - i, i);
}

//{TODO} combinated with - JUST till here - until consio contain these.
