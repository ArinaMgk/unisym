// ASCII C++11 TAB4 LF
// Docutitle: AASM ARM32 Architecture Assembler
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../asm/inc/aasm.h"
#include "../../../../inc/cpp/Format/Code.hpp"
#include "../../../../inc/cpp/Lango/Archend/Archend-arm32.hpp"
#include "../../../../inc/cpp/Lango/Archend/arm32/Instruction-arm32.hpp"

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

const ArchendInfo& ArchendARM32::DumpArchend() const
{
	static const ArchendInfo info {
		"ARM 32-bit architecture",
		"arm32",
		ArchitectureKind::ARM32,
		32,
		"32",
		"cortex-m7",
		"ARMv7-A A32 and Cortex-M7 T32 integer subsets"
	};
	return info;
}

uint32_t ArchendARM32::DefaultCpu() const
{
	return 0;
}

bool ArchendARM32::ParseCpu(const char* name, uint32_t& cpu) const
{
	(void)name;
	(void)cpu;
	return false;
}

bool ArchendARM32::ParseCore(const char* name, uint32_t& core) const
{
	if (!StrCompareInsensitive(name, "cortex-m7") ||
		!StrCompareInsensitive(name, "cortexm7")) {
		core = static_cast<uint32_t>(ARM32Core::CortexM7);
		return true;
	}
	return false;
}

bool ArchendARM32::ParseInstructionSet(const char* name, uint32_t& set) const
{
	if (!StrCompareInsensitive(name, "a32")) {
		set = static_cast<uint32_t>(ARM32InstructionSet::A32);
		return true;
	}
	if (!StrCompareInsensitive(name, "t32")) {
		set = static_cast<uint32_t>(ARM32InstructionSet::T32);
		return true;
	}
	return false;
}

uint32_t ArchendARM32::DefaultInstructionSet(uint32_t core) const
{
	return core == static_cast<uint32_t>(ARM32Core::CortexM7) ?
		static_cast<uint32_t>(ARM32InstructionSet::T32) :
		static_cast<uint32_t>(ARM32InstructionSet::A32);
}

bool ArchendARM32::ValidateCore(uint32_t core, uint32_t set) const
{
	if (core == static_cast<uint32_t>(ARM32Core::Generic)) return true;
	return core == static_cast<uint32_t>(ARM32Core::CortexM7) &&
		set == static_cast<uint32_t>(ARM32InstructionSet::T32);
}

bool ArchendARM32::ValidateInstructionSet(uint32_t core, uint32_t set) const
{
	if (set == static_cast<uint32_t>(ARM32InstructionSet::A32))
		return core == static_cast<uint32_t>(ARM32Core::Generic);
	return set == static_cast<uint32_t>(ARM32InstructionSet::T32) &&
		core == static_cast<uint32_t>(ARM32Core::CortexM7);
}

bool ArchendARM32::ValidateBits(int bits, uint32_t cpu) const
{
	(void)cpu;
	return bits == 32;
}

bool ArchendARM32::ProcessDefault(char* value) const
{
	(void)value;
	return false;
}

void ArchendARM32::Initialize(AssemblySession& session)
{
	(void)session;
}

void ArchendARM32::BeginPass(AssemblySession& session,
	int parse_pass, int output_pass)
{
	(void)session;
	(void)parse_pass;
	(void)output_pass;
}

bool ArchendARM32::IsInstructionStart(const char* word) const
{
	return ARM32Parser::IsInstructionStart(word);
}

