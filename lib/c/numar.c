// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant Host[Allocation]
// LastCheck: RFZ23
// AllAuthor: @dosconio
// ModuTitle: Number Arithmetic
/*
	Copyright 2023 ArinaMgk

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0
	http://unisym.org/license.html

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/
#define _LIB_STRING_HEAP
#include "../../inc/c/ustring.h"
#include "../../inc/c/numar.h"

#define scal(iden,rc,re,rd,ic,ie,id) const static dima iden={\
	.Real.coff=rc,.Real.expo=re,.Real.divr=rd,\
	.Imag.coff=ic,.Imag.expo=ie,.Imag.divr=id}

//{TODO} aflag using

scal(i, "+0", "+0", "+1", "+1", "+0", "+1");
scal(numone, "+1", "+0", "+1", "+0", "+0", "+1");
scal(numneg, "-1", "+0", "+1", "+0", "+0", "+1");
scal(numtwo, "+2", "+0", "+1", "+0", "+0", "+1");
scal(num2i, "+0", "+0", "+1", "+2", "+0", "+1");
scal(numnegi, "+0", "+0", "+1", "-1", "+0", "+1");
scal(numhalf, "+1", "+0", "+2", "+0", "+0", "+1");

dima* NumNewComplex(const char* Rcof, const char* Rexp, const char* Rdiv,
	const char* Icof, const char* Iexp, const char* Idiv)
{
	dima* s = zalcof(dima);
	s->Real.coff = StrHeap(Rcof);
	s->Real.expo = StrHeap(Rexp);
	s->Real.divr = StrHeap(Rdiv);
	s->Imag.coff = StrHeap(Icof);
	s->Imag.expo = StrHeap(Iexp);
	s->Imag.divr = StrHeap(Idiv);
	return s;
}

dima* NumNew(const char* xcoff, const char* ycoff, const char* zcoff, const char* tcoff)
{
	dima* res = zalcof(dima);
	res->x.coff = StrHeap(xcoff); res->x.expo = StrHeap("+0"); res->x.divr = StrHeap("+1");
	res->y.coff = StrHeap(ycoff); res->y.expo = StrHeap("+0"); res->y.divr = StrHeap("+1");
	res->z.coff = StrHeap(zcoff); res->z.expo = StrHeap("+0"); res->z.divr = StrHeap("+1");
	res->t.coff = StrHeap(tcoff); res->t.expo = StrHeap("+0"); res->t.divr = StrHeap("+1");
	return res;
}

dima* NumCpy(const dima* num)
{
	// return NumNewComplex(num->Real.coff, num->Real.expo, num->Real.divr, num->Imag.coff, num->Imag.expo, num->Imag.divr);
	dima* res = zalcof(dima);
	{
		res->x.coff = StrHeap(num->x.coff);
		res->x.expo = StrHeap(num->x.expo);
		res->x.divr = StrHeap(num->x.divr);
		res->t.coff = StrHeap(num->t.coff);
		res->t.expo = StrHeap(num->t.expo);
		res->t.divr = StrHeap(num->t.divr);
	}
	if (num->y.coff)// if not 2d, then 4d
	{
		res->y.coff = StrHeap(num->y.coff);
		res->y.expo = StrHeap(num->y.expo);
		res->y.divr = StrHeap(num->y.divr);
		res->z.coff = StrHeap(num->z.coff);
		res->z.expo = StrHeap(num->z.expo);
		res->z.divr = StrHeap(num->z.divr);
	}
	return res;
}

void NumDel(dima* num)
{
	memfree(num->Real.coff);
	memfree(num->Real.expo);
	memfree(num->Real.divr);
	memfree(num->Imag.coff);
	memfree(num->Imag.expo);
	memfree(num->Imag.divr);
	if (num->y.coff)
	{
		memfree(num->y.coff);
		memfree(num->y.expo);
		memfree(num->y.divr);
		memfree(num->z.coff);
		memfree(num->z.expo);
		memfree(num->z.divr);
	}
	memfree(num);
}

// ---- ---- ---- ---- Convert ---- ---- ---- ---- 

cplx_d NumToComplex(const dima* dest)
{
	//MSVC cplx_d ret = { CoeToDouble(&dest->Real) , CoeToDouble(&dest->Imag) };
	cplx_d ret;
	((double*)&ret)[0] = CoeToDouble(&dest->Real);
	((double*)&ret)[1] = CoeToDouble(&dest->Imag);
	return ret;
}

dima* NumFromComplex(cplx_d flt)
{
	//MSVC coe* c1 = CoeFromDouble(flt._Val[0]);
	//MSVC coe* c2 = CoeFromDouble(flt._Val[1]);
	coe* c1 = CoeFromDouble(((double*)&flt)[0]);
	coe* c2 = CoeFromDouble(((double*)&flt)[1]);
	dima* res = NumNewComplex(c1->coff, c1->expo, c1->divr, c2->coff, c2->expo, c2->divr);
	CoeDel(c1); CoeDel(c2);
	return res;
}

char* NumToString(const dima* dest, int opt)
{
	size_t pos = 0;
	char c;
	char
		* str1 = CoeToLocale(&dest->Real, opt),
		* str2 = CoeToLocale(&dest->Imag, opt);
	size_t alsize = StrLength(str1) + StrLength(str2) + 3;// \0, space and i
	char* buf = salc(alsize);
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
	memfree(str1);
	memfree(str2);
	return buf;
}

// ---- ---- ---- ---- Operator ---- ---- ---- ---- 
coe* NumAbs(const dima* s)// r, i.e. hypot
{
	coe* re = 0;
	if (!s->y.coff)
		CoeHypot(re = CoeCpy(&s->Real), &s->Imag);
	else
	{
		int sign = 0;
		coe* tmp = 0;
		re = CoeNew("+0", "+0", "+1");
		CoeMul(tmp = CoeCpy(&s->x), &s->x);
		CoeAdd(re, tmp);
		CoeDel(tmp);
		CoeMul(tmp = CoeCpy(&s->y), &s->y);
		CoeAdd(re, tmp);
		CoeDel(tmp);
		CoeMul(tmp = CoeCpy(&s->z), &s->z);
		CoeAdd(re, tmp);
		CoeDel(tmp);
		CoeMul(tmp = CoeCpy(&s->t), &s->t);
		CoeSub(re, tmp);
		CoeDel(tmp);
		CoeSqrt(re);// signed
	}
	return re;
}
coe* NumAng(const dima* s)// theta, i.e. angle
{
	int statex = CoeCmp(&s->Real, &(coe){.coff = "+0", .expo = "+0", .divr = "+1" });
	int statey = CoeCmp(&s->Imag, &(coe){.coff = "+0", .expo = "+0", .divr = "+1" });
	if (!statey)
	{
		if (!statex) return CoeNew("+0", "+0", "+1");
		coe* cpi = s->Real.coff[0] == '+' ? CoeNew("+0", "+0", "+1") : CoePi();
		return cpi;
	}
	else if (!statex)
	{
		coe* cpi = CoePi();
		CoeDiv(cpi, &(coe){.coff = "+2", .expo = "+0", .divr = "+1" });
		if (statey < 0) cpi->coff[0] = '-';
		return cpi;
	}
	coe* ret = CoeCpy(&s->Imag);
	CoeDiv(ret, &s->Real);
	CoeAtan(ret);
	if (statex < 0)
	{
		coe* cpi = CoePi();
		if (statey < 0) cpi->coff[0] = '-';
		CoeAdd(ret, cpi);
		CoeDel(cpi);
	}
	return ret;
}

void NumAdd(dima* dest, const dima* sors)
{
	CoeAdd(&dest->Real, &sors->Real);
	CoeAdd(&dest->Imag, &sors->Imag);
	if (sors->y.coff)// else omit it
		if (dest->y.coff)
		{
			CoeAdd(&dest->y, &sors->y);
			CoeAdd(&dest->z, &sors->z);
		}
		else
		{
			dest->y.coff = StrHeap(sors->y.coff);
			dest->y.expo = StrHeap(sors->y.expo);
			dest->y.divr = StrHeap(sors->y.divr);
			dest->z.coff = StrHeap(sors->z.coff);
			dest->z.expo = StrHeap(sors->z.expo);
			dest->z.divr = StrHeap(sors->z.divr);
		}
}

void NumSub(dima* dest, const dima* sors)
{
	CoeSub(&dest->Real, &sors->Real);
	CoeSub(&dest->Imag, &sors->Imag);
	if (sors->y.coff)// else omit it
		if (dest->y.coff)
		{
			CoeSub(&dest->y, &sors->y);
			CoeSub(&dest->z, &sors->z);
		}
		else
		{
			dest->y.coff = StrHeap(sors->y.coff);
			dest->y.expo = StrHeap(sors->y.expo);
			dest->y.divr = StrHeap(sors->y.divr);
			dest->z.coff = StrHeap(sors->z.coff);
			dest->z.expo = StrHeap(sors->z.expo);
			dest->z.divr = StrHeap(sors->z.divr);
		}
}

void NumMul(dima* dest, const dima* sors)// aka NumDot
{
	if ((size_t)(dest->y.coff) ^ (size_t)(sors->y.coff)) return;
	if (!dest->y.coff)// (a+bi)*(c+di) = ac-bd + i(ad+bc)
	{
		coe* s1 = CoeCpy(&dest->Real); CoeMul(s1, &sors->Real);// ac
		coe* s2 = CoeCpy(&dest->Imag); CoeMul(s2, &sors->Imag);// bd
		coe* s3 = CoeCpy(&dest->Real); CoeMul(s3, &sors->Imag);// ad
		coe* s4 = CoeCpy(&dest->Imag); CoeMul(s4, &sors->Real);// bc
		s1 = CoeSub(s1, s2);// ac-bd
		s3 = CoeAdd(s3, s4);// ad+bc
		// *&dest->Real = *s1; 
		CoeRst(&dest->Real, StrHeap(s1->coff), StrHeap(s1->expo), StrHeap(s1->divr));
		// *&dest->Imag = *s3; 
		CoeRst(&dest->Imag, StrHeap(s3->coff), StrHeap(s3->expo), StrHeap(s3->divr));
		CoeDel(s1); CoeDel(s2); CoeDel(s3); CoeDel(s4);
	}
	else// x0*x1 + y0*y1 + z0*z1 - t0*t1
	{
		CoeMul(&dest->x, &sors->x);
		CoeMul(&dest->y, &sors->y);
		CoeMul(&dest->z, &sors->z);
		CoeMul(&dest->t, &sors->t);
		CoeAdd(&dest->x, &dest->y);
		CoeAdd(&dest->x, &dest->z);
		CoeSub(&dest->x, &dest->t);
		CoeRst(&dest->y, StrHeap("+0"), StrHeap("+0"), StrHeap("+1"));
		CoeRst(&dest->z, StrHeap("+0"), StrHeap("+0"), StrHeap("+1"));
		CoeRst(&dest->t, StrHeap("+0"), StrHeap("+0"), StrHeap("+1"));
	}
}

void NumCross(dima* dest, const dima* sors)// ¦Å d_i s_j e_k
{
	// dest_x dest_y dest_z
	// sors_x sors_y sors_z
	// e_x    e_y    e_z
	// X: (dest_y*sors_z - dest_z*sors_y)
	// Y: (dest_z*sors_x - dest_x*sors_z)
	// Z: (dest_x*sors_y - dest_y*sors_x)
	coe* resx = CoeCpy(&dest->y); CoeMul(resx, &sors->z);
	coe* tmp = CoeCpy(&dest->z); CoeMul(tmp, &sors->y);
	CoeSub(resx, tmp);
	CoeDel(tmp);
	coe* resy = CoeCpy(&dest->z); CoeMul(resy, &sors->x);
	tmp = CoeCpy(&dest->x); CoeMul(tmp, &sors->z);
	CoeSub(resy, tmp);
	CoeDel(tmp);
	coe* resz = CoeCpy(&dest->x); CoeMul(resz, &sors->y);
	tmp = CoeCpy(&dest->y); CoeMul(tmp, &sors->x);
	CoeSub(resz, tmp);
	CoeDel(tmp);
	CoeRst(&dest->x, resx->coff, resx->expo, resx->divr);
	CoeRst(&dest->y, resy->coff, resy->expo, resy->divr);
	CoeRst(&dest->z, resz->coff, resz->expo, resz->divr);
	memf(resx); memf(resy); memf(resz);
}

void NumDiv(dima* dest, const dima* sors)
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
	CoeRst(&dest->Real, StrHeap(t1->coff), StrHeap(t1->expo), StrHeap(t1->divr));
	CoeRst(&dest->Imag, StrHeap(t4->coff), StrHeap(t4->expo), StrHeap(t4->divr));
	CoeDel(t1); CoeDel(t2); CoeDel(t3); CoeDel(t4); CoeDel(t5); CoeDel(t6);
}

void NumPow(dima* dest, const dima* sors)
{
	// (a+bi)^(c+di)
	// exp( (c+di)*ln(a+bi) )
	// exp( (c+di)*(ln(abs(a+bi))+i*arg(a+bi)) )
	// exp( (c+di)*(ln(abs(a+bi)))+i*(c+di)*arg(a+bi)) )
	// exp( (c+di)*(ln(abs(a+bi)))+i*(c*arg(a+bi)+d*arg(a+bi)) )
	// exp( (c*ln(abs(a+bi))-d*arg(a+bi))+i*(c*arg(a+bi)+d*ln(abs(a+bi))) )
	NumLog(dest);
	NumMul(dest, sors);
	NumExp(dest);
}

void NumSqrt(dima* dest)
{
	NumPow(dest, &numhalf);
}

void NumExp(dima* dest)
{
	// Exp(a+bi)=Exp(a)*(cos b +i*sin b)
	CoeExp(&dest->Real);
	coe* cospart = CoeCpy(&dest->Imag); CoeCos(cospart);
	coe* sinpart = CoeCpy(&dest->Imag); CoeSin(sinpart);
	CoeRst(&dest->Imag, StrHeap(dest->Real.coff), StrHeap(dest->Real.expo), StrHeap(dest->Real.divr));
	CoeMul(&dest->Real, cospart);
	CoeMul(&dest->Imag, sinpart);
	CoeDel(sinpart);
	CoeDel(cospart);
	return;
}

void NumLog(dima* dest)
{
	// log(a+bi)=ln(abs(a+bi))+i*arg(a+bi)
	coe* abs = NumAbs(dest);
	coe* arg = NumAng(dest);
	CoeLog(abs);
	CoeRst(&dest->Real, StrHeap(abs->coff), StrHeap(abs->expo), StrHeap(abs->divr));
	CoeRst(&dest->Imag, StrHeap(arg->coff), StrHeap(arg->expo), StrHeap(arg->divr));
	CoeDel(abs);
	CoeDel(arg);
	return;
}

void NumSin(dima* dest)
{
	//=(exp(iz)-exp(-iz))/(2i)
	NumMul(dest, &i);
	dima* another = NumCpy(dest);
	NumMul(another, &numneg);
	NumExp(dest); NumExp(another);
	NumSub(dest, another);
	NumDiv(dest, &num2i);
	NumDel(another);
}

void NumCos(dima* dest)
{
	//=(exp(iz)+exp(-iz))/2
	NumMul(dest, &i);
	dima* another = NumCpy(dest);
	NumMul(another, &numneg);
	NumExp(dest); NumExp(another);
	NumAdd(dest, another);
	NumDiv(dest, &numtwo);
	NumDel(another);
}

void NumTan(dima* dest)
{
	dima* tmp = NumCpy(dest);
	NumCos(tmp);
	NumSin(dest);
	NumDiv(dest, tmp);
	NumDel(tmp);
}

void NumAsin(dima* dest)
{
	// ln(iz+sqrt(1-z*z))*(-i)
	// no check definition, TODO.
	dima* dd = NumCpy(dest);
	NumMul(dd, dest); NumMul(dd, &numneg); NumAdd(dd, &numone);// 1-z*z
	NumPow(dd, &numhalf);
	NumMul(dest, &i);
	NumAdd(dest, dd);
	NumLog(dest);
	NumMul(dest, &numnegi);
	NumDel(dd);
	return;
}

void NumAcos(dima* dest)
{
	// multivalued function ?
	// ln(z+sqrt(z*z-1))*(-i)
	// ln(z+i*sqrt(1-z^2))*(-i)
	// no check definition, TODO.
	dima* d_square = NumCpy(dest);
	dima* d_origin = NumCpy(dest);
	NumMul(d_square, dest);
	CoeRst(&dest->Real, StrHeap("+1"), StrHeap("+0"), StrHeap("+1"));
	CoeRst(&dest->Imag, StrHeap("+0"), StrHeap("+0"), StrHeap("+1"));
	NumSub(dest, d_square);
	NumSqrt(dest);
	NumMul(dest, &i);
	NumAdd(dest, d_origin);
	NumLog(dest);
	NumMul(dest, &numnegi);
	NumDel(d_square);
	NumDel(d_origin);
	return;
}

void NumAtan(dima* dest)
{
	// (-i)* (ln ((1+i*z)/(1-i*z)) )/2 ?
	// (-i)* ln div (i-z,i+z) /2
	//{TODO} checking definition.
	dima* ano = NumCpy(dest);
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

void NumSinh(dima* dest)
{
	//=(exp(z)-exp(-z))/2
	dima* another = NumCpy(dest);
	NumMul(another, &numneg);
	NumExp(dest); NumExp(another);
	NumSub(dest, another);
	NumDiv(dest, &numtwo);
	NumDel(another);
}

void NumCosh(dima* dest)
{
	//=(exp(z)+exp(-z))/2
	dima* another = NumCpy(dest);
	NumMul(another, &numneg);
	NumExp(dest); NumExp(another);
	NumAdd(dest, another);
	NumDiv(dest, &numtwo);
	NumDel(another);
}

void NumTanh(dima* dest)
{
	dima* cosh_t = NumCpy(dest); NumCosh(cosh_t);
	dima* sinh_t = dest; NumSinh(sinh_t);
	NumDiv(sinh_t, cosh_t);
	NumDel(cosh_t);
}

void NumAsinh(dima* dest)
{
	//  ln( z+ sqrt(z*z+1) )
	//{TODO} checking definition.
	dima* dd = NumCpy(dest);
	NumMul(dd, dest); NumAdd(dd, &numone);// z*z+1
	NumPow(dd, &numhalf);
	NumAdd(dest, dd);
	NumLog(dest);
	NumDel(dd);
	return;
}

void NumAcosh(dima* dest) 
{
	//  ln( z+ sqrt(z*z-1) )
	//{TODO} checking definition.

	dima* dd = NumCpy(dest);
	NumMul(dd, dest); NumSub(dd, &numone);// z*z-1
	NumPow(dd, &numhalf);
	NumAdd(dest, dd);
	NumLog(dest);
	NumDel(dd);
	return;
}

void NumAtanh(dima* dest)
{
	//  ln( div( 1+z, 1-z ) )/2
	//{TODO} checking definition.
	dima* dd = NumCpy(dest); NumMul(dd, &numneg);
	NumAdd(dest, &numone); NumAdd(dd, &numone);
	NumDiv(dest, dd);
	NumLog(dest);
	NumDiv(dest, &numtwo);
	NumDel(dd);
	return;
}
