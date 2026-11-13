// ASCII C++11 TAB4 LF
// Docutitle: Lango Architecture Assembler Interface
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0
/*
	Copyright 2026 ArinaMgk

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0
	http://unisym.org/license.html

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/

#ifndef UNISYM_LANGO_ARCHEND_HPP
#define UNISYM_LANGO_ARCHEND_HPP

#include <stdio.h>
#include "../../c/stdinc.h"
#include "./Target.hpp"

struct AssemblySession;

namespace uni::lango {

enum class AssemblySignedness : uint8_t {
	Default,
	Signed,
	Unsigned
};

struct ArchendInfo {
	const char* fullname;
	const char* shortname;
	ArchitectureKind architecture;
	int default_bits;
};

// Result of parsing and encoding one architecture-specific source line.
struct ArchendLineResult {
	int64_t size = 0;
	bool valid = true;
};

// Common interface implemented by architecture-specific assemblers.
class ArchendInterface {
public:
	virtual ~ArchendInterface() = default;

	virtual const ArchendInfo& DumpArchend() const = 0;
	virtual uint32_t DefaultCpu() const = 0;
	virtual bool ParseCpu(const char* name, uint32_t& cpu) const = 0;
	virtual bool ValidateBits(int bits, uint32_t cpu) const = 0;
	virtual uint32_t DefaultAbi(int bits, uint32_t cpu) const
	{
		(void)bits;
		(void)cpu;
		return 0;
	}
	virtual bool ParseAbi(const char* name, uint32_t& abi) const
	{
		(void)name;
		(void)abi;
		return false;
	}
	virtual bool ValidateAbi(int bits, uint32_t cpu, uint32_t abi) const
	{
		(void)bits;
		(void)cpu;
		return abi == 0;
	}
	virtual bool ProcessDefault(char* value) const = 0;
	virtual void Initialize(AssemblySession& session) = 0;
	virtual void BeginPass(AssemblySession& session,
		int parse_pass, int output_pass) = 0;
	virtual bool ProcessArchitectureStatement(AssemblySession& session,
		char* source, int64_t repeat, int parse_pass, int output_pass,
		ArchendLineResult& result)
	{
		(void)session;
		(void)source;
		(void)repeat;
		(void)parse_pass;
		(void)output_pass;
		(void)result;
		return false;
	}
	virtual bool IsArchitectureStatementStart(const char* word) const
	{
		(void)word;
		return false;
	}
	virtual bool IsInstructionStart(const char* word) const = 0;
	virtual ArchendLineResult ProcessInstruction(AssemblySession& session,
		char* source, int64_t repeat,
		int parse_pass, int output_pass) = 0;
	virtual void EndPass(AssemblySession& session) = 0;
	virtual void Cleanup(AssemblySession& session) = 0;
};

ArchendInterface& DefaultArchend();
ArchendInterface* FindArchend(const char* name);
void ListArchends(const ArchendInterface& selected, FILE* output);

}

#endif
