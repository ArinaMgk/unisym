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

#define _PORT_SPECIAL
#include "../../../inc/c/driver/DS1302.h"

#if defined(_DS1302_SCLK) && defined(_DS1302_IO) && defined(_DS1302_CE)

#define DS1302_SECOND		0x80
#define DS1302_MINUTE		0x82
#define DS1302_HOUR			0x84
#define DS1302_DATE			0x86
#define DS1302_MONTH		0x88
#define DS1302_DAY			0x8A
#define DS1302_YEAR			0x8C
#define DS1302_WP			0x8E

signed char DS1302_Time[] = { 19,11,16,12,59,55,6 };

void DS1302Init(void)
{
	outpi(_DS1302_CE, 0);
	outpi(_DS1302_SCLK, 0);
}

void DS1302WriteByte(byte Command, byte Data)
{
	byte i;
	outpi(_DS1302_CE, 1);
	for (i = 0;i < 8;i++)
	{
		outpi(_DS1302_IO, Command & (0x01 << i));
		outpi(_DS1302_SCLK, 1);
		outpi(_DS1302_SCLK, 0);
	}
	for (i = 0;i < 8;i++)
	{
		outpi(_DS1302_IO, Data & (0x01 << i));
		outpi(_DS1302_SCLK, 1);
		outpi(_DS1302_SCLK, 0);
	}
	outpi(_DS1302_CE, 0);
}

byte DS1302ReadByte(byte Command)
{
	byte i, Data = 0x00;
	Command |= 0x01;	// Reading Command
	outpi(_DS1302_CE, 1);
	for (i = 0;i < 8;i++)
	{
		outpi(_DS1302_IO, Command & (0x01 << i));
		outpi(_DS1302_SCLK, 0);
		outpi(_DS1302_SCLK, 1);
	}
	for (i = 0;i < 8;i++)
	{
		outpi(_DS1302_SCLK, 1);
		outpi(_DS1302_SCLK, 0);
		if (innpi(_DS1302_IO)) { Data |= (0x01 << i); }
	}
	outpi(_DS1302_CE, 0);
	outpi(_DS1302_IO, 0); // set IO to 0 after reading, otherwise the read data will be incorrect
	return Data;
}

void DS1302SetTime(void)
{
	DS1302WriteByte(DS1302_WP, 0x00);
	DS1302WriteByte(DS1302_YEAR, DS1302_Time[0] / 10 * 16 + DS1302_Time[0] % 10);// DEC -> BCD
	DS1302WriteByte(DS1302_MONTH, DS1302_Time[1] / 10 * 16 + DS1302_Time[1] % 10);
	DS1302WriteByte(DS1302_DATE, DS1302_Time[2] / 10 * 16 + DS1302_Time[2] % 10);
	DS1302WriteByte(DS1302_HOUR, DS1302_Time[3] / 10 * 16 + DS1302_Time[3] % 10);
	DS1302WriteByte(DS1302_MINUTE, DS1302_Time[4] / 10 * 16 + DS1302_Time[4] % 10);
	DS1302WriteByte(DS1302_SECOND, DS1302_Time[5] / 10 * 16 + DS1302_Time[5] % 10);
	DS1302WriteByte(DS1302_DAY, DS1302_Time[6] / 10 * 16 + DS1302_Time[6] % 10);
	DS1302WriteByte(DS1302_WP, 0x80);
}

void DS1302GetTime(void)
{
	byte Temp;
	Temp = DS1302ReadByte(DS1302_YEAR);
	DS1302_Time[0] = Temp / 16 * 10 + Temp % 16;
	// BCD 2 DEC: DEC=BCD/16*10+BCD%16
	// DEC 2 BCD: BCD=DEC/10*16+DEC%10
	Temp = DS1302ReadByte(DS1302_MONTH);
	DS1302_Time[1] = Temp / 16 * 10 + Temp % 16;
	Temp = DS1302ReadByte(DS1302_DATE);
	DS1302_Time[2] = Temp / 16 * 10 + Temp % 16;
	Temp = DS1302ReadByte(DS1302_HOUR);
	DS1302_Time[3] = Temp / 16 * 10 + Temp % 16;
	Temp = DS1302ReadByte(DS1302_MINUTE);
	DS1302_Time[4] = Temp / 16 * 10 + Temp % 16;
	Temp = DS1302ReadByte(DS1302_SECOND);
	DS1302_Time[5] = Temp / 16 * 10 + Temp % 16;
	Temp = DS1302ReadByte(DS1302_DAY);
	DS1302_Time[6] = Temp / 16 * 10 + Temp % 16;
}

#endif
