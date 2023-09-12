// RFV12 ASCII TAB4 C99 ArnAssume ApacheLicense2
// gcc ./CoeFromLocale.c ../source/cdear.c ../source/hstring.c ../source/ustring.c -D_dbg -o ../../../_bin/tc4local.exe
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
	
	coco = CoeFromLocale("-2023.0912e-2");
	p = CoeToLocale(coco, 1);
	printf("-2023.0912e-2=%s\n", p);
	memf(p); CoeDel(coco);

	coco = CoeFromLocale("2.e1");
	p = CoeToLocale(coco, 1);
	printf("2.e1=%s\n", p);
	memf(p); CoeDel(coco);

	coco = CoeFromLocale(".2e-1");
	p = CoeToLocale(coco, 1);
	printf(".2e-1=%s\n", p);
	memf(p); CoeDel(coco);

	coco = CoeFromLocale("123");
	p = CoeToLocale(coco, 1);
	printf("123=%s\n", p);
	memf(p); CoeDel(coco);


	printf("dbg-check-memory-trash:%"PRIXPTR"\n", malc_count);
}
/* Output passed
	-2023.0912e-2=-20.230912
	2.e1=+20
	.2e-1=+0.02
	123=+123
	dbg-check-memory-trash:0
*/

