// ASCII C++11 TAB4 LF
// Docutitle: AASM ARM32 Architecture Assembler
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../inc/cpp/Lango/Archend/Archend-arm32.hpp"

namespace uni::lango {

const ArchendInfo& ArchendARM32::DumpArchend() const
{
	static const ArchendInfo info {
		"ARM 32-bit architecture",
		"arm32",
		ArchitectureKind::ARM32,
		32,
		"32",
		"none (instruction encoding not implemented)"
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
	(void)word;
	return false;
}

ArchendLineResult ArchendARM32::ProcessInstruction(AssemblySession& session,
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
