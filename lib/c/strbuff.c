// ASCII C99 TAB4 CRLF
// Docutitle: (Module) 
// Codifiers: @dosconio: 20240906 ~ <Last-check> 
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

#include "../../inc/c/strbuff.h"
#include "../../inc/c/ustring.h"

//{TEMP}
#define likely(x)   (!!(x))
#define unlikely(x) (!!(x))

#define _BLKSHIFT	16
#define _BLKLEN	(_IMM1 << _BLKSHIFT)

Strbuff* StrbuffNew(stduint elm_len) 
{
	Strbuff* s = zalcof(Strbuff);
	if (!s) return NULL;
	s->blk_len = (elm_len >= _BLKLEN) ?
		elm_len : _BLKLEN - (_BLKLEN % elm_len);
	s->elm_len = elm_len;
	char* data = malc(s->alloc_len = s->blk_len);
	s->blk_p_cnt = s->blk_cnt = 1;
	s->blks_ptrs = malcof(char*);
	s->blks_ptrs[0] = data;
	s->p = s->q = &s->blks_ptrs[0];
	return s;
}

void StrbuffFree(Strbuff* s)
{
	char** tmp = s->blks_ptrs;
	for0(i, s->blk_cnt)
		memf(*tmp++);
	memfree(s->blks_ptrs);
	memfree(s);
}

// Block Allocate
static void StrbuffBalloc(Strbuff* s)
{
	size_t blkn = s->blk_cnt++;
	if (blkn >= s->blk_p_cnt) {
		size_t rindex = s->q - s->blks_ptrs;
		size_t windex = s->p - s->blks_ptrs;
		//: enlarge it
		s->blks_ptrs = realloc(s->blks_ptrs,
			(s->blk_p_cnt <<= 1) * sizeof(pureptr_t));
		s->q = s->blks_ptrs + rindex;
		s->p = s->blks_ptrs + windex;
	}
	s->blks_ptrs[blkn] = malc(s->blk_len);
	s->alloc_len += s->blk_len;
}

// Element Allocate
pureptr_t StrbuffEallic(Strbuff* s)
{
	pureptr_t res;

	if (s->p_inn % s->elm_len)
		return NULL;
	if (s->p_inn + s->elm_len > s->blk_len) {
		if (s->p_inn != s->blk_len)
			return NULL;
		if (s->p_glb + s->elm_len > s->alloc_len)
			StrbuffBalloc(s);
		s->p++;
		s->p_inn = 0;
	}
	res = *s->p + s->p_inn;
	s->p_inn += s->elm_len;
	s->p_glb += s->elm_len;
	MAX(s->total_len, s->p_glb);
	return res;
}

void StrbuffSend(Strbuff* s, pureptr_t src, stduint len)
{
	const char* d = src;
	while (len) {
		size_t l = s->blk_len - s->p_inn;
		MIN(l, len);
		if (l) {
			if (d) {
				MemCopyN(*s->p + s->p_inn, d, l);
				d += l;
			}
			else
				MemSet(*s->p + s->p_inn, 0, l);
			s->p_inn += l;
			s->p_glb += l;
			len -= l;
			MAX(s->total_len, s->p_glb);
		}
		if (len) {
			if (s->p_glb >= s->alloc_len)
				StrbuffBalloc(s);
			s->p++;
			s->p_inn = 0;
		}
	}
}

void StrbuffRewind(Strbuff* s)
{
	s->q = s->blks_ptrs;
	s->q_inn = s->q_glb = 0;
}

pureptr_t StrbuffBread(Strbuff* s)
{
	void* res;
	if (s->q_glb + s->elm_len > s->total_len)
		return NULL;
	if (s->q_inn % s->elm_len)
		return NULL;// misaligned
	if (s->q_inn + s->elm_len > s->blk_len) {
		s->q++;
		s->q_inn = 0;
	}
	res = *s->q + s->q_inn;
	s->q_inn += s->elm_len;
	s->q_glb += s->elm_len;
	return res;
}

const void* StrbuffRead(Strbuff* s, stduint* lenp)
{
	const void* res;
	size_t len;
	if (s->q_glb >= s->total_len) {
		*lenp = 0;
		return NULL;
	}
	if (s->q_inn >= s->blk_len) {
		s->q++;
		s->q_inn = 0;
	}
	len = *lenp;
	MIN(len, s->total_len - s->q_glb);
	MIN(len, s->blk_len - s->q_inn);
	*lenp = len;
	res = *s->q + s->q_inn;
	s->q_inn += len;
	s->q_glb += len;
	return res;
}

