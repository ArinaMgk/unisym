// ASCII C++11 TAB4 LF
// Docutitle: Lango Architecture Decoder Registry
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../inc/c/ustring.h"
#include "../../../../inc/cpp/Lango/Archend/Archend-x86.hpp"

namespace uni::lango {

namespace {

ArchendDecodeInterface** RegisteredDecoders(size_t& count)
{
	static ArchendDecodeInterface* decoders[] = {
		&X86Decoder(),
	};
	count = numsof(decoders);
	return decoders;
}

}

ArchendDecodeInterface& DefaultArchendDecoder()
{
	size_t count;
	ArchendDecodeInterface** decoders = RegisteredDecoders(count);
	return *decoders[0];
}

ArchendDecodeInterface* FindArchendDecoder(const char* name)
{
	size_t count;
	ArchendDecodeInterface** decoders = RegisteredDecoders(count);
	for (size_t index = 0; index < count; index++) {
		if (!StrCompareInsensitive(
			name, decoders[index]->DumpArchend().shortname))
			return decoders[index];
	}
	return nullptr;
}

void ListArchendDecoders(
	const ArchendDecodeInterface& selected, FILE* output)
{
	size_t count;
	ArchendDecodeInterface** decoders = RegisteredDecoders(count);
	for (size_t index = 0; index < count; index++) {
		const ArchendInfo& info = decoders[index]->DumpArchend();
		fprintf(output, "  %c %-10s%s\n",
			decoders[index] == &selected ? '*' : ' ',
			info.shortname, info.fullname);
		fprintf(output, "      BITS: %s\n      INST: %s\n",
			info.supported_bits, info.instruction_sets);
	}
}

void ListArchendDecoderNames(FILE* output)
{
	size_t count;
	ArchendDecodeInterface** decoders = RegisteredDecoders(count);
	for (size_t index = 0; index < count; index++) {
		fprintf(output, "%s%s", index ? ", " : "",
			decoders[index]->DumpArchend().shortname);
	}
}

}
