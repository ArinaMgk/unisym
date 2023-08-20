// ASCII TAB4 C99 ArnAssume
// Operations for string of memory, as the complement of ISO/IEC standard.
/*
* CAUTION! The codes was written by Arina lonely, except some codes with comments that express the codes from others, however, the exception is greatly changed. So, do not take it for granted that the codes is completely correct. If the unexpected appears after you use unisym, nobody shall help you!
* Personal-style symbol according to Arina's bad habits: _dbg, malc_count, malc_limit, _noheap, ...
*/
// E X P O S T U ** DO NOT TRAP IN C TOO MUCH! BE YOURSELF. ** L A T I O N //
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
/* Linkage
 * o malc_count yo [_dbg], numof area allocated in memory.
 * i malc_limit, yo ![_noheap],the maximum permission for heap allocated or sizeof buffer.
 * o malc_occupy. Zero means failure(heap unit size over or precise loss occur) or nothing for writing.
 * x arna_eflag {(Permit)PrecLoss[io], HeapYo[io], Signed[i], ARN_DIGIT[]}
 * - arna_LupRange {L, M}
 * i arna_precise
*/

// If you use for bootstrap programs or MCU developing, _noheap may be defined.

#ifndef ModUnisymString// ---- ---- ---- ----
#define ModUnisymString

#ifndef _INC_STDDEF
#include <stddef.h>
#endif

#ifndef _noheap
#include "aldbg.h"
#endif

#ifndef _noheap
extern size_t malc_limit;// the maximum for an allocation. 0 for disable any, but if you do not define _noheap, the relative codes and data will be compiled.
	#ifdef _dbg
		extern size_t malc_count;
	#endif
	#ifndef _INC_STDLIB
		#include <stdlib.h>
	#endif
#endif
;// format form of codes for VSCode
extern size_t malc_occupy;
extern size_t arna_precise;

extern char arna_tempor[];// as result
extern char arna_tmpslv[];// Do not use as result, same size with tempor.
extern char arna_tmpext[];

struct ArinaeFlag
{
	unsigned PrecLoss : 1, HeapYo : 1, Signed : 1, ARN_DIGIT : 1;
};extern struct ArinaeFlag arna_eflag;

// arna_LupRange for CoeAr and other advanced form.

//---- ---- ---- ---- dnode ---- ---- ---- ----
#ifndef _noheap
#ifndef ModDnode // double-directions double-order-mode node
#define ModDnode

typedef struct Dnode
{
	struct Dnode* left;// lower address
	char* addr;// for order
	union { size_t len, type; };// non-order
	union { struct Dnode* next, * right; };// higher address
} Dnode;

Dnode* DnodeCreate(Dnode* any, char* addr, size_t len);
Dnode* DnodeRewind(Dnode* any);
size_t DnodeCount(Dnode* any);
void DnodeRelease(Dnode* some);
void DnodesRelease(Dnode* first);
#endif
#endif // endof _noheap

//---- ---- ---- ---- string ---- ---- ---- ----
#ifndef ModString
#define ModString

typedef enum TokType
{
	tok_EOF = 0,// -1 or 0
	tok_any,
	tok_string,// "Hallo"
	tok_comment,// //
	tok_directive,// #include
	tok_number,// 1
	tok_sym,// +-*/
	tok_iden,// iden
	tok_space,// ' ' or \t or excluding new-line
	tok_else,
} TokType;

typedef struct Toknode
{
	// struct Dnode;
	struct Toknode* left;
	union { char* addr; size_t index; };
	union { size_t len; TokType type; };
	union { struct Toknode* next, * right; };
	// [ L | ADDR & LEN/TYPE | R ]
	size_t row, col;
} Toknode, Tode;



static inline char* MemRelative(char* addr, size_t width, ptrdiff_t times)
{
	register size_t i = 0;
	if (width == 0 || times == 0) return addr;
	if (times < 0) while (i < width) { *(addr + i + times) = addr[i]; i++; }
	else while (i < width) { addr[width + times - i - 1] = addr[width - i - 1]; i++; }
	return addr + times;
}/* Update: Arina fixed REC06 */

static inline char* MemAbsolute(char* dest, const char* sors, size_t width)
{
	register size_t i = 0;
	if (dest < sors) while (i < width) { dest[i] = sors[i]; i++; }
	else while (i < width) { dest[width - 1 - i] = sors[width - 1 - i]; i++; }
	return dest;
}/* Update: Arina fixed REC06 */

static inline char* StrCopy(char* dest, const char* sors)
{
	register char* d = dest;
	while (*d++ = *sors++); return dest;
}

static inline void* MemSet(void* s, int c, size_t n)
{
	while (n) { n--; ((char*)s)[n] = (char)c; }
	return s;
}

static inline char* MemCopyN(char* dest, const char* sors, size_t n)
{
	register char* d = dest;
	while (n--) *d++ = *sors++;
	return dest;
}

