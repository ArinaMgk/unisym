// ASCII C++11 TAB4 LF
// Docutitle: AASM RISC-V Architecture Assembler
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../asm/inc/aasm.h"
#include "../../../../inc/cpp/Format/Code.hpp"
#include "../../../../inc/cpp/Lango/Archend/Archend-riscv.hpp"
#include "../../../../inc/cpp/Lango/Archend/riscv/Instruction-riscv.hpp"

namespace uni::lango {

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
	return 0;
}

bool ArchendRISCV::ParseCpu(const char* name, uint32_t& cpu) const
{
	if (!StrCompareInsensitive(name, "i")) cpu = 0;
	else return false;
	return true;
}

bool ArchendRISCV::ValidateBits(int bits, uint32_t cpu) const
{
	if (bits != 32 && bits != 64) return false;
	return !cpu || cpu == static_cast<uint32_t>(bits);
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
	unsigned instruction_count;
	if (!RISCVEncoder::Measure(session, instruction, instruction_count)) {
		result.valid = false;
		return result;
	}
	int64_t single_size = static_cast<int64_t>(instruction_count) * 4;
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
		if (sequence.count != instruction_count) {
			aasm_log(_LOG_PANIC,
				"RISC-V instruction expansion changed during output");
			result.valid = false;
			return result;
		}
		for (unsigned item = 0; item < sequence.count; item++) {
			RISCVEncodingResult& encoded = sequence.instructions[item];
			int64_t instruction_offset = offset + item * 4;
			if (encoded.fixup != RISCVFixupKind::None) {
				if (encoded.fixup == RISCVFixupKind::PcRelativeHigh) {
					char anchor[96];
					snprintf(anchor, sizeof(anchor), "..@riscv.pcrel.%d.%lld",
						session.Location.segment,
						static_cast<long long>(instruction_offset));
					codeend.DefineSymbol(anchor, session.Location.segment,
						instruction_offset, 0, nullptr);
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
					static_cast<uint32_t>(ArchitectureKind::RISCV)
				};
				session.Listing->EmitFixup(fixup);
				codeend.EmitFixup(fixup);
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
				sizeof(bytes)
			};
			session.Listing->EmitCode(instruction_offset, output);
			codeend.Emit(output);
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
