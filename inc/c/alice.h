// ASCII C/C++ TAB4 CRLF
// Docutitle: Micro Core
// Codifiers: @dosconio: ~ 20240520
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
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
#define __USYM__ 2 // generation
#define _LIB_C
#endif

#ifndef _LIB_UNISYM
#define _LIB_UNISYM

// ArnCovenant
#define _BYTE_BITS_ 8


typedef void* pureptr_t;
typedef void(*_tofree_ft)(pureptr_t);
typedef  int(*_tocomp_ft)(pureptr_t, pureptr_t);
typedef void(symbol_t)(void);

#if defined(__cplusplus) && !defined(_INC_CPP)
	#define _INC_CPP
	#define _CALL_C extern "C"
#else	
	#define _CALL_C
#endif

#ifndef _INC_CPP
	#define pointer_t(_typ) _typ * 
	#define pointerf_t(_ret_typ) _ret_typ(*) // e.g. `int x = sizeof(pointerf(void)(int));` 
	// compatible with Magice pointer: "pointer(pointer(void)) pp"
	#define _REGISTER register
	// no `this`
#else
extern "C++" {
	namespace uni {
		// [TEST] let var float a = 2.0; then Castype(int, a) <=> cast<int>(a)
		template<typename typed, typename types> inline static typed& cast(types& value) {
			return *(typed*)(&value);
		}
	}
}
	#define _REGISTER
	#define self (*this)
#endif

// __ENDIAN__
#ifndef __ENDIAN__
	#define __ENDIAN__ 0 //[Optional] 1 for big endian, 0 for little endian
#endif
// __ARCH__
// __BITS__
	// [Rely-on] stdinc.h
#ifdef _MSC_VER // for MSVC
	#define _DEV_MSVC
	// __FUNCIDEN__ : function identifier
	#define __FUNCIDEN__ __FUNCDNAME__
	#define _ALIGN
	#define _ASM __asm

#elif defined(__GNUC__)
	#define _DEV_GCC
	#define __FUNCIDEN__ __func__ // cannot auto-strcat
	#define _ALIGN(n) __attribute__((aligned(n)))
	#define _ASM __asm__
#endif

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

#define Ranginc(i,range) ++(i) %= range

#define If(con) if(0||con) //<=> if(1&&con) : avoid error such as mixing "a=0" and "a==0"

#define byteof sizeof
#define bitsof(x) (sizeof(x)*_BYTE_BITS_)
#define numsof(x) (sizeof(x)/sizeof(*(x)))
#define offsof    _BYTE_BITS_*offsetof

#if !defined(_DEBUG) && !defined(_dbg)
#define memalloc(dest,size)\
		((*(char**)&dest=(char*)malloc(size))? "":"! MEMORY RUN OUT!")
#define memfree(x) {if(x)free((char*)(x));}
#define srs(x,y) {void*ebx=(void*)(y);if(x)free((char*)x);*(void**)&(x)=ebx;}
#define malc(size) (void*)(malloc(size))
#define zalc(size) (void*)(calloc(size,1))
#endif

#define zalloc(x) zalc(x)// Zero Alloc
#define zalcof(x) (x*)zalc(sizeof(x))
#define malcof(x) (x*)malc(sizeof(x))
#define ralcof(x,addr,nums) (x*)realloc((void*)(addr),(nums)*sizeof(x))
#define memf(x)     memfree(x)
#define mfree(x) do{memfree(x);(x)=0;}while(0)

// Added RFW24, Exchange but not for pointer, and address of `a` should not be the same as `b`
#define xchg(a,b) (a)^=(b)^=(a)^=(b)
// check a==b before using
#define xchgptr(a,b) *(size_t*)&(a)^=*(size_t*)&(b)^=*(size_t*)&(a)^=*(size_t*)&(b)

#define AssignParallel(l,m,r) ((l=m),(m=r))// different from `l=m=r`
#define Assign3(l,m,r) do{if(&(l)==&(r))AssignParallel(l,m,r); else xchg((l),(m));}while(0)
#define Assign3Pointer(l,m,r) do{if(&(l)==&(r))AssignParallel(l,m,r); else xchgptr(a,b);}while(0)
// Use 0 and ~0 as special invalid value, while Rust uses `Option` containing null.
// Example for the parameter: Bnode * inp = (Bnode*)None
#define nulrecurs(inp, root, rets) do {if (!inp) return rets; else if (!~(stduint)inp) inp = root; } while (0)

#define Castype(des,val) *(des*)&(val)
#define Letvar(iden,type,init) type iden = (type)init
#define floorAlign(align,val) ((val/align)*align)
#define ceilAlign(align,val)  floorAlign(align,val+(align-1))

#define foreach_str(iden,x) for(char iden, *_pointer=(char*)(x);iden=*_pointer;_pointer++)
#define for0(iden,times) for(size_t iden=0, _LIMIT=(times);iden<(_LIMIT);iden++)
#define for0r(iden,times) for(size_t iden=(times);iden--;)
#define for1(iden,times) for(size_t iden=1;iden<=(times);iden++)
#define for0a(iden,array) for0(iden,numsof(array))

#define getExfield(a) ((byte*)&(a) + sizeof(a)) // for l-value object

#define _TEMP
#define _TODO

#endif
