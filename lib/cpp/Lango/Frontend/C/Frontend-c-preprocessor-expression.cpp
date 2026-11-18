// ASCII C++11 TAB4 LF
// Docutitle: Lango C Frontend Preprocessor Expressions
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

	Value BooleanValue(bool value) {
		Value result;
		result.bits = value ? 1 : 0;
		return result;
	}

	Value CommonValue(Value value, bool make_unsigned) {
		value.is_unsigned = make_unsigned;
		return value;
	}

	class ExpressionParser {
	private:
		const uni::Vector<Token>* tokens = nullptr;
		const FrontendCDiagnostics* diagnostics = nullptr;
		const FrontendCTargetInfo* target = nullptr;
		stduint index = 0;
		bool valid = true;

		const Token* Peek() const {
			return index < tokens->Count() ? &tokens->begin()[index] : nullptr;
		}

		bool Match(const char* spelling) {
			const Token* token = Peek();
			if (!token || !IsToken(*token, spelling)) return false;
			index++;
			return true;
		}

		bool Error(const char* message, const Token* token = nullptr) {
			if (!token) token = Peek();
			stduint row = token ? token->row : 0;
			stduint column = token ? token->column : 0;
			diagnostics->Report(DiagnosticStage::Preprocessor,
				message, row, column);
			valid = false;
			return false;
		}

		Value ParsePrimary(bool evaluate) {
			const Token* token = Peek();
			if (!token) {
				Error("expected expression after #if");
				return {};
			}
			if (Match("(")) {
				Value value = ParseConditional(evaluate);
				if (!Match(")")) Error("expected ')' in #if expression");
				return value;
			}
			index++;
			if (token->kind == TokenKind::PPNumber) {
				stdsint literal = 0;
				FrontendCType type;
				if (!frontend_c_detail::ParseIntegerLiteral(
					token->spelling.reference(), *target, literal, type) ||
					!type.IsInteger()) {
					Error("#if requires an integer constant", token);
					return {};
				}
				Value value;
				value.bits = (uint64)(sint64)literal;
				value.is_unsigned = !type.is_signed;
				return value;
			}
			if (token->kind == TokenKind::CharacterConstant) {
				stduint length = token->spelling.getByteCount();
				uni::String contents;
				if (length >= 2)
					for (stduint offset = 1; offset + 1 < length; offset++)
						contents << token->spelling[offset];
				stdsint literal = 0;
				if (!frontend_c_detail::ParseCharacterLiteral(
					contents.reference(), *target, literal)) {
					Error("invalid character constant in #if expression", token);
					return {};
				}
				Value value;
				value.bits = (uint64)(sint64)literal;
				return value;
			}
			Error("invalid token in #if expression", token);
			return {};
		}

		Value ParseUnary(bool evaluate) {
			if (Match("+")) return ParseUnary(evaluate);
			if (Match("-")) {
				Value value = ParseUnary(evaluate);
				if (evaluate) value.bits = uint64(0) - value.bits;
				return value;
			}
			if (Match("!")) return BooleanValue(!ParseUnary(evaluate).Truth());
			if (Match("~")) {
				Value value = ParseUnary(evaluate);
				if (evaluate) value.bits = ~value.bits;
				return value;
			}
			return ParsePrimary(evaluate);
		}

		Value ParseMultiplicative(bool evaluate) {
			Value left = ParseUnary(evaluate);
			for (;;) {
				const Token* operation = Peek();
				if (!operation || (!IsToken(*operation, "*") &&
					!IsToken(*operation, "/") && !IsToken(*operation, "%")))
					return left;
				index++;
				Value right = ParseUnary(evaluate);
				if (!evaluate || !valid) continue;
				bool unsigned_operation = left.is_unsigned || right.is_unsigned;
				if (IsToken(*operation, "*")) left.bits *= right.bits;
				else {
					if (!right.bits) {
						Error("division by zero in #if expression", operation);
						return {};
					}
					if (unsigned_operation) {
						left.bits = IsToken(*operation, "/") ?
							left.bits / right.bits : left.bits % right.bits;
					}
					else {
						sint64 signed_left = (sint64)left.bits;
						sint64 signed_right = (sint64)right.bits;
						if (signed_left == (sint64)(uint64(1) << 63) && signed_right == -1) {
							Error("signed division overflows in #if expression", operation);
							return {};
						}
						left.bits = (uint64)(IsToken(*operation, "/") ?
							signed_left / signed_right : signed_left % signed_right);
					}
				}
				left.is_unsigned = unsigned_operation;
			}
		}

		Value ParseAdditive(bool evaluate) {
			Value left = ParseMultiplicative(evaluate);
			for (;;) {
				const Token* operation = Peek();
				if (!operation || (!IsToken(*operation, "+") && !IsToken(*operation, "-")))
					return left;
				index++;
				Value right = ParseMultiplicative(evaluate);
				if (evaluate && valid)
					left.bits = IsToken(*operation, "+") ?
						left.bits + right.bits : left.bits - right.bits;
				left.is_unsigned = left.is_unsigned || right.is_unsigned;
			}
		}

		Value ParseShift(bool evaluate) {
			Value left = ParseAdditive(evaluate);
			for (;;) {
				const Token* operation = Peek();
				if (!operation || (!IsToken(*operation, "<<") && !IsToken(*operation, ">>")))
					return left;
				index++;
				Value right = ParseAdditive(evaluate);
				if (!evaluate || !valid) continue;
				if (right.bits >= 64) {
					Error("shift count is outside the supported range", operation);
					return {};
				}
				if (IsToken(*operation, "<<")) left.bits <<= right.bits;
				else if (left.is_unsigned) left.bits >>= right.bits;
				else left.bits = (uint64)((sint64)left.bits >> right.bits);
			}
		}

		Value ParseRelational(bool evaluate) {
			Value left = ParseShift(evaluate);
			for (;;) {
				const Token* operation = Peek();
				if (!operation || (!IsToken(*operation, "<") &&
					!IsToken(*operation, "<=") && !IsToken(*operation, ">") &&
					!IsToken(*operation, ">="))) return left;
				index++;
				Value right = ParseShift(evaluate);
				if (!evaluate || !valid) {
					left = {};
					continue;
				}
				bool result;
				if (left.is_unsigned || right.is_unsigned) {
					if (IsToken(*operation, "<")) result = left.bits < right.bits;
					else if (IsToken(*operation, "<=")) result = left.bits <= right.bits;
					else if (IsToken(*operation, ">")) result = left.bits > right.bits;
					else result = left.bits >= right.bits;
				}
				else {
					sint64 signed_left = (sint64)left.bits;
					sint64 signed_right = (sint64)right.bits;
					if (IsToken(*operation, "<")) result = signed_left < signed_right;
					else if (IsToken(*operation, "<=")) result = signed_left <= signed_right;
					else if (IsToken(*operation, ">")) result = signed_left > signed_right;
					else result = signed_left >= signed_right;
				}
				left = BooleanValue(result);
			}
		}

		Value ParseEquality(bool evaluate) {
			Value left = ParseRelational(evaluate);
			for (;;) {
				const Token* operation = Peek();
				if (!operation || (!IsToken(*operation, "==") && !IsToken(*operation, "!=")))
					return left;
				index++;
				Value right = ParseRelational(evaluate);
				if (evaluate && valid) left = BooleanValue(IsToken(*operation, "==") ?
					left.bits == right.bits : left.bits != right.bits);
				else left = {};
			}
		}

		Value ParseBitAnd(bool evaluate) {
			Value left = ParseEquality(evaluate);
			while (Match("&")) {
				Value right = ParseEquality(evaluate);
				if (evaluate && valid) left.bits &= right.bits;
				left.is_unsigned = left.is_unsigned || right.is_unsigned;
			}
			return left;
		}

		Value ParseBitXor(bool evaluate) {
			Value left = ParseBitAnd(evaluate);
			while (Match("^")) {
				Value right = ParseBitAnd(evaluate);
				if (evaluate && valid) left.bits ^= right.bits;
				left.is_unsigned = left.is_unsigned || right.is_unsigned;
			}
			return left;
		}

		Value ParseBitOr(bool evaluate) {
			Value left = ParseBitXor(evaluate);
			while (Match("|")) {
				Value right = ParseBitXor(evaluate);
				if (evaluate && valid) left.bits |= right.bits;
				left.is_unsigned = left.is_unsigned || right.is_unsigned;
			}
			return left;
		}

		Value ParseLogicalAnd(bool evaluate) {
			Value left = ParseBitOr(evaluate);
			while (Match("&&")) {
				bool left_truth = left.Truth();
				Value right = ParseBitOr(evaluate && left_truth);
				if (evaluate) left = BooleanValue(left_truth && right.Truth());
			}
			return left;
		}

		Value ParseLogicalOr(bool evaluate) {
			Value left = ParseLogicalAnd(evaluate);
			while (Match("||")) {
				bool left_truth = left.Truth();
				Value right = ParseLogicalAnd(evaluate && !left_truth);
				if (evaluate) left = BooleanValue(left_truth || right.Truth());
			}
			return left;
		}

		Value ParseConditional(bool evaluate) {
			Value condition = ParseLogicalOr(evaluate);
			if (!Match("?")) return condition;
			bool condition_truth = condition.Truth();
			Value true_value = ParseConditional(evaluate && condition_truth);
			if (!Match(":")) {
				Error("expected ':' in #if conditional expression");
				return {};
			}
			Value false_value = ParseConditional(evaluate && !condition_truth);
			if (!evaluate) return {};
			Value result = condition_truth ? true_value : false_value;
			return CommonValue(result,
				true_value.is_unsigned || false_value.is_unsigned);
		}

	public:
		ExpressionParser(const uni::Vector<Token>& expression,
			const FrontendCDiagnostics& diagnostic_output,
			const FrontendCTargetInfo& target_information) :
			tokens(&expression), diagnostics(&diagnostic_output),
			target(&target_information) {}

		bool Parse(Value& value) {
			if (!tokens->Count()) return Error("#if requires an expression");
			value = ParseConditional(true);
			if (valid && index != tokens->Count())
				return Error("unexpected token in #if expression");
			return valid;
		}
	};

}

