// ASCII C TAB4 CRLF
// Attribute: Freq(11.0592MHz)
// LastCheck: 2024Mar04
// AllAuthor: @dosconio
// ModuTitle: 8051 MCU Delay 
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

