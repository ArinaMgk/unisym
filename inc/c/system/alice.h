// ASCII C/C++ TAB4 CRLF
// Docutitle: Micro Core Pseudo Keywords
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
#define __ARNC_COVENANT__ 2023 // RFR07
#define _BYTE_BITS_ 8

#define _TEMP
#define _TODO

typedef void* pureptr_t;
typedef void(*_tofree_ft)(pureptr_t);
typedef  int(*_tocomp_ft)(pureptr_t, pureptr_t);
typedef int*(*_tocall_ft)(void*, ...);
typedef void(symbol_t)(void);
typedef void(*Handler_t)(void);
typedef const char* rostr;// read-only string

#if defined(__cplusplus) && !defined(_INC_CPP)
	#define _INC_CPP
#endif

#ifndef _INC_CPP
	// #define _ESYM_C
	#define _ESYM_C   extern
	#define pointer_t(_typ) _typ * 
	#define pointerf_t(_ret_typ) _ret_typ(*) // e.g. `int x = sizeof(pointerf(void)(int));` 
	// compatible with Magice pointer: "pointer(pointer(void)) pp"
	#define _REGISTER register
	// no `this`
#else
	#define _ESYM_C   extern "C"
	#define _ESYM_CPP extern "C++"
extern "C++" {
	namespace uni {
		// [TEST] let var float a = 2.0; then Castype(int, a) <=> cast<int>(a)
		template<typename typed, typename types> constexpr inline static typed& cast(types& value) {
			return *(typed*)(&value);
		}
		// Treat the address as a type
		template<typename typed, typename types> constexpr inline static typed& treat(types addr) {
			return *(typed*)addr;
		}
	}
}
#define _REGISTER
#define self (*this)
#endif

// __ENDIAN__ : Byte Order in Target Host
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
//#undef _CALL_C
//#define _CALL_C __cdecl
#define _WEAK
#define _PACKED(x) x //{unchk}

#elif defined(__GNUC__)
#define _DEV_GCC
#define __FUNCIDEN__ __func__ // cannot auto-strcat
#define _ALIGN(n) __attribute__((aligned(n)))
#define _ASM __asm__
#define _WEAK __attribute__((weak))
#define _PACKED(x) x __attribute__((packed))

#elif defined(__UVISION_VERSION)
#define _DEV_KEIL
//{TODO}:
#define __FUNCIDEN__ 
#define _ALIGN
#define _ASM
#define _PACKED(x) x __attribute__((packed))

#endif


#define _ENDIAN_LOW // little endian for bit-field, ...
#define _ENDIAN_BIG //    big endian for bit-field, ...

// ARINA-COVE C23-STYLE Attribute
#define _Heap
#define _Need_free _Heap
#define _Heap_tmpher// The function with _Heap_tmpher should not call each other
#define _Comment(x)
#define toheap
#define unchecked
#define _NOT_ABSTRACTED// into UNISYM

#define asserv(x) if(x)x // Assert Value
#define asserz(x) if(!x)x // Assert Value Not
#define asrtand(x) (x)&&x // Assert Value
#define asrtinc(p) if(p)*(p)++ // for array
#define asrtdec(p) if(p)*--(p) // for stack
#define asrtequ(x,y) if((x)==(y))(x) // Assert Value
#define asrtret(w) if(!(w)) return false; // this can be with an `else`
#define stepval(x) (!x)?0:x // do not nested by "()" !
	// E.g. paralext = stepval(subtail)->next;

#define Rangein(i,a,b) ((a)<=(i) && (i)<(b)) // i in [a,b) a..b
#define Ranglin(i,a,l) ((a)<=(i) && (i)<(a+l)) // i in [a,a+l) a..a+l
#define Ranginc(i,range) do ++i, (i)%=(range);  while(0) //(++(i) %= range)
#define Rangincx(i,min,max) do if(i>=max)i=min; else++i;  while(0)// max included
#define Rangdec(i,range) do if(!(i)) i=range-1; else (--(i) %= range); while(0)
#define Rangdecx(i,min,max) do if(i<=min)i=max; else--i;  while(0)// min included
#define Rangstp(i,step,range) (((i)+=(step))%=(range)) // step
#define Rangsub(i,step,range) do if(i < step) i = range - step; else i -= step; while(0)

