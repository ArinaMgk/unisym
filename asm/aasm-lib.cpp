// ASCII C TAB4 CRLF
// Docutitle: (Module) 
// Codifiers: @dosconio: 20241114
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

#include <stdinc.h>
#include <multichar.h>
#include <uctype.h>
_ESYM_C {
#include <insns.h>

}

_ESYM_C uint64_t _crc64_tab_nasm[256];

struct cpu_list_t { rostr idn; uint32 val; };
cpu_list_t cpu_list[] = {
	{"8086", IF_8086},
	{"186", IF_186},
	{"286", IF_286},
	{"386", IF_386},
	{"486", IF_486},
	{"586", IF_PENT}, {"pentium", IF_PENT},
	{"686", IF_P6}, {"ppro", IF_P6}, {"pentiumpro", IF_P6}, {"p2", IF_P6},
	{"p3", IF_KATMAI}, {"katmai", IF_KATMAI},
	{"p4", IF_WILLAMETTE}, {"willamette", IF_WILLAMETTE},
	{"prescott", IF_PRESCOTT},
	{"x64", IF_X86_64}, {"x86-64", IF_X86_64},
	{"ia64", IF_IA64},
	{"ia-64", IF_IA64},
	{"itanium", IF_IA64},
	{"itanic", IF_IA64},
	{"merced", IF_IA64},
};




_ESYM_C{
	// ---- CRC64 ----
	uint64_t crc64(uint64_t crc, const char* str) {
		foreach_byt(ch, str)
			crc = _crc64_tab_nasm[(byte)crc ^ ch] ^ (crc >> _BYTE_BITS_);
		return crc;
	}
	uint64_t crc64i(uint64_t crc, const char* str) {
		foreach_byt(ch, str)
			crc = _crc64_tab_nasm[(byte)crc ^ ascii_tolower(ch)] ^ (crc >> _BYTE_BITS_);
		return crc;
	}
	// ---- CPU ----
	uint32 get_cpu_id(rostr iden) {
		for0a(i, cpu_list) if (!StrCompareInsensitive(cpu_list[i].idn, iden)) {
			return cpu_list[i].val;
		}
		return IF_PLEVEL;
	}
	// ---- Char ----
	//{UNCHK} Apply a specific string transform and return it in a nasm_malloc'd buffer, returning the length.  On error, returns (size_t)-1 and no buffer is allocated.
	size_t string_transform(char* str, size_t len, char** out, enum strfunc func)
	{
		if (func == 0)
			return CscUTF(8, 16, str, len, (pureptr_t*)out);
		else if (func == 1)
			return CscUTF(8, 32, str, len, (pureptr_t*)out);
		else return 0;
	}

}


