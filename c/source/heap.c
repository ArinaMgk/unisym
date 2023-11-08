// ASCII TAB4 C99 ArnCovenant TAB CRLF
// @ArinaMgk @dosconio
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

#include "../heap.h"
#include "../aldbg.h"

//---- ---- ---- ---- strpool ---- ---- ---- ----

static void Func_StrPoolRelease(void* memblk);

#define NewPool() {\
	TMP=CRT_POOL;\
	*(void**)(CRT_POOL = (char*)malc(_ModString_Strpool_UNIT_SIZE)) = 0;\
	if(TMP) *(void**)TMP=CRT_POOL;\
	Available = (_ModString_Strpool_UNIT_SIZE - sizeof(void*));\
	}

#define Macro_StrPoolRelease() Func_StrPoolRelease((void*)STR_POOL)

static char* STR_POOL = 0, * CRT_POOL = 0, * TMP;
static size_t Available;

void StrPoolInit()
{
	if (STR_POOL) Macro_StrPoolRelease();
	NewPool();
	STR_POOL = CRT_POOL;
}

char* StrPoolHeap(const char* str, size_t length)
{
	if (!length) while (str[length]) { length++; } length++;
	char* ebx = StrPoolAlloc(length);
	StrCopyN(ebx, str, length);
	return ebx;
}

char* StrPoolAllocZero(size_t length)
{
	if (!length) return 0;
	if (length <= _ModString_Strpool_UNIT_SIZE - sizeof(void*));
	else
	{
		TMP = CRT_POOL;
		*(void**)(CRT_POOL = (char*)zalc(sizeof(void*) + length)) = 0;
		if (TMP) *(void**)TMP = CRT_POOL;
		Available = 0;
		return CRT_POOL + sizeof(void*);
	}
	size_t ecx = length;// wish auto register
	if (Available <= length) { NewPool(); }
	Available -= length;
	length = (size_t)CRT_POOL + _ModString_Strpool_UNIT_SIZE - Available;
	while (ecx--)*(char*)(--length) = 0;
	return (char*)(length);
}

char* StrPoolAlloc(size_t length)
{
	if (!length) return 0;
	if (length <= _ModString_Strpool_UNIT_SIZE - sizeof(void*));
	else
	{
		TMP = CRT_POOL;
		*(void**)(CRT_POOL = (char*)malc(sizeof(void*) + length)) = 0;
		if (TMP) *(void**)TMP = CRT_POOL;
		Available = 0;
		return CRT_POOL + sizeof(void*);
	}
	if (Available <= length) { NewPool(); }
	char* ret = (CRT_POOL + _ModString_Strpool_UNIT_SIZE - Available);
	Available -= length;
	return ret;
}

static void Func_StrPoolRelease(void* memblk)
{
	if (!memblk)return;
	if (*(void**)memblk)
	{
		Func_StrPoolRelease(*(void**)memblk);
		*(void**)memblk = 0;
	}
	memf(memblk);
}

void StrPoolRelease()
{
	Macro_StrPoolRelease();
	STR_POOL = 0;
}