#define If(con) if(0||con) //<=> if(1&&con) : avoid error such as mixing "a=0" and "a==0"

#define byteof       sizeof
#define sliceof(x) x,sizeof(x)
#define bitsof(x) (sizeof(x)*_BYTE_BITS_)
#define numsof(x) (sizeof(x)/sizeof(*(x)))
#define offsof    _BYTE_BITS_*offsetof

#define zalloc(x) zalc(x)// Zero Alloc
#define zalcof(x) (x*)zalc(sizeof(x))
#define malcof(x) (x*)malc(sizeof(x))
#define ralcof(x,addr,nums) (x*)realloc((void*)(addr),(nums)*sizeof(x))

#define xchg(a,b) do if(a!=b){(a)^=(b);(b)^=(a);(a)^=(b);}while(0) // cancelled (a)^=(b)^=(a)^=(b);
#define xchgptr(a,b) do if((size_t*)a!=(size_t*)b)*(size_t*)&(a)^=*(size_t*)&(b)^=*(size_t*)&(a)^=*(size_t*)&(b);while(0)

#define AssignParallel(l,m,r) ((l=m),(m=r))// different from `l=m=r`
#define Assign3(l,m,r) do{if(&(l)==&(r))AssignParallel(l,m,r); else xchg((l),(m));}while(0)
#define Assign3Pointer(l,m,r) do{if(&(l)==&(r))AssignParallel(l,m,r); else xchgptr(a,b);}while(0)
// Use 0 and ~0 as special invalid value, while Rust uses `Option` containing null.
// Example for the parameter: Bnode * inp = (Bnode*)None
#define nulrecurs(inp, root, rets) do {if (!inp) return rets; else if (!~(stduint)inp) inp = root; } while (0)

#define Castype(des,val) *(des*)&(val)
#define Letvar(iden,type,init) type iden = (type)(init)

#define floorAlign(align,val) ((val/align)*align)
#define vaultAlign(align,val)  floorAlign(align,val+(align-1))
#define alignf floorAlign
#define alignc vaultAlign
#define ceilAlign vaultAlign
#define vaultAlignHexpow(align,val) ((val+_IMM(align-1))&~_IMM(align-1))

#define foreach_str(iden,x) for(char iden, *_pointer=(char*)(x);iden=*_pointer;_pointer++)
#define foreach_byt(iden,x) for(byte iden, *_pointer=(byte*)(x);iden=*_pointer;_pointer++)
#define for0(iden,times) for(size_t iden=0, _LIMIT=(times);iden<(_LIMIT);iden++)
#define for0r(iden,times) for(size_t iden=(times);iden--;)//<=> for(.=N-1;.>=0;.--)
#define forp(ptr,times) for(pureptr_t _LIMIT=(pureptr_t)(ptr+times);(pureptr_t)ptr<_LIMIT;ptr++)
#define for0p(typ,ptr,since,times) for(typ* ptr=(typ*)(since);_IMM(ptr)<_IMM(since)+(times);ptr++)
#define for1(iden,times) for(size_t iden=1;iden<=(times);iden++)
#define for1r(iden,times) for(size_t iden=(times);--iden;)//<=> for(.=N-1;.>0;.--)
#define for0a(iden,array) for0(iden,numsof(array))

#define sort0f(arr,len,cmp) sort(arr,arr+len,cmp)

#define bitguard(x) ((x)&-(x))

#define getExfield(a) ((byte*)&(a) + sizeof(a)) // for l-value object

#endif
