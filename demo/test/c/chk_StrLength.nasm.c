// ASCII Haruno RFX(09) Binary(32)

// Build:
// - 

#include <stdio.h>
// Windows
__fastcall unsigned StrLength(const char* str);

int main()
{
	printf("len of \"\" zo 0x[%08X]\n", StrLength(""));
	printf("len of \"m\" zo 0x[%08X]\n", StrLength("m"));
	printf("len of \"ma\" zo 0x[%08X]\n", StrLength("ma"));
}
