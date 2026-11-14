// ASCII C++11 TAB4 LF
// Docutitle: AASM ARM64 Architecture Assembler
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../asm/inc/aasm.h"
#include "../../../../inc/cpp/Format/Code.hpp"
#include "../../../../inc/cpp/Lango/Archend/Archend-arm64.hpp"
#include "../../../../inc/cpp/Lango/Archend/arm64/Instruction-arm64.hpp"

namespace uni::lango {

namespace {

void SetStatementDebugLine(AssemblySession& session)
{
	static int32_t line_number = 0;
	static char* line_filename = nullptr;
	if (src_get(&line_number, &line_filename))
		session.Options->Target.Codeend().SetDebugLine(
			line_filename, line_number, session.Location.segment);
}

}

const ArchendInfo& ArchendARM64::DumpArchend() const
{
	static const ArchendInfo info {
		"ARM 64-bit architecture",
		"arm64",
		ArchitectureKind::ARM64,
		64,
		"64",
		"A64 minimal integer and load/store subset"
	};
	return info;
}

uint32_t ArchendARM64::DefaultCpu() const
{
	return 0;
}

bool ArchendARM64::ParseCpu(const char* name, uint32_t& cpu) const
{
	(void)name;
	(void)cpu;
	return false;
}

bool ArchendARM64::ValidateBits(int bits, uint32_t cpu) const
{
	(void)cpu;
	return bits == 64;
}

bool ArchendARM64::ProcessDefault(char* value) const
{
	(void)value;
	return false;
}

void ArchendARM64::Initialize(AssemblySession& session)
{
	(void)session;
}

void ArchendARM64::BeginPass(AssemblySession& session,
	int parse_pass, int output_pass)
{
	(void)session;
	(void)parse_pass;
	(void)output_pass;
}

bool ArchendARM64::IsInstructionStart(const char* word) const
{
	return ARM64Parser::IsInstructionStart(word);
}

ArchendLineResult ArchendARM64::ProcessInstruction(AssemblySession& session,
	char* source, int64_t repeat, int parse_pass, int output_pass)
{
	(void)output_pass;
	ArchendLineResult result;
	ARM64Instruction instruction;
	if (!ARM64Parser::Parse(session, source, parse_pass, instruction)) {
		result.valid = false;
		return result;
	}
	int64_t measured_offset = session.Location.offset;
	for (int64_t index = 0; index < repeat; index++) {
		unsigned measured_size;
		if (!ARM64Encoder::Measure(
			session, instruction, measured_offset, measured_size)) {
			result.valid = false;
			return result;
		}
		result.size += measured_size;
		measured_offset += measured_size;
	}
	if (parse_pass == 1) return result;

	uni::CodeendInterface& codeend = session.Options->Target.Codeend();
	int64_t offset = session.Location.offset;
	for (int64_t index = 0; index < repeat; index++) {
		unsigned measured_size;
		if (!ARM64Encoder::Measure(
			session, instruction, offset, measured_size)) {
			result.valid = false;
			return result;
		}
		ARM64EncodingSequence sequence;
		if (!ARM64Encoder::Encode(session, instruction, offset, sequence)) {
			result.valid = false;
			return result;
		}
		if (sequence.count * 4 != measured_size) {
			aasm_log(_LOG_PANIC,
				"ARM64 instruction expansion changed during output");
			result.valid = false;
			return result;
		}
		int64_t instruction_offset = offset;
		for (unsigned item = 0; item < sequence.count; item++) {
			ARM64EncodingResult& encoded = sequence.instructions[item];
			if (encoded.fixup != ARM64FixupKind::None) {
				uni::CodeFixupKind kind;
				uni::CodeFixupEncoding encoding {};
				encoding.storage_bits = 32;
				switch (encoded.fixup) {
				case ARM64FixupKind::Branch:
				case ARM64FixupKind::Call:
					kind = encoded.fixup == ARM64FixupKind::Call ?
						uni::CodeFixupKind::Call : uni::CodeFixupKind::Branch;
					encoding.value_bits = 28;
					encoding.right_shift = 2;
					encoding.signed_value = true;
					encoding.fields[0] = {0, 26, 0};
					encoding.field_count = 1;
					break;
				case ARM64FixupKind::ConditionalBranch:
					kind = uni::CodeFixupKind::Branch;
					encoding.value_bits = 21;
					encoding.right_shift = 2;
					encoding.signed_value = true;
					encoding.fields[0] = {0, 19, 5};
					encoding.field_count = 1;
					break;
				case ARM64FixupKind::PageRelative:
					kind = uni::CodeFixupKind::PageRelative;
					encoding.value_bits = 33;
					encoding.right_shift = 12;
					encoding.signed_value = true;
					encoding.fields[0] = {0, 2, 29};
					encoding.fields[1] = {2, 19, 5};
					encoding.field_count = 2;
					break;
				case ARM64FixupKind::PageOffset:
					kind = uni::CodeFixupKind::PageOffset;
					encoding.value_bits = 12;
					encoding.fields[0] = {0, 12, 10};
					encoding.field_count = 1;
					break;
				default:
					aasm_log(_LOG_PANIC, "invalid ARM64 fixup kind");
					result.valid = false;
					return result;
				}
				bool paired = encoded.pair_role != ARM64FixupPairRole::None;
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
					static_cast<uint32_t>(ArchitectureKind::ARM64),
					{
						static_cast<int32_t>(paired ?
							session.Location.segment : NO_SEG),
						encoded.pair_offset,
						encoded.pair_role == ARM64FixupPairRole::Head ?
							uni::CodeFixupPairRole::Head :
							(encoded.pair_role == ARM64FixupPairRole::Tail ?
								uni::CodeFixupPairRole::Tail :
								uni::CodeFixupPairRole::None)
					}
				};
				session.Listing->EmitFixup(fixup);
				SetStatementDebugLine(session);
				codeend.EmitFixup(fixup);
				instruction_offset += 4;
				continue;
			}
			uint32_t encoding = encoded.encoding;
			uint8_t bytes[4] = {
				static_cast<uint8_t>(encoding),
				static_cast<uint8_t>(encoding >> 8),
				static_cast<uint8_t>(encoding >> 16),
				static_cast<uint8_t>(encoding >> 24)
			};
			uni::CodeendOutput output {
				session.Location.segment,
				bytes,
				uni::CodeendOutputType::Instruction,
				sizeof(bytes)
			};
			session.Listing->EmitCode(instruction_offset, output);
			SetStatementDebugLine(session);
			codeend.Emit(output);
			instruction_offset += sizeof(bytes);
		}
		offset += measured_size;
	}
	return result;
}

void ArchendARM64::EndPass(AssemblySession& session)
{
	(void)session;
}

void ArchendARM64::Cleanup(AssemblySession& session)
{
	(void)session;
}

ArchendInterface& ARM64Archend()
{
	static ArchendARM64 archend;
	return archend;
}

}
