// ASCII C TAB4 CRLF
// Attribute: depend(HD44780.c)
// LastCheck: 2024Mar02
// AllAuthor: @dosconio
// ModuTitle: LCD1602 Header for MCU
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

#ifndef _INC_DEV_LCD1602
#define _INC_DEV_LCD1602

//{TODO} combinated with - JUST till here - until consio contain these.

#include "../stdinc.h"
#if defined(_INC_CPP) && defined(_SUPPORT_GPIO)

#include "../../cpp/Device/IIC"
#include "../../cpp/Device/GPIO"

#define _ADDR_PCF8574_T   0x4E
#define _ADDR_PCF8574_AT  0x7E

#define _LCD1602_ADDR_ROW1 0x80// address of row 0
#define _LCD1602_ADDR_ROW2 0xc0// address of row 1

namespace uni {
	//{TODO} class LCD1602, IIC as a case.
	class LCD1602_IIC_t {
		IIC_t IIC;
		// bool state;
		void (*delay_ms)(stduint ms);

		void Send(byte dat, bool is_cmd = true) {
			const byte mask = is_cmd ? 0x0C : 0x0D;
			byte tmp;
			IIC.SendStart();
			IIC << _ADDR_PCF8574_T;
			tmp = dat & 0xF0 | mask;// keep high 4 bits, set RS = 0, RW = 0, EN = 1
			IIC << tmp;
			asserv(delay_ms)(1);
			tmp &= 0xFB; // make EN = 0
			IIC << tmp;
			dat <<= 4;
			tmp = dat | mask;
			IIC << tmp;
			asserv(delay_ms)(1);
			tmp &= 0xFB;
			IIC << tmp;
		}


	public:
		LCD1602_IIC_t(IIC_t& iic, void (*delay_ms)(stduint ms) = nullptr);

		void setMode() {
			Send(0x33); // BUS8 -> BUS4
			Send(0x32); //
			Send(0x28); // 4b，2rows，5*7 while 0x38 for 8b
			Send(0x0C); // on display, off cursor, no blink
			Send(0x06); // no inc
			Clear();
		}

		void Clear() {
			Send(0x01); // Clear Screen
		}

		void DrawString(const char* str, word x = 0, word y = 0)
		{
			MIN(x, 15);
			MIN(y, 1);
			byte addr = 0x80 + 0x40 * y + x; // Move cursor
			Send(addr);
			while (*str)
			{
				Send(*str++, false);
			}
		}

	};
}

#elif defined(_MCU_Intel8051)

#include "HD44780.h"

void LCD1602_Initialize(void);

void LCD1602_Clear(void);

void LCD1602_CursorSet(byte col, byte row);

//

void LCD1602_Position(word posi);

void LCD1602_OutText(const char* str, word len);

#define LCD1602_OutString(a) LCD1602_OutText(a, ~(dword)0)

void LCD1602_OutChar(const char chr);

void LCD1602_Outi8hex(const byte inp);

void LCD1602_Outi16hex(const word inp);

void LCD1602_Outi32hex(const dword inp);

void LCD1602_Outu8dec(const sbyte inp, byte len);

#endif
#endif
