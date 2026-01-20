// ASCII C/C++ TAB4 CRLF
// Docutitle: (Format: File) Executable and Linkable Format
// Codifiers: @dosconio: 20240427 ~ <Last-check> 
// Attribute: <ArnCovenant> Any-Architect <Environment> <Reference/Dependence>
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
#define ELFBIT 64
#include "../../../inc/c/format/ELF.h"
#include "../../../inc/c/ustring.h"

stduint ELF64_LoadExecFromMemory(const void* memsrc, void** p_entry)
{
	struct ELF_Header_t* header = (struct ELF_Header_t*)memsrc;
	stduint retval = 0;
	void* entry = (void*)header->e_entry;
	for0(i, header->e_phnum)
	{
		struct ELF_PHT_t* ph = (struct ELF_PHT_t*)((byte*)memsrc + header->e_phoff + header->e_phentsize * i);
		if (ph->p_type == PT_LOAD && (++retval))//{TEMP}VAddress
		{
			// ploginfo("ELF64_LoadExecFromMemory: %[64H]->%[64H]", ph->p_vaddr, (byte*)memsrc + ph->p_offset);
			MemCopyN((pureptr_t)ph->p_vaddr, (byte*)memsrc + ph->p_offset, ph->p_memsz);
		}
	}
	if (p_entry) *p_entry = entry;
	return retval;
}


