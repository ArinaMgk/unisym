// ASCII C++11 TAB4 LF
// Docutitle: AASM Assembly Statement Parser
// Copyright: UNISYM, under Apache License 2.0

#include "inc/aasm.h"
#include "../inc/cpp/Format/Code.hpp"
#include "../inc/cpp/Lango/Archend.hpp"

_ESYM_C {
extern bool using_debug_info;
}

namespace {

enum class CommonStatementKind {
	None,
	Data,
	Reserve,
	Incbin
};

enum class DataItemKind {
	None,
	Number,
	String,
	OwnedString
};

struct DataItem {
	DataItem* Next;
	DataItemKind Kind;
	char* String;
	size_t Length;
	int64_t Offset;
	int32_t Section;
	int32_t Wrt;
};

char* SkipSpace(char* text)
{
	while (*text && ascii_isspace(*text)) text++;
	return text;
}

bool EqualWord(const char* text, const char* word, const char*& end)
{
	while (*word && *text && ascii_tolower(*text) == *word) {
		text++;
		word++;
	}
	end = text;
	return !*word && (!*text || ascii_isspace(*text) || *text == ';');
}

bool IsCommonStatementStart(const char* word)
{
	static const char* const words[] = {
		"db", "dd", "do", "dq", "dt", "dw", "dy", "equ", "incbin",
		"resb", "resd", "reso", "resq", "rest", "resw", "resy", "times"
	};
	for (const char* candidate : words)
		if (!StrCompareInsensitive(word, candidate)) return true;
	return false;
}

uint16_t DataBits(AssemblyDataWidth width)
{
	return static_cast<uint16_t>(width);
}

size_t DataBytes(AssemblyDataWidth width)
{
	return DataBits(width) / 8;
}

int32_t DataDebugType(AssemblyDataWidth width, bool floating)
{
	switch (width) {
	case AssemblyDataWidth::Bits8: return TY_BYTE;
	case AssemblyDataWidth::Bits16: return TY_WORD;
	case AssemblyDataWidth::Bits32: return floating ? TY_FLOAT : TY_DWORD;
	case AssemblyDataWidth::Bits64: return TY_QWORD;
	case AssemblyDataWidth::Bits80: return TY_TBYTE;
	case AssemblyDataWidth::Bits128: return TY_OWORD;
	case AssemblyDataWidth::Bits256: return TY_YWORD;
	}
	return TY_LABEL;
}

CommonStatementKind ClassifyCommonStatement(
	char* source, AssemblyDataWidth& width, char*& operands)
{
	char* end = source;
	while (isidchar(*end)) end++;
	char saved = *end;
	*end = '\0';
	CommonStatementKind kind = CommonStatementKind::None;

	struct Entry {
		const char* Name;
		CommonStatementKind Kind;
		AssemblyDataWidth Width;
	};
	static const Entry entries[] = {
		{"db", CommonStatementKind::Data, AssemblyDataWidth::Bits8},
		{"dw", CommonStatementKind::Data, AssemblyDataWidth::Bits16},
		{"dd", CommonStatementKind::Data, AssemblyDataWidth::Bits32},
		{"dq", CommonStatementKind::Data, AssemblyDataWidth::Bits64},
		{"dt", CommonStatementKind::Data, AssemblyDataWidth::Bits80},
		{"do", CommonStatementKind::Data, AssemblyDataWidth::Bits128},
		{"dy", CommonStatementKind::Data, AssemblyDataWidth::Bits256},
		{"resb", CommonStatementKind::Reserve, AssemblyDataWidth::Bits8},
		{"resw", CommonStatementKind::Reserve, AssemblyDataWidth::Bits16},
		{"resd", CommonStatementKind::Reserve, AssemblyDataWidth::Bits32},
		{"resq", CommonStatementKind::Reserve, AssemblyDataWidth::Bits64},
		{"rest", CommonStatementKind::Reserve, AssemblyDataWidth::Bits80},
		{"reso", CommonStatementKind::Reserve, AssemblyDataWidth::Bits128},
		{"resy", CommonStatementKind::Reserve, AssemblyDataWidth::Bits256},
		{"incbin", CommonStatementKind::Incbin, AssemblyDataWidth::Bits8}
	};
	for (const Entry& entry : entries) {
		if (!StrCompareInsensitive(source, entry.Name)) {
			kind = entry.Kind;
			width = entry.Width;
			break;
		}
	}
	*end = saved;
	operands = SkipSpace(end);
	return kind;
}

bool IsCommaNext()
{
	char* cursor = stdscan_bufptr;
	struct tokenval value;
	int token = common_stdscan(nullptr, &value);
	stdscan_bufptr = cursor;
	return token == ',' || token == ';' || token == TOKEN_EOS;
}

void FreeDataItems(DataItem* item)
{
	while (item) {
		DataItem* next = item->Next;
		if (item->Kind == DataItemKind::OwnedString) memf(item->String);
		memf(item);
		item = next;
	}
}

out_type StatementListingType(uni::CodeendOutputType type)
{
	switch (type) {
	case uni::CodeendOutputType::RawData: return OUT_RAWDATA;
	case uni::CodeendOutputType::Reserve: return OUT_RESERVE;
	}
	return OUT_RAWDATA;
}

void SetStatementDebugLine(AssemblySession& session)
{
	static int32_t line_number = 0;
	static char* line_filename = nullptr;
	if (src_get(&line_number, &line_filename))
		session.Options->Target.Codeend().SetDebugLine(
			line_filename, line_number, session.Location.segment);
}

void EmitStatementData(AssemblySession& session, int64_t offset,
	const void* data, uni::CodeendOutputType type, uint64_t size)
{
	aasmlist.output(offset, data, StatementListingType(type), size);
	SetStatementDebugLine(session);

	uni::CodeendOutput output {
		session.Location.segment,
		data,
		type,
		size
	};
	session.Options->Target.Codeend().Emit(output);
}

void EmitStatementFixup(AssemblySession& session, int64_t offset,
	int64_t addend, size_t size, int32_t target_section, int32_t wrt)
{
	aasmlist.output(offset, &addend, OUT_ADDRESS, size);
	SetStatementDebugLine(session);

	uni::CodeFixup fixup {
		session.Location.segment,
		static_cast<uint64_t>(offset),
		uni::CodeFixupKind::Absolute,
		{
			static_cast<uint16_t>(size * 8),
			static_cast<uint16_t>(size * 8),
			0,
			0,
			false
		},
		target_section,
		addend,
		0,
		wrt,
		0
	};
	session.Options->Target.Codeend().EmitFixup(fixup);
}

int64_t DataItemSize(const DataItem& item, size_t width)
{
	if (item.Kind == DataItemKind::Number) return width;
	if (item.Kind == DataItemKind::String ||
		item.Kind == DataItemKind::OwnedString) {
		size_t padding = item.Length % width;
		if (padding) padding = width - padding;
		return item.Length + padding;
	}
	return 0;
}

bool ReadRelocatableExpression(expr* value, int opflags,
	int32_t& section, int64_t& offset, int32_t& wrt, bool& external)
{
	external = !!(opflags & OPFLAG_EXTERN);
	if (is_unknown(value)) {
		section = NO_SEG;
		offset = 0;
		wrt = NO_SEG;
		return true;
	}
	if (!is_reloc(value)) return false;
	section = reloc_seg(value);
	offset = reloc_value(value);
	wrt = reloc_wrt(value);
	return true;
}

}

