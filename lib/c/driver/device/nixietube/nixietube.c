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

#include "../../../../inc/c/driver/nixietube.h"

byte _LED_SEGMENTS_HEXINUM[16] = // [A~F]
{
	0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F,
	0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71
};// or 0x80 for pointing-with

#if defined(_NIXIETUBE_PORT_DAT)

void Nixie_Show(unsigned char Location, Number)// Omit Type: same with the late one?
{
	outpb(_NIXIETUBE_PORT_DAT, 0);
	_NIXIETUBE_PORT_DAT = 0;
	P2_4=Location&0x4;
	P2_3=Location&0x2;
	P2_2=Location&0x1;
	_NIXIETUBE_PORT_DAT = LEDNumTab[Number];
}



#endif
