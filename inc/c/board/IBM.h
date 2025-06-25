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
#define BYTE_EOI 0x20

// ---- ---- ---- ---- INTERRUPT ---- ---- ---- ----
// same (compatible) with the younger boards: AT ATX ITX (?)


typedef enum
{
// ---- x86 Instuction Set Architecture Definitions
	// The former 20 is for exceptions request handlers
	ERQ_Divide_By_Zero = 0,
	ERQ_Step = 1,
	ERQ_NMI = 2,
	ERQ_Breakpoint = 3,
	ERQ_Overflow = 4,
	ERQ_Bound = 5,
	ERQ_Invalid_Opcode = 6,
	ERQ_Coprocessor_Not_Available = 7,
	ERQ_Double_Fault = 8,
	ERQ_Coprocessor_Segment_Overrun = 9,
	ERQ_Invalid_TSS = 10,
	ERQ_x0B = 11,
	ERQ_x0C = 12,
	ERQ_x0D = 13,
	ERQ_Page_Fault = 14,
	ERQ_x0F = 15,
	// The latter 16 is for IRQ request handlers
#define _i8259A_MAS     0X20
#define _i8259A_MAS_IMR 0X21
	IRQ_PIT = 0x20, // 8253/8254 PIT (Programmable Interval Timer)
	IRQ_Keyboard = 0x21,
	IRQ_Cascade = 0x22, // 8259A Cascade
	IRQ_Serial = 0x23, // default ethernet interrupt vector, or RS232 interrupt vector for port 2 {??? 16450 Serial Port}
	IRQ_RS232_Port1 = 0x24, //  RS232 interrupt vector for port 1
	IRQ_XT_WINI = 0x25, // ???
	IRQ_Floppy = 0x26, // 8250 Floppy Disk Controller
	IRQ_LPT1 = 0x27, // 8255 Parallel Port
#define _i8259A_SLV     0XA0
#define _i8259A_SLV_IMR 0XA1
	IRQ_RTC = 0x70,// Realtime Clock
	IRQ_0x71 = 0x71,
	IRQ_0x72 = 0x72,
	IRQ_0x73 = 0x73,
	IRQ_0x74 = 0x74,
	IRQ_0x75 = 0x75,
	IRQ_0x76 = 0x76,
	IRQ_0x77 = 0x77,
// ---- IBM Specific Definitions


} Request_t;

typedef enum
{
	DEV_MAS_PIT = IRQ_PIT - 0x20, 
	DEV_MAS_Keyboard = IRQ_Keyboard - 0x20,
	DEV_MAS_Cascade = IRQ_Cascade - 0x20,
	DEV_MAS_Serial = IRQ_Serial - 0x20, 
	DEV_MAS_RS232_Port1 = IRQ_RS232_Port1 - 0x20, 
	DEV_MAS_XT_WINI = IRQ_XT_WINI - 0x20, 
	DEV_MAS_Floppy = IRQ_Floppy - 0x20, 
	_TODO
} Request_Master_t;


#define PORT_KBD_BUFFER 0x60 // R:Buffer W:Buffer(8042 Data&8048 Command)


#define PIT_TIMER0     0x40 // I/O port for timer channel 0
#define TIMER_MODE     0x43 // I/O port for timer mode control

#ifdef _INC_CPP
extern "C" {
#endif

// The former 20 is for exceptions request handlers
void Else_ERQHandler();

void Divide_By_Zero_ERQHandler();
void Step_ERQHandler();
void NMI_ERQHandler();
void Breakpoint_ERQHandler();
void Overflow_ERQHandler();
void Bound_ERQHandler();
void Invalid_Opcode_ERQHandler();
void Coprocessor_Not_Available_ERQHandler();
void Double_Fault_ERQHandler(dword ErrorCode);
void Coprocessor_Segment_Overrun_ERQHandler();
void Invalid_TSS_ERQHandler(dword ErrorCode);
void x0B_ERQHandler(dword ErrorCode);
void x0C_ERQHandler(dword ErrorCode);
void x0D_ERQHandler(dword ErrorCode);
void Page_Fault_ERQHandler(dword ErrorCode);
void x0F_ERQHandler();
void X87_FPU_Floating_Point_Error_ERQHandler();

// User Defined
void ERQ_Handler(sdword iden, dword para
#ifdef _INC_CPP
	= 0
#endif
);

#ifdef _INC_CPP
}
#endif



#endif
