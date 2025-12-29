// ASCII CPL TAB4 CRLF
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

#ifndef _INC_DEVICE_Mouse
#define _INC_DEVICE_Mouse

#include "../stdinc.h"

// x86
#include "./keyboard.h"

struct MouseMessage {
	byte BtnLeft : 1;
	byte BtnRight : 1;
	byte BtnMiddle : 1;
	byte HIGH : 1;// always 1
	byte DirX : 1;
	byte DirY : 1;
	byte OvfX : 1;
	byte OvfY : 1;
	byte X;
	byte Y;
};

// pres: init keyboard
_ESYM_C void Mouse_Init();



#endif
