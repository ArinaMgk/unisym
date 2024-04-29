//TEMP only for mecocoa
//TEMP merge by Macro-switch

#include "../../../../inc/c/stdinc.h"

#if defined(ADDR_CountSeconds) && defined(_MCCA) 

// depend(lib/asm/x86/inst/manage.asm)
void delay001s()
{
	stduint i = *(stduint*)ADDR_CountSeconds;
	while (*(stduint*)ADDR_CountSeconds == i) HALT();
}

#endif
