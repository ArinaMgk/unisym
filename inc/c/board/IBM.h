// UTF-8 CPP-ISO11 TAB4 CRLF
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

#include "../stdinc.h"

#define _VIDEO_ADDR_BUFFER 0xB8000
#define BYTE_EOI 0x20

// ---- ---- ---- ---- INTERRUPT ---- ---- ---- ----
// same (compatible) with the younger boards: AT ATX ITX (?)

// from ISA Bus
typedef enum
{
// Less the number is, the higher the priority is.
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
	#define PIC_MAS_IDSTART     0X20
	IRQ_PIT = 0x20,      // 8253/8254 PIT (Programmable Interval Timer)
	IRQ_Keyboard = 0x21,
	IRQ_Cascade = 0x22,  // 8259A Cascade --> PORT_i8259A_SLV_A
	IRQ_Serial = 0x23,   // COM2 or default ethernet interrupt vector, or RS232 interrupt vector for port 2 {??? 16450 Serial Port}
	IRQ_RS232_P1 = 0x24, // COM1 or RS232 interrupt vector for port 1
	IRQ_XT_WINI = 0x25,  // or LPT2
	IRQ_Floppy = 0x26,   // 8250 Floppy Disk Controller
	IRQ_LPT1 = 0x27,     // 8255 Parallel Port
	#define PIC_SLV_IDSTART     0X70
	IRQ_RTC = 0x70,// Realtime Clock
	IRQ_0x71 = 0x71,
	IRQ_0x72 = 0x72,
	IRQ_0x73 = 0x73,
	IRQ_PS2_Mouse = 0x74,  // PS2 Mouse
	IRQ_Coprocessor = 0x75,// FPU / Coprocessor / Inter-processor
	IRQ_ATA_DISK0 = 0x76,  // Primary ATA Hard Disk
	IRQ_ATA_DISK1 = 0x77,  // Secondary ATA Hard Disk
// ---- IBM Specific Definitions


} Request_t;

