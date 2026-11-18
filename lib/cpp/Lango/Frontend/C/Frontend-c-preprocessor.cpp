// ASCII C++11 TAB4 LF
// Docutitle: Lango C Frontend Preprocessor
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

#include "../../../../../inc/cpp/Lango/Frontend/C/Frontend-c-preprocessor.hpp"

namespace uni::lango::frontend_c_preprocessor {

namespace {

	bool IsToken(const Token& token, const char* spelling) {
		return token.spelling.getByteCount() &&
			!StrCompare(token.spelling.reference(), spelling);
	}

	bool IsDirectiveMarker(const Token& token) {
		return token.line_start && token.kind == TokenKind::Punctuator &&
			(IsToken(token, "#") || IsToken(token, "%:"));
	}

	stduint NextNonNewline(const uni::Vector<Token>& tokens,
		stduint index, stduint end) {
		while (index < end && tokens.begin()[index].kind == TokenKind::Newline)
			index++;
		return index;
	}

	Token LocationToken(const Token& source, const Token& invocation) {
		Token token = source;
		token.row = invocation.row;
		token.column = invocation.column;
		token.line_start = false;
		return token;
	}

	Token CommaToken(const Token& invocation) {
		Token token;
		token.kind = TokenKind::Punctuator;
		token.spelling = ",";
		token.row = invocation.row;
		token.column = invocation.column;
		return token;
	}

	bool AppendRange(uni::Vector<Token>& output,
		const uni::Vector<Token>& input, stduint begin, stduint end) {
		for (stduint index = begin; index < end; index++)
			if (!output.Append(input.begin()[index])) return false;
		return true;
	}

