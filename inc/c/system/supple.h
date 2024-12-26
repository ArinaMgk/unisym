// ASCII C TAB4 CRLF
// Docutitle: Supplement and Complement
// Codifiers: @dosconio: 20240603
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

#ifndef _INC_COMPLEMENT
#define _INC_COMPLEMENT

// ---- { STDC:: stdarg.h } 20240824 ----
// by makeing use of stack(BP) and C calling convention

typedef struct {
	byte* stack_ptr;
} para_list;

#define _para_align(type) (((sizeof(type) + sizeof(stduint) - 1) / sizeof(stduint)) * sizeof(stduint))

#if defined(_Linux) && __BITS__ == 64 || defined(_OPT_RISCV64) || defined(_HIS_IMPLEMENT)
//#define para_ento(ap, param) (ap.stack_ptr = (byte*)&param + ...)// by dscn trial, 20240908
#if defined(_DEV_GCC)//{TEMP} System V AMD64 ABI
	#define va_start(ap,last) (__builtin_va_start(ap, last))
	#define va_arg(ap,type) (__builtin_va_arg(ap, type))
	#define va_end(ap) (__builtin_va_end(ap))
	#define va_copy(d,s) (__builtin_va_copy(d, s))
	typedef __builtin_va_list va_list;
#else
	#include <stdarg.h>
#endif
#define para_ento(ap, param) va_start(ap, param)
#define para_next(ap, type) (type)va_arg(ap, type)
#define para_next_char(ap) (char)para_next(ap, int)
#define para_endo(ap) va_end(ap)
#define Letpara(argiden, cdecl_iden) va_list argiden; para_ento(argiden, cdecl_iden)
#define para_list va_list
#else// Win, Lin32
#define para_ento(ap, param) (ap.stack_ptr = (byte*)((stduint*)&param + 1))
#define para_next(ap, type) (*(type *)((ap.stack_ptr += _para_align(type)) - _para_align(type)))
#define para_next_char(ap) para_next(ap, char)
#define para_endo(ap) (ap.stack_ptr = NULL)// optional now
#define Letpara(argiden, cdecl_iden) para_list argiden; para_ento(argiden, cdecl_iden);// we can use as uni::Letpara
#endif


#define para_copy(dest, src, type) MemCopyN((void *)&dest, (void *)&src, _para_align(type))

// ---- { STDC:: setjmp.h } 20240902 (Win32) ----

// x86
typedef struct {
	uint32 ret;
	uint32 ebx;
	uint32 esi;
	uint32 edi;
	uint32 ebp;
	uint32 esp;
} Retpoint;

//{TEMP} only support 1 level of calling - please use setjmp.h now
extern Retpoint _ERRO_JUMP;
#define ErroPoint() (char*)MarkPoint(&_ERRO_JUMP)
//extern char* ErroPoint();
extern pureptr_t MarkPoint(Retpoint* buf);
extern void JumpPoint(const Retpoint* buf, pureptr_t val);

//#define setjmp MarkPoint
//inline static void longjmp(jmp_buf buf, int v) { JumpPoint(&buf, v); }

// ---- { MORE } ----

#endif