void StrbuffPop(Strbuff* s, pureptr_t dest, stduint len)
{
	char* d = dest;
	if (s->q_glb + len > s->total_len) {
		return;//{}overrun
	}
	while (len) {
		stduint l = len;
		const void* p = StrbuffRead(s, &l);
		MemCopyN(d, p, l);
		d += l;
		len -= l;
	}
}

//:{} Same as StrbuffPop, except position the counter first
void StrbuffPopx(Strbuff* s, stduint posi, pureptr_t dest, stduint len)
{
	size_t ix;
	if (posi + len > s->total_len) {
		return;//{} overrun
	}
	if (likely(s->blk_len == _BLKLEN)) {
		ix = posi >> _BLKSHIFT;
		s->q_inn = posi & (_BLKLEN - 1);
	}
	else {
		ix = posi / s->blk_len;
		s->q_inn = posi % s->blk_len;
	}
	s->q_glb = posi;
	s->q = &s->blks_ptrs[ix];
	StrbuffPop(s, dest, len);
}
void StrbuffSendx(Strbuff* s, stduint posi, const pureptr_t src, stduint len)
{
	size_t ix;
	if (posi > s->total_len) {
		//{}Seek beyond the end of the existing array not supported
		return;//{}overrun
	}
	if (likely(s->blk_len == _BLKLEN)) {
		ix = posi >> _BLKSHIFT;
		s->p_inn = posi & (_BLKLEN - 1);
	}
	else {
		ix = posi / s->blk_len;
		s->p_inn = posi % s->blk_len;
	}
	s->p_glb = posi;
	s->p = &s->blks_ptrs[ix];
	if (!s->p_inn) {
		s->p_inn = s->blk_len;
		s->p--;
	}
	StrbuffSend(s, src, len);
}

void StrbuffSendFile(Strbuff* s, FILE* fp)
{
	const char* data;
	size_t len;
	StrbuffRewind(s);
	while (len = s->total_len, (data = StrbuffRead(s, &len)) != NULL)
		fwrite(data, 1, len, fp);
}

//:---- Covenant Rules Little Endian ----

void StrbuffSend8(Strbuff* s, uint8 v)
{
	StrbuffSend(s, &v, 1);
}
void StrbuffSend16(Strbuff* s, uint16 v)
{
	StrbuffSend(s, &v, 2);
}
void StrbuffSend32(Strbuff* s, uint32 v)
{
	StrbuffSend(s, &v, 4);
}
#ifdef _BIT_SUPPORT_64
void StrbuffSend64(Strbuff* s, uint64 v)
{
	StrbuffSend(s, &v, 8);
}
#endif
void StrbuffSendStd(Strbuff* s, stduint v)
{
	StrbuffSend(s, &v, byteof(v));
}

//!---- Covenant Rules Little Endian ----
//{} Q
void StrbuffSendWleb128u(Strbuff* s, int value)
{
	char temp[64], * ptemp;
	uint8_t byte;
	int len;
	ptemp = temp;
	len = 0;
	do {
		byte = value & 127;
		value >>= 7;
		if (value != 0)// more bytes to come
			byte |= 0x80;
		*ptemp = byte;
		ptemp++;
		len++;
	} while (value != 0);
	StrbuffSend(s, temp, len);
}

//{} Q
void StrbuffSendWleb128s(Strbuff* s, int value)
{
	char temp[64], * ptemp;
	uint8_t byte;
	bool more, negative;
	int size, len;
	ptemp = temp;
	more = 1;
	negative = (value < 0);
	size = bitsof(int);
	len = 0;
	while (more) {
		byte = value & 0x7f;
		value >>= 7;
		if (negative) // sign extend
			value |= -(1 << (size - 7));
		// sign bit of byte is second high order bit (0x40)
		if ((value == 0 && !(byte & 0x40)) ||
			((value == -1) && (byte & 0x40)))
			more = 0;
		else
			byte |= 0x80;
		*ptemp = byte;
		ptemp++;
		len++;
	}
	StrbuffSend(s, temp, len);
}
