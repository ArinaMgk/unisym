// ASCII
/*
	Copyright 2023 ArinaMgk

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/
#include "../../ustring.h"
#include "../../cdear.h"
#include "../../alice.h"
#include "../../aldbg.h"
#include "../../NumAr.h"

struct NumFlag_t NumFlag;

const static scalar i = 
{
	.Real.coff = "+0", .Real.expo = "+0", .Real.divr = "+1",
	.Imag.coff = "+1", .Imag.expo = "+0", .Imag.divr = "+1"
};
const static scalar numone =
{
	.Real.coff = "+1",.Real.expo = "+0",.Real.divr = "+1",
	.Imag.coff = "+0",.Imag.expo = "+0",.Imag.divr = "+1",
};
const static scalar numneg = 
{ 
	.Real.coff = "-1",.Real.expo="+0",.Real.divr="+1",
	.Imag.coff = "+0",.Imag.expo = "+0",.Imag.divr = "+1",
};
const static scalar numtwo =
{
	.Real.coff = "+2",.Real.expo = "+0",.Real.divr = "+1",
	.Imag.coff = "+0",.Imag.expo = "+0",.Imag.divr = "+1",
};
const static scalar num2i =
{
	.Real.coff = "+0",.Real.expo = "+0",.Real.divr = "+1",
	.Imag.coff = "+2",.Imag.expo = "+0",.Imag.divr = "+1",
};
const static scalar numnegi =
{
	.Real.coff = "+0",.Real.expo = "+0",.Real.divr = "+1",
	.Imag.coff = "-1",.Imag.expo = "+0",.Imag.divr = "+1",
};
const static scalar numhalf =
{
	.Real.coff = "+1",.Real.expo = "+0",.Real.divr = "+2",
	.Imag.coff = "+0",.Imag.expo = "+0",.Imag.divr = "+1",
};


scalar* NumNew(const char* Rcof, const char* Rexp, const char* Rdiv,
	const char* Icof, const char* Iexp, const char* Idiv)
{
	scalar* s;
	memalloc(s, sizeof(scalar));
	s->Real.coff = StrHeap(Rcof);
	s->Real.expo = StrHeap(Rexp);
	s->Real.divr = StrHeap(Rdiv);
	s->Imag.coff = StrHeap(Icof);
	s->Imag.expo = StrHeap(Iexp);
	s->Imag.divr = StrHeap(Idiv);
	return s;
}

scalar* NumCpy(const scalar* num)
{
	return NumNew(num->Real.coff, num->Real.expo, num->Real.divr, num->Imag.coff, num->Imag.expo, num->Imag.divr);
}

void NumDel(scalar* num)
{
	memfree(num->Real.coff);
	memfree(num->Real.expo);
	memfree(num->Real.divr);
	memfree(num->Imag.coff);
	memfree(num->Imag.expo);
	memfree(num->Imag.divr);
	memfree(num);
}

// Convert

cplx_l NumToLDComplex(const scalar* dest)
{
#ifdef _MSVC
	cplx_l ret = { CoeToLDouble(&dest->Real) , CoeToLDouble(&dest->Imag) };
#else
	cplx_d ret;
	((long double*)&ret)[0] = CoeToLDouble(&dest->Real);
	((long double*)&ret)[1] = CoeToLDouble(&dest->Imag);
#endif
	return ret;
}

cplx_d NumToDComplex(const scalar* dest)
{
#ifdef _MSVC
	cplx_d ret = { CoeToDouble(&dest->Real) , CoeToDouble(&dest->Imag) };
#else
	cplx_d ret;
	((double*)&ret)[0] = CoeToDouble(&dest->Real);
	((double*)&ret)[1] = CoeToDouble(&dest->Imag);
#endif
	return ret;
}

cplx_f NumToFComplex(const scalar* dest)
{
#ifdef _MSVC
	cplx_f ret = { CoeToFloat(&dest->Real) , CoeToFloat(&dest->Imag) };
#else
	cplx_d ret;
	((float*)&ret)[0] = CoeToFloat(&dest->Real);
	((float*)&ret)[1] = CoeToFloat(&dest->Imag);
#endif
	return ret;
}

scalar* NumFromLDComplex(cplx_l flt)
{
#ifdef _MSVC
	coe* c1 = CoeFromLDouble(flt._Val[0]);
	coe* c2 = CoeFromLDouble(flt._Val[1]);
#else
	coe* c1 = CoeFromLDouble(((long double*)&flt)[0]);
	coe* c2 = CoeFromLDouble(((long double*)&flt)[1]);
#endif
	scalar* res = NumNew(c1->coff, c1->expo, c1->divr, c2->coff, c2->expo, c2->divr);
	CoeDel(c1); CoeDel(c2);
	return res;
}

scalar* NumFromDComplex(cplx_d flt)
{
#ifdef _MSVC
	coe* c1 = CoeFromDouble(flt._Val[0]);
	coe* c2 = CoeFromDouble(flt._Val[1]);
#else
	coe* c1 = CoeFromDouble(((double*)&flt)[0]);
	coe* c2 = CoeFromDouble(((double*)&flt)[1]);
#endif
	scalar* res = NumNew(c1->coff, c1->expo, c1->divr, c2->coff, c2->expo, c2->divr);
	CoeDel(c1); CoeDel(c2);
	return res;
}

scalar* NumFromFComplex(cplx_f flt)
{
#ifdef _MSVC
	coe* c1 = CoeFromFloat(flt._Val[0]);
	coe* c2 = CoeFromFloat(flt._Val[1]);
#else
	coe* c1 = CoeFromFloat(((float*)&flt)[0]);
	coe* c2 = CoeFromFloat(((float*)&flt)[1]);
#endif
	scalar* res = NumNew(c1->coff, c1->expo, c1->divr, c2->coff, c2->expo, c2->divr);
	CoeDel(c1); CoeDel(c2);
	return res;
}

scalar* NumFromLLong(long long signedll)
{
	scalar* ret; memalloc(ret, sizeof(scalar));
	coe* tmpcoe = CoeFromLLong(signedll);
	ret->Real = *tmpcoe;
	memfree(tmpcoe);// keep the string in heap
	ret->Imag.coff = StrHeap("+0");
	ret->Imag.expo = StrHeap("+0");
	ret->Imag.divr = StrHeap("+1");
	return ret;
}

// Operator
static coe* NumAbs(const scalar* s)// r
{
	// hypot
	coe* re = CoeCpy(&s->Real);
	CoeHypot(re, &s->Imag);
	return re;
}
static coe* NumAng(const scalar* s)// theta
{
	// (-pi,pi]
	int statex = CoeCmp(&s->Real, &(coe){.coff = "+0", .expo = "+0", .divr = "+1" });
	int statey = CoeCmp(&s->Imag, &(coe){.coff = "+0", .expo = "+0", .divr = "+1" });
	if (!statey)
	{
		coe* cpi = CoePi();
		return cpi;
	}
	else if (!statex)
	{
		coe* cpi = CoePi();
		CoeDiv(cpi, &(coe){.coff = "+2", .expo = "+0", .divr = "+1" });
		if (statey < 0) cpi->coff[1] = '-';
		return cpi;
	}
	coe* ret = CoeCpy(&s->Imag);
	CoeDiv(ret, &s->Real);
	CoeAtan(ret);
	if (statex < 0)
	{
		coe* cpi = CoePi();
		if (statey < 0) cpi->coff[1] = '-';
		CoeAdd(ret, cpi);
		CoeDel(cpi);
	}
	return ret;
}

void NumAdd(scalar* dest, const scalar* sors)
{
	CoeAdd(&dest->Real, &sors->Real);
	CoeAdd(&dest->Imag, &sors->Imag);
}

void NumSub(scalar* dest, const scalar* sors)
{
	CoeSub(&dest->Real, &sors->Real);
	CoeSub(&dest->Imag, &sors->Imag);
}

void NumMul(scalar* dest, const scalar* sors)
{
	// (a+bi)*(c+di) = ac-bd + i(ad+bc)
	coe* s1 = CoeCpy(&dest->Real); CoeMul(s1, &sors->Real);// ac
	coe* s2 = CoeCpy(&dest->Imag); CoeMul(s2, &sors->Imag);// bd
	coe* s3 = CoeCpy(&dest->Real); CoeMul(s3, &sors->Imag);// ad
	coe* s4 = CoeCpy(&dest->Imag); CoeMul(s4, &sors->Real);// bc
	s1 = CoeSub(s1, s2);// ac-bd
	s3 = CoeAdd(s3, s4);// ad+bc
	// *&dest->Real = *s1; 
	srs(dest->Real.coff, StrHeap(s1->coff));
	srs(dest->Real.expo, StrHeap(s1->expo));
	srs(dest->Real.divr, StrHeap(s1->divr));
	// *&dest->Imag = *s3; 
	srs(dest->Imag.coff, StrHeap(s3->coff));
	srs(dest->Imag.expo, StrHeap(s3->expo));
	srs(dest->Imag.divr, StrHeap(s3->divr));
	CoeDel(s1); CoeDel(s2); CoeDel(s3); CoeDel(s4);
}

void NumDiv(scalar* dest, const scalar* sors)
{
	// (s1+m1) / (s2+m2)
	// (s1+m1)(s2-m2) / (s2*s2 + m2*m2)
	// (s1*s2+m1*m2 -s1*m2+s2*m1) / (s2*s2 + m2*m2)
	// (A+B)/(E+F) (D-C)m/(E+F)
	// t1/t5 t4/t5
	coe* t1, * t2, * t3, * t4, * t5, * t6;
	t1 = CoeCpy(&dest->Real); CoeMul(t1, &sors->Real);
	t2 = CoeCpy(&dest->Imag); CoeMul(t2, &sors->Imag);
	t3 = CoeCpy(&dest->Real); CoeMul(t3, &sors->Imag);
	t4 = CoeCpy(&sors->Real); CoeMul(t4, &dest->Imag);
	t5 = CoeCpy(&sors->Real); CoeMul(t5, &sors->Real);
	t6 = CoeCpy(&sors->Imag); CoeMul(t6, &sors->Imag);
	CoeAdd(t5, t6);
	CoeAdd(t1, t2);
	CoeSub(t4, t3);
	CoeDiv(t1, t5);
	CoeDiv(t4, t5);
	// dest->Real = NumCpy(t1);
	srs(dest->Real.coff, StrHeap(t1->coff));
	srs(dest->Real.expo, StrHeap(t1->expo));
	srs(dest->Real.divr, StrHeap(t1->divr));
	// dest->Imag = NumCpy(t4);
	srs(dest->Imag.coff, StrHeap(t4->coff));
	srs(dest->Imag.expo, StrHeap(t4->expo));
	srs(dest->Imag.divr, StrHeap(t4->divr));
	CoeDel(t1); CoeDel(t2); CoeDel(t3); CoeDel(t4); CoeDel(t5); CoeDel(t6);
}

void NumFactorial(scalar* dest)
{
	if (!StrCompare("-1", dest->Real.expo)) dest->Real.expo[0] = '+';
	if (StrCompare(dest->Real.divr, "+1") || dest->Real.expo[0] == '-' || dest->Imag.coff[1] != '0')
	{
		// erro("Fac Must input a real interger.");
		//TODO.
		return;
	}
	CoeFac(&dest->Real);
}

void NumPow(scalar* dest, const scalar* sors)
{
#ifdef _MSVC
	cplx_d
		d = { CoeToDouble(&dest->Real), CoeToDouble(&dest->Imag) },
		s = { CoeToDouble(&sors->Real), CoeToDouble(&sors->Imag) };
#else
	cplx_d d, s;
	((double*)&d)[0] = CoeToDouble(&dest->Real);
	((double*)&d)[1] = CoeToDouble(&dest->Imag);
	((double*)&s)[0] = CoeToDouble(&sors->Real);
	((double*)&s)[1] = CoeToDouble(&sors->Imag);
#endif
	d = cpow(d, s);
	scalar* res = NumFromDComplex(d);
	srs(dest->Real.coff, StrHeap(res->Real.coff));
	srs(dest->Real.expo, StrHeap(res->Real.expo));
	srs(dest->Real.divr, StrHeap(res->Real.divr));
	srs(dest->Imag.coff, StrHeap(res->Imag.coff));
	srs(dest->Imag.expo, StrHeap(res->Imag.expo));
	srs(dest->Imag.divr, StrHeap(res->Imag.divr));
	NumDel(res);
	return;// res
}

void NumSin(scalar* dest)
{
	//=(exp(iz)-exp(-iz))/(2i)
	NumMul(dest, &i);
	scalar* another = NumCpy(dest);
	NumMul(another, &numneg);
	NumExp(dest); NumExp(another);
	NumSub(dest, another);
	NumDiv(dest, &num2i);
	NumDel(another);
}

void NumCos(scalar* dest)
{
	//=(exp(iz)+exp(-iz))/2
	NumMul(dest, &i);
	scalar* another = NumCpy(dest);
	NumMul(another, &numneg);
	NumExp(dest); NumExp(another);
	NumAdd(dest, another);
	NumDiv(dest, &numtwo);
	NumDel(another);
}

void NumTan(scalar* dest)
{
	scalar* tmp = NumCpy(dest);
	NumCos(tmp);
	NumSin(dest);
	NumDiv(dest, tmp);
	NumDel(tmp);
}

void NumAsin(scalar* dest)
{
	//  ln(iz+sqrt(1-z*z))*(-i)
	// no check definition, TODO.
	scalar* dd = NumCpy(dest);
	NumMul(dd, dest); NumMul(dd, &numneg); NumAdd(dd, &numone);// 1-z*z
	NumPow(dd, &numhalf);
	NumMul(dest, &i);
	NumAdd(dest, dd);
	NumLog(dest);
	NumMul(dest, &numnegi);
	NumDel(dd);
	return;
}

void NumAcos(scalar* dest)
{
	//  ln(z+sqrt(z*z-1))*(-i)
	// no check definition, TODO.
	scalar* dd = NumCpy(dest);
	NumMul(dd, dest); NumSub(dd, &numone);// z*z-1
	NumPow(dd, &numhalf);
	NumAdd(dest, dd);
	NumLog(dest);
	NumMul(dest, &numnegi);
	NumDel(dd);
	return;
}

void NumAtan(scalar* dest)
{
	// (-i)* (ln ((1+i*z)/(1-i*z)) )/2 ?
	// (-i)* ln div (i-z,i+z) /2
	// no check definition, TODO.
	//NumMul(dest, &i);
	//scalar* ano = NumCpy(dest);
	//NumMul(ano, &numneg);
	//NumAdd(dest, &numone);
	//NumAdd(ano, &numone);
	//NumDiv(dest, ano);
	//NumLog(dest);
	//NumDiv(dest, &numtwo);
	//NumMul(dest, &numnegi);
	//NumDel(ano);
	scalar* ano = NumCpy(dest);
	NumMul(dest, &numneg);
	NumAdd(dest, &i);
	NumAdd(ano, &i);
	NumDiv(dest, ano);
	NumLog(dest);
	NumDiv(dest, &numtwo);
	NumMul(dest, &numnegi);
	NumDel(ano);
	return;
}

void NumLog(scalar* dest)
{
	// {ln abs z, arg z}
	coe* real = NumAbs(dest);
	coe* imag = NumAng(dest);
	CoeLog(real);
	srs(dest->Real.coff, StrHeap(real->coff));
	srs(dest->Real.expo, StrHeap(real->expo));
	srs(dest->Real.divr, StrHeap(real->divr));
	srs(dest->Imag.coff, StrHeap(imag->coff));
	srs(dest->Imag.expo, StrHeap(imag->expo));
	srs(dest->Imag.divr, StrHeap(imag->divr));
	CoeDel(real);
	CoeDel(imag);
	return;
}

void NumExp(scalar* dest)
{
	// Exp(a+bi)=Exp(a)*(cos b +i*sin b)
	CoeExp(&dest->Real);
	coe* cospart = CoeCpy(&dest->Imag); CoeCos(cospart);
	coe* sinpart = CoeCpy(&dest->Imag); CoeSin(sinpart);
	srs(dest->Imag.coff, StrHeap(dest->Real.coff));
	srs(dest->Imag.expo, StrHeap(dest->Real.expo));
	srs(dest->Imag.divr, StrHeap(dest->Real.divr));
	CoeMul(&dest->Real, cospart);
	CoeMul(&dest->Imag, sinpart);
	CoeDel(sinpart);
	CoeDel(cospart);
	return;
}

void NumSinh(scalar* dest)
{
	//=(exp(z)-exp(-z))/2
	scalar* another = NumCpy(dest);
	NumMul(another, &numneg);
	NumExp(dest); NumExp(another);
	NumSub(dest, another);
	NumDiv(dest, &numtwo);
	NumDel(another);
}

void NumCosh(scalar* dest)
{
	//=(exp(z)+exp(-z))/2
	scalar* another = NumCpy(dest);
	NumMul(another, &numneg);
	NumExp(dest); NumExp(another);
	NumAdd(dest, another);
	NumDiv(dest, &numtwo);
	NumDel(another);
}

void NumTanh(scalar* dest)
{
	scalar* cosh_t = NumCpy(dest); NumCosh(cosh_t);
	scalar* sinh_t = dest; NumSinh(sinh_t);
	NumDiv(sinh_t, cosh_t);
	NumDel(cosh_t);
}

void NumAsinh(scalar* dest)
{
	//  ln( z+ sqrt(z*z+1) )
	// no check definition, TODO.
	scalar* dd = NumCpy(dest);
	NumMul(dd, dest); NumAdd(dd, &numone);// z*z+1
	NumPow(dd, &numhalf);
	NumAdd(dest, dd);
	NumLog(dest);
	NumDel(dd);
	return;
}

void NumAcosh(scalar* dest) 
{
	//  ln( z+ sqrt(z*z-1) )
	// no check definition, TODO.
	scalar* dd = NumCpy(dest);
	NumMul(dd, dest); NumSub(dd, &numone);// z*z-1
	NumPow(dd, &numhalf);
	NumAdd(dest, dd);
	NumLog(dest);
	NumDel(dd);
	return;
}

void NumAtanh(scalar* dest)
{
	//  ln( div( 1+z, 1-z ) )/2
	// no check definition, TODO.
	scalar* dd = NumCpy(dest); NumMul(dd, &numneg);
	NumAdd(dest, &numone); NumAdd(dd, &numone);
	NumDiv(dest, dd);
	NumLog(dest);
	NumDiv(dest, &numtwo);
	NumDel(dd);
	return;
}

// opt: 0[auto] 1[int or float] 2[e format]
size_t NumToString(const scalar* dest, int opt, char* buf, size_t buflen)
{
	size_t pos = 0;
	char c;
	char
		* str1 = CoeToLocaleClassic(&dest->Real, opt),
		* str2 = CoeToLocaleClassic(&dest->Imag, opt);
	for (size_t i = 0; c = str1[i]; i++)
	{
		if (pos >= buflen) break;
		buf[pos++] = c;
	}
	if (pos + 1 < buflen) buf[pos++] = ' ';
	for (size_t i = 0; c = str2[i]; i++)
	{
		if (pos >= buflen) break;
		buf[pos++] = c;
	}
	if (pos + 1 < buflen) buf[pos++] = 'i';
	buf[min(pos, buflen - 1)] = 0;
	memfree(str1);
	memfree(str2);
	return pos;
}

char* NumToStringAuto(const scalar* dest, int opt)
{
	size_t pos = 0;
	char c;
	char
		* str1 = CoeToLocaleClassic(&dest->Real, opt),
		* str2 = CoeToLocaleClassic(&dest->Imag, opt);
	size_t alsize = StrLength(str1) + StrLength(str2) + 2 + 1 + 3;// 2 \0 and a space and 3 safety.
	char* buf; memalloc(buf, alsize);
	for (size_t i = 0; c = str1[i]; i++)
	{
		buf[pos++] = c;
	}
	if (dest->Imag.coff[1] != '0')
	{
		buf[pos++] = ' ';
		for (size_t i = 0; c = str2[i]; i++)
		{
			buf[pos++] = c;
		}
		buf[pos++] = 'i';
	}
	buf[min(pos, alsize - 1)] = 0;
	memfree(str1);
	memfree(str2);
	return buf;
}
