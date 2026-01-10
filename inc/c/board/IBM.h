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
	IRQ_Cascade = 0x22, // 8259A Cascade --> _i8259A_SLV
	IRQ_Serial = 0x23, // default ethernet interrupt vector, or RS232 interrupt vector for port 2 {??? 16450 Serial Port}
	IRQ_RS232_Port1 = 0x24, //  RS232 interrupt vector for port 1
	IRQ_XT_WINI = 0x25, // ???
	IRQ_Floppy = 0x26, // 8250 Floppy Disk Controller
	IRQ_LPT1 = 0x27, // 8255 Parallel Port
#define _i8259A_SLV     0XA0
#define _i8259A_SLV_IMR 0XA1
#define _i8259A_SLV_IDSTART     0X70
	IRQ_RTC = 0x70,// Realtime Clock
	IRQ_0x71 = 0x71,
	IRQ_0x72 = 0x72,
	IRQ_0x73 = 0x73,
	IRQ_PS2_Mouse = 0x74,// PS2 Mouse
	IRQ_Coprocessor = 0x75,// FPU / Coprocessor / Inter-processor
	IRQ_ATA_DISK0 = 0x76,// Primary ATA Hard Disk
	IRQ_ATA_DISK1 = 0x77,// Secondary ATA Hard Disk
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
	DEV_MAS_LPT1 = IRQ_LPT1 - 0x20,
} Request_Master_t;
typedef enum
{
	DEV_SLV_RTC = IRQ_RTC - 0x70,
	DEV_SLV_0x71 = IRQ_0x71 - 0x70,
	DEV_SLV_0x72 = IRQ_0x72 - 0x70,
	DEV_SLV_0x73 = IRQ_0x73 - 0x70,
	DEV_SLV_PS2_Mouse = IRQ_PS2_Mouse - 0x70,
	DEV_SLV_Coprocessor = IRQ_Coprocessor - 0x70,
	DEV_SLV_ATA_DISK0 = IRQ_ATA_DISK0 - 0x70,
	DEV_SLV_ATA_DISK1 = IRQ_ATA_DISK1 - 0x70,
} Request_Slave_t;



#define PORT_PIT_TIMER0     0x40 // I/O port for timer channel 0
#define PORT_PIT_TIMER1     0x41 // I/O port for timer channel 1
#define PORT_PIT_TIMER2     0x42 // I/O port for timer channel 2
#define PORT_TIMER_MODE     0x43 // I/O port for timer mode control

// AT keyboard
// 8042 controller
#define PORT_KBD_BUFFER  0x60 // R:Buffer W:Buffer(8042 Data&8048 Command)
#define KEYBOARD_DAT     0x60 // R(Buffer), W(Buffer, 8042 Data & 8048 Command)
#define PORT_SPEAKER     0x61
#define KEYBOARD_CMD     0x64 // R(Status), W(8042 Command)

#define PORT_CMOS_ADDR 0x70
#define PORT_CMOS_DATA 0x71




// [CMOS] ---- 0x70~0x71 ----

#define CMOS_SECOND 0x00  // (0 ~ 59)
#define CMOS_SECOND_ALARM 0x01
#define CMOS_MINUTE 0x02  // (0 ~ 59)
#define CMOS_MINUTE_ALARM 0x03 
#define CMOS_HOUR   0x04  // (0 ~ 23)
#define CMOS_HOUR_ALARM 0x05
//
#define CMOS_WDAY  0x06   // (1 ~ 7), 1 for Sunday
#define CMOS_MDAY  0x07   // (1 ~ 31)
#define CMOS_MONTH 0x08   // (1 ~ 12)
#define CMOS_YEAR  0x09   // (0 ~ 99)
//
#define CMOS_STATUS_A 0x0A
#define CMOS_STATUS_B 0x0B
#define CMOS_STATUS_C 0x0C
#define CMOS_STATUS_D 0x0D
#define CMOS_POST_DIAGNOSTIC 0x0E
#define CMOS_SHUT_DIAGNOSTIC 0x0F
#define CMOS_FLOPPY_TYPE     0x10
#define CMOS_SYS_CONFIG      0x11
#define CMOS_HDD_TYPE        0x12
#define CMOS_EQUIPMENT       0x14
#define CMOS_BASE_MEM_LOW    0x15
#define CMOS_BASE_MEM_HIGH   0x16
#define CMOS_EXT_MEM_LOW     0x17
#define CMOS_EXT_MEM_HIGH    0x18
#define CMOS_HDD_1_TYPE      0x19
#define CMOS_HDD_2_TYPE      0x1A
#define CMOS_EXT_MEM_2_LOW   0x30
#define CMOS_EXT_MEM_2_HIGH  0x31
#define CMOS_CENTURY 0x32
#define CMOS_NMI 0x80











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
