// TAB CRLF

#ifndef _INTERFACE_X86_H
#define _INTERFACE_X86_H

#include "../alice.h"

// lib/asm/x86/inst/ioport.asm
void OUT_b(word Port, word Data);
word IN_b(word Port);
void OUT_w(word Port, word Data);
word IN_w(word Port);

// lib/asm/x86/inst/manage.x86.asm
void HALT(void);
void InterruptEnable(void);
void InterruptDisable(void);

#endif // !_INTERFACE_X86_H
