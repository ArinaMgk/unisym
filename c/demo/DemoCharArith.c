// RFT15 ASCII TAB4 C99 ArnAssume
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
// gcc ./DemoCharArith.c ../source/ustring.c -D_dbg
#include<stddef.h>
#include<stdio.h>
#include<stdlib.h>
#include<inttypes.h>
#include"../alice_dbg.h"
#include"../ustring.h"

size_t malc_count, malc_occupy;
size_t malc_limit = 4096;
char arna_tempor[128];
char arna_tmpslv[sizeof(arna_tempor)];
char arna_tmpext[sizeof(arna_tempor)];

void erro(char* errmsg)
{
	puts(errmsg);
	exit(0);
}
char buf[32] = {0};
char* s;
int main()
{
	extern int TestInstoa(), TestAtoins();
	extern int TestChrHexToDec(),\
		TestChrAddAlone(),\
		TestChrAddSub(),\
		TestChrMul(),\
		TestChrDiv(),\
		TestChrFactorial(),\
		TestChrArrange(),\
		TestChrCombinate(),\
		TestChrComDiv(),\
		TestChrComMul(),\
		TestChrDecToHex();
	TestInstoa();
	TestAtoins();
	TestChrAddAlone();
	TestChrAddSub();
	TestChrMul();
	TestChrDiv();
	TestChrFactorial();
	TestChrArrange();
	TestChrCombinate();
	TestChrComDiv();
	TestChrComMul();
	TestChrDecToHex();
	TestChrHexToDec();
	printf("dbg-check-memory-trash:%"PRIXPTR"\n", malc_count); system("pause");
}
int TestInstoa()
{
	// unsigned heapyo
	arna_eflag.Signed=0;
	s = instoa(123);
	printf("123 = %s\n", s); memfree(s);
	s = instoa(0);
	printf("0 = %s\n", s); memfree(s);
	// signed heapyo
	arna_eflag.Signed=1;
	s = instoa(-123);
	printf("-123 = %s\n", s); memfree(s);
	s = instoa(0);
	printf("0 = %s\n", s); memfree(s);

	malc_limit = 32;
	// unsigned bufferyo
	arna_eflag.Signed=0;
	s = instob(123, buf);
	printf("123 = %s\n", s);
	s = instob(0, buf);
	printf("0 = %s\n", s);
	// signed bufferyo
	arna_eflag.Signed=1;
	s = instob(-123, buf);
	printf("-123 = %s, len=%"PRIxPTR"\n", s, malc_occupy);
	s = instob(0, buf);
	printf("0 = %s, len=%"PRIxPTR"\n", s, malc_occupy);
}

int TestAtoins()
{
	printf("-1470 = %"PRIdPTR"\n", atoins("-1470"));
	printf("-0 = %"PRIdPTR"\n", atoins("-0"));
}

int TestChrHexToDec()
{
	// unsigned heapyo
	arna_eflag.HeapYo = 1;
	arna_eflag.Signed = 0;
	s = ChrHexToDec("-FE");
	printf("-FE = %s\n", s); memfree(s);
	
	// signed heapyo
	arna_eflag.HeapYo = 1;
	arna_eflag.Signed = 1;
	s = ChrHexToDec("-ff");
	printf("-ff = %s\n", s); memfree(s);

	malc_limit = 32;
	// unsigned bufferyo
	arna_eflag.HeapYo = 0;
	arna_eflag.Signed = 0;
	s = ChrHexToDec("+F0");
	printf("F0H = %s, len=%"PRIxPTR"\n", s, malc_occupy);

	// signed bufferyo
	arna_eflag.HeapYo = 0;
	arna_eflag.Signed = 1;
	s = ChrHexToDec("+10");
	printf("10H = %s, len=%"PRIxPTR"\n", s, malc_occupy);
}

int TestChrAddAlone()
{
	// unsigned heapyo
	arna_eflag.HeapYo = 1;
	arna_eflag.Signed = 0;
	s = ChrAdd("1","2");
	printf("1+2 = %s\n", s); memfree(s);

	// signed heapyo
	arna_eflag.HeapYo = 1;
	arna_eflag.Signed = 1;
	s = ChrAdd("+0", "-0");
	printf("0+0 = %s\n", s); memfree(s);

	malc_limit = 32;
	// unsigned bufferyo
	arna_eflag.HeapYo = 0;
	arna_eflag.Signed = 0;
	s = ChrAdd("0", "9");
	printf("0+9 = %s, len=%"PRIxPTR"\n", s, malc_occupy);

	// signed bufferyo
	arna_eflag.HeapYo = 0;
	arna_eflag.Signed = 1;
	s = ChrAdd("+5", "+29");
	printf("5+29 = %s, len=%"PRIxPTR"\n", s, malc_occupy);
}

int TestChrAddSub()
{
	// unsigned heapyo
	arna_eflag.HeapYo = 1;
	arna_eflag.Signed = 0;
	s = ChrSub("1", "2");
	printf("1-2 = %s\n", s); memfree(s);

	// signed heapyo
	arna_eflag.HeapYo = 1;
	arna_eflag.Signed = 1;
	s = ChrAdd("+1", "-0");
	printf("1-0 = %s\n", s); memfree(s);

	malc_limit = 32;
	// unsigned bufferyo
	arna_eflag.HeapYo = 0;
	arna_eflag.Signed = 0;
	s = ChrSub("0", "9");
	printf("0-9 = %s, len=%"PRIxPTR"\n", s, malc_occupy);

	// signed bufferyo
	arna_eflag.HeapYo = 0;
	arna_eflag.Signed = 1;
	s = ChrAdd("+5", "-29");
	printf("5-29 = %s, len=%"PRIxPTR"\n", s, malc_occupy);
}

