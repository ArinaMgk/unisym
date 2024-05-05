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

// slice of Arina Coding Habits

#ifndef __USYM__
#define __USYM__ 1
#define _LIB_C
#endif

#ifndef _LIB_UNISYM//alias ArnHabit
#define _LIB_UNISYM

#ifndef _INC_CPP
	#define pointer_t(_typ) _typ * 
	#define pointerf_t(_ret_typ) _ret_typ(*) // e.g. `int x = sizeof(pointerf(void)(int));` 
	// compatible with Magice pointer: "pointer(pointer(void)) pp"
#endif

typedef void(*_tofree_ft)(void*);
typedef void* pureptr_t;

// __ENDIAN__
#define __ENDIAN__ 0 //[Optional] 1 for big endian, 0 for little endian
// __ARCH__
// __BITS__
	// [Rely-on] stdinc.h
// __FUNCIDEN__ : function identifier
#ifdef _MSC_VER // for MSVC
#define __FUNCIDEN__ __FUNCDNAME__
#elif defined(__GNUC__)
#define __FUNCIDEN__ __func__ // cannot auto-strcat
#endif

#define masm __asm

// ARINA-COVE C23-STYLE Attribute
#define _Heap
#define _Need_free _Heap
#define _Heap_tmpher// The function with _Heap_tmpher should not call each other
#define toheap
#define unchecked
#define _NOT_ABSTRACTED// into UNISYM

#define asserv(x) if(x)x // Assert Value
#define asrtand(x) (x)&&x // Assert Value
#define asrtequ(x,y) if((x)==(y))(x) // Assert Value
#define stepval(x) (!x)?0:x // do not nested by "()" !
	// E.g. paralext = stepval(subtail)->next;

#define If(con) if(0||con) //<=> if(1&&con) : avoid error such as mixing "a=0" and "a==0"

#define byteof sizeof
#define numsof(x) (sizeof(x)/sizeof(*(x)))

#if !defined(_DEBUG) && !defined(_dbg)
#define memalloc(dest,size)\
		((*(char**)&dest=(char*)malloc(size))? "":"! MEMORY RUN OUT!")
#define memfree(x) {if(x)free((char*)(x));}
#define srs(x,y) {void*ebx=(void*)(y);if(x)free((char*)x);*(void**)&(x)=ebx;}
#define malc(size) (void*)(malloc(size))
#define zalc(size) (void*)(calloc(size,1))
#endif
#define zalloc(x) calloc((x),1)// Zero Alloc
#define zalcof(x) (x*)zalc(sizeof(x))
#define malcof(x) (x*)malc(sizeof(x))
#define memf(x)   memfree(x)
#define mfree(x) do{memfree(x);(x)=0;}while(0)

// Added RFW24, Exchange but not for pointer, and address of `a` should not be the same as `b`
#define xchg(a,b) (a)^=(b)^=(a)^=(b)
#define xchgptr(a,b) *(size_t*)&(a)^=*(size_t*)&(b)^=*(size_t*)&(a)^=*(size_t*)&(b)

#define AssignParallel(l,m,r) ((l=m),(m=r))// different from `l=m=r`
#define Assign3(l,m,r) do{if(&(l)==&(r))AssignParallel(l,m,r); else xchg((l),(m));}while(0)
#define Assign3Pointer(l,m,r) do{if(&(l)==&(r))AssignParallel(l,m,r); else xchgptr(a,b);}while(0)
// Use 0 and ~0 as special invalid value, while Rust uses `Option` containing null.
// Example for the parameter: Bnode * inp = (Bnode*)~(stduint)0
#define nulrecurs(inp, root, rets) do {if (!inp) return rets; else if (!~(stduint)inp) inp = root; } while (0)

///#define foreachstr(iden,x) for(char iden, *iden#ptr=(char*)(x);iden=*iden#ptr;iden#ptr++)// {why} error tip yo VSCODE::IntelliSenseMode(GCC)
#define for0(iden,times) for(size_t iden=0, _LIMIT=(times);iden<(_LIMIT);iden++)
#define for1(iden,times) for(size_t iden=1;iden<=(times);iden++)

#endif
