// ASCII CPL-C99 TAB4 LF
// Docutitle: Magice Compiler
// Datecheck: 20240416 ~ .
// Developer: @dosconio @ UNISYM
// Attribute: [Allocate]
// Reference: None
// Dependens: None
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

#include <stdio.h>
#include <stdlib.h>
#include "../inc/c/host.h"

//{} Style: Intel-ASM

static const char *_ASM_GLOBAL[] = {
	"GLOBAL main ; default entry",
	".global main # default entry",
};
static const char *_ASM_MOV[] = {
	"mov eax, %d",
	"li a0, %d",
};

// [ARCH]
// AASM Architecture_x86     [temp: unchk]
// GAS  Architecture_RISCV64

//[Update] Accept + and - operators

int main(int argc, char **argv)
{
	enum Architecture_t platform = Architecture_RISCV64;
	if (argc != 2)
	{
		fprintf(stderr, "%s: invalid count of arguments\n", argv[0]);
		return 1;
	}
	puts(_ASM_GLOBAL[platform]);// "GLOBAL"
	puts("main:");
	printf("\t");
	printf(_ASM_MOV[platform], atoi(argv[1]));
	printf("\n");
	puts("\tret");
}
