// RFV12 ASCII TAB4 C99 ArnAssume ApacheLicense2
// gcc ./CoeDiv.c ../source/cdear.c ../source/hstring.c ../source/ustring.c -D_dbg -o ../../../_bin/tcoediv.exe
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
	CoeDiv(c0, c1);
	p = CoeToLocale(c0, 1);
	printf("-0.123 / 456=%s\n", p);
	memf(p); CoeDel(c0), CoeDel(c1);

	c0 = CoeNew("+2","-1","+6");
	c1 = CoeNew("+1","+1","+7");
	CoeDiv(c0, c1);
	p = CoeToLocale(c0, 1);
	printf("+0.0333... / 1.4285714=%s\n", p);
	memf(p); CoeDel(c0), CoeDel(c1);

	c0 = CoeNew("+2","-1","+6");
	c1 = CoeNew("+0","+1","+7");
	CoeDiv(c0, c1);
	p = CoeToLocale(c0, 1);
	printf("+0.0333... / 0=%s\n", p);
	memf(p); CoeDel(c0), CoeDel(c1);

	printf("dbg-check-memory-trash:%"PRIXPTR"\n", malc_count);
}
/* Output passed
	-0.123 / 456=-0.000269736842105263131249917054558153088825267422355175606949760328
	+0.0333... / 1.4285714=+0.0233333333333333333333333333333333333333333333333333333333333333
	CoeDiv: zero sors.
	+0.0333... / 0=+0.0333333333333333333333333333333333333333333333333333333333333333
	dbg-check-memory-trash:0
*/

