// ASCII C++11 TAB4 LF
// Docutitle: AASM RISC-V Instruction Parser
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../../asm/inc/aasm.h"
#include "../../../../../inc/cpp/Lango/Archend/riscv/Instruction-riscv.hpp"

namespace uni::lango {

namespace {

struct OpcodeEntry {
	const char* name;
	RISCVOpcode opcode;
};

const OpcodeEntry Opcodes[] = {
	{"lui", RISCVOpcode::Lui}, {"auipc", RISCVOpcode::Auipc},
	{"jal", RISCVOpcode::Jal}, {"jalr", RISCVOpcode::Jalr},
	{"beq", RISCVOpcode::Beq}, {"bne", RISCVOpcode::Bne},
	{"blt", RISCVOpcode::Blt}, {"bge", RISCVOpcode::Bge},
	{"bltu", RISCVOpcode::Bltu}, {"bgeu", RISCVOpcode::Bgeu},
	{"lb", RISCVOpcode::Lb}, {"lh", RISCVOpcode::Lh},
	{"lw", RISCVOpcode::Lw}, {"lbu", RISCVOpcode::Lbu},
	{"lhu", RISCVOpcode::Lhu}, {"lwu", RISCVOpcode::Lwu},
	{"ld", RISCVOpcode::Ld}, {"sb", RISCVOpcode::Sb},
	{"sh", RISCVOpcode::Sh}, {"sw", RISCVOpcode::Sw},
	{"sd", RISCVOpcode::Sd}, {"addi", RISCVOpcode::Addi},
	{"slti", RISCVOpcode::Slti}, {"sltiu", RISCVOpcode::Sltiu},
	{"xori", RISCVOpcode::Xori}, {"ori", RISCVOpcode::Ori},
	{"andi", RISCVOpcode::Andi}, {"slli", RISCVOpcode::Slli},
	{"srli", RISCVOpcode::Srli}, {"srai", RISCVOpcode::Srai},
	{"add", RISCVOpcode::Add}, {"sub", RISCVOpcode::Sub},
	{"sll", RISCVOpcode::Sll}, {"slt", RISCVOpcode::Slt},
	{"sltu", RISCVOpcode::Sltu}, {"xor", RISCVOpcode::Xor},
	{"srl", RISCVOpcode::Srl}, {"sra", RISCVOpcode::Sra},
	{"or", RISCVOpcode::Or}, {"and", RISCVOpcode::And},
	{"addiw", RISCVOpcode::Addiw}, {"slliw", RISCVOpcode::Slliw},
	{"srliw", RISCVOpcode::Srliw}, {"sraiw", RISCVOpcode::Sraiw},
	{"addw", RISCVOpcode::Addw}, {"subw", RISCVOpcode::Subw},
	{"sllw", RISCVOpcode::Sllw}, {"srlw", RISCVOpcode::Srlw},
	{"sraw", RISCVOpcode::Sraw}, {"fence", RISCVOpcode::Fence},
	{"ecall", RISCVOpcode::Ecall}, {"ebreak", RISCVOpcode::Ebreak}
};

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

RISCVOpcode FindOpcode(const char* name)
{
	for (const OpcodeEntry& entry : Opcodes)
		if (Equal(name, entry.name)) return entry.opcode;
	return RISCVOpcode::None;
}

bool IsAlias(const char* name)
{
	static const char* const aliases[] = {
		"j", "jr", "mov", "movu", "nop", "ret"
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

bool ParseAddressOperand(char* text, int parse_pass, bool require_base,
	bool require_width, RISCVOperand& operand)
{
	text = SkipSpace(text);
	uint8_t width = 0;
	ParseWidth(text, width);
	if (require_width && !width) return false;
	if (*text != '[') return false;
	char* close = text + StrLength(text);
	while (close > text && ascii_isspace(close[-1])) close--;
	if (close <= text + 1 || close[-1] != ']') return false;
	close[-1] = '\0';
	char* content = SkipSpace(text + 1);
	TrimEnd(content);

	operand.kind = RISCVOperandKind::Address;
	operand.width = width;
	if (!require_base) return ParseValue(content, parse_pass, operand.value);

	char* register_end = content;
	while (isidchar(*register_end)) register_end++;
	char saved = *register_end;
	*register_end = '\0';
	if (!ParseRegister(content, operand.reg)) {
		*register_end = saved;
		return false;
	}
	*register_end = saved;
	char* displacement = SkipSpace(register_end);
	if (!*displacement) return true;
	if (*displacement != '+' && *displacement != '-') return false;
	return ParseValue(displacement, parse_pass, operand.value);
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

bool RequireAddress(char* text, int parse_pass, bool base, bool width,
	RISCVOperand& operand, unsigned number)
{
	if (ParseAddressOperand(text, parse_pass, base, width, operand)) return true;
	aasm_log(_LOG_ERROR, "RISC-V operand %u: address expression expected", number);
	return false;
}

bool IsRegisterOperation(RISCVOpcode opcode)
{
	return (opcode >= RISCVOpcode::Add && opcode <= RISCVOpcode::And) ||
		(opcode >= RISCVOpcode::Addw && opcode <= RISCVOpcode::Sraw);
}

bool IsImmediateOperation(RISCVOpcode opcode)
{
	return (opcode >= RISCVOpcode::Addi && opcode <= RISCVOpcode::Srai) ||
		(opcode >= RISCVOpcode::Addiw && opcode <= RISCVOpcode::Sraiw);
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

}

bool RISCVParser::IsInstructionStart(const char* word)
{
	return FindOpcode(word) != RISCVOpcode::None || IsAlias(word);
}

bool RISCVParser::Parse(AssemblySession& session, char* source,
	int parse_pass, RISCVInstruction& instruction)
{
	char* mnemonic_end = source;
	while (isidchar(*mnemonic_end)) mnemonic_end++;
	char saved = *mnemonic_end;
	*mnemonic_end = '\0';
	char mnemonic[16];
	StrCopyN(mnemonic, source, sizeof(mnemonic));
	*mnemonic_end = saved;
	char* operand_text = SkipSpace(mnemonic_end);
	char* operands[3];
	unsigned count = SplitOperands(operand_text, operands, numsof(operands));
	if (count > numsof(operands)) {
		aasm_log(_LOG_ERROR, "RISC-V: invalid operand list");
		return false;
	}

	if (Equal(mnemonic, "nop")) {
		if (count) goto bad_count;
		instruction.opcode = RISCVOpcode::Addi;
		instruction.operand_count = 3;
		instruction.operands[0].kind = RISCVOperandKind::Register;
		instruction.operands[1].kind = RISCVOperandKind::Register;
		instruction.operands[2].kind = RISCVOperandKind::Immediate;
		return true;
	}
	if (Equal(mnemonic, "ret")) {
		if (count) goto bad_count;
		instruction.opcode = RISCVOpcode::Jalr;
		instruction.operand_count = 2;
		instruction.operands[0].kind = RISCVOperandKind::Register;
		instruction.operands[1].kind = RISCVOperandKind::Address;
		instruction.operands[1].reg = 1;
		return true;
	}
	if (Equal(mnemonic, "j")) {
		if (count != 1) goto bad_count;
		instruction.opcode = RISCVOpcode::Jal;
		instruction.operand_count = 2;
		instruction.operands[0].kind = RISCVOperandKind::Register;
		if (!RequireAddress(operands[0], parse_pass, false, false,
			instruction.operands[1], 1)) return false;
		return true;
	}
	if (Equal(mnemonic, "jr")) {
		if (count != 1) goto bad_count;
		instruction.opcode = RISCVOpcode::Jalr;
		instruction.operand_count = 2;
		instruction.operands[0].kind = RISCVOperandKind::Register;
		if (!RequireRegister(operands[0], instruction.operands[1], 1)) return false;
		instruction.operands[1].kind = RISCVOperandKind::Address;
		return true;
	}
	if (Equal(mnemonic, "mov") || Equal(mnemonic, "movu")) {
		if (count != 2) goto bad_count;
		bool zero_extend = Equal(mnemonic, "movu");
		RISCVOperand destination;
		if (ParseRegisterOperand(operands[0], destination)) {
			RISCVOperand source_operand;
			if (ParseRegisterOperand(operands[1], source_operand)) {
				if (zero_extend) {
					aasm_log(_LOG_ERROR, "RISC-V MOVU requires a memory source");
					return false;
				}
				instruction.opcode = RISCVOpcode::Addi;
				instruction.operand_count = 3;
				instruction.operands[0] = destination;
				instruction.operands[1] = source_operand;
				instruction.operands[2].kind = RISCVOperandKind::Immediate;
				return true;
			}
			if (!RequireAddress(operands[1], parse_pass, true, true,
				source_operand, 2)) return false;
			instruction.opcode = LoadOpcode(
				source_operand.width, zero_extend, session.CurrentBits);
			if (instruction.opcode == RISCVOpcode::None) {
				aasm_log(_LOG_ERROR, "RISC-V MOV load width is not supported");
				return false;
			}
			instruction.operand_count = 2;
			instruction.operands[0] = destination;
			instruction.operands[1] = source_operand;
			return true;
		}
		if (zero_extend) {
			aasm_log(_LOG_ERROR, "RISC-V MOVU cannot store to memory");
			return false;
		}
		if (!RequireAddress(operands[0], parse_pass, true, true,
			destination, 1) ||
			!RequireRegister(operands[1], instruction.operands[0], 2)) return false;
		instruction.opcode = StoreOpcode(destination.width);
		if (instruction.opcode == RISCVOpcode::None) {
			aasm_log(_LOG_ERROR, "RISC-V MOV store width is not supported");
			return false;
		}
		instruction.operand_count = 2;
		instruction.operands[1] = destination;
		return true;
	}

	instruction.opcode = FindOpcode(mnemonic);
	if (instruction.opcode == RISCVOpcode::None) {
		aasm_log(_LOG_ERROR, "RISC-V instruction expected");
		return false;
	}
	if (instruction.opcode == RISCVOpcode::Fence ||
		instruction.opcode == RISCVOpcode::Ecall ||
		instruction.opcode == RISCVOpcode::Ebreak) {
		if (count) goto bad_count;
		return true;
	}
	if (instruction.opcode == RISCVOpcode::Lui ||
		instruction.opcode == RISCVOpcode::Auipc) {
		if (count != 2) goto bad_count;
		instruction.operand_count = 2;
		return RequireRegister(operands[0], instruction.operands[0], 1) &&
			RequireImmediate(operands[1], parse_pass, instruction.operands[1], 2);
	}
	if (instruction.opcode == RISCVOpcode::Jal) {
		if (count != 1 && count != 2) goto bad_count;
		instruction.operand_count = 2;
		if (count == 1) {
			instruction.operands[0].kind = RISCVOperandKind::Register;
			instruction.operands[0].reg = 1;
			return RequireAddress(operands[0], parse_pass, false, false,
				instruction.operands[1], 1);
		}
		return RequireRegister(operands[0], instruction.operands[0], 1) &&
			RequireAddress(operands[1], parse_pass, false, false,
				instruction.operands[1], 2);
	}
	if (instruction.opcode == RISCVOpcode::Jalr) {
		if (count != 2) goto bad_count;
		instruction.operand_count = 2;
		return RequireRegister(operands[0], instruction.operands[0], 1) &&
			RequireAddress(operands[1], parse_pass, true, false,
				instruction.operands[1], 2);
	}
	if (instruction.opcode >= RISCVOpcode::Beq &&
		instruction.opcode <= RISCVOpcode::Bgeu) {
		if (count != 3) goto bad_count;
		instruction.operand_count = 3;
		return RequireRegister(operands[0], instruction.operands[0], 1) &&
			RequireRegister(operands[1], instruction.operands[1], 2) &&
			RequireAddress(operands[2], parse_pass, false, false,
				instruction.operands[2], 3);
	}
	if (instruction.opcode >= RISCVOpcode::Lb &&
		instruction.opcode <= RISCVOpcode::Ld) {
		if (count != 2) goto bad_count;
		instruction.operand_count = 2;
		return RequireRegister(operands[0], instruction.operands[0], 1) &&
			RequireAddress(operands[1], parse_pass, true, false,
				instruction.operands[1], 2);
	}
	if (instruction.opcode >= RISCVOpcode::Sb &&
		instruction.opcode <= RISCVOpcode::Sd) {
		if (count != 2) goto bad_count;
		instruction.operand_count = 2;
		return RequireRegister(operands[0], instruction.operands[0], 1) &&
			RequireAddress(operands[1], parse_pass, true, false,
				instruction.operands[1], 2);
	}
	if (IsRegisterOperation(instruction.opcode)) {
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
	if (IsImmediateOperation(instruction.opcode)) {
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
}

}
