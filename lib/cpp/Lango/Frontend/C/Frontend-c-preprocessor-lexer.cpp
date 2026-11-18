// ASCII C++11 TAB4 LF
// Docutitle: Lango C Frontend Preprocessor Lexer
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

	struct SourceCharacter {
		char value = 0;
		stduint row = 0;
		stduint column = 0;
	};

	bool IsIdentifierStart(char character) {
		return character == '_' || ascii_isalpha((unsigned char)character);
	}

	bool IsIdentifierContinue(char character) {
		return IsIdentifierStart(character) ||
			ascii_isdigit((unsigned char)character);
	}

	bool IsHorizontalSpace(char character) {
		return character == ' ' || character == '\t' ||
			character == '\v' || character == '\f';
	}

	bool Match(const uni::Vector<SourceCharacter>& characters,
		stduint index, const char* spelling) {
		for (stduint offset = 0; spelling[offset]; offset++) {
			if (index + offset >= characters.Count() ||
				characters.begin()[index + offset].value != spelling[offset])
				return false;
		}
		return true;
	}

	uni::String CopySpelling(const uni::Vector<SourceCharacter>& characters,
		stduint begin, stduint end) {
		uni::String spelling;
		for (stduint index = begin; index < end; index++)
			spelling << characters.begin()[index].value;
		return spelling;
	}

	bool AppendNewline(uni::Vector<Token>& tokens,
		const SourceCharacter& character) {
		Token token;
		token.kind = TokenKind::Newline;
		token.spelling = "\n";
		token.row = character.row;
		token.column = character.column;
		return tokens.Append(token);
	}

	bool BuildLogicalCharacters(const uni::String& source,
		uni::Vector<SourceCharacter>& characters) {
		stduint row = 1;
		stduint column = 1;
		for (stduint index = 0; index < source.getByteCount();) {
			char character = source[index];
			if (character == '\\' && index + 1 < source.getByteCount() &&
				(source[index + 1] == '\n' || source[index + 1] == '\r')) {
				char newline = source[index + 1];
				index += 2;
				if (index < source.getByteCount() &&
					(source[index] == '\n' || source[index] == '\r') &&
					source[index] != newline) index++;
				row++;
				column = 1;
				continue;
			}
			SourceCharacter logical;
			logical.row = row;
			logical.column = column;
			if (character == '\n' || character == '\r') {
				logical.value = '\n';
				characters.Append(logical);
				char newline = character;
				index++;
				if (index < source.getByteCount() &&
					(source[index] == '\n' || source[index] == '\r') &&
					source[index] != newline) index++;
				row++;
				column = 1;
				continue;
			}
			logical.value = character;
			if (!characters.Append(logical)) return false;
			index++;
			column++;
		}
		return true;
	}

	bool LexCharacters(const uni::Vector<SourceCharacter>& characters,
		uni::Vector<Token>& tokens, const FrontendCDiagnostics& diagnostics) {
		static const char* const punctuators[] = {
			"%:%:", ">>=", "<<=", "...", "##", "->", "++", "--",
			"<<", ">>", "<=", ">=", "==", "!=", "&&", "||",
			"*=", "/=", "%=", "+=", "-=", "&=", "^=", "|=",
			"<:", ":>", "<%", "%>", "%:"
		};
		bool line_start = true;
		bool leading_space = false;
		for (stduint index = 0; index < characters.Count();) {
			const SourceCharacter& current = characters.begin()[index];
			char character = current.value;
			if (character == '\n') {
				if (!AppendNewline(tokens, current)) return false;
				line_start = true;
				leading_space = false;
				index++;
				continue;
			}
			if (IsHorizontalSpace(character)) {
				leading_space = true;
				index++;
				continue;
			}
			if (character == '/' && index + 1 < characters.Count() &&
				characters.begin()[index + 1].value == '/') {
				leading_space = true;
				index += 2;
				while (index < characters.Count() &&
					characters.begin()[index].value != '\n') index++;
				continue;
			}
			if (character == '/' && index + 1 < characters.Count() &&
				characters.begin()[index + 1].value == '*') {
				SourceCharacter opening = current;
				leading_space = true;
				index += 2;
				bool closed = false;
				while (index < characters.Count()) {
					if (characters.begin()[index].value == '*' &&
						index + 1 < characters.Count() &&
						characters.begin()[index + 1].value == '/') {
						index += 2;
						closed = true;
						break;
					}
					if (characters.begin()[index].value == '\n') {
						if (!AppendNewline(tokens, characters.begin()[index])) return false;
						line_start = true;
						leading_space = false;
					}
					index++;
				}
				if (!closed) {
					diagnostics.Report(DiagnosticStage::Preprocessor,
						"unterminated block comment", opening.row, opening.column);
					return false;
				}
				continue;
			}

			Token token;
			token.row = current.row;
			token.column = current.column;
			token.leading_space = leading_space;
			token.line_start = line_start;
			stduint begin = index;
			if (IsIdentifierStart(character)) {
				token.kind = TokenKind::Identifier;
				while (index < characters.Count() &&
					IsIdentifierContinue(characters.begin()[index].value)) index++;
			}
			else if (ascii_isdigit((unsigned char)character) ||
				(character == '.' && index + 1 < characters.Count() &&
				ascii_isdigit((unsigned char)characters.begin()[index + 1].value))) {
				token.kind = TokenKind::PPNumber;
				char previous = 0;
				while (index < characters.Count()) {
					char number_character = characters.begin()[index].value;
					bool exponent_sign = (number_character == '+' || number_character == '-') &&
						(previous == 'e' || previous == 'E' ||
						previous == 'p' || previous == 'P');
					if (!(IsIdentifierContinue(number_character) ||
						number_character == '.' || exponent_sign)) break;
					previous = number_character;
					index++;
				}
			}
			else if (character == '"' || character == '\'') {
				char quote = character;
				token.kind = quote == '"' ? TokenKind::StringLiteral :
					TokenKind::CharacterConstant;
				index++;
				bool closed = false;
				while (index < characters.Count()) {
					char literal_character = characters.begin()[index].value;
					if (literal_character == '\n') break;
					index++;
					if (literal_character == '\\' && index < characters.Count()) {
						index++;
						continue;
					}
					if (literal_character == quote) {
						closed = true;
						break;
					}
				}
				if (!closed) {
					diagnostics.Report(DiagnosticStage::Preprocessor,
						quote == '"' ? "unterminated string literal" :
						"unterminated character constant", token.row, token.column);
					return false;
				}
			}
			else {
				token.kind = TokenKind::Punctuator;
				stduint length = 1;
				for (const char* punctuator : punctuators) {
					if (Match(characters, index, punctuator)) {
						length = StrLength(punctuator);
						break;
					}
				}
				index += length;
			}
			token.spelling = CopySpelling(characters, begin, index);
			if (!tokens.Append(token)) return false;
			line_start = false;
			leading_space = false;
		}
		return true;
	}

}

bool Lex(uni::IstreamTrait& input, uni::Vector<Token>& tokens,
	const FrontendCDiagnostics& diagnostics) {
	uni::String source;
	for (;;) {
		int character = input.inn();
		if (character < 0) break;
		source << (char)character;
	}
	uni::Vector<SourceCharacter> characters;
	if (!BuildLogicalCharacters(source, characters)) {
		diagnostics.Report(DiagnosticStage::Preprocessor,
			"not enough memory while reading source", 1, 1);
		return false;
	}
	return LexCharacters(characters, tokens, diagnostics);
}

bool LexGenerated(const char* text, uni::Vector<Token>& tokens,
	const FrontendCDiagnostics& diagnostics, stduint row, stduint column) {
	uni::Vector<SourceCharacter> characters;
	for (stduint index = 0; text && text[index]; index++) {
		SourceCharacter character;
		character.value = text[index];
		character.row = row;
		character.column = column + index;
		if (!characters.Append(character)) return false;
	}
	return LexCharacters(characters, tokens, diagnostics);
}

}
