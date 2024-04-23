// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant Host[Allocation]
// LastCheck: RFZ23
// AllAuthor: @dosconio
// ModuTitle: Register-united Arithmetic
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

// I cannot make sense of some Phinae Code. --Haruno
// Cuter code need to be implemented by ASM. --Haruno

//{Potential Issue} Memory Leak (malc_count) - Have not tested.

#include "../../inc/c/regar.h"
#include "../../inc/c/stdinc.h"
#include "../../inc/c/ustring.h"
#include "../../inc/c/aldbg.h"
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <float.h>

extern size_t _size_decimal;
void _size_decimal_get();

extern char* arna_tempor;
extern char* arna_tmpslv;
extern char* arna_tmpext;

//---- ---- ---- ---- Single Style ---- ---- ---- ---- 

// Return the remainder of the sstr/divr
// Range of divr is half of that of the size_t.
// slen is the limit.
size_t RsgDiv8(size_t * sstr, size_t slen, unsigned char divr)
{
	// temp / divr = q ... r
	size_t crtq, temp;
	crtq = 0;
	for (unsigned char* s = (unsigned char*)(sstr + slen) - 1; s >= (unsigned char*)sstr; s--)
	{
		crtq <<= CHAR_BIT;
		temp = *s + crtq;
		*s = temp / divr,
			crtq = temp % divr;
	}
	return crtq;
}

//
static inline signed RsgZero(const size_t* sstr, size_t slen)
{
	do if (*sstr++) return 0; while (--slen);
	return 1;
}

// The return is based on size_t and at least 1.
static inline size_t RsgRealen(const size_t* sstr, size_t slen)
{
	for (size_t* str = (size_t*)(sstr + slen - 1); str > sstr; str--)
	{
		if (!*str) slen--;
		else break;
	}
	return slen;
}

// The return is based on byte and at least 1.
static inline byte RsgRealenByte(size_t* sstr, size_t slen)
{
	size_t count = slen * sizeof(*sstr);
	for (byte* str = (byte*)(sstr + slen) - 1; str > (byte*)sstr; str--)
	{
		if (!*str) count--;
		else break;
	}
	return count;
}

// The return is based on byte and at least 1. The relative parameters are byte
static inline byte RsgByteRealenByte(size_t* sstr, size_t slen)
{
	size_t count = slen;
	for (byte* str = (byte*)sstr + slen - 1; str > (byte*)sstr; str--)
	{
		if (!*str) count--;
		else break;
	}
	return count;
}

// `alen` or `blen` is based on size_t by default. Return -1 for failure.
signed RsgCmp(const size_t* a, const size_t* b, size_t alen, size_t blen)
{
	if (!alen || !blen) return -1;
	size_t efflena = RsgRealen(a, alen),
		efflenb = RsgRealen(b, blen);
	if (efflena == efflenb)
	{
		unsigned char* aa = (unsigned char*)a, * bb = (unsigned char*)b;
		for (ptrdiff_t i = sizeof(size_t) * efflena - 1; i >= 0; i--)
		{
			int state = (unsigned int)aa[i] - (unsigned int)bb[i];
			if (state) return state;
		}
		return 0;
	}
	else return efflena > efflenb;
}

// the relative parameters are byte
signed RsgCmpByte(const unsigned char* a, const unsigned char* b, size_t alen, size_t blen)
{
	int state;
	if (!alen || !blen) return -1;
	size_t efflena = RsgByteRealenByte((size_t *)a, alen),
		efflenb = RsgByteRealenByte((size_t *)b, blen);
	if (efflena == efflenb)
	{
		for (ptrdiff_t i = efflena - 1; i >= 0; i--)
		{
			if (state = (unsigned int)a[i] - (unsigned int)b[i])
				return state;
		}
		return 0;
	}
	else return efflena > efflenb;
}


// ---- ---- ---- ---- CPL cannot make use of CF directly. 

// Return whether the result is over the limit.
int RsgMul8(size_t* sstr, size_t slen, unsigned char mult)
{
	// 23_45*6
	size_t crtq, temp;
	crtq = 0;
	for (unsigned char* s = (unsigned char*)sstr; (size_t*)s < sstr + slen; s++)
	{
		temp = *s * mult + crtq;
		*s = temp & 0xFF;
		temp >>= CHAR_BIT;
		crtq = temp;
	}
	return !!temp;
}

// Return the carry of total.
int RsgAdd8(size_t* sstr, size_t slen, unsigned char plus)
{
	size_t carry, temp;
	carry = plus;
	for (unsigned char* s = (unsigned char*)sstr; (size_t*)s < sstr + slen; s++)
	{
		temp = *s + carry;
		*s = temp & 0xFF;
		temp >>= CHAR_BIT;
		carry = temp;
		if (!carry) break;
	}
	return carry;
}

// Return the sign of the result.
int RsgSub8(size_t* sstr, size_t slen, unsigned char subr)
{
	// 6543-50 -> [0065+9999=0064][0043-0050=9993] -> 6493 No Borrow
	size_t borrow, temp;
	borrow = subr;// 2's complement
	if (RsgCmp(sstr, &borrow, slen, 1) >= 0)
	{
		borrow = -subr;
		//{} NEG [borrow], `borrow = -borrow;` will rise a mistake in MSVC2022
		for (unsigned char* s = (unsigned char*)sstr; (size_t*)s < sstr + slen; s++)
		{
			temp = *s + borrow;
			*s = temp & 0xFF;
			temp >>= CHAR_BIT;
			borrow = temp;
			///if (!borrow) break;
		}
		///return borrow;
		return 0;
	}
	else
	{
		*sstr = borrow - *sstr;
		// (void)MemSet((void*)(sstr + 1), 0, sizeof(size_t) * (slen - 1));
		return 1;
	}
}

// Add zero as padding bit if dlen>slen
size_t* _Need_free RsgResize(const size_t* sstr, size_t slen, size_t dlen)
{
	if (!slen || !dlen) return 0;
	if (dlen <= slen) return MemHeap(sstr, sizeof(size_t) * dlen);
	else
	{
		size_t* ret;
		memalloc(ret, sizeof(size_t) * dlen);
		MemCopyN(ret, sstr, sizeof(size_t) * slen);
		MemSet((void*)(ret + slen), 0, sizeof(size_t) * (dlen - slen));
		return ret;
	}
}

// syst: 10 for decimal; 16 for hexi. `0xaa`; -16 for Hexi. `0xAA`;
char* _Need_free RsgToString(const size_t* sstr, size_t slen, signed syst)
{
	if (!slen) return StrHeap("0");
	if (!_size_decimal) _size_decimal_get();
	size_t malc_len = 1 + slen * _size_decimal;
	size_t* str = MemHeap(sstr, slen * sizeof(size_t));
	char* ret;
	memalloc(ret, malc_len);
	char* p = ret + malc_len - 1;
	*p-- = 0;
	switch (syst)
	{
	case 10:
		do
		{
			*p-- = RsgDiv8(str, slen, 10) + 0x30;
		} while (!RsgZero(str, slen));
		break;
	case 16:
		do
		{
			unsigned char tmpuc = RsgDiv8(str, slen, 16);
			*p-- = tmpuc > 9 ? tmpuc + 'a' - 10 : tmpuc + '0';
		} while (!RsgZero(str, slen));
		break;
	case -16:
		do
		{
			unsigned char tmpuc = RsgDiv8(str, slen, 16);
			*p-- = tmpuc > 9 ? tmpuc + 'A' - 10 : tmpuc + '0';
		} while (!RsgZero(str, slen));
		break;
	default:
		break;
	}
	p++;
	MemAbsolute(ret, p, StrLength(p) + 1);
	memfree(str);
	return ret;
}

