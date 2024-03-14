//TEMP only for mecocoa
#ifdef ADDR_CountSeconds

#include "../../../../inc/c/alice.h"
#include "../../../../inc/c/x86/interface.x86.h"

// depend(lib/asm/x86/inst/manage.asm)
void delay001s()
{
	stduint i = *(stduint*)ADDR_CountSeconds;
	while (*(stduint*)ADDR_CountSeconds == i) HALT();
}

#endif
