// Test File for regar.c
//WinCmd$ gcc TestRegarPhinae.c ../source/regar.c ../source/ustring.c -o a.exe -D_dbg -I../
//---- ---- ---- ---- Phinae Style ---- ---- ---- ---- 
#include "aldbg.h"
#include "ustring.h"
#include "regar.h"
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

#define MALC_LIMIT 0x1001
size_t regcof = 0x1000, regdif = 0x1000, regdiv = 0x1000;

size_t malc_count, malc_occupy;
size_t malc_limit = MALC_LIMIT;
char arna_tempor[MALC_LIMIT];
char arna_tmpslv[MALC_LIMIT];
char arna_tmpext[MALC_LIMIT];

void erro(char* str) {}

void printr(const Rfnar_t* r)
{
	printf("\n%c %s * 10^{%c %s} / %s with %" PRIuPTR "\n", r->cofsign ? '-' : '+',
		RsgToString(r->coff, r->defalen, 10),
		r->expsign ? '-' : '+',
		RsgToString(r->expo, r->defalen, 10),
		RsgToString(r->divr, r->defalen, 10),
		r->defalen);
}

int main()
{
	char* p;
	
	//size_t a[5] = {0x12121212,0x84848484};
	//size_t b[5] = {0x33333333,0x1};
	//RsgDiv(a, b, 5);
	//printf("9548902812706017810/5153960755=%s...%s",
	//	RsgToString(a, 5, 10),
	//	RsgToString(b, 5, 10));
	//return 0;

	Rfnar_t* r0 = RedNewImm(1, 0, 1, 2);
	//printr(r0);
	//RedDig(r0, 1);
	//printr(r0);
	//RedDig(r0, 3);
	//printr(r0);

	//RedZip(r0);
	//printr(r0);

	//srs(r0, RedNewImm(0x20, 0, 0x400, 2));
	//RedReduct(r0);
	//printr(r0);

	//srs(r0, RedNewImm(1, 1, 2, 2)); r0->expsign = 0;// expo(1)
	Rfnar_t* r1 = RedNewImm(2, 2, 1, 2);// expo(2)
	//// RedAlignExpo(r0, r1);
	//RedAlign(r0, r1);
	//printf("r0"); printr(r0);
	//printf("r1"); printr(r1);

	//srs(r0, RedNewImm(1, 0, 3, 2));
	//RedDivrUnit(r0);
	//printf("r1.RedDivrUnit:"); printr(r0);

	RedDel(r0);
	
	//// 0.33-512
	//r0 = RedNewImm(1, 0, 3, 2);
	//r1 = RedNewImm(2, 2, 1, 2); r1->cofsign = 1;
	//RedAdd(r0, r1);
	//printf("1/3 - 2*256:"); printr(r0);

	//// 0.33--512
	//r0 = RedNewImm(1, 0, 3, 2);
	//r1 = RedNewImm(2, 2, 1, 2); r1->cofsign = 1;
	//RedSub(r0, r1);
	//printf("1/3 + 2*256:"); printr(r0);

	//r0 = RedNewImm(1, 0, 3, 2);
	//r1 = RedNewImm(2, 2, 1, 2); r1->cofsign = 1;
	//printf("r0~r1:%d\nr1~r0:%d\n", RedCmp(r0, r1), RedCmp(r1, r0));

	//r0 = RedNewImm(1, 1, 3, 2);
	//r1 = RedNewImm(16, 0, 3, 2);
	//printf("r0~r1:%d\n", RedCmp(r0, r1));

	//// 0.33*-512
	//r0 = RedNewImm(1, 0, 3, 2);
	//r1 = RedNewImm(2, 2, 1, 2); r1->cofsign = 1;
	//RedMul(r0, r1);
	//printf("1/3*-512:"); printr(r0);

	//// 0.33/-512
	//r0 = RedNewImm(1, 0, 3, 2);
	//r1 = RedNewImm(2, 2, 1, 2); r1->cofsign = 1;
	//RedDiv(r0, r1);    RedDivrUnit(r0);
	//printf("1/3/-512:"); printr(r0);
	//printf("\talso equals %lf\n", RedToDouble(r0));

	// r0 = RedNewImm(2, 1, 1, 1);
	// puts("2*16=32");
	// printf("\talso equals %lf\n", RedToDouble(r0));

	// r0 = RedNewImm(2, 0, 3, 1);
	// puts("2 / 3");
	// printf("\talso equals %lf\n", RedToDouble(r0));

// ---- ---- RFV2 FIN CHECK (RedFromDouble) and (RedToDouble) ---- ---- ARNCHK
	//	printf("[! %llx]\n", malc_count);
	//	r0 = RedFromDouble(123400, 2, 10);
	//	//printr(r0);
	//	printf("123400 equals %lf\n", RedToDouble(r0));
	//	RedDel(r0);
	//	r0 = RedFromDouble(-12345678.87654321, 4, 10);
	//	//printr(r0);
	//	printf("1234.5678 equals %.8lf\n", RedToDouble(r0));
	//	RedDel(r0);
	//	printf("[! %llx]\n", malc_count);

	
// ---- ---- RFV3 FIN CHECK (ChrHexToDecFloat) ---- ---- ARNCHK
	//	printf("0.ABH = 171/256 = 0.%s\n", p = ChrHexToDecFloat("AB"));
	//	memfree(p);
	//	printf("0.3039H = 2345/65536 = 0.%s\n", p = ChrHexToDecFloat("3039"));
	//	memfree(p);

// ---- ---- RFV3 FIN CHECK (ChrDecToHexFloat) ---- ---- ARNCHK
	printf("0.99 = 0.%sH\n", p = ChrDecToHexFloat("99", 12));
	memfree(p);
	{
		char pp[] = "12";
		printf("12.34 = %s.%sH\n", ChrDecToHex(pp), p = ChrDecToHexFloat("34", 12));
		memfree(p);
	}






	
	RedDel(r1);
	printf("[! %llx]\n", malc_count);
}
