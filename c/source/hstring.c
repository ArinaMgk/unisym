// ASCII TAB4 C99 ArnAssume
// String output specially in heap
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
//depend: ustring
#define _ARN_INSIDE_LIBRARY_INCLUDE
#define _LIB_STRING_HEAP
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>// na isXType()
#include "../alice.h"
#include "../aldbg.h"
#include "../ustring.h"

//---- ---- ---- ---- Dnode ---- ---- ---- ----

Dnode* DnodeCreate(Dnode* any, char* addr, size_t len)
{
	Dnode* crt = any;
	if (!crt)
	{
		crt = malc(sizeof(Dnode));
		crt->left = 0;
		crt->next = 0;
		crt->addr = addr;
		crt->len = len;
		return crt;
	}
	Dnode* NEW = malc(sizeof(Dnode));
	NEW->addr = addr;
	NEW->len = len;
	judge:
	if (addr < crt->addr)
	{
		if (crt->left && addr < crt->left->addr)
		{
			crt = crt->left;
			goto judge;
		}
		else // create in the left (>=)
		{
			// (A) B<CRT> (C)
			// (A-) -NEW- -B (C)
			NEW->left = crt->left;
			NEW->next = crt;
			if(crt->left) crt->left->next = NEW;
			crt->left = NEW;
		}
	}
	else // addr >= any->addr
	{
		if (crt->next && addr >= crt->next->addr)
		{
			crt = crt->next;
			goto judge;
		}
		else
		{
			// (A) B<CRT> (C)
			// (A) B- -NEW- (-C)
			NEW->left = crt;
			NEW->next = crt->next;
			if (crt->next) crt->next->left = NEW;
			crt->next = NEW;
		}
	}
	return crt;
}

Dnode* DnodeRewind(Dnode* any)//need not order
{
	if (!any)return any;// 0
	while (any->left)any = any->left;
	return any;
}

size_t DnodeCount(Dnode* any)
{
	if (!any)return 0;
	size_t count = 0;
	while (any->left) any = any->left;
	while (any->next) { any = any->next; count++; }
	return count;
}

void DnodeRelease(Dnode* some)
{
	if (some->left) some->left->next = some->next;
	if (some->next) some->next->left = some->left;
	memfree(some);
}

void DnodesRelease(Dnode* first)
{
	if (first->next) DnodesRelease(first->next);
	memfree(first);
}

//---- ---- ---- ---- ChrAr ---- ---- ---- ----
// Chr+-*/ would not call each other without considering the aflag.

static char* ChrInsPow(const char* in, size_t times)
{
	char* sum = StrHeap(arna_eflag.Signed ? "+1" : "1");
	while (times--)
		srs(sum, ChrMul(sum, in));
	return sum;
} static char* ChrInsPow(const char* in, size_t times);// GCC required outside declaration


char* _Need_free ChrHexToDec(const char* hex)// slv using
{
	int sign = 0;
	int Signed
	#ifndef _ARN_FLAG_DISABLE
		= aflag.Signed;
	#else
		=1;	
	#endif
	///if (!hex || !*hex)
	if (*hex == '-')sign++, hex++;
	else if (*hex == '+')hex++;
	size_t hexlen = StrLength(hex);// skip sign digit
	
	char* sum, c;
	sum = StrHeap(Signed ? (sign ? "-0" : "+0") : "0");
	//
	for (size_t i = 0; i < hexlen; i++)// loop
	{
		c = hex[i];
		char* ttemp = StrHeap(Signed ? "+00" : "00");// 0~F 0~15
		char* ttemp2 = ChrInsPow(Signed ? "+16" : "16", hexlen - i - 1);
		if (c <= '9' && c >= '0') ttemp[2 - (!Signed)] += c - '0';
		else
		{
			ttemp[2 - (!Signed)] += c - (c >= 'a' ? 'a' : 'A');
			ttemp[1 - (!Signed)] = '1';
		}
		srs(ttemp, ChrMul(ttemp, ttemp2));
		srs(sum, ChrAdd(sum, ttemp));
		memfree(ttemp);
		memfree(ttemp2);
	}
	if (sign && Signed && *sum == '+') *sum = '-';
	#ifndef _ARN_FLAG_DISABLE
	///malc_occupy = StrLength(ptr_tempmas) + 1;
	aflag.PrecLoss = 0;
	aflag.Failure = 0;
	aflag.Sign = sign;
	#endif
	return sum;
}

char* ChrDecToHex(char* dec)// Output: upper case
{
	// pass a non-0~9 number will cause potential mistake.
	int sign = 0;
	int Signed
	#ifndef _ARN_FLAG_DISABLE
		= aflag.Signed;
	#else
		=1;	
	#endif
	///if (!dec || !*dec)
	if(Signed)
	if (*dec == '-')sign++, dec++;
	else if (*dec == '+') dec++;
	const char* list = { "0123456789ABCDEF" };
	size_t declen = StrLength(dec);
	size_t declen2 = declen;
	char diver[4], * p;
	#ifndef _ARN_FLAG_DISABLE
	if (declen + 1 + 1 > malc_limit)
	{
		///malc_occupy = 0;
		aflag.Failure = 1;
		call_state = 1;
		return 0;
	}
	#endif
	memalloc(p, declen + 1 + 1);
	if(Signed) *p = '+';
	// here
	StrCopy(p + !(!Signed), dec);
	while (declen)
	{
		StrCopy(diver, Signed ? "+16" : "16");
		ChrDiv(p, diver);
		if (diver[2-!Signed]) dec[declen - 1] = "ABCDEF"[diver[2-!Signed] - '0'];
		else dec[declen - 1] = diver[1-!Signed];
		declen--;
	}
	ChrCpz(dec);
	if (Signed) dec[-1] = sign ? '-' : '+';
	memfree(p);
	#ifndef _ARN_FLAG_DISABLE
	///malc_occupy = declen2 + 1 + 1;
	aflag.PrecLoss = 0;
	aflag.Failure = 0;
	aflag.Sign = sign;
	#endif
	return dec - Signed;
}

