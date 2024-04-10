// ASCII C99 TAB4 CRLF
// LastCheck: 2023 Nov 16
// AllAuthor: @ArinaMgk; @dosconio
// ModuTitle: General Header
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
#ifndef __USYM__
#define __USYM__ 1
#define _LIB_C
#endif

#ifndef _LIB_UNISYM//alias ArnHabit
#define _LIB_UNISYM

#include "integer.h"

#ifndef _INC_CPP
	#define pointer(_typ) _typ * 
	#define pointerf(_ret_typ) _ret_typ(*) // e.g. `int x = sizeof(pointerf(void)(int));` 
	// compatible with Magice pointer: "pointer(pointer(void)) pp"
#endif

typedef void(*_tofree_ft)(void*);

#define __ENDIAN__ 0//{TODO} 1 for big endian, 0 for little endian
#define __BITS__ _BINARY

#define _NOT_ABSTRACTED// into UNISYM

#define nil 0
#define NUL 0

#define masm __asm

#define unchecked
#define toheap

#define print(x) printf("%s",(x))
#define printline(x) printf("%s\n",(x))
#define pll(x,y)   printf("%s : %llx\n",(x),(y))
#define pd(x) printf("%d\n",(x))

#define concur(r,c) (r*cols+c)// ConsoleCursor

#define AlignEven(x) ((x)+1)&(~1)// {Q} the size of ~1 may be flexible

#define jump goto
//#define call(x)   (x)()
#define callnz(x) (x)&&(x)()
#define callif(x) (x?(void)(x)():(void)0)
#define callifidn(x, iden) do if(x)(x)(iden); while(0)

#define asserv(x) if(x)x // Assert Value
#define asrtand(x) (x)&&x // Assert Value
#define asrtequ(x,y) if((x)==(y))(x) // Assert Value
#define stepval(x) (!x)?0:x // do not nested by "()" !
	// E.g. paralext = stepval(subtail)->next;

// These IF can avoid error such as mixing "a=0" and "a==0"
#define If(con) if(0||con)
#define If_(con) if(1&&con)
#define RNZ(a) {if(a) return (a);}// Return if not zero
#define RZ(a) {if(!a) return (a);}// Return if zero

#define idle() {jump: goto jump;}

#define zalloc(x) calloc((x),1)// Zero Alloc

// Added RFW24, Exchange but not for pointer, and address of `a` should not be the same as `b`
#define xchg(a,b) (a)^=(b)^=(a)^=(b)
#define xchgptr(a,b) *(size_t*)&(a)^=*(size_t*)&(b)^=*(size_t*)&(a)^=*(size_t*)&(b)

#define AssignParallel(l,m,r) ((l=m),(m=r))// different from `l=m=r`

#include <stdlib.h>

// ---- ---- Integer and Floating ---- ----


// Floating
#define isZeroMantissa(flt) ((int)(flt)==(flt)) 

#define chars_stack(x) &(char[]){#x "\0"}// chars_stack(123)
#define chstk chars_stack

// ARINA-COVE C23-STYLE Attribute
#define _Heap
#define _Need_free _Heap
#define _Heap_tmpher// The function with _Heap_tmpher should not call each other

#define AddDecimalDigitsLen(i,num) do{(i)++;(num)/=10;}while(num)// e.g. for "0" is 1, "12" is 2

///#define foreachstr(iden,x) for(char iden, *iden#ptr=(char*)(x);iden=*iden#ptr;iden#ptr++)// {why} error tip yo VSCODE::IntelliSenseMode(GCC)
#define for0(iden,times) for(size_t iden=0, _LIMIT=(times);iden<(_LIMIT);iden++)
#define for1(iden,times) for(size_t iden=1;iden<=(times);iden++)

#define byteof sizeof
#define numsof(x) (sizeof(x)/sizeof(*(x)))

// __FUNCIDEN__ : function identifier
#ifdef _MSC_VER// for MSVC
	#define __FUNCIDEN__ __FUNCDNAME__
#elif defined(__GNUC__)
	#define __FUNCIDEN__ __func__// cannot auto-strcat
#endif

#include "call.h"
#include "aflag.h"

extern size_t _size_decimal;

#endif
// more to see "aldbg.h"
// ---- ---- ---- ---- ---- ---- ---- ----

