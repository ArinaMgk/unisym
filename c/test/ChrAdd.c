// RFV10 ASCII TAB4 C99 ArnAssume ApacheLicense2
// gcc ./ChrAdd.c ../source/hstring.c ../source/ustring.c -D_dbg -o ../../../_bin/tca.exe
#define _LIB_STRING_HEAP
#include<stddef.h>
#include<stdio.h>
#include<stdlib.h>
#include<inttypes.h>
#include"../aldbg.h"
#include"../ustring.h"
ulibsym(0x1000)

void erro(char* errmsg)
{
	puts(errmsg);
	exit(0);
}
int main()
{
	// not consider the order of a and b
	char* p;
	// [u] unsigned, both un-0
	aflag.Signed = 0; p = ChrAdd("1", "2");
	printf("1+2=%s : sign(%c) fail(%d)\n", p, aflag.Sign ? '-' : '+', aflag.Failure);
	memf(p);
	// [s] unsigned, both un-0
	aflag.Signed = 1; p = ChrAdd("1", "2");
	printf("1+2=%s : sign(%c) fail(%d)\n", p, aflag.Sign ? '-' : '+', aflag.Failure);
	memf(p);
	// [u] single sign, both un-0, neg>pos
	aflag.Signed = 0; p = ChrAdd("1", "-2");
	printf("1-2=%s : sign(%c) fail(%d)\n", p, aflag.Sign ? '-' : '+', aflag.Failure);
	memf(p);
	// [s] single sign, both un-0, neg<pos
	aflag.Signed = 1; p = ChrAdd("-1", "2");
	printf("2-1=%s : sign(%c) fail(%d)\n", p, aflag.Sign ? '-' : '+', aflag.Failure);
	memf(p);
	// [s] signed, both un-0, neg>pos
	aflag.Signed = 1; p = ChrAdd("+1", "-2");
	printf("1-2=%s : sign(%c) fail(%d)\n", p, aflag.Sign ? '-' : '+', aflag.Failure);
	memf(p);
	// [s] signed, both un-0
	aflag.Signed = 1; p = ChrAdd("+1", "+2");
	printf("1+2=%s : sign(%c) fail(%d)\n", p, aflag.Sign ? '-' : '+', aflag.Failure);
	memf(p);
	// [s] signed, both un-0
	aflag.Signed = 1; p = ChrAdd("-1", "-2");
	printf("-1-2=%s : sign(%c) fail(%d)\n", p, aflag.Sign ? '-' : '+', aflag.Failure);
	memf(p);
	// [s] +0 and -0
	aflag.Signed = 1; p = ChrAdd("+0", "-0");
	printf("0-0=%s : sign(%c) fail(%d)\n", p, aflag.Sign ? '-' : '+', aflag.Failure);
	memf(p);
	// [s] neg and +0
	aflag.Signed = 1; p = ChrAdd("-1", "0");
	printf("-1+0=%s : sign(%c) fail(%d)\n", p, aflag.Sign ? '-' : '+', aflag.Failure);
	memf(p);
	// [s] dif sign, equal
	aflag.Signed = 1; p = ChrAdd("-1", "+1");
	printf("1-1=%s : sign(%c) fail(%d)\n", p, aflag.Sign ? '-' : '+', aflag.Failure);
	memf(p);
	printf("dbg-check-memory-trash:%"PRIXPTR"\n", malc_count);
}
/* Output passed
	1+2=3 : sign(+) fail(0)
	1+2=+3 : sign(+) fail(0)
	1-2=1 : sign(-) fail(0)
	2-1=+1 : sign(+) fail(0)
	1-2=-1 : sign(-) fail(0)
	1+2=+3 : sign(+) fail(0)
	-1-2=-3 : sign(-) fail(0)
	0-0=+0 : sign(+) fail(0)
	-1+0=-1 : sign(-) fail(0)
	1-1=+0 : sign(+) fail(0)
	dbg-check-memory-trash:0
*/