int TestChrMul()
{
	// unsigned heapyo
	arna_eflag.HeapYo = 1;
	arna_eflag.Signed = 0;
	s = ChrMul("1", "2");
	printf("1*2 = %s\n", s); memfree(s);

	// signed heapyo
	arna_eflag.HeapYo = 1;
	arna_eflag.Signed = 1;
	s = ChrMul("+0", "-0");
	printf("0*0 = %s\n", s); memfree(s);

	malc_limit = 32;
	// unsigned bufferyo
	arna_eflag.HeapYo = 0;
	arna_eflag.Signed = 0;
	s = ChrMul("0", "9");
	printf("0*9 = %s, len=%"PRIxPTR"\n", s, malc_occupy);

	// signed bufferyo
	arna_eflag.HeapYo = 0;
	arna_eflag.Signed = 1;
	s = ChrMul("+599999", "-29001");
	printf("599999*-29001 = %s, len=%"PRIxPTR"\n", s, malc_occupy);
}

int TestChrDiv()
{
	arna_eflag.Signed = 1;
	char a[5] = "+45"
		, b[5] = "+10";
	arna_eflag.HeapYo = 1;
	ChrDiv(a, b);
	printf("45/10=%s...%s\n", a, b);
	char a2[5] = "+45"
		, b2[5] = "-19";
	arna_eflag.HeapYo = 0;
	ChrDiv(a2, b2);
	printf("45/-19=%s...%s\n", a2, b2);

	arna_eflag.Signed = 0;
	char a3[5] = "45"
		, b3[5] = "10";
	arna_eflag.HeapYo = 1;
	ChrDiv(a3, b3);
	printf("45/10=%s...%s\n", a3, b3);
	char a4[5] = "45"
		, b4[5] = "19";
	arna_eflag.HeapYo = 0;
	ChrDiv(a4, b4);
	printf("45/-19=%s...%s\n", a4, b4);
}

int TestChrFactorial()
{
	// unsigned heapyo
	arna_eflag.HeapYo = 1;
	arna_eflag.Signed = 0;
	s = ChrFactorial("5");
	printf("5! = %s\n", s); memfree(s);

	// signed heapyo
	arna_eflag.HeapYo = 1;
	arna_eflag.Signed = 1;
	s = ChrFactorial("+0");
	printf("0! = %s\n", s); memfree(s);

	malc_limit = 32;
	// unsigned bufferyo
	arna_eflag.HeapYo = 0;
	arna_eflag.Signed = 0;
	s = ChrFactorial("0");
	printf("0! = %s, len=%"PRIxPTR"\n", s, malc_occupy);

	// signed bufferyo
	arna_eflag.HeapYo = 0;
	arna_eflag.Signed = 1;
	s = ChrFactorial("+4");
	printf("4! = %s, len=%"PRIxPTR"\n", s, malc_occupy);
}

int TestChrArrange()
{
	// signed heapyo
	arna_eflag.HeapYo = 1;
	arna_eflag.Signed = 1;
	s = ChrArrange("+3", "+2");
	printf("A_3`2 = %s\n", s); memfree(s);

	// signed bufferyo
	arna_eflag.HeapYo = 0;
	arna_eflag.Signed = 1;
	s = ChrArrange("+5","+3");
	printf("A_5`3 = %s, len=%"PRIdPTR"\n", s, malc_occupy);
}

int TestChrCombinate()
{
	// signed heapyo
	arna_eflag.HeapYo = 1;
	arna_eflag.Signed = 1;
	s = ChrCombinate("+3", "+2");
	printf("C_3`2 = %s\n", s); memfree(s);

	// signed bufferyo
	arna_eflag.HeapYo = 0;
	arna_eflag.Signed = 1;
	s = ChrCombinate("+5","+3");
	printf("C_5`3 = %s, len=%"PRIdPTR"\n", s, malc_occupy);
}

int TestChrComDiv()
{
	// signed heapyo
	arna_eflag.HeapYo = 1;
	arna_eflag.Signed = 1;
	s = ChrComDiv("+3", "+2");
	printf("GCD 3 2 = %s\n", s); memfree(s);

	// signed bufferyo
	arna_eflag.HeapYo = 0;
	arna_eflag.Signed = 1;
	s = ChrComDiv("+15","+20");
	printf("GCD 15 20 = %s, len=%"PRIdPTR"\n", s, malc_occupy);
}

int TestChrComMul()
{
	// signed heapyo
	arna_eflag.HeapYo = 1;
	arna_eflag.Signed = 1;
	s = ChrComMul("+3", "+2");
	printf("LCM 3 2 = %s\n", s); memfree(s);

	// signed bufferyo
	arna_eflag.HeapYo = 0;
	arna_eflag.Signed = 1;
	s = ChrComMul("+5","+20");
	printf("LCM 5 20 = %s, len=%"PRIdPTR"\n", s, malc_occupy);
}

int TestChrDecToHex()
{
	// unsigned heapyo
	arna_eflag.HeapYo = 1;
	arna_eflag.Signed = 0;
	s = ChrDecToHex((char[]) { "20" });
	printf("-20 = -0x%s\n", s);

	// signed bufferyo
	arna_eflag.HeapYo = 0;
	arna_eflag.Signed = 0;
	s = ChrDecToHex((char[]) { "30" });
	printf("30 = 0x%s, len=%"PRIdPTR"\n", s, malc_occupy);
}
