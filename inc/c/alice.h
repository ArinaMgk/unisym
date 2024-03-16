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

#include <limits.h>

#ifndef _INC_CPP
	#define pointer(_typ) _typ * 
	#define pointerf(_ret_typ) _ret_typ(*) // e.g. `int x = sizeof(pointerf(void)(int));` 
	// compatible with Magicoll pointer: "pointer(pointer(void)) pp"
#endif


// temporarily make use of standard library
#if SIZE_MAX==0xFFFF
	#define _BINARY 16 
	#define __BIT_STR__ "16"
#elif SIZE_MAX==0xFFFFFFFF
	#define _BINARY 32
	#define __BIT_STR__ "32"
#elif SIZE_MAX==0xFFFFFFFFFFFFFFFF
	#define _BINARY 64
	#define __BIT_STR__ "64"
#else 
	#define _BINARY 8
	#define __BIT_STR__ "8"
#endif

#define __STDC_FORMAT_MACROS// for GCC
#include <inttypes.h>
#define PRIdSTD PRIdPTR
#define PRIuSTD PRIuPTR
#define PRIxSTD PRIxPTR

#define __ENDIAN__ 0//{TODO} 1 for big endian, 0 for little endian

#define __BITS__ _BINARY

#if defined(_WinNT)&&(_BINARY==64)
	#include "us_win64.h"
#else
	#include <stddef.h>
	#include <stdint.h>
	#if !defined(uint)&&!defined(_Linux)// avoid GCC duplicate 'unsigned'
		#define uint unsigned int
	#endif
	#define llong long long int 
	#define sll signed long long int
	#define ullong unsigned long long int
	#define ull unsigned long long int
	typedef unsigned char byte; // [MinGW-i686 Conflict] #define byte unsigned char
	typedef   signed char sbyte;
	typedef uint16_t  word;// unsigned short int
	typedef  int16_t  sword;// signed short int
	typedef uint32_t  dword;// unsigned int
	typedef  int32_t  sdword;// signed int
	typedef uint64_t  qword;// unsigned long long int
	typedef  int64_t  sqword;// signed long long int
	typedef size_t    stduint;
	typedef ptrdiff_t stdint ;
	//
	#define valword(x) (*(word*)&(x))// will be template overload as C++ version
#endif

#define _NOT_ABSTRACTED// into UNISYM

#define nil 0
#define NUL 0

#define masm __asm

#define print(x) printf("%s",(x))
#define printline(x) printf("%s\n",(x))
#define pll(x,y)   printf("%s : %llx\n",(x),(y))
#define pd(x) printf("%d\n",(x))

#define concur(r,c) (r*cols+c)// ConsoleCursor


#define AlignEven(x) ((x)+1)&(~1)// {Q} the size of ~1 may be flexible

#define jump goto
#define call(x)   (x)()
#define callnz(x) (x)&&(x)()
#define callif(x) (x?(void)(x)():(void)0)

#define asserv(x) if(x)x // Assert Value
#define asrtand(x) (x)&&x // Assert Value
#define asrtequ(x,y) if((x)==(y))x // Assert Value
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

//{} May be conflict with stdlib.h, so temporarily:
#include <stdlib.h>
#ifndef maxof//(a,b)
#define maxof(a,b) ((a)>(b)?(a):(b))
#endif
#ifndef minof//(a,b)
#define minof(a,b) ((a)<(b)?(a):(b))
#endif



#define MAX(d,s) if((d)<(s)){(d)=(s);}

#define MIN(d,s) if((d)>(s)){(d)=(s);}
//{} ---- ---- algorithm ---- ----
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

extern size_t _size_decimal;
	
// ==== ==== <mcore.c> ==== ====	
extern struct _aflag_t// for some unisym functions, stored in stack for calling other functions. Method of application: register the current address of aflag globally before calling corresponding functions.
{
	byte carry : 1;                    // CF
	byte autosort : 1;                 ///1 or ASF (Auto Sort Flag) 
	byte parity : 1;                   // PF
	byte signsym : 1;                  ///0 or SEF (Signed Operand Flag)
	byte auxiliary : 1;                // AF
	byte one : 1;                      ///0 or ONF (One Flag)
	byte zero : 1;                     // ZF
	byte sign : 1;                     // SF
	byte debug : 1;                    // TF (Trap Flag)
	byte interrupt : 1;                // IF
	byte direction : 1;                // DF (0 for increasing, 1 for decreasing)
	byte overflow : 1;                 // OF or precies loss
	byte io_privilege_level : 2;       // IOPL
	byte nested_task : 1;              // NT
	byte fail : 1;                     ///0 or FF (Fail Flag)
	//
	byte resume : 1;                   // RF
	byte virtual_8086 : 1;             // VM
	byte alignment_check : 1;          // AC
	byte virtual_interrupt : 1;        // VIF
	byte virtual_interrupt_pending : 1;// VIP
	byte identification : 1;           // ID
	byte : 2;
	byte : 8;
} aflaga;

#define _aflaga_init() (MemSet(&aflaga, 0, sizeof aflaga), aflaga.autosort = 1)

#endif
// more to see "aldbg.h"
// ---- ---- ---- ---- ---- ---- ---- ----

