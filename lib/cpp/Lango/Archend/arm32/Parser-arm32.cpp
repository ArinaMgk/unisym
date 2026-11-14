// ASCII C++11 TAB4 LF
// Docutitle: AASM ARM32 Instruction Parser
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../../asm/inc/aasm.h"
#include "../../../../../inc/cpp/Lango/Archend/arm32/Instruction-arm32.hpp"

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
	return Equal(name, "call") || Equal(name, "jmp") ||
		Equal(name, "mov") || Equal(name, "ret");
}

bool BaseName(const char* word, char output[], size_t capacity)
{
	size_t length = 0;
	while (word[length] && word[length] != '.') length++;
	if (!length || length >= capacity) return false;
	for (size_t index = 0; index < length; index++) output[index] = word[index];
	output[length] = '\0';
	return true;
}

bool ParseMnemonic(char* word, ARM32Condition& condition, bool& set_flags)
{
	bool seen_condition = false;
	char* qualifier = word;
	while (*qualifier && *qualifier != '.') qualifier++;
	if (!*qualifier) return true;
	*qualifier++ = '\0';
	for (;;) {
		char* end = qualifier;
		while (*end && *end != '.') end++;
		char saved = *end;
		*end = '\0';
		if (!*qualifier) {
			aasm_log(_LOG_ERROR, "ARM32 instruction has an empty qualifier");
			return false;
		}
		ARM32Condition parsed;
		if (ParseARM32Condition(qualifier, parsed)) {
			if (seen_condition || set_flags) {
				aasm_log(_LOG_ERROR,
					"ARM32 condition must appear once and before .S");
				return false;
			}
			condition = parsed;
			seen_condition = true;
		}
		else if (Equal(qualifier, "s")) {
			if (set_flags) {
				aasm_log(_LOG_ERROR, "ARM32 instruction has multiple .S qualifiers");
				return false;
			}
			set_flags = true;
		}
		else {
			aasm_log(_LOG_ERROR, "ARM32 instruction qualifier is invalid");
			return false;
		}
		if (!saved) break;
		qualifier = end + 1;
	}
	return true;
}

bool ParseValue(char* text, int parse_pass, ARM32Value& result)
{
	text = SkipSpace(text);
	TrimEnd(text);
	if (*text == '#') return false;
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

bool ParseRegisterOperand(char* text, ARM32Operand& operand)
{
	text = SkipSpace(text);
	TrimEnd(text);
	ARM32Register reg;
	if (!ParseARM32Register(text, reg)) return false;
	operand.kind = ARM32OperandKind::Register;
	operand.reg = reg;
	return true;
}

bool ParseValueOperand(char* text, int parse_pass,
	ARM32OperandKind kind, ARM32Operand& operand)
{
	if (!ParseValue(text, parse_pass, operand.value)) return false;
	operand.kind = kind;
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
	else {
		*end = saved;
		return false;
	}
	*end = saved;
	text = SkipSpace(end);
	return true;
}

bool ParseAddressOperand(char* text, int parse_pass, bool require_width,
	ARM32Operand& operand)
{
	text = SkipSpace(text);
	if (require_width && !ParseWidth(text, operand.width)) return false;
	if (*text != '[') return false;
	char* close = text + StrLength(text);
	while (close > text && ascii_isspace(close[-1])) close--;
	if (close <= text + 1 || close[-1] != ']') return false;
	char saved_close = close[-1];
	close[-1] = '\0';
	char* content = SkipSpace(text + 1);
	TrimEnd(content);
	operand.kind = ARM32OperandKind::Address;

	char* register_end = content;
	while (isidchar(*register_end)) register_end++;
	char saved = *register_end;
	*register_end = '\0';
	ARM32Register base;
	if (ParseARM32Register(content, base)) {
		*register_end = saved;
		operand.address_kind = ARM32AddressKind::RegisterRelative;
		operand.reg = base;
		char* displacement = SkipSpace(register_end);
		bool valid = true;
		if (*displacement) {
			if (*displacement != '+' && *displacement != '-') valid = false;
			else valid = ParseValue(displacement, parse_pass, operand.value);
		}
		close[-1] = saved_close;
		return valid;
	}
	*register_end = saved;
	operand.address_kind = ARM32AddressKind::Expression;
	bool valid = ParseValue(content, parse_pass, operand.value);
	close[-1] = saved_close;
	return valid;
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
		if (!*cursor || *cursor == ';' || (*cursor == ',' && !brackets)) {
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

bool ReadSignedness(const char* text, size_t length,
	AssemblySignedness& result)
{
	const char* expected = nullptr;
	AssemblySignedness value = AssemblySignedness::Default;
	if (length == 6) {
		expected = "signed";
		value = AssemblySignedness::Signed;
	}
	else if (length == 8) {
		expected = "unsigned";
		value = AssemblySignedness::Unsigned;
	}
	if (!expected) return false;
	for (size_t index = 0; index < length; index++)
		if (ascii_tolower(text[index]) != expected[index]) return false;
	result = value;
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
				"ARM32 instruction has multiple signedness qualifiers");
			return false;
		}
		signedness = value;
		while (start != cursor) *start++ = ' ';
	}
	return true;
}

