// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant
// LastCheck: RFZ18
// AllAuthor: @ArinaMgk; @dosconio
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

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wparentheses"
#endif

#include "stdinc.h"
#include "uctype.h"

#ifndef EOF
#define EOF (-1)
#endif

typedef enum string_layout_t {
	_string_zero_terminated = 0,
	_string_length_prefixed
} string_layout_t;

typedef enum _token_t
{
	tok_none = 0,  // special meaning, e.g. FAILURE
	tok_any,      // for features of any token
	//
	tok_symbol,   // +-*/ ...
	tok_spaces,   // ' ' or \t or excluding new-line
	tok_number,   // [multi-method] 1, 0x12, 23U, 0.1
	tok_identy,   // identifier
	//
	tok_others,   // above are usually not the entity
	//
	tok_comment,  // /* */ #
	tok_direct,// directive like #include
	tok_string,   // "Hallo"
	//
	tok_EOF // a placeholder, do not use it but can succeed since it.
} toktype;// the counts should not be greater than 15.
// phrase 0 : token into what is or above tok_others
// phrase 1 : comment, directive, string, ...


enum {
	tok_func = tok_EOF + 1,

	tok__continue
};

//#define isidnsym(type)(type<tok_others)
#define maysymbol(type)(type==tok_symbol)
//#define isentity(type)(type>tok_others)
#define maynotsym(type)(type>tok_symbol)


#ifndef _INC_TNODE
#include "dnode.h"
#endif

//---- ---- ---- ---- { TODO } ---- ---- ---- ----
#define _INC_USTRING_INLINE// delete this after TODO


//
#define StrCompareLocale strcoll

//
#define StrCopyLocale strxfrm

//
#define StrGetError strerror

