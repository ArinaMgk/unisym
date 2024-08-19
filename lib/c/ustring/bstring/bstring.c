// ASCII TAB4 C99 ArnAssume
// String output specially in buffer
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



#define _ARN_INSIDE_LIBRARY_INCLUDE
#define _LIB_STRING_BUFFER
#include <stddef.h>
#include <stdio.h>
#include <ctype.h>// na isXType()
#include "../../inc/c/stdinc.h"
#include "../../inc/c/ustring.h"

#define erro(x) ((void)(x))// CHEAT

#if defined(_LIB_STRING_BUFFER) && !defined(_LIB_STRING_BUFFER_GUARD)// bstring.c, need pre-set buffer
#define _LIB_STRING_BUFFER_GUARD
	extern char arna_tempor[];// as result
	extern char arna_tmpslv[];// Do not use as result, same size with tempor.
	extern char arna_tmpext[];

#endif
//
// Buf-special part
//
#if (defined _LIB_STRING_BUFFER) && !(defined _LIB_STRING_HEAP) && defined(_LIB_STRING_BUFFER_0HEAP_GUARD)// {TODO}GUARD
	#define _LIB_STRING_BUFFER_0HEAP_GUARD
	#define ChrAdd ChrAddBuf
	#define ChrSub ChrSubBuf
	#define ChrMul ChrMulBuf
	#define ChrDiv ChrDivBuf
	#define stradd ChrAdd
	#define strsub ChrSub
	#define strmul ChrMul
	#define instoa instob
	// {TODO}...
	//ChrHexToDecBuf
#endif// End of Buf-special part

static char* ptr_tempmas = arna_tempor;
static char* ptr_tempslv = arna_tmpslv;
static char* ptr_tempext = arna_tmpext;

