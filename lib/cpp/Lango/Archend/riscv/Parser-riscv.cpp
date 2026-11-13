// ASCII C++11 TAB4 LF
// Docutitle: AASM RISC-V Instruction Parser
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../../asm/inc/aasm.h"
#include "../../../../../inc/cpp/Lango/Archend/riscv/Instruction-riscv.hpp"
#include "../../../../../inc/cpp/Lango/Archend/riscv/Vector-riscv.hpp"

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
		"call", "call_plt", "j", "jr", "mov", "nop", "ret"
	};
	for (const char* alias : aliases)
		if (Equal(name, alias)) return true;
	return false;
}

bool ParseRegister(const char* text, uint8_t& reg)
{
	RISCVRegister parsed;
	if (!ParseRISCVRegister(text, parsed) ||
		parsed.kind != RISCVRegisterClass::Integer) return false;
	reg = parsed.index;
	return true;
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

bool ExtractAtomicOrdering(char* source, RISCVAtomicOrdering& ordering)
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
		size_t length = cursor - start;
		RISCVAtomicOrdering value = RISCVAtomicOrdering::None;
		if (length == 2 && ascii_tolower(start[0]) == 'a' &&
			ascii_tolower(start[1]) == 'q')
			value = RISCVAtomicOrdering::Acquire;
		else if (length == 2 && ascii_tolower(start[0]) == 'r' &&
			ascii_tolower(start[1]) == 'l')
			value = RISCVAtomicOrdering::Release;
		if (value == RISCVAtomicOrdering::None) continue;
		if ((static_cast<uint8_t>(ordering) & static_cast<uint8_t>(value))) {
			aasm_log(_LOG_ERROR,
				"RISC-V instruction has duplicate atomic ordering qualifiers");
			return false;
		}
		ordering = static_cast<RISCVAtomicOrdering>(
			static_cast<uint8_t>(ordering) | static_cast<uint8_t>(value));
		while (start != cursor) *start++ = ' ';
	}
	return true;
}

bool ExtractRoundingMode(char* source, RISCVRoundingMode& rounding,
	bool& specified)
{
	char* start = SkipSpace(source);
	char* cursor = start;
	while (isidchar(*cursor)) cursor++;
	char saved = *cursor;
	*cursor = '\0';
	RISCVRoundingMode value;
	bool matched = ParseRISCVRoundingMode(start, value);
	*cursor = saved;
	if (!matched) return true;
	rounding = value;
	specified = true;
	while (start != cursor) *start++ = ' ';
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

bool ParseTypedRegisterOperand(char* text, RISCVRegisterClass kind,
	RISCVOperand& operand)
{
	text = SkipSpace(text);
	TrimEnd(text);
	RISCVRegister reg;
	if (!ParseRISCVRegister(text, reg) || reg.kind != kind) return false;
	operand.kind = RISCVOperandKind::Register;
	operand.reg = reg.index;
	operand.reg_class = reg.kind;
	return true;
}

bool ParseImmediateOperand(char* text, int parse_pass, RISCVOperand& operand)
{
	text = SkipSpace(text);
	char* word_end = text;
	while (isidchar(*word_end)) word_end++;
	char saved = *word_end;
	*word_end = '\0';
	if (Equal(text, "high"))
		operand.relocation = RISCVRelocationKind::High;
	else if (Equal(text, "low"))
		operand.relocation = RISCVRelocationKind::Low;
	else if (Equal(text, "got"))
		operand.relocation = RISCVRelocationKind::Got;
	else if (Equal(text, "pcrel_high"))
		operand.relocation = RISCVRelocationKind::PcRelativeHigh;
	else if (Equal(text, "pcrel_low"))
		operand.relocation = RISCVRelocationKind::PcRelativeLow;
	*word_end = saved;
	if (operand.relocation != RISCVRelocationKind::None) {
		if (*word_end && !ascii_isspace(*word_end)) return false;
		text = SkipSpace(word_end);
	}
	operand.kind = RISCVOperandKind::Immediate;
	return ParseValue(text, parse_pass, operand.value);
}

bool ParseRelocationValue(char* text, int parse_pass,
	RISCVRelocationKind& relocation, RISCVValue& value)
{
	text = SkipSpace(text);
	char* expression = text;
	char sign = '\0';
	if (*text == '+' || *text == '-') {
		sign = *text++;
		text = SkipSpace(text);
	}
	char* word_end = text;
	while (isidchar(*word_end)) word_end++;
	char saved = *word_end;
	*word_end = '\0';
	if (Equal(text, "high")) relocation = RISCVRelocationKind::High;
	else if (Equal(text, "low")) relocation = RISCVRelocationKind::Low;
	else if (Equal(text, "got")) relocation = RISCVRelocationKind::Got;
	else if (Equal(text, "pcrel_high"))
		relocation = RISCVRelocationKind::PcRelativeHigh;
	else if (Equal(text, "pcrel_low"))
		relocation = RISCVRelocationKind::PcRelativeLow;
	*word_end = saved;
	if (relocation == RISCVRelocationKind::None)
		return ParseValue(expression, parse_pass, value);
	if (*word_end && !ascii_isspace(*word_end)) return false;
	if (!ParseValue(SkipSpace(word_end), parse_pass, value)) return false;
	if (sign == '-') value.offset = -value.offset;
	return true;
}

bool ParseCSROperand(char* text, int parse_pass, RISCVOperand& operand)
{
	text = SkipSpace(text);
	TrimEnd(text);
	uint16_t csr;
	if (!ParseRISCVCSR(text, csr)) {
		RISCVValue value;
		if (!ParseValue(text, parse_pass, value) || value.unknown ||
			value.external || value.section != NO_SEG || value.wrt != NO_SEG ||
			value.offset < 0 || value.offset > 0xfff) return false;
		csr = static_cast<uint16_t>(value.offset);
	}
	operand.kind = RISCVOperandKind::CSR;
	operand.value.offset = csr;
	return true;
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
		return ParseRelocationValue(displacement, parse_pass,
			operand.relocation, operand.value);
	}
	*register_end = saved;
	operand.address_kind = RISCVAddressKind::Expression;
	return ParseValue(content, parse_pass, operand.value);
}