AssemblyStatementParser::AssemblyStatementParser(AssemblySession& session)
	: Session(session)
{
}

char* AssemblyStatementParser::ParseLabel(
	char*& source, bool& explicit_label)
{
	explicit_label = false;
	source = SkipSpace(source);
	if (!*source || *source == ';') return nullptr;

	char* label = source;
	if (*label == '$' && isidstart(label[1])) label++;
	if (!isidstart(*label)) return nullptr;

	char* end = label + 1;
	while (isidchar(*end)) end++;
	char* after = SkipSpace(end);
	explicit_label = *after == ':';

	char saved = *end;
	*end = '\0';
	bool instruction = IsCommonStatementStart(label) ||
		Session.ActiveArchend->IsInstructionStart(label);
	if (!explicit_label && instruction) {
		*end = saved;
		return nullptr;
	}

	if (explicit_label) after++;
	source = SkipSpace(after);
	return label;
}

bool AssemblyStatementParser::IsEquStatement(
	const char* source, char*& expression) const
{
	const char* end;
	if (!EqualWord(source, "equ", end)) return false;
	expression = SkipSpace((char*)end);
	return true;
}

bool AssemblyStatementParser::ParseRepeat(char*& source, int64_t& repeat)
{
	const char* word_end;
	if (!EqualWord(source, "times", word_end)) return true;

	char* expression = SkipSpace((char*)word_end);
	char* cursor = expression;
	char* separator = nullptr;
	int nesting = 0;
	bool have_expression = false;

	while (*cursor) {
		if (*cursor == ';') break;
		if (*cursor == '\'' || *cursor == '"' || *cursor == '`') {
			char quote = *cursor++;
			while (*cursor && *cursor != quote) cursor++;
			if (*cursor) cursor++;
			have_expression = true;
			continue;
		}
		if (*cursor == '(' || *cursor == '[') {
			nesting++;
			have_expression = true;
			cursor++;
			continue;
		}
		if (*cursor == ')' || *cursor == ']') {
			if (nesting) nesting--;
			have_expression = true;
			cursor++;
			continue;
		}
		if (ascii_isspace(*cursor)) {
			separator = cursor;
			cursor = SkipSpace(cursor);
			if (!*cursor) break;
		}
		if (nesting == 0 && have_expression && isidstart(*cursor)) {
			char* end = cursor + 1;
			while (isidchar(*end)) end++;
			char saved = *end;
			*end = '\0';
			bool instruction = IsCommonStatementStart(cursor) ||
				Session.ActiveArchend->IsInstructionStart(cursor);
			*end = saved;
			if (instruction && separator) {
				char saved_separator = *separator;
				*separator = '\0';
				stdscan_reset();
				stdscan_bufptr = expression;
				struct tokenval token;
				token.t_type = TOKEN_INVALID;
				expr* value = eval_evaluate(common_stdscan, nullptr, &token,
					nullptr, pass0, nullptr);
				*separator = saved_separator;
				if (!value || token.t_type != TOKEN_EOS) return false;
				if (!is_simple(value)) {
					aasm_log(_LOG_ERROR,
						"non-constant argument supplied to TIMES");
					repeat = 1;
				}
				else {
					repeat = reloc_value(value);
					if (repeat < 0 && pass0 == 2) {
						aasm_log(_LOG_ERROR,
							"TIMES value %d is negative", repeat);
						repeat = 0;
					}
				}
				source = cursor;
				return true;
			}
		}
		have_expression = true;
		cursor++;
	}

	aasm_log(_LOG_ERROR, "instruction expected after TIMES expression");
	return false;
}

