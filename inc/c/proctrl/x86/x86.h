// ASCII C99 TAB4 CRLF
// Attribute: Little-Endian(Byte, Bit)
// LastCheck: 20240216
// AllAuthor: @dosconio
// ModuTitle: General Header for x86 CPU

#include "../../stdinc.h"
#ifndef _INC_X86
#define _INC_X86

#include "../IAx86_64.h"

#ifdef _INC_CPP

static inline void Descriptor32Set(descriptor_t* gdte, dword base, dword limit, _CPU_descriptor_type typ, byte DPL, byte not_sys, byte db, byte gran)
{
	gdte->setRange(base, limit);
	gdte->typ = typ;
	gdte->notsys = not_sys;
	gdte->DPL = DPL;
	gdte->present = 1;
	gdte->DB = db;
	gdte->granularity = gran;
}

extern "C" {
#endif


// ---- lib/asm/x86/inst/manage.asm ----

static inline
void PagingEnable(void) {
	__asm("movl %cr0, %eax\n");
	__asm("or   $0x80000000, %eax\n");// enable paging
	__asm("movl %eax, %cr0\n");
}

// ---- ---- JMPs ---- ---- //
void jmpTask(dword tss_no/*, dword old_no*/);// <=> advanced JMP GDT:TSS
void CallFar(dword offs, dword selc);// <=> callf : call far

void returnfar(void);// <=> retf

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



#ifdef _INC_CPP
}
#endif
#endif
