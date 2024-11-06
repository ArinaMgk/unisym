// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Module) Delay or Sleep
// Codifiers: @dosconio: 20240830 ~ <Last-check> 
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



//{TODO} sleep() function

#include "../../inc/c/stdinc.h"

#if defined(_MCCA) && (_MCCA==0x8616||_MCCA==0x8632)

//: external linkage
dword* ADDR_CountSeconds = (dword*)(0x500 + 0x24);
word* ADDR_CountMSeconds = (word*)(0x500 + 0x28);

// depend(lib/asm/x86/inst/manage.asm)
void delay001s()
{
	dword i = *ADDR_CountSeconds;
	while (*ADDR_CountSeconds == i) HALT();
}

void delay001ms()
{
	word i = *ADDR_CountMSeconds;
	while (*ADDR_CountMSeconds == i) HALT();
}
#else


#endif
#if defined(_MCU_Intel8051) && _SYS_FREQ == 11059200
#define nop _nop_
extern void nop(void);

void delay010us()
{
	unsigned char i;
	i = 2;
	while (--i);
}

void delay100us()
{
	unsigned char i;
	nop();
	i = 43;
	while (--i);
}

void delay500us()
{
	unsigned char i;
	nop();
	i = 227;
	while (--i);
}

void delay001ms()
{
	unsigned char i, j;
	nop();
	i = 2;
	j = 199;
	do
	{
		while (--j);
	} while (--i);
}

#endif