bool ParseAtomicAddressOperand(char* text, RISCVOperand& operand)
{
	text = SkipSpace(text);
	TrimEnd(text);
	size_t length = StrLength(text);
	if (length < 3 || text[0] != '[' || text[length - 1] != ']') return false;
	text[length - 1] = '\0';
	char* content = SkipSpace(text + 1);
	TrimEnd(content);
	uint8_t reg;
	if (!ParseRegister(content, reg)) return false;
	operand.kind = RISCVOperandKind::Address;
	operand.address_kind = RISCVAddressKind::RegisterRelative;
	operand.reg = reg;
	return true;
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

bool RequireTypedRegister(char* text, RISCVRegisterClass kind,
	RISCVOperand& operand, unsigned number)
{
	if (ParseTypedRegisterOperand(text, kind, operand)) return true;
	const char* name = kind == RISCVRegisterClass::Float ? "floating-point" :
		kind == RISCVRegisterClass::Vector ? "vector" : "integer";
	aasm_log(_LOG_ERROR, "RISC-V operand %u: %s register expected", number, name);
	return false;
}

bool RequireImmediate(char* text, int parse_pass,
	RISCVOperand& operand, unsigned number)
{
	if (ParseImmediateOperand(text, parse_pass, operand)) return true;
	aasm_log(_LOG_ERROR, "RISC-V operand %u: immediate expression expected", number);
	return false;
}

bool RequireCSR(char* text, int parse_pass,
	RISCVOperand& operand, unsigned number)
{
	if (ParseCSROperand(text, parse_pass, operand)) return true;
	aasm_log(_LOG_ERROR, "RISC-V operand %u: CSR name or number expected", number);
	return false;
}

bool RequireAtomicAddress(char* text, RISCVOperand& operand, unsigned number)
{
	if (ParseAtomicAddressOperand(text, operand)) return true;
	aasm_log(_LOG_ERROR,
		"RISC-V operand %u: atomic register address expected", number);
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

bool SupportsExtensions(const AssemblySession& session,
	const RISCVInstructionInfo& instruction)
{
	return (session.CurrentCpu & instruction.required_extensions) ==
		instruction.required_extensions;
}

bool IsAtomicFormat(RISCVEncodingFormat format)
{
	return format == RISCVEncodingFormat::AtomicLoad ||
		format == RISCVEncodingFormat::Atomic;
}

bool InstructionUsesRounding(const RISCVInstructionInfo& info)
{
	if (info.format == RISCVEncodingFormat::FloatUnary ||
		info.format == RISCVEncodingFormat::FloatConvertToInteger ||
		info.format == RISCVEncodingFormat::FloatConvertFromInteger ||
		info.format == RISCVEncodingFormat::FloatFused) return true;
	if (info.format == RISCVEncodingFormat::FloatConvertFormat)
		return info.opcode == RISCVOpcode::FcvtSD;
	if (info.format != RISCVEncodingFormat::FloatRegister) return false;
	switch (info.opcode) {
	case RISCVOpcode::FaddS: case RISCVOpcode::FsubS:
	case RISCVOpcode::FmulS: case RISCVOpcode::FdivS:
	case RISCVOpcode::FaddD: case RISCVOpcode::FsubD:
	case RISCVOpcode::FmulD: case RISCVOpcode::FdivD:
		return true;
	default: return false;
	}
}

bool ParseRoundingOperand(char* text, RISCVInstruction& instruction,
	bool& specified)
{
	text = SkipSpace(text);
	TrimEnd(text);
	RISCVRoundingMode value;
	if (!ParseRISCVRoundingMode(text, value)) return false;
	if (specified) {
		aasm_log(_LOG_ERROR,
			"RISC-V instruction has multiple rounding-mode qualifiers");
		return false;
	}
	instruction.rounding = value;
	specified = true;
	return true;
}

bool ParseFenceSet(char* text, uint8_t& result)
{
	text = SkipSpace(text);
	TrimEnd(text);
	if (text[0] == '0' && !text[1]) {
		result = 0;
		return true;
	}
	if (!*text) return false;
	uint8_t value = 0;
	for (; *text; text++) {
		uint8_t access;
		switch (ascii_tolower(*text)) {
		case 'i': access = static_cast<uint8_t>(RISCVFenceAccess::Input); break;
		case 'o': access = static_cast<uint8_t>(RISCVFenceAccess::Output); break;
		case 'r': access = static_cast<uint8_t>(RISCVFenceAccess::Read); break;
		case 'w': access = static_cast<uint8_t>(RISCVFenceAccess::Write); break;
		default: return false;
		}
		if (value & access) return false;
		value |= access;
	}
	result = value;
	return true;
}

bool ParseFenceMode(char* text, int parse_pass, uint8_t& result)
{
	RISCVValue value;
	if (!ParseValue(text, parse_pass, value) || value.unknown ||
		value.external || value.section != NO_SEG || value.wrt != NO_SEG ||
		value.offset != 0) return false;
	result = 0;
	return true;
}

}

bool RISCVParser::IsInstructionStart(const char* word)
{
	return FindRISCVInstruction(word) || IsAlias(word) ||
		Equal(word, "signed") || Equal(word, "unsigned") ||
		Equal(word, "aq") || Equal(word, "rl") ||
		Equal(word, "rne") || Equal(word, "rtz") || Equal(word, "rdn") ||
		Equal(word, "rup") || Equal(word, "rmm") || Equal(word, "dyn");
}

bool RISCVParser::Parse(AssemblySession& session, char* source,
	int parse_pass, RISCVInstruction& instruction)
{
	if (!ExtractSignedness(source, instruction.signedness)) return false;
	if (!ExtractAtomicOrdering(source, instruction.atomic_ordering)) return false;
	bool rounding_specified = false;
	if (!ExtractRoundingMode(source, instruction.rounding,
		rounding_specified)) return false;
	source = SkipSpace(source);
	char* mnemonic_end = source;
	while (isidchar(*mnemonic_end)) mnemonic_end++;
	char saved = *mnemonic_end;
	*mnemonic_end = '\0';
	char mnemonic[32];
	StrCopyN(mnemonic, source, sizeof(mnemonic));
	*mnemonic_end = saved;
	char* operand_text = SkipSpace(mnemonic_end);
	char* operands[8];
	const RISCVInstructionInfo* info = nullptr;
	unsigned count = SplitOperands(operand_text, operands, numsof(operands));
	if (count > numsof(operands)) {
		aasm_log(_LOG_ERROR, "RISC-V: invalid operand list");
		return false;
	}
	if (instruction.atomic_ordering != RISCVAtomicOrdering::None &&
		IsAlias(mnemonic)) {
		aasm_log(_LOG_ERROR,
			"RISC-V instruction does not support atomic ordering qualifiers");
		return false;
	}
	if (rounding_specified && IsAlias(mnemonic)) {
		aasm_log(_LOG_ERROR,
			"RISC-V instruction does not support a rounding-mode qualifier");
		return false;
	}
	if (Equal(mnemonic, "call") || Equal(mnemonic, "call_plt")) {
		if (instruction.signedness != AssemblySignedness::Default)
			goto bad_signedness;
		if (count != 1) goto bad_count;
		instruction.kind = Equal(mnemonic, "call") ?
			RISCVInstructionKind::Call : RISCVInstructionKind::CallPlt;
		instruction.operand_count = 1;
		return RequireTarget(operands[0], parse_pass,
			instruction.operands[0], 1);
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
		if (count != 1 && count != 2) goto bad_count;
		instruction.opcode = RISCVOpcode::Jal;
		instruction.operand_count = 2;
		instruction.operands[0].kind = RISCVOperandKind::Register;
		if (!RequireTarget(operands[0], parse_pass,
			instruction.operands[1], 1)) return false;
		if (count == 2) {
			RISCVOperand temporary;
			if (!RequireRegister(operands[1], temporary, 2)) return false;
			if (!temporary.reg) {
				aasm_log(_LOG_ERROR,
					"RISC-V long jump temporary register cannot be zero");
				return false;
			}
			instruction.temporary_reg = temporary.reg;
			instruction.has_temporary = true;
		}
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
	if (!SupportsExtensions(session, *info)) {
		aasm_log(_LOG_ERROR,
			"RISC-V instruction `%s' is disabled by the current instruction set",
			info->name);
		return false;
	}
	if (instruction.atomic_ordering != RISCVAtomicOrdering::None &&
		!IsAtomicFormat(info->format)) {
		aasm_log(_LOG_ERROR,
			"RISC-V instruction does not support atomic ordering qualifiers");
		return false;
	}
	if (rounding_specified && !InstructionUsesRounding(*info)) {
		aasm_log(_LOG_ERROR,
			"RISC-V instruction does not support a rounding-mode qualifier");
		return false;
	}
	if (info->operands == RISCVOperandForm::Fence) {
		if (!count) return true;
		unsigned first_set = 0;
		if (count == 3) {
			if (!ParseFenceMode(operands[0], parse_pass,
				instruction.fence.mode)) {
				aasm_log(_LOG_ERROR,
					"RISC-V FENCE mode must be the supported value 0");
				return false;
			}
			first_set = 1;
		}
		else if (count != 2) goto bad_count;
		if (!ParseFenceSet(operands[first_set],
			instruction.fence.predecessor) ||
			!ParseFenceSet(operands[first_set + 1],
				instruction.fence.successor)) {
			aasm_log(_LOG_ERROR,
				"RISC-V FENCE set must contain unique I, O, R, W letters or 0");
			return false;
		}
		return true;
	}
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
		if (count != 1 && count != 2 && count != 3) goto bad_count;
		instruction.operand_count = 2;
		if (count == 1) {
			instruction.operands[0].kind = RISCVOperandKind::Register;
			instruction.operands[0].reg = 1;
			return RequireTarget(operands[0], parse_pass,
				instruction.operands[1], 1);
		}
		if (!RequireRegister(operands[0], instruction.operands[0], 1) ||
			!RequireTarget(operands[1], parse_pass,
				instruction.operands[1], 2)) return false;
		if (count == 3) {
			RISCVOperand temporary;
			if (!RequireRegister(operands[2], temporary, 3)) return false;
			if (!temporary.reg) {
				aasm_log(_LOG_ERROR,
					"RISC-V long jump temporary register cannot be zero");
				return false;
			}
			instruction.temporary_reg = temporary.reg;
			instruction.has_temporary = true;
		}
		return true;
	}
	if (info->operands == RISCVOperandForm::RdIndirectTarget) {
		if (count != 2) goto bad_count;
		instruction.operand_count = 2;
		return RequireRegister(operands[0], instruction.operands[0], 1) &&
			RequireIndirectTarget(operands[1], parse_pass,
				instruction.operands[1], 2);
	}
	if (info->operands == RISCVOperandForm::RsRsTarget) {
		if (count != 3 && count != 4) goto bad_count;
		instruction.operand_count = 3;
		if (!RequireRegister(operands[0], instruction.operands[0], 1) ||
			!RequireRegister(operands[1], instruction.operands[1], 2) ||
			!RequireTarget(operands[2], parse_pass,
				instruction.operands[2], 3)) return false;
		if (count == 4) {
			RISCVOperand temporary;
			if (!RequireRegister(operands[3], temporary, 4)) return false;
			if (!temporary.reg) {
				aasm_log(_LOG_ERROR,
					"RISC-V long branch temporary register cannot be zero");
				return false;
			}
			instruction.temporary_reg = temporary.reg;
			instruction.has_temporary = true;
		}
		return true;
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
	if (info->operands == RISCVOperandForm::RdCsrRs) {
		if (count != 3) goto bad_count;
		instruction.operand_count = 3;
		return RequireRegister(operands[0], instruction.operands[0], 1) &&
			RequireCSR(operands[1], parse_pass, instruction.operands[1], 2) &&
			RequireRegister(operands[2], instruction.operands[2], 3);
	}
	if (info->operands == RISCVOperandForm::RdCsrImmediate) {
		if (count != 3) goto bad_count;
		instruction.operand_count = 3;
		return RequireRegister(operands[0], instruction.operands[0], 1) &&
			RequireCSR(operands[1], parse_pass, instruction.operands[1], 2) &&
			RequireImmediate(operands[2], parse_pass, instruction.operands[2], 3);
	}
	if (info->operands == RISCVOperandForm::RdAtomicAddress) {
		if (count != 2) goto bad_count;
		instruction.operand_count = 2;
		return RequireRegister(operands[0], instruction.operands[0], 1) &&
			RequireAtomicAddress(operands[1], instruction.operands[1], 2);
	}
	if (info->operands == RISCVOperandForm::RdRsAtomicAddress) {
		if (count != 3) goto bad_count;
		instruction.operand_count = 3;
		return RequireRegister(operands[0], instruction.operands[0], 1) &&
			RequireRegister(operands[1], instruction.operands[1], 2) &&
			RequireAtomicAddress(operands[2], instruction.operands[2], 3);
	}
	if (info->operands == RISCVOperandForm::RsRsOptional) {
		if (count != 0 && count != 2) goto bad_count;
		instruction.operand_count = 2;
		if (!count) {
			instruction.operands[0].kind = RISCVOperandKind::Register;
			instruction.operands[1].kind = RISCVOperandKind::Register;
			return true;
		}
		return RequireRegister(operands[0], instruction.operands[0], 1) &&
			RequireRegister(operands[1], instruction.operands[1], 2);
	}
	if (info->operands == RISCVOperandForm::FdAddress ||
		info->operands == RISCVOperandForm::FsAddress) {
		if (count != 2 && count != 3) goto bad_count;
		instruction.operand_count = count;
		if (!RequireTypedRegister(operands[0], RISCVRegisterClass::Float,
			instruction.operands[0], 1) ||
			!RequireAddress(operands[1], parse_pass, false,
				instruction.operands[1], 2)) return false;
		if (instruction.operands[1].address_kind == RISCVAddressKind::Expression) {
			if (count != 3) {
				aasm_log(_LOG_ERROR,
					"RISC-V floating expression address requires a temporary integer register");
				return false;
			}
			return RequireRegister(operands[2], instruction.operands[2], 3);
		}
		if (count != 2) goto bad_count;
		return true;
	}
	if (info->operands == RISCVOperandForm::FdFsFs ||
		info->operands == RISCVOperandForm::RdFsFs) {
		unsigned base_count = 3;
		if (count != base_count && count != base_count + 1) goto bad_count;
		instruction.operand_count = 3;
		RISCVRegisterClass destination = info->operands == RISCVOperandForm::FdFsFs ?
			RISCVRegisterClass::Float : RISCVRegisterClass::Integer;
		if (!RequireTypedRegister(operands[0], destination,
			instruction.operands[0], 1) ||
			!RequireTypedRegister(operands[1], RISCVRegisterClass::Float,
				instruction.operands[1], 2) ||
			!RequireTypedRegister(operands[2], RISCVRegisterClass::Float,
				instruction.operands[2], 3)) return false;
		if (count == 4 && (!InstructionUsesRounding(*info) ||
			!ParseRoundingOperand(operands[3], instruction,
			rounding_specified))) goto bad_count;
		return true;
	}
	if (info->operands == RISCVOperandForm::FdFs ||
		info->operands == RISCVOperandForm::RdFs ||
		info->operands == RISCVOperandForm::FdRs) {
		if (count != 2 && count != 3) goto bad_count;
		instruction.operand_count = 2;
		RISCVRegisterClass destination = info->operands == RISCVOperandForm::RdFs ?
			RISCVRegisterClass::Integer : RISCVRegisterClass::Float;
		RISCVRegisterClass source_class = info->operands == RISCVOperandForm::FdRs ?
			RISCVRegisterClass::Integer : RISCVRegisterClass::Float;
		if (!RequireTypedRegister(operands[0], destination,
			instruction.operands[0], 1) ||
			!RequireTypedRegister(operands[1], source_class,
				instruction.operands[1], 2)) return false;
		if (count == 3 && (!InstructionUsesRounding(*info) ||
			!ParseRoundingOperand(operands[2], instruction,
				rounding_specified))) goto bad_count;
		return true;
	}
	if (info->operands == RISCVOperandForm::FdFsFsFs) {
		if (count != 4 && count != 5) goto bad_count;
		instruction.operand_count = 4;
		for (unsigned index = 0; index < 4; index++)
			if (!RequireTypedRegister(operands[index], RISCVRegisterClass::Float,
				instruction.operands[index], index + 1)) return false;
		if (count == 5 && (!InstructionUsesRounding(*info) ||
			!ParseRoundingOperand(operands[4], instruction,
				rounding_specified))) goto bad_count;
		return true;
	}
	if (info->operands == RISCVOperandForm::VdVsVs ||
		info->operands == RISCVOperandForm::VdVsRs ||
		info->operands == RISCVOperandForm::VdVsImmediate) {
		if (count != 3 && count != 4) goto bad_count;
		instruction.operand_count = 3;
		if (!RequireTypedRegister(operands[0], RISCVRegisterClass::Vector,
			instruction.operands[0], 1) ||
			!RequireTypedRegister(operands[1], RISCVRegisterClass::Vector,
				instruction.operands[1], 2)) return false;
		if (info->operands == RISCVOperandForm::VdVsVs) {
			if (!RequireTypedRegister(operands[2], RISCVRegisterClass::Vector,
				instruction.operands[2], 3)) return false;
		}
		else if (info->operands == RISCVOperandForm::VdVsRs) {
			if (!RequireRegister(operands[2], instruction.operands[2], 3)) return false;
		}
		else if (!RequireImmediate(operands[2], parse_pass,
			instruction.operands[2], 3)) return false;
		if (count == 4 && !ParseRISCVVectorMask(operands[3], instruction.masked))
			goto bad_count;
		return true;
	}
	if (info->operands == RISCVOperandForm::RdRsVtype ||
		info->operands == RISCVOperandForm::RdImmediateVtype) {
		if (count < 4 || count > 6) goto bad_count;
		instruction.operand_count = 2;
		if (!RequireRegister(operands[0], instruction.operands[0], 1)) return false;
		if (info->operands == RISCVOperandForm::RdRsVtype) {
			if (!RequireRegister(operands[1], instruction.operands[1], 2)) return false;
		}
		else if (!RequireImmediate(operands[1], parse_pass,
			instruction.operands[1], 2)) return false;
		if (!ParseRISCVVectorType(operands, 2, count, instruction.vector_type)) {
			aasm_log(_LOG_ERROR, "RISC-V vector type is invalid");
			return false;
		}
		return true;
	}
	if (info->operands == RISCVOperandForm::RdRsRs) {
		if (count != 3) goto bad_count;
		instruction.operand_count = 3;
		return RequireRegister(operands[0], instruction.operands[0], 1) &&
			RequireRegister(operands[1], instruction.operands[1], 2) &&
			RequireRegister(operands[2], instruction.operands[2], 3);
	}
	if (info->operands == RISCVOperandForm::VdAddress ||
		info->operands == RISCVOperandForm::VsAddress) {
		if (count != 2) goto bad_count;
		instruction.operand_count = 2;
		return RequireTypedRegister(operands[0], RISCVRegisterClass::Vector,
			instruction.operands[0], 1) &&
			RequireAddress(operands[1], parse_pass, false,
				instruction.operands[1], 2) &&
			instruction.operands[1].address_kind == RISCVAddressKind::RegisterRelative;
	}
	if (info->operands == RISCVOperandForm::CRegImmediate) {
		if (count != 2) goto bad_count;
		instruction.operand_count = 2;
		return RequireRegister(operands[0], instruction.operands[0], 1) &&
			RequireImmediate(operands[1], parse_pass, instruction.operands[1], 2);
	}
	if (info->operands == RISCVOperandForm::CRegSpImmediate) {
		if (count != 3) goto bad_count;
		instruction.operand_count = 3;
		return RequireRegister(operands[0], instruction.operands[0], 1) &&
			RequireRegister(operands[1], instruction.operands[1], 2) &&
			RequireImmediate(operands[2], parse_pass, instruction.operands[2], 3);
	}
	if (info->operands == RISCVOperandForm::CReg) {
		if (count != 2) goto bad_count;
		instruction.operand_count = 2;
		return RequireRegister(operands[0], instruction.operands[0], 1) &&
			RequireRegister(operands[1], instruction.operands[1], 2);
	}
	if (info->operands == RISCVOperandForm::CRegister) {
		if (count != 1) goto bad_count;
		instruction.operand_count = 1;
		return RequireRegister(operands[0], instruction.operands[0], 1);
	}
	if (info->operands == RISCVOperandForm::CJump) {
		if (count != 1 && count != 2) goto bad_count;
		instruction.operand_count = 1;
		if (!RequireTarget(operands[0], parse_pass,
			instruction.operands[0], 1)) return false;
		if (count == 2) {
			RISCVOperand temporary;
			if (!RequireRegister(operands[1], temporary, 2)) return false;
			if (!temporary.reg) {
				aasm_log(_LOG_ERROR,
					"RISC-V long jump temporary register cannot be zero");
				return false;
			}
			instruction.temporary_reg = temporary.reg;
			instruction.has_temporary = true;
		}
		return true;
	}
	if (info->operands == RISCVOperandForm::CBranch) {
		if (count != 2 && count != 3) goto bad_count;
		instruction.operand_count = 2;
		if (!RequireRegister(operands[0], instruction.operands[0], 1) ||
			!RequireTarget(operands[1], parse_pass,
				instruction.operands[1], 2)) return false;
		if (count == 3) {
			RISCVOperand temporary;
			if (!RequireRegister(operands[2], temporary, 3)) return false;
			if (!temporary.reg) {
				aasm_log(_LOG_ERROR,
					"RISC-V long branch temporary register cannot be zero");
				return false;
			}
			instruction.temporary_reg = temporary.reg;
			instruction.has_temporary = true;
		}
		return true;
	}
	if (info->operands == RISCVOperandForm::CLoad ||
		info->operands == RISCVOperandForm::CStore ||
		info->operands == RISCVOperandForm::CStackLoad ||
		info->operands == RISCVOperandForm::CStackStore) {
		if (count != 2) goto bad_count;
		instruction.operand_count = 2;
		return RequireRegister(operands[0], instruction.operands[0], 1) &&
			RequireAddress(operands[1], parse_pass, false,
				instruction.operands[1], 2);
	}
	if (info->operands == RISCVOperandForm::CFloatLoad ||
		info->operands == RISCVOperandForm::CFloatStore ||
		info->operands == RISCVOperandForm::CFloatStackLoad ||
		info->operands == RISCVOperandForm::CFloatStackStore) {
		if (count != 2) goto bad_count;
		instruction.operand_count = 2;
		return RequireTypedRegister(operands[0], RISCVRegisterClass::Float,
			instruction.operands[0], 1) &&
			RequireAddress(operands[1], parse_pass, false,
				instruction.operands[1], 2);
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