ArchendLineResult ArchendARM32::ProcessInstruction(AssemblySession& session,
	char* source, int64_t repeat, int parse_pass, int output_pass)
{
	(void)output_pass;
	ArchendLineResult result;
	ARM32Instruction instruction;
	if (!ARM32Parser::Parse(session, source, parse_pass, instruction)) {
		result.valid = false;
		return result;
	}
	int64_t measured_offset = session.Location.offset;
	for (int64_t index = 0; index < repeat; index++) {
		unsigned measured_size;
		if (!ARM32Encoder::Measure(
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
		if (!ARM32Encoder::Measure(
			session, instruction, offset, measured_size)) {
			result.valid = false;
			return result;
		}
		ARM32EncodingSequence sequence;
		if (!ARM32Encoder::Encode(session, instruction, offset, sequence)) {
			result.valid = false;
			return result;
		}
		unsigned encoded_size = 0;
		for (unsigned item = 0; item < sequence.count; item++)
			encoded_size += sequence.instructions[item].size;
		if (encoded_size != measured_size) {
			aasm_log(_LOG_PANIC,
				"ARM32 instruction expansion changed during output");
			result.valid = false;
			return result;
		}
		int64_t instruction_offset = offset;
		for (unsigned item = 0; item < sequence.count; item++) {
			ARM32EncodingResult& encoded = sequence.instructions[item];
			if (encoded.fixup != ARM32FixupKind::None) {
				uni::CodeFixupKind kind;
				uni::CodeFixupEncoding encoding {};
				encoding.storage_bits = encoded.size * 8;
				if (encoded.fixup == ARM32FixupKind::Branch ||
					encoded.fixup == ARM32FixupKind::Call) {
					kind = encoded.fixup == ARM32FixupKind::Call ?
						uni::CodeFixupKind::Call : uni::CodeFixupKind::Branch;
					encoding.value_bits = encoded.size == 2 ?
						(instruction.condition == ARM32Condition::AL ? 12 : 9) : 26;
					encoding.right_shift = encoded.size == 2 ? 1 : 2;
					encoding.signed_value = true;
					encoding.fields[0] = {0,
						static_cast<uint8_t>(encoded.size == 2 ?
							(instruction.condition == ARM32Condition::AL ? 11 : 8) : 24), 0};
					encoding.field_count = 1;
				}
				else {
					kind = encoded.fixup == ARM32FixupKind::AddressLow ?
						uni::CodeFixupKind::Low : uni::CodeFixupKind::High;
					encoding.value_bits = 32;
					encoding.fields[0] = {0, 12, 0};
					encoding.fields[1] = {12, 4, 16};
					encoding.field_count = 2;
				}
				bool paired = encoded.pair_role != ARM32FixupPairRole::None;
				uni::CodeFixup fixup {
					session.Location.segment,
					static_cast<uint64_t>(instruction_offset),
					kind,
					encoding,
					encoded.encoding,
					encoded.target.section,
					encoded.target.offset,
					encoded.fixup == ARM32FixupKind::Branch ||
						encoded.fixup == ARM32FixupKind::Call ?
							(encoded.size == 2 ? 4 : 8) : 0,
					encoded.target.wrt,
					static_cast<uint32_t>(ArchitectureKind::ARM32),
					{
						static_cast<int32_t>(paired ?
							session.Location.segment : NO_SEG),
						encoded.pair_offset,
						encoded.pair_role == ARM32FixupPairRole::Head ?
							uni::CodeFixupPairRole::Head :
							(encoded.pair_role == ARM32FixupPairRole::Tail ?
								uni::CodeFixupPairRole::Tail :
								uni::CodeFixupPairRole::None)
					}
				};
				session.Listing->EmitFixup(fixup);
				SetStatementDebugLine(session);
				codeend.EmitFixup(fixup);
				instruction_offset += encoded.size;
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
			encoded.size
			};
			session.Listing->EmitCode(instruction_offset, output);
			SetStatementDebugLine(session);
			codeend.Emit(output);
			instruction_offset += encoded.size;
		}
		offset += measured_size;
	}
	return result;
}

void ArchendARM32::EndPass(AssemblySession& session)
{
	(void)session;
}

void ArchendARM32::Cleanup(AssemblySession& session)
{
	(void)session;
}

ArchendInterface& ARM32Archend()
{
	static ArchendARM32 archend;
	return archend;
}

}
