// ASCII TAB4 C99 ArnAssume
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

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>// na isXType()
#include "../alice.h"
#include "../aldbg.h"
#include "../ustring.h"

static char* ptr_tempmas = arna_tempor;
static char* ptr_tempslv = arna_tmpslv;
static char* ptr_tempext = arna_tmpext;

#define erro(x) ((void)(x))// CHEAT

// It may be better to use Regular Expression
const char static EscSeq[] = 
{
	'n','\n','r','\r','a','\a','b','\b','f','\f','t','\t','v','\v'
};// other equal to literal char except the "\x"[0~2] "\"[0~2]

struct ArinaeFlag arna_eflag = {0};
//---- ---- ---- ---- dnode ---- ---- ---- ----
#ifndef _noheap
Dnode* DnodeCreate(Dnode* any, char* addr, size_t len)
{
	Dnode* crt = any;
	if (!crt)
	{
		crt = (Dnode*)malloc(sizeof(Dnode));
		#ifdef _dbg
			malc_count++;
		#endif
		crt->left = 0;
		crt->next = 0;
		crt->addr = addr;
		crt->len = len;
		return crt;
	}
	Dnode* NEW = (Dnode*)malloc(sizeof(Dnode));
	#ifdef _dbg
		malc_count++;
	#endif
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
	free(some);
	#ifdef _dbg
		malc_count--;
	#endif
}

void DnodesRelease(Dnode* first)
{
	// Depend on stack
	if (first->next) DnodesRelease(first->next);
	free(first);
	#ifdef _dbg
		malc_count--;
	#endif
}
#endif

//---- ---- ---- ---- string ---- ---- ---- ----
#if 1// convenient for reading codes
void StrFilterOut(char* p, char c)
{
	char* q = p, chr;
	for (; (chr = *p) != 0; p++)
		if (chr != c)
			*q++ = chr;
	*q = 0;
}

void StrFilter(char* p, enum TokType tt)
{
	char* q = p;
	char c;
	int (*jdg)(int);
	switch (tt)
	{
	case tok_space: jdg = isspace; break;
	case tok_number: jdg = isdigit; break;
	// ...
	default: return;
	}
	for (; (c = *p) != 0; p++)
		if (jdg(c))
			*q++ = c;
	*q = 0;
}

void StrFilterString(char* p, const char* needs)
{
	char* q = p;
	char c;
	int (*jdg)(int);
	for (; (c = *p) != 0; p++)
		if (StrIndexChar(needs, c))
			*q++ = c;
	*q = 0;
}

void StrFilterOutString(char* p, const char* neednot)
{
	char* q = p;
	char c;
	int (*jdg)(int);
	for (; (c = *p) != 0; p++)
		if (!StrIndexChar(neednot, c))
			*q++ = c;
	*q = 0;
}
#endif
//---- ---- ---- ---- HEAP FUNCS ---- ---- ---- ----
#ifndef _noheap

#ifdef ModDnode
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

void StrTokenClear(Toknode* token_in_chain)
{
	Toknode* crt = token_in_chain;
	if (!token_in_chain) return;
	while (crt->left) crt = crt->left;
	memalloc(crt->left, 4);// Use time to get better struct
	do
	{
		memfree(crt->left);
		if (crt->addr)
			memfree(crt->addr);
	} while (crt->next && (crt = crt->next));
	memfree(crt);
}

