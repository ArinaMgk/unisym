// RFV12 ASCII TAB4 C99 ArnAssume ApacheLicense2
// gcc ./CoeAdd.c ../source/cdear.c ../source/hstring.c ../source/ustring.c -D_dbg -o ../../../_bin/tcoeadd.exe
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
	exit(0);
}
int main()
{
	coe* c0, * c1;
	char* p;
	CoeInit();
	
	c0 = CoeFromDouble(-.123);
	c1 = CoeFromDouble(456);
	CoeAdd(c0, c1);
	p = CoeToLocale(c0, 1);
	printf("-0.123 + 456=%s\n", p);
	memf(p); CoeDel(c0), CoeDel(c1);

	c0 = CoeNew("+2","-1","+6");
	c1 = CoeNew("+1","+1","+7");
	CoeAdd(c0, c1);
	p = CoeToLocale(c0, 1);
	printf("+0.0333... + 1.4285714=%s\n", p);
	memf(p); CoeDel(c0), CoeDel(c1);

	printf("dbg-check-memory-trash:%"PRIXPTR"\n", malc_count);
}
/* Output passed
	-0.123 + 456=+455.87700000000000001215003782312148219149567805540603992323090929
	+0.0333... + 1.4285714=+1.46190476190476190476190476190476190476190476190476190476190476
	dbg-check-memory-trash:0
*/

