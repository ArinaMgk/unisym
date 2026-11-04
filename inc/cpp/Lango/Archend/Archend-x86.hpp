// ASCII C++11 TAB4 LF
// Docutitle: AASM x86 Architecture Assembler
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

#ifndef UNISYM_LANGO_ARCHEND_X86_HPP
#define UNISYM_LANGO_ARCHEND_X86_HPP

#include "../Archend.hpp"

namespace uni::lango {

// Adapter exposing the existing x86 parser and encoder through ArchendInterface.
class ArchendX86 : public ArchendInterface {
private:
	class Implementation;
	Implementation* State;

public:
	ArchendX86();
	virtual ~ArchendX86() override;
	ArchendX86(const ArchendX86&) = delete;
	ArchendX86& operator=(const ArchendX86&) = delete;

	virtual const char* Name() const override;
	virtual uint32_t DefaultCpu() const override;
	virtual bool ParseCpu(const char* name, uint32_t& cpu) const override;
	virtual bool ValidateBits(int bits, uint32_t cpu) const override;
	virtual bool ProcessDefault(char* value) const override;
	virtual void Initialize(AssemblySession& session) override;
	virtual void BeginPass(AssemblySession& session,
		int parse_pass, int output_pass) override;
	virtual ArchendLineResult ProcessLine(AssemblySession& session,
		char* source, int parse_pass, int output_pass) override;
	virtual void EndPass(AssemblySession& session) override;
	virtual void Cleanup(AssemblySession& session) override;
};

ArchendInterface& X86Archend();

}

#endif