typedef enum
{
	DEV_MAS_PIT = IRQ_PIT - 0x20, 
	DEV_MAS_Keyboard = IRQ_Keyboard - 0x20,
	DEV_MAS_Cascade = IRQ_Cascade - 0x20,
	DEV_MAS_Serial = IRQ_Serial - 0x20, 
	DEV_MAS_RS232_Port1 = IRQ_RS232_P1 - 0x20, 
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

enum _PORT {
	//{} 0x0000 - 0x000F	DMA (8237A)

	// PIC i8259A Master
	PORT_i8259A_MAS_A = 0X20,
	PORT_i8259A_MAS_B = 0X21,

	// PIT i8254A
	PORT_PIT_TIMER0 = 0x40, // timer channel 0
	PORT_PIT_TIMER1 = 0x41, // timer channel 1
	PORT_PIT_TIMER2 = 0x42, // timer channel 2
	PORT_TIMER_MODE = 0x43, // timer mode control

	// 8042 PS/2 and Speaker
	PORT_KEYBOARD_DAT = 0x60, // R:Buffer W:Buffer(8042 Data&8048 Command)
	PORT_SPEAKER = 0x61,
	PORT_KEYBOARD_CMD = 0x64, // R(Status), W(8042 Command)

	// CMOS / RTC
	PORT_CMOS_ADDR = 0x70,  // CMOS address register
	PORT_CMOS_DATA = 0x71,  // CMOS data register

	//{} 0x0080 - 0x008F	DMA Page

	// PIC i8259A Slave
	PORT_i8259A_SLV_A = 0XA0,
	PORT_i8259A_SLV_B = 0XA1,

	//{} 0x0170 - 0x0177	第二IDE通道
	//{} 0x01F0 - 0x01F7	第一IDE通道
	//{} 0x02E8, 0x02E9	COM4（串口4）
	//{} 0x02F8, 0x02F9	COM2（串口2）

	// CRT Control Registers
	PORT_CRT_CRAR = 0x03D4, // CRT Control Address Register
	PORT_CRT_DRAR = 0x03D5, // CRT Control Data Register

	//{} 0x03B0 - 0x03BB	MDA（单显适配器）
	//{} 0x03C0 - 0x03CF	EGA/VGA
	//{} 0x03D0 - 0x03DF	CGA（彩显图形适配器）
	//{} 0x03E8, 0x03E9	COM3（串口3）
	//{} 0x03F8, 0x03F9	COM1（串口1）
	//{} 0x0CF8, 0x0CFC	PCI配置空间访问
	//{} 0x0378, 0x0379	LPT1（并口1）
};

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

// at 0x00400
// 20250802 fo https://stanislavs.org/helppc/bios_data_area.html
_PACKED(struct) BIOS_DataArea {
	// 0x00
	word port_address_COM1;
	word port_address_COM2;
	word port_address_COM3;
	word port_address_COM4;
	word port_address_LPT1;
	word port_address_LPT2;
	word port_address_LPT3;
	word port_address_LPT4;// (except PS/2)
	// 0x10
	byte equipment_list_flags[2];//{} (see INT 11)
	byte PCjr;// infrared keyboard link error count
	word memory_size;// KB (see INT 12)
	byte RESERVED_0;
	byte PS2_BIOS_cflag;// PS/2 BIOS control flags
	byte keyboard_flags[2];//{}
	byte keypad_entry;// Storage for alternate keypad entry
	word kbd_buff_head;// Offset from 40:00 to keyboard buffer head
	word kbd_buff_tail;// Offset from 40:00 to keyboard buffer tail
	byte kbd_buff[32];// (circular queue buffer)
	byte drv_recalibration_status;// Drive recalibration status
	byte diskette_motor_status;// Diskette motor status
	// 0x40
	byte motor_shutoff_counter;// Motor shutoff counter (decremented by INT 8)
	byte diskette_operation_status;// Status of last diskette operation (see INT 13,1)
	byte NEC_diskette_cflags[7];// NEC diskette controller status (see FDC)
	byte crt_video_mode;// (see VIDEO MODE)
	word screen_columns;
	word crt_video_buflen;// Size of current video regen buffer in bytes
	word crt_video_bufptr;// Offset of current video page in video regen buffer
	// 0x50
	word curposis[8];// Cursor position of pages 1-8, high order byte=row low order byte=column; changing this data isn't reflected immediately on the display
	// 0x60
	byte cur_scanline_btm;// Cursor ending (bottom) scan line (don't modify)
	byte cur_scanline_top;// Cursor starting (top) scan line (don't modify)
	byte crt_video_pgnumber;// Active display page number
	word port_address_6845;// Port address of 6845 CRT controller (3B4h = mono, 3D4h = color)
	byte cr_6845;// 6845 CRT mode control register value (port 3x8h); EGA/VGA values emulate those of the MDA/CGA
	byte crt_CGA_color_palette;// CGA current color palette mask setting (port 3d9h); EGA and VGA values emulate the CGA
	_PACKED(union) {
		dword pointer_back_pe;// CS:IP for 286 return from protected mode
		dword pointer_shutdown;// Temp storage for SS:SP during shutdown
		dword days;// Day counter on all products after AT
		dword reset_code_ps2;// PS/2 Pointer to reset code with memory preserved
		byte cassette_tape_controls[5];// Cassette tape control (before AT)
	} pointers;
	dword count_daily_timer;// Daily timer counter, equal to zero at midnight; incremented by INT 8; read/set by INT 1A
	// 0x70
	byte clock_rollover_flag;// Clock rollover flag, set when 40:6C exceeds 24hrs
	byte BIOS_break_code;// BIOS break flag, bit 7 is set if Ctrl-Break was *ever* hit; set by INT 9
	word soft_reset_flag;// via Ctl-Alt-Del or JMP FFFF:0
	//	1234h  Bypass memory tests & CRT initialization
	//	4321h  Preserve memory
	//	5678h  System suspend
	//	9ABCh  Manufacturer test
	//	ABCDh  Convertible POST loop
	//	????h  many other values are used during POST
	byte hdisk_opstatus;// Status of last hard disk operation (see INT 13,1)
	byte hdisk_number;// Number of hard disks attached

	_TODO
};

// ---- ---- VGA

enum {
	CRT_CDR_HorizonalTotal,
	CRT_CDR_HorizonalDisplayEnd,
	CRT_CDR_HorizonalBlankingStart,
	CRT_CDR_HorizonalBlankingEnd,
	CRT_CDR_HorizonalRetraceStart,
	CRT_CDR_HorizonalRetraceEnd,
	//
	CRT_CDR_VerticalTotal,
	CRT_CDR_Overflow,
	CRT_CDR_PresetRowScan,// 0x08
	CRT_CDR_MaxScanLine,
	CRT_CDR_CursorStart,
	CRT_CDR_CursorEnd,
	CRT_CDR_StartAddressHigh,
	CRT_CDR_StartAddressLow,
	CRT_CDR_CursorLocationHigh,
	CRT_CDR_CursorLocationLow,
	CRT_CDR_VerticalRetraceStart,// 0x10
	CRT_CDR_VerticalRetraceEnd,
	CRT_CDR_DisplayEnd,
	CRT_CDR_Offset,
	CRT_CDR_UnderlineLocation,
	CRT_CDR_VerticalBlankingStart,
	CRT_CDR_VerticalBlankingEnd,
	CRT_CDR_ModeControl,// CRTC Mode Control Register 0x17
	CRT_CDR_LineCompare,
};// CRT Controller Data Registers



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
