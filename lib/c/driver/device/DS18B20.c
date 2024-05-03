// ASCII CPL TAB4 CRLF
// Docutitle: DS18B20
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

#include "../../../../inc/c/stdinc.h"
#include "../../../../inc/c/driver/DS18B20.h"

#if defined(_MCU_Intel8051)

// DS18B20 Instruction
#define DS18B20_SKIP_ROM			0xCC
#define DS18B20_CONVERT_T			0x44
#define DS18B20_READ_SCRATCHPAD 	0xBE

void DS18B20_Convert(void)// Convert Temperature
{
	OneWire_Init();
	OneWire_SetByte(DS18B20_SKIP_ROM);
	OneWire_SetByte(DS18B20_CONVERT_T);
}

float DS18B20_Read(void)
{
	unsigned char TLSB, TMSB;
	int Temp;
	float T;
	OneWire_Init();
	OneWire_SetByte(DS18B20_SKIP_ROM);
	OneWire_SetByte(DS18B20_READ_SCRATCHPAD);
	TLSB = OneWire_GetByte();
	TMSB = OneWire_GetByte();
	Temp = (TMSB << 8) | TLSB;
	T = Temp / 16.0;
	return T;
}

#endif


