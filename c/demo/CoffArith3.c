#include<stddef.h>
#include<stdio.h>
#include<stdlib.h>
#include<inttypes.h>
#include"../ustring.h"
#include"../coear.h"
#include"../alice_dbg.h"
#include"../../../unisym.h"
#include"../chrar.h"

void erro(char* errmsg)
{
	puts(errmsg);
	exit(0);
}

void CoePrint(coe* co)
{
	const char* fmt = "[%s]*[%s]x10^[%s]/[%s]\n";
	const char* tmp_flag = "~";
	printf(fmt, co->coff, tmp_flag, co->expo, co->divr);
}

int main()
{
	coe* c;
	coe* cc;
	coe* ccc;
	char* tmp;
	// CoePow(c, cc);
	int ALL = 1;
	
	if (ALL)
	{
		c = CoeNew("+157", "-2", "+1");
		cc = CoeNew("-31415", "-4", "+1");
		ccc = CoeNew("-1", "+1", "+1");
		// CoePrint(CoexSub(CoeCpy(c), CoeCpy(c)));

		CoeSinh(c); 
		CoeDivrUnit(c, show_precise);
		tmp = CoeToLocaleClassic(c, 1); CoeDel(c);
		printf("sinh +1.57 = %s [%llu]\n", tmp, lup_last); memfree(tmp);

		CoeCosh(cc); CoeDivrUnit(cc, show_precise);
		tmp = CoeToLocaleClassic(cc, 1); CoeDel(cc);
		printf("cosh -3.1415 = %s [%llu]\n", tmp, lup_last); memfree(tmp);

		CoeTanh(ccc); CoeDivrUnit(ccc, show_precise);
		tmp = CoeToLocaleClassic(ccc, 1); CoeDel(ccc);
		printf("tanh -10 = %s [%llu]\n", tmp, lup_last); memfree(tmp);
	}
	if (ALL)
	{
		c = CoeNew("+157", "-2", "+1");
		cc = CoeNew("+31415", "-4", "+1");
		ccc = CoeNew("-1", "-1", "+1");

		CoeAsinh(c); CoeDivrUnit(c, show_precise);
		tmp = CoeToLocaleClassic(c, 1); CoeDel(c);
		printf("asinh +1.57 = %s [%llu]\n", tmp, lup_last); memfree(tmp);

		CoeAcosh(cc); 
		CoeDivrUnit(cc, show_precise);
		tmp = CoeToLocaleClassic(cc, 1); CoeDel(cc);
		printf("acosh +3.1415 = %s [%llu]\n", tmp, lup_last); memfree(tmp);

		CoeAtanh(ccc); 
		// atanh -10 --> NAN
		CoeDivrUnit(ccc, show_precise);
		tmp = CoeToLocaleClassic(ccc, 1); CoeDel(ccc);
		printf("atanh -0.1 = %s [%llu]\n", tmp, lup_last); memfree(tmp);
	}
	{
		c = CoeNew("+2", "+0", "+1");
		cc = CoeNew("+1", "+0", "+2");// 1/2
		CoePow(c, cc); CoeDivrUnit(c, show_precise);
		tmp = CoeToLocaleClassic(c, 1); CoeDel(c); CoeDel(cc);
		printf("sqrt +2 = %s [%llu]\n", tmp, lup_last); memfree(tmp);
	}
	{
		c = CoeNew("+2", "+0", "+1");
		cc = CoeNew("-1", "+0", "+1");// 1/2
		CoePow(c, cc); CoeDivrUnit(c, show_precise);
		tmp = CoeToLocaleClassic(c, 1); CoeDel(c); CoeDel(cc);
		printf("1/2 = %s [%llu]\n", tmp, lup_last); memfree(tmp);
	}

	printf("%lld", malc_count);
}


