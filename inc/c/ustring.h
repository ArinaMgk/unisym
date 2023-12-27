// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant
// LastCheck: RFZ18
// AllAuthor: @dosconio
// ModuTitle: Operations for ASCIZ Character-based String
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

// E X P O S T U ** DO NOT TRAP IN C TOO MUCH! BE YOURSELF(Practical). ** L A T I O N //
/* {} Component
* common heap operations
* strpool
* Buf-special part
* General String Function
* ChrAr
* Temporarily Stage Area
*/

/*
* astring, aka Allocating String, used name `hstring` (heap) ;
* bstring, aka Buffer String, keep the functions can run in Free-standing Environment
* * If a function is in bstring, it must has at least one astring version
* If a function is excluded from bstring, that is the general function, as a part of ustring.
* ! cautious 'a' and 'b' affix.
*/

#if !defined(_INC_USTRING)
#define _INC_USTRING
#define _LIB_STRING

#include "host.h"
#include "aldbg.h"
#include "uctype.h"
#include <string.h>//{TODO} STD LIB

typedef enum _token_t
{
	tok_EOF = 0,  // 
	tok_any,      // for features of any token
	tok_comment,  // /* */ #
	tok_direct,// directive like #include
	tok_symbol,   // +-*/ ...
	tok_spaces,   // ' ' or \t or excluding new-line
	tok_others,   // above are usually not the entity

	tok_identy,   // identifier
	tok_string,   // "Hallo"
	tok_number,   // 1
} toktype;// the counts should not be greater than 15.

#ifndef _INC_TNODE
#include "tnode.h"
#endif

//---- ---- ---- ---- { TODO } ---- ---- ---- ----
#define _INC_USTRING_INLINE// delete this after TODO


//
#define StrCompareLocale strcoll

//
#define StrCopyLocale strxfrm

//
#define StrGetError strerror

//---- ---- ---- ---- { General String Function } ---- ---- ---- ----

// [ASTRING] Convert char in string in heap
// equivalent `StrHeapAppendChars("", c, 1)` ---> {TODO: auto omit null destination string}
char* StrHeapFromChar(char c);

// [ASTRING]
char* StrHeap(const char* valit_str);

// [ASTRING]
void* MemHeap(const void* sors, size_t bytelen);

// [ASTRING]
char* StrHeapN(const char* valit_str, size_t strlen);

// [ASTRING]
char* StrHeapAppend(const char* dest, const char* sors);

// [ASTRING]
char* StrHeapAppendN(const char* dest, const char* sors, size_t n);

// [ASTRING]
char* StrHeapAppendChars(char* dest, char chr, size_t n);

// [ASTRING]
char* StrReplace(const char* dest, const char* subfirstrom, const char* subto, size_t* times);

// [ASTRING]
char* StrHeapInsertThrow(const char* d, const char* s, size_t posi, size_t thrown);

// [ASTRING]
char* instoa(ptrdiff_t num);// [Instant to ASCII yo heap]

//
#ifdef _INC_USTRING_INLINE
static inline char* MemRelative(char* addr, size_t width, ptrdiff_t times)
{
	register size_t i = 0;
	if (width == 0 || times == 0) return addr;
	if (times < 0) while (i < width) { *(addr + i + times) = addr[i]; i++; }
	else while (i < width) { addr[width + times - i - 1] = addr[width - i - 1]; i++; }
	return addr + times;
}/* Update: Arina fixed REC06 */
#else
#define MemRelative(addr,width,times) memmove((addr)+(times), addr, width) ///{TODO}
#endif

#ifdef _INC_USTRING_INLINE
static inline char* MemAbsolute(char* dest, const char* sors, size_t width)
{
	register size_t i = 0;
	if (dest < sors) while (i < width) { dest[i] = sors[i]; i++; }
	else while (i < width) { dest[width - 1 - i] = sors[width - 1 - i]; i++; }
	return dest;
}/* Update: Arina fixed REC06 */
#else
#define MemAbsolute memmove ///{TODO}
#endif

#ifdef _INC_USTRING_INLINE
static inline char* StrCopy(char* dest, const char* sors)
{
	register char* d = dest;
	while (*d++ = *sors++); return dest;
}
#else
#define StrCopy strcpy ///{TODO}
#endif

#ifdef _INC_USTRING_INLINE
static inline void* MemSet(void* s, int c, size_t n)
{
	while (n) { n--; ((char*)s)[n] = (char)c; }
	return s;
}
#else
void* MemSet(void* s, int c, size_t n);
#endif