// Return the carry
int RsgAdd(size_t* dstr, const size_t* sstr, size_t comlen)
{
	unsigned char* d = (unsigned char*)dstr;
	unsigned char const* s = (const unsigned char*)sstr;
	size_t carry, temp;
	carry = 0;
	for (size_t i = 0; i < comlen * sizeof(size_t); i++)
	{
		temp = d[i] + s[i] + carry;
		d[i] = temp & 0xFF;
		temp >>= CHAR_BIT;
		carry = temp;
	}
	return carry;///!!carry
}

// Return the sign
int RsgSub(size_t* dstr, const size_t* sstr, size_t comlen)
{
	int sign = 0;
	ptrdiff_t borrow, temp;
	unsigned char* d = (unsigned char*)dstr;
	unsigned char const* s = (const unsigned char*)sstr;
	borrow = 0;
	// assure d > s in abs.
	if (RsgCmp(dstr, sstr, comlen, comlen) < 0)
	{
		sign = 1;
		d = MemHeap(sstr, sizeof(size_t) * comlen);
		s = (void*)dstr;
	} 
	else
	{
		d = (void*)dstr; s = (void*)sstr;
	}
	for (size_t i = 0; i < comlen * sizeof(size_t); i++)
	{
		temp = (ptrdiff_t)d[i] - (ptrdiff_t)s[i] + borrow;
		d[i] = temp & 0xFF;
		temp >>= CHAR_BIT;// SAR
		borrow = temp;
	}
	if (sign)
	{
		MemCopyN(dstr, d, sizeof(size_t) * comlen);
		memfree(d);
	}
	return sign;
}

// 
void RsgSubComple(size_t* dstr, const size_t* sstr, size_t comlen)
{
	int sign = 0;
	ptrdiff_t borrow, temp;
	unsigned char* d = (unsigned char*)dstr;
	unsigned char const* s = (const unsigned char*)sstr;
	borrow = 0;
	// assure d > s in abs.
	d = (void*)dstr; s = (void*)sstr;
	for (size_t i = 0; i < comlen * sizeof(size_t) || borrow; i++)
	{
		if (i < comlen * sizeof(size_t))
			temp = (ptrdiff_t)d[i] - (ptrdiff_t)s[i] + borrow;
		else 
			temp = (ptrdiff_t)d[i] + borrow;
		d[i] = temp & 0xFF;
		temp >>= CHAR_BIT;// SAR
		borrow = temp;
	}
}

// Return whether over the limit. The abandoned method is use dstr and sstr as high and low part of the result. May only Phinae will match the method.
int _Heap RsgMul(size_t* dstr, const size_t* sstr, size_t comlen)
{
	typedef unsigned char ArUnit;// RFB15: Do not use signed, for its 1-padding.
	ArUnit* res = (void*)RsgImm(0, comlen * 2);
	size_t carry, temp;
	// Phinae Version can update by the method.
	ArUnit* d = (void*)dstr, * s = (void*)sstr;
	for (size_t i = 0; i < sizeof(size_t) * comlen; i++)
	{
		carry = 0;
		for (size_t j = 0; j < sizeof(size_t) * comlen; j++)
		{
			temp = (size_t)d[i] * (size_t)s[j] + carry + (size_t)res[i + j];
			res[i + j] = temp & ~(ArUnit)0;
			carry = temp >> (sizeof(ArUnit) * CHAR_BIT);
		}
		res[i + sizeof(size_t) * comlen] = carry;// comlen is spec. of s
	}
	MemCopyN(dstr, res, sizeof(size_t) * comlen);
	int Over = !RsgZero((size_t*)(res + sizeof(size_t) / sizeof(ArUnit) * comlen), comlen);
	memfree(res);
	return Over;
}

// Return the quotient as dstr, rem as sstr, state as return, where -1 is 0-divr exception or others; 0 for success.
int _Heap_tmpher RsgDiv(size_t* dstr, size_t* sstr, size_t comlen)
{
	// differ the method of ChrDiv of ustring mold.
	// I cannot make sense of Phinae algorithm.
	// The function just meet the function (maybe), no considering the faster way.
	// I try to use size_t, word as the ari-unit, but all fail, the detail\
		is in my locale git repo., that's for my low programming skill.
	typedef unsigned char ArUnit;// Now limit by Byte(8)
	if (RsgZero(sstr, comlen))
		return -1;
	if (RsgZero(dstr, comlen))
	{
		MemSet(sstr, 0, sizeof(*sstr) * comlen);
		return 0;
	}
	if (RsgCmp(dstr, sstr, comlen, comlen) < 0)
	{
		// 2 / 3 = 0 ... 2. This case may be faster after independent.
		MemCopyN(sstr, dstr, sizeof(*sstr) * comlen);
		MemSet(dstr, 0, sizeof(*sstr) * comlen);
		return 0;
	}
	// arna_tempor as quo.
	// arna_tmpext as temp
	size_t bytlend = RsgRealenByte(dstr, comlen);
	size_t bytlens = RsgRealenByte(sstr, comlen);
	if (malc_limit < bytlend) return -1;
	MemSet(arna_tempor, 0, bytlend);
	MemSet(arna_tmpslv, 0, bytlens);// temp
	byte* crtd = (byte*)dstr + bytlend - bytlens;
	byte* crtresptr = (byte*)arna_tempor + bytlend - bytlens;
	for (size_t i = 0; i <= bytlend - bytlens; i++)
	{
		size_t times = 0;
		MemSet(arna_tmpext, 0, bytlend);
		times += crtd[bytlens - 1] / (((byte*)sstr)[bytlens - 1] + 1);
		MemCopyN(arna_tmpext, sstr, bytlens);
		RsgMul8((void*)arna_tmpext, bytlend, times);// bytlens should be good
		RsgSubComple((void*)crtd, (void*)arna_tmpext, bytlend);
		(*crtresptr) += times;
		///while (MemCompareRight(crtd, sstr, bytlens) >= 0)
		while (RsgCmpByte((void*)crtd, (void*)sstr, (i <= bytlend - bytlens && i>0) ?
			bytlens + 1 : bytlens, bytlens) >= 0)
		{
			(*crtresptr)++;
			RsgSubComple((void*)crtd, (void*)sstr, bytlens);
			MemSet(arna_tmpext, 0, bytlend);
			times = crtd[bytlens - 1] / (((byte*)sstr)[bytlens - 1] + 1);
			MemCopyN(arna_tmpext, sstr, bytlens);
			RsgMul8((void*)arna_tmpext, bytlend, times);// bytlens should be good
			RsgSubComple((void*)crtd, (void*)arna_tmpext, bytlend);
			(*crtresptr) += times;
		}
		crtresptr--;
		crtd--;
	}
	// BUF Method only temporarily.
	MemSet(sstr, 0, sizeof(*sstr) * comlen);
	MemCopyN(sstr, dstr, bytlend);
	MemSet(dstr, 0, sizeof(*sstr) * comlen);
	MemCopyN(dstr, arna_tempor, bytlend);
	return 0;
}

