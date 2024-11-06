// ASCII C-SDCC TAB4 CRLF
// Attribute: 
// LastCheck: <date>
// AllAuthor: ...
// ModuTitle: ...
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

#include "../../inc/c/stdinc.h"

#if defined(_MCU_Intel8051)
#include "../../inc/c/MCU/Intel/i8051.h"
#include "../../inc/c/MCU/port.h"

void outpohi(byte port, byte pinidn)
{
	switch(port)
	{
		case 1:
			p1 |= pinidn;
			break;
		case 2:
			p2 |= pinidn;
			break;
		case 3:
			p3 |= pinidn;
			break;
	}
}

void outpolo(byte port, byte pinidn)
{
	switch(port)
	{
		case 1:
			p1 &= ~pinidn;
			break;
		case 2:
			p2 &= ~pinidn;
			break;
		case 3:
			p3 &= ~pinidn;
			break;
	}
}

void outpb(byte port, byte pinsmap)
{
	switch(port)
	{
		case 1:
			p1 = pinsmap;
			break;
		case 2:
			p2 = pinsmap;
			break;
		case 3:
			p3 = pinsmap;
			break;
	}
}

byte innpb(byte port)
{
	switch(port)
	{
		case 1:
			return p1;
		case 2:
			return p2;
		case 3:
			return p3;
	}
	return 0xA5;// from 0xAA55
}

#endif
