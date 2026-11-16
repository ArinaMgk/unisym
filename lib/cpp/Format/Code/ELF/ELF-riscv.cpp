// ASCII C++11 TAB4 LF
// Docutitle: RISC-V ELF Target Metadata
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

#ifndef _MCCA

#include "../../../../../inc/cpp/Lango/Archend/riscv/Target-riscv.hpp"
#include "../../../../../inc/cpp/Format/Code.hpp"
#include "../../../../../inc/cpp/Format/Code/ELF/ELF-riscv.hpp"

namespace uni {

namespace {

bool MatchesFixedWidthEncoding(const CodeFixup& fixup,
	unsigned storage_bits, unsigned value_bits, unsigned bit_offset,
	unsigned right_shift, bool signed_value)
{
	return fixup.encoding.form == CodeFixupEncodingForm::FixedWidth &&
		fixup.encoding.storage_bits == storage_bits &&
		fixup.encoding.value_bits == value_bits &&
		fixup.encoding.bit_offset == bit_offset &&
		fixup.encoding.right_shift == right_shift &&
		fixup.encoding.signed_value == signed_value &&
		fixup.encoding.field_count == 0;
}

}

int RISCVRelocationType(const CodeFixup& fixup)
{
	if (fixup.encoding.form == CodeFixupEncodingForm::UnsignedLeb128 &&
		fixup.encoding.storage_bits >= 8 &&
		fixup.encoding.storage_bits <= 80 &&
		!(fixup.encoding.storage_bits & 7) &&
		fixup.encoding.value_bits == fixup.encoding.storage_bits &&
		fixup.encoding.bit_offset == 0 &&
		fixup.encoding.right_shift == 0 &&
		!fixup.encoding.signed_value &&
		fixup.encoding.field_count == 0) {
		if (fixup.kind == CodeFixupKind::Set)
			return R_RISCV_SET_ULEB128;
		if (fixup.kind == CodeFixupKind::Subtract)
			return R_RISCV_SUB_ULEB128;
		return -1;
	}
	if (fixup.encoding.form != CodeFixupEncodingForm::FixedWidth)
		return -1;
	if (fixup.kind == CodeFixupKind::Absolute) {
		if (MatchesFixedWidthEncoding(fixup, 32, 32, 0, 0, false))
			return R_RISCV_32;
		if (MatchesFixedWidthEncoding(fixup, 64, 64, 0, 0, false))
			return R_RISCV_64;
	}
	if ((fixup.kind == CodeFixupKind::Call ||
		fixup.kind == CodeFixupKind::Plt) &&
		MatchesFixedWidthEncoding(fixup, 64, 32, 0, 0, true))
		return fixup.kind == CodeFixupKind::Plt ?
			R_RISCV_CALL_PLT : R_RISCV_CALL;
	if (fixup.kind == CodeFixupKind::PcRelative &&
		MatchesFixedWidthEncoding(fixup, 32, 32, 0, 0, true))
		return R_RISCV_32_PCREL;
	if (fixup.kind == CodeFixupKind::Got &&
		MatchesFixedWidthEncoding(fixup, 32, 32, 0, 0, true))
		return R_RISCV_GOT32_PCREL;
	if (fixup.kind == CodeFixupKind::Plt &&
		MatchesFixedWidthEncoding(fixup, 32, 32, 0, 0, true))
		return R_RISCV_PLT32;
	if (fixup.kind == CodeFixupKind::Got &&
		MatchesFixedWidthEncoding(fixup, 32, 20, 12, 12, true))
		return R_RISCV_GOT_HI20;
	if (fixup.kind == CodeFixupKind::High &&
		MatchesFixedWidthEncoding(fixup, 32, 20, 12, 12, true))
		return R_RISCV_HI20;
	if (fixup.kind == CodeFixupKind::Low &&
		MatchesFixedWidthEncoding(fixup, 32, 12, 20, 0, true))
		return R_RISCV_LO12_I;
	if (fixup.kind == CodeFixupKind::Low &&
		MatchesFixedWidthEncoding(fixup, 32, 12, 7, 0, true))
		return R_RISCV_LO12_S;
	if (fixup.kind == CodeFixupKind::Branch &&
		MatchesFixedWidthEncoding(fixup, 32, 13, 0, 1, true))
		return R_RISCV_BRANCH;
	if ((fixup.kind == CodeFixupKind::Branch ||
		fixup.kind == CodeFixupKind::Call) &&
		MatchesFixedWidthEncoding(fixup, 32, 21, 0, 1, true))
		return R_RISCV_JAL;
	if (fixup.kind == CodeFixupKind::Branch &&
		MatchesFixedWidthEncoding(fixup, 16, 9, 0, 1, true))
		return R_RISCV_RVC_BRANCH;
	if (fixup.kind == CodeFixupKind::Branch &&
		MatchesFixedWidthEncoding(fixup, 16, 12, 0, 1, true))
		return R_RISCV_RVC_JUMP;
	if (fixup.kind == CodeFixupKind::PageRelative &&
		MatchesFixedWidthEncoding(fixup, 32, 20, 12, 12, true))
		return R_RISCV_PCREL_HI20;
	if (fixup.kind == CodeFixupKind::PageOffset &&
		MatchesFixedWidthEncoding(fixup, 32, 12, 20, 0, true))
		return R_RISCV_PCREL_LO12_I;
	if (fixup.kind == CodeFixupKind::PageOffset &&
		MatchesFixedWidthEncoding(fixup, 32, 12, 7, 0, true))
		return R_RISCV_PCREL_LO12_S;
	if ((fixup.kind == CodeFixupKind::Subtract ||
		fixup.kind == CodeFixupKind::Set) &&
		MatchesFixedWidthEncoding(fixup, 8, 6, 0, 0, false))
		return fixup.kind == CodeFixupKind::Set ?
			R_RISCV_SET6 : R_RISCV_SUB6;
	if (fixup.kind == CodeFixupKind::Add) {
		switch (fixup.encoding.storage_bits) {
		case 8: return MatchesFixedWidthEncoding(fixup, 8, 8, 0, 0, false) ?
			R_RISCV_ADD8 : -1;
		case 16: return MatchesFixedWidthEncoding(fixup, 16, 16, 0, 0, false) ?
			R_RISCV_ADD16 : -1;
		case 32: return MatchesFixedWidthEncoding(fixup, 32, 32, 0, 0, false) ?
			R_RISCV_ADD32 : -1;
		case 64: return MatchesFixedWidthEncoding(fixup, 64, 64, 0, 0, false) ?
			R_RISCV_ADD64 : -1;
		}
	}
	if (fixup.kind == CodeFixupKind::Subtract) {
		switch (fixup.encoding.storage_bits) {
		case 8: return MatchesFixedWidthEncoding(fixup, 8, 8, 0, 0, false) ?
			R_RISCV_SUB8 : -1;
		case 16: return MatchesFixedWidthEncoding(fixup, 16, 16, 0, 0, false) ?
			R_RISCV_SUB16 : -1;
		case 32: return MatchesFixedWidthEncoding(fixup, 32, 32, 0, 0, false) ?
			R_RISCV_SUB32 : -1;
		case 64: return MatchesFixedWidthEncoding(fixup, 64, 64, 0, 0, false) ?
			R_RISCV_SUB64 : -1;
		}
	}
	if (fixup.kind == CodeFixupKind::Set) {
		switch (fixup.encoding.storage_bits) {
		case 8: return MatchesFixedWidthEncoding(fixup, 8, 8, 0, 0, false) ?
			R_RISCV_SET8 : -1;
		case 16: return MatchesFixedWidthEncoding(fixup, 16, 16, 0, 0, false) ?
			R_RISCV_SET16 : -1;
		case 32: return MatchesFixedWidthEncoding(fixup, 32, 32, 0, 0, false) ?
			R_RISCV_SET32 : -1;
		}
	}
	return -1;
}

bool RISCVFixupSupportsRelaxation(const CodeFixup& fixup)
{
	switch (RISCVRelocationType(fixup)) {
	case R_RISCV_BRANCH:
	case R_RISCV_JAL:
	case R_RISCV_CALL:
	case R_RISCV_CALL_PLT:
	case R_RISCV_GOT_HI20:
	case R_RISCV_PCREL_HI20:
	case R_RISCV_PCREL_LO12_I:
	case R_RISCV_PCREL_LO12_S:
	case R_RISCV_HI20:
	case R_RISCV_LO12_I:
	case R_RISCV_LO12_S:
	case R_RISCV_RVC_BRANCH:
	case R_RISCV_RVC_JUMP:
		return true;
	default:
		return false;
	}
}

namespace { 

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
	if (lango::HasRISCVExtension(Extensions, lango::RISCVExtension::C)) flags |= 1;
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