//
size_t* _Need_free RsgNew(const size_t* sors, size_t len)
{
	if (!len)return 0;
	return MemHeap(sors, sizeof(size_t) * len);
}

//
size_t* _Need_free RsgImm(size_t sors, size_t len)
{
	size_t* r;
	if (!len)return 0;
	memalloc(r, sizeof(size_t) * len);
	*r = sors;
	if (len > 1) for (size_t i = 1; i < len; i++)
		r[i] = 0;
	return r;
}

// Return whether over the limit. 
int _Heap_tmpher RsgPow(size_t* base, const size_t* expo, size_t comlen)
{
	size_t* tmp;
	int state;
	if (!comlen) return -1;
	if (RsgZero(base, comlen))// pow(0, any) = 0 (Haruno no Cove)
	{
		MemSet(base, 0, sizeof(*base) * comlen);
		return 0;
	}
	else if (RsgZero(expo, comlen))// pow(else any,0) = 1
	{
		MemSet(base, 0, sizeof(*base) * comlen);
		*base = 1;
		return 0;
	}
	if (malc_limit < sizeof(size_t) * comlen)
		return 1;
	size_t* exp = (void*)arna_tmpslv;
	size_t* res = (void*)arna_tempor;
	MemCopyN(exp, expo, sizeof(*base) * comlen);
	MemCopyN(res, base, sizeof(*base) * comlen);
	RsgSub8(exp, comlen, 1);
	while (!RsgZero(exp, comlen))
	{
		state = RsgMul(res, base, comlen);
		if (state) return state;
		RsgSub8(exp, comlen, 1);
	}
	MemCopyN(base, res, sizeof(*base) * comlen);
	return 0;
}

// Return whether over the limit. 
int _Heap RsgArrange(size_t* total, const size_t* items, size_t comlen)
{
	size_t* i = MemHeap(items, comlen * sizeof * items);
	size_t* res = RsgImm(1, comlen);
	int state;
	while (!RsgZero(i, comlen))
	{
		state = RsgMul(res, total, comlen);
		if (state) return state;
		RsgSub8(i, comlen, 1);
		RsgSub8(total, comlen, 1);
	}
	MemCopyN(total, res, comlen * sizeof(*res));
	memfree(res);
	memfree(i);
	return 0;
}

// Return whether over the limit. 
int _Heap RsgFactorial(size_t* base, size_t comlen)
{
	if (RsgZero(base, comlen))
	{
		*base = 1;
	}
	size_t* res = RsgImm(1, comlen);
	while (!RsgZero(base, comlen))
	{
		int state;
		state = RsgMul(res, base, comlen);
		if (state) return state;
		RsgSub8(base, comlen, 1);
	}
	MemCopyN(base, res, comlen * sizeof(*res));
	return 0;
}

// Return whether over the limit. 
int _Heap RsgCombinate(size_t* total, const size_t* items, size_t comlen)
{
	int state;
	state = RsgArrange(total, items, comlen);
	if (state) return state;
	size_t* divr = RsgNew(items, comlen);
	state = RsgFactorial(divr, comlen);
	if (state) return state;
	RsgDiv(total, divr, comlen);
	memfree(divr);
	return 0;
}

// Return the errno. 
int _Heap RsgComDiv(size_t* d, const size_t* s, size_t comlen)
{
	int state;
	int xchgf = 0;
	///if (RsgZero(d, comlen) || RsgZero(s, comlen)) return 1;
	size_t* a = d, * b = RsgNew(s, comlen);
	size_t* const ToFree = b;
	state = RsgCmp(a, b, comlen, comlen);
	if (state < 0)
	{
		xchgptr(a, b);// assure a>b
		xchgf = 1;
	}
	if (RsgZero(b, comlen))
	{
		if(xchgf) MemCopyN(d, b, comlen * sizeof(*b));
		memfree(ToFree);
		return 0;
	}
	else if (state == 0)
	{
		memfree(ToFree);
		return 0;
	}
	else
	{
		size_t* newb = RsgNew(b, comlen);
		state = RsgDiv(a, newb, comlen);
		if (state) return state;
		state = RsgComDiv(b, newb, comlen);
		if (state) return state;
		if (!xchgf)
			MemCopyN(d, b, comlen * sizeof(*b));
		memfree(newb); memfree(ToFree);	return 0;
	}
}

// Return the errno. 
int _Heap RsgComMul(size_t* d, const size_t* s, size_t comlen)
{
	int state = 0;
	size_t* newd = RsgNew(d, comlen);
	state = RsgMul(d, s, comlen); if (state) goto endo;
	state = RsgComDiv(newd, s, comlen); if (state) goto endo;
	state = RsgDiv(d, newd, comlen); if (state) goto endo;
endo:
	memfree(newd);
	return state;
}

//---- ---- ---- ---- Phinae Style ---- ---- ---- ---- 
//  Prefix Red means Reg-coff-expo-divr, i.e. Reg-PhinaeCoe
// +-*/, pow, log, (a)[sin-family](h), taylor (7+3*2*2=19)
/* Special result (ARN Ver.)
coff	expo	divr	description
0000	xxxx	0000	Not a/one number (0 or multiple)
not0	var_n	0000	infinite (=lim x->inf` pow(x,var_n))
0000	0000	0001	Standard 0
0000	0000	xxxx	0
0000	erro	xxxx	{	0000: over memory limit
							0001: precise loss
}
*/

static size_t _DIG_CUT = 4;

const static Rfnar_t constr_1 = {
	.coff = (size_t[]){1},
	.divr = (size_t[]){1},
	.expo = (size_t[]){0},
	.defalen = 1
};

const static Rfnar_t constr_2pi = {
	.coff = (size_t[]){(size_t)(3.141592653589793238 * 2 * 256 * 256 * 256 * 256 * 256 * 256 * 256)},
	.divr = (size_t[]){1},
	.expo = (size_t[]){7}, .expsign = 1,
	.defalen = 1
};

// Heap all the pointed.
Rfnar_t* _Need_free RedNew(size_t* coff, size_t* expo, size_t* divr, size_t len)
{
	if (!coff || !expo || !divr || !len) return 0;
	Rfnar_t* r = zalc(sizeof * r);
	r->coff = RsgNew(coff, len);
	r->expo = RsgNew(expo, len);
	r->divr = RsgNew(divr, len);
	r->defalen = len;
	return r;
}

Rfnar_t* _Need_free RedNewImm(size_t coff, size_t expo, size_t divr, size_t len)
{
	if (!len) return 0;
	Rfnar_t* r = zalc(sizeof * r);
	r->coff = RsgImm(coff, len);
	r->expo = RsgImm(expo, len);
	r->divr = RsgImm(divr, len);
	r->defalen = len;
	return r;
}

void RedDel(Rfnar_t* elm)
{
	memfree(elm->coff);
	memfree(elm->expo);
	memfree(elm->divr);
	memfree(elm);
}

Rfnar_t* RedHeap(const Rfnar_t* obj)
{
	Rfnar_t* r = MemHeap(obj, sizeof *obj);
	r->coff = MemHeap(obj->coff, sizeof(*obj->coff) * obj->defalen);
	r->expo = MemHeap(obj->expo, sizeof(*obj->expo) * obj->defalen);
	r->divr = MemHeap(obj->divr, sizeof(*obj->divr) * obj->defalen);
	return r;
}