// RFV3 (ArnMgk). E.g. "FE" stands for "0.FEH" and this return "9921875" stand for "0.9921875"
//   (15/16+14/256) or (254/256), so "ABCD" : "ABH/256+CDH/256/256"
//   171_0_0000_0000_0000_0000/256 = 66796875000000000
//   205_0_0000_0000_0000_0000/65536=  312805175781250
//   then cut the trailing zeros and the sign prefix{TODO optional}
// ! input upper cases and this excludes element check.
char* _Need_free ChrHexToDecFloat(const char* hexf)
{
	// assume limit 0X800, adapt in the future.
	#ifndef _ARN_FLAG_DISABLE
	struct ArinaeFlag tmpflg = aflag;
	aflag.Signed = 1;
	#endif
	char unit_diver[] = "+16";
	char* CrtDiver = StrHeap(unit_diver);// keep this size < 0x1000
	char* temp = zalc(0X800 + 1);
	char* temp_for_crt = zalc(0X800 + 1);
	size_t tempexpo = 0X800 - 3;// based on 10
	char* res = StrHeap("+0");
	size_t paralen = StrLength(hexf);
	char c;
	for (size_t i = 0; i < paralen; i++)
	{
		char* ptr = temp;
		*ptr++ = '+';
		if (hexf[i] >= '0' && hexf[i] <= '9') *ptr++ = hexf[i];
		else
		{
			*ptr++ = '1';
			*ptr++ = hexf[i] - 'A' + '0';
		}
		size_t CrtCompensate = tempexpo;// 0x1000 - (ptr - temp);
		MemSet(ptr, '0', CrtCompensate);
		ptr[CrtCompensate] = 0;
		char* crtdiv = StrCopyN(temp_for_crt, CrtDiver, 0X800);
		ChrDiv(temp, crtdiv);// LIMIT EXIS
		srs(res, ChrAdd(res, temp));
		srs(CrtDiver, ChrMul(CrtDiver, unit_diver));
	}
	memfree(CrtDiver);
	memfree(temp);
	memfree(temp_for_crt);
	ChrCtz(res);
	srs(res, StrHeap(res + 1));
	#ifndef _ARN_FLAG_DISABLE
	aflag = tmpflg;
	#endif
	return res;
}

// RFV3 (ArnMgk). E.g. "81" ---> "", based on ChrHexToDecFloat
//  0.99 --> FD70AF
//    0.99*16=15.84 0.84*16=13.44 0.44*16=7.04 0.04*16=0.64 0.64*16=10.24
//    0.24*64=15.36 0.36...
char* _Need_free ChrDecToHexFloat(const char* decf, size_t digits)
{
	#ifndef _ARN_FLAG_DISABLE
	struct ArinaeFlag tmpflg = aflag;
	aflag.Signed = 1;
	#endif
	size_t CrtLastDigs = StrLength(decf), CrtAfterDigs;
	char* const res = zalc(digits + 1);
	char* ptr = res;
	char* buf = malc(CrtLastDigs + 1 + 1 + 2);// Sign+Null+Extern
	buf[0] = '+';
	StrCopy(buf + 1, decf);
	for (size_t i = 0; i < digits; i++)
	{
		srs(buf, ChrMul("+16", buf));
		CrtAfterDigs = StrLength(buf + 1);
		if (CrtAfterDigs - CrtLastDigs == 2)
		{
			if (buf[1] == '1')
			{
				*ptr++ = 'A' + (buf[2] - '0');
			}
			else (void)erro;
			MemRelative(buf + 3, CrtAfterDigs - 2 + 1, -2);// include 0
		}
		else if (CrtAfterDigs - CrtLastDigs == 1)
		{
			*ptr++ = (buf[1]);
			MemRelative(buf + 2, CrtAfterDigs, -1);// include 0
		}
		else
		{
			*ptr++ = '0';
		}
	}
	memfree(buf);
	#ifndef _ARN_FLAG_DISABLE
	aflag = tmpflg;
	#endif
	return res;
}


