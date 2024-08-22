//TEMP only for mecocoa
//TEMP merge by Macro-switch

#include "../../../../inc/c/stdinc.h"

#if defined(_MCCA) && (_MCCA==0x8616||_MCCA==0x8632)

//: external linkage
dword* ADDR_CountSeconds = (dword*)(0x500 + 0x24);
word* ADDR_CountMSeconds = (word*)(0x500 + 0x28);

// depend(lib/asm/x86/inst/manage.asm)
void delay001s()
{
	dword i = *ADDR_CountSeconds;
	while (*ADDR_CountSeconds == i) HALT();
}

void delay001ms()
{
	word i = *ADDR_CountMSeconds;
	while (*ADDR_CountMSeconds == i) HALT();
}
#else


#endif