// Enlarge or cut the range.
void RedDig(Rfnar_t* dest, size_t prec)
{
	if (!prec || prec == dest->defalen) return;
	if (prec < dest->defalen)// cut
		dest->defalen = prec;
	else// pad 0
	{
		size_t* temp;
		temp = RsgResize(dest->coff, dest->defalen, prec);
		srs(dest->coff, temp);
		temp = RsgResize(dest->expo, dest->defalen, prec);
		srs(dest->expo, temp);
		temp = RsgResize(dest->divr, dest->defalen, prec);
		srs(dest->divr, temp);
		dest->defalen = prec;
	}
}

// Make the len least and keep the current value expressed.
void RedZip(Rfnar_t* dest)
{
	size_t LenMax = 1;
	size_t LenTmp;
	LenTmp = RsgRealen(dest->coff, dest->defalen);
	if (LenTmp > LenMax) LenMax = LenTmp;
	LenTmp = RsgRealen(dest->expo, dest->defalen);
	if (LenTmp > LenMax) LenMax = LenTmp;
	LenTmp = RsgRealen(dest->divr, dest->defalen);
	if (LenTmp > LenMax) LenMax = LenTmp;
	dest->defalen = LenMax;
} 

static inline void RedExpoAdd(Rfnar_t* dest, size_t* expo)
{
	// [L] same length
	if (dest->expsign)// -
	{
		// -3+5=2      -3+2=-1
		// (-)3-5=2(-) (-)3-2=1(+)
		int reverse = RsgSub(dest->expo, expo, dest->defalen);
		if (reverse) dest->expsign = 0;
	}
	else// +
		RsgAdd(dest->expo, expo, dest->defalen);
}

static inline void RedExpoAdd8(Rfnar_t* dest, unsigned char expo)
{
	if (dest->expsign)// -
	{
		int reverse = RsgSub8(dest->expo, dest->defalen, expo);
		if (reverse) dest->expsign = 0;
	}
	else// +
		RsgAdd8(dest->expo, dest->defalen, expo);
}

static inline void RedExpoSub(Rfnar_t* dest, size_t* expo)
{
	// [L] same length
	if (dest->expsign)// -
		RsgAdd(dest->expo, expo, dest->defalen);
	else// +
	{
		// 3-5=-2      3-2=1
		// (+)3-5=2(-) (+)3-2=1(+)
		int reverse = RsgSub(dest->expo, expo, dest->defalen);
		if (reverse) dest->expsign = 1;
	}
}

static inline void RedExpoSub8(Rfnar_t* dest, unsigned char expo)
{
	if (dest->expsign)// -
		RsgAdd8(dest->expo, dest->defalen, expo);
	else// +
	{
		int reverse = RsgSub8(dest->expo, dest->defalen, expo);
		if (reverse) dest->expsign = 1;
	}
}

void RedAlignExpo(Rfnar_t* d, Rfnar_t* s)
{
	if (d->defalen != s->defalen)
	{
		if (d->defalen > s->defalen)
			RedDig(s, d->defalen);
		else RedDig(d, s->defalen);
	}
	size_t dlen = d->defalen;
	int state;
	if ((state = RsgCmp(d->expo, s->expo, dlen, dlen)) == 0 &&
		(d->expsign == s->expsign || RsgZero(d->expo, dlen)))
	{
		return;
	}
	// assure a>b of expo
	if (d->expsign == 1 && s->expsign == 0 ||
		d->expsign == 0 && s->expsign == 0 && state < 0 ||
		d->expsign == 1 && s->expsign == 1 && state > 0)
	{
		xchgptr(d, s);
	}
	// degrade a(d) for b(s)
	size_t* shift_times = RsgNew(d->expo, dlen);
	if (d->expsign == 1 || s->expsign == 0)// {s.exps==1 -2>(-3)} OR {2>(1)} : SUB
		RsgSub(shift_times, s->expo, dlen);
	else// 2>(-1) : ADD
		RsgAdd(shift_times, s->expo, dlen);//{TODO} OVER LIMIT
	srs(d->expo, RsgNew(s->expo, dlen));
	d->expsign = s->expsign;
	//{TD} if (RsgRealen(shift_times, dlen) > 1)// memory may be not enough to use (QAQ)
	if (*shift_times & 1)StrShiftLeft4(d->coff, sizeof(size_t) * dlen);
	*shift_times >>= 1;
	StrShiftLeft8n((void*)d->coff, sizeof(size_t) * dlen, *shift_times);
	memfree(shift_times);
}

// Without check
void RedExpoZero(Rfnar_t* d)// RFV2 ArnMgk [divr should be 1]
{
	const size_t tempo_lim = 0x1000;
	size_t dlen = d->defalen;
	if (RsgZero(d->expo, dlen)) return;
	if (RsgRealen(d->coff, d->defalen) > 1 || d->expo[0] > tempo_lim) return;//{TOD} err
	if (d->expsign == 1)
		StrShiftRight8n((void*)d->coff, sizeof(size_t) * dlen, d->expo[0] * 2);//{} !
	else
	{
		if (RsgRealenByte(d->coff, dlen) + d->expo[0] * 2 > d->defalen * sizeof(size_t))
		{
			Rfnar_t* tmp = RedNewImm(0, 0, 1, (RsgRealenByte(d->coff, dlen) + d->expo[0] * 2 + sizeof(size_t) - 1) / sizeof(size_t));
			MemCopyN(tmp->coff, d->coff, RsgRealenByte(d->coff, dlen));
		}
		StrShiftLeft8n((void*)d->coff, sizeof(size_t) * dlen, d->expo[0] * 2);
	}
	d->expsign = 0;
	for (size_t i = 0;i < d->defalen;i++) d->expo[i] = 0;
}


static inline void RedCoffAdd(Rfnar_t* dest, size_t* coff)
{
	// [L] same length
	if (dest->expsign)// -
	{
		int reverse = RsgSub(dest->coff, coff, dest->defalen);
		if (reverse) dest->expsign = 0;
	}
	else// +
		RsgAdd(dest->coff, coff, dest->defalen);
}

static inline void RedCoffAdd8(Rfnar_t* dest, unsigned char coff)
{
	if (dest->expsign)// -
	{
		int reverse = RsgSub8(dest->coff, dest->defalen, coff);
		if (reverse) dest->expsign = 0;
	}
	else// +
		RsgAdd8(dest->coff, dest->defalen, coff);
}

static inline void RedCoffSub(Rfnar_t* dest, size_t* coff)
{
	// [L] same length
	if (dest->expsign)// -
		RsgAdd(dest->coff, coff, dest->defalen);
	else// +
	{
		int reverse = RsgSub(dest->coff, coff, dest->defalen);
		if (reverse) dest->expsign = 1;
	}
}

static inline void RedCoffSub8(Rfnar_t* dest, unsigned char coff)
{
	if (dest->expsign)// -
		RsgAdd8(dest->coff, dest->defalen, coff);
	else// +
	{
		int reverse = RsgSub8(dest->coff, dest->defalen, coff);
		if (reverse) dest->expsign = 1;
	}
}

void RedDivrUnit(Rfnar_t* d)
{
	RedReduct(d);
	size_t dlen = d->defalen;
	if (!RsgCmp(d->divr, constr_1.coff, dlen, 1)) return;
	size_t shl_bytes = dlen * sizeof(size_t) - RsgRealenByte(d->coff, dlen);
	if (shl_bytes > 0xFF) return;//{} Use this to limit to fast the arithmetic.
	StrShiftLeft8n(d->coff, dlen * sizeof(size_t), shl_bytes);
	RedExpoSub8(d, (shl_bytes << 1) & 0xFF);
	RsgDiv(d->coff, d->divr, dlen);
	if (!RsgZero(d->divr, dlen))
		d->preloss = 1;
	srs(d->divr, RsgImm(1, d->defalen));
	RedReduct(d);
}

