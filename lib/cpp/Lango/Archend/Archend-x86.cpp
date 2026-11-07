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

#include "../../../../inc/c/strbuff.h"
#include "../../../../asm/inc/aasm.h"
#include "../../../../inc/cpp/Lango/Archend/x86/Instruction-x86.hpp"
#include "../../../../inc/cpp/Format/Code.hpp"
#include "../../../../inc/cpp/Lango/Archend/Archend-x86.hpp"

_ESYM_C {
extern bool using_debug_info;
}

namespace uni::lango {

class ArchendX86::Implementation {
public:
	struct ForwardReference {
		int LineNumber;
		int Operand;
	};

	Strbuff* ForwardReferences = nullptr;
	const ForwardReference* CurrentForwardReference = nullptr;
};

ArchendX86::ArchendX86() : State(new Implementation())
{
}

ArchendX86::~ArchendX86()
{
	if (State->ForwardReferences) StrbuffFree(State->ForwardReferences);
	delete State;
}

const ArchendInfo& ArchendX86::DumpArchend() const
{
	static const ArchendInfo info {
		"Intel and AMD x86 architecture",
		"x86",
		ArchitectureKind::X86,
		16
	};
	return info;
}

uint32_t ArchendX86::DefaultCpu() const
{
	return IF_PLEVEL;
}

bool ArchendX86::ParseCpu(const char* name, uint32_t& cpu) const
{
	cpu = X86CpuId(name);
	return cpu != IF_PLEVEL;
}

bool ArchendX86::ValidateBits(int bits, uint32_t cpu) const
{
	switch (bits) {
	case 16:
		return true;
	case 32:
		return cpu >= IF_386;
	case 64:
		return cpu >= IF_X86_64;
	default:
		return false;
	}
}

bool ArchendX86::ProcessDefault(char* value) const
{
	tokenval token;
	stdscan_reset();
	stdscan_bufptr = value;
	token.t_type = TOKEN_INVALID;
	if (stdscan(nullptr, &token) != TOKEN_SPECIAL) return false;
	switch (static_cast<int>(token.t_integer)) {
	case S_REL:
		globalrel = 1;
		return true;
	case S_ABS:
		globalrel = 0;
		return true;
	default:
		return false;
	}
}

void ArchendX86::Initialize(AssemblySession& session)
{
	if (!State->ForwardReferences)
		State->ForwardReferences = StrbuffNew(
			sizeof(Implementation::ForwardReference));
	State->CurrentForwardReference = nullptr;
	(void)session;
}

void ArchendX86::BeginPass(AssemblySession& session,
	int parse_pass, int output_pass)
{
	(void)parse_pass;
	(void)output_pass;
	if (session.PassNumber > 1) {
		StrbuffRewind(State->ForwardReferences);
		State->CurrentForwardReference = (Implementation::ForwardReference*)
			StrbuffEalloc(State->ForwardReferences);
	}
}

bool ArchendX86::IsInstructionStart(const char* word) const
{
	char keyword[MAX_KEYWORD + 1];
	struct tokenval value;
	size_t length = StrLength(word);

	if (!length || length > MAX_KEYWORD) return false;
	for (size_t index = 0; index < length; index++)
		keyword[index] = ascii_tolower(word[index]);
	keyword[length] = '\0';

	int token = aasm_token_hash(keyword, &value);
	if (token == TOKEN_INSN || token == TOKEN_PREFIX) return true;
	return token == TOKEN_REG &&
		!(REG_SREG & ~aasm_reg_flags[value.t_integer]);
}

ArchendLineResult ArchendX86::ProcessInstruction(AssemblySession& session,
	char* source, int64_t repeat, int parse_pass, int output_pass)
{
	(void)output_pass;
	ArchendLineResult result;
	X86Instruction instruction;
	uni::CodeendInterface& codeend = session.Options->Target.Codeend();

	X86Parser::ParseLine(parse_pass, source, &instruction, eval_evaluate);
	instruction.times = repeat;

	if (optimizing > 0) {
		if (State->CurrentForwardReference &&
			session.GlobalLineNumber ==
			State->CurrentForwardReference->LineNumber) {
			instruction.forw_ref = true;
			do {
				instruction.oprs[State->CurrentForwardReference->Operand].opflags |=
					OPFLAG_FORWARD;
				State->CurrentForwardReference = (Implementation::ForwardReference*)
					StrbuffEread(State->ForwardReferences);
			} while (State->CurrentForwardReference &&
				State->CurrentForwardReference->LineNumber ==
				session.GlobalLineNumber);
		}
		else {
			instruction.forw_ref = false;
		}

		if (instruction.forw_ref && session.PassNumber == 1) {
			for (int index = 0; index < instruction.operands; index++) {
				if (!(instruction.oprs[index].opflags & OPFLAG_FORWARD)) continue;
				Implementation::ForwardReference* reference =
					(Implementation::ForwardReference*)
					StrbuffEalloc(State->ForwardReferences);
				reference->LineNumber = session.GlobalLineNumber;
				reference->Operand = index;
			}
		}
	}

	if (parse_pass == 1) {
		int64_t size = X86InstructionSize(session.Location.segment,
			session.Location.offset, session.SectionBits,
			session.CurrentCpu, &instruction);
		if (using_debug_info) codeend.SetDebugType(TY_LABEL);
		if (size != -1) result.size = size;
		else result.valid = false;
	}
	else {
		result.size = X86Assemble(session.Location.segment,
			session.Location.offset, session.SectionBits,
			session.CurrentCpu, &instruction, &codeend, session.Listing);
	}

	return result;
}

void ArchendX86::EndPass(AssemblySession& session)
{
	(void)session;
}

void ArchendX86::Cleanup(AssemblySession& session)
{
	(void)session;
	if (State->ForwardReferences) {
		StrbuffFree(State->ForwardReferences);
		State->ForwardReferences = nullptr;
	}
	State->CurrentForwardReference = nullptr;
}

ArchendInterface& X86Archend()
{
	static ArchendX86 archend;
	return archend;
}

}
