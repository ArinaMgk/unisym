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

namespace {

void EmitDebugType(const insn& instruction, uni::CodeendInterface& codeend)
{
	int32_t type = TYS_ELEMENTS(instruction.operands);
	switch (instruction.opcode) {
	case I_RESB:
		type = TYS_ELEMENTS(instruction.oprs[0].offset) | TY_BYTE;
		break;
	case I_RESW:
		type = TYS_ELEMENTS(instruction.oprs[0].offset) | TY_WORD;
		break;
	case I_RESD:
		type = TYS_ELEMENTS(instruction.oprs[0].offset) | TY_DWORD;
		break;
	case I_RESQ:
		type = TYS_ELEMENTS(instruction.oprs[0].offset) | TY_QWORD;
		break;
	case I_REST:
		type = TYS_ELEMENTS(instruction.oprs[0].offset) | TY_TBYTE;
		break;
	case I_RESO:
		type = TYS_ELEMENTS(instruction.oprs[0].offset) | TY_OWORD;
		break;
	case I_RESY:
		type = TYS_ELEMENTS(instruction.oprs[0].offset) | TY_YWORD;
		break;
	case I_DB:
		type |= TY_BYTE;
		break;
	case I_DW:
		type |= TY_WORD;
		break;
	case I_DD:
		type |= instruction.eops_float ? TY_FLOAT : TY_DWORD;
		break;
	case I_DQ:
		type |= TY_QWORD;
		break;
	case I_DT:
		type |= TY_TBYTE;
		break;
	case I_DO:
		type |= TY_OWORD;
		break;
	case I_DY:
		type |= TY_YWORD;
		break;
	default:
		type = TY_LABEL;
		break;
	}
	codeend.SetDebugType(type);
}

void DefineEqu(insn& instruction, int parse_pass, X86LabelDefinition define, uni::CodeendInterface& output)
{
	if (parse_pass == 1) {
		if (!instruction.label) {
			aasm_log(_LOG_ERROR, "EQU not preceded by label");
			return;
		}
		if (instruction.label[0] == '.' && instruction.label[1] == '.' &&
			instruction.label[2] != '@') return;

		if (instruction.operands == 1 &&
			(instruction.oprs[0].type & IMMEDIATE) &&
			instruction.oprs[0].wrt == NO_SEG) {
			bool external = !!(instruction.oprs[0].opflags & OPFLAG_EXTERN);
			define(instruction.label, instruction.oprs[0].segment,
				instruction.oprs[0].offset, nullptr, false, external, output);
			return;
		}
		if (instruction.operands == 2 &&
			(instruction.oprs[0].type & IMMEDIATE) &&
			(instruction.oprs[0].type & COLON) &&
			instruction.oprs[0].segment == NO_SEG &&
			instruction.oprs[0].wrt == NO_SEG &&
			(instruction.oprs[1].type & IMMEDIATE) &&
			instruction.oprs[1].segment == NO_SEG &&
			instruction.oprs[1].wrt == NO_SEG) {
			define(instruction.label,
				instruction.oprs[0].offset | SEG_ABS,
				instruction.oprs[1].offset,
				nullptr, false, false, output);
			return;
		}
		aasm_log(_LOG_ERROR, "bad syntax for EQU");
		return;
	}

	if (!instruction.label || instruction.label[0] != '.' ||
		instruction.label[1] != '.' || instruction.label[2] == '@') return;

	if (instruction.operands == 1 &&
		(instruction.oprs[0].type & IMMEDIATE)) {
		Labelsys::Define(instruction.label, instruction.oprs[0].segment,
			instruction.oprs[0].offset, nullptr, false, false, output);
		return;
	}
	if (instruction.operands == 2 &&
		(instruction.oprs[0].type & IMMEDIATE) &&
		(instruction.oprs[0].type & COLON) &&
		instruction.oprs[0].segment == NO_SEG &&
		(instruction.oprs[1].type & IMMEDIATE) &&
		instruction.oprs[1].segment == NO_SEG) {
		Labelsys::Define(instruction.label,
			instruction.oprs[0].offset | SEG_ABS,
			instruction.oprs[1].offset,
			nullptr, false, false, output);
		return;
	}
	aasm_log(_LOG_ERROR, "bad syntax for EQU");
}

}

ArchendX86::ArchendX86() : State(new Implementation())
{
}

ArchendX86::~ArchendX86()
{
	if (State->ForwardReferences) StrbuffFree(State->ForwardReferences);
	delete State;
}

const char* ArchendX86::Name() const
{
	return "x86";
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
	X86Parser::Initialize(*session.Options->Codeend, &session.Location);
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

ArchendLineResult ArchendX86::ProcessLine(AssemblySession& session,
	char* source, int parse_pass, int output_pass)
{
	(void)output_pass;
	ArchendLineResult result;
	X86Instruction instruction;
	X86LabelDefinition define = session.PassNumber > 1 ?
		Labelsys::Redefine : Labelsys::Define;
	uni::CodeendInterface& codeend = *session.Options->Codeend;

	X86Parser::ParseLine(parse_pass, source, &instruction,
		eval_evaluate, define, codeend);

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

	if (instruction.opcode == I_EQU) {
		DefineEqu(instruction, parse_pass, define, codeend);
	}
	else if (parse_pass == 1) {
		int64_t size = X86InstructionSize(session.Location.segment,
			session.Location.offset, session.SectionBits,
			session.CurrentCpu, &instruction);
		if (using_debug_info) EmitDebugType(instruction, codeend);
		if (size != -1) result.size = size;
		else result.valid = false;
	}
	else {
		result.size = X86Assemble(session.Location.segment,
			session.Location.offset, session.SectionBits,
			session.CurrentCpu, &instruction, &codeend, &aasmlist);
	}

	X86Parser::CleanupInstruction(&instruction);
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
