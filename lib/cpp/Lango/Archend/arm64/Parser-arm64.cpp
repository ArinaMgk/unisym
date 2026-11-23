// ASCII C++11 TAB4 LF
// Docutitle: AASM ARM64 Instruction Parser
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../../asm/inc/aasm.h"
#include "../../../../../inc/cpp/Lango/Archend/arm64/Instruction-arm64.hpp"

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
	return Equal(name, "call") || Equal(name, "jmp") || Equal(name, "mov");
}

bool ParseValue(char* text, int parse_pass, ARM64Value& result)
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

bool ParseRegisterOperand(char* text, ARM64Operand& operand)
{
	text = SkipSpace(text);
	TrimEnd(text);
	ARM64Register reg;
	if (!ParseARM64Register(text, reg)) return false;
	operand.kind = ARM64OperandKind::Register;
	operand.reg = reg;
	return true;
}

bool ParseValueOperand(char* text, int parse_pass,
	ARM64OperandKind kind, ARM64Operand& operand)
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
	else if (Equal(start, "byt8")) width = 8;
	else {
		*end = saved;
		return false;
	}
	*end = saved;
	text = SkipSpace(end);
	return true;
}

bool ParseIndexExtend(char* text, int parse_pass, ARM64Address& address)
{
	text = SkipSpace(text);
	if (!*text) return true;
	if (*text != ',') return false;
	text = SkipSpace(text + 1);
	char* end = text;
	while (isidchar(*end)) end++;
	if (end == text) return false;
	char saved = *end;
	*end = '\0';
	if (Equal(text, "lsl")) address.extend = ARM64IndexExtend::LSL;
	else if (Equal(text, "uxtw")) address.extend = ARM64IndexExtend::UXTW;
	else if (Equal(text, "sxtw")) address.extend = ARM64IndexExtend::SXTW;
	else if (Equal(text, "sxtx")) address.extend = ARM64IndexExtend::SXTX;
	else {
		*end = saved;
		return false;
	}
	*end = saved;
	text = SkipSpace(end);
	if (!*text) return true;
	ARM64Value shift;
	if (!ParseValue(text, parse_pass, shift) || shift.unknown ||
		shift.external || shift.section != NO_SEG || shift.wrt != NO_SEG ||
		shift.offset < 0 || shift.offset > 255) return false;
	address.shift = static_cast<uint8_t>(shift.offset);
	return true;
}

bool ParseAddressOperand(char* text, char* post_index, int parse_pass,
	bool require_width, ARM64Operand& operand)
{
	text = SkipSpace(text);
	if (require_width && !ParseWidth(text, operand.width)) return false;
	if (*text != '[') return false;
	char* end = text + StrLength(text);
	while (end > text && ascii_isspace(end[-1])) end--;
	bool writeback = false;
	if (end > text && end[-1] == '!') {
		writeback = true;
		end--;
		while (end > text && ascii_isspace(end[-1])) end--;
	}
	if (end <= text + 1 || end[-1] != ']' || (writeback && post_index))
		return false;
	char* close = end - 1;
	char saved_close = *close;
	*close = '\0';
	char* content = SkipSpace(text + 1);
	TrimEnd(content);
	operand.kind = ARM64OperandKind::Address;

	char* register_end = content;
	while (isidchar(*register_end)) register_end++;
	char saved = *register_end;
	*register_end = '\0';
	ARM64Register base;
	if (ParseARM64Register(content, base)) {
		*register_end = saved;
		if (base.width != 64 || base.kind == ARM64RegisterKind::Zero) {
			*close = saved_close;
			return false;
		}
		operand.address.base = base;
		char* displacement = SkipSpace(register_end);
		if (post_index) {
			bool valid = !*displacement &&
				ParseValue(post_index, parse_pass,
					operand.address.displacement);
			operand.address.mode = ARM64AddressMode::PostIndex;
			*close = saved_close;
			return valid;
		}
		if (!*displacement) {
			operand.address.mode = writeback ? ARM64AddressMode::PreIndex :
				ARM64AddressMode::ImmediateOffset;
			*close = saved_close;
			return true;
		}
		if (*displacement != '+' && *displacement != '-') {
			*close = saved_close;
			return false;
		}
		char sign = *displacement;
		char* second = SkipSpace(displacement + 1);
		char* second_end = second;
		while (isidchar(*second_end)) second_end++;
		char second_saved = *second_end;
		*second_end = '\0';
		ARM64Register index;
		bool register_offset = ParseARM64Register(second, index);
		*second_end = second_saved;
		if (register_offset) {
			bool valid = sign == '+' && !writeback;
			operand.address.mode = ARM64AddressMode::RegisterOffset;
			operand.address.index = index;
			if (valid)
				valid = ParseIndexExtend(second_end, parse_pass, operand.address);
			*close = saved_close;
			return valid;
		}
		bool valid = ParseValue(displacement, parse_pass,
			operand.address.displacement);
		operand.address.mode = writeback ? ARM64AddressMode::PreIndex :
			ARM64AddressMode::ImmediateOffset;
		*close = saved_close;
		return valid;
	}
	*register_end = saved;
	operand.address.mode = ARM64AddressMode::Expression;
	bool valid = !writeback && !post_index &&
		ParseValue(content, parse_pass, operand.address.displacement);
	*close = saved_close;
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
				"ARM64 instruction has multiple signedness qualifiers");
			return false;
		}
		signedness = value;
		while (start != cursor) *start++ = ' ';
	}
	return true;
}