static inline char* StrCopyN(char* dest, const char* sors, size_t n)
{
	register char* d = dest;
	if (!sors)goto endo;
	while (n && (*d++ = *sors++)) n--;
	*d = 0;
endo:
	return dest;
}

static inline char* StrAppend(char* dest, const char* sors)
{
	register char* d = dest;
	while (*d) { d++; };
	while (*d++ = *sors++);
	return dest;
}

static inline char* StrAppendN(char* dest, const char* sors, size_t n)
{
	register char* d = dest;
	while (*d) { d++; };
	while (n && (*d++ = *sors++)) n--;
	if (!n) *d = 0;
	return dest;
}

/* COMPARE */

static inline int MemCompare(const char* a, const char* b, size_t n)
{
	register char tmp = 0;
	while (n && !(tmp = (*a - *b))) n--; return (int)tmp;
}

static inline int StrCompare(const char* a, const char* b)
{
	int tmp = 0;
	while (!(tmp = (*a - *b)) && *a++ && *b++); return tmp;
}

static inline int StrCompareN(const char* a, const char* b, size_t n)
{
	int tmp; tmp = 0;// MSVC' red tape
	while (n && !(tmp = (*a - *b)) && *a++ && *b++) n--; return tmp;
}

/**/

static inline size_t StrLength(const char* s)
{
	if (!s)return 0;
	register const char* offset = s;
	while (*offset) { offset++; }
	return offset - s;
}

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

static inline char StrCharLast(const char* s)//= StrElement(s, -1)
{
	if (!s)return 0;
	while (*s) s++;
	return *(s-1);
}

static inline const char* MemIndexByte(const char* s, int c, size_t n)
{
	while (n--) { if (*s == c) return s; else s++; } return NULL;
}

static inline const char* StrIndexChar(const char* s, int c)
{
	register char tmp;
	do if ((tmp = *s) == c) return s; else s++; while (tmp);
	return NULL;
}

static inline const char* StrIndexCharRight(const char* s, int c)
{
	register char tmp; const char* res = 0;
	do if ((tmp = *s) == c) res = s++; else s++; while (tmp);
	return res;
}

static inline size_t StrLenSameChar(const char* str, int c, const char** ret)
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

static inline const char* StrIndexString(const char* dest, const char* sub)
{
	register size_t len;
	while (*dest) { len = 0; while ((sub[len] == dest[len]) && (sub[len])) len++; if (!sub[len]) return dest; dest++; }
	return NULL;
}

static inline size_t StrSpanInclude(const char* s1, const char* s2)
{
	size_t res = 0; register size_t offs; register char c;
	while (c = *s1++) { offs = 0; while ((s2[offs]) && (s2[offs] != c)) offs++; if (!s2[offs]) return res; res++; }
	return res;
}

static inline size_t StrSpanExclude(const char* s, const char* reject)
{
	size_t ret = 0;
	register const char* ecx; char TmpChar, TmpElement;
	while (TmpChar = *s++) { ecx = reject; while (TmpElement = *ecx++) if (TmpElement == TmpChar)   return ret--; ret++; }
	return ret;
}

static inline const char* StrIndexChars(const char* s1, const char* s2)
{
	register char c; size_t offs;
	while (c = *s1) { offs = 0; while (s2[offs]) if (s2[offs++] == c) return s1; s1++; } return NULL;
}

void StrFilterOut(char* p, char c);
void StrFilter(char* p, enum TokType tt);//TODO. no considering new-line
void StrFilterString(char* p, const char* needs);
void StrFilterOutString(char* p, const char* neednot);

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

#ifndef _noheap

	#ifdef ModDnode



	Toknode* StrTokenAll(int (*getnext)(void), void (*seekback)(ptrdiff_t chars), char* buffer);
	void StrTokenClearAll(Toknode* tstr);
	void StrTokenThrow(Toknode* one);// a b c --> a c
	inline static Toknode* StrTokenBind(Toknode* left, Toknode* mid, Toknode* right)
	{
		if (left)
			left->next = mid;
		if (mid)
			mid->next = right;
		if (right)
			right->left = mid;
		if (mid)
			mid->left = left;
	}

	///inline static void StrTokenPrint(Toknode* first)
	///{
	#define StrTokenPrint(first)\
		printf("Token: [R %llu,C %llu][%s] %s\n", first->row, first->col,\
			((const char* []){"END", "ANY", "STR", "CMT", "DIR", "NUM", "SYM", "IDN", "SPC", "ELS"})\
			[first->type], first->type == tok_space ? "" : first->addr);
	///}
	
	// For the string of the Tode.
	///inline static void StrTokenPrintAll(Toknode* first)
	///{
	#define StrTokenPrintAll(first)\
		do StrTokenPrint(first);\
		while (first = first->next);
	///}
	
	#endif

