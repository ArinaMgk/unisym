// ASCII C99 TAB4 CRLF
// Attribute: Little-Endian(Byte, Bit)
// LastCheck: 20240216
// AllAuthor: @dosconio
// ModuTitle: General Header for x86 CPU

#include "../../stdinc.h"
#ifndef _INC_X86
#define _INC_X86

enum _CPU_x86_descriptor_type
{
	_Dptr_TSS286_Available = 1,
	_Dptr_LDT = 2,
	_Dptr_TSS286_Busy = 3,
	_Dptr_CallGate286 = 4,
	_Dptr_TaskGate = 5,
	_Dptr_InterruptGate286 = 6,
	_Dptr_TrapGate286 = 7,
	//
	_Dptr_TSS386_Available = 9,
	_Dptr_TSS386_Busy = 0xB,
	_Dptr_CallGate386 = 0xC,
	_Dptr_InterruptGate386 = 0xE,
	_Dptr_TrapGate386 = 0xF,
};
typedef struct _CPU_x86_descriptor
{
	word limit_low;
	word base_low;
	byte base_middle;
	byte typ : 4;
	byte notsys : 1;
	byte DPL : 2;
	byte present : 1;
	byte limit_high : 4;
	byte available : 1;
	byte Mod64 : 1;
	byte DB : 1; // 32-bitmode
	byte granularity : 1; // 4k-times
	byte base_high;
#ifdef _INC_CPP
	void setRange(dword addr, dword limit) {
		limit_low = limit;
		limit_high = limit >> 16;
		base_low = addr;
		base_middle = addr >> 16;
		base_high = addr >> 24;
	}
#endif
} descriptor_t;

// return nothing
static inline void GlobalDescriptor32Set(descriptor_t* gdte, dword base, dword limit, byte typ, byte DPL, byte not_sys, byte db, byte gran)
{
	gdte->limit_low = limit & 0xFFFF;
	gdte->limit_high = (limit >> 16) & 0xF;
	gdte->base_low = base & 0xFFFF;
	gdte->base_middle = (base >> 16) & 0xFF;
	gdte->base_high = (base >> 24) & 0xFF;
	gdte->typ = typ;
	gdte->notsys = not_sys;
	gdte->DPL = DPL;
	gdte->present = 1;
	gdte->DB = db;
	gdte->granularity = gran;
}

typedef struct _CPU_x86_gate
{
	word offset_low;
	word selector;
	byte param_count : 5;
	byte zero : 3;
	byte type : 4;
	byte notsys : 1;
	byte DPL : 2;
	byte present : 1;
	word offset_high;
#ifdef _INC_CPP
	void setRange(dword addr, word segsel) {
		offset_low = addr;
		offset_high = addr >> 16;
		selector = segsel;
	}
	// default (1_11_01100_00000000)
	// - zero parameter
	// - ring 3
	void setModeCall(dword addr, word segsel) {
		setRange(addr, segsel);
		param_count = 0;
		zero = 0;
		type = 0b1100;
		notsys = 0;
		DPL = 3;
		present = 1;
	}
#endif
} gate_t;

#ifdef _INC_CPP
extern "C" {
#endif

static inline dword DescriptorBaseGet(descriptor_t* desc)
{
	return desc->base_low | (desc->base_middle << 16) | (desc->base_high << 24);
}

static inline gate_t* GateStructInterruptR0(gate_t* gate, dword addr, word segm, byte paracnt)
{
	gate->offset_low = addr & 0xFFFF;
	gate->selector = segm;
	gate->param_count = paracnt;
	gate->type = 0xE;// interrupt gate
	gate->notsys = 0;
	gate->DPL = 0;
	gate->present = 1;
	gate->offset_high = (addr >> 16) & 0xFFFF;
	return gate;
}

// lib/asm/x86/inst/inst.asm
int setA32(int);

// lib/asm/x86/inst/ioport.asm
void OUT_b(word Port, byte Data);
word IN_b(word Port);
void OUT_w(word Port, word Data);
word IN_w(word Port);
#define outpi// Out to Port's Pin
#define outpb OUT_b
#define outpw OUT_w
#define innpi// In from Port's Pin
#define innpb IN_b
#define innpw IN_w

// ---- lib/asm/x86/inst/manage.asm ----
void HALT(void);

void InterruptEnable(void);
void InterruptDisable(void);
static inline void enInterrupt(int enable) {
	if (enable)
		InterruptEnable();
	else
		InterruptDisable();
}

void InterruptDTabLoad(void* addr);

inline static void loadIDT(uint32 address, uint16 length) {
	_PACKED(struct) { uint16 u_16fore; uint32 u_32back; } tmp48_le;
	tmp48_le.u_32back = address;
	tmp48_le.u_16fore = length;
	InterruptDTabLoad(&tmp48_le);// AAS(LIDT tmp48_le), GAS("lidt %0" :: "m" (tmp48_le))
}
dword getCR3();
dword getEflags();
void jmpFar(dword offs, dword selc);//{TODO} JumpFar
void CallFar(dword offs, dword selc);
void returnfar(void);

void TaskReturn();

// ---- lib/asm/x86/inst/interrupt.asm ----
void _returni(void);// for C

// ---- lib/asm/x86/inst/stack.asm ----
void _pushad(void);
void _popad(void);
void _pushfd(void);
void _popfd(void);

// ---- lib/c/processor/x86/delay.c ----
void delay001s();
void delay001ms();

// ---- ---- VGA

// ---- CRT Control Registers
#define CRT_CR_AR 0x03D4 // Address Register
#define CRT_CR_DR 0x03D5 // Data Register

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


// 0x00400
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


#ifdef _INC_CPP
}
#endif
#endif
