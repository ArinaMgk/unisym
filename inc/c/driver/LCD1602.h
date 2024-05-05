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
