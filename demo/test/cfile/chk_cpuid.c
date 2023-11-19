// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant yo Free bin^16+ CPU(80586+)
// LastCheck: 2023 Nov 16
// AllAuthor: @dosconio
// ModuTitle: Instruction CPUID

#include "../../../inc/c/cpuid.h"
#include <stdio.h>

char buf[1 + 4 * 4 * 3];

int main()
{
	// Show CPU Brand
	CpuBrand(buf);
	puts(buf);
}
