// ASCII C++11 TAB4 LF
// Docutitle: AASM RISC-V Architecture Assembler
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../asm/inc/aasm.h"
#include "../../../../inc/cpp/Format/Code.hpp"
#include "../../../../inc/cpp/Lango/Archend/Archend-riscv.hpp"
#include "../../../../inc/cpp/Lango/Archend/riscv/Instruction-riscv.hpp"
#include "../../../../inc/cpp/Lango/Archend/riscv/Target-riscv.hpp"

namespace uni::lango {

namespace {

void AppendUnsigned(char*& cursor, uint64_t value)
{
	char digits[24];
	unsigned count = 0;
	do {
		digits[count++] = static_cast<char>('0' + value % 10);
		value /= 10;
	} while (value);
	while (count) *cursor++ = digits[--count];
}

void MakePairAnchor(char* output, int32_t section, uint64_t offset)
{
	char* cursor = output;
	const char prefix[] = "..@riscv.pcrel.";
	for (const char* item = prefix; *item; item++) *cursor++ = *item;
	if (section < 0) {
		*cursor++ = '-';
		AppendUnsigned(cursor, static_cast<uint64_t>(-static_cast<int64_t>(section)));
	}
	else AppendUnsigned(cursor, static_cast<uint64_t>(section));
	*cursor++ = '.';
	AppendUnsigned(cursor, offset);
	*cursor = '\0';
}

constexpr uint32_t Extension(RISCVExtension extension)
{
	return static_cast<uint32_t>(extension);
}

constexpr uint32_t ImplementedExtensions =
	Extension(RISCVExtension::I) |
	Extension(RISCVExtension::M) |
	Extension(RISCVExtension::A) |
	Extension(RISCVExtension::F) |
	Extension(RISCVExtension::D) |
	Extension(RISCVExtension::C) |
	Extension(RISCVExtension::V) |
	Extension(RISCVExtension::Zicsr);

bool ParseInstructionSet(const char* name, uint32_t& extensions)
{
	if (!name || ascii_tolower(*name) != 'i') return false;
	uint32_t result = Extension(RISCVExtension::I);
	const char* cursor = name + 1;
	const char order[] = "mafdcv";
	int previous = -1;
	while (*cursor && *cursor != '_') {
		char letter = ascii_tolower(*cursor++);
		int current = -1;
		for (int index = 0; order[index]; index++)
			if (order[index] == letter) {
				current = index;
				break;
			}
		if (current < 0) return false;
		if (current <= previous) return false;
		previous = current;
		switch (letter) {
		case 'm': result |= Extension(RISCVExtension::M); break;
		case 'a': result |= Extension(RISCVExtension::A); break;
		case 'f': result |= Extension(RISCVExtension::F); break;
		case 'd': result |= Extension(RISCVExtension::D); break;
		case 'c': result |= Extension(RISCVExtension::C); break;
		case 'v': result |= Extension(RISCVExtension::V); break;
		default: return false;
		}
	}
	while (*cursor) {
		if (*cursor++ != '_') return false;
		const char* start = cursor;
		while (*cursor && *cursor != '_') cursor++;
		size_t length = cursor - start;
		if (length != 5 || StrCompareNInsensitive(start, "zicsr", length))
			return false;
		if (result & Extension(RISCVExtension::Zicsr)) return false;
		result |= Extension(RISCVExtension::Zicsr);
	}
	if (result & ~ImplementedExtensions) return false;
	if (result & Extension(RISCVExtension::D))
		result |= Extension(RISCVExtension::F);
	if (result & Extension(RISCVExtension::V))
		result |= Extension(RISCVExtension::F) | Extension(RISCVExtension::D);
	if (result & (Extension(RISCVExtension::F) |
		Extension(RISCVExtension::D) | Extension(RISCVExtension::V)))
		result |= Extension(RISCVExtension::Zicsr);
	extensions = result;
	return true;
}

}

const ArchendInfo& ArchendRISCV::DumpArchend() const
{
	static const ArchendInfo info {
		"RISC-V base integer architecture",
		"riscv",
		ArchitectureKind::RISCV,
		64
	};
	return info;
}

uint32_t ArchendRISCV::DefaultCpu() const
{
	return Extension(RISCVExtension::I);
}

bool ArchendRISCV::ParseCpu(const char* name, uint32_t& cpu) const
{
	return ParseInstructionSet(name, cpu);
}

bool ArchendRISCV::ValidateBits(int bits, uint32_t cpu) const
{
	if (bits != 32 && bits != 64) return false;
	return (cpu & Extension(RISCVExtension::I)) &&
		!(cpu & ~ImplementedExtensions);
}

uint32_t ArchendRISCV::DefaultAbi(int bits, uint32_t cpu) const
{
	(void)cpu;
	return static_cast<uint32_t>(DefaultRISCVABI(bits));
}

bool ArchendRISCV::ParseAbi(const char* name, uint32_t& abi) const
{
	RISCVABI parsed;
	if (!ParseRISCVABI(name, parsed)) return false;
	abi = static_cast<uint32_t>(parsed);
	return true;
}

bool ArchendRISCV::ValidateAbi(int bits, uint32_t cpu, uint32_t abi) const
{
	return ValidateRISCVABI(bits, cpu, static_cast<RISCVABI>(abi));
}

bool ArchendRISCV::ProcessDefault(char* value) const
{
	(void)value;
	return false;
}

void ArchendRISCV::Initialize(AssemblySession& session)
{
	(void)session;
}

void ArchendRISCV::BeginPass(AssemblySession& session,
	int parse_pass, int output_pass)
{
	(void)session;
	(void)parse_pass;
	(void)output_pass;
}

bool ArchendRISCV::IsInstructionStart(const char* word) const
{
	return RISCVParser::IsInstructionStart(word);
}

ArchendLineResult ArchendRISCV::ProcessInstruction(AssemblySession& session,
	char* source, int64_t repeat, int parse_pass, int output_pass)
{
	(void)output_pass;
	ArchendLineResult result;
	RISCVInstruction instruction;
	if (!RISCVParser::Parse(session, source, parse_pass, instruction)) {
		result.valid = false;
		return result;
	}
	unsigned measured_size;
	if (!RISCVEncoder::Measure(session, instruction, measured_size)) {
		result.valid = false;
		return result;
	}
	int64_t single_size = measured_size;
	result.size = repeat * single_size;
	if (parse_pass == 1) return result;

	uni::CodeendInterface& codeend = session.Options->Target.Codeend();
	for (int64_t index = 0; index < repeat; index++) {
		int64_t offset = session.Location.offset + index * single_size;
		RISCVEncodingSequence sequence;
		if (!RISCVEncoder::Encode(session, instruction, offset, sequence)) {
			result.valid = false;
			return result;
		}
		unsigned encoded_size = 0;
		for (unsigned item = 0; item < sequence.count; item++)
			encoded_size += sequence.instructions[item].size;
		if (encoded_size != measured_size) {
			aasm_log(_LOG_PANIC,
				"RISC-V instruction expansion changed during output");
			result.valid = false;
			return result;
		}
		int64_t instruction_offset = offset;
		for (unsigned item = 0; item < sequence.count; item++) {
			RISCVEncodingResult& encoded = sequence.instructions[item];
			if (encoded.fixup != RISCVFixupKind::None) {
				bool paired = encoded.pair_role != RISCVFixupPairRole::None;
				bool pair_fixup = encoded.fixup == RISCVFixupKind::GotHigh ||
					encoded.fixup == RISCVFixupKind::PcRelativeHigh ||
					encoded.fixup == RISCVFixupKind::PcRelativeLowI ||
					encoded.fixup == RISCVFixupKind::PcRelativeLowS;
				if (paired != pair_fixup) {
					aasm_log(_LOG_PANIC,
						"RISC-V paired fixup metadata is inconsistent");
					result.valid = false;
					return result;
				}
				if (encoded.pair_role == RISCVFixupPairRole::Head) {
					char anchor[96];
					MakePairAnchor(anchor, session.Location.segment,
						encoded.pair_offset);
					codeend.DefineSymbol(anchor, session.Location.segment,
						static_cast<int64_t>(encoded.pair_offset), 0, nullptr);
				}
				uni::CodeFixupKind kind;
				uni::CodeFixupEncoding encoding;
				switch (encoded.fixup) {
				case RISCVFixupKind::Branch:
					kind = uni::CodeFixupKind::Branch;
					encoding = {32, 13, 0, 1, true};
					break;
				case RISCVFixupKind::Jump:
					kind = instruction.operands[0].reg ?
						uni::CodeFixupKind::Call : uni::CodeFixupKind::Branch;
					encoding = {32, 21, 0, 1, true};
					break;
				case RISCVFixupKind::Call:
					kind = uni::CodeFixupKind::Call;
					encoding = {64, 32, 0, 0, true};
					break;
				case RISCVFixupKind::CallPlt:
					kind = uni::CodeFixupKind::Plt;
					encoding = {64, 32, 0, 0, true};
					break;
				case RISCVFixupKind::High:
					kind = uni::CodeFixupKind::High;
					encoding = {32, 20, 12, 12, true};
					break;
				case RISCVFixupKind::LowI:
					kind = uni::CodeFixupKind::Low;
					encoding = {32, 12, 20, 0, true};
					break;
				case RISCVFixupKind::LowS:
					kind = uni::CodeFixupKind::Low;
					encoding = {32, 12, 7, 0, true};
					break;
				case RISCVFixupKind::GotHigh:
					kind = uni::CodeFixupKind::Got;
					encoding = {32, 20, 12, 12, true};
					break;
				case RISCVFixupKind::CompressedBranch:
					kind = uni::CodeFixupKind::Branch;
					encoding = {16, 9, 0, 1, true};
					break;
				case RISCVFixupKind::CompressedJump:
					kind = uni::CodeFixupKind::Branch;
					encoding = {16, 12, 0, 1, true};
					break;
				case RISCVFixupKind::PcRelativeHigh:
					kind = uni::CodeFixupKind::PageRelative;
					encoding = {32, 20, 12, 12, true};
					break;
				case RISCVFixupKind::PcRelativeLowI:
					kind = uni::CodeFixupKind::PageOffset;
					encoding = {32, 12, 20, 0, true};
					break;
				case RISCVFixupKind::PcRelativeLowS:
					kind = uni::CodeFixupKind::PageOffset;
					encoding = {32, 12, 7, 0, true};
					break;
				default:
					aasm_log(_LOG_PANIC, "invalid RISC-V fixup kind");
					result.valid = false;
					return result;
				}
				uni::CodeFixup fixup {
					session.Location.segment,
					static_cast<uint64_t>(instruction_offset),
					kind,
					encoding,
					encoded.encoding,
					encoded.target.section,
					encoded.target.offset,
					0,
					encoded.target.wrt,
					static_cast<uint32_t>(ArchitectureKind::RISCV),
					{
						paired ? session.Location.segment : NO_SEG,
						encoded.pair_offset,
						encoded.pair_role == RISCVFixupPairRole::Head ?
							uni::CodeFixupPairRole::Head :
							(encoded.pair_role == RISCVFixupPairRole::Tail ?
								uni::CodeFixupPairRole::Tail :
								uni::CodeFixupPairRole::None)
					},
					session.Options->EnableRISCVRelax
				};
				session.Listing->EmitFixup(fixup);
				codeend.EmitFixup(fixup);
				instruction_offset += encoded.size;
				continue;
			}
			uint8_t bytes[4] = {
				static_cast<uint8_t>(encoded.encoding),
				static_cast<uint8_t>(encoded.encoding >> 8),
				static_cast<uint8_t>(encoded.encoding >> 16),
				static_cast<uint8_t>(encoded.encoding >> 24)
			};
			uni::CodeendOutput output {
				session.Location.segment,
				bytes,
				uni::CodeendOutputType::RawData,
				encoded.size
			};
			session.Listing->EmitCode(instruction_offset, output);
			codeend.Emit(output);
			instruction_offset += encoded.size;
		}
	}
	return result;
}

void ArchendRISCV::EndPass(AssemblySession& session)
{
	(void)session;
}

void ArchendRISCV::Cleanup(AssemblySession& session)
{
	(void)session;
}

ArchendInterface& RISCVArchend()
{
	static ArchendRISCV archend;
	return archend;
}

}
