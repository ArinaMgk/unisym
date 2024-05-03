// ASCII CPL TAB4 CRLF
// Docutitle: Inter-Integrated Circuit, I2C
// Datecheck: 20240502 ~ <Last-check>
// Developer: @dosconio
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
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

#if !defined(_INC_DEVICE_XPT2046)
#define _INC_DEVICE_XPT2046

#if defined(_MCU_Intel8051)

#define XPT2046_VBAT 0xAC
#define XPT2046_AUX  0xEC
#define XPT2046_XP   0x9C //0xBC
#define XPT2046_YP   0xDC

unsigned int XPT2046_Ana2Dig(unsigned char Command);

#endif

#endif