// Do not change expo unless only coff or expo is times of 0x10.
void RedReduct(Rfnar_t* d)
{
	size_t dlen = d->defalen;
	if (RsgZero(d->divr, dlen))
	{
		//{}
		return;
	}
	if (RsgZero(d->coff, dlen))
	{
		MemSet(d->coff, 0, sizeof(size_t) * dlen);
		*d->coff = 1;
		return;
	}
	
	size_t* GCD = RsgNew(d->coff, dlen);
	RsgComDiv(GCD, d->divr, dlen);
	size_t* GCDCpy = RsgNew(GCD, dlen);
	if (RsgCmp(GCD, constr_1.coff, dlen, 1) > 0)
	{
		RsgDiv(d->coff, GCD, dlen);
		RsgDiv(d->divr, GCDCpy, dlen);
	}
	memfree(GCD);
	memfree(GCDCpy);
	while (*d->coff & 0xF == 0)
	{
		StrShiftRight4(d->coff, sizeof(size_t) * dlen);
		RedExpoAdd8(d, 1);
	}
	while (!(*d->divr & 0xF))
	{
		StrShiftRight4(d->divr, sizeof(size_t) * dlen);
		RedExpoSub8(d, 1);
	}
	return;
}

// A[Same-long]
void RedAlign(Rfnar_t* d, Rfnar_t* s)
{
	if (d->defalen != s->defalen)
	{
		if (d->defalen > s->defalen)
			RedDig(s, d->defalen);
		else RedDig(d, s->defalen);
	}
	RedAlignExpo(d, s);
	size_t dlen = d->defalen;
	// Align divr, here does not change expo
	size_t* GCM = RsgNew(d->divr, dlen);
	RsgComMul(GCM, s->divr, dlen);
	size_t* GCMCpy = RsgNew(GCM, dlen);
	size_t* ddif = RsgNew(d->divr, dlen);
	size_t* sdif = RsgNew(s->divr, dlen);
	srs(d->divr, RsgNew(GCM, dlen));
	srs(s->divr, RsgNew(GCM, dlen));
	RsgDiv(GCM, ddif, dlen);
	RsgMul(d->coff, GCM, dlen);
	RsgDiv(GCMCpy, sdif, dlen);
	RsgMul(s->coff, GCMCpy, dlen);
	memfree(GCM);
	memfree(GCMCpy);
	memfree(ddif);
	memfree(sdif);
}

// below: 0 for success
int RedAdd(Rfnar_t* d, const Rfnar_t* s)
{
	int state = 0;
	if (RsgZero(s->coff, s->defalen)) goto endo;
	if (RsgZero(d->coff, d->defalen))
	{
		srs(d->coff, RsgNew(s->coff, s->defalen));
		srs(d->expo, RsgNew(s->expo, s->defalen));
		srs(d->divr, RsgNew(s->divr, s->defalen));
		goto endo;
	}
	Rfnar_t* ss = RedHeap(s);
	RedAlign(d, ss);
	if (d->cofsign && s->cofsign || !d->cofsign && !s->cofsign)
		RsgAdd(d->coff, ss->coff, d->defalen);
	else
	{
		// case d[1-]s[+]r[0+] 1-   -2+1=-1
		// case d[1-]s[+]r[1-] 0+   -2+3=+1
		// case d[0+]s[-]r[0+] 0+   +2-1=+1
		// case d[0+]s[-]r[1-] 1-   +2-3=-1
		d->cofsign ^= RsgSub(d->coff, ss->coff, d->defalen);
	}
	RedDel(ss);
endo:
	if (RsgZero(d->coff, d->defalen))
	{
		srs(d->expo, RsgImm(0, d->defalen));
		srs(d->divr, RsgImm(1, d->defalen));
		d->cofsign = d->expsign = 0;
	}
	RedReduct(d);
	return state;
}

int RedSub(Rfnar_t* d, const Rfnar_t* s)
{
	int state = 0;
	if (RsgZero(s->coff, s->defalen)) goto endo;
	if (RsgZero(d->coff, d->defalen))
	{
		srs(d->coff, RsgNew(s->coff, s->defalen));
		srs(d->expo, RsgNew(s->expo, s->defalen));
		srs(d->divr, RsgNew(s->divr, s->defalen));
		d->cofsign = !s->cofsign;
		goto endo;
	}
	Rfnar_t* ss = RedHeap(s);
	RedAlign(d, ss);
	if (d->cofsign && s->cofsign || !d->cofsign && !s->cofsign)
	{
		// case d[0+]s[+]r[0+] 0+   +2-1=+1
		// case d[0+]s[+]r[1-] 1-   +2-3=-1
		// case d[1-]s[-]r[0+] 1-   -2+1=-1
		// case d[1-]s[-]r[1-] 0+   -2+3=+1
		d->cofsign ^= RsgSub(d->coff, ss->coff, d->defalen);
	}		
	else RsgAdd(d->coff, ss->coff, d->defalen);// need not change symbol
	RedDel(ss);
endo:
	if (RsgZero(d->coff, d->defalen))
	{
		srs(d->expo, RsgImm(0, d->defalen));
		srs(d->divr, RsgImm(1, d->defalen));
		d->cofsign = d->expsign = 0;
	}
	RedReduct(d);
	return state;
}

// Return -1 for d<s; 0 for d=s; 1 for d>s.
int RedCmp(const Rfnar_t* d, const Rfnar_t* s)
{
	int r;
	Rfnar_t* dd = RedHeap(d);
	Rfnar_t* ss = RedHeap(s);
	RedAlign(dd, ss);
	if (dd->cofsign && ss->cofsign || !dd->cofsign && !ss->cofsign)
		dd->cofsign ^= RsgSub(dd->coff, ss->coff, dd->defalen);
	else RsgAdd(dd->coff, ss->coff, dd->defalen);
	if (RsgZero(dd->coff, dd->defalen))
		r = 0;
	else r = dd->cofsign ? -1 : 1;
	RedDel(dd), RedDel(ss);
	return r;
}

// Return -1 for over limit, -2 for exception.
int RedMul(Rfnar_t* dest, const Rfnar_t* sors)
{
	size_t dlen = dest->defalen;
	int new_sors = 0;
	if (RsgZero(dest->coff, dest->defalen) || RsgZero(sors->coff, sors->defalen))
	{
		srs(dest->coff, RsgImm(0, dlen));
		srs(dest->expo, RsgImm(0, dlen));
		srs(dest->divr, RsgImm(1, dlen));
		dest->preloss = 0;
		dest->expsign = dest->cofsign = 0;
		return 0;
	}
	if (dest->defalen != sors->defalen)
	{
		if (dest->defalen > sors->defalen)
		{
			sors = RedHeap(sors);
			RedDig((void*)sors, dest->defalen);
			new_sors = 1;
		}
		else RedDig(dest, sors->defalen);
		dlen = dest->defalen;
	}
	if (RsgMul(dest->coff, sors->coff, dlen)) return -1;
	if (RsgMul(dest->divr, sors->divr, dlen)) return -1;
	dest->cofsign ^= sors->cofsign;
	RedExpoAdd(dest, sors->expo);
	if (new_sors) RedDel((void*)sors);
	RedReduct(dest);
	return 0;
}

