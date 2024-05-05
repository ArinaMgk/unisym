// ASCII CPL TAB4 CRLF
// Docutitle: Nixie-Tube
// Datecheck: 20240417 ~ <Last-check>
// Developer: @dosconio
// Attribute: <ArnCovenant> <Environment> <Platform>
// Reference: <Reference>
// Dependens: <Dependence>
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
#define _PORT_SPECIAL
#include "../../../../../inc/c/driver/nixietube.h"

byte _LED_SEGMENTS_HEXINUM[16] = // [A~F]
{
	0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F,
	0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71
};// or 0x80 for pointing-with

#if defined(_MCU_Intel8051)

#if defined(_IMPLEMENT_SDCC8051)
#include "../../../../../inc/c/MCU/Intel/sdcc8051.h"
#elif defined(_IMPLEMENT_KEIL8051)
#include "../../../../../inc/c/MCU/Intel/keil8051.h"
#else
#endif

#if defined(_NIXIETUBE_PORT_DAT) && defined(_NIXIETUBE_PIN_COM0) && defined(_NIXIETUBE_PIN_COM1) && defined(_NIXIETUBE_PIN_COM2) 

void Nixie_Show(unsigned char Location, uint8 Number, byte Dot)// Omit Type: same with the late one?
{
	outpb(_NIXIETUBE_PORT_DAT, 0);
	_NIXIETUBE_PORT_DAT = 0;
	outpi(_NIXIETUBE_PIN_COM2, Location & 0x4);
	outpi(_NIXIETUBE_PIN_COM1, Location & 0x2);
	outpi(_NIXIETUBE_PIN_COM0, Location & 0x1);
	if (Number < numsof(_LED_SEGMENTS_HEXINUM))
		_NIXIETUBE_PORT_DAT = _LED_SEGMENTS_HEXINUM[Number] | (Dot? 0x80: 0);
	else _NIXIETUBE_PORT_DAT = 0;
}


#endif

#endif