char* StrHeap(const char* valit_str);
inline static char* StrHeapFromChar(char c)// Convert char in string in heap
{
	char* ptr;
	memalloc(ptr, 2);
	if (!ptr)return 0;
	*ptr = c;
	ptr[1] = 0;
	return ptr;
}
void* MemHeap(const void* sors, size_t byteof);
char* StrHeapN(const char* valit_str, size_t strlen);
char* StrHeapAppend(const char* dest, const char* sors);
char* StrHeapAppendN(const char* dest, const char* sors, size_t n);
char* StrReplaceHeap(const char* dest, const char* subfirstrom, const char* subto, size_t* times);
char* StrHeapInsertThrow(const char* d, const char* s, size_t posi, size_t thrown);
// posi_start and later positions of string move into the lower endian RFE02:16
inline static void StrSubWithdraw(char* posi_start, size_t len)
{
	MemRelative(posi_start, StrLength(posi_start) + 1, -(ptrdiff_t)len);
}

/* the previous strpool */
/* ROUTINE EXAMPLE
	StrPoolInit();
	atexit(StrPoolRelease);
	printf("%s", str = StrPoolHeap("Hello,world!"));
*/
#define _ModString_Strpool_UNIT_SIZE 4096// A page align by default

void StrPoolInit();
char* StrPoolHeap(const char*, size_t);
char* StrPoolAlloc(size_t);
char* StrPoolAllocZero(size_t);
void StrPoolRelease();

#endif

//---- ---- ---- ---- chrar part ---- ---- ---- ----
// Having brewed about 2 years, the design has matured and new non-destructive schemes have been tried and conceived. This, after 2023 (included), with the serived & advanced structures, also BCD-Arith., will be stopped updated. However, these are also a good tool for us.

// failure: either *buffer=0 or heap=0
// result for buffer in arna_tempor
#ifndef _noheap
char* instoa(ptrdiff_t num);// [Instant to ASCII yo heap]
#endif
char* instob(ptrdiff_t num, char* buf);// [Instant to ASCII yo buffer]
ptrdiff_t atoins(const char* str);// [ASCII to Instant]

char* ChrHexToDec(const char* hex);
// Output: upper case
char* ChrDecToHex(char* dec);

void ChrCpz(char* str);// Clear prefix zeros, "+001"-->"+1".
char* ChrAdd(const char* dest, const char* sors);
char* ChrSub(const char* dest, const char* sors);
char* ChrMul(const char* a, const char* b);
void ChrDiv(char* a, char* b);// return a as Quotient, b as remainder.
int ChrCmp(const char* a, const char* b);// -1 0 1
char* ChrFactorial(const char* a);// a has prefix '+'

void DigInc(int ascii, char* posi);
void DigDec(int ascii, char* posi);

char* ChrArrange(const char* total, const char* items);
char* ChrCombinate(const char* total, const char* items);
char* ChrComDiv(const char* op1, const char* op2);// [Get Greatest Common Divisor]
char* ChrComMul(const char* op1, const char* op2);// [Get Least Common Multiple]

// RegAr used, temporarily set here

// Boundary
static inline unsigned char StrShiftLeft4(unsigned char* s, size_t len)
{
	unsigned char carry;
	unsigned char lastc = 0;
	do
	{
		carry = (*s & 0xF0) >> 4;// ArinaCove
		*s <<= 4;
		*s++ |= lastc;
		lastc = carry;
	} while (--len);
	return lastc;
}

// Boundary
static inline void StrShiftLeft8n(unsigned char* s, size_t len, size_t n)
{
	///MemRelative(s, len, (ptrdiff_t)n);
	register size_t i = 0;
	if (len == 0 || n == 0) return;
	if (n >= len) MemSet(s, 0, len);
	else
	{
		while (i < len) { if (i + 1 > n) s[len + n - i - 1] = s[len - i - 1]; i++; }
		for (size_t j = 0; j < n; j++) s[j] = 0;
	}
	return;
}

// Boundary
static inline unsigned char StrShiftRight4(unsigned char* s, size_t len)
{
	unsigned char carry;
	unsigned char lastc = 0;
	s += len - 1;
	do
	{
		carry = (*s & 0x0F) << 4;// ArinaCove
		*s >>= 4;
		*s-- |= lastc;
		lastc = carry;
	} while (--len);
	return lastc;
}

// Boundary
static inline void StrShiftRight8n(unsigned char* s, size_t len, size_t n)
{
	///MemRelative(s, len, -(ptrdiff_t)n);
	register size_t i = 0;
	if (len == 0 || n == 0) return;
	if (n >= len) MemSet(s, 0, len);
	else
	{
		while (i < len) { if (i >= n) *(s + i - n) = s[i]; i++; }
		for (size_t j = 0; j < n; j++) (s - n + len)[j] = 0;
	}
}

static inline signed MemCompareRight(const unsigned char* a, const unsigned char* b, size_t n)
{
	for (ptrdiff_t i = n - 1; i >= 0; i--)
	{
		int state = (unsigned int)a[i] - (unsigned int)b[i];
		if (state) return state;
	}
	return 0;
}

#endif

#endif// ---- ---- ModUnisymString ---- ----

// IN MEMORY OF THE PAST YEARS //
// History in `ustring.c`
