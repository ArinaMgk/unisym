//TEMP only for mecocoa
//TEMP merge by Macro-switch

#include "../../../../inc/c/stdinc.h"

#if defined(_MCCA) && (_MCCA==0x8616||_MCCA==0x8632)
#if defined(ADDR_CountSeconds)

// depend(lib/asm/x86/inst/manage.asm)
void delay001s()
{
	stduint i = *(stduint*)ADDR_CountSeconds;
	while (*(stduint*)ADDR_CountSeconds == i) HALT();
}

void delay001ms()
{
	word* milsec_p = (word*)((stduint)ADDR_CountSeconds + sizeof(dword));
	word i = *milsec_p;
	while (*milsec_p == i) HALT();
}

#endif
#endif