bool ParseConditionalBranch(const char* mnemonic, ARM64Condition& condition)
{
	if (ascii_tolower(mnemonic[0]) != 'b' || mnemonic[1] != '.') return false;
	return ParseARM64Condition(mnemonic + 2, condition);
}

bool ParseNativeOperands(const ARM64InstructionInfo& info, char* operands[],
	unsigned count, int parse_pass, ARM64Instruction& instruction)
{
	instruction.opcode = info.opcode;
	if (info.operands == ARM64OperandForm::None)
		return count == 0;
	if (info.operands == ARM64OperandForm::Target) {
		if (count != 1) return false;
		instruction.operand_count = 1;
		return ParseValueOperand(operands[0], parse_pass,
			ARM64OperandKind::Target, instruction.operands[0]);
	}
	if (info.operands == ARM64OperandForm::Register ||
		info.operands == ARM64OperandForm::RegisterOptional) {
		if (!count && info.operands == ARM64OperandForm::RegisterOptional) {
			instruction.operand_count = 1;
			instruction.operands[0].kind = ARM64OperandKind::Register;
			instruction.operands[0].reg = {
				ARM64RegisterKind::General, 30, 64
			};
			return true;
		}
		if (count != 1) return false;
		instruction.operand_count = 1;
		return ParseRegisterOperand(operands[0], instruction.operands[0]);
	}
	if (info.operands == ARM64OperandForm::RdRnRm) {
		if (count != 3) return false;
		instruction.operand_count = 3;
		return ParseRegisterOperand(operands[0], instruction.operands[0]) &&
			ParseRegisterOperand(operands[1], instruction.operands[1]) &&
			ParseRegisterOperand(operands[2], instruction.operands[2]);
	}
	if (info.operands == ARM64OperandForm::RdRnImmediate) {
		if (count != 3) return false;
		instruction.operand_count = 3;
		return ParseRegisterOperand(operands[0], instruction.operands[0]) &&
			ParseRegisterOperand(operands[1], instruction.operands[1]) &&
			ParseValueOperand(operands[2], parse_pass,
				ARM64OperandKind::Immediate, instruction.operands[2]);
	}
	if (info.operands == ARM64OperandForm::RnRm) {
		if (count != 2) return false;
		instruction.operand_count = 2;
		return ParseRegisterOperand(operands[0], instruction.operands[0]) &&
			ParseRegisterOperand(operands[1], instruction.operands[1]);
	}
	if (info.operands == ARM64OperandForm::RnImmediate) {
		if (count != 2) return false;
		instruction.operand_count = 2;
		return ParseRegisterOperand(operands[0], instruction.operands[0]) &&
			ParseValueOperand(operands[1], parse_pass,
				ARM64OperandKind::Immediate, instruction.operands[1]);
	}
	if (info.operands == ARM64OperandForm::RtAddress) {
		if (count != 2 && count != 3) return false;
		instruction.operand_count = 2;
		return ParseRegisterOperand(operands[0], instruction.operands[0]) &&
			ParseAddressOperand(operands[1], count == 3 ? operands[2] : nullptr,
				parse_pass, false,
				instruction.operands[1]);
	}
	if (info.operands == ARM64OperandForm::RtRtAddress) {
		if (count != 3 && count != 4) return false;
		instruction.operand_count = 3;
		return ParseRegisterOperand(operands[0], instruction.operands[0]) &&
			ParseRegisterOperand(operands[1], instruction.operands[1]) &&
			ParseAddressOperand(operands[2], count == 4 ? operands[3] : nullptr,
				parse_pass, false, instruction.operands[2]);
	}
	return false;
}

