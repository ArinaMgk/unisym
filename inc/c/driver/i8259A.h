// ASCII C TAB4 CRLF
// Attribute: 
// LastCheck: 2024Mar05
// AllAuthor: @dosconio
// ModuTitle: 
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

#ifndef _INC_DEV_I8259A
#define _INC_DEV_I8259A

#include "../stdinc.h"
#include "../x86/interface.x86.h"

#define EOI ' '// 0x20, End of Interrupt

typedef struct _i8259A_ICW
{
	word port;
	struct
	{
		byte ICW4_USED : 1;
		byte CAS : 1;// for MAS and SLV
		byte ADI : 1;
		byte NotEdge : 1;
		byte ENA : 1; // [1]
		byte : 3; // [X]*3
	} ICW1;
	struct
	{
		byte IntNo;
	} ICW2;
	union
	{
		byte CasPortMap;// 8 bits Bit
		byte CasPortIdn;// 3 bits Hex
	} ICW3;
	struct
	{
		byte Not8b : 1;
		byte AutoEOI : 1;
		byte IsMas : 1;
		byte Buf : 1;
		byte SFNM : 1;
		byte : 3; // [X]*3
	} ICW4;
} _8259A_init_t;

void i8259A_init(const struct _i8259A_ICW* inf);

#endif
