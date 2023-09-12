// RFV12 ASCII TAB4 C99 ArnAssume ApacheLicense2
// gcc ./CoeMul.c ../source/cdear.c ../source/hstring.c ../source/ustring.c -D_dbg -o ../../../_bin/tcoemul.exe
#define _LIB_STRING_HEAP
#include<stddef.h>
#include<stdio.h>
#include<stdlib.h>
#include<inttypes.h>
#include"../aldbg.h"
#include"../ustring.h"
#include"../cdear.h"
ulibsym(0x1000)

void erro(char* errmsg)
{
	puts(errmsg);
	// exit(0);
}
int main()
{
	coe* c0, * c1;
	char* p;
	CoeInit();
	
	c0 = CoeFromDouble(-.123);
	c1 = CoeFromDouble(456);
	CoeMul(c0, c1);
	p = CoeToLocale(c0, 1);
	printf("-0.123 * 456=%s\n", p);
	memf(p); CoeDel(c0), CoeDel(c1);

	c0 = CoeNew("+2","-1","+6");
	c1 = CoeNew("+1","+1","+7");
	CoeMul(c0, c1);
	p = CoeToLocale(c0, 1);
	printf("+0.0333... * 1.4285714=%s\n", p);
	memf(p); CoeDel(c0), CoeDel(c1);

	c0 = CoeNew("+2","-1","+0");
	c1 = CoeNew("+1","+1","+7");
	CoeMul(c0, c1);
	p = CoeToLocale(c0, 1);
	printf("+INF * 1.4285714=%s\n", p);
	memf(p); CoeDel(c0), CoeDel(c1);

	printf("dbg-check-memory-trash:%"PRIXPTR"\n", malc_count);
}
/* Output passed
	-0.123 * 456=-56.08799999999999445958275265660412067797080673484579500670536376
	+0.0333... * 1.4285714=+0.0476190476190476190476190476190476190476190476190476190476190476
	Zero divided exception!
	Zero divided exception!
	+INF * 1.4285714=+0.2
	dbg-check-memory-trash:0
*/