bool ParseMove(char* operands[], unsigned count, int parse_pass,
	ARM64Instruction& instruction)
{
	if (count != 2 && count != 3) {
		aasm_log(_LOG_ERROR, "ARM64 MOV expects two operands and optional post-index");
		return false;
	}
	ARM64Operand destination;
	if (ParseRegisterOperand(operands[0], destination)) {
		ARM64Operand source;
		if (count == 2 && ParseRegisterOperand(operands[1], source)) {
			instruction.kind = ARM64InstructionKind::MoveRegister;
			instruction.operand_count = 2;
			instruction.operands[0] = destination;
			instruction.operands[1] = source;
			return true;
		}
		if (ParseAddressOperand(operands[1], count == 3 ? operands[2] : nullptr,
			parse_pass, true, source)) {
			instruction.kind = ARM64InstructionKind::MoveLoad;
			instruction.operand_count = 2;
			instruction.operands[0] = destination;
			instruction.operands[1] = source;
			return true;
		}
		if (count != 2 || !ParseValueOperand(operands[1], parse_pass,
			ARM64OperandKind::Immediate, source)) {
			aasm_log(_LOG_ERROR,
				"ARM64 MOV operand 2 must be a register, expression, or address");
			return false;
		}
		instruction.kind = ARM64InstructionKind::MoveImmediate;
		instruction.operand_count = 2;
		instruction.operands[0] = destination;
		instruction.operands[1] = source;
		return true;
	}
	if (!ParseAddressOperand(operands[0], count == 3 ? operands[2] : nullptr,
		parse_pass, true, destination)) {
		aasm_log(_LOG_ERROR,
			"ARM64 MOV operand 1 must be a register or sized address");
		return false;
	}
	ARM64Operand source;
	if (!ParseRegisterOperand(operands[1], source)) {
		aasm_log(_LOG_ERROR, "ARM64 MOV store source must be a register");
		return false;
	}
	instruction.kind = ARM64InstructionKind::MoveStore;
	instruction.operand_count = 2;
	instruction.operands[0] = destination;
	instruction.operands[1] = source;
	return true;
}

bool ParseJumpAlias(const char* mnemonic, char* operands[], unsigned count,
	int parse_pass, ARM64Instruction& instruction)
{
	if (count != 1) {
		aasm_log(_LOG_ERROR, "ARM64 %s expects one operand", mnemonic);
		return false;
	}
	if (ParseRegisterOperand(operands[0], instruction.operands[0])) {
		instruction.opcode = Equal(mnemonic, "call") ?
			ARM64Opcode::Blr : ARM64Opcode::Br;
		instruction.operand_count = 1;
		return true;
	}
	if (!ParseValueOperand(operands[0], parse_pass,
		ARM64OperandKind::Target, instruction.operands[0])) {
		aasm_log(_LOG_ERROR,
			"ARM64 %s target must be a register or address expression", mnemonic);
		return false;
	}
	instruction.opcode = Equal(mnemonic, "call") ?
		ARM64Opcode::Bl : ARM64Opcode::B;
	instruction.operand_count = 1;
	return true;
}

}

bool ARM64Parser::IsInstructionStart(const char* word)
{
	if (IsAlias(word)) return true;
	ARM64Condition condition;
	if (ParseConditionalBranch(word, condition)) return true;
	size_t count;
	return FindARM64Instructions(word, count) != nullptr;
}

bool ARM64Parser::Parse(AssemblySession& session, char* source,
	int parse_pass, ARM64Instruction& instruction)
{
	(void)session;
	if (!ExtractSignedness(source, instruction.signedness)) return false;
	char* mnemonic = SkipSpace(source);
	char* end = mnemonic;
	while (isidchar(*end)) end++;
	char saved = *end;
	*end = '\0';
	char* operands_text = SkipSpace(end + (saved ? 1 : 0));
	if (saved) *end = '\0';
	char* operands[4];
	unsigned count = SplitOperands(operands_text, operands, numsof(operands));
	if (count > numsof(operands)) {
		aasm_log(_LOG_ERROR, "ARM64 instruction has too many or malformed operands");
		return false;
	}

	if (Equal(mnemonic, "mov")) return ParseMove(
		operands, count, parse_pass, instruction);
	if (Equal(mnemonic, "jmp") || Equal(mnemonic, "call")) {
		if (instruction.signedness != AssemblySignedness::Default) {
			aasm_log(_LOG_ERROR,
				"ARM64 control-flow instruction does not support signedness");
			return false;
		}
		return ParseJumpAlias(mnemonic, operands, count, parse_pass, instruction);
	}
	ARM64Condition condition;
	if (ParseConditionalBranch(mnemonic, condition)) {
		if (instruction.signedness != AssemblySignedness::Default || count != 1 ||
			!ParseValueOperand(operands[0], parse_pass,
				ARM64OperandKind::Target, instruction.operands[0])) {
			aasm_log(_LOG_ERROR, "ARM64 conditional branch operand is invalid");
			return false;
		}
		instruction.kind = ARM64InstructionKind::ConditionalBranch;
		instruction.condition = condition;
		instruction.operand_count = 1;
		return true;
	}
	if (instruction.signedness != AssemblySignedness::Default) {
		aasm_log(_LOG_ERROR,
			"ARM64 native instruction does not support signedness qualifiers");
		return false;
	}

	size_t candidate_count;
	const ARM64InstructionInfo* candidates =
		FindARM64Instructions(mnemonic, candidate_count);
	if (!candidates) {
		aasm_log(_LOG_ERROR, "ARM64 instruction expected");
		return false;
	}
	for (size_t index = 0; index < candidate_count; index++) {
		ARM64Instruction candidate;
		if (ParseNativeOperands(candidates[index], operands, count,
			parse_pass, candidate)) {
			instruction = candidate;
			return true;
		}
	}
	aasm_log(_LOG_ERROR, "ARM64 instruction operands do not match `%s'", mnemonic);
	return false;
}

}
