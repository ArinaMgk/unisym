// RFV11 ASCII TAB4 C99 ArnAssume ApacheLicense2
// gcc ./ChrDiv.c ../source/hstring.c ../source/ustring.c -D_dbg -o ../../../_bin/tcd.exe
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
	char* p, * q;
	// [unsigned] both un-0
	aflag.Signed = 0; ChrDiv(p = chstk(1), q = chstk(2));
	printf("1/2=%s...%s : sign(%c) fail(%d)\n", p, q, aflag.Sign ? '-' : '+', aflag.Failure);

	// both un-0
	aflag.Signed = 1; ChrDiv(p = chstk(+1), q = chstk(-2));
	printf("+1/-2=%s...%s : sign(%c) fail(%d)\n", p, q, aflag.Sign ? '-' : '+', aflag.Failure);

	// +0 and -0
	aflag.Signed = 1; ChrDiv(p = chstk(+0), q = chstk(-0));
	printf("+0/-0=%s...%s : sign(%c) fail(%d)\n", p, q, aflag.Sign ? '-' : '+', aflag.Failure);

	// +0 and -12
	aflag.Signed = 1; ChrDiv(p = chstk(+0), q = chstk(-12));
	printf("+0/-12=%s...%s : sign(%c) fail(%d)\n", p, q, aflag.Sign ? '-' : '+', aflag.Failure);

	// +12 and -0
	aflag.Signed = 1; ChrDiv(p = chstk(+12), q = chstk(-0));
	printf("+12/-0=%s...%s : sign(%c) fail(%d)\n", p, q, aflag.Sign ? '-' : '+', aflag.Failure);

	aflag.Signed = 1; ChrDiv(p = chstk(-9876543210), q = chstk(+1234567890));
	printf("-9876543210/+1234567890=%s...%s : sign(%c) fail(%d)\n", p, q, aflag.Sign ? '-' : '+', aflag.Failure);

	aflag.Signed = 1; ChrDiv(p = chstk(+88888888666666444422), q = chstk(-2));
	printf("+88888888666666444422/-2=%s...%s : sign(%c) fail(%d)\n", p, q, aflag.Sign ? '-' : '+', aflag.Failure);

	printf("dbg-check-memory-trash:%"PRIXPTR"\n", malc_count);
}
/* Output passed
	1/2=0...1 : sign(+) fail(0)
	+1/-2=-0...+1 : sign(-) fail(0)
	+0/-0=-0...+0 : sign(-) fail(1)
	+0/-12=-0...+0 : sign(-) fail(0)
	+12/-0=-12...+0 : sign(-) fail(1)
	-9876543210/+1234567890=-8...-90 : sign(-) fail(0)
	+88888888666666444422/-2=-44444444333333222211...+0 : sign(-) fail(0)
	dbg-check-memory-trash:0
*/