// ASCII String Add (without input check)
char* ChrAdd(const char* a, const char* b)
{
	// may xchg to make length a >= b
	if (!a || !b || !*a || !*b)
	{
		#ifndef _ARN_FLAG_DISABLE// with malc_states
		///malc_occupy = 0;
		aflag.PrecLoss = 1;
		aflag.Failure = 1;
		call_state = 0;
		#endif
		return 0;
	}
	struct { unsigned carry : 1, SignB : 1, SignA : 1; } flag = { 0 };
	if (*a == '+') { a++; }
	if (*a == '-') { a++;flag.SignA = 1; }
	if (*b == '+') { b++; }
	if (*b == '-') { b++;flag.SignB = 1; }
	if (flag.SignA ^ flag.SignB)
	{
		// the control of aflag is given to ChrSub(). 
		return (flag.SignA ? ChrSub(b, a) : ChrSub(a, b));
	}
	const char* endofa = a, * endofb = b;// last digit
	for (; '0' <= *endofa && *endofa <= '9'; endofa++); endofa--;
	for (; '0' <= *endofb && *endofb <= '9'; endofb++); endofb--;
	if (endofa < a || endofb < b)
	{
		#ifndef _ARN_FLAG_DISABLE
		///malc_occupy = 0;
		aflag.Failure = 1;
		call_state = 2;
		#endif
		return 0;
	}
	if (endofa - a < endofb - b) // xchg
	{
		xchgptr(a, b);
		xchgptr(endofa, endofb);
		xchg(flag.SignA, flag.SignB);
	}
	size_t siz = endofa - a + 4;// Magic 4: Potential Symbol, \0, LengthDif:1, CarryDigit
	#ifndef _ARN_FLAG_DISABLE
	if (siz > malc_limit)
	{
		///malc_occupy = 0;
		aflag.Failure = 1;
		call_state = 1;
		return 0;
	}
	#endif
	char* q = malc(siz);
#ifndef _ARN_FLAG_DISABLE// or execuate once
	aflag.Sign = flag.SignA;
	if (aflag.Signed)
	{
		q[siz - 1] = 0;
		*q = flag.SignA ? '-' : '+';
	} else q[siz - 2] = 0;
#else
	q[siz - 1] = 0;
	*q = flag.SignA ? '-' : '+';
#endif
	size_t loop_time = siz - 3;// detail to see old version
	////for (size_t i = 0; i < siz - 3; i++)
	if(loop_time) do
	{
		unsigned char regichar = *(endofa--) - 0x30;
		if (endofb >= b) regichar += *(endofb--) - 0x30;
		regichar += flag.carry;
		if (flag.carry = (regichar > 9)) regichar -= 10;
#ifndef _ARN_FLAG_DISABLE
		q[loop_time-- + aflag.Signed] = regichar + 0x30;
#else
		q[loop_time-- + 1] = regichar + 0x30;
#endif
	} while (loop_time);
#ifndef _ARN_FLAG_DISABLE
	q[aflag.Signed ? 1 : 0] = flag.carry + 0x30;
	ChrCpz(q);
	aflag.PrecLoss = 0;
	///malc_occupy = siz;
	aflag.Failure = 0;
#else
	q[1] = flag.carry + 0x30;
	ChrCpz(q);
#endif
	return q;
}

// ASCII String Sub (without input check)
char* ChrSub(const char* a, const char* b)
{
	// may xchg to make length a >= b
	if (!a || !b || !*a || !*b)
	{
		#ifndef _ARN_FLAG_DISABLE
		///malc_occupy = 0;
		aflag.PrecLoss = 1;
		aflag.Failure = 1;
		call_state = 0;
		#endif
		return 0;
	}
	struct { unsigned borrow : 1, SignB : 1, SignA : 1, xchged : 1; } flag = { 0 };
	if (*a == '+') { a++; }
	if (*a == '-') { a++;flag.SignA = 1; }
	if (*b == '+') { b++; }
	if (*b == '-') { b++;flag.SignB = 1; }
	if (flag.SignA ^ flag.SignB)
		if (flag.SignA)// (-a)-(+b)
		{
			char* r = ChrAdd(a, b);
			#ifndef _ARN_FLAG_DISABLE
			if (aflag.Signed)
				aflag.Sign = 1, r[0] = '-';
			#else
			r[0] = '-';// default signed
			#endif
			return r;
		}
		else return ChrAdd(a, b);// (+a)-(-b)
	const char* endofa = a, * endofb = b;// last digit
	for (; '0' <= *endofa && *endofa <= '9'; endofa++); endofa--;
	for (; '0' <= *endofb && *endofb <= '9'; endofb++); endofb--;
	if (endofa < a || endofb < b)
	{
		#ifndef _ARN_FLAG_DISABLE
		///malc_occupy = 0;
		aflag.Failure = 1;
		call_state = 2;
		#endif
		return 0;
	}
	if (endofa - a < endofb - b || \
		(endofa - a == endofb - b) && (StrCompare(a, b) < 0))
	{
		xchgptr(a, b);
		xchgptr(endofa, endofb);
		xchg(flag.SignA, flag.SignB);
		flag.xchged = 1;
	}
	size_t siz = endofa - a + 3;// Magic Number 3: Potential Symbol, \0, LengthDif:1
	#ifndef _ARN_FLAG_DISABLE
	if (siz > malc_limit)// OMIT SIGN <TODO>
	{
		///malc_occupy = 0;
		aflag.Failure = 1;
		call_state = 1;
		return 0;
	}
	#endif
	char* q = malc(siz);
#ifndef _ARN_FLAG_DISABLE
	aflag.Sign = (flag.SignA ^ flag.xchged);
	if (aflag.Signed)
	{
		q[siz - 1] = 0;
		// if no changed, the sign keep with them
		*q = (flag.SignA ^ flag.xchged) ? '-' : '+';
	} else q[siz - 2] = 0;
#else
	q[siz - 1] = 0;
	*q = (flag.SignA ^ flag.xchged) ? '-' : '+';
#endif
	size_t loop_time = siz - 2;
	if(loop_time) do
	{
		signed char regichar = *(endofa--) - 0x30;
		if (endofb >= b) regichar -= *(endofb--) - 0x30;
		if (flag.borrow = ((regichar -= flag.borrow) < 0)) regichar += 10;
		#ifndef _ARN_FLAG_DISABLE
		q[loop_time - 1 + aflag.Signed] = regichar + 0x30;
		#else
		q[loop_time] = regichar + 0x30;
		#endif
	} while (--loop_time);
	ChrCpz(q);
	#ifndef _ARN_FLAG_DISABLE
	aflag.PrecLoss = 0;
	///malc_occupy = siz;
	aflag.Failure = 0;
	#endif
	return q;
}

