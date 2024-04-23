// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant
// LastCheck: RFZ06
// AllAuthor: @dosconio
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

#ifndef _INC_USTRING_INLINE

#include "../../../../inc/c/stdinc.h"

#if __BITS__ >= 64
void* MemSet(void* s, int c, size_t n)
{
	register union { byte* bptr;  word* wptr; dword* dptr; qword* qptr; size_t val; } ptr;
	register qword qwd;
	byte remain;
	if (!n) return s;
	ptr.bptr = (byte*)s;
	while (ptr.val & 0b111) { *ptr.bptr++ = c; n--; if (!n) return s; }
	remain = n & 0b111;
	qwd = (qword)((byte)c) << 0x38 | (qword)((byte)c) << 0x30 | (qword)((byte)c) << 0x28 | (qword)((byte)c) << 0x20 | 
		(qword)((byte)c) << 0x18 | (qword)((byte)c) << 0x10 | (qword)((byte)c) << 8 | (byte)c;
	n >>= 3;
	if (n) do// loop
	{
		*ptr.qptr++ = qwd;
	} while (--n);
	if (remain) do *ptr.bptr++ = (byte)c; while (--remain);// STOSB
	return s;
}
#elif __BITS__ >= 32
void* MemSet(void* s, int c, size_t n)
{
	register union { byte* bptr;  word* wptr; dword* dptr; size_t val; } ptr;
	register dword dwd;
	byte remain;
	if (!n) return s;
	ptr.bptr = (byte*)s;
	while (ptr.val & 0b11) { *ptr.bptr++ = c; n--; if (!n) return s; }
	remain = n & 0b11;
	dwd = (dword)((byte)c) << 24 | (dword)((byte)c) << 16 | (dword)((byte)c << 8) | (byte)c;
	n >>= 2;
	if (n) do// loop
	{
		*ptr.dptr++ = dwd;// STOSD
	} while (--n);
	if (remain) do *ptr.bptr++ = (byte)c; while (--remain);// STOSB
	return s;
}
#elif __BITS__ >= 16
void* MemSet(void* s, int c, size_t n)
{
	register union { byte* bptr;  word* wptr; size_t val; } ptr;
	register word wrd;
	byte remain;
	if (!n) return s;
	ptr.bptr = (byte*)s;
	if (ptr.val & 1) { *ptr.bptr++ = c; n--; if (!n) return s; }
	remain = n & 1;
	wrd = (word)((byte)c) << 8 | (byte)c;
	n >>= 1;
	if (n) do// loop
	{
		*ptr.wptr++ = wrd;// STOSW
	} while (--n);
	if (remain) *ptr.bptr++ = (byte)c;// STOSB
	return s;
}
#else
void* MemSet(void* s, int c, size_t n)
{
	while (n) { n--; ((char*)s)[n] = (char)c; }
	return s;
}
#endif



#endif
