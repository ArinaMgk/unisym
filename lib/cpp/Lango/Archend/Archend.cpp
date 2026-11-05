// ASCII C++11 TAB4 LF
// Docutitle: Lango Architecture Assembler Registry
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../inc/c/ustring.h"
#include "../../../../inc/cpp/Lango/Archend/Archend-riscv.hpp"
#include "../../../../inc/cpp/Lango/Archend/Archend-x86.hpp"

namespace uni::lango {

namespace {

ArchendInterface** RegisteredArchends(size_t& count)
{
	static ArchendInterface* archends[] = {
		&X86Archend(),
		&RISCVArchend()
	};
	count = numsof(archends);
	return archends;
}

}

ArchendInterface& DefaultArchend()
{
	return X86Archend();
}

ArchendInterface* FindArchend(const char* name)
{
	size_t count;
	ArchendInterface** archends = RegisteredArchends(count);
	for (size_t index = 0; index < count; index++) {
		if (!StrCompareInsensitive(
			name, archends[index]->DumpArchend().shortname))
			return archends[index];
	}
	return nullptr;
}

void ListArchends(const ArchendInterface& selected, FILE* output)
{
	size_t count;
	ArchendInterface** archends = RegisteredArchends(count);
	for (size_t index = 0; index < count; index++) {
		const ArchendInfo& info = archends[index]->DumpArchend();
		fprintf(output, "  %c %-10s%s\n",
			archends[index] == &selected ? '*' : ' ',
			info.shortname, info.fullname);
	}
}

}