// ASCII String Mul (without input check)
char* ChrMul(const char* a, const char* b)
{
	// may xchg to make length a >= b
	if (!a || !b || !*a || !*b)
	{
		#ifndef _ARN_FLAG_DISABLE
		///malc_occupy = 0;
		aflag.PrecLoss = 1;
		aflag.Failure = 1;
		call_state = 0;
		#endif
		return 0;
	}
	struct { unsigned carry_int : 8, SignB : 1, SignA : 1; } flag = { 0 };
	char* q = 0, * p = 0;
	size_t siz;
	if (*a == '+') { a++; }
	if (*b == '+') { b++; }
	if (*a == '-') { a++; flag.SignA = 1; }
	if (*b == '-') { b++, flag.SignB = 1; }
	const char* endofa = a, * endofb = b;// last digits
	for (; '0' <= *endofa && *endofa <= '9'; endofa++); endofa--;
	for (; '0' <= *endofb && *endofb <= '9'; endofb++); endofb--;
	if (endofa < a || endofb < b)
	{
		#ifndef _ARN_FLAG_DISABLE
		///malc_occupy = 0;
		aflag.Failure = 1;
		call_state = 2;
		#endif
		return 0;
	}
	if (endofa - a < endofb - b || (endofa - a == endofb - b) && (StrCompare(a, b) < 0))
	{
		xchgptr(a, b);
		xchgptr(endofa, endofb);
		xchg(flag.SignA, flag.SignB);
	}
	#ifndef _ARN_FLAG_DISABLE
	if ((siz = (endofa - a + 1 + endofb - b + 1 + 1 + aflag.Signed)) > malc_limit)// Termino-Null and Sign, 9*9=81
	{
		///malc_occupy = 0;
		aflag.Failure = 1;
		call_state = 1;
		return 0;
	}
	#else
	siz = (endofa - a + 1 + endofb - b + 1 + 1 + 1);
	#endif
	q = salc(siz);
	MemSet(q, '0', siz - 1);
#ifndef _ARN_FLAG_DISABLE
	aflag.Sign = (flag.SignA ^ flag.SignB);
	if (aflag.Signed)
#endif
	{
		*q = (flag.SignA ^ flag.SignB) ? '-' : '+';
	}
	for (int i = 0; i < endofb - b + 1; i++)
	{
		p = q + siz - 2 - i;
		flag.carry_int = 0;
		for (int j = 0; j < endofa - a + 1; j++)
		{
			int tmp = (*(endofa - j) - '0') * (*(endofb - i) - '0') + flag.carry_int;
			if (tmp > 9)
				tmp -= 10 * (flag.carry_int = tmp / 10);
			else flag.carry_int = 0;
			DigInc(tmp + '0', p--);
		}
		if (flag.carry_int) DigInc(flag.carry_int + '0', p--);
	}
	ChrCpz(q);
	#ifndef _ARN_FLAG_DISABLE
	aflag.PrecLoss = 0;
	///malc_occupy = siz;
	aflag.Failure = 0;
	#endif
	return q;
}

// ASCII String Div (without input check)(aflag.Signed decide I&O but just O)
void ChrDiv(char* a, char* b)
{
	if (!a || !b || !*a || !*b)// {TODO} option to close, for faster speed of arith.
	{
		#ifndef _ARN_FLAG_DISABLE// with malc_states
		///malc_occupy = 0;
		aflag.PrecLoss = 1;
		aflag.Failure = 1;
		call_state = 0;
		#endif
		return;
	}
	struct { unsigned SignB : 1, SignA : 1; } flag = { 0 };
	int tmp;//
#ifndef _ARN_FLAG_DISABLE
	if (aflag.Signed)
	{
		if (*a != '+' && *a != '-' || *b != '+' && *b != '-')
		{
			#ifndef _ARN_FLAG_DISABLE
			///malc_occupy = 0;
			aflag.Failure = 1;
			call_state = 2;
			#endif
			return;// BAD INPUT
		}
		int sign_a = (*a == '-') ^ (*b == '-');
		aflag.Sign = sign_a;
		*b++ = *a;
		*a++ = sign_a ? '-' : '+';
	}
#else
	aflag.Sign = 0;
#endif
	char* endofa = a, * endofb = b;// last digits
	for (; '0' <= *endofa && *endofa <= '9'; endofa++); endofa--;
	for (; '0' <= *endofb && *endofb <= '9'; endofb++); endofb--;
	if (endofa < a || endofb < b)
	{
		#ifndef _ARN_FLAG_DISABLE
		///malc_occupy = 0;
		aflag.Failure = 1;
		call_state = 2;
		#endif
		return;
	}
	if ((a < b) && (endofa + 1 >= b - 1) || (a == b) || (a > b) && (endofb + 1 >= a - 1))
	{
		#ifndef _ARN_FLAG_DISABLE
		///malc_occupy = 0;
		aflag.Failure = 1;
		call_state = 2;
		#endif
		return;// INPUT CROSS
	}
	if (*b == '0')
	{
		#ifndef _ARN_FLAG_DISABLE
		///malc_occupy = 0;
		aflag.Failure = 1;
		call_state = 2;
		#endif
		aflag.PrecLoss = 1;
		return;// EXCEPTION
	}
	if (*a == '0' || (endofb - b > endofa - a) || (endofb - b == endofa - a && (StrCompare(a, b) < 0)))
	{
		for (register int _c = 0; _c < endofa - a + 1; _c++) b[_c] = a[_c];
		b[endofa - a + 1] = 0;
		*a++ = '0'; *a = 0;
		#ifndef _ARN_FLAG_DISABLE
		aflag.Failure = 0;
		aflag.PrecLoss = 0;
		#endif
		return;
	}
	size_t siz = endofa - a + 2;// sign and ter-0
	#ifndef _ARN_FLAG_DISABLE
	if (siz > malc_limit)
	{
		///malc_occupy = 0;
		aflag.Failure = 1;
		call_state = 1;
		return;// LACK MEMORY
	}
	#endif
	char* buf_sub = salc(siz);
	char* const buf_sub_fix = buf_sub;
	size_t buf_rem = siz - 1;
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
			for (size_t _c = 0; _c < buf_rem; _c++) b[_c] = buf_sub[_c]; /* b[buf_rem] = 0; */
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
	memfree(buf_sub_fix);
	ChrCpz(a);
	aflag.PrecLoss = 0;
	aflag.Failure = 0;
	return;
}