bool ParseShiftOperand(char* text, int parse_pass, ARM32Shift& shift)
{
	text = SkipSpace(text);
	char* end = text;
	while (isidchar(*end)) end++;
	if (end == text) return false;
	char saved = *end;
	*end = '\0';
	bool valid = ParseARM32Shift(text, shift.kind);
	*end = saved;
	if (!valid) return false;
	char* amount = SkipSpace(end);
	return *amount && ParseValue(amount, parse_pass, shift.amount);
}

bool ParseOperand2(char* text, char* shift_text, int parse_pass,
	ARM32Operand& operand)
{
	if (ParseRegisterOperand(text, operand)) {
		if (shift_text && !ParseShiftOperand(
			shift_text, parse_pass, operand.shift)) return false;
		return true;
	}
	if (shift_text) return false;
	return ParseValueOperand(
		text, parse_pass, ARM32OperandKind::Immediate, operand);
}

bool ParseNativeOperands(const ARM32InstructionInfo& info, char* operands[],
	unsigned count, int parse_pass, ARM32Instruction& instruction)
{
	instruction.opcode = info.opcode;
	if (info.operands == ARM32OperandForm::None) return count == 0;
	if (info.operands == ARM32OperandForm::Target) {
		if (count != 1) return false;
		instruction.operand_count = 1;
		return ParseValueOperand(operands[0], parse_pass,
			ARM32OperandKind::Target, instruction.operands[0]);
	}
	if (info.operands == ARM32OperandForm::Register) {
		if (count != 1) return false;
		instruction.operand_count = 1;
		return ParseRegisterOperand(operands[0], instruction.operands[0]);
	}
	if (info.operands == ARM32OperandForm::RdRnOperand2) {
		if (count != 3 && count != 4) return false;
		instruction.operand_count = 3;
		return ParseRegisterOperand(operands[0], instruction.operands[0]) &&
			ParseRegisterOperand(operands[1], instruction.operands[1]) &&
			ParseOperand2(operands[2], count == 4 ? operands[3] : nullptr,
				parse_pass, instruction.operands[2]);
	}
	if (info.operands == ARM32OperandForm::RnOperand2) {
		if (count != 2 && count != 3) return false;
		instruction.operand_count = 2;
		return ParseRegisterOperand(operands[0], instruction.operands[0]) &&
			ParseOperand2(operands[1], count == 3 ? operands[2] : nullptr,
				parse_pass, instruction.operands[1]);
	}
	if (info.operands == ARM32OperandForm::RtAddress) {
		if (count != 2) return false;
		instruction.operand_count = 2;
		return ParseRegisterOperand(operands[0], instruction.operands[0]) &&
			ParseAddressOperand(operands[1], parse_pass, false,
				instruction.operands[1]);
	}
	return false;
}

bool ParseMove(char* operands[], unsigned count, int parse_pass,
	ARM32Instruction& instruction)
{
	if (count < 2 || count > 3) {
		aasm_log(_LOG_ERROR, "ARM32 MOV expects two operands and optional shift");
		return false;
	}
	ARM32Operand destination;
	if (ParseRegisterOperand(operands[0], destination)) {
		ARM32Operand source;
		if (ParseAddressOperand(operands[1], parse_pass, true, source)) {
			if (count != 2 || instruction.set_flags) {
				aasm_log(_LOG_ERROR,
					"ARM32 memory MOV does not support a shift or .S");
				return false;
			}
			instruction.kind = ARM32InstructionKind::MoveLoad;
			instruction.operand_count = 2;
			instruction.operands[0] = destination;
			instruction.operands[1] = source;
			return true;
		}
		if (!ParseOperand2(operands[1], count == 3 ? operands[2] : nullptr,
			parse_pass, source)) {
			aasm_log(_LOG_ERROR,
				"ARM32 MOV source must be a register, immediate, or sized address");
			return false;
		}
		if (source.kind == ARM32OperandKind::Immediate && instruction.set_flags) {
			aasm_log(_LOG_ERROR,
				"ARM32 immediate MOV does not support .S");
			return false;
		}
		if (instruction.signedness != AssemblySignedness::Default) {
			aasm_log(_LOG_ERROR,
				"ARM32 non-memory MOV does not support signedness");
			return false;
		}
		instruction.kind = ARM32InstructionKind::MoveOperand;
		instruction.operand_count = 2;
		instruction.operands[0] = destination;
		instruction.operands[1] = source;
		return true;
	}
	if (count != 2 || instruction.set_flags ||
		!ParseAddressOperand(operands[0], parse_pass, true, destination)) {
		aasm_log(_LOG_ERROR,
			"ARM32 MOV destination must be a register or sized address");
		return false;
	}
	ARM32Operand source;
	if (!ParseRegisterOperand(operands[1], source)) {
		aasm_log(_LOG_ERROR, "ARM32 MOV store source must be a register");
		return false;
	}
	instruction.kind = ARM32InstructionKind::MoveStore;
	instruction.operand_count = 2;
	instruction.operands[0] = destination;
	instruction.operands[1] = source;
	return true;
}

