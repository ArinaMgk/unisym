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
#define _CRT_SECURE_NO_WARNINGS
#include "ustring.h"
#include "alice_dbg.h"
#include "stdio.h"
#include "stdlib.h"
// ARN's IS ACCORDING TO A-
// RINA'S DEVELOPING EXPER-
// IENCE. APPRECIATE SHARI-
// NG YOUR FANTASTIC IDE-
// A! -- SELF, RFT03.
char chrar_sgned = 1;
#include "E:/PROJ/SVGN/unisym.h"
// test token all
// ArnMgk RFT03
char buf[256];
void erro() {} size_t malc_count;

char a[] = "0x+(1.2+i++);;\n";
char* pos = a;
FILE* fp;
int fgetnext()
{
	if (!fp) return EOF;
	return fgetc(fp);
}
int sgetnext()
{
	if (!*pos) return EOF;
	else return *pos++;
}
void fseekback(ptrdiff_t l)
{
	if (!fp) return;
	fseek(fp, l, SEEK_CUR);
}
void sseekback(ptrdiff_t l)
{
	pos += l;
}
int use_file = 0;
int main()
{
	
	Toknode* tn = 0;
	if (use_file)
	{
		printf("This is from text file.\n");
		fp = fopen("./DemoStrTokenAll.txt", "r");
		if (fp)
		{
			tn = StrTokenAll(fgetnext, fseekback, buf);
			printf("[%llx]:\n", malc_count);
		}
	}
	else
	{
		printf("This is from text buffer.\n");
		puts(a);
		tn = StrTokenAll(sgetnext, sseekback, buf);
		printf("[%llx]:\n", malc_count);
	}
	// over them
	{
		Toknode* last = tn;
		while (last)
		{
			if (last->addr) printf("\t%s\n", last->addr);
			last = last->next;
		}
	}
	StrTokenClear(tn);
	printf(" %llx\n\n", malc_count);
	if (!use_file)
	{
		use_file = 1;
		main();
	}
	return 0;
}