void StrTokenThrow(Toknode* one)
{
	if (one->left) one->left->next = one->next;
	if (one->next) one->next->left = one->left;
	memfree(one->addr);
	memfree(one);
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
#endif
//	//---- ---- ---- ---- ---- ---- ---- ----

char* StrHeap(const char* valit_str)
{
	size_t strlen = StrLength(valit_str);
	char* ret = zalloc(strlen + 1);
	StrCopy(ret, valit_str);
	#ifdef _dbg
		malc_count++;
	#endif
	return ret;
}

void* MemHeap(const void* sors, size_t byteof)
{
	char* ret = malloc(byteof);
	for (size_t i = 0; i < byteof; i++) ret[i] = ((const char*)sors)[i];
	#ifdef _dbg
		malc_count++;
	#endif
	return ret;
}

char* StrHeapN(const char* valit_str, size_t strlen)
{
	char* ret = zalloc(strlen + 1);
	StrCopyN(ret, valit_str, strlen);
	#ifdef _dbg
		malc_count++;
	#endif
	return ret;
}

char* StrHeapAppend(const char* dest, const char* sors)
{
	size_t dlen = StrLength(dest);
	size_t slen = StrLength(sors);
	char* ret = (char*)malloc(dlen + slen + 1);
	StrCopy(ret, dest);
	StrCopy(ret + dlen, sors);
	ret[dlen + slen] = 0;
	#ifdef _dbg
		malc_count++;
	#endif
	return ret;
}

char* StrHeapAppendN(const char* dest, const char* sors, size_t n)
{
	size_t dlen = StrLength(dest);
	char* ret = (char*)malloc(dlen + n + 1);
	StrCopy(ret, dest);
	StrCopyN(ret + dlen, sors, n);
	#ifdef _dbg
		malc_count++;
	#endif
	return ret;
}

// From left, one by one, return all in heap! must free() by yourself. Inputs won't be free!
char* StrReplaceHeap(const char* dest, const char* subfirstrom, const char* subto, size_t* times)
{
	#ifdef _dbg
		malc_count++;
	#endif
	if (times) *times = 0;
	if (!dest || !subfirstrom || !subto || !malc_limit)
	{
		return zalloc(1);
	}
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
		char* ret = malloc((size_t)((ptrdiff_t)sz_len + (ptrdiff_t)chars_add + (ptrdiff_t)1));
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
	ret = (char*)malloc(dlen + slen - thrown + 1);
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

#endif

//---- ---- ---- ---- STRPOOL ---- ---- ---- ----
#ifndef _noheap
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

#endif

//---- ---- ---- ---- ChrAr ---- ---- ---- ----
#define AddDecimalDigitsLen(i,num) do{(i)++;(num)/=10;}while(num)
size_t size_dec = 0; static void size_dec_get()
{
	register unsigned int i = 0;
	register size_t r = (size_t)~0;
	while (r)
		i++, r /= 10;
	//x64 for 20, and x86 for 10
	size_dec = i;
}
#ifndef _noheap
char* instoa(ptrdiff_t num)
{
	size_t numlen = !!arna_eflag.Signed + 1;
	size_t numslv = num > 0 ? num : -num;
	AddDecimalDigitsLen(numlen, numslv);
	if (numlen > malc_limit)
	{
		malc_occupy = 0;
		arna_eflag.PrecLoss = 1;
		return 0;
	}
	char* buf; memalloc(buf, numlen);

	char* p = buf + numlen - 2;
	if(arna_eflag.Signed) (*buf = num < 0 ? '-' : '+');
	buf[numlen - 1] = 0;

	numslv = num > 0 ? num : -num;
	do
	{
		*p-- = (numslv % 10) + '0';
		numslv /= 10;
	} while (numslv);
	arna_eflag.PrecLoss = 0;
	arna_eflag.HeapYo = 1;
	malc_occupy = numlen;
	return buf;
}
#endif

char* instob(ptrdiff_t num, char* buf)
{
	// Parallel: instoa()
	size_t numlen = !!arna_eflag.Signed + 1;
	size_t numslv = num > 0 ? num : -num;
	AddDecimalDigitsLen(numlen, numslv);
	if (malc_limit < numlen || malc_limit < 3 - !arna_eflag.Signed || !buf)
	{
		malc_occupy = 0;
		arna_eflag.PrecLoss = 1;
		return 0;
	}
	
	char* p = buf + numlen - 2;
	if(arna_eflag.Signed) (*buf = num < 0 ? '-' : '+');
	buf[numlen - 1] = 0;

	numslv = num > 0 ? num : -num;
	do
	{
		*p-- = (numslv % 10) + '0';
		numslv /= 10;
	} while (numslv);
	arna_eflag.PrecLoss = 0;
	arna_eflag.HeapYo = 0;
	malc_occupy = numlen;
	return buf;
}

ptrdiff_t atoins(const char* str)
{
	int sign = 0;
	if (!str || !*str)return 0;
	if (*str == '-') sign++, str++;
	else if (*str == '+') str++;// assert(arna_eflag.Signed) tolerated.
	if (!size_dec) size_dec_get();
	const char* ptr = str;
	size_t inst = 0;
	while (*ptr && *ptr <= '9' && *ptr >= '0' && ptr - str + !arna_eflag.Signed <= size_dec)
	{
		inst *= 10;
		inst += *ptr - '0';
		ptr++;
	}
	if (*ptr <= '9' && *ptr >= '0')
	{
		arna_eflag.PrecLoss = 1;
		return ~0;
	}
	arna_eflag.PrecLoss = 0;
	return sign ? -(ptrdiff_t)inst : (ptrdiff_t)inst;
}

#ifndef _noheap
// Besides, SLV using
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
char* ChrHexToDec(const char* hex)// slv using
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

char* ChrDecToHex(char* dec)// Output: upper case
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
	malc_occupy = declen2 + 1 + 1;
	arna_eflag.PrecLoss = 0;
	return dec - !(!arna_eflag.Signed);
}

// Clear prefix zeros of hexa. E.g. "00012500" >>> "12500"
void ChrCpz(char* str)
{
	if (arna_eflag.Signed) str++;// TRUST YOU~ NO ASSERT
	size_t siz = StrLength(str);
	size_t num = 0;
	char c;
	while ((c = str[num]) && c == '0') num++;
	if (!num) return;
	if (c == 0 && str[num - 1] == '0')
	{
		*str = '0';
		str[1] = 0;
	}
	else MemRelative(str + num, siz - num + 1, -(ptrdiff_t)num);
	// 000905$
	// *  *  * 
}

// Besides MAS: None Using
char* ChrAdd(const char* a, const char* b)
{
	// may xchg to make length a >= b
	if (!a || !b || !*a || !*b)
	{
		malc_occupy = 0;
		arna_eflag.PrecLoss = 1;
		return 0;
	}
	char CF = 0, mask;// {carry_flag`sign_b`sign_a}
	if (arna_eflag.Signed)
	{
		CF |= (*a == '-');
		CF |= (*b == '-') << 1;
	}
	if (arna_eflag.Signed)
	if (CF & 0b01 && !(CF & 0b10))// -a +b
	{
		size_t templen = StrLength(a) + 1;
		if(templen > malc_limit)
		{
			malc_occupy = 0;
			arna_eflag.PrecLoss = 1;
			return 0;
		}
		char* temp = arna_eflag.HeapYo ? StrCopy(malc(templen), a) : StrCopy(ptr_tempslv, a);
		*temp = '+';
		if (arna_eflag.HeapYo)
		{
			srs(temp, ChrSub(b, temp));
		}
		else
			ChrSub(b, temp);// --> mas_tmp
		return temp;
	}
	else if (CF & 0b10 && !(CF & 0b01))// +a -b 
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
		mask = ~0b100;
		CF &= mask;
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
	if(arna_eflag.HeapYo) memfree(buf_sub_fix);
	ChrCpz(a - (arna_eflag.Signed ? 1 : 0));
	arna_eflag.PrecLoss = 0;
	return;
}

int ChrCmp(const char* a, const char* b)
{
	int sga = 0, sgb = 0;
	const char* endofa = a, * endofb = b;
	int greater = 1;
	if (!StrCompare(a, b)) return 0;
	for (; *endofa; endofa++); endofa--;
	for (; *endofb; endofb++); endofb--;
	if (*a == '+')a++;
	if (*b == '+')b++;
	if (*a == '-') { a++; sga = 1; }
	if (*b == '-') { b++, sgb = 1; }
	if (*a == '0' && *b == '0') return 0;// more effective
	if (endofa - a == endofb - b) greater = (StrCompare(a, b)) > 0;
	else greater = endofa - a > endofb - b;
	if (sga ^ sgb) return sga ? -1 : 1;
	return ((!sga) ^ (!greater)) ? 1 : -1;
}

static size_t ChrFactorialFixStack(size_t x)
{
	return x <= 1 ? 1 : x * ChrFactorialFixStack(x - 1);
}
// limit should:>= 3
// Besides MAS: Nothing using, NO for occupy for heap
char* ChrFactorial(const char* a)
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

void DigInc(int ascii, char* posi)
{
	if ((ascii = *posi += ascii -= '0') <= '9') return;
	do *posi-- = ascii - 10; while ((ascii = *posi + 1) > '9'); (*posi)++;
}

void DigDec(int ascii, char* posi)
{
	if ((ascii = *posi -= ascii -= '0') >= '0') return;
	do *posi-- = ascii + 10; while ((ascii = *posi - 1) < '0'); (*posi)--;
}

static size_t ChrArrangeFixStack(size_t t, size_t i)// A_t(i)
{
	return i == 0 ? 1 :
		t * ChrArrangeFixStack(t - 1, i - 1);
}
// limit should:>= 3
char* ChrArrange(const char* total, const char* items)// Amn
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
char* ChrCombinate(const char* total, const char* items)
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
char* ChrComDiv(const char* op1, const char* op2)
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
char* ChrComMul(const char* op1, const char* op2)
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

#endif


//---- ---- ---- ---- .HISTORY ---- ---- ---- ----
// typedef struct LinearPropToken
// {
//  char* tok_content;// addr
//  size_t toklen;// canceled
//  struct LinearPropToken* left, * right;// left, next
//  enum TokType toktype;// len
// } tokprep;
//
// RFR18 Append Dnode into ustring
// RFT02 Append ChrAr into ustring
// RFT15 Decide to make basic Wiki in current STD-C style before history; Auto adapt lower or upper letters and sign symbol; New demonstrations. The next day, uploaded.

