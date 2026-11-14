// ASCII C++11 TAB4 LF
// Docutitle: AASM ARM64 Architecture Assembler
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../inc/cpp/Lango/Archend/Archend-arm64.hpp"

namespace uni::lango {

const ArchendInfo& ArchendARM64::DumpArchend() const
{
	static const ArchendInfo info {
		"ARM 64-bit architecture",
		"arm64",
		ArchitectureKind::ARM64,
		64,
		"64",
		"none (instruction encoding not implemented)"
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
	(void)word;
	return false;
}

ArchendLineResult ArchendARM64::ProcessInstruction(AssemblySession& session,
	char* source, int64_t repeat, int parse_pass, int output_pass)
{
	(void)session;
	(void)source;
	(void)repeat;
	(void)parse_pass;
	(void)output_pass;
	ArchendLineResult result;
	result.valid = false;
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
