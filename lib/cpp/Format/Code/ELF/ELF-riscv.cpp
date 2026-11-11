// ASCII C++11 TAB4 LF
// Docutitle: RISC-V ELF Target Metadata
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

#ifndef _MCCA

#include "../../../../../inc/cpp/Lango/Archend/riscv/Target-riscv.hpp"
#include "../../../../../inc/cpp/Format/Code/ELF/ELF-riscv.hpp"

namespace uni {

namespace { 

constexpr uint32_t CompressedExtensionMask = UINT32_C(1) << 5;

void Write32(uint8_t* output, uint32_t value)
{
	output[0] = static_cast<uint8_t>(value);
	output[1] = static_cast<uint8_t>(value >> 8);
	output[2] = static_cast<uint8_t>(value >> 16);
	output[3] = static_cast<uint8_t>(value >> 24);
}

size_t StringLength(const char* text)
{
	size_t length = 0;
	while (text[length]) ++length;
	return length;
}

void CopyBytes(unsigned char* output, const char* input, size_t size)
{
	for (size_t i = 0; i < size; ++i)
		output[i] = static_cast<unsigned char>(input[i]);
}

}

void RISCVELFTarget::Reset(int bits)
{
	Bits = bits;
	Extensions = 0;
	Abi = static_cast<uint32_t>(lango::DefaultRISCVABI(bits));
	HasExplicitAbi = false;
	ReportedAbiConflict = false;
}

bool RISCVELFTarget::Update(int bits, unsigned int cpu,
	unsigned int abi, bool abi_explicit)
{
	Bits = bits;
	Extensions |= cpu;
	if (!abi_explicit) return true;
	if (!HasExplicitAbi) {
		Abi = abi;
		HasExplicitAbi = true;
		return true;
	}
	if (Abi != abi && !ReportedAbiConflict) {
		ReportedAbiConflict = true;
		return false;
	}
	return true;
}

unsigned int RISCVELFTarget::Flags() const
{
	unsigned int flags = 0;
	if (Extensions & CompressedExtensionMask) flags |= 1;
	flags |= lango::RISCVABIFlags(static_cast<lango::RISCVABI>(Abi));
	return flags;
}

size_t RISCVELFTarget::BuildAttributes(
	unsigned char* output, size_t capacity) const
{
	char architecture[384];
	if (!lango::FormatRISCVArchitecture(
		Bits, Extensions, architecture, sizeof(architecture))) return 0;
	size_t architecture_size = StringLength(architecture) + 1;
	size_t file_size = 1 + 4 + 2 + 1 + architecture_size;
	size_t section_size = 1 + 4 + 6 + file_size;
	if (!output || section_size > capacity) return 0;

	unsigned char* cursor = output;
	*cursor++ = 'A';
	Write32(cursor, static_cast<uint32_t>(section_size - 1));
	cursor += 4;
	CopyBytes(cursor, "riscv", 6);
	cursor += 6;
	*cursor++ = 1;
	Write32(cursor, static_cast<uint32_t>(file_size));
	cursor += 4;
	*cursor++ = 4;
	*cursor++ = 16;
	*cursor++ = 5;
	CopyBytes(cursor, architecture, architecture_size);
	return section_size;
}

}

#endif
