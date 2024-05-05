// ASCII CPL TAB4 CRLF
// Docutitle: Inter-Integrated Circuit, I2C
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

#include "../../../inc/c/stdinc.h"
#include "../../../inc/c/driver/OneWire.h"

#if defined(_MCU_Intel8051) && defined(_OneWire_DQ)

unsigned char OneWire_Init(void)
{
	unsigned char i;
	unsigned char AckBit;
	pIE_EA = 0;
	outpi(_OneWire_DQ, 1);
	outpi(_OneWire_DQ, 0);
	i = 247;while (--i);		//Delay 500us
	outpi(_OneWire_DQ, 1);
	i = 32;while (--i);			//Delay 70us
	AckBit = innpi(_OneWire_DQ);
	i = 247;while (--i);		//Delay 500us
	pIE_EA = 1;
	return AckBit;// response-bit of slave，0 is response，1 is not
}

void OneWire_SetBit(unsigned char Bit)
{
	unsigned char i;
	pIE_EA = 0;
	outpi(_OneWire_DQ, 0);
	i = 4;while (--i);			//Delay 10us
	outpi(_OneWire_DQ, Bit);
	i = 24;while (--i);			//Delay 50us
	outpi(_OneWire_DQ, 1);
	pIE_EA = 1;
}

unsigned char OneWire_GetBit(void)
{
	unsigned char i;
	unsigned char Bit;
	pIE_EA = 0;
	outpi(_OneWire_DQ, 0);
	i = 2;while (--i);			//Delay 5us
	outpi(_OneWire_DQ, 1);
	i = 2;while (--i);			//Delay 5us
	Bit = innpi(_OneWire_DQ);
	i = 24;while (--i);			//Delay 50us
	pIE_EA = 1;
	return Bit;
}

void OneWire_SetByte(unsigned char Byte)
{
	unsigned char i;
	for (i = 0;i < 8;i++)
	{
		OneWire_SetBit(Byte & (0x01 << i));
	}
}

unsigned char OneWire_GetByte(void)
{
	unsigned char i;
	unsigned char Byte = 0x00;
	for (i = 0;i < 8;i++)
	{
		if (OneWire_GetBit()) { Byte |= (0x01 << i); }
	}
	return Byte;
}


#endif
