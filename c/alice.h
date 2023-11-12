// ASCII,GB Arina_habit_slice C99 TAB4
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
#ifndef __USYM__
#define __USYM__ 1
#define _LIB_C
#endif

#ifndef _LIB_UNISYM//alias ArnHabit
#define _LIB_UNISYM

#include <limits.h>

#define pointer(typ) typ * 
// compatible with MIKA pointer: "pointer(pointer(void)) pp"

#if SIZE_MAX==0xFFFF
	#define _BINARY 16 
#elif SIZE_MAX==0xFFFFFFFF
	#define _BINARY 32
#elif SIZE_MAX==0xFFFFFFFFFFFFFFFF
	#define _BINARY 64
#else 
	#define _BINARY 8
#endif

#define __BITS__ _BINARY

#if defined(_WinNT)&&(_BINARY==64)
	#include "us_win64.h"
#else
	#include <stddef.h>
	#if !defined(uint)&&!defined(_Linux)// avoid GCC duplicate ¡®unsigned¡¯
		#define uint unsigned int
	#endif
	#define llong long long int
	#define sll signed long long int
	#define ullong unsigned long long int
	#define ull unsigned long long int
	#define byte unsigned char
	#define word unsigned short int
	#define dword unsigned int
	#define qword unsigned long long int
	#define stduint size_t
	#define stdint ptrdiff_t
#endif


#define nil 0
#define nop 0x90

#define masm __asm

#define print(x) printf("%s",(x))
#define printline(x) printf("%s\n",(x))
#define pll(x,y)   printf("%s : %llx\n",(x),(y))
#define pd(x) printf("%d\n",(x))

#define concur(r,c) (r*cols+c)// ConsoleCursor


#define AlignEven(x) ((x)+1)&(~1)// {Q} the size of ~1 may be flexible

#define jump goto
#define call(x) (x)()
#define callif(x) (x?(void)(x)():(void)0)

// These IF can avoid error such as mixing "a=0" and "a==0"
#define If(con) if(0||con)
#define If_(con) if(1&&con)
#define RNZ(a) {if(a) return (a);}// Return if not zero
#define RZ(a) {if(!a) return (a);}// Return if zero

#define idle() {jump: goto jump;}

#define zalloc(x) calloc((x),1)// Zero Alloc

// Added RFW24, Exchange
#define xchg(a,b) (a)^=(b)^=(a)^=(b)
#define xchgptr(a,b) *(size_t*)&(a)^=*(size_t*)&(b)^=*(size_t*)&(a)^=*(size_t*)&(b)

#ifndef max//(a,b)
#define max(a,b) ((a)>(b)?(a):(b))
#endif
#define MAX(d,s) if((d)<(s)){(d)=(s);}
#ifndef min//(a,b)
#define min(a,b) ((a)<(b)?(a):(b))
#endif
#define MIN(d,s) if((d)>(s)){(d)=(s);}
// ---- ---- algorithm ---- ----
#define isodd(x) ((x)&1)
#define iseven(x) !((x)&1)
#define isZeroMantissa(flt) ((int)(flt)==(flt)) // RFW23

#define chars_stack(x) &(char[]){#x "\0"}// chars_stack(123)
#define chstk chars_stack

// ARINA-COVE C23-STYLE Attribute
#define _Heap
#define _Need_free _Heap
#define _Heap_tmpher// The function with _Heap_tmpher should not call each other

#define AddDecimalDigitsLen(i,num) do{(i)++;(num)/=10;}while(num)// e.g. for "0" is 1, "12" is 2

///#define foreachstr(iden,x) for(char iden, *iden#ptr=(char*)(x);iden=*iden#ptr;iden#ptr++)// {why} error tip yo VSCODE::IntelliSenseMode(GCC)
#define for0(iden,times) for(size_t iden=0;iden<(times);iden++)
#define for1(iden,times) for(size_t iden=1;iden<=(times);iden++)

#define byteof sizeof
#define numsof(x) (sizeof(x)/sizeof(*(x)))

#define BitsTog(x,bits) ((x)^=(bits))
#define ascii_isdigit(c) ((c)-'0'<10)
#define ascii_islower(c) ((c)-'a'<26)
#define ascii_isupper(c) ((c)-'A'<26)
#define ascii_tolower(c) ((c)-'A'<26?(c)|0x20:c)
#define ascii_toupper(c) ((c)-'a'<26?(c)&~0x20:c)
#define ascii_tohexad(c) ((c)>='a'?(c)-'a'+10:(c)>='A'?(c)-'A'+10 :(c)-'0')

#define immed_tobool(i) !!(i) // != 0
#if defined(_SUPPORT_BOOL) && !defined(_SUPPORT_BOOL_DEFINED)
	#define _SUPPORT_BOOL_DEFINED
	typedef enum bool { false, true } bool;// use immed_tobool() to convert with this
#endif

#ifdef _MSC_VER// for MSVC
	#define __FUNCIDEN__ __FUNCDNAME__
#elif defined(__GNUC__)
	#define __FUNCIDEN__ __func__// cannot auto-strcat
#endif

#endif
// more to see "aldbg.h"
// ---- ---- ---- ---- ---- ---- ---- ----

