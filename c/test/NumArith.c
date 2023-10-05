// ASCII GCC RFC04 by Dscn.Haruno
// Test for CPL version of NumAr
// // {TEMP} for Complex but 4D
//$ gcc NumAr.c ../source/numar.c ../source/cdear.c ../source/hstring.c ../source/ustring.c

#include "../ustring.h"
#include "../numar.h"
#include "stdio.h"
#include "inttypes.h"
ulibsym(1024);
void erro(char* str) { puts(str); }

int main()
{
	numa* num;
	numa* ext;
	char* str;

	NumInit();
	extern size_t LIB_CDE_PRECISE_SHOW;
	LIB_CDE_PRECISE_SHOW = 8;
	
	{
		num = NumNewComplex("+1", "+0", "+1", "+2", "+0", "+1");// 1 + 2i
		coe* arg = NumAng(num);
		coe* abs = NumAbs(num);
		char* argtxt = CoeToLocale(arg, 1);
		char* abstxt = CoeToLocale(abs, 1);
		printf("(1+2i) Ang=%s, Abs=%s\n", argtxt, abstxt);
		NumDel(num); CoeDel(arg); CoeDel(abs); memf(argtxt); memf(abstxt);
		
		for (int i = 0; i < 3; i++)
			for (int j = 0; j < 3; j++)
			{
				num = NumNewComplex(i == 0 ? "-1" : i == 1 ? "+0" : "+1", "+0", "+1",
					j == 0 ? "-1" : j == 1 ? "+0" : "+1", "+0", "+1");
				arg = NumAng(num);
				abs = NumAbs(num);
				argtxt = CoeToLocale(arg, 1);
				abstxt = CoeToLocale(abs, 1);
				printf("(%s+j%s) Ang=%s, Abs=%s\n", num->Real.coff, num->Imag.coff, argtxt, abstxt);
				NumDel(num); CoeDel(arg); CoeDel(abs); memf(argtxt); memf(abstxt);
			}
		puts("");
	}
	// +
	{
		num = NumNewComplex("+1", "+0", "+1", "+2", "+0", "+1");// 1 + 2i
		ext = NumNewComplex("+3", "+0", "+1", "+4", "+0", "+1");// 3 + 4i
		NumAdd(num, ext);// 4 +6i
		str = NumToString(num, 1);
		printf("(1+2i) + (3+4i) = (%s)\n", str);
		NumDel(num); NumDel(ext); memf(str);
	}
	// -
	{
		num = NumNewComplex("+1", "+0", "+1", "+2", "+0", "+1");// 1 + 2i
		ext = NumNewComplex("+3", "+0", "+1", "+4", "+0", "+1");// 3 + 4i
		NumSub(num, ext);// -2 -2i
		str = NumToString(num, 1);
		printf("(1+2i) - (3+4i) = (%s)\n", str);
		NumDel(num); NumDel(ext); memf(str);
	}
	// *
	{
		num = NumNewComplex("+1", "+0", "+1", "-2", "+0", "+1");// 1 - 2i
		ext = NumNewComplex("+3", "+0", "+1", "-4", "+0", "+1");// 3 - 4i
		NumMul(num, ext);// -5 -10i
		str = NumToString(num, 1);
		printf("(1+2i) * (3+4i) = (%s)\n", str);
		NumDel(num); NumDel(ext); memf(str);
	}
	// /
	{
		num = NumNewComplex("+1", "+0", "+1", "-2", "+0", "+1");// 1 - 2i
		ext = NumNewComplex("+3", "+0", "+1", "-4", "+0", "+1");// 3 - 4i
		NumDiv(num, ext);
		str = NumToString(num, 1);
		printf("(1+2i) / (3+4i) = (%s)\n", str);
		cplx_d cplx = NumToComplex(num);
		printf("\tReal=%lf, Imag=%lf\n", ((double*)&cplx)[0], ((double*)&cplx)[1]);
		NumDel(num); NumDel(ext); memf(str);
	}
	// ^
	
	{
		cplx_d cplx = { 1, -2 };
		num = NumFromComplex(cplx);
		ext = NumNewComplex("+3", "+0", "+1", "-4", "+0", "+1");// 3 - 4i
		NumPow(num, ext);
		str = NumToString(num, 1);
		printf("(1+2i) ^ (3+4i) = (%s)\n", str);
		NumDel(num); NumDel(ext); memf(str);
	}
	// sin
	{
		num = NumNewComplex("+1", "+0", "+1", "-2", "+0", "+1");// 1 - 2i
		NumSin(num);
		str = NumToString(num, 1);
		printf("sin(1-2i) = (%s)\n", str);
		NumDel(num); memf(str);
	}
	// cos
	{
		num = NumNewComplex("+1", "+0", "+1", "-2", "+0", "+1");// 1 - 2i
		NumCos(num);
		str = NumToString(num, 1);
		printf("cos(1-2i) = (%s)\n", str);
		NumDel(num); memf(str);
	}
	// tan
	{
		num = NumNewComplex("+1", "+0", "+1", "-2", "+0", "+1");// 1 - 2i
		NumTan(num);
		str = NumToString(num, 1);
		printf("tan(1-2i) = (%s)\n", str);
		NumDel(num); memf(str);
	}
	// asin
	{
		num = NumNewComplex("+1", "+0", "+1", "-2", "+0", "+1");// 1 - 2i
		NumAsin(num);
		str = NumToString(num, 1);
		printf("asin(1-2i) = (%s)\n", str);
		NumDel(num); memf(str);
	}
	// acos
	{
		num = NumNewComplex("+1", "+0", "+1", "-2", "+0", "+1");// 1 - 2i
		NumAcos(num);
		str = NumToString(num, 1);
		printf("acos(1-2i) = (%s)\n", str);
		NumDel(num); memf(str);
	}
	// atan
	{
		num = NumNewComplex("+1", "+0", "+1", "-2", "+0", "+1");// 1 - 2i
		NumAtan(num);
		str = NumToString(num, 1);
		printf("atan(1-2i) = (%s)\n", str);
		NumDel(num); memf(str);
	}
	// sinh
	{
		num = NumNewComplex("+1", "+0", "+1", "-2", "+0", "+1");// 1 - 2i
		NumSinh(num);
		str = NumToString(num, 1);
		printf("sinh(1-2i) = (%s)\n", str);
		NumDel(num); memf(str);
	}
	// cosh
	{
		num = NumNewComplex("+1", "+0", "+1", "-2", "+0", "+1");// 1 - 2i
		NumCosh(num);
		str = NumToString(num, 1);
		printf("cosh(1-2i) = (%s)\n", str);
		NumDel(num); memf(str);
	}
	// tanh
	{
		num = NumNewComplex("+1", "+0", "+1", "-2", "+0", "+1");// 1 - 2i
		NumTanh(num);
		str = NumToString(num, 1);
		printf("tanh(1-2i) = (%s)\n", str);
		NumDel(num); memf(str);
	}
	// asinh
	{
		num = NumNewComplex("+1", "+0", "+1", "-2", "+0", "+1");// 1 - 2i
		NumAsinh(num);
		str = NumToString(num, 1);
		printf("asinh(1-2i) = (%s)\n", str);
		NumDel(num); memf(str);
	}
	// acosh
	{
		num = NumNewComplex("+1", "+0", "+1", "+2", "+0", "+1");// 1 + 2i
		NumAcosh(num);
		str = NumToString(num, 1);
		printf("acosh(1+2i) = (%s)\n", str);
		NumDel(num); memf(str);
	}
	// atanh
	{
		num = NumNewComplex("+1", "+0", "+1", "+2", "+0", "+1");// 1 + 2i
		NumAtanh(num);
		str = NumToString(num, 1);
		printf("atanh(1+2i) = (%s)\n", str);
		NumDel(num); memf(str);
	}
	// log
	{
		num = NumNewComplex("+1", "+0", "+1", "+2", "+0", "+1");// 1 + 2i
		NumLog(num);
		str = NumToString(num, 1);
		printf("log(1+2i) = (%s)\n", str);
		NumDel(num); memf(str);
	}
	// exp
	{
		num = NumNewComplex("+1", "+0", "+1", "+2", "+0", "+1");// 1 + 2i
		NumExp(num);
		str = NumToString(num, 1);
		printf("exp(1+2i) = (%s)\n", str);
		NumDel(num); memf(str);
	}
	// sqrt
	{
		num = NumNewComplex("+1", "+0", "+1", "+2", "+0", "+1");// 1 + 2i
		NumSqrt(num);
		str = NumToString(num, 1);
		printf("sqrt(1+2i) = (%s)\n", str);
		NumDel(num); memf(str);
	}

	if(malc_count) printf("System leak: %" PRIiPTR " times!\n", malc_count);
	return 0;
}
