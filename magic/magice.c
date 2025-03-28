// ASCII CPL-C99 TAB4 LF
// Docutitle: Magice Compiler
// Datecheck: 20240416 ~ .
// Developer: @dosconio, @ArinaMgk
// Attribute: [Allocate]
// Reference: None
// Dependens: None
// Descriptn: .mgc -> {.mid / .mak} for {.exe / .html}
// Copyright: UNISYM, under Apache License 2.0
/*
	Copyright 2023 ArinaMgk

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0
	http://unisym.org/license.html

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/

#include "../inc/c/stdinc.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "../inc/c/compile/asmcode.h"

long int strtol(const char *str, char **endptr, int base);


enum Architecture_t platform = Architecture_RISCV64;
// AASM Architecture_x86     
// GAS  Architecture_RISCV64

//[Update] Accept + and - operators

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		fprintf(stderr, "%s: invalid count of arguments\n", argv[0]);
		return 1;
	}
	printf("%s%s\n", _AUT_HEAD[platform], _AUT_ARCHITECT[platform]);
	puts(_ASM_GLOBAL[platform]); // GLOBAL main
	puts("main:");

	// make into structure: num (op num) (op num)...
	printf("\t");
	char *p = argv[1];
	printf(_ASM_MOV[platform], strtol(p, &p, 10));

	while (*p)
	{
		if (*p == '+')
		{
			++p; // add its operand
			printf("\t");
			printf(_ASM_ADD[platform], strtol(p, &p, 10));
		}
		else if (*p == '-')
		{
			++p; // add its negative operand
			printf("\t");
			printf(_ASM_ADD[platform], -strtol(p, &p, 10));
		}
		else if (isspace(*p))
			++p;
		else
		{
			fprintf(stderr, "%s: invalid character '%c'\n", argv[0], *p);
			return 1;
		}
	}
	printf("\t");
	puts(_ASM_RET[platform]);
}