	const char* MaterializedPunctuator(const Token& token) {
		if (IsToken(token, "<:")) return "[";
		if (IsToken(token, ":>")) return "]";
		if (IsToken(token, "<%")) return "{";
		if (IsToken(token, "%>")) return "}";
		if (IsToken(token, "%:")) return "#";
		if (IsToken(token, "%:%:")) return "##";
		return token.spelling.reference();
	}

}

Macro* Preprocessor::FindMacro(const char* name) {
	for (Macro& macro : macros)
		if (macro.active && !StrCompare(macro.name.reference(), name)) return &macro;
	return nullptr;
}

const Macro* Preprocessor::FindMacro(const char* name) const {
	for (const Macro& macro : macros)
		if (macro.active && !StrCompare(macro.name.reference(), name)) return &macro;
	return nullptr;
}

bool Preprocessor::IsMacroDisabled(const char* name,
	const uni::Vector<uni::String>& disabled) const {
	for (const uni::String& disabled_name : disabled)
		if (!StrCompare(disabled_name.reference(), name)) return true;
	return false;
}

bool Preprocessor::IsActive() const {
	return !conditionals.Count() ||
		conditionals.begin()[conditionals.Count() - 1].branch_active;
}

bool Preprocessor::Report(const char* message, const Token& token) const {
	diagnostics.Report(DiagnosticStage::Preprocessor,
		message, token.row, token.column);
	return false;
}

bool Preprocessor::AppendToken(uni::Vector<Token>& output, const Token& token) {
	if (token.kind != TokenKind::Newline && token.kind != TokenKind::Placemark &&
		++emitted_tokens > MaximumOutputTokens)
		return Report("preprocessor output exceeds token limit", token);
	return output.Append(token);
}

bool Preprocessor::MacroDefinitionsEqual(const Macro& left,
	const Macro& right) const {
	if (left.function_like != right.function_like ||
		left.variadic != right.variadic ||
		left.parameters.Count() != right.parameters.Count() ||
		left.replacement.Count() != right.replacement.Count()) return false;
	for (stduint index = 0; index < left.parameters.Count(); index++)
		if (StrCompare(left.parameters.begin()[index].reference(),
			right.parameters.begin()[index].reference())) return false;
	for (stduint index = 0; index < left.replacement.Count(); index++) {
		const Token& left_token = left.replacement.begin()[index];
		const Token& right_token = right.replacement.begin()[index];
		if (left_token.kind != right_token.kind ||
			StrCompare(left_token.spelling.reference(), right_token.spelling.reference()) ||
			(index && left_token.leading_space != right_token.leading_space)) return false;
	}
	return true;
}

bool Preprocessor::HandleDefine(const uni::Vector<Token>& input,
	stduint begin, stduint end, const Token& directive) {
	if (begin >= end || input.begin()[begin].kind != TokenKind::Identifier)
		return Report("#define requires a macro name", directive);
	Macro macro;
	const Token& name = input.begin()[begin++];
	macro.name = name.spelling;
	macro.row = name.row;
	macro.column = name.column;
	if (begin < end && IsToken(input.begin()[begin], "(") &&
		!input.begin()[begin].leading_space) {
		macro.function_like = true;
		begin++;
		if (begin < end && IsToken(input.begin()[begin], ")")) begin++;
		else {
			for (;;) {
				if (begin >= end)
					return Report("unterminated function-like macro parameter list", name);
				const Token& parameter = input.begin()[begin];
				if (IsToken(parameter, "...")) {
					macro.variadic = true;
					begin++;
					if (begin >= end || !IsToken(input.begin()[begin], ")"))
						return Report("... must terminate a macro parameter list", parameter);
					begin++;
					break;
				}
				if (parameter.kind != TokenKind::Identifier)
					return Report("macro parameter must be an identifier", parameter);
				for (const uni::String& existing : macro.parameters)
					if (!StrCompare(existing.reference(), parameter.spelling.reference()))
						return Report("duplicate macro parameter", parameter);
				macro.parameters.Append(parameter.spelling);
				begin++;
				if (begin < end && IsToken(input.begin()[begin], ")")) {
					begin++;
					break;
				}
				if (begin >= end || !IsToken(input.begin()[begin], ","))
					return Report("expected ',' or ')' in macro parameter list", parameter);
				begin++;
				if (begin < end && IsToken(input.begin()[begin], "...")) {
					macro.variadic = true;
					begin++;
					if (begin >= end || !IsToken(input.begin()[begin], ")"))
						return Report("... must terminate a macro parameter list",
							input.begin()[begin - 1]);
					begin++;
					break;
				}
			}
		}
	}
	for (; begin < end; begin++) {
		Token replacement = input.begin()[begin];
		replacement.line_start = false;
		if (!macro.replacement.Append(replacement))
			return Report("not enough memory for macro definition", name);
	}
	if (macro.replacement.Count()) {
		if (IsToken(macro.replacement.begin()[0], "##") ||
			IsToken(macro.replacement.begin()[0], "%:%:") ||
			IsToken(macro.replacement.begin()[macro.replacement.Count() - 1], "##") ||
			IsToken(macro.replacement.begin()[macro.replacement.Count() - 1], "%:%:"))
			return Report("## cannot appear at either end of a macro replacement list",
				macro.replacement.begin()[0]);
	}
	if (macro.function_like) {
		for (stduint index = 0; index < macro.replacement.Count(); index++) {
			const Token& token = macro.replacement.begin()[index];
			if (!IsToken(token, "#") && !IsToken(token, "%:")) continue;
			if (index + 1 >= macro.replacement.Count() ||
				ParameterIndex(macro, macro.replacement.begin()[index + 1]) < 0)
				return Report("# in a macro replacement list must precede a parameter", token);
		}
	}
	Macro* existing = FindMacro(macro.name.reference());
	if (existing) {
		if (!MacroDefinitionsEqual(*existing, macro))
			return Report("incompatible macro redefinition", name);
		return true;
	}
	if (!macros.Append(macro)) return Report("not enough memory for macro definition", name);
	return true;
}

bool Preprocessor::HandleUndef(const uni::Vector<Token>& input,
	stduint begin, stduint end, const Token& directive) {
	if (begin >= end || input.begin()[begin].kind != TokenKind::Identifier)
		return Report("#undef requires a macro name", directive);
	const Token& name = input.begin()[begin++];
	if (begin != end) return Report("unexpected tokens after #undef", input.begin()[begin]);
	for (stduint index = 0; index < macros.Count(); index++) {
		if (StrCompare(macros.begin()[index].name.reference(), name.spelling.reference()))
			continue;
		macros.begin()[index].active = false;
		break;
	}
	return true;
}

bool Preprocessor::HandleIf(const uni::Vector<Token>& input,
	stduint begin, stduint end, const Token& directive) {
	ConditionalFrame frame;
	frame.parent_active = IsActive();
	frame.row = directive.row;
	frame.column = directive.column;
	if (frame.parent_active) {
		Value value;
		if (!EvaluateIfExpression(input, begin, end, value)) return false;
		frame.branch_active = value.Truth();
		frame.branch_taken = frame.branch_active;
	}
	return conditionals.Append(frame);
}

bool Preprocessor::HandleIfdef(const uni::Vector<Token>& input,
	stduint begin, stduint end, const Token& directive, bool negative) {
	if (begin >= end || input.begin()[begin].kind != TokenKind::Identifier)
		return Report(negative ? "#ifndef requires an identifier" :
			"#ifdef requires an identifier", directive);
	const Token& name = input.begin()[begin++];
	if (begin != end) return Report("unexpected tokens after conditional identifier",
		input.begin()[begin]);
	ConditionalFrame frame;
	frame.parent_active = IsActive();
	frame.row = directive.row;
	frame.column = directive.column;
	if (frame.parent_active) {
		bool defined = FindMacro(name.spelling.reference()) != nullptr;
		frame.branch_active = negative ? !defined : defined;
		frame.branch_taken = frame.branch_active;
	}
	return conditionals.Append(frame);
}

bool Preprocessor::HandleElif(const uni::Vector<Token>& input,
	stduint begin, stduint end, const Token& directive) {
	if (conditionals.Count() <= conditional_floor)
		return Report("#elif without matching #if", directive);
	ConditionalFrame& frame = conditionals.begin()[conditionals.Count() - 1];
	if (frame.seen_else) return Report("#elif after #else", directive);
	if (!frame.parent_active || frame.branch_taken) {
		frame.branch_active = false;
		return true;
	}
	Value value;
	if (!EvaluateIfExpression(input, begin, end, value)) return false;
	frame.branch_active = value.Truth();
	if (frame.branch_active) frame.branch_taken = true;
	return true;
}

bool Preprocessor::HandleElse(const uni::Vector<Token>& input,
	stduint begin, stduint end, const Token& directive) {
	if (begin != end) return Report("unexpected tokens after #else", input.begin()[begin]);
	if (conditionals.Count() <= conditional_floor)
		return Report("#else without matching #if", directive);
	ConditionalFrame& frame = conditionals.begin()[conditionals.Count() - 1];
	if (frame.seen_else) return Report("duplicate #else", directive);
	frame.seen_else = true;
	frame.branch_active = frame.parent_active && !frame.branch_taken;
	frame.branch_taken = true;
	return true;
}

bool Preprocessor::HandleEndif(const uni::Vector<Token>& input,
	stduint begin, stduint end, const Token& directive) {
	if (begin != end) return Report("unexpected tokens after #endif", input.begin()[begin]);
	if (conditionals.Count() <= conditional_floor)
		return Report("#endif without matching #if", directive);
	conditionals.Remove(conditionals.Count() - 1);
	return true;
}

bool Preprocessor::HandleDirective(const uni::Vector<Token>& input,
	stduint begin, stduint end, uni::Vector<Token>& output,
	const char* source_name, stduint include_depth) {
	const Token& marker = input.begin()[begin++];
	if (begin >= end) return true;
	const Token& directive = input.begin()[begin++];
	if (directive.kind != TokenKind::Identifier)
		return IsActive() ? Report("invalid preprocessor directive", directive) : true;
	const char* name = directive.spelling.reference();
	if (!StrCompare(name, "if")) return HandleIf(input, begin, end, directive);
	if (!StrCompare(name, "ifdef"))
		return HandleIfdef(input, begin, end, directive, false);
	if (!StrCompare(name, "ifndef"))
		return HandleIfdef(input, begin, end, directive, true);
	if (!StrCompare(name, "elif")) return HandleElif(input, begin, end, directive);
	if (!StrCompare(name, "else")) return HandleElse(input, begin, end, directive);
	if (!StrCompare(name, "endif")) return HandleEndif(input, begin, end, directive);
	if (!IsActive()) return true;
	if (!StrCompare(name, "define")) return HandleDefine(input, begin, end, directive);
	if (!StrCompare(name, "undef")) return HandleUndef(input, begin, end, directive);
	if (!StrCompare(name, "include")) return HandleInclude(input, begin, end,
		directive, output, source_name, include_depth);
	(void)marker;
	return Report("unsupported preprocessor directive", directive);
}

stdint Preprocessor::ParameterIndex(const Macro& macro, const Token& token) const {
	if (token.kind != TokenKind::Identifier) return -1;
	for (stduint index = 0; index < macro.parameters.Count(); index++)
		if (!StrCompare(macro.parameters.begin()[index].reference(),
			token.spelling.reference())) return (stdint)index;
	if (macro.variadic && !StrCompare(token.spelling.reference(), "__VA_ARGS__"))
		return (stdint)macro.parameters.Count();
	return -1;
}

bool Preprocessor::ParseMacroArguments(const uni::Vector<Token>& input,
	stduint open, stduint end, stduint& close,
	uni::Vector<uni::Vector<Token>>& arguments) const {
	uni::Vector<Token> argument;
	stduint depth = 1;
	bool has_token = false;
	for (stduint index = open + 1; index < end; index++) {
		const Token& token = input.begin()[index];
		if (IsToken(token, "(")) {
			depth++;
			argument.Append(token);
			has_token = true;
			continue;
		}
		if (IsToken(token, ")")) {
			if (--depth == 0) {
				if (has_token || arguments.Count()) arguments.Append(argument);
				close = index;
				return true;
			}
			argument.Append(token);
			has_token = true;
			continue;
		}
		if (depth == 1 && IsToken(token, ",")) {
			arguments.Append(argument);
			argument.Clear();
			has_token = false;
			continue;
		}
		argument.Append(token);
		if (token.kind != TokenKind::Newline) has_token = true;
	}
	return false;
}

bool Preprocessor::StringifyArgument(const uni::Vector<Token>& argument,
	const Token& invocation, Token& result) const {
	result.kind = TokenKind::StringLiteral;
	result.row = invocation.row;
	result.column = invocation.column;
	result.spelling << '"';
	bool first = true;
	bool pending_space = false;
	for (const Token& token : argument) {
		if (token.kind == TokenKind::Newline) {
			pending_space = true;
			continue;
		}
		if (!first && (pending_space || token.leading_space)) result.spelling << ' ';
		const char* spelling = token.spelling.reference();
		for (; spelling && *spelling; spelling++) {
			if (*spelling == '\\' || *spelling == '"') result.spelling << '\\';
			result.spelling << *spelling;
		}
		first = false;
		pending_space = false;
	}
	result.spelling << '"';
	return true;
}

bool Preprocessor::PasteTokens(uni::Vector<Token>& tokens,
	const Token& invocation) {
	uni::Vector<Token> pasted;
	for (stduint index = 0; index < tokens.Count();) {
		const Token& token = tokens.begin()[index];
		if (!IsToken(token, "##") && !IsToken(token, "%:%:")) {
			pasted.Append(token);
			index++;
			continue;
		}
		if (!pasted.Count() || index + 1 >= tokens.Count() ||
			IsToken(tokens.begin()[index + 1], "##") ||
			IsToken(tokens.begin()[index + 1], "%:%:"))
			return Report("invalid ## placement in macro replacement", token);
		Token left = pasted.begin()[pasted.Count() - 1];
		Token right = tokens.begin()[index + 1];
		uni::Vector<Token> prefix;
		if (!AppendRange(prefix, pasted, 0, pasted.Count() - 1)) return false;
		pasted = prefix;
		Token combined;
		if (left.kind == TokenKind::Placemark && right.kind == TokenKind::Placemark) {
			combined = left;
		}
		else if (left.kind == TokenKind::Placemark) combined = right;
		else if (right.kind == TokenKind::Placemark) combined = left;
		else {
			uni::String spelling(left.spelling);
			spelling += right.spelling;
			uni::Vector<Token> generated;
			if (!LexGenerated(spelling.reference(), generated, diagnostics,
				invocation.row, invocation.column)) return false;
			stduint count = 0;
			for (const Token& generated_token : generated)
				if (generated_token.kind != TokenKind::Newline) {
					combined = generated_token;
					count++;
				}
			if (count != 1)
				return Report("## does not produce one preprocessing token", invocation);
		}
		combined.leading_space = left.leading_space;
		combined.line_start = left.line_start;
		pasted.Append(combined);
		index += 2;
	}
	tokens.Clear();
	for (const Token& token : pasted)
		if (token.kind != TokenKind::Placemark) tokens.Append(token);
	return true;
}

bool Preprocessor::SubstituteMacro(const Macro& macro,
	const uni::Vector<uni::Vector<Token>>& raw_arguments,
	const uni::Vector<uni::Vector<Token>>& expanded_arguments,
	const Token& invocation, uni::Vector<Token>& replacement) {
	for (stduint index = 0; index < macro.replacement.Count(); index++) {
		const Token& token = macro.replacement.begin()[index];
		if ((IsToken(token, "#") || IsToken(token, "%:")) &&
			index + 1 < macro.replacement.Count()) {
			stdint parameter = ParameterIndex(macro, macro.replacement.begin()[index + 1]);
			if (parameter >= 0) {
				Token stringified;
				if (!StringifyArgument(raw_arguments.begin()[parameter],
					invocation, stringified)) return false;
				replacement.Append(stringified);
				index++;
				continue;
			}
		}
		stdint parameter = ParameterIndex(macro, token);
		if (parameter < 0) {
			replacement.Append(LocationToken(token, invocation));
			continue;
		}
		bool paste_left = index &&
			(IsToken(macro.replacement.begin()[index - 1], "##") ||
			IsToken(macro.replacement.begin()[index - 1], "%:%:"));
		bool paste_right = index + 1 < macro.replacement.Count() &&
			(IsToken(macro.replacement.begin()[index + 1], "##") ||
			IsToken(macro.replacement.begin()[index + 1], "%:%:"));
		const uni::Vector<Token>& argument = paste_left || paste_right ?
			raw_arguments.begin()[parameter] : expanded_arguments.begin()[parameter];
		bool appended = false;
		for (const Token& argument_token : argument) {
			if (argument_token.kind == TokenKind::Newline) continue;
			Token substituted = argument_token;
			if (!appended) substituted.leading_space = token.leading_space;
			replacement.Append(substituted);
			appended = true;
		}
		if (!appended && (paste_left || paste_right)) {
			Token placemark = LocationToken(token, invocation);
			placemark.kind = TokenKind::Placemark;
			placemark.spelling.Clear();
			replacement.Append(placemark);
		}
	}
	if (!PasteTokens(replacement, invocation)) return false;
	if (replacement.Count()) {
		replacement.begin()[0].leading_space = invocation.leading_space;
		replacement.begin()[0].line_start = invocation.line_start;
	}
	return true;
}

bool Preprocessor::ExpandMacro(uni::Vector<Token>& pending, stduint index,
	const Macro& macro, uni::Vector<uni::String>& disabled, stduint depth) {
	const Token invocation = pending.begin()[index];
	uni::Vector<Token> replacement;
	stduint consumed_end = index + 1;
	if (!macro.function_like) {
		for (const Token& token : macro.replacement)
			replacement.Append(LocationToken(token, invocation));
	}
	else {
		stduint open = NextNonNewline(pending, index + 1, pending.Count());
		if (open >= pending.Count() || !IsToken(pending.begin()[open], "("))
			return false;
		stduint close = 0;
		uni::Vector<uni::Vector<Token>> parsed_arguments;
		if (!ParseMacroArguments(pending, open, pending.Count(), close, parsed_arguments))
			return Report("unterminated function-like macro invocation", invocation);
		stduint fixed_count = macro.parameters.Count();
		if (!parsed_arguments.Count() && fixed_count)
			parsed_arguments.Append(uni::Vector<Token>());
		uni::Vector<uni::Vector<Token>> raw_arguments;
		if (!macro.variadic) {
			if (parsed_arguments.Count() != fixed_count)
				return Report("wrong number of macro arguments", invocation);
			raw_arguments = parsed_arguments;
		}
		else {
			if (parsed_arguments.Count() < fixed_count ||
				(fixed_count && parsed_arguments.Count() == fixed_count))
				return Report("not enough macro arguments", invocation);
			for (stduint argument = 0; argument < fixed_count; argument++)
				raw_arguments.Append(parsed_arguments.begin()[argument]);
			uni::Vector<Token> variadic;
			for (stduint argument = fixed_count;
				argument < parsed_arguments.Count(); argument++) {
				if (argument != fixed_count) variadic.Append(CommaToken(invocation));
				AppendRange(variadic, parsed_arguments.begin()[argument], 0,
					parsed_arguments.begin()[argument].Count());
			}
			raw_arguments.Append(variadic);
		}
		uni::Vector<uni::Vector<Token>> expanded_arguments;
		for (const uni::Vector<Token>& raw : raw_arguments) {
			uni::Vector<Token> expanded;
			if (!ExpandSequence(raw, 0, raw.Count(), expanded, disabled, depth + 1))
				return false;
			expanded_arguments.Append(expanded);
		}
		if (!SubstituteMacro(macro, raw_arguments, expanded_arguments,
			invocation, replacement)) return false;
		consumed_end = close + 1;
	}
	if (replacement.Count()) {
		replacement.begin()[0].leading_space = invocation.leading_space;
		replacement.begin()[0].line_start = invocation.line_start;
	}
	for (Token& token : replacement) {
		for (const uni::String& unavailable : invocation.unavailable_macros) {
			bool exists = false;
			for (const uni::String& current : token.unavailable_macros)
				if (!StrCompare(current.reference(), unavailable.reference())) {
					exists = true;
					break;
				}
			if (!exists) token.unavailable_macros.Append(unavailable);
		}
		bool macro_exists = false;
		for (const uni::String& unavailable : token.unavailable_macros)
			if (!StrCompare(unavailable.reference(), macro.name.reference())) {
				macro_exists = true;
				break;
			}
		if (!macro_exists) token.unavailable_macros.Append(macro.name);
	}
	uni::Vector<Token> rescanned;
	AppendRange(rescanned, pending, 0, index);
	AppendRange(rescanned, replacement, 0, replacement.Count());
	AppendRange(rescanned, pending, consumed_end, pending.Count());
	pending = rescanned;
	return true;
}

bool Preprocessor::ExpandSequence(const uni::Vector<Token>& input,
	stduint begin, stduint end, uni::Vector<Token>& output,
	uni::Vector<uni::String>& disabled, stduint depth) {
	if (depth > MaximumExpansionDepth) {
		Token location;
		if (begin < end) location = input.begin()[begin];
		return Report("macro expansion exceeds recursion limit", location);
	}
	uni::Vector<Token> pending;
	if (!AppendRange(pending, input, begin, end)) return false;
	stduint expansion_count = 0;
	for (stduint index = 0; index < pending.Count();) {
		const Token& token = pending.begin()[index];
		const Macro* macro = token.kind == TokenKind::Identifier ?
			FindMacro(token.spelling.reference()) : nullptr;
		bool unavailable = !macro || IsMacroDisabled(
			token.spelling.reference(), disabled);
		if (macro && !unavailable)
			for (const uni::String& name : token.unavailable_macros)
				if (!StrCompare(name.reference(), macro->name.reference())) {
					unavailable = true;
					break;
				}
		bool invoked = macro && !unavailable;
		if (invoked && macro->function_like) {
			stduint open = NextNonNewline(pending, index + 1, pending.Count());
			invoked = open < pending.Count() && IsToken(pending.begin()[open], "(");
		}
		if (invoked) {
			if (++expansion_count > MaximumOutputTokens)
				return Report("macro expansion exceeds token limit", token);
			if (!ExpandMacro(pending, index, *macro, disabled, depth)) return false;
			continue;
		}
		if (!AppendToken(output, token)) return false;
		index++;
	}
	return true;
}

bool Preprocessor::Materialize(const uni::Vector<Token>& input,
	uni::Dchain& output) const {
	output.func_free = DnodeHeapFreeSimple;
	output.setExtnField(sizeof(TnodeField));
	for (const Token& token : input) {
		if (token.kind == TokenKind::Newline || token.kind == TokenKind::Placemark)
			continue;
		const char* spelling = token.spelling.reference();
		stduint length = token.spelling.getByteCount();
		stduint type = tok_symbol;
		if (token.kind == TokenKind::Identifier) type = tok_identy;
		else if (token.kind == TokenKind::PPNumber) type = tok_number;
		else if (token.kind == TokenKind::StringLiteral) {
			type = tok_string;
			if (length >= 2) {
				spelling++;
				length -= 2;
			}
		}
		else if (token.kind == TokenKind::CharacterConstant) {
			type = ToToken(FrontendCToken::CharacterLiteral);
			if (length >= 2) {
				spelling++;
				length -= 2;
			}
		}
		else if (token.kind == TokenKind::Punctuator) {
			spelling = MaterializedPunctuator(token);
			length = StrLength(spelling);
		}
		Dnode* node = output.Append(StrHeapN(spelling, length), false);
		if (!node) return false;
		node->type = type;
		node->GetTnodeField()->row = token.row;
		node->GetTnodeField()->col = token.column;
	}
	return true;
}

bool Preprocessor::ProcessSource(uni::IstreamTrait& input,
	uni::Vector<Token>& selected, const char* source_name,
	stduint include_depth) {
	const char* previous_source = diagnostics.source_name;
	diagnostics.source_name = source_name ? source_name : "<input>";
	uni::Vector<Token> source;
	if (!Lex(input, source, diagnostics)) {
		diagnostics.source_name = previous_source;
		return false;
	}
	uni::Vector<uni::String> disabled;
	stduint previous_floor = conditional_floor;
	stduint source_floor = conditionals.Count();
	conditional_floor = source_floor;
	for (stduint index = 0; index < source.Count();) {
		if (IsDirectiveMarker(source.begin()[index])) {
			stduint end = index + 1;
			while (end < source.Count() &&
				source.begin()[end].kind != TokenKind::Newline) end++;
			if (!HandleDirective(source, index, end, selected,
				source_name, include_depth)) {
				conditional_floor = previous_floor;
				diagnostics.source_name = previous_source;
				return false;
			}
			index = end < source.Count() ? end + 1 : end;
			continue;
		}
		stduint end = index + 1;
		while (end < source.Count() && !IsDirectiveMarker(source.begin()[end])) end++;
		if (IsActive() && !ExpandSequence(source, index, end,
			selected, disabled, 0)) {
			conditional_floor = previous_floor;
			diagnostics.source_name = previous_source;
			return false;
		}
		index = end;
	}
	if (conditionals.Count() != source_floor) {
		const ConditionalFrame& frame =
			conditionals.begin()[conditionals.Count() - 1];
		diagnostics.Report(DiagnosticStage::Preprocessor,
			"unterminated conditional directive", frame.row, frame.column);
		while (conditionals.Count() > source_floor)
			conditionals.Remove(conditionals.Count() - 1);
		conditional_floor = previous_floor;
		diagnostics.source_name = previous_source;
		return false;
	}
	conditional_floor = previous_floor;
	diagnostics.source_name = previous_source;
	return true;
}

bool Preprocessor::Process(uni::IstreamTrait& input, uni::Dchain& output) {
	uni::Vector<Token> selected;
	if (!ProcessSource(input, selected, diagnostics.source_name, 0)) return false;
	return Materialize(selected, output);
}

}
