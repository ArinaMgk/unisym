// ASCII C-SDCC TAB4 CRLF
// Attribute: depend(delay.h)
// LastCheck: 2024Mar02
// AllAuthor: @dosconio
// ModuTitle: HD44780 Driver
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

// Dosconio' tentative design with macro:
/* E.g. @$(CC) $(DIR)/hd44780.rel -c $(ULIB)/c/driver/HD44780.c -DHD44780_Out=LCD1602_Output -D_HD44780_PORT_CMD=p2 \
	-D_HD44780_PORT_DAT=p0 -D_IMPLEMENT_SDCC8051 \
	-D_HD44780_PIN_RS=p2_6 -D_HD44780_PIN_RW=p2_5 -D_HD44780_PIN_EN=p2_7
*/

#ifndef _INC_CHIP_HD44780
#define _INC_CHIP_HD44780

#if defined(_IMPLEMENT_SDCC8051)
#include "../../../inc/c/MCU/Intel/sdcc8051.h"
#elif defined(_IMPLEMENT_KEIL8051)
#include "../../../inc/c/MCU/Intel/keil8051.h"
#else
#endif
#include "../stdinc.h"

void HD44780_Out(byte cmddat, byte notcmd);

#endif
