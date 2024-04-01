// ASCII C99 TAB4 LF
// Attribute: 
// LastCheck: 
// AllAuthor: @dosconio
// ModuTitle: Magice Compiler
// Copyright: ArinaMgk UniSym, Apache License Version 2.0

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		fprintf(stderr, "%s: invalid count of arguments\n", argv[0]);
		return 1;
	}
	puts(".global main # default entry");// "GLOBAL"
	puts("main:");
	printf("\tli a0, %d\n", atoi(argv[1]));// "mov eax, %d"
	puts("\tret");
}
