// UTF-8 CPL TAB4 CRLF
// Docutitle: (Device) Mouse / マウス
// Codifiers: @ArinaMgk
// Attribute: 
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

#include "../../../inc/c/driver/mouse.h"
#include "../../../inc/c/driver/i8259A.h"

#ifdef _SUPPORT_Port8

#define KEYCMD_SENDTO_MOUSE 0xD4
#define MOUSECMD_ENABLE     0xF4

void Mouse_Init()
{
	i8259Master_Enable(2);
	i8259Slaver_Enable(4);// KBD
	//
	// Keyboard_Wait();
	// outpb(KEYBOARD_CMD, 0xA8); // enable aux device
	//
	Keyboard_Wait();
	outpb(KEYBOARD_CMD, KEYCMD_SENDTO_MOUSE);
	Keyboard_Wait();
	outpb(KEYBOARD_DAT, MOUSECMD_ENABLE);
	return; /* うまくいくとACK(0xfa)が送信されてくる */
}

#endif