#ifdef _INC_CPP
extern "C" {
#endif

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
// IO times: input pointer to maximum times to replace, output pointer to replaced times
char* StrReplace(const char* dest, const char* subfirstrom, const char* subto, size_t* times);

// [ASTRING]
char* StrHeapInsertThrow(const char* d, const char* s, size_t posi, size_t thrown);

// [ASTRING]
char* instoa(ptrdiff_t num);// [Instant to ASCII yo heap]

//
#ifdef _INC_USTRING_INLINE
static inline char* MemRelative(char* addr, size_t width, ptrdiff_t times)
{
	_REGISTER size_t i = 0;
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
	_REGISTER size_t i = 0;
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
	_REGISTER char* d = dest;
	while (*d++ = *sors++);
	return dest;
}
#else
#define StrCopy strcpy ///{TODO}
#endif

#ifdef _INC_USTRING_INLINE
static inline void* MemSet(void* d, int c, size_t n)
{
	while (n) ((char*)d)[--n] = (char)c;
	return d;
}
#else
void* MemSet(void* d, int c, size_t n);
#endif
#define MetSetof(arr,c) MemSet(arr,c,sizeof(arr))

// RFB31 changed from `static inline char* MemCopyN(char* dest, const char* sors, size_t n)`
#ifdef _INC_USTRING_INLINE
static inline void* MemCopyN(void* dest, const void* sors, size_t n)
{
	_REGISTER char* d = (char*)dest;
	while (n--) *d++ = *(*((const char**)&sors))++;
	return dest;
}
#else
#define MemCopyN memcpy ///{TODO}
#endif
#define MemCopySrc(dest,sors) MemCopyN(dest,sors,sizeof(*sors))

#ifdef _INC_USTRING_INLINE
// 20250329 make same with `strncpy`
// n includes terminating-0
static inline char* StrCopyN(char* dest, const char* sors, size_t n)
{
	_REGISTER char* d = dest;
	if (!sors)goto endo;
	while (n && (*d++ = *sors++)) n--;
	//d[n ? -1 : 0] = 0; // n excludes terminating-0
endo:
	return dest;
}
#else
#define StrCopyN strncpy ///{TODO}
#endif

#ifdef _INC_USTRING_INLINE
static inline char* StrAppend(char* dest, const char* sors)
{
	_REGISTER char* d = dest;
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
	_REGISTER char* d = dest;
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
	_REGISTER char* d = dest;
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
	_REGISTER int tmp = 0;
	while (n && !(tmp = (*a++ - *b++))) n--;
	return tmp;
}
#else
#define MemCompare memcmp ///{TODO}
#endif

static inline int MemCompareInsensitive(const char* a, const char* b, size_t n)
{
	_REGISTER int tmp = 0;
	while (n && !(tmp = (int)(ascii_tolower(*a++) - ascii_tolower(*b++)))) n--;
	return tmp;
}

#ifdef _INC_USTRING_INLINE
static inline int StrCompare(const char* a, const char* b)
{
	int tmp = 0;
	while (!(tmp = (*a - *b)) && *a++ && *b++);
	return tmp;
}
#else
#define StrCompare strcmp ///{TODO}
#endif

// aka stricmp
static inline int StrCompareInsensitive(const char* a, const char* b)// RFC12
{
	int tmp = 0;
	while (!(tmp = (int)(ascii_tolower(*a) - ascii_tolower(*b))) && *a++ && *b++);
	return tmp;
}


//
#ifdef _INC_USTRING_INLINE
static inline int StrCompareN(const char* a, const char* b, size_t n)
{
	int tmp = 0;
	while (n && !(tmp = (*a - *b)) && *a++ && *b++)
		n--;
	return tmp;
}
#else
#define StrCompareN strncmp ///{TODO}
#endif

///{TODO} 2 Ver
static inline int StrCompareNInsensitive(const char* a, const char* b, size_t n)// RFC12
{
	int tmp;
	while (n && !(tmp = (ascii_tolower(*a) - ascii_tolower(*b))) && *a++ && *b++)
		n--;
	return tmp;
}

// RFV12 Updated.
#ifdef _INC_USTRING_INLINE
static inline size_t StrLength(const char* s)
{
	// do not judge s zo null for better debug
	_REGISTER size_t len = 0;
	while (s[len])
		len++;
	return len;
}
#else
#endif


//{TODO} 2 Ver
static inline char* StrElement(char* s, ptrdiff_t idx)
{
	// a  b  c  [\0]
	// +0 +1 +2 +3
	// -3 -2 -1
	if (!s || !*s)return 0;
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
	while (n--) { if (*s == c) return s; else s++; }
	return NULL;
}
#else
#define MemIndexByte memchr ///{TODO}
#endif

#ifdef _INC_USTRING_INLINE
static inline const char* StrIndexChar(const char* s, int c)
{
	_REGISTER char tmp;
	do if ((tmp = *s) == c) return s; else s++; while (tmp);
	return NULL;
}
#else
#define StrIndexChar strchr ///{TODO}
#endif

#ifdef _INC_USTRING_INLINE
static inline const char* StrIndexCharRight(const char* s, int c)
{
	_REGISTER char tmp; const char* res = 0;
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
	_REGISTER size_t len;
	while (*dest) { len = 0; while ((sub[len] == dest[len]) && (sub[len])) len++; if (!sub[len]) return dest; dest++; }
	return NULL;
}
#else
#define StrIndexString strstr ///{TODO}
#endif

//{TODO} StrIndexStrings

static inline const char* StrIndexStringRight(const char* dest, const char* sub)
{
	const char* ret = 0;
	const char* ptr = 0;
	size_t destlen = StrLength(dest);
	size_t subslen = StrLength(sub);
	if (destlen < subslen) return 0;
	ptr = dest + destlen - subslen;
	while ((ptr >= dest) && !(ret = StrIndexString(ptr--, sub)));
	return ret;
}

#ifdef _INC_USTRING_INLINE
static inline size_t StrSpanInclude(const char* s1, const char* s2)
{
	size_t res = 0; _REGISTER size_t offs; _REGISTER char c;
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
	_REGISTER const char* ecx; char TmpChar, TmpElement;
	while (TmpChar = *s++) { ecx = reject; while (TmpElement = *ecx++) if (TmpElement == TmpChar)   return ret--; ret++; }
	return ret;
}
#else
#define StrSpanExclude strcspn ///{TODO}
#endif

#ifdef _INC_USTRING_INLINE
static inline const char* StrIndexChars(const char* s1, const char* s2)
{
	_REGISTER char c; size_t offs;
	while (c = *s1) { offs = 0; while (s2[offs]) if (s2[offs++] == c) return s1; s1++; } return NULL;
}
#else
#define StrIndexChars strpbrk ///{TODO}
#endif

//{TODO} 2 Ver
static inline const char* StrIndexCharsExcept(const char* s1, const char* s2)// RFV24
{
	_REGISTER char c; 
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
	_REGISTER char c; size_t offs; const char* res = 0;
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
	if (!s1 && !(s1 = _ModString_StrTokenOnce))
		return NULL;
	_REGISTER size_t offs;
	/*Skip prefix*/ while (*s1) {
		offs = 0; while (s2[offs]) {
			if (s2[offs] == *s1)
				break;
			offs++;
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

// We can use this to use Big Endian
inline static void MemReverse(char* str, stduint len) {
	for0(i, len >> 1) xchg(str[i], str[len - i - 1]);
}

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

// ---- ---- ---- ---- { Convert } ---- ---- ---- ----
#define restrict //{TEMP}
//
char* instob(ptrdiff_t num, char* buf);

//
ptrdiff_t atoins(const char* str);

//#define atoi _atoidefa
int _atoidefa(const char* inp);

//#define atol atolong
long int atolong(const char* inp);
//#define strtol StrTokenLong
long int StrTokenLong(const char* restrict inp, char** restrict endptr, int base);
//#define strtoul StrTokenULong
unsigned long int StrTokenULong(const char* restrict inp, char** restrict endptr, int base);

//{TODO} rename atodbl
//#define atof atoflt
double atoflt(const char* astr);
//#define StrTokenDouble strtod
double StrTokenDouble(const char* restrict inp, char** restrict endptr);
//#define StrTokenFloat strtof
float StrTokenFloat(const char* restrict inp, char** restrict endptr);
//#define StrTokenLDouble strtold
long double StrTokenLDouble(const char* restrict inp, char** restrict endptr);

#ifdef _BIT_SUPPORT_64
//#define atoll atollong
long long int atollong(const char* inp);
//#define strtoll StrTokenLLong
long long int StrTokenLLong(const char* restrict inp, char** restrict endptr, int base);
//#define strtoull StrTokenULLong
unsigned long long int StrTokenULLong(const char* restrict inp, char** restrict endptr, int base);
#endif

#undef restrict

//---- ---- ---- ---- { ChrAr } ---- ---- ---- ----
// Have been brewed since 2022 Aug.
// Chr+-*/ would not call each other without considering the aflaga.

size_t lookupDecimalDigits(size_t expo);
extern size_t _size_decimal_get();

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

// ---- ---- ---- ---- { Buffer } ---- ---- ---- ----

// like sprintf
int outsfmtbuf(char* buf, const char* fmt, ...);
// like vsprintf
int outsfmtlstbuf(char* buf, const char* fmt, para_list lst);

// get the length of the string that would be output by outsfmtbuf
// excluding the null - terminator.
// same as the StrLength(ret)
int outsfmtlstlen(const char* fmt, para_list lst);
int lensfmt(const char* fmt, ...);



// ---- ---- ---- ---- { Wide / Multi } ---- ---- ---- ----
_TODO//#define mbstowcs StrWideFoMulti
_TODO//#define wcstombs StrWideToMulti
_TODO//#define mbtowc   ChrWideFoMulti
_TODO//#define wctomb   ChrWideToMulti

#ifdef _INC_CPP
}
#endif

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#endif// !defined(_LIB_STRING) && !defined(_INC_USTRING)
// IN MEMORY OF OUR PAST YEARS //
