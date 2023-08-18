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
#include "../ustring.h"
#include "../aldbg.h"
#include "stdio.h"
#include "stdlib.h"
// ARN's IS ACCORDING TO A-
// RINA'S DEVELOPING EXPER-
// IENCE. APPRECIATE SHARI-
// NG YOUR FANTASTIC IDE-
// A! -- SELF, RFT03.

char chrar_sgned = 1;
size_t malc_count, malc_limit = 64;
size_t malc_occupy;
char arna_tempor[1], arna_tmpslv[1], arna_tmpext[1];
//int main()
//{
//	char* c = ChrAdd("+1", "+3");
//	puts(c);
//}
// test token all
// ArnMgk RFT03, RFT26
char buf[256];


char a[] = "0x+(1.2+i++);;\n\r\n\ra\nb\n";
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
static void StrTokenPrintAll(Tode* first)
{
	do printf("Token: [R%llu,C%llu][%s][%llu] %s\n", first->row, first->col, ((const char* []){"END", "ANY", "STR", "CMT", "DIR", "NUM", "SYM", "IDN", "SPC", "ELS"})[first->type], StrLength(first->addr), first->type == tok_space ? "" : first->addr);
	while (first = first->next);
}
int main()
{
	
	Toknode* tn = 0;
	if (use_file)
	{
		printf("This is from text file.\n");
		fp = fopen("./DemoStrTokenAll.txt", "rb");
		if (fp)
		{
			tn = StrTokenAll(fgetnext, fseekback, buf);
			printf("[malc_count %llx]:\n", malc_count);
		}
	}
	else
	{
		printf("This is from text buffer.\n");
		// puts(a);
		tn = StrTokenAll(sgetnext, sseekback, buf);
		printf("[malc_count %llx]:\n", malc_count);
	}
	// over them
	if (!tn) return 0;
	while (tn->left) tn = tn->left;
	StrTokenPrintAll(tn);
	StrTokenClear(tn);
	printf("[malc_count %llx]:\n", malc_count);
	if (!use_file)
	{
		use_file = 1;
		main();
	}
	system("pause");
	return 0;
}