// RFB31 changed from `static inline char* MemCopyN(char* dest, const char* sors, size_t n)`
#ifdef _INC_USTRING_INLINE
static inline void* MemCopyN(void* dest, const void* sors, size_t n)
{
	register char* d = (char*)dest;
	while (n--) *d++ = *(*((const char**)&sors))++;
	return dest;
}
#else
#define MemCopyN memcpy ///{TODO}
#endif

// n excludes terminating-0
#ifdef _INC_USTRING_INLINE
static inline char* StrCopyN(char* dest, const char* sors, size_t n)
{
	register char* d = dest;
	if (!sors)goto endo;
	while (n && (*d++ = *sors++)) n--;
	*d = 0;
endo:
	return dest;
}
#else
#define StrCopyN strncpy ///{TODO}
#endif

#ifdef _INC_USTRING_INLINE
static inline char* StrAppend(char* dest, const char* sors)
{
	register char* d = dest;
	while (*d) { d++; };
	while (*d++ = *sors++);
	return dest;
}
#else
#define StrAppend strcat ///{TODO}
#endif

#ifdef _INC_USTRING_INLINE
static inline char* StrAppendN(char* dest, const char* sors, size_t n)
{
	register char* d = dest;
	while (*d) { d++; };
	while (n && (*d++ = *sors++)) n--;
	if (!n) *d = 0;
	return dest;
}
#else
#define StrAppendN strncat ///{TODO}
#endif

#ifdef _INC_USTRING_INLINE
static inline char* StrAppendChars(char* dest, char chr, size_t n)
{
	register char* d = dest;
	while (*d) { d++; };
	while (n--) *d++ = chr;
	*d = 0;
	return dest;
}// RFV20
#else
///{TODO}
#endif

//---- ---- ---- ---- { Compare Function } ---- ---- ---- ----

#ifdef _INC_USTRING_INLINE
static inline int MemCompare(const char* a, const char* b, size_t n)
{
	register char tmp = 0;
	while (n && !(tmp = (*a - *b))) n--; return (int)tmp;
}
#else
#define MemCompare memcpy ///{TODO}
#endif

#ifdef _INC_USTRING_INLINE
static inline int StrCompare(const char* a, const char* b)
{
	int tmp = 0;
	while (!(tmp = (*a - *b)) && *a++ && *b++); return tmp;
}
#else
#define StrCompare strcmp ///{TODO}
#endif

///{TODO} 2 Ver
static inline int StrCompareInsensitive(const char* a, const char* b)// RFC12
{
	int tmp = 0;
	while (!(tmp = (ascii_tolower(*a) - ascii_tolower(*b))) && *a++ && *b++); return tmp;
}


//
#ifdef _INC_USTRING_INLINE
static inline int StrCompareN(const char* a, const char* b, size_t n)
{
	int tmp;
	while (n && !(tmp = (*a - *b)) && *a++ && *b++) n--; return tmp;
}
#else
#define StrCompareN strncmp ///{TODO}
#endif

///{TODO} 2 Ver
static inline int StrCompareNInsensitive(const char* a, const char* b, size_t n)// RFC12
{
	int tmp;
	while (n && !(tmp = (ascii_tolower(*a) - ascii_tolower(*b))) && *a++ && *b++) n--; return tmp;
}

// RFV12 Updated.
#ifdef _INC_USTRING_INLINE
static inline size_t StrLength(const char* s)
{
	// do not judge s zo null for better debug
	register size_t len = 0;
	while (s[len]) len++;
	return len;
}
#else
#define StrLength strlen ///{TODO}
#endif

//{TODO} 2 Ver
static inline char* StrElement(char* s, ptrdiff_t idx)
{
	// a  b  c  [\0]
	// +0 +1 +2 +3
	// -3 -2 -1
	if (!s || !*s)return 0;
	int sign = 0;
	size_t len = 0;
	while (s[len]) len++;
	if (idx > 0) if ((size_t)idx >= len) return 0;
	if (idx < 0) if ((size_t)(-idx) > len) return 0;
	return (idx < 0) ? s + len + idx : s + idx;
}

//{TODO} 2 Ver
static inline char StrCharLast(const char* s)//= *StrElement(s, -1)
{
	if (!s)return 0;
	while (*s) s++;
	return *(s-1);
}

//
#ifdef _INC_USTRING_INLINE
static inline const char* MemIndexByte(const char* s, int c, size_t n)
{
	while (n--) { if (*s == c) return s; else s++; } return NULL;
}
#else
#define MemIndexByte memchr ///{TODO}
#endif

#ifdef _INC_USTRING_INLINE
static inline const char* StrIndexChar(const char* s, int c)
{
	register char tmp;
	do if ((tmp = *s) == c) return s; else s++; while (tmp);
	return NULL;
}
#else
#define StrIndexChar strchr ///{TODO}
#endif

