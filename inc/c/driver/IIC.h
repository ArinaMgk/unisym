// ASCII CPL TAB4 CRLF
// Docutitle: (protocol) Inter-Integrated Circuit, I2C
// Datecheck: 20240429 ~ <Last-check>
// Developer: @dosconio
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
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

#if !defined(_INC_STANDARD_IIC)
#define _INC_STANDARD_IIC

#if defined(_MCU_Intel8051)

void I2C_Start(void);

void I2C_SetByte(unsigned char Byte);
unsigned char I2C_GetAck(void);

unsigned char I2C_GetByte(void);
void I2C_SetAck(unsigned char AckBit);

void I2C_Stop(void);

#endif

#endif
