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
	result.size = repeat * 4;
	if (parse_pass == 1) return result;

	uni::CodeendInterface& codeend = session.Options->Target.Codeend();
	for (int64_t index = 0; index < repeat; index++) {
		int64_t offset = session.Location.offset + index * 4;
		RISCVEncodingResult encoded;
		if (!RISCVEncoder::Encode(session, instruction, offset, encoded)) {
			result.valid = false;
			return result;
		}
		if (encoded.fixup != RISCVFixupKind::None) {
			bool jump = encoded.fixup == RISCVFixupKind::Jump;
			uni::CodeFixup fixup {
				session.Location.segment,
				static_cast<uint64_t>(offset),
				jump && instruction.operands[0].reg ?
					uni::CodeFixupKind::Call : uni::CodeFixupKind::Branch,
				{
					32,
					static_cast<uint16_t>(jump ? 21 : 13),
					0,
					1,
					true
				},
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
		session.Listing->EmitCode(offset, output);
		codeend.Emit(output);
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