// Return -2 for exception.
int RedDiv(Rfnar_t* dest, const Rfnar_t* sors)
{
	size_t dlen = dest->defalen;
	int new_sors = 0;
	if (RsgZero(sors->coff, sors->defalen))
		return -2;
	if (RsgZero(dest->coff, dest->defalen))
	{
		srs(dest->coff, RsgImm(0, dlen));
		srs(dest->expo, RsgImm(0, dlen));
		srs(dest->divr, RsgImm(1, dlen));
		dest->preloss = 0;
		dest->expsign = dest->cofsign = 0;
		return 0;
	}
	if (dest->defalen != sors->defalen)
	{
		if (dest->defalen > sors->defalen)
		{
			sors = RedHeap(sors);
			RedDig((void*)sors, dest->defalen);
			new_sors = 1;
		}
		else RedDig(dest, sors->defalen);
		dlen = dest->defalen;
	}
	if (RsgMul(dest->coff, sors->divr, dlen)) return -1;
	if (RsgMul(dest->divr, sors->coff, dlen)) return -1;
	dest->cofsign ^= sors->cofsign;
	RedExpoSub(dest, sors->expo);
	if (new_sors) RedDel((void*)sors);
	RedReduct(dest);
	return 0;
}

inline static int RedTaylor(Rfnar_t* dest, unsigned char dptor, const Rfnar_t* period, size_t digcut) {}
//{TODO}
//{
//
//
//	char* tmp;
//	if (digcut > malc_count) digcut = 0;
//	coe* crt = CoeCpy(dest);
//	CoeDivrUnit(crt, digcut);// then we can omit .divr
//	if (period)
//	{
//		while (CoeCmp(crt, &coezero) < 0)
//			CoeAdd(crt, period);
//		while (CoeCmp(crt, period) >= 0)
//			CoeSub(crt, period);
//	}
//	coe* CrtPow = CoeNew(tmp = instoa((0b00001100 & dptor) >> 2), "+0", "+1");
//	memfree(tmp);
//	boolean sign = (crt->coff[0] == '-');
//	boolean signcrt = ((0b00000010 & dptor) >> 1);// ^ sign
//	boolean signshock = (0b00000001 & dptor);
//	enum { all_ = 0, only_odd, only_even, none_ } powtype = (0b00110000 & dptor) >> 4;
//	if (powtype == none_) goto endo;
//	enum { one_, n_, nf_, nn_ } divtype = (0b11000000 & dptor) >> 6;
//	if (divtype == nn_) goto endo;// do not support NOW.
//	coe* result;
//	coe* plus;
//	coe* diver = divtype == one_ ? CoeCpy(&coeone) :
//		divtype == n_ ? CoeCpy(CrtPow) :
//		divtype == nf_ ? CoeFac(CoeCpy(CrtPow)) : 0;
//
//	// Endo Condition: "5+9<5", no rounding increase, when expodif==8;
//	//  or expodif > 8
//	//  (assume show_precise=8)
//	//    123456789 <res>  123456789 +0
//	//    000000005 <plus> 5 -8
//	// first loop
//	{
//		result = CoeCpy(&coezero);
//	}
//	// += sgn* powx/divcof
//	// more loop
//	boolean conti = 1;
//	size_t i = 0;
//	lup_last = 0;
//	do
//	{
//		lup_last++;
//		plus = CoeCpy(crt);
//		CoePow(plus, CrtPow);
//		CoeDiv(plus, diver);
//		// conti = () . NOW do not care, because we need consider the change rate of a and b of a/b
//		if (plus->coff[1] != '0')
//		{
//			char* ori = StrHeapN(result->coff, show_precise + 1);
//			// coe* test_precise = CoeCpy(result);
//			// CoeDiv(test_precise, plus);
//			if (signcrt) plus->coff[0] = (plus->coff[0] == '-' ? '+' : '-');
//			CoeAdd(result, plus);
//			CoeDivrUnit(result, show_precise + 1);
//			conti = ((++i < lup_times) || StrCompareN(result->coff, ori, show_precise + 1)) && (i < lup_limit);
//			// [Old condi] ChrCmp(test_precise->expo, "+0") <= 0 && ...
//			// [Old condi] (ptrdiff_t)StrLength(result->coff) - atoins(result->expo) <= show_precise) ...
//			// CoeDel(test_precise);
//			if (signshock) signcrt = !signcrt;
//			CoeAdd(CrtPow, powtype == all_ ? &coeone : &coetwo);
//			CoeDel(diver);
//			diver = divtype == one_ ? CoeCpy(&coeone) :
//				divtype == n_ ? CoeCpy(CrtPow) :
//				divtype == nf_ ? CoeFac(CoeCpy(CrtPow)) : 0;
//			memfree(ori);
//		}
//		CoeDel(plus);
//	} while (conti);
//	if (StrLength(result->coff) > show_precise + 1) CoeDig(result, show_precise, 2);// nearest
//	CoeDel(diver);
//endo:
//	CoeDel(CrtPow);
//	CoeDel(crt);
//	srs(dest->coff, StrHeap(result->coff));
//	srs(dest->expo, StrHeap(result->expo));
//	srs(dest->divr, StrHeap(result->divr));
//	CoeDel(result);
//	return dest;
//}

Rfnar_t* RedPow(Rfnar_t* dest, const Rfnar_t* sors);
//{TODO}
//{
//
//
//	if (*sors->coff == '+' && !StrCompare(sors->divr, "+1"))
//	{
//		if (sors->coff[1] == '0')
//		{
//			srs(dest->coff, StrHeap("+1"));
//			srs(dest->expo, StrHeap("+0"));
//			srs(dest->divr, StrHeap("+1"));
//			return dest;
//		}
//		coe* CrtExp = CoeCpy(sors);
//		coe* ret = CoeCpy(dest);// attention! the char* will be free, so use this but "coe ret;" in stack
//		CoeSub(CrtExp, &coeone);
//		while (CoeCmp(CrtExp, &coezero) > 0)
//		{
//			CoeMul(ret, dest);
//			CoeSub(CrtExp, &coeone);
//			// void CoePrint(coe * co); CoePrint(ret); CoePrint(dest);
//			// srs(CrtExp, ChrSub(CrtExp, "+1"));
//		}
//		CoeDel(CrtExp);
//		srs(dest->coff, StrHeap(ret->coff));
//		srs(dest->expo, StrHeap(ret->expo));
//		srs(dest->divr, StrHeap(ret->divr));
//		CoeDel(ret);
//	}
//	else
//	{
//		// Pow(a,b) == Exp( mul( log(a) , b) )
//		int t_sign = (*dest->coff == '-');
//		ptrdiff_t t_expo = atoins(dest->expo);
//		if (!(t_expo & 1)) t_sign = 0;
//		dest = CoeLog(dest);
//		CoeMul(dest, sors);
//		CoeExp(dest);
//		*dest->coff = t_sign ? '-' : '+';
//	}
//	return dest;
//}

int RedHypot(Rfnar_t* dest, const Rfnar_t* sors);



// Log`10(x) == Log`(default e)(x)/Log`e(10)
Rfnar_t* RedLog(Rfnar_t* dest);

int RedSgn(const Rfnar_t* dest);

