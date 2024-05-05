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
#include "../../../inc/c/driver/IIC.h"

#if defined(_MCU_Intel8051) && defined(_I2C_SCL) && defined(_I2C_SDA)

// sbit _I2C_SCL = P2 ^ 1;
// sbit _I2C_SDA = P2 ^ 0;

void I2C_Start(void)
{
	outpi(_I2C_SDA, 1);
	outpi(_I2C_SCL, 1);
	outpi(_I2C_SDA, 0);
	outpi(_I2C_SCL, 0);
}

void I2C_Stop(void)
{
	outpi(_I2C_SDA, 0);
	outpi(_I2C_SCL, 1);
	outpi(_I2C_SDA, 1);
}

void I2C_SetByte(unsigned char Byte)
{
	unsigned char i;
	for (i = 0;i < 8;i++)
	{
		outpi(_I2C_SDA, Byte & (0x80 >> i));
		outpi(_I2C_SCL, 1);
		outpi(_I2C_SCL, 0);
	}
}

unsigned char I2C_GetByte(void)
{
	unsigned char i, b = 0x00;
	outpi(_I2C_SDA, 1);
	for (i = 0;i < 8;i++)
	{
		outpi(_I2C_SCL, 1);
		if (innpi(_I2C_SDA)) { b |= (0x80 >> i); }
		outpi(_I2C_SCL, 0);
	}
	return b;
}

void I2C_SetAck(unsigned char AckBit)// *@param AckBit 0 is ACKnowledge
{
	outpi(_I2C_SDA, AckBit);
	outpi(_I2C_SCL, 1);
	outpi(_I2C_SCL, 0);
}

unsigned char I2C_GetAck(void)
{
	unsigned char AckBit;
	outpi(_I2C_SDA, 1);
	outpi(_I2C_SCL, 1);
	AckBit = innpi(_I2C_SDA);
	outpi(_I2C_SCL, 0);
	return AckBit;
}

#endif
