// ASCII CPL TAB4 CRLF
// Docutitle: (Device) Keyboard
// Codifiers: @dosconio: 20240502 ~ 20240502
// Attribute: Arn-Covenant Any-Architect Bit-32mode Non-Dependence
// Copyright: UNISYM, under Apache License 2.0; Dosconio Mecocoa, BSD 3-Clause License
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

#ifndef _INC_DEVICE_Keyboard
#define _INC_DEVICE_Keyboard

#include "../stdinc.h"

typedef struct {
	byte ascii_usual;
	byte ascii_shift;
	const char* label_usual;// if not null, the char is not printable
	const char* label_shift;// if not null, the char is not printable
	rostr label_prefE0;
} keymap_element_t;

struct keyboard_state_t {
	byte l_ctrl : 1;
	byte r_ctrl : 1;
	byte l_shift : 1;
	byte r_shift : 1;
	byte l_alt : 1;
	byte r_alt : 1;
	byte l_logo : 1;// e.g. Windows key
	byte r_logo : 1;
	//
	byte lock_number : 1;
	byte lock_caps : 1;
	byte lock_scroll : 1;
	//
};

#define KEYBOARD_LED 0xED
#define KEYBOARD_ACK 0xFA

extern keymap_element_t _tab_keycode2ascii[0x80];

_ESYM_C void Keyboard_Init();
_ESYM_C void Keyboard_Wait();

// stat: B2 Caps, B1 Num, B0 Scroll
_ESYM_C void KbdSetLED(byte stat);

#if 1


#endif

#endif
