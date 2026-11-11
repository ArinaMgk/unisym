// ASCII C++11 TAB4 LF
// Docutitle: AASM RISC-V Target Description
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../../asm/inc/aasm.h"
#include "../../../../../inc/cpp/Lango/Archend/riscv/Instruction-riscv.hpp"
#include "../../../../../inc/cpp/Lango/Archend/riscv/Target-riscv.hpp"

namespace uni::lango {

namespace {

constexpr uint32_t Extension(RISCVExtension extension)
{
	return static_cast<uint32_t>(extension);
}

bool Equal(const char* left, const char* right)
{
	return !StrCompareInsensitive(left, right);
}

bool Append(char* output, size_t capacity, size_t& length, const char* text)
{
	size_t size = StrLength(text);
	if (length + size >= capacity) return false;
	MemCopyN(output + length, text, size);
	length += size;
	output[length] = '\0';
	return true;
}

}

RISCVABI DefaultRISCVABI(int bits)
{
	return bits == 32 ? RISCVABI::ILP32 : RISCVABI::LP64;
}

bool ParseRISCVABI(const char* name, RISCVABI& abi)
{
	if (Equal(name, "ilp32")) abi = RISCVABI::ILP32;
	else if (Equal(name, "ilp32f")) abi = RISCVABI::ILP32F;
	else if (Equal(name, "ilp32d")) abi = RISCVABI::ILP32D;
	else if (Equal(name, "lp64")) abi = RISCVABI::LP64;
	else if (Equal(name, "lp64f")) abi = RISCVABI::LP64F;
	else if (Equal(name, "lp64d")) abi = RISCVABI::LP64D;
	else return false;
	return true;
}

bool ValidateRISCVABI(int bits, uint32_t extensions, RISCVABI abi)
{
	bool bits32 = abi == RISCVABI::ILP32 || abi == RISCVABI::ILP32F ||
		abi == RISCVABI::ILP32D;
	bool bits64 = abi == RISCVABI::LP64 || abi == RISCVABI::LP64F ||
		abi == RISCVABI::LP64D;
	if ((bits == 32 && !bits32) || (bits == 64 && !bits64)) return false;
	if (abi == RISCVABI::ILP32F || abi == RISCVABI::LP64F)
		return extensions & Extension(RISCVExtension::F);
	if (abi == RISCVABI::ILP32D || abi == RISCVABI::LP64D)
		return extensions & Extension(RISCVExtension::D);
	return bits32 || bits64;
}

uint32_t RISCVABIFlags(RISCVABI abi)
{
	if (abi == RISCVABI::ILP32F || abi == RISCVABI::LP64F) return 2;
	if (abi == RISCVABI::ILP32D || abi == RISCVABI::LP64D) return 4;
	return 0;
}

bool FormatRISCVArchitecture(int bits, uint32_t extensions,
	char* output, size_t capacity)
{
	if (!output || !capacity || (bits != 32 && bits != 64)) return false;
	int written = outsfmtbufn(output, capacity, "rv%di2p1", bits);
	if (written < 0 || static_cast<size_t>(written) >= capacity) return false;
	size_t length = static_cast<size_t>(written);
	struct Entry { RISCVExtension extension; const char* name; };
	static const Entry entries[] = {
		{RISCVExtension::M, "_m2p0"}, {RISCVExtension::A, "_a2p1"},
		{RISCVExtension::F, "_f2p2"}, {RISCVExtension::D, "_d2p2"},
		{RISCVExtension::C, "_c2p0"}, {RISCVExtension::V, "_v1p0"},
		{RISCVExtension::Zicsr, "_zicsr2p0"}
	};
	for (const Entry& entry : entries)
		if ((extensions & Extension(entry.extension)) &&
			!Append(output, capacity, length, entry.name)) return false;
	if ((extensions & Extension(RISCVExtension::M)) &&
		!Append(output, capacity, length, "_zmmul1p0")) return false;
	if (extensions & Extension(RISCVExtension::A)) {
		if (!Append(output, capacity, length, "_zaamo1p0_zalrsc1p0")) return false;
	}
	if (extensions & Extension(RISCVExtension::C)) {
		if (!Append(output, capacity, length, "_zca1p0")) return false;
		if ((extensions & Extension(RISCVExtension::D)) &&
			!Append(output, capacity, length, "_zcd1p0")) return false;
		if (bits == 32 && (extensions & Extension(RISCVExtension::F)) &&
			!Append(output, capacity, length, "_zcf1p0")) return false;
	}
	if (extensions & Extension(RISCVExtension::V)) {
		if (!Append(output, capacity, length,
			"_zve32f1p0_zve32x1p0_zve64d1p0_zve64f1p0_zve64x1p0"
			"_zvl128b1p0_zvl32b1p0_zvl64b1p0")) return false;
	}
	return true;
}

}
