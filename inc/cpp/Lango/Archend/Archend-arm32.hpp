// ASCII C++11 TAB4 LF
// Docutitle: AASM ARM32 Architecture Assembler
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

#ifndef UNISYM_LANGO_ARCHEND_ARM32_HPP
#define UNISYM_LANGO_ARCHEND_ARM32_HPP

#include "../Archend.hpp"

namespace uni::lango {

class ArchendARM32 : public ArchendInterface {
public:
	virtual const ArchendInfo& DumpArchend() const override;
	virtual uint32_t DefaultCpu() const override;
	virtual bool ParseCpu(const char* name, uint32_t& cpu) const override;
	virtual bool ValidateBits(int bits, uint32_t cpu) const override;
	virtual bool ProcessDefault(char* value) const override;
	virtual void Initialize(AssemblySession& session) override;
	virtual void BeginPass(AssemblySession& session,
		int parse_pass, int output_pass) override;
	virtual bool IsInstructionStart(const char* word) const override;
	virtual ArchendLineResult ProcessInstruction(AssemblySession& session,
		char* source, int64_t repeat,
		int parse_pass, int output_pass) override;
	virtual void EndPass(AssemblySession& session) override;
	virtual void Cleanup(AssemblySession& session) override;
};

ArchendInterface& ARM32Archend();

}

#endif
