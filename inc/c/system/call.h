// ASCII C++ TAB4 CRLF
// Attribute: 
// LastCheck: 20240331
// AllAuthor: @dosconio
// ModuTitle: Calling
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

#ifndef _INC_CALL
#define _INC_CALL

#if defined(_Linux) && __BITS__ == 32
#define _CALL_FAST __attribute__((fastcall))
#define _CALL_CPL  __attribute__((cdecl))
#elif defined(_WinNT)
#define _CALL_FAST __fastcall 
#define _CALL_CPL __cdecl
#else
#define _CALL_FAST
#define _CALL_CPL 
#endif

//#define call(x)   (x)()
#define callnz(x) (x)&&(x)()
#define callif(x) (x?(void)(x)():(void)0)
#define callifidn(x, iden) do if(x)(x)(iden); while(0)

extern size_t call_state;// <OUT>
// call_state :
// 00: nullptr
// 01: over the limit for size
// 02: invaild input but not null pointer

#endif