#ifdef _INC_USTRING_INLINE
static inline const char* StrIndexCharRight(const char* s, int c)
{
	register char tmp; const char* res = 0;
	do if ((tmp = *s) == c) res = s++; else s++; while (tmp);
	return res;
}
#else
#define StrIndexCharRight strrchr ///{TODO}
#endif

// RFV07 Rename from "StrLenSameChar"
#ifdef _INC_USTRING_INLINE
static inline size_t StrLengthSameChar(const char* str, int c, const char** ret)
{
	// subord of StrSpanInclude()
	const char* p = str;
	size_t len;
	while (1) if (*p == c)
	{
		len = 1;
		while (p[len] == c)len++;
		*ret = p;
		return len;
	}
	else if (!*p) break; else p++;
	*ret = 0;
	return 0;
}// A.R.I.N.A.
#else
///{TODO}
#endif

#ifdef _INC_USTRING_INLINE
static inline const char* StrIndexString(const char* dest, const char* sub)
{
	register size_t len;
	while (*dest) { len = 0; while ((sub[len] == dest[len]) && (sub[len])) len++; if (!sub[len]) return dest; dest++; }
	return NULL;
}
#else
#define StrIndexString strstr ///{TODO}
#endif

#ifdef _INC_USTRING_INLINE
static inline size_t StrSpanInclude(const char* s1, const char* s2)
{
	size_t res = 0; register size_t offs; register char c;
	while (c = *s1++) { offs = 0; while ((s2[offs]) && (s2[offs] != c)) offs++; if (!s2[offs]) return res; res++; }
	return res;
}
#else
#define StrSpanInclude strspn ///{TODO}
#endif

#ifdef _INC_USTRING_INLINE
static inline size_t StrSpanExclude(const char* s, const char* reject)
{
	size_t ret = 0;
	register const char* ecx; char TmpChar, TmpElement;
	while (TmpChar = *s++) { ecx = reject; while (TmpElement = *ecx++) if (TmpElement == TmpChar)   return ret--; ret++; }
	return ret;
}
#else
#define StrSpanExclude strcspn ///{TODO}
#endif

#ifdef _INC_USTRING_INLINE
static inline const char* StrIndexChars(const char* s1, const char* s2)
{
	register char c; size_t offs;
	while (c = *s1) { offs = 0; while (s2[offs]) if (s2[offs++] == c) return s1; s1++; } return NULL;
}
#else
#define StrIndexChars strpbrk ///{TODO}
#endif

//{TODO} 2 Ver
static inline const char* StrIndexCharsExcept(const char* s1, const char* s2)// RFV24
{
	register char c; 
	for (;c = *s1;s1++)
	{
		int state = 0;
		for (size_t offs = 0; s2[offs]; offs++)
			if (s2[offs] == c)
			{
				state = 1;
				break;
			}
		if (!state) return s1;
	}
	return NULL;
}

//{TODO} 2 Ver
static inline const char* StrIndexCharsRight(const char* s1, const char* s2)// RFV19
{
	register char c; size_t offs; const char* res = 0;
	while (c = *s1)
	{
		offs = 0; while (s2[offs]) if (s2[offs++] == c) res = s1; s1++;
	}
	return res;
}

//{TODO} static inline const char* StrIndexCharsExceptRight(const char* s1, const char* s2)
//

//
void StrFilterOut(char* p, char c);
//TODO. no considering new-line
void StrFilter(char* p, toktype tt);
//
void StrFilterString(char* p, const char* needs);
//
void StrFilterOutString(char* p, const char* neednot);
//
size_t StrDeprefixSpaces(char* str);
//
size_t StrDesuffixSpaces(char* str);

#ifdef _INC_USTRING_INLINE
static inline char* StrTokenOnce(char* s1, const char* s2)
{
	static char* _ModString_StrTokenOnce;// na StrToken, Arina updated yo RFT02.
	if (!s1) if (_ModString_StrTokenOnce) s1 = _ModString_StrTokenOnce; else return NULL;
	register size_t offs;
	/*Skip prefix*/ while (*s1) {
		offs = 0; while (s2[offs]) {
			if (s2[offs] == *s1) break; offs++;
		} if (!s2[offs]) break; s1++;
	}
	_ModString_StrTokenOnce = s1;
	while (*s1++)
	{
		offs = 0; while (s2[offs]) { if (s2[offs] == *s1) { *s1 = 0; s2 = _ModString_StrTokenOnce; _ModString_StrTokenOnce = s1 + 1; return (char*)s2; }; offs++; }
	} s2 = _ModString_StrTokenOnce;
	_ModString_StrTokenOnce = 0;
	return (char*)s2;
}
#else
#define StrTokenOnce strtok ///{TODO}
#endif

