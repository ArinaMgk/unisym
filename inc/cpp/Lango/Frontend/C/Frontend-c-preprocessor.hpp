// ASCII C++11 TAB4 LF
// Docutitle: Lango C Frontend Preprocessor Internals
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

#ifndef UNISYM_LANGO_FRONTEND_C_PREPROCESSOR_HPP
#define UNISYM_LANGO_FRONTEND_C_PREPROCESSOR_HPP

#include "Frontend-c-internal.hpp"

namespace uni::lango::frontend_c_preprocessor {

enum class TokenKind {
	Identifier,        // Identifier before C keyword classification.
	PPNumber,          // C preprocessing number.
	StringLiteral,     // Complete spelling including double quotes.
	CharacterConstant,// Complete spelling including single quotes.
	Punctuator,        // One C punctuator.
	Newline,           // Non-spliced logical line boundary.
	Placemark          // Empty macro argument adjacent to ##.
};

struct Token {
	uni::String spelling;
	TokenKind kind = TokenKind::Identifier;
	stduint row = 0;
	stduint column = 0;
	bool leading_space = false;
	bool line_start = false;
	uni::Vector<uni::String> unavailable_macros; // Expansion exclusions inherited from macro replacement.
};

struct Macro {
	uni::String name;
	bool active = true;
	bool function_like = false;
	bool variadic = false;
	uni::Vector<uni::String> parameters;
	uni::Vector<Token> replacement;
	stduint row = 0;
	stduint column = 0;
};

struct ConditionalFrame {
	bool parent_active = true;
	bool branch_active = false;
	bool branch_taken = false;
	bool seen_else = false;
	stduint row = 0;
	stduint column = 0;
};

struct Value {
	uint64 bits = 0;
	bool is_unsigned = false;

	bool Truth() const { return bits != 0; }
};

// Tokenize one complete source input through C line-splicing and comment phases.
bool Lex(uni::IstreamTrait& input, uni::Vector<Token>& tokens,
	const FrontendCDiagnostics& diagnostics);

// Tokenize one generated spelling, used to validate the result of ##.
bool LexGenerated(const char* text, uni::Vector<Token>& tokens,
	const FrontendCDiagnostics& diagnostics, stduint row, stduint column);

class Preprocessor {
private:
	static constexpr stduint MaximumExpansionDepth = 256;
	static constexpr stduint MaximumIncludeDepth = 64;
	static constexpr stduint MaximumOutputTokens = 1u << 20;

	FrontendCDiagnostics diagnostics;
	const FrontendCTargetInfo* target = nullptr;
	const uni::Vector<uni::String>* include_paths = nullptr;
	uni::Vector<Macro> macros;
	uni::Vector<ConditionalFrame> conditionals;
	stduint emitted_tokens = 0;
	stduint conditional_floor = 0;

	Macro* FindMacro(const char* name);
	const Macro* FindMacro(const char* name) const;
	bool IsMacroDisabled(const char* name,
		const uni::Vector<uni::String>& disabled) const;
	bool IsActive() const;
	bool Report(const char* message, const Token& token) const;
	bool AppendToken(uni::Vector<Token>& output, const Token& token);

	bool HandleDirective(const uni::Vector<Token>& input,
		stduint begin, stduint end, uni::Vector<Token>& output,
		const char* source_name, stduint include_depth);
	bool HandleInclude(const uni::Vector<Token>& input,
		stduint begin, stduint end, const Token& directive,
		uni::Vector<Token>& output, const char* source_name,
		stduint include_depth);
	bool HandleDefine(const uni::Vector<Token>& input,
		stduint begin, stduint end, const Token& directive);
	bool HandleUndef(const uni::Vector<Token>& input,
		stduint begin, stduint end, const Token& directive);
	bool HandleIf(const uni::Vector<Token>& input,
		stduint begin, stduint end, const Token& directive);
	bool HandleIfdef(const uni::Vector<Token>& input,
		stduint begin, stduint end, const Token& directive, bool negative);
	bool HandleElif(const uni::Vector<Token>& input,
		stduint begin, stduint end, const Token& directive);
	bool HandleElse(const uni::Vector<Token>& input,
		stduint begin, stduint end, const Token& directive);
	bool HandleEndif(const uni::Vector<Token>& input,
		stduint begin, stduint end, const Token& directive);

	bool MacroDefinitionsEqual(const Macro& left, const Macro& right) const;
	bool ExpandSequence(const uni::Vector<Token>& input,
		stduint begin, stduint end, uni::Vector<Token>& output,
		uni::Vector<uni::String>& disabled, stduint depth);
	bool ExpandMacro(uni::Vector<Token>& pending, stduint index,
		const Macro& macro, uni::Vector<uni::String>& disabled, stduint depth);
	bool ParseMacroArguments(const uni::Vector<Token>& input,
		stduint open, stduint end, stduint& close,
		uni::Vector<uni::Vector<Token>>& arguments) const;
	bool SubstituteMacro(const Macro& macro,
		const uni::Vector<uni::Vector<Token>>& raw_arguments,
		const uni::Vector<uni::Vector<Token>>& expanded_arguments,
		const Token& invocation, uni::Vector<Token>& replacement);
	bool PasteTokens(uni::Vector<Token>& tokens, const Token& invocation);
	bool StringifyArgument(const uni::Vector<Token>& argument,
		const Token& invocation, Token& result) const;
	stdint ParameterIndex(const Macro& macro, const Token& token) const;

	bool EvaluateIfExpression(const uni::Vector<Token>& input,
		stduint begin, stduint end, Value& value);

	bool Materialize(const uni::Vector<Token>& input, uni::Dchain& output) const;
	bool ProcessSource(uni::IstreamTrait& input, uni::Vector<Token>& output,
		const char* source_name, stduint include_depth);

public:
	Preprocessor(const FrontendCDiagnostics& diagnostic_output,
		const FrontendCTargetInfo& target_information,
		const uni::Vector<uni::String>& header_paths) :
		diagnostics(diagnostic_output), target(&target_information),
		include_paths(&header_paths) {}

	bool Process(uni::IstreamTrait& input, uni::Dchain& output);
};

}

#endif
