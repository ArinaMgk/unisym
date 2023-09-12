// RFV12 ASCII TAB4 C99 ArnAssume ApacheLicense2
// gcc ./CoeToLocale.c ../source/cdear.c ../source/hstring.c ../source/ustring.c -D_dbg -o ../../../_bin/tc2local.exe
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
	coe* coco; char* p;
	
	CoeInit();
	
	coco = CoeNew("+123", "-1", "+1");
	p = CoeToLocale(coco, 0);
	printf("12.3=%s\n", p);
	memf(p); CoeDel(coco);

	coco = CoeNew("+1234567890", "-1", "+1");
	p = CoeToLocale(coco, 0);
	printf("1234567890/10=%s\n", p);
	memf(p); CoeDel(coco);

	coco = CoeNew("-12", "-10", "+1");
	p = CoeToLocale(coco, 0);
	printf("-1.2E10=%s\n", p);// Arina use upper case to express negative objects.
	memf(p); CoeDel(coco);

	coco = CoeNew("-1", "-1", "+3");
	p = CoeToLocale(coco, 0);
	printf("-0.0333...=%s\n", p);
	memf(p); CoeDel(coco);

	coco = CoeNew("+2", "-1", "+3");
	p = CoeToLocale(coco, 1);
	printf("+0.0666...=%s\n", p);
	memf(p); CoeDel(coco);

	printf("dbg-check-memory-trash:%"PRIXPTR"\n", malc_count);
}
/* Output passed
	12.3=+12.3
	1234567890/10=+1.234567890e+8
	-1.2E10=-1.2e-9
	-0.0333...=-3.33333333333333333333333333333333333333333333333333333333333333e-2
	+0.0666...=+0.0666666666666666666666666666666666666666666666666666666666666667
	dbg-check-memory-trash:0
*/