void AssemblyStatementParser::DefineLabel(char* label)
{
	if (!label) return;
	int32_t section = Session.InAbsoluteSection ?
		Session.AbsoluteSection : Session.Location.segment;
	using DefineFunction = void (*)(rostr, int32_t, int64_t, char*, bool,
		bool, uni::CodeendInterface&);
	auto define = Session.PassNumber > 1 ?
		&Labelsys::Redefine : static_cast<DefineFunction>(&Labelsys::Define);
	define(label, section, Session.Location.offset, nullptr,
		true, false, Session.Options->Target.Codeend());
}

uni::lango::ArchendLineResult AssemblyStatementParser::ProcessEqu(
	char* label, char* expression, int parse_pass)
{
	uni::lango::ArchendLineResult result;
	if (!label) {
		aasm_log(_LOG_ERROR, "EQU not preceded by label");
		return result;
	}

	stdscan_reset();
	stdscan_bufptr = expression;
	struct tokenval token;
	token.t_type = TOKEN_INVALID;
	int opflags = 0;
	expr* first = eval_evaluate(common_stdscan, nullptr, &token,
		&opflags, parse_pass == 2 ? 2 : 0, nullptr);
	if (!first) return result;

	int32_t first_section;
	int64_t first_offset;
	int32_t first_wrt;
	bool external;
	if (!ReadRelocatableExpression(first, opflags,
		first_section, first_offset, first_wrt, external)) {
		aasm_log(_LOG_ERROR, "bad syntax for EQU");
		return result;
	}

	bool far_absolute = token.t_type == ':';
	int64_t second_offset = 0;
	if (far_absolute) {
		token.t_type = TOKEN_INVALID;
		opflags = 0;
		expr* second = eval_evaluate(common_stdscan, nullptr, &token,
			&opflags, parse_pass == 2 ? 2 : 0, nullptr);
		int32_t second_section;
		int32_t second_wrt;
		bool second_external;
		if (!second || !ReadRelocatableExpression(second, opflags,
			second_section, second_offset, second_wrt, second_external) ||
			first_section != NO_SEG || first_wrt != NO_SEG ||
			second_section != NO_SEG || second_wrt != NO_SEG ||
			token.t_type != TOKEN_EOS) {
			aasm_log(_LOG_ERROR, "bad syntax for EQU");
			return result;
		}
	}
	else if (token.t_type != TOKEN_EOS || first_wrt != NO_SEG) {
		aasm_log(_LOG_ERROR, "bad syntax for EQU");
		return result;
	}

	uni::CodeendInterface& codeend = Session.Options->Target.Codeend();
	if (parse_pass == 1) {
		if (label[0] == '.' && label[1] == '.' && label[2] != '@')
			return result;
		using DefineFunction = void (*)(rostr, int32_t, int64_t, char*, bool,
			bool, uni::CodeendInterface&);
		auto define = Session.PassNumber > 1 ?
			&Labelsys::Redefine : static_cast<DefineFunction>(&Labelsys::Define);
		define(label,
			far_absolute ? (int32_t)(first_offset | SEG_ABS) : first_section,
			far_absolute ? second_offset : first_offset,
			nullptr, false, far_absolute ? false : external, codeend);
	}
	else if (label[0] == '.' && label[1] == '.' && label[2] != '@') {
		Labelsys::Define(label,
			far_absolute ? (int32_t)(first_offset | SEG_ABS) : first_section,
			far_absolute ? second_offset : first_offset,
			nullptr, false, false, codeend);
	}
	return result;
}