// You should provide some pre-set buffers for this, because bstring doed not use heap area.
// If use the bstring and do not hstring, a macro for ChrAddBuf to be ChrAdd will be provided by ustring.
/*
// Besides MAS: None Using
char* ChrAddBuf(const char* a, const char* b)
{
	// may xchg to make length a >= b
	if (!a || !b || !*a || !*b)
	{
		malc_occupy = 0;
		aflag.PrecLoss = 1;
		aflag.Failure = 1;
		aflag.state = 0;// null exception
		return 0;
	}
	union { unsigned carry : 1, SignB : 1, SignA : 1; } flag = { 0 };
	if (*a == '+') { a++; }
	if (*a == '-') { a++;flag.SignA = 1; }
	if (*b == '+') { b++; }
	if (*b == '-') { b++;flag.SignB = 1; }

	if (flag.SignA && !flag.SignB)// -a +b
	{
		size_t templen = StrLength(a) + 1;
		if(templen > malc_limit)
		{
			malc_occupy = 0;
			aflag.PrecLoss = 1;
			aflag.Failure = 1;
			aflag.state = 1;
			return 0;
		}
		char* temp = aflag.HeapYo ? StrCopy(malc(templen), a) : StrCopy(ptr_tempslv, a);
		*temp = '+';
		if (arna_eflag.HeapYo)
		{
			srs(temp, ChrSub(b, temp));
		}
		else
			ChrSub(b, temp);// --> mas_tmp
		return temp;
	}
	else if (flag.SignB && !flag.SignA)// +a -b 
	{
		size_t templen = StrLength(b) + 1;
		if (templen > malc_limit)
		{
			malc_occupy = 0;
			arna_eflag.PrecLoss = 1;
			return 0;
		}
		char* temp = arna_eflag.HeapYo ? StrCopy(malc(templen), b) : StrCopy(ptr_tempslv, b);
		*temp = '+';
		if (arna_eflag.HeapYo)
		{
			srs(temp, ChrSub(a, temp));
		}
		else
			ChrSub(a, temp);// --> mas_tmp
		return temp;
	}

	if (arna_eflag.Signed) { a++; b++; }
	const char* endofa = a, * endofb = b;// last digit
	for (; '0' <= *endofa && *endofa <= '9'; endofa++); endofa--;
	for (; '0' <= *endofb && *endofb <= '9'; endofb++); endofb--;
	if (endofa - a < endofb - b) // xchg
	{
		xchgptr(a, b);
		xchgptr(endofa, endofb);
	}
	size_t siz;// Magic 4: Potential Symbol, \0, LengthDif:1, CarryDigit
	if ((siz = max(endofa - a, endofb - b) + 4) > malc_limit)// OMIT SIGN <TODO>
	{
		malc_occupy = 0;
		arna_eflag.PrecLoss = 1;
		return 0;
	}

	char* q;
	if (arna_eflag.HeapYo)
		memalloc(q, siz);
	else q = ptr_tempmas;
	q[siz - (arna_eflag.Signed ? 1 : 2)] = 0;
	if(arna_eflag.Signed) *q = (CF & 1) ? '-' : '+';
	for (size_t i = 0; i < siz - 3; i++)
	{
		unsigned char regichar = *(endofa - i) - 0x30;
		if (i <= (size_t)(endofb - b)) regichar += *(endofb - i) - 0x30;
		regichar += ((CF & 0b100) ? 1 : 0);
		CF &= ~(unsigned char)0b100;
		if ((CF |= (regichar > 9) << 2) & 0b100) regichar -= 10;
		*(char*)(q + siz - 2 - i - (arna_eflag.Signed ? 0 : 1)) = regichar + 0x30;
	}
	q[arna_eflag.Signed ? 1 : 0] = ((CF & 0b100) >> 2) + 0x30;
	ChrCpz(q);
	arna_eflag.PrecLoss = 0;
	malc_occupy = siz;
	return q;
}

// Besides MAS: None using
char* ChrSub(const char* a, const char* b)
{
	// length a >= b
	if (!a || !b || !*a || !*b)// same with ChrAdd, tolerate the input sign: RFT17.
	{
		malc_occupy = 0;
		arna_eflag.PrecLoss = 1;
		return 0;
	}
	char BF = 0;// {borrow_flag`sign_b`sign_a}
	if (arna_eflag.Signed)
	{
		BF |= (*a == '-');
		BF |= (*b == '-') << 1;
	}
	if (arna_eflag.Signed)
	if ((BF & 0b01) ^ ((BF & 0b10) >> 1))
	{
		size_t templen = StrLength(b) + 1;
		if (templen > malc_limit)
		{
			malc_occupy = 0;
			arna_eflag.PrecLoss = 1;
			return 0;
		}
		char* temp = arna_eflag.HeapYo ? StrCopy(malc(templen), b) : StrCopy(ptr_tempslv, b);
		*temp = (BF & 0b01) ? '-' : '+';
		if (arna_eflag.HeapYo)
		{
			srs(temp, ChrAdd(a, temp));
		}
		else
			ChrAdd(a, temp);// --> mas_tmp
		return temp;
	}
	if (arna_eflag.Signed) { a++; b++; }
	const char* endofa = a, * endofb = b;// last digit
	for (; '0' <= *endofa && *endofa <= '9'; endofa++); endofa--;
	for (; '0' <= *endofb && *endofb <= '9'; endofb++); endofb--;

	BF &= 0b011;// bit2 : if the result is negative
	BF |= (BF & 1) << 2;// now let us assume a > b so ...
	if (endofa - a < endofb - b || \
		(endofa - a == endofb - b) && (StrCompare(a, b) < 0))
	{
		xchgptr(a, b);
		xchgptr(endofa, endofb);
		if (BF & 0b100) BF &= 0b011; else BF |= 0b100;
	}// make abs(a)>abs(b)
	size_t siz;// Magic Number 3: Symbol, \0, LengthDif:1
	if ((siz = max(endofa - a, endofb - b) + 3) > malc_limit)// OMIT SIGN <TODO>
	{
		malc_occupy = 0;
		arna_eflag.PrecLoss = 1;
		return 0;
	}
	char* q;
	if (arna_eflag.HeapYo)
		memalloc(q, siz);
	else q = ptr_tempmas;
	q[siz - (arna_eflag.Signed ? 1 : 2)] = 0;
	if(arna_eflag.Signed) *q = (BF & 0b100) ? '-' : '+';
	BF ^= BF;// now assume BF for borrow
	for (size_t i = 0; i < siz - 2; i++)
	{
		signed char regichar = *(endofa - i) - 0x30;
		if (i <= (size_t)(endofb - b)) regichar -= *(endofb - i) - 0x30;
		if (BF = ((regichar -= (BF ? 1 : 0)) < 0)) regichar += 10;
		*(char*)(q + (size_t)endofa - a + 1 - i - (arna_eflag.Signed ? 0 : 1)) = regichar + 0x30;
	}
	ChrCpz(q);
	arna_eflag.PrecLoss = 0;
	malc_occupy = siz;
	return q;
}

// Besides MAS: None using
char* ChrMul(const char* a, const char* b)
{
	int sga = 0, sgb = 0;//bool false is +
	char* q = 0, * p = 0;
	int greater = 1;//bool
	int carry_digit = 0;
	size_t size;
	if (*a == '+') { a++; }
	if (*b == '+') { b++; }
	if (*a == '-') { a++; sga = 1; }
	if (*b == '-') { b++, sgb = 1; }
	// Tolerating some case, care you and others.
	const char* endofa = a, * endofb = b;// last digits
	for (; '0' <= *endofa && *endofa <= '9'; endofa++); endofa--;
	for (; '0' <= *endofb && *endofb <= '9'; endofb++); endofb--;
	if (endofa - a == endofb - b) greater = (StrCompare(a, b)) > 0;
	else greater = endofa - a > endofb - b;
	if (!greater)
	{
		xchgptr(a, b);
		xchgptr(endofa, endofb);
	}
	if ((size = ((size_t)endofa - (size_t)a + 1 + (size_t)endofb - (size_t)b + 1 + 2)) > malc_limit)//TODO. now the cancelled ERRO mechanism is actually useful.
	{
		malc_occupy = 0;
		arna_eflag.PrecLoss = 1;
		return 0;// no memalloc above~~~
	}
	if (arna_eflag.HeapYo)
		memalloc(q, size);
	else q = ptr_tempmas;
	q[size - 1] = 0;
	for (int k = 0; k < size - 1; k++) q[k] = '0';
	if(arna_eflag.Signed) *q = (sga ^ sgb) ? '-' : '+';
	for (int i = 0; i < endofb - b + 1; i++)
	{
		p = (char*)((size_t)q + size - 2 - i);//escape '\0'
		carry_digit = 0;
		for (int j = 0; j < endofa - a + 1; j++)
		{
			int tmp;
			((tmp = (*(endofa - j) - '0') * (*(endofb - i) - '0') + carry_digit) < 100);
			if (tmp > 9)
				tmp -= 10 * (carry_digit = tmp / 10);
			else carry_digit = 0;
			DigInc(tmp + '0', p--);
		}
		if (carry_digit) DigInc(carry_digit + '0', p--);
	}
	ChrCpz(q);
	arna_eflag.PrecLoss = 0;
	malc_occupy = size;
	return q;
}

// Besides MAS: SLV using, NO for occupy
void ChrDiv(char* a, char* b)
{
	_Bool sga = 0, sgb = 0;//false is +
	char* buf_sub = 0;
	int tmp;
	// Tolerate for sign
	if (*b == '-') { sgb = 1; }
	if (*a == '-') { sga = 1; }
	if (arna_eflag.Signed) *b++ = sga ? '-' : '+';
	if(arna_eflag.Signed) *a++ = (sga ^ sgb) ? '-' : '+';
	char* endofa = a, * endofb = b;// last digits
	for (; '0' <= *endofa && *endofa <= '9'; endofa++); endofa--;
	for (; '0' <= *endofb && *endofb <= '9'; endofb++); endofb--;
	if ((a < b) && (endofa + 1 >= b - 1) || (a == b) || (a > b) && (endofb + 1 >= a - 1))
	{
		malc_occupy = 0;
		arna_eflag.PrecLoss = 1;
		return;// INPUT CROSS
	}
	if (*b == '0')
	{
		malc_occupy = 0;
		arna_eflag.PrecLoss = 1;
		return;// EXCEPTION
	}
	if (*a == '0') 
	{ 
		*b++ = '0'; *b = 0;
		*++a = 0;
		arna_eflag.PrecLoss = 0;
		return; 
	}
	if ((endofb - b > endofa - a) || (endofb - b == endofa - a && (StrCompare(a, b) < 0)))
	{
		for (register int _c = 0; _c < endofa - a + 1; _c++) b[_c] = a[_c];
		b[endofa - a + 1] = 0;
		*a++ = '0'; *a = 0;
		arna_eflag.PrecLoss = 0;
		return;
	}
	if (endofa - a + 2 > malc_limit)
	{
		malc_occupy = 0;
		arna_eflag.PrecLoss = 1;
		return;// LACK MEMORY
	}
	if (arna_eflag.HeapYo)
		memalloc(buf_sub, endofa - a + 2);
	else buf_sub = ptr_tempslv;
	buf_sub[endofa - a + 1] = 0;
	char* const buf_sub_fix = buf_sub;
	size_t buf_rem = endofa - a + 1;
	const size_t div_size = endofb - b + 1;
	for (register int _c = 0; _c < endofa - a + 1; _c++)
	{
		buf_sub[_c] = a[_c];
		a[_c] = '0';
	}
	while (buf_rem > div_size)
	{
		tmp = 0;
		if (StrCompareN(buf_sub, b, div_size) >= 0)
		{
			do
			{
				tmp++;
				for (register unsigned int _c = 0; _c < div_size; _c++)
					DigDec(b[div_size - 1 - _c],
						(char*)((size_t)buf_sub + div_size - 1 - _c));
			} while (StrCompareN(buf_sub, b, div_size) >= 0);
			*(endofa - buf_rem + div_size) = '0' + tmp;
		}
		else
		{
			do
			{
				tmp++;
				for (register unsigned int _c = 0; _c < div_size; _c++)
					DigDec(b[div_size - 1 - _c],
						(char*)((size_t)buf_sub + div_size - _c));
			} while (StrCompareN(buf_sub, b, div_size) < 0 && *buf_sub != '0' ||
				(StrCompareN(buf_sub + 1, b, div_size) >= 0 && *buf_sub == '0'));
			*(endofa - buf_rem + div_size + 1) = '0' + tmp;
		}
		for (; *buf_sub == '0' && buf_sub[1] != 0; buf_rem--, buf_sub++);
	}
	if (buf_rem == div_size)
	{
		if ((tmp = StrCompareN(buf_sub, b, buf_rem)) < 0)
		{
			for (register unsigned int _c = 0; _c < buf_rem; _c++) b[_c] = buf_sub[_c]; b[buf_rem] = 0;
		}
		else if (tmp == 0)
		{
			b[0] = '0'; b[1] = 0; DigInc('1', endofa);
		}
		else
		{
			tmp = 0;
			do
			{
				tmp++;
				for (register size_t _c = 0; _c < buf_rem; _c++)
					DigDec(b[div_size - 1 - _c], (char*)((size_t)buf_sub + buf_rem - 1 - _c));
			} while (StrCompareN(buf_sub, b, buf_rem) >= 0);
			DigInc('0' + tmp, endofa);
			for (; *buf_sub == '0'; buf_rem--, buf_sub++);
			for (size_t _c = 0; _c < buf_rem; _c++) b[_c] = buf_sub[_c];
			if(buf_rem) b[buf_rem] = 0;// fix @RFW16
			else
			{
				*b = '0';
				b[1] = 0;
			}
		}
	}
	else
	{
		for (register unsigned int _c = 0; _c < buf_rem; _c++)
			b[_c] = buf_sub[_c]; b[buf_rem] = 0;
	}
	if(arna_eflag.HeapYo) memfree(buf_sub_fix);
	ChrCpz(a - (arna_eflag.Signed ? 1 : 0));
	arna_eflag.PrecLoss = 0;
	return;
}
*/
/*
// [Buf Ver] Besides, SLV using
static char* ChrInsPow(const char* in, size_t times)
{
	char* sum;
	if(arna_eflag.HeapYo)
	{
		sum = StrHeap(arna_eflag.Signed ? "+1" : "1");
		while (times--)
			srs(sum, ChrMul(sum, in));
	}
	else
	{
		if ((malc_limit < (arna_eflag.Signed ? 3 : 2)) || !ptr_tempslv)
		{
			;// erro(__FUNC__)
			//TODO
			return 0;
		}
		sum = StrCopy(ptr_tempslv, arna_eflag.Signed ? "+1" : "1");
		while (times-- && *sum)
			ChrMul(sum, in);
	}
	return sum;
} static char* ChrInsPow(const char* in, size_t times);// GCC required outside declaration

char* _Need_free ChrHexToDecBuf(const char* hex)// slv using
{
	int sign = 0;
	if (!hex || !*hex || !ptr_tempmas || !ptr_tempslv)
	{
		malc_occupy = 0;
		arna_eflag.PrecLoss = 1;
		return 0;
	}
	if (*hex == '-')sign++, hex++;
	else if (*hex == '+')hex++;
	size_t hexlen = StrLength(hex);// skip sign digit
	
	char* sum, c;
	if (arna_eflag.HeapYo)
	{
		sum = StrHeap(arna_eflag.Signed ? (sign ? "-0" : "+0") : "0");
		//
		for (size_t i = 0; i < hexlen; i++)// loop
		{
			c = hex[i];
			char* ttemp = StrHeap(arna_eflag.Signed ? "+00" : "00");// 0~F 0~15
			char* ttemp2 = ChrInsPow(arna_eflag.Signed ? "+16" : "16", hexlen - i - 1);
			if (c <= '9' && c >= '0') ttemp[2 - (!arna_eflag.Signed)] += c - '0';
			else
			{
				ttemp[2 - (!arna_eflag.Signed)] += c - (c >= 'a' ? 'a' : 'A');
				ttemp[1 - (!arna_eflag.Signed)] = '1';
			}
			srs(ttemp, ChrMul(ttemp, ttemp2));
			srs(sum, ChrAdd(sum, ttemp));
			memfree(ttemp);
			memfree(ttemp2);
		}
	if (sign && arna_eflag.Signed && *sum == '+') *sum = '-';
	}
	else
	{
		if ((malc_limit < (arna_eflag.Signed ? 3 : 2)) || !ptr_tempmas)
		{
			;// erro(__FUNC__)
			malc_occupy = 0;
			arna_eflag.PrecLoss = 1;
			return 0;
		}
		sum = StrCopy(ptr_tempmas, arna_eflag.Signed ? "+0" : "0");
		char ttemp[4];// sign, end-zero, 00~15
		for (size_t i = 0; i < hexlen; i++)// loop
		{
			c = hex[i];
			StrCopy(ttemp, arna_eflag.Signed ? "+00" : "00");
			xchgptr(ptr_tempmas, ptr_tempext);
			ChrInsPow(arna_eflag.Signed ? "+16" : "16", hexlen - i - 1);
			xchgptr(ptr_tempmas, ptr_tempext);
			if (c <= '9' && c >= '0')
				ttemp[2 - (!arna_eflag.Signed)] += c - '0';
			else
			{
				ttemp[2 - (!arna_eflag.Signed)] += c - (c >= 'a' ? 'a' : 'A');
				ttemp[1 - (!arna_eflag.Signed)] = '1';
			}
			xchgptr(ptr_tempmas, ptr_tempslv);// slv is mas , mas is slv
			ChrMul(ptr_tempext, ttemp);
			xchgptr(ptr_tempmas, ptr_tempext);// ext is slv, mas is ext, slv is mas
			ChrAdd(ptr_tempext, ptr_tempslv);
			xchgptr(ptr_tempmas, ptr_tempext);
			xchgptr(ptr_tempmas, ptr_tempslv);
			StrCopy(ptr_tempmas, ptr_tempext);
		}
	}
	malc_occupy = StrLength(ptr_tempmas) + 1;
	arna_eflag.PrecLoss = 0;
	return sum;
}

char* ChrDecToHexBuf(char* dec)// Output: upper case
{
	// pass a non-0~9 number will cause potential mistake.
	int sign = 0;
	if (!dec || !*dec)
	{
		malc_occupy = 0;
		arna_eflag.PrecLoss = 1;
		return 0;
	}
	if(arna_eflag.Signed)
	if (*dec == '-')sign++, dec++;
	else if (*dec == '+') dec++;
	const char* list = { "0123456789ABCDEF" };
	size_t declen = StrLength(dec);
	size_t declen2 = declen;
	char diver[4], * p;
	
	if (declen + 1 + 1 > malc_limit)// Omit
	{
		malc_occupy = 0;
		arna_eflag.PrecLoss = 1;
		return 0;
	}
	if (arna_eflag.HeapYo)
	{
		memalloc(p, declen + 1 + 1);
	}
	else p = ptr_tempmas;
	if(arna_eflag.Signed) *p = '+';
	// here
	StrCopy(p + !(!arna_eflag.Signed), dec);
	while (declen)
	{
		StrCopy(diver, arna_eflag.Signed ? "+16" : "16");
		xchgptr(ptr_tempmas, ptr_tempext);
		ChrDiv(p, diver);
		xchgptr(ptr_tempmas, ptr_tempext);
		if (diver[2-!arna_eflag.Signed]) dec[declen - 1] = "ABCDEF"[diver[2-!arna_eflag.Signed] - '0'];
		else dec[declen - 1] = diver[1-!arna_eflag.Signed];
		declen--;
	}
	ChrCpz(dec);
	if (arna_eflag.Signed) dec[-1] = sign ? '-' : '+';
	if(arna_eflag.HeapYo) memfree(p);
	///malc_occupy = declen2 + 1 + 1;
	arna_eflag.PrecLoss = 0;
	aflag.Failure = 0;
	aflag.Sign = sign;
	return dec - !(!arna_eflag.Signed);
}

static size_t ChrFactorialFixStack(size_t x)
{
	return x <= 1 ? 1 : x * ChrFactorialFixStack(x - 1);
}
// limit should:>= 3
// Besides MAS: Nothing using, NO for occupy for heap
char* ChrFactorialBuf(const char* a)
{
	if (!arna_eflag.HeapYo)
	{
		instob(ChrFactorialFixStack(atoins(a)), ptr_tempmas);// Auto for 	arna_eflag.PrecLoss
		return ptr_tempmas;
	}
	char* res = StrHeap(arna_eflag.Signed ? "+1" : "1");
	if (a[1] == '0') return res;
	char* tmp = res;
	a = (const char*)StrHeap(a);
	while (ChrCmp(a, arna_eflag.Signed ? "+1" : "1") > 0)
	{
		srs(res, ChrMul(res, a));
		srs(a, ChrSub(a, arna_eflag.Signed ? "+1" : "1"));
	}
	memfree(a);
	arna_eflag.PrecLoss = 0;
	return res;
}

static size_t ChrArrangeFixStack(size_t t, size_t i)// A_t(i)
{
	return i == 0 ? 1 :
		t * ChrArrangeFixStack(t - 1, i - 1);
}
// limit should:>= 3
char* ChrArrangeBuf(const char* total, const char* items)// Amn
{
	if (!arna_eflag.HeapYo)
	{
		instob(ChrArrangeFixStack(atoins(total), atoins(items)), ptr_tempmas);// Auto for 	arna_eflag.PrecLoss
		return ptr_tempmas;
	}
	char* res = StrHeap(arna_eflag.Signed ? "+1" : "1");
	items = StrHeap(items);
	total = StrHeap(total);
	while (ChrCmp(items, "+0") > 0)
	{
		srs(res, ChrMul(res, total));
		srs(total, ChrSub(total, arna_eflag.Signed ? "+1" : "1"));
		srs(items, ChrSub(items, arna_eflag.Signed ? "+1" : "1"));
	}
	memfree(items);
	memfree(total);
	return res;
}

static size_t ChrCombinateFixStack(size_t t, size_t i)// C_t(i)
{
	if (i == 0 || i == t) return 1;
	else return ChrCombinateFixStack(t - 1, i - 1) + ChrCombinateFixStack(t - 1, i);
}
// limit should:>= 3
char* ChrCombinateBuf(const char* total, const char* items)
{
	if (!arna_eflag.HeapYo)
	{
		instob(ChrCombinateFixStack(atoins(total), atoins(items)), ptr_tempmas);// Auto for 	arna_eflag.PrecLoss
		return ptr_tempmas;
	}
	char* temp0, * temp1;
	temp0 = ChrArrange(total, items);
	temp1 = ChrFactorial(items);
	ChrDiv(temp0, temp1);
	memfree(temp1);
	return temp0;
}

static size_t ChrComDivFixStack(size_t a, size_t b)
{
	if (a < b) xchg(a, b);
	if (!b) return a;
	else if (a == b) return a;
	else return ChrComDivFixStack(b, a % b);
}
// [Get Greatest Common Divisor]// limit should:>= 3
char* ChrComDivBuf(const char* op1, const char* op2)
{
	//1. Get the below one, assume it as op1
	//2. !(op2%op1) then dec a;
	if (!arna_eflag.HeapYo)
	{
		instob(ChrComDivFixStack(atoins(op1), atoins(op2)), ptr_tempmas);// Auto for 	arna_eflag.PrecLoss
		return ptr_tempmas;
	}

	if (op2[!!arna_eflag.Signed] == '0') return StrHeap(op2);
	if (arna_eflag.Signed && (*op1 != '+' && *op1 != '-')) { op1 = StrHeapAppend("+", op1); }
	else op1 = StrHeap(op1);
	if (op1[!!arna_eflag.Signed] == '0') return (char*)op1;
	if (arna_eflag.Signed && (*op2 != '+' && *op2 != '-')) { op2 = StrHeapAppend("+", op2); }
	else op2 = StrHeap(op2);
	int state = ChrCmp(op1, op2);
	if (!state)// equal
	{
		memfree(op2);
		return (char*)op1;
	}
	if (state < 0) xchgptr(op1, op2);
	// now +op1 > +op2
	char* tmp_op1;
	while (op2[!!arna_eflag.Signed] != '0')
	{
		tmp_op1 = StrHeap(op1);
		srs(op1, StrHeap(op2));
		ChrDiv(tmp_op1, (char*)op2);
		memfree(tmp_op1);
	}
	memfree(op2);
	return (char*)op1;
}

static size_t ChrComMulFixStack(size_t a, size_t b)
{
	if (!a || !b) return 1;//  or 0?
	return (a * b) / ChrComDivFixStack(a, b);
}
// [Get Least Common Multiple]// limit should:>= 3
char* ChrComMulBuf(const char* op1, const char* op2)
{
	// assume the GCD is g
	// then LCM = m*(b/m)*(a/m) = a*b/m
	if (!arna_eflag.HeapYo)
	{
		instob(ChrComMulFixStack(atoins(op1), atoins(op2)), ptr_tempmas);// Auto for 	arna_eflag.PrecLoss
		return ptr_tempmas;
	}
	char* res = ChrMul(op1, op2);
	char* m = ChrComDiv(op1, op2);
	ChrDiv(res, m);// quo=>res rem=>m
	memfree(m);
	return res;
}


*/