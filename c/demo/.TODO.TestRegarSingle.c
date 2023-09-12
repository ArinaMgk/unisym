// Test File for regar.c

#include "regar.h"
#include <stdlib.h>
#include <stdio.h>

size_t regcof = 0x1000, regdif = 0x1000, regdiv = 0x1000;

size_t malc_count, malc_occupy;
size_t malc_limit = 4096;
char arna_tempor[128];
char arna_tmpslv[sizeof(arna_tempor)];
char arna_tmpext[sizeof(arna_tempor)];

void erro() {}

int main() {

	//size_t reg[] = { 0x12345678, 0x12345678 };
	//size_t rlen = sizeof(reg) / sizeof(*reg);
	//char* p; size_t res;
	//p = RsgToString(reg, rlen, 10);
	//printf("r = %s\n", p);
	//free(p);
	
	//res = RsgDiv8(reg, rlen, 77);
	//p = RsgToString(reg, rlen, -16);
	//printf("r/16 then r=%s rem=%hhu\n", p, res);
	//free(p);

	//res = RsgMul8(reg, rlen, 3);
	//p = RsgToString(reg, rlen, 16);
	//printf("r*3 then r = %s, res=%d\n", p, res);
	//free(p);

	//res = RsgAdd8(reg, rlen, 88);
	//p = RsgToString(reg, rlen, 10);
	//printf("r+88 then r = %s carry:%d\n", p, res);
	//free(p);

	//size_t r2[] = { 0xFFFFFFF0 };

	//res = RsgAdd8(r2, 1, 0x20);

	//res = RsgCmp(reg, r2, 1, 1);

	//size_t r3[] = { 6543 };
	//size_t r4[] = { 20 };

	//res = RsgSub8(r3, 1, 50);
	//p = RsgToString(r3, 1, 10);
	//printf("6543-50 then = %s bor:%d\n", p, res);
	//free(p);
	//res = RsgSub8(r4, 1, 50);
	//p = RsgToString(r4, 1, 10);
	//printf("20-50 then = %s bor:%d\n", p, res);
	//free(p);

	//size_t* r5 = RsgResize(r3, 1, 2);
	//r5[1] = 1;
	//p = RsgToString(r5, 2, 10);
	//printf("[6543][1] = %s\n", p);
	//free(p); free(r5);

	//size_t r5[] = { 0x98765432,2 };
	//size_t r6[5] = { 0x12345678,1 };

	//RsgAdd(r5, r6, 2);
	// 
	//res = RsgSub(r5, r6, 2);
	//p = RsgToString(r5, 2, -16);
	//printf("r5 = %s res=%d\n", p, res);
	//free(p);
	//*r5 = 0x98765432; r5[1] = 2;
	//res = RsgSub(r6, r5, 2);
	//p = RsgToString(r6, 2, -16);
	//printf("r6 = %s res=%d\n", p, res);
	//free(p);

	//res = RsgMul(r5, r6, 2);
	//p = RsgToString(r5, 2, -16);
	//printf("r5*r6: r5 = %s res=%d\n", p, res);
	//free(p);

	//r5[0] = 1; r5[1] = 1;
	//r6[0] = 2; r6[1] = 2;
	//res = RsgMul(r5, r6, 2);
	//p = RsgToString(r5, 2, 16);
	//printf("r5*r6: r5 = %s res=%d\n", p, res);
	//free(p);

	//res = RsgDiv(r5, r6, 2);
	//printf("r5/r6: r5 = %s, r6 = %s\n", RsgToString(r5, 2, -16), RsgToString(r6, 2, -16));

	//r5[0] = 1999; r5[1] = 1;
	//r6[0] = 20; r6[1] = 0; r6[2] = 2;
	//RsgSubComple(r6, r5, 1);
	//printf("r6-=r5: r6=%s\n", RsgToString(r6, 3, -16));

	//size_t r7[5] = {0x12,0x34,0x56,0x78,0x9};
	//size_t r8[5] = {0x55,0x44};
	//res = RsgDiv(r7, r8, 5);
	//printf("r7/r8: r7=%s, r8=%s\n", RsgToString(r7, 5, -16), RsgToString(r8, 5, -16));

	//size_t r7[5] = { 0x13,0x34,0x56,0x78,0x9 };
	//size_t r8[5] = { 0x2 };
	//res = RsgDiv(r7, r8, 5);
	//printf("r7/r8: r7=%s, r8=%s\n", RsgToString(r7, 5, -16), RsgToString(r8, 5, -16));

	//size_t r7[5] = { 9 };
	//size_t r8[5] = { 24 };
	//res = RsgPow(r7, r8, 5);
	//printf("9^24=%s [%d]\n", RsgToString(r7, 5, 10), res);
	
	//size_t r7[5] = { 10 };
	//size_t r8[5] = { 3 };
	//res = RsgArrange(r7, r8, 5);
	//printf("A10_3=%s [%d]\n", RsgToString(r7, 5, 10), res);

	//size_t r7[5] = { 4 };
	//res = RsgFactorial(r7, 5);
	//printf("4!=%s [%d]\n", RsgToString(r7, 5, 10), res);

	//size_t r7[5] = { 10 };
	//size_t r8[5] = { 3 };
	//res = RsgCombinate(r7, r8, 5);
	//printf("C10_3=%s [%d]\n", RsgToString(r7, 5, 10), res);

	//size_t r7[5] = { 3*7 };
	//size_t r8[5] = { 7*91 };
	//res = RsgComDiv(r7, r8, 5);
	//printf("ComDiv of 3*7 and 7*91 = %s [%d]\n", RsgToString(r7, 5, 10), res);

	//size_t r7[5] = { 3*7 };
	//size_t r8[5] = { 7*4 };
	//res = RsgComMul(r7, r8, 5);
	//printf("ComMul of 3*7 and 4*7 = %s [%d]\n", RsgToString(r7, 5, 10), res);
}