char* ChrFactorial(const char* a)
{
	char* res = StrHeap(aflag.Signed ? "+1" : "1");
	if (a[1] == '0') goto endo;
	char* tmp = res;
	a = (const char*)StrHeap(a);
	while (ChrCmp(a, aflag.Signed ? "+1" : "1") > 0)
	{
		srs(res, ChrMul(res, a));
		srs(a, ChrSub(a, aflag.Signed ? "+1" : "1"));
	}
	memfree((char*)a);
endo:
	#ifndef _ARN_FLAG_DISABLE
	aflag.PrecLoss = 0;
	///malc_occupy = siz;
	aflag.Failure = 0;
	aflag.Sign = (*res == '-');
	#endif
	return res;
}

char* ChrArrange(const char* total, const char* items)// Amn
{
	int Signed
	#ifndef _ARN_FLAG_DISABLE
		= aflag.Signed;
	#else
		=1;	
	#endif
	char* res = StrHeap(Signed ? "+1" : "1");
	items = StrHeap(items);
	total = StrHeap(total);
	while (ChrCmp(items, "+0") > 0)
	{
		srs(res, ChrMul(res, total));
		srs(total, ChrSub(total, Signed ? "+1" : "1"));
		srs(items, ChrSub(items, Signed ? "+1" : "1"));
	}
	memfree(items);
	memfree(total);
	#ifndef _ARN_FLAG_DISABLE
	aflag.PrecLoss = 0;
	///malc_occupy = siz;
	aflag.Failure = 0;
	aflag.Sign = (*res == '-');
	#endif
	return res;
}

char* ChrCombinate(const char* total, const char* items)
{
	char* temp0, * temp1;
	temp0 = ChrArrange(total, items);
	temp1 = ChrFactorial(items);
	ChrDiv(temp0, temp1);
	memfree(temp1);
	return temp0;
}

char* ChrComDiv(const char* op1, const char* op2)
{
	//1. Get the below one, assume it as op1
	//2. !(op2%op1) then dec a;
	// to polish
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

char* ChrComMul(const char* op1, const char* op2)
{
	// assume the GCD is g
	// then LCM = m*(b/m)*(a/m) = a*b/m
	// to polish
	char* res = ChrMul(op1, op2);
	char* m = ChrComDiv(op1, op2);
	ChrDiv(res, m);// quo=>res rem=>m
	memfree(m);
	return res;
}

//---- ---- ---- ---- strpool ---- ---- ---- ----

static void Func_StrPoolRelease(void* memblk);
#ifdef _dbg
#define NewPool() {\
	TMP=CRT_POOL;\
	*(void**)(CRT_POOL = (char*)malloc(_ModString_Strpool_UNIT_SIZE)) = 0;\
	if(TMP) *(void**)TMP=CRT_POOL;\
	Available = (_ModString_Strpool_UNIT_SIZE - sizeof(void*));\
	malc_count++;\
	}
#else
#define NewPool() {\
	TMP=CRT_POOL;\
	*(void**)(CRT_POOL = (char*)malloc(_ModString_Strpool_UNIT_SIZE)) = 0;\
	if(TMP) *(void**)TMP=CRT_POOL;\
	Available = (_ModString_Strpool_UNIT_SIZE - sizeof(void*));\
	}
#endif

#define Macro_StrPoolRelease() Func_StrPoolRelease((void*)STR_POOL)

static char* STR_POOL = 0, * CRT_POOL = 0, * TMP;
static size_t Available;

void StrPoolInit()
{
	if (STR_POOL) Macro_StrPoolRelease();
	NewPool();
	STR_POOL = CRT_POOL;
}

char* StrPoolHeap(const char* str, size_t length)
{
	if (!length) while (str[length]) { length++; } length++;
	char* ebx = StrPoolAlloc(length);
	StrCopyN(ebx, str, length);
	return ebx;
}

char* StrPoolAllocZero(size_t length)
{
	if (!length) return 0;
	if (length <= _ModString_Strpool_UNIT_SIZE - sizeof(void*));
	else
	{
		TMP = CRT_POOL;
		*(void**)(CRT_POOL = (char*)zalloc(sizeof(void*) + length)) = 0;
		if (TMP) *(void**)TMP = CRT_POOL;
		#ifdef _dbg
		malc_count++;
		#endif
		Available = 0;
		return CRT_POOL + sizeof(void*);
	}
	register unsigned int ecx = length;
	if (Available <= length) { NewPool(); }
	Available -= length;
	length = (size_t)CRT_POOL + _ModString_Strpool_UNIT_SIZE - Available;
	while (ecx--)*(char*)(--length) = 0;
	return (char*)(length);
}

char* StrPoolAlloc(size_t length)
{
	if (!length) return 0;
	if (length <= _ModString_Strpool_UNIT_SIZE - sizeof(void*));
	else
	{
		TMP = CRT_POOL;
		*(void**)(CRT_POOL = (char*)malloc(sizeof(void*) + length)) = 0;
		if (TMP) *(void**)TMP = CRT_POOL;
		#ifdef _dbg
		malc_count++;
		#endif
		Available = 0;
		return CRT_POOL + sizeof(void*);
	}
	if (Available <= length) { NewPool(); }
	char* ret = (CRT_POOL + _ModString_Strpool_UNIT_SIZE - Available);
	Available -= length;
	return ret;
}

static void Func_StrPoolRelease(void* memblk)
{
	if (!memblk)return;
	if (*(void**)memblk)
	{
		Func_StrPoolRelease(*(void**)memblk);
		*(void**)memblk = 0;
	}
	free(memblk);
	#ifdef _dbg
		malc_count--;
	#endif
}

void StrPoolRelease()
{
	Macro_StrPoolRelease();
	STR_POOL = 0;
}