bool Preprocessor::EvaluateIfExpression(const uni::Vector<Token>& input,
	stduint begin, stduint end, Value& value) {
	uni::Vector<Token> protected_expression;
	for (stduint index = begin; index < end;) {
		const Token& token = input.begin()[index];
		if (token.kind != TokenKind::Identifier ||
			StrCompare(token.spelling.reference(), "defined")) {
			protected_expression.Append(token);
			index++;
			continue;
		}
		stduint operand = index + 1;
		while (operand < end && input.begin()[operand].kind == TokenKind::Newline)
			operand++;
		bool parenthesized = operand < end && IsToken(input.begin()[operand], "(");
		if (parenthesized) {
			operand++;
			while (operand < end && input.begin()[operand].kind == TokenKind::Newline)
				operand++;
		}
		if (operand >= end || input.begin()[operand].kind != TokenKind::Identifier)
			return Report("defined requires an identifier", token);
		const Token& name = input.begin()[operand++];
		if (parenthesized) {
			while (operand < end && input.begin()[operand].kind == TokenKind::Newline)
				operand++;
			if (operand >= end || !IsToken(input.begin()[operand], ")"))
				return Report("expected ')' after defined operand", token);
			operand++;
		}
		Token result = token;
		result.kind = TokenKind::PPNumber;
		result.spelling = FindMacro(name.spelling.reference()) ? "1" : "0";
		protected_expression.Append(result);
		index = operand;
	}

	uni::Vector<Token> expanded;
	uni::Vector<uni::String> disabled;
	if (!ExpandSequence(protected_expression, 0, protected_expression.Count(),
		expanded, disabled, 0)) return false;
	uni::Vector<Token> normalized;
	for (Token token : expanded) {
		if (token.kind == TokenKind::Newline) continue;
		if (token.kind == TokenKind::Identifier) {
			token.kind = TokenKind::PPNumber;
			token.spelling = "0";
		}
		normalized.Append(token);
	}
	ExpressionParser parser(normalized, diagnostics, *target);
	return parser.Parse(value);
}

}
