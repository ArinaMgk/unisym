// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant CPU(i80586+)
// LastCheck: RFZ03
// AllAuthor: @dosconio
// ModuTitle: Test for Instruction CPUID
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

#include "../../inc/c/cpuid.h"
#include <stdio.h>
#include <stdlib.h>

char buf[1 + _CPU_BRAND_SIZE];

static void show_cpu_brand(void)
{
	CpuBrand(buf);
	printf("CPU Brand:  %s\n", buf);
}

int main(int argc, char* argv[])
{
	int badcmd = 0;
	if (argc > 1 && argv[1][0] == '-')
	{
		if (!argv[1][2]) switch (argv[1][1]) {
		case 'b': show_cpu_brand(); break;
		default:
			printf("Usage: %s [-b] [comments]\n", argv[0]);
			badcmd = 1;
			break;
		}
		else badcmd = 1;
	}
	else { // show all
		show_cpu_brand();
	}
	exit(badcmd);
}