//---- ---- ---- ---- common ---- ---- ---- ----

// It may be better to use Regular Expression
const char static EscSeq[] = 
{
	'n','\n','r','\r','a','\a','b','\b','f','\f','t','\t','v','\v'
};// other equal to literal char except the "\x"[0~2] "\"[0~2]

#define DECLEN_16B 5 // ~0 = 65535
#define DECLEN_32B 10//      4,294,967,295
#define DECLEN_64B 20//      18,446,744,073,709,551,615
static const unsigned DECLEN_a[] = { 1,2,3,DECLEN_16B, DECLEN_32B, DECLEN_64B };

static Toknode* StrTokenAppend(Toknode* any, char* content, size_t contlen, size_t ttype, size_t row, size_t col);
static TokType getctype(char chr);
static size_t StrTokenAll_NumChk(int (*getnext)(void), void (*seekback)(ptrdiff_t chars), char* bufptr);
static size_t crtline = 0, crtcol = 0;
Toknode* StrTokenAll(int (*getnext)(void), void (*seekback)(ptrdiff_t chars), char* buffer)
{
	// Combination of -File and -Buf, ArinaMgk, RFT03.
	// More infomation, to see the previous version.
	
	Toknode* first = 0, * crt = 0;
	TokType CrtTType;
	size_t CrtTLen = 0;// = real length minus 1
	int YoString = 0; 
	char strtok;
	char* bufptr;

	int c;
	crtcol = crtline = 0;
	memalloc(first, sizeof(Toknode));
	crt = first;
	first->left = first->next = 0;
	first->addr = 0;
	CrtTType = first->len = tok_any;
	first->col = first->row = 0;
	bufptr = buffer;
	// alnum & space & symbol
	while ((crtcol++, c = getnext()) != EOF)
	{
		char dbg_chr = (char)c;
		if (YoString)// Care about CrtTLen and *bufptr especially
		{
			if (strtok == c)// exit
			{
				CrtTType = tok_string;
				crt = StrTokenAppend(crt, buffer, CrtTLen, CrtTType, crtline, crtcol);// including terminated-symbol
				bufptr = buffer;
				YoString = 0;
				CrtTLen = 0;
				CrtTType = tok_any;
			}
			else if (c == '\\')// escape sequence
			{
				crtcol++, c = getnext();
				if (c == EOF) break;
				if (isdigit(c))
				{
					// octal: nest 0~2 3 numbers at most
					int nest1 = (crtcol++, getnext());
					if (nest1 == EOF) break;
					if (isdigit(nest1))
					{
						int nest2 = (crtcol++, getnext());
						if (nest2 == EOF) break;
						if (isdigit(nest2))
						{
							// \123
							*bufptr++ = (unsigned char)(nest2 - '0' + (nest1 - '0') * 8 + (c - '0') * 8 * 8);
							CrtTLen++;
						}
						else
						{
							// \12;
							*bufptr++ = (unsigned char)((nest1 - '0') + (c - '0') * 8);
							CrtTLen++;
							crtcol--, seekback(-1);
						}
					}
					else
					{
						// \1;
						*bufptr++ = (unsigned char)(c - '0');
						CrtTLen++;
						crtcol--, seekback(-1);
					}
				}
				else if (c == 'x')// nest 0~3 and use isxdigit()
				{
					crtcol++, c = getnext();
					if (c == EOF) break;
					if (isxdigit(c))
					{
						int nest1 = (crtcol++, getnext());
						if (nest1 == EOF) break;
						if (isxdigit(nest1))
						{
							// \x12;
							nest1 = (isdigit(nest1)) ? (nest1 - '0') : (isupper(nest1)) ? (nest1 - 'A' + 10) : (nest1 - 'a' + 10);
							c = (isdigit(c)) ? (c - '0') : (isupper(c)) ? (c - 'A' + 10) : (c - 'a' + 10);
							*bufptr++ = (unsigned char)(c * 16 + nest1);
							CrtTLen++;
						}
						else
						{
							// \x1;
							c = (isdigit(c)) ? (c - '0') : (isupper(c)) ? (c - 'A' + 10) : (c - 'a' + 10);
							*bufptr++ = (unsigned char)(c);
							CrtTLen++;
							crtcol--, seekback(-1);
						}
					}
					else
					{
						// \x; = \0;
						*bufptr++ = 0;
						CrtTLen++;
						crtcol--, seekback(-1);
					}
				}
				else
				{
					// escape char and other
					if (c == EOF) break;
					size_t listlen = sizeof(EscSeq) / 2;
					for (size_t i = 0; i < listlen; i++)
					{
						if (EscSeq[i << 1] == c)
						{
							*bufptr++ = EscSeq[(i << 1) + 1];
							CrtTLen++;
							listlen = 0;
							break;
						}
					}
					if (!listlen) continue;
					*bufptr++ = c;
					CrtTLen++;
				}
			}
			else
			{
				*bufptr++ = c;
				CrtTLen++;
			}
		}
		else if (c == '\n' || c == '\r')
		{
			// CRLF LFCR CR LF
			int cc = (crtcol++, getnext());
			if (cc == '\n' || cc == '\r')
				if (c == cc)
				{
					crtcol--, seekback(-1);
				}
				else;
			else if (cc != EOF) crtcol--, seekback(-1);
			else break;
			if (CrtTLen)
			{
				crtcol--;
				crt = StrTokenAppend(crt, buffer, CrtTLen, CrtTType, crtline, crtcol - 1);
				bufptr = buffer;
				CrtTLen = 0;
			}
			crtline++;
			crtcol ^= crtcol;
		}
		else if (c == '#' || c == '%')// Line Comment
		{
			crt = StrTokenAppend(crt, buffer, CrtTLen, CrtTType, crtline, crtcol - 1);
			bufptr = buffer;
			CrtTLen = 0;
			CrtTType = (c == '#') ? tok_comment : tok_directive;
			while ((crtcol++, c = getnext()) != EOF && c != '\n' && c != '\r')
			{
				CrtTLen++;
				*bufptr++ = c;
			}
			if (c == EOF)break;
			crt = StrTokenAppend(crt, buffer, CrtTLen, CrtTType, crtline, crtcol - 1);
			bufptr = buffer;
			CrtTLen = 0;
			CrtTType = tok_any;
			crtcol--, seekback(-1);
		}
		else if (c == '\"' || c == '\'')// enter
		{
			crt = StrTokenAppend(crt, buffer, CrtTLen, CrtTType, crtline, crtcol - 1);
			CrtTLen = 0;
			bufptr = buffer;

			YoString = 1;
			strtok = c;
			CrtTLen = 0;
			bufptr = buffer;
		}
		else if ((isdigit(c)) && CrtTType != tok_iden &&
			((CrtTType != tok_any && (crt = StrTokenAppend(crt, buffer, CrtTLen, CrtTType, crtline, crtcol - 1))) || CrtTType == tok_any)
			&& (CrtTLen = StrTokenAll_NumChk(getnext, seekback, buffer)))
		{
			CrtTType = tok_number;
			crt = StrTokenAppend(crt, buffer, CrtTLen, CrtTType, crtline, crtcol);
			CrtTType = tok_any;
			CrtTLen = 0;// NOTICE!
			bufptr = buffer;
			continue;
		}
		else if (isalnum(c) || c == '_')
		{
			if (CrtTType == tok_iden || CrtTType == tok_any)
			{
				CrtTLen++;// seem a waste after declaring bufptr.
				*bufptr++ = c;
				if (CrtTType == tok_any) CrtTType = tok_iden;
				if (CrtTLen >= malc_limit)// sizeof(array buffer), "=" considers terminated-zero.
				{
					#ifdef _dbg
					// erro("Buffer Oversize!");
					#endif
					return 0;
				}
			}
			else
			{
				crt = StrTokenAppend(crt, buffer, CrtTLen, CrtTType, crtline, crtcol - 1);
				CrtTType = getctype(c);
				CrtTLen = 1;
				bufptr = buffer;
				*bufptr++ = c;
			}
		}
		else
		{
			if (CrtTType == getctype(c) || CrtTType == tok_any)
			{
				CrtTLen++;// seem a waste after declaring bufptr.
				*bufptr++ = c;
				if (CrtTType == tok_any) CrtTType = getctype(c);
				if (CrtTLen >= malc_limit)// sizeof(array buffer), "=" considers terminated-zero.
				{
					#ifdef _dbg
					// erro("Buffer Oversize!");
					#endif
					return 0;
				}
			}
			else
			{
				crt = StrTokenAppend(crt, buffer, CrtTLen, CrtTType, crtline, crtcol - 1);
				CrtTLen = 1;
				bufptr = buffer;
				*bufptr++ = c;
				CrtTType = getctype(c);
			}
		}
	}
	if (CrtTLen)
		crt = StrTokenAppend(crt, buffer, CrtTLen, CrtTType, crtline, crtcol - 1);
endo:
	return first;
}

