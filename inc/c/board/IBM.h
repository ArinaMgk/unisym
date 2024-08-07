// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Board) IBM Compatible 
// Codifiers: @dosconio: 20240804
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

#ifndef _INC_Board_IBMCompatible
#define _INC_Board_IBMCompatible

#define _VIDEO_ADDR_BUFFER 0xB8000

typedef enum
{
	IRQ_PIT = 0, // 8253/8254 PIT (Programmable Interval Timer)
	IRQ_Keyboard = 1, // 8042? Keyboard Controller
	IRQ_Cascade = 2, // 8259A Cascade
	IRQ_Serial = 3, // default ethernet interrupt vector, or RS232 interrupt vector for port 2 {??? 16450 Serial Port}
	IRQ_RS232_Port1 = 4, //  RS232 interrupt vector for port 1
	IRQ_XT_WINI = 5, // ???
	IRQ_Floppy = 6, // 8250 Floppy Disk Controller
	_TODO
} Request_Master_t;

#define _i8259A_MAS     0X20
#define _i8259A_MAS_IMR 0X21
#define _i8259A_SLV     0XA0
#define _i8259A_SLV_IMR 0XA1

#define PORT_KBD 0x21
#define PORT_KBD_BUFFER 0x60 // R:Buffer W:Buffer(8042 Data&8048 Command)

#define PORT_RTC 0x70

#define PIT_TIMER0     0x40 // I/O port for timer channel 0
#define TIMER_MODE     0x43 // I/O port for timer mode control

#endif
