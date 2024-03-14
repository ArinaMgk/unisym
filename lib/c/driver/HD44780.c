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

//{TODO} Just for port that cannot be assigned by pointers, solve it.

#define _HD44780_BIT4 0// ? detail in old cotlib aka unisyml

#if defined(_HD44780_PORT_CMD) && defined(_HD44780_PORT_DAT) && defined(_HD44780_PIN_RS) && defined(_HD44780_PIN_RW) && defined(_HD44780_PIN_EN)// && defined(_HD44780_BIT4)

#include "../../../inc/c/MCU/delay.h"
#include "../../../inc/c/driver/HD44780.h"

void HD44780_Out(byte cmddat, byte notcmd)
{
	outpi(_HD44780_PIN_RS, notcmd);
	outpi(_HD44780_PIN_RW, 0);
	outpi(_HD44780_PIN_EN, 0);
	outpi(_HD44780_PORT_DAT, cmddat);
	delay500us();
	outpi(_HD44780_PIN_EN, 1);
	delay500us();
	outpi(_HD44780_PIN_EN, 0);
	#if _HD44780_BIT4==1
	outpi(_HD44780_PORT_DAT, cmddat << 4);
	delay500us();
	outpi(_HD44780_PIN_EN, 1);
	delay500us();
	outpi(_HD44780_PIN_EN, 0);
	#endif
}

#endif// _HD44780_PORT
