// TAB CRLF
//{TODO} CPU-unlimited
#ifndef _INTERFACE_X86_H
#define _INTERFACE_X86_H

#include "../stdinc.h"

// lib/asm/x86/inst/ioport.asm
#include "../port.h"
void OUT_b(word Port, byte Data);
word IN_b(word Port);
void OUT_w(word Port, word Data);
word IN_w(word Port);

// lib/asm/x86/inst/manage.asm
void HALT(void);
void InterruptEnable(void);
void InterruptDisable(void);
void InterruptDTabLoad(void* addr);

// lib/asm/x86/inst/interrupt.asm
void returni(void);// for C

// lib/asm/x86/inst/stack.asm
void pushad(void);
void popad(void);
void pushfd(void);
void popfd(void);

#endif // !_INTERFACE_X86_H
