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
// Previous name "NumAr.cpp", which has same .obj name with "NumAr.c", which is in the same linkage list.

#if 0

extern "C"
{
	#include <stddef.h>
	#include <stdlib.h>
	#include "../../c/ustring.h"
	
	#include "../../c/numar.h"
}
#include <iostream>
#include "../../inc/cpp/number"

static const coe numone = { (char*)"+1", (char*)"+1", (char*)"+0" };

Number::Number(int a) {
	char* str;
	scalar* sca = NumNew(str = instoa(a), "+0", "+1", "+0", "+0", "+1");
	content = *sca;
	memfree(str);
	memfree(sca);
}

Number::Number(ptrdiff_t a)
{
	char* str;
	scalar* sca = NumNew(str = instoa(a), "+0", "+1", "+0", "+0", "+1");
	content = *sca;
	memfree(str);
	memfree(sca);
}

Number::Number(double a)
{
	scalar* s = NumFromDComplex(
//#ifdef _MSVC
//	(cplx_d)
//#else
//	(double _Complex)
//#endif
	{
		a, 0.0
	});
	content = *s;
	memfree(s);
}

Number::Number(double a, double b)
{
	scalar* s = NumFromDComplex({ a, b });
	content = *s;
	memfree(s);
}

Number::Number(const Number& p)
{
	Number* n = (Number*) & p;
	const scalar* s = &p.content;
	content.Real.coff = StrHeap(s->Real.coff);
	content.Real.expo = StrHeap(s->Real.expo);
	content.Real.divr = StrHeap(s->Real.divr);
	content.Imag.coff = StrHeap(s->Imag.coff);
	content.Imag.expo = StrHeap(s->Imag.expo);
	content.Imag.divr = StrHeap(s->Imag.divr);
}

Number::~Number()
{
	memfree(content.Real.coff);
	memfree(content.Real.expo);
	memfree(content.Real.divr);
	memfree(content.Imag.coff);
	memfree(content.Imag.expo);
	memfree(content.Imag.divr);
}

std::string Number::RealToString()
{
	char* str = CoeToLocaleClassic((const coe*) & content.Real, 0);
	std::string s = str;// can be used in this stack or the caller's?
	memfree(str);
	return s;
}

std::string Number::ToString(int opt)
{
	char* str = NumToStringAuto(&content, opt);
	std::string s = str;
	memfree(str);
	return s;
}

Number& operator++ (Number& num)// prefix
{
	CoeAdd(&num.content.Real, &numone);
	return num;
}

Number Number::operator++ (int)// suffix, do not pass Reference&
{
	// Number* prev = new Number (*this);
	//-What is the principle? Haruno saw the return is called by twice before return then destructure.
	Number prev = *this;
	CoeAdd(&this->content.Real, &numone);
	return prev;
}

Number& operator-- (Number& num)// prefix
{
	// based on above
	CoeSub(&num.content.Real, &numone);
	return num;
}

Number Number::operator-- (int)// suffix, do not pass Reference&
{
	// based on above
	Number prev = *this;
	CoeSub(&this->content.Real, &numone);
	return prev;
}

Number Number::operator+ (Number& s1)
{
	scalar* res = NumCpy(&this->content);
	NumAdd(res, &s1.content);
	Number ret(res);
	memfree(res);
	return ret;
}

Number operator+ (const Number& s0, const double s1)
{
	scalar* res = NumCpy(&s0.content);
	scalar* res1;
	NumAdd(res, res1 = NumFromDComplex({ s1,0.0 }));
	Number ret(res);
	memfree(res);
	NumDel(res1);
	return ret;
}

Number operator+ (const double s1, const Number& s0)
{
	scalar* res = NumCpy(&s0.content);
	scalar* res1;
	NumAdd(res, res1 = NumFromDComplex({ s1,0.0 }));
	Number ret(res);
	memfree(res);
	NumDel(res1);
	return ret;
}

Number Number::operator- (Number& s1)
{
	scalar* res = NumCpy(&this->content);
	NumSub(res, &s1.content);
	Number ret(res);
	memfree(res);
	return ret;
}

Number operator- (const Number& s0, const double s1)
{
	scalar* res = NumCpy(&s0.content);
	scalar* res1;
	NumSub(res, res1 = NumFromDComplex({ s1,0.0 }));
	Number ret(res);
	memfree(res);
	NumDel(res1);
	return ret;
}

Number operator- (const double s1, const Number& s0)
{
	scalar* res = NumCpy(&s0.content);
	scalar* res1;
	NumSub(res, res1 = NumFromDComplex({ s1,0.0 }));
	Number ret(res);
	memfree(res);
	NumDel(res1);
	return ret;
}

Number Number::operator* (Number& s1)
{
	scalar* res = NumCpy(&this->content);
	NumMul(res, &s1.content);
	Number ret(res);
	memfree(res);
	return ret;
}

Number operator* (const Number& s0, const double s1)
{
	scalar* res = NumCpy(&s0.content);
	scalar* res1;
	NumMul(res, res1 = NumFromDComplex({ s1,0.0 }));
	Number ret(res);
	memfree(res);
	NumDel(res1);
	return ret;
}

Number operator* (const double s1, const Number& s0)
{
	scalar* res = NumCpy(&s0.content);
	scalar* res1;
	NumMul(res, res1 = NumFromDComplex({ s1,0.0 }));
	Number ret(res);
	memfree(res);
	NumDel(res1);
	return ret;
}

Number Number::operator/ (Number& s1)
{
	scalar* res = NumCpy(&this->content);
	NumDiv(res, &s1.content);
	Number ret(res);
	memfree(res);
	return ret;
}

Number operator/ (const Number& s0, const double s1)
{
	scalar* res = NumCpy(&s0.content);
	scalar* res1;
	NumDiv(res, res1 = NumFromDComplex({ s1,0.0 }));
	Number ret(res);
	memfree(res);
	NumDel(res1);
	return ret;
}

Number operator/ (const double s1, const Number& s0)
{
	scalar* res = NumCpy(&s0.content);
	scalar* res1;
	NumDiv(res, res1 = NumFromDComplex({ s1,0.0 }));
	Number ret(res);
	memfree(res);
	NumDel(res1);
	return ret;
}

#endif