bool ParseJumpAlias(const char* mnemonic, char* operands[], unsigned count,
	int parse_pass, ARM32Instruction& instruction)
{
	if (count != 1 || instruction.set_flags) {
		aasm_log(_LOG_ERROR, "ARM32 %s expects one operand and no .S", mnemonic);
		return false;
	}
	if (ParseRegisterOperand(operands[0], instruction.operands[0])) {
		instruction.opcode = Equal(mnemonic, "call") ?
			ARM32Opcode::Blx : ARM32Opcode::Bx;
		instruction.operand_count = 1;
		return true;
	}
	if (!ParseValueOperand(operands[0], parse_pass,
		ARM32OperandKind::Target, instruction.operands[0])) {
		aasm_log(_LOG_ERROR,
			"ARM32 %s target must be a register or address expression", mnemonic);
		return false;
	}
	instruction.opcode = Equal(mnemonic, "call") ?
		ARM32Opcode::Bl : ARM32Opcode::B;
	instruction.operand_count = 1;
	return true;
}

bool ParseReturn(char* operands[], unsigned count, ARM32Instruction& instruction)
{
	(void)operands;
	if (count || instruction.set_flags) {
		aasm_log(_LOG_ERROR, "ARM32 RET expects no operands and no .S");
		return false;
	}
	instruction.opcode = ARM32Opcode::Bx;
	instruction.operand_count = 1;
	instruction.operands[0].kind = ARM32OperandKind::Register;
	instruction.operands[0].reg.index = 14;
	return true;
}

}

bool ARM32Parser::IsInstructionStart(const char* word)
{
	char base[32];
	if (!BaseName(word, base, numsof(base))) return false;
	if (IsAlias(base)) return true;
	size_t count;
	return FindARM32Instructions(base, count) != nullptr;
}

bool ARM32Parser::Parse(AssemblySession& session, char* source,
	int parse_pass, ARM32Instruction& instruction)
{
	(void)session;
	if (!ExtractSignedness(source, instruction.signedness)) return false;
	char* mnemonic = SkipSpace(source);
	char* end = mnemonic;
	while (isidchar(*end)) end++;
	char saved = *end;
	*end = '\0';
	char* operands_text = saved ? SkipSpace(end + 1) : end;
	if (!ParseMnemonic(mnemonic, instruction.condition, instruction.set_flags))
		return false;
	char* operands[4];
	unsigned count = SplitOperands(operands_text, operands, numsof(operands));
	if (count > numsof(operands)) {
		aasm_log(_LOG_ERROR, "ARM32 instruction has too many or malformed operands");
		return false;
	}

	if (Equal(mnemonic, "mov"))
		return ParseMove(operands, count, parse_pass, instruction);
	if (Equal(mnemonic, "jmp") || Equal(mnemonic, "call")) {
		if (instruction.signedness != AssemblySignedness::Default) {
			aasm_log(_LOG_ERROR,
				"ARM32 control-flow instruction does not support signedness");
			return false;
		}
		return ParseJumpAlias(mnemonic, operands, count, parse_pass, instruction);
	}
	if (Equal(mnemonic, "ret")) {
		if (instruction.signedness != AssemblySignedness::Default) {
			aasm_log(_LOG_ERROR, "ARM32 RET does not support signedness");
			return false;
		}
		return ParseReturn(operands, count, instruction);
	}
	if (instruction.signedness != AssemblySignedness::Default) {
		aasm_log(_LOG_ERROR,
			"ARM32 native instruction does not support signedness qualifiers");
		return false;
	}

	size_t candidate_count;
	const ARM32InstructionInfo* candidates =
		FindARM32Instructions(mnemonic, candidate_count);
	if (!candidates) {
		aasm_log(_LOG_ERROR, "ARM32 instruction expected");
		return false;
	}
	for (size_t index = 0; index < candidate_count; index++) {
		ARM32Instruction candidate;
		candidate.condition = instruction.condition;
		candidate.set_flags = instruction.set_flags;
		if (candidate.set_flags && !candidates[index].supports_set_flags)
			continue;
		if (ParseNativeOperands(candidates[index], operands, count,
			parse_pass, candidate)) {
			instruction = candidate;
			return true;
		}
	}
	aasm_log(_LOG_ERROR, "ARM32 instruction operands do not match `%s'", mnemonic);
	return false;
}

}