//---- ---- ---- ---- { Set and Sorting } ---- ---- ---- ----

// RFV30 GHC. Param:order[0:little>big 1:big>little]
//{TODO} 2 Ver
void StrSortBubble(char* str, int order);

// RFC12
//{TODO} 2 Ver
//{TODO} move to uctypes?
static inline char* StrToLower(char* str)
{
	char* p = str;
	while(*p) { *p = ascii_tolower(*p); p++; }
	return str;
}

// RFC12
//{TODO} 2 Ver
//{TODO} move to uctypes?
static inline char* StrToUpper(char* str)
{
	char* p = str;
	while (*p) { *p = ascii_toupper(*p); p++; }
	return str;
}

//{TODO} StrRelative and StrRelativeN
#define StrSubWithdraw(posi_start,len)\
	MemRelative((posi_start), StrLength(posi_start) + 1, -(ptrdiff_t)(len))

//
char* instob(ptrdiff_t num, char* buf);

//
ptrdiff_t atoins(const char* str);

//---- ---- ---- ---- { ChrAr } ---- ---- ---- ----
// Have been brewed since 2022 Aug.
// Chr+-*/ would not call each other without considering the aflaga.

// Clear prefix zeros, "+001"-->"+1".
size_t ChrCpz(char* str);

// Char-Arithmetic Cut Trailing zeros. Return the counts of chars that have been cut.
size_t ChrCtz(char* str);

//
char* _Need_free ChrHexToDec(const char* hex);

// Output: upper case
char* ChrDecToHex(char* dec);

//
char* _Need_free ChrHexToDecFloat(const char* hexf);

//
char* _Need_free ChrDecToHexFloat(const char* decf, size_t digits);

//
char* ChrAdd(const char* dest, const char* sors);

//
char* ChrSub(const char* dest, const char* sors);

//
char* ChrMul(const char* a, const char* b);

// return a as Quotient, b as remainder.
void ChrDiv(char* a, char* b);

// a has prefix '+'
char* ChrFactorial(const char* a);

//
char* ChrArrange(const char* total, const char* items);

//
char* ChrCombinate(const char* total, const char* items);

// [Get Greatest Common Divisor]
char* ChrComDiv(const char* op1, const char* op2);

// [Get Least Common Multiple]
char* ChrComMul(const char* op1, const char* op2);

char* ChrInsPow(const char* in, size_t times);

//---- ---- ---- ---- { ChrArBuf now aka ChrBr } ---- ---- ---- ----

#ifdef _LIB_STRING_BUFFER///{TODO}
char* _Need_free ChrHexToDecBuf(const char* hex);
char* ChrDecToHexBuf(char* dec);// Output: upper case
char* ChrAddBuf(const char* dest, const char* sors);
char* ChrSubBuf(const char* dest, const char* sors);
char* ChrMulBuf(const char* a, const char* b);
void ChrDivBuf(char* a, char* b);// return a as Quotient, b as remainder.
char* ChrFactorialBuf(const char* a);// a has prefix '+'
char* ChrArrangeBuf(const char* total, const char* items);
char* ChrCombinateBuf(const char* total, const char* items);
char* ChrComDivBuf(const char* op1, const char* op2);// [Get Greatest Common Divisor]
char* ChrComMulBuf(const char* op1, const char* op2);// [Get Least Common Multiple]
#endif

//---- ---- ---- ---- { ChrAr General } ---- ---- ---- ----

//
int ChrCmp(const char* a, const char* b);// -1 0 1

//
void DigInc(int ascii, char* posi);

//
void DigDec(int ascii, char* posi);

//---- ---- ---- ---- { Others or Temporarily Stage Area } ---- ---- ---- ----

// 9876.5432H <=== 0x32,0x54`0x76,0x98;
char* _Need_free bcdtoa(unsigned char* str, size_t bylen);

// 98765432H ===> 0x32,0x54,0x76,0x98;
unsigned char* _Need_free atobcd(char* str);

// Boundary
unsigned char StrShiftLeft4(void* s, size_t len);

// Boundary
void StrShiftLeft8n(void* s, size_t len, size_t n);

// Boundary
unsigned char StrShiftRight4(void* s, size_t len);

// Boundary, may renamed StrShiftRightBytes
void StrShiftRight8n(void* s, size_t len, size_t n);

// In the direction of the left
stdint MemCompareRight(const unsigned char* a, const unsigned char* b, size_t n);

#endif// !defined(_LIB_STRING) && !defined(_INC_USTRING)
// IN MEMORY OF OUR PAST YEARS //