Rfnar_t* RedExp(Rfnar_t* dest);//TEST-STAGE

Rfnar_t* RedPi();

int RedSin(Rfnar_t* dest)
{
	RedTaylor(dest, TAYLOR_SIN, &constr_2pi, _DIG_CUT);
}

int RedCos(Rfnar_t* dest);

Rfnar_t* RedTan(Rfnar_t* dest);

Rfnar_t* RedAsin(Rfnar_t* dest);

Rfnar_t* RedAcos(Rfnar_t* dest);

Rfnar_t* RedAtan(Rfnar_t* dest);

Rfnar_t* RedSinh(Rfnar_t* dest);

Rfnar_t* RedCosh(Rfnar_t* dest);

Rfnar_t* RedTanh(Rfnar_t* dest);

Rfnar_t* RedAsinh(Rfnar_t* dest);

Rfnar_t* RedAcosh(Rfnar_t* dest);

Rfnar_t* RedAtanh(Rfnar_t* dest);

//

// Below 4 were by Arina RFB29
double _Heap RedToDouble(const Rfnar_t* dest)
{
	// based on CoeAr
	Rfnar_t* d = RedHeap(dest);
	RedDivrUnit(d);
	double ll = 0.0;
	if (RsgRealenByte(d->expo, d->defalen) > sizeof(size_t) || (*(ptrdiff_t*)d->expo) < 0 || ((ptrdiff_t)d->defalen) < 0)// Check the exponent is too big
		return INFINITY;
	ptrdiff_t CrtPow = *d->expo;
	if (d->expsign)CrtPow = -CrtPow;
	if (CrtPow > (DBL_MAX_EXP >> 4))
		return INFINITY;// base2 to base16
	if ((CrtPow + 2 * RsgRealenByte(d->expo, d->defalen)) < (DBL_MIN_EXP >> 4))
		return 0.0;
	// Precise cut: only keep the highest size_t of the coff, which differs from the ToString
	// [12345678][23456789]16^[7FFF FFFF]
	const unsigned char* const p = (void*)d->coff;
	size_t plen = RsgRealenByte(d->coff, d->defalen);
	for (ptrdiff_t i = 0; i < plen; i++)
	{
		if (CrtPow >= DBL_MIN_EXP)
			ll += (double)(p[i]) * exp2((double)CrtPow * 4);// base2 to base16, CrtPow << 2, I made a mistake "SHL 4" -- ArinaMgk RFB31
		CrtPow += 2;// not coff *= 256 --> expo += 8
		if (CrtPow > DBL_MAX_EXP) break;
	}
	if (d->cofsign)ll *= -1;
	RedDel(d);
	return ll;
}

#include <stdio.h>
//
Rfnar_t* _Need_free RedFromDouble(double flt, size_t rfnumlen, size_t FetchDigits)
{
	// based on CoeAr
	if (isnan(flt)) return RedNewImm(1, 0, 0, rfnumlen);
	Rfnar_t* res = RedNewImm(0, 0, 1, rfnumlen);
	if (flt == 0.0) return res;
	if (res->cofsign = (flt < 0)) flt = -flt;
	ptrdiff_t crtpow = ((ptrdiff_t)log2(flt)) & (~(ptrdiff_t)0b0111);
	size_t luptimes = 0;
	char* const buf = zalc((DBL_MAX_EXP >> 3) + 1);//{TOD} [BUF]
	char* ptr = buf;
	unsigned char* coffptr = (void*)res->coff;
	unsigned char c;
	double delta = 0.0;
	if (rfnumlen * sizeof(size_t) < FetchDigits) FetchDigits = rfnumlen * sizeof(size_t);
	while (crtpow > DBL_MIN_EXP && luptimes++ < FetchDigits)
	{
		delta = exp2(crtpow);
		c = flt / delta;
		flt -= c * delta;
		*ptr++ = c;
		crtpow -= 8;// 256 = 2^8
	}
	for (size_t i = 0; ptr > buf; i++)
	{
		*coffptr++ = *--ptr;
	}
	crtpow += 8;
	if (crtpow < 0)
	{
		res->expsign = 1;
		crtpow = -crtpow;
	}
	res->expo[0] = (crtpow >> (3 - 1));// 16-based
	memfree(buf);
	return res;
}

// {TODO}
// opt: 0[auto 3-opt format when bytof(size_t)>2 or abs(expo)>1] 1[int or float] 2[e format(+2.0e-2)], 3[e format((e-2)+2.0)], in decimal
char* _Need_free RedToLocaleClassic(const Rfnar_t* obj, int opt)
{
	// 9876_5432H === 0x32,0x54,0x76,0x98;
	// 9876.5432H === 0x32,0x54`0x76,0x98;
	Rfnar_t* d = RedHeap(obj);
	RedDivrUnit(d);
	if (RsgRealen(d->expo, d->defalen) > 1)
	{
		char* ret = StrHeap(d->expsign ?
			(d->cofsign ? "-0" : "+0") : (d->cofsign ? "-INF" : "+INF"));
		RedDel(d);
		return ret;
	}
	int chosen_3 = (RsgRealenByte(d->coff, d->defalen) > 2) || (*d->expo > 1);
	switch (opt)
	{
	case 0:
		// {TODO} ---- suspended ----
		break;
	case 1:
		// {TODO}
		break;
	case 2:
		// {TODO}
		break;
	case 3:
		// {TODO}
		break;
	default:
		RedDel(d);
		return 0;
		break;// Cheat some compilers
	}
}

// {TODO}
Rfnar_t* _Need_free RedFromLocaleClassic(const char* str)
{
	// based on CoeAr
	const char* ParA = str, * ParB = 0, * ParC = 0;
	size_t LenA = 0, LenB = 0, LenC = 0;
	char c;
	Rfnar_t* co = 0;
//	size_t coflen = 0;
//loop: switch (c = *str++)
//	{
//	case 0:
//		// Chk and gen co.
//		memalloc(co, sizeof(coe));
//		co->symb = 0;
//		co->divr = StrHeap("+1");
//		if (ParC) LenC = str - ParC - 1;// e2$$
//		else if (ParB) LenB = str - ParB - 1;
//		else LenA = str - ParA;
//
//		if (ParC && LenC && ParC[LenC - 1] != '+' && ParC[LenC - 1] != '-')
//		{
//			co->expo = StrHeapN(ParC, LenC);
//			if (*co->expo != '+' && *co->expo != '-')
//				srs(co->expo, StrHeapAppend(("+"), co->expo));
//		}
//		else co->expo = StrHeap("+0");
//		if (!LenA && !LenB)
//		{
//			co->coff = StrHeap("+1");
//			goto loopend;
//		}
//
//		if (!ParB || ParA == ParB || !LenB)// only interger
//		{
//			co->coff = StrHeapN(ParA, LenA);
//			if (*co->coff != '+' && *co->coff != '-')
//				srs(co->coff, StrHeapAppend(("+"), co->coff));
//			if (!co->coff[1])
//				srs(co->coff, StrHeapAppend(co->coff, "1"));
//			goto loopend;
//		}
//		else
//		{
//			memalloc(co->coff, LenA + LenB + 1);
//			if (LenA)
//				StrCopyN(co->coff, ParA, LenA);// 0.x case
//			StrCopyN(co->coff + LenA, ParB, LenB);
//			co->coff[LenA + LenB] = 0;
//			char* tmpptr = instoa(LenB);
//			srs(co->expo, ChrSub(co->expo, tmpptr));
//			memfree(tmpptr);
//			if (*co->coff != '+' && *co->coff != '-')
//				srs(co->coff, StrHeapAppend(("+"), co->coff));
//			if (!co->coff[1])
//				srs(co->coff, StrHeapAppend(co->coff, "1"));
//			goto loopend;
//		}
//		break;
//	default:
//		if (c >= '0' && c <= '9');
//		else goto loopend;
//		break;
//	case '+':
//	case '-':
//		break;
//	case 'e':
//		if (ParC) goto loopend;
//		ParC = str;
//		if (ParA && ParB)//1.2e3
//		{
//			LenB = str - ParB - 1;
//			ParC = str;
//		}
//		else if (ParA && !ParB)//1e2
//		{
//			LenA = str - ParA - 1;
//			ParB = ParA;
//			ParC = str;
//		}
//		else goto loopend;
//		break;
//	case '.':
//		if (ParB || ParC) goto loopend;
//		ParB = str;
//		LenA = str - ParA - 1;//1.1
//		break;
//	} goto loop; 
//	loopend:
//	if (!co) return co;
//	ChrCpz(co->coff);// +002$ 
//	ChrCpz(co->expo);
	return co;
}

