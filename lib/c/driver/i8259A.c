// ASCII C TAB4 CRLF
// Attribute: Bits(32)
// LastCheck: 20240219
// AllAuthor: @dosconio
// ModuTitle: PIC - Intel 8259A Driver
// Copyright: Dosconio Mecocoa, BSD 3-Clause License &
//            ArinaMgk UniSym, Apache License Version 2.0
// BaseOn   : unisym/lib/asm/x86/interrupt/x86_i8259A.asm

#include "../../../inc/c/driver/i8259A.h"

#ifdef _SUPPORT_Port8

void i8259A_init(const struct _i8259A_ICW* inf)
{
	word port = inf->port;
	outpb(port, valword(inf->ICW1));
	outpb(port + 1, valword(inf->ICW2));
	outpb(port + 1, valword(inf->ICW3));
	if (inf->ICW1.ICW4_USED) outpb(port + 1, valword(inf->ICW4));
}

#endif