inline static void StrTokenClear(Toknode* one)
{
	memfree(one->addr);
	memfree(one);
}

void StrTokenClearAll(Toknode* tstr)
{
	while (tstr->left) tstr = tstr->left;
	while (tstr)
	{
		tstr = tstr->next;
		StrTokenClear(tstr->left);
	}
}

void StrTokenThrow(Toknode* one)
{
	if (one->left) one->left->next = one->next;
	if (one->next) one->next->left = one->left;
	StrTokenClear(one);
}

static Toknode* StrTokenAppend(Toknode* any, char* content, size_t contlen, size_t ttype, size_t row, size_t col)//TODO. TokType ttype
{
	Toknode* crt = any, * ret = 0;
	if (!contlen) return any;
	while (crt->next) crt = crt->next;
	memalloc(ret, sizeof(Toknode));
	ret->left = crt;
	ret->next = 0;
	crt->next = ret;
	ret->addr = StrHeapN(content, contlen);// TODO. order&addr
	ret->len = ttype;//TODO. union{len, toktype}
	ret->row = row;
	ret->col = col;
	return ret;
}

static TokType getctype(char chr)// Excluding Number
{
	// is going to be renamed "ctype(char)"
	if (isalnum(chr))
		return tok_iden;
	else if (isspace(chr))
		return tok_space;
	else if (ispunct(chr))
		return tok_sym;// including '_' but should be of identifier
	return tok_else;
}

static size_t StrTokenAll_NumChk(int (*getnext)(void), void (*seekback)(ptrdiff_t chars), char* bufptr)
{
	// Combination of -File and -Buf, ArinaMgk, RFT03.
	// More infomation, to see the previous version.
	
	size_t res = 0;
	int c;
	int last_zo_num = 0;
	struct OnceOccur
	{
		unsigned bodx : 1;
		unsigned e : 1;
		unsigned dot : 1;
		unsigned sign : 1;
	} OnceO = { 0 };
	crtcol--, seekback(-1);
	while ((crtcol++, c = getnext()) != EOF)
	{
		if (StrIndexChar("bodx", c))
			if (OnceO.bodx)
				break;
			else
				{last_zo_num = 0; OnceO.bodx = 1;}
		else if (c == 'e')
			if (OnceO.e || !last_zo_num)
				break;
			else
				{last_zo_num = 0; OnceO.e = 1;}
		else if (c == '.')
			if (OnceO.dot)
				break;
			else
				{last_zo_num = 0; OnceO.dot = 1;}
		else if (c == '+' || c == '-')
			if (OnceO.sign || !OnceO.e || *(bufptr - 1) != 'e')
				break;
			else
				{last_zo_num = 0; OnceO.sign = 1;}
		else if (c >= '0' && c <= '9') last_zo_num = 1;
		else break;
		res++;
		*bufptr++ = c;
	}
	crtcol--;
	if (c != EOF)
		seekback(-1);
	if (res > 1 && (*(bufptr - 1) == '+' || *(bufptr - 1) == '-' || *(bufptr - 1) == '.'))
	{
		crtcol--, seekback(-1);
		res--;
	}
	return res;
}

