// ASCII C++11 TAB4 LF
// Docutitle: AASM RISC-V Target Description
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../../asm/inc/aasm.h"
#include "../../../../../inc/cpp/Lango/Archend/riscv/Target-riscv.hpp"

namespace uni::lango {

namespace {

enum class ExtensionNameKind : uint8_t {
	SingleLetter,
	MultiLetter
};

struct ExtensionInfo {
	RISCVExtension extension;
	const char* name;
	const char* attribute;
	ExtensionNameKind kind;
	uint8_t order;
	uint32_t implied;
};

constexpr uint32_t Extension(RISCVExtension extension)
{
	return RISCVExtensionMask(extension);
}

const ExtensionInfo ExtensionTable[] = {
	{RISCVExtension::I, "i", "i2p1", ExtensionNameKind::SingleLetter, 0, 0},
	{RISCVExtension::M, "m", "m2p0", ExtensionNameKind::SingleLetter, 1, 0},
	{RISCVExtension::A, "a", "a2p1", ExtensionNameKind::SingleLetter, 2, 0},
	{RISCVExtension::F, "f", "f2p2", ExtensionNameKind::SingleLetter, 3,
		Extension(RISCVExtension::Zicsr)},
	{RISCVExtension::D, "d", "d2p2", ExtensionNameKind::SingleLetter, 4,
		Extension(RISCVExtension::F)},
	{RISCVExtension::C, "c", "c2p0", ExtensionNameKind::SingleLetter, 5, 0},
	{RISCVExtension::V, "v", "v1p0", ExtensionNameKind::SingleLetter, 6,
		Extension(RISCVExtension::F) | Extension(RISCVExtension::D) |
		Extension(RISCVExtension::Zicsr)},
	{RISCVExtension::Zicsr, "zicsr", "zicsr2p0",
		ExtensionNameKind::MultiLetter, 7, 0},
	{RISCVExtension::Zifencei, "zifencei", "zifencei2p0",
		ExtensionNameKind::MultiLetter, 8, 0}
};

constexpr uint32_t ImplementedExtensions =
	Extension(RISCVExtension::I) |
	Extension(RISCVExtension::M) |
	Extension(RISCVExtension::A) |
	Extension(RISCVExtension::F) |
	Extension(RISCVExtension::D) |
	Extension(RISCVExtension::C) |
	Extension(RISCVExtension::V) |
	Extension(RISCVExtension::Zicsr) |
	Extension(RISCVExtension::Zifencei);

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

const ExtensionInfo* FindExtension(const char* name, size_t length,
	ExtensionNameKind kind)
{
	for (const ExtensionInfo& extension : ExtensionTable)
		if (extension.kind == kind && StrLength(extension.name) == length &&
			!StrCompareNInsensitive(name, extension.name, length))
			return &extension;
	return nullptr;
}

uint32_t ApplyImpliedExtensions(uint32_t extensions)
{
	for (;;) {
		uint32_t previous = extensions;
		for (const ExtensionInfo& extension : ExtensionTable)
			if (extensions & Extension(extension.extension))
				extensions |= extension.implied;
		if (extensions == previous) return extensions;
	}
}

}

bool ParseRISCVInstructionSet(const char* name, uint32_t& extensions)
{
	if (!name || ascii_tolower(*name) != 'i') return false;
	uint32_t result = Extension(RISCVExtension::I);
	const char* cursor = name + 1;
	uint8_t previous_order = 0;
	while (*cursor && *cursor != '_') {
		const ExtensionInfo* extension = FindExtension(
			cursor, 1, ExtensionNameKind::SingleLetter);
		if (!extension || extension->extension == RISCVExtension::I ||
			extension->order <= previous_order) return false;
		uint32_t mask = Extension(extension->extension);
		if (result & mask) return false;
		result |= mask;
		previous_order = extension->order;
		cursor++;
	}
	while (*cursor) {
		if (*cursor++ != '_') return false;
		const char* start = cursor;
		while (*cursor && *cursor != '_') cursor++;
		size_t length = static_cast<size_t>(cursor - start);
		if (!length) return false;
		const ExtensionInfo* extension = FindExtension(
			start, length, ExtensionNameKind::MultiLetter);
		if (!extension || extension->order <= previous_order) return false;
		uint32_t mask = Extension(extension->extension);
		if (result & mask) return false;
		result |= mask;
		previous_order = extension->order;
	}
	result = ApplyImpliedExtensions(result);
	if (!ValidateRISCVInstructionSet(result)) return false;
	extensions = result;
	return true;
}

bool ValidateRISCVInstructionSet(uint32_t extensions)
{
	if (!(extensions & Extension(RISCVExtension::I)) ||
		(extensions & ~ImplementedExtensions)) return false;
	return ApplyImpliedExtensions(extensions) == extensions;
}

bool HasRISCVExtension(uint32_t extensions, RISCVExtension extension)
{
	return (extensions & Extension(extension)) != 0;
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
		return HasRISCVExtension(extensions, RISCVExtension::F);
	if (abi == RISCVABI::ILP32D || abi == RISCVABI::LP64D)
		return HasRISCVExtension(extensions, RISCVExtension::D);
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
	for (const ExtensionInfo& extension : ExtensionTable) {
		if (extension.extension == RISCVExtension::I ||
			!HasRISCVExtension(extensions, extension.extension)) continue;
		if (!Append(output, capacity, length, "_") ||
			!Append(output, capacity, length, extension.attribute)) return false;
	}
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