bool AssemblyStatementParser::ProcessCommonStatement(
	char* source, int64_t repeat, int parse_pass,
	uni::lango::ArchendLineResult& result)
{
	AssemblyDataWidth width;
	char* operands;
	CommonStatementKind kind = ClassifyCommonStatement(source, width, operands);
	if (kind == CommonStatementKind::None) return false;

	size_t width_bytes = DataBytes(width);
	uni::CodeendInterface& codeend = Session.Options->Target.Codeend();

	if (kind == CommonStatementKind::Reserve) {
		stdscan_reset();
		stdscan_bufptr = operands;
		struct tokenval token;
		token.t_type = TOKEN_INVALID;
		expr* value = eval_evaluate(common_stdscan, nullptr, &token,
			nullptr, parse_pass == 2 ? 2 : 0, nullptr);
		int64_t count = 0;
		if (!value || token.t_type != TOKEN_EOS) {
			result.valid = false;
			return true;
		}
		if (is_unknown(value)) count = 0;
		else if (is_simple(value)) count = reloc_value(value);
		else {
			aasm_log(_LOG_ERROR, "non-constant reserve size");
			result.valid = false;
			return true;
		}

		int64_t unit_size = count * width_bytes;
		result.size = unit_size * repeat;
		if (parse_pass == 1) {
			if (using_debug_info)
				codeend.SetDebugType(TYS_ELEMENTS(unit_size) | TY_BYTE);
		}
		else if (result.size > 0) {
			EmitStatementData(Session, Session.Location.offset, nullptr,
				uni::CodeendOutputType::Reserve, result.size);
		}
		return true;
	}

	if (kind == CommonStatementKind::Incbin) {
		stdscan_reset();
		stdscan_bufptr = operands;
		struct tokenval token;
		int current = common_stdscan(nullptr, &token);
		if (current != TOKEN_STR) {
			aasm_log(_LOG_ERROR, "`incbin' expects a file name");
			result.valid = false;
			return true;
		}
		const char* filename = token.t_charptr;
		int64_t base = 0;
		int64_t limit = -1;
		current = common_stdscan(nullptr, &token);
		for (int parameter = 0; current == ',' && parameter < 2; parameter++) {
			token.t_type = TOKEN_INVALID;
			expr* value = eval_evaluate(common_stdscan, nullptr, &token,
				nullptr, parse_pass == 2 ? 2 : 0, nullptr);
			if (!value || (!is_simple(value) && !is_unknown(value))) {
				aasm_log(_LOG_ERROR, "`incbin' numeric parameter expected");
				result.valid = false;
				return true;
			}
			int64_t number = is_unknown(value) ? 0 : reloc_value(value);
			if (!parameter) base = number;
			else limit = number;
			current = token.t_type;
		}
		if (current != TOKEN_EOS) {
			aasm_log(_LOG_ERROR, "`incbin': more than three parameters");
			result.valid = false;
			return true;
		}

		FILE* file = fopen(filename, "rb");
		if (!file) {
			aasm_log(_LOG_ERROR, "`incbin': unable to open file `%s'", filename);
			return true;
		}
		if (fseek(file, 0L, SEEK_END) < 0) {
			aasm_log(_LOG_ERROR, "`incbin': unable to seek on file `%s'", filename);
			fclose(file);
			return true;
		}
		int64_t length = ftell(file) - base;
		if (length < 0) length = 0;
		if (limit >= 0 && length > limit) length = limit;
		result.size = length * repeat;

		if (parse_pass == 1) {
			if (using_debug_info) codeend.SetDebugType(TY_LABEL);
			fclose(file);
			return true;
		}

		static char buffer[4096];
		aasmlist.output(Session.Location.offset, nullptr, OUT_RAWDATA, 0);
		aasmlist.uplevel(LIST_INCBIN);
		int64_t output_offset = Session.Location.offset;
		for (int64_t iteration = 0; iteration < repeat; iteration++) {
			fseek(file, base, SEEK_SET);
			int64_t remaining = length;
			while (remaining > 0) {
				size_t block = remaining > (int64_t)sizeof(buffer) ?
					sizeof(buffer) : (size_t)remaining;
				size_t read = fread(buffer, 1, block, file);
				if (!read) {
					aasm_log(_LOG_ERROR,
						"`incbin': unexpected EOF while reading file `%s'",
						filename);
					remaining = 0;
					break;
				}
				EmitStatementData(Session, output_offset, buffer,
					uni::CodeendOutputType::RawData, read);
				output_offset += read;
				remaining -= read;
			}
		}
		aasmlist.downlevel(LIST_INCBIN);
		if (repeat > 1) {
			aasmlist.output(output_offset, nullptr, OUT_RAWDATA, 0);
			aasmlist.uplevel(LIST_TIMES);
			aasmlist.downlevel(LIST_TIMES);
		}
		fclose(file);
		return true;
	}

	DataItem* first = nullptr;
	DataItem** tail = &first;
	bool floating = false;
	int operand_number = 0;
	stdscan_reset();
	stdscan_bufptr = operands;
	struct tokenval token;
	int current;

	while ((current = common_stdscan(nullptr, &token)) != TOKEN_EOS) {
		DataItem* item = (DataItem*)malc(sizeof(DataItem));
		MemSet(item, 0, sizeof(DataItem));
		item->Section = item->Wrt = NO_SEG;
		*tail = item;
		tail = &item->Next;
		operand_number++;
		int sign = 1;

		if (current == TOKEN_STR && IsCommaNext()) {
			item->Kind = DataItemKind::String;
			item->String = token.t_charptr;
			item->Length = token.t_inttwo;
			current = common_stdscan(nullptr, &token);
		}
		else if (current == TOKEN_STRFUNC) {
			bool parentheses = false;
			const char* function_name = token.t_charptr;
			enum strfunc function = (enum strfunc)token.t_integer;
			current = common_stdscan(nullptr, &token);
			if (current == '(') {
				parentheses = true;
				current = common_stdscan(nullptr, &token);
			}
			if (current != TOKEN_STR) {
				aasm_log(_LOG_ERROR,
					"%s must be followed by a string constant", function_name);
			}
			else {
				item->Kind = DataItemKind::OwnedString;
				item->Length = string_transform(token.t_charptr,
					token.t_inttwo, &item->String, function);
				if (item->Length == (size_t)-1) {
					aasm_log(_LOG_ERROR, "invalid string for transform");
					item->Kind = DataItemKind::None;
				}
			}
			current = common_stdscan(nullptr, &token);
			if (parentheses) {
				if (current != ')')
					aasm_log(_LOG_ERROR, "unterminated %s function", function_name);
				else current = common_stdscan(nullptr, &token);
			}
		}
		else {
			if (current == '-' || current == '+') {
				char* saved_cursor = stdscan_bufptr;
				int sign_token = current;
				sign = current == '-' ? -1 : 1;
				current = common_stdscan(nullptr, &token);
				if (current != TOKEN_FLOAT) {
					stdscan_bufptr = saved_cursor;
					token.t_type = (token_type)sign_token;
					current = sign_token;
				}
			}

			if (current == TOKEN_FLOAT) {
				item->Kind = DataItemKind::OwnedString;
				item->Length = width_bytes;
				item->String = (char*)malc(width_bytes);
				floating = true;
				if (width == AssemblyDataWidth::Bits256) {
					aasm_log(_LOG_ERROR,
						"floating-point constant encountered in 256-bit data");
					item->Length = 0;
				}
				if (!item->Length || !float_const(token.t_charptr, sign,
					(uint8_t*)item->String, item->Length))
					item->Kind = DataItemKind::None;
				current = common_stdscan(nullptr, &token);
			}
			else {
				expr* value = eval_evaluate(common_stdscan, nullptr, &token,
					nullptr, kind == CommonStatementKind::Incbin ? 2 :
					(parse_pass == 2 ? 2 : 0), nullptr);
				current = token.t_type;
				if (!value) {
					FreeDataItems(first);
					result.valid = false;
					return true;
				}
				if (is_unknown(value)) {
					item->Kind = DataItemKind::Number;
					item->Offset = 0;
				}
				else if (is_reloc(value)) {
					item->Kind = DataItemKind::Number;
					item->Offset = reloc_value(value);
					item->Section = reloc_seg(value);
					item->Wrt = reloc_wrt(value);
				}
				else {
					aasm_log(_LOG_ERROR,
						"operand %d: expression is not simple or relocatable",
						operand_number);
				}
			}
		}

		if (current == TOKEN_EOS) break;
		if (current != ',') {
			aasm_log(_LOG_ERROR, "comma expected after operand %d", operand_number);
			FreeDataItems(first);
			result.valid = false;
			return true;
		}
	}

	if (!operand_number) {
		log_pass1 = 1;
		aasm_log(_LOG_WARN, "no operand for data declaration");
	}

	int64_t single_size = 0;
	for (DataItem* item = first; item; item = item->Next)
		single_size += DataItemSize(*item, width_bytes);
	result.size = single_size * repeat;

	if (parse_pass == 1) {
		if (using_debug_info)
			codeend.SetDebugType(TYS_ELEMENTS(operand_number) |
				DataDebugType(width, floating));
		FreeDataItems(first);
		return true;
	}

	int64_t output_offset = Session.Location.offset;
	for (int64_t iteration = 0; iteration < repeat; iteration++) {
		for (DataItem* item = first; item; item = item->Next) {
			if (item->Kind == DataItemKind::Number) {
				if (width_bytes == 1 &&
					(item->Section != NO_SEG || item->Wrt != NO_SEG)) {
					aasm_log(_LOG_ERROR, "one-byte relocation attempted");
				}
				else if (width_bytes == 1) {
					uint8_t value = item->Offset;
					EmitStatementData(Session, output_offset, &value,
						uni::CodeendOutputType::RawData, 1);
				}
				else if (width_bytes > 8) {
					aasm_log(_LOG_ERROR,
						"integer supplied to data wider than 64 bits");
				}
				else if (item->Section != NO_SEG || item->Wrt != NO_SEG) {
					EmitStatementFixup(Session, output_offset, item->Offset,
						width_bytes, item->Section, item->Wrt);
				}
				else {
					uint8_t converted[8];
					uint8_t* cursor = converted;
					WRITEADDR(cursor, item->Offset, width_bytes);
					EmitStatementData(Session, output_offset, converted,
						uni::CodeendOutputType::RawData, width_bytes);
				}
				output_offset += width_bytes;
			}
			else if (item->Kind == DataItemKind::String ||
				item->Kind == DataItemKind::OwnedString) {
				EmitStatementData(Session, output_offset, item->String,
					uni::CodeendOutputType::RawData, item->Length);
				size_t padding = item->Length % width_bytes;
				if (padding) {
					padding = width_bytes - padding;
					EmitStatementData(Session, output_offset + item->Length,
						zero_buffer, uni::CodeendOutputType::RawData, padding);
				}
				output_offset += item->Length + padding;
			}
		}
		if (iteration == 0 && repeat > 1) {
			aasmlist.output(output_offset, nullptr, OUT_RAWDATA, 0);
			aasmlist.uplevel(LIST_TIMES);
		}
	}
	if (repeat > 1) aasmlist.downlevel(LIST_TIMES);
	FreeDataItems(first);
	return true;
}

uni::lango::ArchendLineResult AssemblyStatementParser::ProcessLine(
	char* source, int parse_pass, int output_pass)
{
	uni::lango::ArchendLineResult result;
	bool explicit_label;
	char* label = ParseLabel(source, explicit_label);
	source = SkipSpace(source);

	if (!*source || *source == ';') {
		DefineLabel(label);
		if (label && !explicit_label) {
			log_pass1 = 1;
			log_warnt = ERR_WARN_OL;
			aasm_log(_LOG_WARN,
				"label alone on a line without a colon might be in error");
		}
		return result;
	}

	char* expression;
	if (IsEquStatement(source, expression))
		return ProcessEqu(label, expression, parse_pass);

	DefineLabel(label);
	int64_t repeat = 1;
	if (!ParseRepeat(source, repeat)) {
		result.valid = false;
		return result;
	}
	if (ProcessCommonStatement(
		source, repeat, parse_pass, result)) return result;
	return Session.ActiveArchend->ProcessInstruction(
		Session, source, repeat, parse_pass, output_pass);
}
