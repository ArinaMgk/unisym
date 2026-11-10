// ASCII C++11 TAB4 LF
// Docutitle: AASM RISC-V Instruction Parser
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../../asm/inc/aasm.h"
#include "../../../../../inc/cpp/Lango/Archend/riscv/Instruction-riscv.hpp"

namespace uni::lango {

namespace {

char* SkipSpace(char* text)
{
	while (*text && ascii_isspace(*text)) text++;
	return text;
}

void TrimEnd(char* text)
{
	size_t length = StrLength(text);
	while (length && ascii_isspace(text[length - 1])) text[--length] = '\0';
}

bool Equal(const char* left, const char* right)
{
	return !StrCompareInsensitive(left, right);
}

bool IsAlias(const char* name)
{
	static const char* const aliases[] = {
		"j", "jr", "mov", "nop", "ret"
	};
	for (const char* alias : aliases)
		if (Equal(name, alias)) return true;
	return false;
}

bool ParseRegister(const char* text, uint8_t& reg)
{
	if ((text[0] == 'x' || text[0] == 'X') && text[1]) {
		char* end;
		long number = strtol(text + 1, &end, 10);
		if (!*end && number >= 0 && number <= 31) {
			reg = static_cast<uint8_t>(number);
			return true;
		}
	}
	struct RegisterEntry {
		const char* name;
		uint8_t number;
	};
	static const RegisterEntry registers[] = {
		{"zero", 0}, {"ra", 1}, {"sp", 2}, {"gp", 3}, {"tp", 4},
		{"t0", 5}, {"t1", 6}, {"t2", 7}, {"s0", 8}, {"fp", 8},
		{"s1", 9}, {"a0", 10}, {"a1", 11}, {"a2", 12}, {"a3", 13},
		{"a4", 14}, {"a5", 15}, {"a6", 16}, {"a7", 17},
		{"s2", 18}, {"s3", 19}, {"s4", 20}, {"s5", 21},
		{"s6", 22}, {"s7", 23}, {"s8", 24}, {"s9", 25},
		{"s10", 26}, {"s11", 27}, {"t3", 28}, {"t4", 29},
		{"t5", 30}, {"t6", 31}
	};
	for (const RegisterEntry& entry : registers) {
		if (Equal(text, entry.name)) {
			reg = entry.number;
			return true;
		}
	}
	return false;
}

bool ParseValue(char* text, int parse_pass, RISCVValue& result)
{
	text = SkipSpace(text);
	TrimEnd(text);
	stdscan_reset();
	stdscan_bufptr = text;
	struct tokenval token;
	token.t_type = TOKEN_INVALID;
	int flags = 0;
	expr* value = eval_evaluate(common_stdscan, nullptr, &token, &flags,
		parse_pass == 2 ? 2 : 0, nullptr);
	if (!value || token.t_type != TOKEN_EOS) return false;
	result.external = !!(flags & OPFLAG_EXTERN);
	if (is_unknown(value)) {
		result.unknown = true;
		return true;
	}
	if (!is_reloc(value)) return false;
	result.offset = reloc_value(value);
	result.section = reloc_seg(value);
	result.wrt = reloc_wrt(value);
	return true;
}

bool ParseWidth(char*& text, uint8_t& width)
{
	char* start = SkipSpace(text);
	char* end = start;
	while (isidchar(*end)) end++;
	if (end == start) return false;
	char saved = *end;
	*end = '\0';
	if (Equal(start, "byte")) width = 1;
	else if (Equal(start, "byt2")) width = 2;
	else if (Equal(start, "byt4")) width = 4;
	else if (Equal(start, "byt8")) width = 8;
	else if (Equal(start, "byt10")) width = 10;
	else if (Equal(start, "byt16")) width = 16;
	else if (Equal(start, "byt32")) width = 32;
	else {
		*end = saved;
		return false;
	}
	*end = saved;
	text = SkipSpace(end);
	return true;
}

bool ReadSignedness(const char* text, size_t length,
	AssemblySignedness& signedness)
{
	static const char signed_name[] = "signed";
	static const char unsigned_name[] = "unsigned";
	const char* name = nullptr;
	AssemblySignedness value = AssemblySignedness::Default;
	if (length == sizeof(signed_name) - 1) {
		name = signed_name;
		value = AssemblySignedness::Signed;
	}
	else if (length == sizeof(unsigned_name) - 1) {
		name = unsigned_name;
		value = AssemblySignedness::Unsigned;
	}
	if (!name) return false;
	for (size_t index = 0; index < length; index++)
		if (ascii_tolower(text[index]) != name[index]) return false;
	signedness = value;
	return true;
}

bool ExtractSignedness(char* source, AssemblySignedness& signedness)
{
	unsigned brackets = 0;
	for (char* cursor = source; *cursor && *cursor != ';';) {
		if (*cursor == '[') {
			brackets++;
			cursor++;
			continue;
		}
		if (*cursor == ']') {
			if (brackets) brackets--;
			cursor++;
			continue;
		}
		if (brackets || !isidchar(*cursor)) {
			cursor++;
			continue;
		}
		char* start = cursor;
		while (isidchar(*cursor)) cursor++;
		AssemblySignedness value;
		if (!ReadSignedness(start, cursor - start, value)) continue;
		if (signedness != AssemblySignedness::Default) {
			aasm_log(_LOG_ERROR,
				"RISC-V instruction has multiple signedness qualifiers");
			return false;
		}
		signedness = value;
		while (start != cursor) *start++ = ' ';
	}
	return true;
}

bool ParseRegisterOperand(char* text, RISCVOperand& operand)
{
	text = SkipSpace(text);
	TrimEnd(text);
	uint8_t reg;
	if (!ParseRegister(text, reg)) return false;
	operand.kind = RISCVOperandKind::Register;
	operand.reg = reg;
	return true;
}

bool ParseImmediateOperand(char* text, int parse_pass, RISCVOperand& operand)
{
	operand.kind = RISCVOperandKind::Immediate;
	return ParseValue(text, parse_pass, operand.value);
}

bool ParseTargetOperand(char* text, int parse_pass, RISCVOperand& operand)
{
	operand.kind = RISCVOperandKind::Target;
	return ParseValue(text, parse_pass, operand.value);
}

bool ParseIndirectTargetOperand(char* text, int parse_pass,
	RISCVOperand& operand)
{
	text = SkipSpace(text);
	TrimEnd(text);
	char* register_end = text;
	while (isidchar(*register_end)) register_end++;
	char saved = *register_end;
	*register_end = '\0';
	if (!ParseRegister(text, operand.reg)) {
		*register_end = saved;
		return false;
	}
	*register_end = saved;
	operand.kind = RISCVOperandKind::IndirectTarget;
	char* displacement = SkipSpace(register_end);
	if (!*displacement) return true;
	if (*displacement != '+' && *displacement != '-') return false;
	return ParseValue(displacement, parse_pass, operand.value);
}

bool ParseAddressOperand(char* text, int parse_pass, bool move_operand,
	RISCVOperand& operand)
{
	text = SkipSpace(text);
	if (move_operand) {
		if (!ParseWidth(text, operand.width)) return false;
	}
	if (*text != '[') return false;
	char* close = text + StrLength(text);
	while (close > text && ascii_isspace(close[-1])) close--;
	if (close <= text + 1 || close[-1] != ']') return false;
	close[-1] = '\0';
	char* content = SkipSpace(text + 1);
	TrimEnd(content);

	operand.kind = RISCVOperandKind::Address;
	char* register_end = content;
	while (isidchar(*register_end)) register_end++;
	char saved = *register_end;
	*register_end = '\0';
	if (ParseRegister(content, operand.reg)) {
		*register_end = saved;
		operand.address_kind = RISCVAddressKind::RegisterRelative;
		char* displacement = SkipSpace(register_end);
		if (!*displacement) return true;
		if (*displacement != '+' && *displacement != '-') return false;
		return ParseValue(displacement, parse_pass, operand.value);
	}
	*register_end = saved;
	operand.address_kind = RISCVAddressKind::Expression;
	return ParseValue(content, parse_pass, operand.value);
}

unsigned SplitOperands(char* text, char* operands[], unsigned capacity)
{
	text = SkipSpace(text);
	if (!*text || *text == ';') return 0;
	unsigned count = 0;
	int brackets = 0;
	char* start = text;
	for (char* cursor = text; ; cursor++) {
		if (*cursor == '[') brackets++;
		else if (*cursor == ']') brackets--;
		if ((!*cursor || *cursor == ';' || (*cursor == ',' && !brackets))) {
			if (count >= capacity) return capacity + 1;
			char end = *cursor;
			*cursor = '\0';
			operands[count] = SkipSpace(start);
			TrimEnd(operands[count]);
			if (!*operands[count]) return capacity + 1;
			count++;
			if (!end || end == ';') break;
			start = cursor + 1;
		}
	}
	return brackets ? capacity + 1 : count;
}

bool RequireRegister(char* text, RISCVOperand& operand, unsigned number)
{
	if (ParseRegisterOperand(text, operand)) return true;
	aasm_log(_LOG_ERROR, "RISC-V operand %u: register expected", number);
	return false;
}

bool RequireImmediate(char* text, int parse_pass,
	RISCVOperand& operand, unsigned number)
{
	if (ParseImmediateOperand(text, parse_pass, operand)) return true;
	aasm_log(_LOG_ERROR, "RISC-V operand %u: immediate expression expected", number);
	return false;
}

bool RequireTarget(char* text, int parse_pass,
	RISCVOperand& operand, unsigned number)
{
	if (ParseTargetOperand(text, parse_pass, operand)) return true;
	aasm_log(_LOG_ERROR, "RISC-V operand %u: target expression expected", number);
	return false;
}

bool RequireIndirectTarget(char* text, int parse_pass,
	RISCVOperand& operand, unsigned number)
{
	if (ParseIndirectTargetOperand(text, parse_pass, operand)) return true;
	aasm_log(_LOG_ERROR,
		"RISC-V operand %u: indirect target expression expected", number);
	return false;
}

bool RequireAddress(char* text, int parse_pass, bool move_operand,
	RISCVOperand& operand, unsigned number)
{
	if (ParseAddressOperand(text, parse_pass, move_operand, operand)) return true;
	aasm_log(_LOG_ERROR, "RISC-V operand %u: address expression expected", number);
	return false;
}

bool ValidateStoreTemporary(const RISCVOperand& source,
	const RISCVOperand& temporary)
{
	if (!temporary.reg) {
		aasm_log(_LOG_ERROR,
			"RISC-V expression-address store temporary register cannot be zero");
		return false;
	}
	if (temporary.reg == source.reg) {
		aasm_log(_LOG_ERROR,
			"RISC-V expression-address store temporary register overlaps the source");
		return false;
	}
	return true;
}

RISCVOpcode LoadOpcode(unsigned width, bool zero_extend, int bits)
{
	switch (width) {
	case 1: return zero_extend ? RISCVOpcode::Lbu : RISCVOpcode::Lb;
	case 2: return zero_extend ? RISCVOpcode::Lhu : RISCVOpcode::Lh;
	case 4: return zero_extend && bits == 64 ? RISCVOpcode::Lwu : RISCVOpcode::Lw;
	case 8: return bits == 64 ? RISCVOpcode::Ld : RISCVOpcode::None;
	default: return RISCVOpcode::None;
	}
}

RISCVOpcode StoreOpcode(unsigned width)
{
	switch (width) {
	case 1: return RISCVOpcode::Sb;
	case 2: return RISCVOpcode::Sh;
	case 4: return RISCVOpcode::Sw;
	case 8: return RISCVOpcode::Sd;
	default: return RISCVOpcode::None;
	}
}

bool ApplySignedness(RISCVInstruction& instruction, int bits)
{
	if (instruction.signedness == AssemblySignedness::Default) return true;
	bool use_unsigned = instruction.signedness == AssemblySignedness::Unsigned;
	const RISCVInstructionInfo& info = DumpRISCVInstruction(instruction.opcode);
	RISCVOpcode variant = use_unsigned ?
		info.unsigned_variant : info.signed_variant;
	if (variant == RISCVOpcode::None) {
		aasm_log(_LOG_ERROR,
			"RISC-V instruction does not have a signedness variant");
		return false;
	}
	if (bits == 32 && instruction.opcode == RISCVOpcode::Lw && use_unsigned)
		variant = RISCVOpcode::Lw;
	instruction.opcode = variant;
	return true;
}

}

bool RISCVParser::IsInstructionStart(const char* word)
{
	return FindRISCVInstruction(word) || IsAlias(word) ||
		Equal(word, "signed") || Equal(word, "unsigned");
}

bool RISCVParser::Parse(AssemblySession& session, char* source,
	int parse_pass, RISCVInstruction& instruction)
{
	if (!ExtractSignedness(source, instruction.signedness)) return false;
	source = SkipSpace(source);
	char* mnemonic_end = source;
	while (isidchar(*mnemonic_end)) mnemonic_end++;
	char saved = *mnemonic_end;
	*mnemonic_end = '\0';
	char mnemonic[16];
	StrCopyN(mnemonic, source, sizeof(mnemonic));
	*mnemonic_end = saved;
	char* operand_text = SkipSpace(mnemonic_end);
	char* operands[3];
	const RISCVInstructionInfo* info = nullptr;
	unsigned count = SplitOperands(operand_text, operands, numsof(operands));
	if (count > numsof(operands)) {
		aasm_log(_LOG_ERROR, "RISC-V: invalid operand list");
		return false;
	}

	if (Equal(mnemonic, "nop")) {
		if (instruction.signedness != AssemblySignedness::Default)
			goto bad_signedness;
		if (count) goto bad_count;
		instruction.opcode = RISCVOpcode::Addi;
		instruction.operand_count = 3;
		instruction.operands[0].kind = RISCVOperandKind::Register;
		instruction.operands[1].kind = RISCVOperandKind::Register;
		instruction.operands[2].kind = RISCVOperandKind::Immediate;
		return true;
	}
	if (Equal(mnemonic, "ret")) {
		if (instruction.signedness != AssemblySignedness::Default)
			goto bad_signedness;
		if (count) goto bad_count;
		instruction.opcode = RISCVOpcode::Jalr;
		instruction.operand_count = 2;
		instruction.operands[0].kind = RISCVOperandKind::Register;
		instruction.operands[1].kind = RISCVOperandKind::IndirectTarget;
		instruction.operands[1].reg = 1;
		return true;
	}
	if (Equal(mnemonic, "j")) {
		if (instruction.signedness != AssemblySignedness::Default)
			goto bad_signedness;
		if (count != 1) goto bad_count;
		instruction.opcode = RISCVOpcode::Jal;
		instruction.operand_count = 2;
		instruction.operands[0].kind = RISCVOperandKind::Register;
		if (!RequireTarget(operands[0], parse_pass,
			instruction.operands[1], 1)) return false;
		return true;
	}
	if (Equal(mnemonic, "jr")) {
		if (instruction.signedness != AssemblySignedness::Default)
			goto bad_signedness;
		if (count != 1) goto bad_count;
		instruction.opcode = RISCVOpcode::Jalr;
		instruction.operand_count = 2;
		instruction.operands[0].kind = RISCVOperandKind::Register;
		return RequireIndirectTarget(operands[0], parse_pass,
			instruction.operands[1], 1);
	}
	if (Equal(mnemonic, "mov")) {
		if (count != 2 && count != 3) goto bad_count;
		RISCVOperand destination;
		if (ParseRegisterOperand(operands[0], destination)) {
			if (count != 2) goto bad_count;
			RISCVOperand source_operand;
			if (ParseRegisterOperand(operands[1], source_operand)) {
				instruction.opcode = RISCVOpcode::Addi;
				instruction.operand_count = 3;
				instruction.operands[0] = destination;
				instruction.operands[1] = source_operand;
				instruction.operands[2].kind = RISCVOperandKind::Immediate;
				return true;
			}
			if (!ParseAddressOperand(operands[1], parse_pass, true,
				source_operand)) {
				if (!ParseImmediateOperand(operands[1], parse_pass, source_operand)) {
					aasm_log(_LOG_ERROR,
						"RISC-V operand 2: expression or address expected");
					return false;
				}
				instruction.kind = RISCVInstructionKind::MoveExpression;
				instruction.operand_count = 2;
				instruction.operands[0] = destination;
				instruction.operands[1] = source_operand;
				return true;
			}
			bool unsigned_load =
				instruction.signedness != AssemblySignedness::Signed;
			instruction.opcode = LoadOpcode(
				source_operand.width, unsigned_load, session.CurrentBits);
			if (instruction.opcode == RISCVOpcode::None) {
				aasm_log(_LOG_ERROR, "RISC-V MOV load width is not supported");
				return false;
			}
			instruction.operand_count = 2;
			instruction.operands[0] = destination;
			instruction.operands[1] = source_operand;
			return true;
		}
		if (!RequireAddress(operands[0], parse_pass, true, destination, 1) ||
			!RequireRegister(operands[1], instruction.operands[0], 2)) return false;
		instruction.opcode = StoreOpcode(destination.width);
		if (instruction.opcode == RISCVOpcode::None) {
			aasm_log(_LOG_ERROR, "RISC-V MOV store width is not supported");
			return false;
		}
		instruction.operand_count = 2;
		instruction.operands[1] = destination;
		if (destination.address_kind == RISCVAddressKind::Expression) {
			if (count != 3) {
				aasm_log(_LOG_ERROR,
					"RISC-V expression-address store requires a temporary register");
				return false;
			}
			instruction.operand_count = 3;
			if (!RequireRegister(operands[2], instruction.operands[2], 3))
				return false;
			if (!ValidateStoreTemporary(instruction.operands[0],
				instruction.operands[2])) return false;
		}
		else if (count != 2) goto bad_count;
		return true;
	}

	info = FindRISCVInstruction(mnemonic);
	if (!info) {
		aasm_log(_LOG_ERROR, "RISC-V instruction expected");
		return false;
	}
	instruction.opcode = info->opcode;
	if (!ApplySignedness(instruction, session.CurrentBits)) return false;
	info = &DumpRISCVInstruction(instruction.opcode);
	if (info->operands == RISCVOperandForm::None) {
		if (count) goto bad_count;
		return true;
	}
	if (info->operands == RISCVOperandForm::RdImmediate) {
		if (count != 2) goto bad_count;
		instruction.operand_count = 2;
		return RequireRegister(operands[0], instruction.operands[0], 1) &&
			RequireImmediate(operands[1], parse_pass, instruction.operands[1], 2);
	}
	if (info->operands == RISCVOperandForm::RdTarget) {
		if (count != 1 && count != 2) goto bad_count;
		instruction.operand_count = 2;
		if (count == 1) {
			instruction.operands[0].kind = RISCVOperandKind::Register;
			instruction.operands[0].reg = 1;
			return RequireTarget(operands[0], parse_pass,
				instruction.operands[1], 1);
		}
		return RequireRegister(operands[0], instruction.operands[0], 1) &&
			RequireTarget(operands[1], parse_pass,
				instruction.operands[1], 2);
	}
	if (info->operands == RISCVOperandForm::RdIndirectTarget) {
		if (count != 2) goto bad_count;
		instruction.operand_count = 2;
		return RequireRegister(operands[0], instruction.operands[0], 1) &&
			RequireIndirectTarget(operands[1], parse_pass,
				instruction.operands[1], 2);
	}
	if (info->operands == RISCVOperandForm::RsRsTarget) {
		if (count != 3) goto bad_count;
		instruction.operand_count = 3;
		return RequireRegister(operands[0], instruction.operands[0], 1) &&
			RequireRegister(operands[1], instruction.operands[1], 2) &&
			RequireTarget(operands[2], parse_pass,
				instruction.operands[2], 3);
	}
	if (info->operands == RISCVOperandForm::RdAddress) {
		if (count != 2) goto bad_count;
		instruction.operand_count = 2;
		return RequireRegister(operands[0], instruction.operands[0], 1) &&
			RequireAddress(operands[1], parse_pass, false,
				instruction.operands[1], 2);
	}
	if (info->operands == RISCVOperandForm::RsAddress) {
		if (count != 2 && count != 3) goto bad_count;
		instruction.operand_count = count;
		if (!RequireRegister(operands[0], instruction.operands[0], 1) ||
			!RequireAddress(operands[1], parse_pass, false,
				instruction.operands[1], 2)) return false;
		if (instruction.operands[1].address_kind ==
			RISCVAddressKind::Expression) {
			if (count != 3) {
				aasm_log(_LOG_ERROR,
					"RISC-V expression-address store requires a temporary register");
				return false;
			}
			if (!RequireRegister(operands[2], instruction.operands[2], 3))
				return false;
			return ValidateStoreTemporary(instruction.operands[0],
				instruction.operands[2]);
		}
		if (count != 2) goto bad_count;
		return true;
	}
	if (info->operands == RISCVOperandForm::RdRsRsOptional) {
		if (count != 2 && count != 3) goto bad_count;
		instruction.operand_count = 3;
		if (!RequireRegister(operands[0], instruction.operands[0], 1)) return false;
		if (count == 2) {
			instruction.operands[1] = instruction.operands[0];
			return RequireRegister(operands[1], instruction.operands[2], 2);
		}
		return RequireRegister(operands[1], instruction.operands[1], 2) &&
			RequireRegister(operands[2], instruction.operands[2], 3);
	}
	if (info->operands == RISCVOperandForm::RdRsImmediateOptional) {
		if (count != 2 && count != 3) goto bad_count;
		instruction.operand_count = 3;
		if (!RequireRegister(operands[0], instruction.operands[0], 1)) return false;
		if (count == 2) {
			instruction.operands[1] = instruction.operands[0];
			return RequireImmediate(operands[1], parse_pass,
				instruction.operands[2], 2);
		}
		return RequireRegister(operands[1], instruction.operands[1], 2) &&
			RequireImmediate(operands[2], parse_pass, instruction.operands[2], 3);
	}

	aasm_log(_LOG_ERROR, "RISC-V instruction form is not implemented");
	return false;

bad_count:
	aasm_log(_LOG_ERROR, "RISC-V %s: invalid operand count", mnemonic);
	(void)session;
	return false;

bad_signedness:
	aasm_log(_LOG_ERROR,
		"RISC-V instruction does not have a signedness variant");
	return false;
}

}
