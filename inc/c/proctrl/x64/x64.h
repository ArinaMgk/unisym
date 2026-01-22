// ASCII C99 TAB4 CRLF
// Attribute: Little-Endian(Byte, Bit)
// AllAuthor: @ArinaMgk
// ModuTitle: General Header for x64 CPU

#include "../../stdinc.h"
#ifndef _INC_X64
#define _INC_X64

#include "../IAx86_64.h"

#ifdef _INC_CPP

static inline void Descriptor64SetCode(descriptor_t* gdte, _CPU_descriptor_type typ, byte privilege_level)
{
	gdte->setRange(0, 0 _Comment(or 0xFFFFF));
	gdte->typ = typ;
	gdte->notsys = 1;
	gdte->DPL = privilege_level;
	gdte->present = 1;
	gdte->Mod64 = 1;
	gdte->DB = 0;
	gdte->granularity = 1;
}
static inline void Descriptor64SetData(descriptor_t* gdte, _CPU_descriptor_type typ, byte privilege_level)
{
	gdte->setRange(0, 0 _Comment(or 0xFFFFF));
	gdte->typ = typ;
	gdte->notsys = 1;
	gdte->DPL = privilege_level;
	gdte->present = 1;
	gdte->Mod64 = 0;
	gdte->DB = 1;
	gdte->granularity = 1;
}


#endif


// ---- interrupt.asm ----

void sendEOI(void);

#endif
