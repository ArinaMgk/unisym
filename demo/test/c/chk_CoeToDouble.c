// RFV12 ASCII TAB4 C99 ArnAssume ApacheLicense2
// gcc ./CoeToDouble.c ../source/cdear.c ../source/hstring.c ../source/ustring.c -D_dbg -o ../../../_bin/tc2f.exe
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
	coe* coco; double p;
	
	CoeInit();
	
	coco = CoeNew("+123", "-1", "+1");
	p = CoeToDouble(coco);
	printf("12.3=%lf\n", p);
	CoeDel(coco);

	coco = CoeNew("+1234567890/10", "-1", "+1");
	p = CoeToDouble(coco);
	printf("1234567890=%lf\n", p);
	CoeDel(coco);

	coco = CoeNew("-12", "-10", "+1");
	p = CoeToDouble(coco);
	printf("-1.2E10=%lf\n", p);// Arina use upper case to express negative objects.
	CoeDel(coco);

	coco = CoeNew("-1", "-1", "+3");
	p = CoeToDouble(coco);
	printf("-0.0333...=%lf\n", p);
	CoeDel(coco);

	coco = CoeNew("+2", "-1", "+3");
	p = CoeToDouble(coco);
	printf("+0.0666...=%lf\n", p);
	CoeDel(coco);

	printf("dbg-check-memory-trash:%"PRIXPTR"\n", malc_count);
}
/* Output passed
	12.3=12.300000
	1234567890=123456788991.000000
	-1.2E10=-0.000000
	-0.0333...=-0.033333
	+0.0666...=0.066667
	dbg-check-memory-trash:0
*/

