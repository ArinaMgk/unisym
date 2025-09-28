// ASCII C++ TAB4 CRLF
// Attribute: <ArnCovenant> <Env> <bin^%> <CPU()> [Allocation]
// LastCheck: unchecked for C++ version
// AllAuthor: ...
// ModuTitle: ...
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

#include "../../inc/c/ustring.h"
#include "../../inc/cpp/strpool"

namespace uni {

#define tmpl(...) __VA_ARGS__ Strpool

	tmpl()::Strpool(stduint defablk)
	{
		unit_size = defablk;
		root_pool = (byte*)malc(defablk);
		last_pool = 0;
		*(void**)root_pool = 0;
		crt_pool = root_pool;
		local_ptr = sizeof(byte*);
	}

	tmpl()::~Strpool()
	{
		crt_pool = root_pool;
		while (crt_pool)
		{
			last_pool = crt_pool;
			crt_pool = *(byte**)crt_pool;
			memf(last_pool);
		}
		root_pool = 0;
	}

	tmpl(byte*)::new_pool(stduint len)
	{
		if (!len) len = unit_size;
		if (len < sizeof(byte*)) return 0;
		void* tmp = crt_pool;
		*(void**)(crt_pool = (byte*)malc(len)) = 0;
		if (tmp) *(void**)tmp = crt_pool;
		local_ptr = sizeof(byte*);
		return crt_pool;
	}

	tmpl(byte*)::Alloc(stduint size, const byte* mempos)
	{
		byte* ret = 0;
		if (local_ptr + size >= unit_size)
			if (size > unit_size - sizeof(byte*))
				new_pool(size + sizeof(byte*));
			else new_pool();
		ret = crt_pool + local_ptr;
		local_ptr += size;
		if (mempos) MemCopyN(ret, mempos, size);
		return ret;
	}

	tmpl(byte*)::AllocZero(stduint size)
	{
		byte* ret = Alloc(size);
		MemSet(ret, 0, size);
		return ret;
	}

	tmpl(byte*)::AllocString(const char* str, stduint length)
	{
		if (!str) return 0;
		if (!length) length = StrLength(str);
		byte* ret = Alloc(length + 1);
		MemCopyN(ret, str, length);
		ret[length] = 0;
		return ret;
	}

#undef tmpl
}
