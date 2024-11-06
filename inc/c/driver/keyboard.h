// ASCII CPL TAB4 CRLF
// Docutitle: (Module) Keyboard
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

extern keymap_element_t _tab_keycode2ascii[0x80];

void Keyboard_Init();

#endif