char* StrHeap(const char* valit_str)
{
	size_t strlen = StrLength(valit_str);
	char* ret = zalc(strlen + 1);
	StrCopy(ret, valit_str);
	return ret;
}

void* MemHeap(const void* sors, size_t byteof)
{
	char* ret = malc(byteof);
	for (size_t i = 0; i < byteof; i++) ret[i] = ((const char*)sors)[i];
	return ret;
}

char* StrHeapN(const char* valit_str, size_t strlen)
{
	char* ret = zalc(strlen + 1);
	StrCopyN(ret, valit_str, strlen);
	return ret;
}

char* StrHeapAppend(const char* dest, const char* sors)
{
	size_t dlen = StrLength(dest);
	size_t slen = StrLength(sors);
	char* ret = salc(dlen + slen + 1);
	StrCopy(ret, dest);
	StrCopy(ret + dlen, sors);
	return ret;
}

char* StrHeapAppendN(const char* dest, const char* sors, size_t n)
{
	size_t dlen = StrLength(dest);
	char* ret = malc(dlen + n + 1);
	StrCopy(ret, dest);
	StrCopyN(ret + dlen, sors, n);
	return ret;
}

// From left, one by one, return all in heap! must free() by yourself. Inputs won't be free!
// RFV07 rename from "StrReplaceHeap"
char* StrReplace(const char* dest, const char* subfirstrom, const char* subto, size_t* times)
{
	if (times) *times = 0;
	if (!dest || !subfirstrom || !subto || !malc_limit)
		return zalc(1);
	if (!*dest || !*subfirstrom) return StrHeap(dest);
	Dnode* dn = 0;
	size_t sz_subto = 0, nums = 0, sz_subfirstrom = 0, sz_len = 0;
	ptrdiff_t chars_add = 0;
	for (; subto[sz_subto]; sz_subto++);
	for (; subfirstrom[sz_subfirstrom]; sz_subfirstrom++);
	for (; dest[sz_len]; sz_len++);
	chars_add = (ptrdiff_t)sz_subto - (ptrdiff_t)sz_subfirstrom;
	const char* p = dest;
	while (1)
	{
		p = StrIndexString(p, subfirstrom);
		if (!p) break;
		dn = DnodeCreate(dn, (char*)p, sz_subfirstrom);
		nums++;
		p += sz_subfirstrom;
	}
	if (nums)
	{
		if (times) *times = nums;
		char* ret = malc((ptrdiff_t)sz_len + (ptrdiff_t)chars_add + (ptrdiff_t)1);
		ret[(ptrdiff_t)sz_len + (ptrdiff_t)chars_add] = 0;// for dbg
		p = dest; char* q = ret;
		dn = DnodeRewind(dn);
	loop:
		for (; p < dn->addr; p++) *q++ = *p;
		p += dn->len;
		for (size_t i = 0; subto[i]; i++) *q++ = subto[i];
		if (dn->next) { dn = dn->next; goto loop; }
		for (; *p; p++)*q++ = *p;
		*q = 0;
		DnodesRelease(dn);
		return ret;
	}
	else
		return StrHeap(dest);
}

// throw: for ReplaceChars
char* StrHeapInsertThrow(const char* d, const char* s, size_t posi, size_t thrown)
{
	char* ret;
	char* ptr;
	char c;
	size_t dlen, slen;
	size_t i;// for loops below

	dlen = 0; slen = 0;// suitable for VC++2010
	while (s[slen])slen++;// better than strlen stdfunc.
	while (d[dlen])dlen++;
	if (!slen || !dlen || posi + thrown > dlen)
	{
		ret = StrHeap(d);
	}
	// 123456789 throw 56 for ab
	// 1234ab789 posi=4 throw=2 (at most 5) dlen=9 slen=2 
	ret = (char*)malc(dlen + slen - thrown + 1);
	ptr = ret;
	for (i = 0; i < posi; i++)
		*ptr++ = d[i];
	for (i = 0; i < slen; i++)
		*ptr++ = s[i];
	for (i = posi + thrown; i < dlen; i++)
		*ptr++ = d[i];
	*ptr = 0;// 0-TChars
	return ret;
}// use strcat is a waste.

char* instoa(ptrdiff_t num)
{
	#ifndef _ARN_FLAG_DISABLE
	size_t numlen = aflag.Signed + 1;
	#else
	size_t numlen = 2;
	#endif
	size_t numslv = num > 0 ? num : -num;
	AddDecimalDigitsLen(numlen, numslv);
	#ifndef _ARN_FLAG_DISABLE
	if (numlen > malc_limit)
	{
		///malc_occupy = 0;
		arna_eflag.Failure = 1;
		call_state = 1;
		return 0;
	}
	#endif
	//
	char* buf = salc(numlen);
	char* p = buf + numlen - 2;
	#ifndef _ARN_FLAG_DISABLE
	if (aflag.Signed)
	#endif
		*buf = num < 0 ? '-' : '+';
	numslv = num > 0 ? num : -num;
	do
	{
		*p-- = (numslv % 10) + '0';
		numslv /= 10;
	} while (numslv);
	#ifndef _ARN_FLAG_DISABLE
	aflag.PrecLoss = 0;
	aflag.Failure = 0;
	aflag.Sign = num < 0;
	///malc_occupy = numlen;
	#endif
	return buf;
}


