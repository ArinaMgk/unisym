// ASCII C++11 TAB4 LF
// Docutitle: AASM RISC-V Architecture Assembler
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../asm/inc/aasm.h"
#include "../../../../inc/cpp/Format/Code.hpp"
#include "../../../../inc/cpp/Lango/Archend/Archend-riscv.hpp"
#include "../../../../inc/cpp/Lango/Archend/riscv/Instruction-riscv.hpp"
#include "../../../../inc/cpp/Lango/Archend/riscv/Target-riscv.hpp"

namespace uni::lango {

namespace {

char* SkipSpace(char* text)
{
	while (*text && ascii_isspace(*text)) text++;
	return text;
}

void SetStatementDebugLine(AssemblySession& session)
{
	static int32_t line_number = 0;
	static char* line_filename = nullptr;
	if (src_get(&line_number, &line_filename))
		session.Options->Target.Codeend().SetDebugLine(
			line_filename, line_number, session.Location.segment);
}

void EmitAlignmentCode(AssemblySession& session, int64_t offset,
	const void* data, uint64_t size)
{
	uni::CodeendOutput output {
		session.Location.segment,
		data,
		uni::CodeendOutputType::RawData,
		size
	};
	session.Listing->EmitCode(offset, output);
	SetStatementDebugLine(session);
	session.Options->Target.Codeend().Emit(output);
}

void AppendUnsigned(char*& cursor, uint64_t value)
{
	char digits[24];
	unsigned count = 0;
	do {
		digits[count++] = static_cast<char>('0' + value % 10);
		value /= 10;
	} while (value);
	while (count) *cursor++ = digits[--count];
}

void MakePairAnchor(char* output, int32_t section, uint64_t offset)
{
	char* cursor = output;
	const char prefix[] = "..@riscv.pcrel.";
	for (const char* item = prefix; *item; item++) *cursor++ = *item;
	if (section < 0) {
		*cursor++ = '-';
		AppendUnsigned(cursor, static_cast<uint64_t>(-static_cast<int64_t>(section)));
	}
	else AppendUnsigned(cursor, static_cast<uint64_t>(section));
	*cursor++ = '.';
	AppendUnsigned(cursor, offset);
	*cursor = '\0';
}

constexpr uint32_t Extension(RISCVExtension extension)
{
	return RISCVExtensionMask(extension);
}

enum class RISCVDataRelocationStatement {
	None,
	PcRelative,
	Got,
	Plt
};

RISCVDataRelocationStatement ParseDataRelocationStatement(
	const char* mnemonic)
{
	if (!StrCompareInsensitive(mnemonic, "pcrel32"))
		return RISCVDataRelocationStatement::PcRelative;
	if (!StrCompareInsensitive(mnemonic, "got32"))
		return RISCVDataRelocationStatement::Got;
	if (!StrCompareInsensitive(mnemonic, "plt32"))
		return RISCVDataRelocationStatement::Plt;
	return RISCVDataRelocationStatement::None;
}

}

const ArchendInfo& ArchendRISCV::DumpArchend() const
{
	static const ArchendInfo info {
		"RISC-V base integer architecture",
		"riscv",
		ArchitectureKind::RISCV,
		64,
		"32/64",
		"",
		"I, M, A, F, D, C, V, Zba, Zbb, Zbs, Zicond, Zicsr, Zifencei "
		"(combine in canonical order; default I)"
	};
	return info;
}

uint32_t ArchendRISCV::DefaultCpu() const
{
	return Extension(RISCVExtension::I);
}

bool ArchendRISCV::ParseCpu(const char* name, uint32_t& cpu) const
{
	return ParseRISCVInstructionSet(name, cpu);
}

bool ArchendRISCV::ValidateBits(int bits, uint32_t cpu) const
{
	if (bits != 32 && bits != 64) return false;
	return ValidateRISCVInstructionSet(cpu);
}

uint32_t ArchendRISCV::DefaultAbi(int bits, uint32_t cpu) const
{
	(void)cpu;
	return static_cast<uint32_t>(DefaultRISCVABI(bits));
}

bool ArchendRISCV::ParseAbi(const char* name, uint32_t& abi) const
{
	RISCVABI parsed;
	if (!ParseRISCVABI(name, parsed)) return false;
	abi = static_cast<uint32_t>(parsed);
	return true;
}

bool ArchendRISCV::ValidateAbi(int bits, uint32_t cpu, uint32_t abi) const
{
	return ValidateRISCVABI(bits, cpu, static_cast<RISCVABI>(abi));
}

bool ArchendRISCV::ProcessDefault(char* value) const
{
	(void)value;
	return false;
}

void ArchendRISCV::Initialize(AssemblySession& session)
{
	(void)session;
}

void ArchendRISCV::BeginPass(AssemblySession& session,
	int parse_pass, int output_pass)
{
	(void)session;
	(void)parse_pass;
	(void)output_pass;
}

bool ArchendRISCV::ProcessArchitectureStatement(AssemblySession& session,
	char* source, int64_t repeat, int parse_pass, int output_pass,
	ArchendLineResult& result)
{
	(void)output_pass;
	char* mnemonic = SkipSpace(source);
	char* end = mnemonic;
	while (isidchar(*end)) end++;
	char saved = *end;
	*end = '\0';
	bool alignment_statement = !StrCompareInsensitive(mnemonic, "ralign");
	RISCVDataRelocationStatement data_statement =
		ParseDataRelocationStatement(mnemonic);
	*end = saved;
	if (!alignment_statement &&
		data_statement == RISCVDataRelocationStatement::None) return false;

	if (data_statement != RISCVDataRelocationStatement::None) {
		stdscan_reset();
		stdscan_bufptr = SkipSpace(end);
		struct tokenval token;
		token.t_type = TOKEN_INVALID;
		EvalRelocExpression relocation {};
		expr* value = EvaluateRelocExpression(common_stdscan, nullptr, &token,
			nullptr, parse_pass == 2 ? 2 : 0, nullptr, relocation);
		if (!value || token.t_type != TOKEN_EOS) {
			aasm_log(_LOG_ERROR,
				"RISC-V 32-bit data relocation expects one expression");
			result.valid = false;
			return true;
		}
		result.size = repeat * 4;
		if (parse_pass == 1) return true;
		if (!relocation.Available || relocation.TermCount != 1 ||
			relocation.Terms[0].Coefficient != 1) {
			aasm_log(_LOG_ERROR,
				"RISC-V 32-bit data relocation expects a symbol plus an addend");
			result.valid = false;
			return true;
		}
		const EvalRelocTerm& target = relocation.Terms[0];
		uni::CodeFixupKind kind = uni::CodeFixupKind::PcRelative;
		if (data_statement == RISCVDataRelocationStatement::Got)
			kind = uni::CodeFixupKind::Got;
		else if (data_statement == RISCVDataRelocationStatement::Plt)
			kind = uni::CodeFixupKind::Plt;
		for (int64_t index = 0; index < repeat; index++) {
			uni::CodeFixup fixup {};
			fixup.section = session.Location.segment;
			fixup.offset = static_cast<uint64_t>(
				session.Location.offset + index * 4);
			fixup.kind = kind;
			fixup.encoding.storage_bits = 32;
			fixup.encoding.value_bits = 32;
			fixup.encoding.signed_value = true;
			fixup.target_section = target.Section;
			fixup.addend = relocation.Constant;
			fixup.legacy_wrt = NO_SEG;
			fixup.architecture_kind =
				static_cast<uint32_t>(ArchitectureKind::RISCV);
			if (!target.External) {
				fixup.target.kind = uni::CodeFixupTargetKind::Symbol;
				fixup.target.section = target.Section;
				fixup.target.offset = target.Offset;
				fixup.target.global = target.Global;
			}
			session.Listing->EmitFixup(fixup);
			SetStatementDebugLine(session);
			session.Options->Target.Codeend().EmitFixup(fixup);
		}
		return true;
	}

	if (repeat != 1) {
		aasm_log(_LOG_ERROR, "RALIGN does not support TIMES");
		result.valid = false;
		return true;
	}
	stdscan_reset();
	stdscan_bufptr = SkipSpace(end);
	struct tokenval token;
	token.t_type = TOKEN_INVALID;
	expr* value = eval_evaluate(common_stdscan, nullptr, &token,
		nullptr, parse_pass == 2 ? 2 : 0, nullptr);
	if (!value || token.t_type != TOKEN_EOS || !is_simple(value)) {
		aasm_log(_LOG_ERROR, "RALIGN expects a constant alignment");
		result.valid = false;
		return true;
	}
	int64_t alignment = reloc_value(value);
	if (alignment < 1 || (alignment & (alignment - 1))) {
		aasm_log(_LOG_ERROR,
			"RALIGN alignment must be a positive power of two");
		result.valid = false;
		return true;
	}
	size_t instruction_size = (session.CurrentCpu &
		static_cast<uint32_t>(RISCVExtension::C)) ? 2 : 4;
	int64_t padding = session.Options->EnableRISCVRelax ?
		(alignment > static_cast<int64_t>(instruction_size) ?
			alignment - instruction_size : 0) :
		((alignment - (session.Location.offset & (alignment - 1))) &
			(alignment - 1));
	if (padding % static_cast<int64_t>(instruction_size)) {
		aasm_log(_LOG_ERROR,
			"RALIGN padding is incompatible with the RISC-V instruction size");
		result.valid = false;
		return true;
	}
	result.size = padding;
	if (parse_pass == 1 || !padding) return true;

	uint8_t bytes[256];
	for (size_t index = 0; index < sizeof(bytes); index += instruction_size) {
		bytes[index] = instruction_size == 2 ? 0x01 : 0x13;
		bytes[index + 1] = 0;
		if (instruction_size == 4) {
			bytes[index + 2] = 0;
			bytes[index + 3] = 0;
		}
	}
	if (session.Options->EnableRISCVRelax) {
		int64_t listing_offset = session.Location.offset;
		for (int64_t remaining = padding; remaining > 0;) {
			size_t chunk = remaining < static_cast<int64_t>(sizeof(bytes)) ?
				static_cast<size_t>(remaining) : sizeof(bytes);
			session.Listing->EmitCode(listing_offset, uni::CodeendOutput {
				session.Location.segment, bytes,
				uni::CodeendOutputType::RawData, chunk
			});
			listing_offset += chunk;
			remaining -= chunk;
		}
		uni::CodeFixup fixup {
			session.Location.segment,
			static_cast<uint64_t>(session.Location.offset),
			uni::CodeFixupKind::Alignment,
			{0, 0, 0, 0, false},
			instruction_size,
			NO_SEG,
			padding,
			0,
			NO_SEG,
			static_cast<uint32_t>(ArchitectureKind::RISCV)
		};
		SetStatementDebugLine(session);
		session.Options->Target.Codeend().EmitFixup(fixup);
	}
	else {
		int64_t output_offset = session.Location.offset;
		for (int64_t remaining = padding; remaining > 0;) {
			size_t chunk = remaining < static_cast<int64_t>(sizeof(bytes)) ?
				static_cast<size_t>(remaining) : sizeof(bytes);
			EmitAlignmentCode(session, output_offset, bytes, chunk);
			output_offset += chunk;
			remaining -= chunk;
		}
	}
	return true;
}

bool ArchendRISCV::IsArchitectureStatementStart(const char* word) const
{
	return !StrCompareInsensitive(word, "ralign") ||
		ParseDataRelocationStatement(word) !=
			RISCVDataRelocationStatement::None;
}

bool ArchendRISCV::IsInstructionStart(const char* word) const
{
	return RISCVParser::IsInstructionStart(word);
}

ArchendLineResult ArchendRISCV::ProcessInstruction(AssemblySession& session,
	char* source, int64_t repeat, int parse_pass, int output_pass)
{
	(void)output_pass;
	ArchendLineResult result;
	RISCVInstruction instruction;
	if (!RISCVParser::Parse(session, source, parse_pass, instruction)) {
		result.valid = false;
		return result;
	}
	int64_t measured_offset = session.Location.offset;
	for (int64_t index = 0; index < repeat; index++) {
		unsigned measured_size;
		if (!RISCVEncoder::Measure(
			session, instruction, measured_offset, measured_size)) {
			result.valid = false;
			return result;
		}
		result.size += measured_size;
		measured_offset += measured_size;
	}
	if (parse_pass == 1) return result;

	uni::CodeendInterface& codeend = session.Options->Target.Codeend();
	int64_t offset = session.Location.offset;
	for (int64_t index = 0; index < repeat; index++) {
		unsigned measured_size;
		if (!RISCVEncoder::Measure(
			session, instruction, offset, measured_size)) {
			result.valid = false;
			return result;
		}
		RISCVEncodingSequence sequence;
		if (!RISCVEncoder::Encode(session, instruction, offset, sequence)) {
			result.valid = false;
			return result;
		}
		unsigned encoded_size = 0;
		for (unsigned item = 0; item < sequence.count; item++)
			encoded_size += sequence.instructions[item].size;
		if (encoded_size != measured_size) {
			aasm_log(_LOG_PANIC,
				"RISC-V instruction expansion changed during output");
			result.valid = false;
			return result;
		}
		int64_t instruction_offset = offset;
		for (unsigned item = 0; item < sequence.count; item++) {
			RISCVEncodingResult& encoded = sequence.instructions[item];
			if (encoded.fixup != RISCVFixupKind::None) {
				bool paired = encoded.pair_role != RISCVFixupPairRole::None;
				bool pair_fixup = encoded.fixup == RISCVFixupKind::GotHigh ||
					encoded.fixup == RISCVFixupKind::PcRelativeHigh ||
					encoded.fixup == RISCVFixupKind::PcRelativeLowI ||
					encoded.fixup == RISCVFixupKind::PcRelativeLowS;
				if (paired != pair_fixup) {
					aasm_log(_LOG_PANIC,
						"RISC-V paired fixup metadata is inconsistent");
					result.valid = false;
					return result;
				}
				if (encoded.pair_role == RISCVFixupPairRole::Head) {
					char anchor[96];
					MakePairAnchor(anchor, session.Location.segment,
						encoded.pair_offset);
					codeend.DefineSymbol(anchor, session.Location.segment,
						static_cast<int64_t>(encoded.pair_offset), 0, nullptr);
				}
				uni::CodeFixupKind kind;
				uni::CodeFixupEncoding encoding;
				switch (encoded.fixup) {
				case RISCVFixupKind::Branch:
					kind = uni::CodeFixupKind::Branch;
					encoding = {32, 13, 0, 1, true};
					break;
				case RISCVFixupKind::Jump:
					kind = instruction.operands[0].reg ?
						uni::CodeFixupKind::Call : uni::CodeFixupKind::Branch;
					encoding = {32, 21, 0, 1, true};
					break;
				case RISCVFixupKind::Call:
					kind = uni::CodeFixupKind::Call;
					encoding = {64, 32, 0, 0, true};
					break;
				case RISCVFixupKind::CallPlt:
					kind = uni::CodeFixupKind::Plt;
					encoding = {64, 32, 0, 0, true};
					break;
				case RISCVFixupKind::High:
					kind = uni::CodeFixupKind::High;
					encoding = {32, 20, 12, 12, true};
					break;
				case RISCVFixupKind::LowI:
					kind = uni::CodeFixupKind::Low;
					encoding = {32, 12, 20, 0, true};
					break;
				case RISCVFixupKind::LowS:
					kind = uni::CodeFixupKind::Low;
					encoding = {32, 12, 7, 0, true};
					break;
				case RISCVFixupKind::GotHigh:
					kind = uni::CodeFixupKind::Got;
					encoding = {32, 20, 12, 12, true};
					break;
				case RISCVFixupKind::CompressedBranch:
					kind = uni::CodeFixupKind::Branch;
					encoding = {16, 9, 0, 1, true};
					break;
				case RISCVFixupKind::CompressedJump:
					kind = uni::CodeFixupKind::Branch;
					encoding = {16, 12, 0, 1, true};
					break;
				case RISCVFixupKind::PcRelativeHigh:
					kind = uni::CodeFixupKind::PageRelative;
					encoding = {32, 20, 12, 12, true};
					break;
				case RISCVFixupKind::PcRelativeLowI:
					kind = uni::CodeFixupKind::PageOffset;
					encoding = {32, 12, 20, 0, true};
					break;
				case RISCVFixupKind::PcRelativeLowS:
					kind = uni::CodeFixupKind::PageOffset;
					encoding = {32, 12, 7, 0, true};
					break;
				default:
					aasm_log(_LOG_PANIC, "invalid RISC-V fixup kind");
					result.valid = false;
					return result;
				}
				uni::CodeFixup fixup {
					session.Location.segment,
					static_cast<uint64_t>(instruction_offset),
					kind,
					encoding,
					encoded.encoding,
					encoded.target.section,
					encoded.target.symbol ? encoded.target.addend :
						encoded.target.offset,
					0,
					encoded.target.wrt,
					static_cast<uint32_t>(ArchitectureKind::RISCV),
					{
						paired ? session.Location.segment : NO_SEG,
						encoded.pair_offset,
						encoded.pair_role == RISCVFixupPairRole::Head ?
							uni::CodeFixupPairRole::Head :
							(encoded.pair_role == RISCVFixupPairRole::Tail ?
								uni::CodeFixupPairRole::Tail :
								uni::CodeFixupPairRole::None)
					},
					session.Options->EnableRISCVRelax
				};
				if (encoded.target.symbol) {
					fixup.target.kind = uni::CodeFixupTargetKind::Symbol;
					fixup.target.section = encoded.target.section;
					fixup.target.offset = encoded.target.symbol_offset;
					fixup.target.global = encoded.target.global;
				}
				session.Listing->EmitFixup(fixup);
				codeend.EmitFixup(fixup);
				instruction_offset += encoded.size;
				continue;
			}
			uint8_t bytes[4] = {
				static_cast<uint8_t>(encoded.encoding),
				static_cast<uint8_t>(encoded.encoding >> 8),
				static_cast<uint8_t>(encoded.encoding >> 16),
				static_cast<uint8_t>(encoded.encoding >> 24)
			};
			uni::CodeendOutput output {
				session.Location.segment,
				bytes,
				uni::CodeendOutputType::RawData,
				encoded.size
			};
			session.Listing->EmitCode(instruction_offset, output);
			codeend.Emit(output);
			instruction_offset += encoded.size;
		}
		offset += measured_size;
	}
	return result;
}

void ArchendRISCV::EndPass(AssemblySession& session)
{
	(void)session;
}

void ArchendRISCV::Cleanup(AssemblySession& session)
{
	(void)session;
}

ArchendInterface& RISCVArchend()
{
	static ArchendRISCV archend;
	return archend;
}

}
