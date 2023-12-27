// RFV12 ASCII TAB4 C99 ArnAssume ApacheLicense2
// gcc ./CoeFromDouble.c ../source/cdear.c ../source/hstring.c ../source/ustring.c -D_dbg -o ../../../_bin/tc4f.exe
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
	
	coco = CoeFromDouble(-2023.0912e-2);
	p = CoeToLocale(coco, 1);
	printf("-2023.0912e-2=%s\n", p);
	memf(p); CoeDel(coco);

	coco = CoeFromDouble(2.e1);
	p = CoeToLocale(coco, 1);
	printf("2.e1=%s\n", p);
	memf(p); CoeDel(coco);

	coco = CoeFromDouble(.2e-1);
	printf("%s %s\n", coco->coff, coco->expo);
	p = CoeToLocale(coco, 1);
	printf(".2e-1=%s\n", p);
	memf(p); CoeDel(coco);

	coco = CoeFromDouble(123);
	p = CoeToLocale(coco, 1);
	printf("123=%s\n", p);
	memf(p); CoeDel(coco);

	printf("dbg-check-memory-trash:%"PRIXPTR"\n", malc_count);
}
/* Output passed
	-2023.0912e-2=-20.2309119999999999887918628142252975538475867330165215074743615
	2.e1=+20
	+1999999999999999522253694977993780510600628715110266747140668 -62
	.2e-1=+0.01999999999999999522253694977993780510600628715110266747140668
	123=+123
	dbg-check-memory-trash:0
*/

