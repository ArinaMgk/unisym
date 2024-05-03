// ASCII CPL TAB4 CRLF
// Docutitle: (Module) Real Time Clock
// Codifiers: @dosconio: 20240220 ~ 20240429
// Attribute: Arn-Covenant Any-Architect Bit-32mode <Reference/Dependence>
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

#ifndef _INC_DEVICE_DS1302
#define _INC_DEVICE_DS1302

#include "../stdinc.h"

extern signed char DS1302_Time[];// respectively: year, month, day, hour, minute, second, weekday(signed)
void DS1302Init(void);
void DS1302WriteByte(byte Command, byte Data);
byte DS1302ReadByte(byte Command);
void DS1302SetTime(void);
void DS1302GetTime(void);

#endif
