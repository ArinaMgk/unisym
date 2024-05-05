// ASCII CPL TAB4 CRLF
// Docutitle: One-Wire Protocol
// Datecheck: 20240429 ~ <Last-check>
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

#if !defined(_INC_STANDARD_OneWire)
#define _INC_STANDARD_OneWire

#if defined(_MCU_Intel8051)

unsigned char OneWire_Init(void);

void OneWire_SetBit(unsigned char Bit);
unsigned char OneWire_GetBit(void);

void OneWire_SetByte(unsigned char Byte);
unsigned char OneWire_GetByte(void);

#endif

#endif