//{MSG to dosc.} [After 4 conversion functions, checking all existing fucntions for MALC and give me the mistakes. -- Arin.]
//{TOD} [The version just use coff]
//{TOD} [Buf is optional if the heap is available]
//{TOD} [use Byte-unit and 256-base to make other version: BytAr(duplicate name with the history version: SGA) a independent file but yo ustring, I feel it will wider, though may lower speed. --ArnMgk]

//---- ---- ---- ---- Multid Style ---- ---- ---- ---- 
// cross, dot, +, -, abs (5)
//{TODO} 

Hrnar_t* HrnImm(size_t xcoff, size_t xexpo, size_t xdivr,
	size_t ycoff, size_t yexpo, size_t ydivr,
	size_t zcoff, size_t zexpo, size_t zdivr,
	size_t tcoff, size_t texpo, size_t tdivr,
	size_t comlen)
{
	Hrnar_t* r = zalc(sizeof(Hrnar_t));
	Rfnar_t* tmp;
	tmp = RedNewImm(xcoff, xexpo, xdivr, comlen);
	r->x = *tmp; memfree(tmp);
	tmp = RedNewImm(ycoff, yexpo, ydivr, comlen);
	r->y = *tmp; memfree(tmp);
	tmp = RedNewImm(zcoff, zexpo, zdivr, comlen);
	r->z = *tmp; memfree(tmp);
	tmp = RedNewImm(tcoff, texpo, tdivr, comlen);
	r->t = *tmp; memfree(tmp);
	return r;
}

Hrnar_t* HrnCpy(const Hrnar_t* obj)
{
	Hrnar_t* r = MemHeap(obj, sizeof * obj);
	r->x.coff = MemHeap(obj->x.coff, sizeof(*obj->x.coff) * obj->x.defalen);
	r->x.expo = MemHeap(obj->x.expo, sizeof(*obj->x.expo) * obj->x.defalen);
	r->x.divr = MemHeap(obj->x.divr, sizeof(*obj->x.divr) * obj->x.defalen);
	r->y.coff = MemHeap(obj->y.coff, sizeof(*obj->y.coff) * obj->y.defalen);
	r->y.expo = MemHeap(obj->y.expo, sizeof(*obj->y.expo) * obj->y.defalen);
	r->y.divr = MemHeap(obj->y.divr, sizeof(*obj->y.divr) * obj->y.defalen);
	r->z.coff = MemHeap(obj->z.coff, sizeof(*obj->z.coff) * obj->z.defalen);
	r->z.expo = MemHeap(obj->z.expo, sizeof(*obj->z.expo) * obj->z.defalen);
	r->z.divr = MemHeap(obj->z.divr, sizeof(*obj->z.divr) * obj->z.defalen);
	r->t.coff = MemHeap(obj->t.coff, sizeof(*obj->t.coff) * obj->t.defalen);
	r->t.expo = MemHeap(obj->t.expo, sizeof(*obj->t.expo) * obj->t.defalen);
	r->t.divr = MemHeap(obj->t.divr, sizeof(*obj->t.divr) * obj->t.defalen);
	return r;
}

void HrnClr(Hrnar_t* d)
{
	memfree(d->x.coff);
	memfree(d->x.expo);
	memfree(d->x.divr);
	memfree(d->y.coff);
	memfree(d->y.expo);
	memfree(d->y.divr);
	memfree(d->z.coff);
	memfree(d->z.expo);
	memfree(d->z.divr);
	memfree(d->t.coff);
	memfree(d->t.expo);
	memfree(d->t.divr);
	memfree(d);
}

void HrnAdd(Hrnar_t* d, const Hrnar_t* s)
{
	RedAdd(&d->x, &s->x);
	RedAdd(&d->y, &s->y);
	RedAdd(&d->z, &s->z);
	RedAdd(&d->t, &s->t);
}

void HrnSub(Hrnar_t* d, const Hrnar_t* s)
{
	RedSub(&d->x, &s->x);
	RedSub(&d->y, &s->y);
	RedSub(&d->z, &s->z);
	RedSub(&d->t, &s->t);
}

Hrnar_t* HrnCross(const Hrnar_t* d, const Hrnar_t* s)
{
	// |ex ey ez|
	// |dx dy dz|
	// |sx sy sz|
	// Cross = {dysz-dzsy} {dzsx-dxsz} {dxsy-dysx} yo {r-e}
	Hrnar_t* e = zalc(sizeof(Hrnar_t));
	Hrnar_t* r = zalc(sizeof(Hrnar_t));
	Rfnar_t* tmp;
	tmp = RedHeap(&d->y); r->x = *tmp; memfree(tmp);
	tmp = RedHeap(&d->z); e->x = *tmp; memfree(tmp);
	tmp = RedHeap(&d->z); r->y = *tmp; memfree(tmp);
	tmp = RedHeap(&d->x); e->y = *tmp; memfree(tmp);
	tmp = RedHeap(&d->x); r->z = *tmp; memfree(tmp);
	tmp = RedHeap(&d->y); e->z = *tmp; memfree(tmp);
	RedMul(&r->x, &s->z);
	RedMul(&e->x, &s->y);
	RedMul(&r->y, &s->x);
	RedMul(&e->y, &s->z);
	RedMul(&r->z, &s->y);
	RedMul(&e->z, &s->x);
	HrnSub(r, e);
	HrnClr(e);
	return r;
}

Rfnar_t* HrnDot(const Hrnar_t* d, const Hrnar_t* s)
{
	Rfnar_t* r;
	Hrnar_t* dd = HrnCpy(d);
	RedMul(&dd->x, &s->x);
	RedMul(&dd->y, &s->y);
	RedMul(&dd->z, &s->z);
	RedMul(&dd->t, &s->t); 
	///if (!RsgZero(dd->t.coff, d->x.defalen))	dd->t.expsign ^= 1;// not
	r = RedHeap(&dd->x);
	RedAdd(r, &dd->y);
	RedAdd(r, &dd->z);
	RedSub(r, &dd->t);
	HrnClr(dd);
	return r;
}

Rfnar_t* _Need_free HrnAbs(const Hrnar_t* d)
;

// EOF
