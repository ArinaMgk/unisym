// ASCII C TAB4 CRLF
// Attribute: depend(HD44780.c)
// LastCheck: 2024Mar05
// AllAuthor: @dosconio
// ModuTitle: LCD12864 Header for MCU
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

#ifndef _INC_DEV_LCD12864
#define _INC_DEV_LCD12864

#include "HD44780.h"//{} ST7920 ?

void LCD12864_Initialize(void);

void LCD12864_Clear(void);

void LCD12864_CursorSet(byte col, byte row);

//

void LCD12864_Position(word posi);

void LCD12864_OutText(const char* str, word len);

#define LCD12864_OutString(a) LCD12864_OutText(a, ~(dword)0)

void LCD12864_OutChar(const char chr);

void LCD12864_Outi8hex(const byte inp);

void LCD12864_Outi16hex(const word inp);

void LCD12864_Outi32hex(const dword inp);

#endif
