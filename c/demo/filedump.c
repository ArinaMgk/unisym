// ASCII TAB4 C99 ArnAssume RFC17
/*
	Copyright 2023 ArinaMgk

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/

//$ gcc filedump.c ../source/consio.c -D_WinNT -o fdumpw.exe & gcc filedump.c ../source/consio.c -D_Linux -o fdumpl.exe

#include "../alice.h"
#include "../aldbg.h"
#include "../consio.h"
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

int main(int argc, char** argv)
{
	size_t crtline = 0;
	ConStyleNormal();
	if (argc < 2)
	{
		printline("Usage: filedump <filename>");
		return 0;
	}
	FILE* fp = fopen(argv[1], "rb");
	if (fp == nil)
	{
		printline("Error: file not found");
		return -1;
	}
	fseek(fp, 0, SEEK_END);
	size_t size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	if (size > 0xFFFFFFFF)
	{
		printline("Sorry: this program can only dump files smaller than 4GB");
		fclose(fp);
		return -1;
	}
	byte* buf = malc(size);
	fread(buf, 1, size, fp);
	fclose(fp);

	printf("         ");
	ConStyleAbnormal();
	printf("+0 +1 +2 +3 +4 +5 +6 +7 +8 +9 +A +B +C +D +E +F 0123456789ABCDEF");
	ConStyleNormal();

	printline("");

	ConStyleAbnormal();
	printf("%08" PRIXPTR, crtline);
	crtline += 0x10;
	ConStyleNormal();
	putchar(' ');

	size_t i;
	for (i = 0;i < size;i++)
	{
		printf("%02X", buf[i]);
		if ((i & 0xF) == 0xF)
		{
			putchar(' ');
			i &= ~(size_t)0xF;
			for (size_t j = 0;j < 0x10;j++)
			{
				if (buf[i + j] >= 0x20 && buf[i + j] <= 0x7E)
					putchar(buf[i + j]);
				else putchar('.');
			}
			i |= (size_t)0xF;
			ConStyleAbnormal();
			printf("\n%08" PRIXPTR, crtline);
			crtline += 0x10;
			ConStyleNormal();
			putchar(' ');
		}
		else if((i & 0x7) == 0x7)
			putchar('-');
		else putchar(' ');
	}
	if(i & 0xF)
	{
		unsigned char crtline_rest = i & 0xF;
		for (size_t j = 0;j < 0x10 - crtline_rest;j++)
			printf("   ");
		i &= ~(size_t)0xF;
		for (size_t j = 0;j < crtline_rest;j++)
		{
			if (buf[i + j] >= 0x20 && buf[i + j] <= 0x7E)
				putchar(buf[i + j]);
			else putchar('.');
		}
	}

	printline("");

	memf(buf);
	return 0;
}