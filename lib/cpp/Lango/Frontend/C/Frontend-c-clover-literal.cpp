// ASCII C++11 TAB4 LF
// Docutitle: Lango C Frontend Clover Literal Lowering
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

#include "../../../../../inc/cpp/Lango/Frontend/C/Frontend-c-clover-internal.hpp"
#include "../../../../../inc/c/datype/floating.h"

namespace clover::frontend_c_internal {

	namespace {
		struct IntegerLiteralParts {
			uint64 magnitude = 0;
			bool is_decimal = true;
			bool has_unsigned_suffix = false;
			unsigned long_suffix_count = 0;
		};

		struct IntegerLiteralCandidate {
			FrontendCIntegerRank rank = FrontendCIntegerRank::Int;
			bool is_signed = true;
		};

		unsigned DigitValue(char character) {
			if (character >= '0' && character <= '9')
				return (unsigned)(character - '0');
			if (character >= 'a' && character <= 'f')
				return (unsigned)(character - 'a' + 10);
			if (character >= 'A' && character <= 'F')
				return (unsigned)(character - 'A' + 10);
			return 16;
		}

		bool IsSuffixCharacter(char character, char expected) {
			return character == expected || character == expected - 'a' + 'A';
		}

		double ScaleFloat(double value, double base, int exponent) {
			bool divide = exponent < 0;
			unsigned magnitude = divide ? (unsigned)(-(exponent + 1)) + 1 :
				(unsigned)exponent;
			double scale = 1.0;
			while (magnitude) {
				if (magnitude & 1) scale *= base;
				base *= base;
				magnitude >>= 1;
			}
			return divide ? value / scale : value * scale;
		}

		bool ParseSignedExponent(const char*& cursor, int& exponent) {
			bool negative = false;
			if (*cursor == '+' || *cursor == '-') negative = *cursor++ == '-';
			if (!ascii_isdigit(*cursor)) return false;
			unsigned magnitude = 0;
			while (ascii_isdigit(*cursor)) {
				if (magnitude < 100000)
					magnitude = magnitude * 10 + (unsigned)(*cursor - '0');
				cursor++;
			}
			exponent = negative ? -(int)magnitude : (int)magnitude;
			return true;
		}

		bool ParseDecimalFloat(const char*& cursor, double& value) {
			bool has_digit = false;
			while (ascii_isdigit(*cursor)) {
				value = value * 10.0 + (*cursor++ - '0');
				has_digit = true;
			}
			int fractional_digits = 0;
			bool has_point = *cursor == '.';
			if (has_point) {
				cursor++;
				while (ascii_isdigit(*cursor)) {
					value = value * 10.0 + (*cursor++ - '0');
					fractional_digits++;
					has_digit = true;
				}
			}
			bool has_exponent = *cursor == 'e' || *cursor == 'E';
			int exponent = 0;
			if (has_exponent) {
				cursor++;
				if (!ParseSignedExponent(cursor, exponent)) return false;
			}
			if (!has_digit || (!has_point && !has_exponent)) return false;
			value = ScaleFloat(value, 10.0, exponent - fractional_digits);
			return true;
		}

		bool ParseHexadecimalFloat(const char*& cursor, double& value) {
			cursor += 2;
			bool has_digit = false;
			while (DigitValue(*cursor) < 16) {
				value = value * 16.0 + DigitValue(*cursor++);
				has_digit = true;
			}
			int fractional_digits = 0;
			if (*cursor == '.') {
				cursor++;
				while (DigitValue(*cursor) < 16) {
					value = value * 16.0 + DigitValue(*cursor++);
					fractional_digits++;
					has_digit = true;
				}
			}
			if (!has_digit || (*cursor != 'p' && *cursor != 'P')) return false;
			cursor++;
			int exponent;
			if (!ParseSignedExponent(cursor, exponent)) return false;
			value = ScaleFloat(value, 2.0, exponent - fractional_digits * 4);
			return true;
		}

		bool ParseIntegerLiteralParts(const char* text, IntegerLiteralParts& parts) {
			if (!text || !*text) return false;
			const char* cursor = text;
			unsigned base = 10;
			if (cursor[0] == '0' && (cursor[1] == 'x' || cursor[1] == 'X')) {
				base = 16;
				parts.is_decimal = false;
				cursor += 2;
			}
			else if (cursor[0] == '0') {
				base = 8;
				parts.is_decimal = false;
			}

			bool has_digit = false;
			for (;; cursor++) {
				unsigned digit = DigitValue(*cursor);
				if (digit >= 16) break;
				if (digit >= base) return false;
				if (parts.magnitude > (UINT64_MAX - digit) / base) return false;
				parts.magnitude = parts.magnitude * base + digit;
				has_digit = true;
			}
			if (!has_digit) return false;

			if (IsSuffixCharacter(*cursor, 'u')) {
				parts.has_unsigned_suffix = true;
				cursor++;
				if (IsSuffixCharacter(*cursor, 'l')) {
					char long_character = *cursor;
					parts.long_suffix_count = 1;
					cursor++;
					if (*cursor == long_character) {
						parts.long_suffix_count = 2;
						cursor++;
					}
				}
			}
			else if (IsSuffixCharacter(*cursor, 'l')) {
				char long_character = *cursor;
				parts.long_suffix_count = 1;
				cursor++;
				if (*cursor == long_character) {
					parts.long_suffix_count = 2;
					cursor++;
				}
				if (IsSuffixCharacter(*cursor, 'u')) {
					parts.has_unsigned_suffix = true;
					cursor++;
				}
			}
			return !*cursor;
		}

		unsigned IntegerBits(FrontendCIntegerRank rank,
			const FrontendCTargetInfo& target) {
			switch (rank) {
			case FrontendCIntegerRank::Bool: return 1;
			case FrontendCIntegerRank::Char: return target.char_bits;
			case FrontendCIntegerRank::Short: return target.short_bits;
			case FrontendCIntegerRank::Int: return target.int_bits;
			case FrontendCIntegerRank::Long: return target.long_bits;
			case FrontendCIntegerRank::LongLong: return target.long_long_bits;
			}
			return 0;
		}

		bool MagnitudeFits(uint64 magnitude, unsigned bits, bool is_signed) {
			if (!bits || bits > 64) return false;
			if (is_signed) {
				uint64 maximum = bits == 64 ? (uint64)INT64_MAX :
					((uint64(1) << (bits - 1)) - 1);
				return magnitude <= maximum;
			}
			uint64 maximum = bits == 64 ? UINT64_MAX :
				((uint64(1) << bits) - 1);
			return magnitude <= maximum;
		}

		void AppendCandidate(IntegerLiteralCandidate* candidates, unsigned& count,
			FrontendCIntegerRank rank, bool is_signed) {
			candidates[count].rank = rank;
			candidates[count].is_signed = is_signed;
			count++;
		}

		void ListIntegerCandidates(const IntegerLiteralParts& parts,
			IntegerLiteralCandidate* candidates, unsigned& count) {
			count = 0;
			if (parts.long_suffix_count == 2) {
				AppendCandidate(candidates, count, FrontendCIntegerRank::LongLong,
					!parts.has_unsigned_suffix);
				if (!parts.is_decimal && !parts.has_unsigned_suffix)
					AppendCandidate(candidates, count,
						FrontendCIntegerRank::LongLong, false);
				return;
			}
			if (parts.long_suffix_count == 1) {
				AppendCandidate(candidates, count, FrontendCIntegerRank::Long,
					!parts.has_unsigned_suffix);
				if (!parts.is_decimal && !parts.has_unsigned_suffix)
					AppendCandidate(candidates, count, FrontendCIntegerRank::Long, false);
				AppendCandidate(candidates, count, FrontendCIntegerRank::LongLong,
					!parts.has_unsigned_suffix);
				if (!parts.is_decimal && !parts.has_unsigned_suffix)
					AppendCandidate(candidates, count,
						FrontendCIntegerRank::LongLong, false);
				return;
			}
			AppendCandidate(candidates, count, FrontendCIntegerRank::Int,
				!parts.has_unsigned_suffix);
			if (!parts.is_decimal && !parts.has_unsigned_suffix)
				AppendCandidate(candidates, count, FrontendCIntegerRank::Int, false);
			AppendCandidate(candidates, count, FrontendCIntegerRank::Long,
				!parts.has_unsigned_suffix);
			if (!parts.is_decimal && !parts.has_unsigned_suffix)
				AppendCandidate(candidates, count, FrontendCIntegerRank::Long, false);
			AppendCandidate(candidates, count, FrontendCIntegerRank::LongLong,
				!parts.has_unsigned_suffix);
			if (!parts.is_decimal && !parts.has_unsigned_suffix)
				AppendCandidate(candidates, count,
					FrontendCIntegerRank::LongLong, false);
		}

		stdsint SignedBitPattern(uint64 magnitude) {
			if (magnitude <= (uint64)INT64_MAX) return (stdsint)magnitude;
			return -1 - (stdsint)(UINT64_MAX - magnitude);
		}

		bool DecodeCEscape(const char*& text, unsigned& value) {
			if (!text || !*text) return false;
			value = (unsigned char)*text++;
			if (value != '\\') return true;
			if (!*text) return false;
			char escaped = *text++;
			if (escaped >= '0' && escaped <= '7') {
				value = (unsigned)(escaped - '0');
				for (unsigned count = 1;
					count < 3 && *text >= '0' && *text <= '7'; count++)
					value = value * 8 + (unsigned)(*text++ - '0');
			}
			else if (escaped == 'x') {
				if (!ascii_isxdigit(*text)) return false;
				value = 0;
				while (ascii_isxdigit(*text)) {
					unsigned digit = DigitValue(*text++);
					if (value > (0xFFU - digit) / 16) return false;
					value = value * 16 + digit;
				}
			}
			else {
				switch (escaped) {
				case 'a': value = 7; break;
				case 'b': value = 8; break;
				case 'f': value = 12; break;
				case 'n': value = 10; break;
				case 'r': value = 13; break;
				case 't': value = 9; break;
				case 'v': value = 11; break;
				case '\\': value = '\\'; break;
				case '\'': value = '\''; break;
				case '"': value = '"'; break;
				case '?': value = '?'; break;
				default: return false;
				}
			}
			return value <= 0xFF;
		}
	}

	bool ParseIntegerLiteral(const char* text, const FrontendCTargetInfo& target,
		stdsint& value, FrontendCType& type) {
		IntegerLiteralParts parts;
		if (!ParseIntegerLiteralParts(text, parts)) return false;
		IntegerLiteralCandidate candidates[6];
		unsigned candidate_count;
		ListIntegerCandidates(parts, candidates, candidate_count);
		for (unsigned index = 0; index < candidate_count; index++) {
			const IntegerLiteralCandidate& candidate = candidates[index];
			if (!MagnitudeFits(parts.magnitude,
				IntegerBits(candidate.rank, target), candidate.is_signed)) continue;
			type = FrontendCType();
			type.integer_rank = candidate.rank;
			type.is_signed = candidate.is_signed;
			value = SignedBitPattern(parts.magnitude);
			return true;
		}
		return false;
	}

	bool ParseIntegerLiteral(const char* text, stdsint& value) {
		FrontendCType type;
		return ParseIntegerLiteral(text,
			FrontendCTargetInfo::FromDataModel(CDataModel::LP64), value, type);
	}

	bool ParseFloatingLiteral(const char* text, stdsint& bits,
		FrontendCType& type) {
		if (!text || !*text) return false;
		const char* cursor = text;
		double value = 0.0;
		bool valid = cursor[0] == '0' && (cursor[1] == 'x' || cursor[1] == 'X') ?
			ParseHexadecimalFloat(cursor, value) : ParseDecimalFloat(cursor, value);
		if (!valid) return false;
		bool is_float = *cursor == 'f' || *cursor == 'F';
		if (is_float) cursor++;
		else if (*cursor == 'l' || *cursor == 'L') return false;
		if (*cursor) return false;

		type = FrontendCType();
		type.scalar_kind = is_float ? FrontendCScalarKind::Float :
			FrontendCScalarKind::Double;
		if (is_float) {
			union {
				float value;
				uint32 bits;
			} encoded;
			encoded.value = (float)value;
			bits = (stdsint)encoded.bits;
		}
		else {
			union {
				double value;
				uint64 bits;
			} encoded;
			encoded.value = value;
			bits = (stdsint)encoded.bits;
		}
		return true;
	}

	bool ParseLongDoubleConstant(const char* text, stdsint& low, stdsint& high) {
		if (!text || !*text) return false;
		stduint length = StrLength(text);
		if (!length) return false;
		bool has_suffix = text[length - 1] == 'l' || text[length - 1] == 'L' ||
			text[length - 1] == 'f' || text[length - 1] == 'F';
		stduint literal_length = length - (has_suffix ? 1 : 0);
		char* literal = new char[literal_length + 1];
		MemCopyN(literal, text, literal_length);
		literal[literal_length] = 0;
		uint8 bytes[16] = {};
		bool valid = float_const(literal, 1, bytes, 16) != 0;
		delete[] literal;
		if (!valid) return false;
		uint64 low_word = 0;
		uint64 high_word = 0;
		for (unsigned index = 0; index < 8; index++) {
			low_word |= (uint64)bytes[index] << (index * 8);
			high_word |= (uint64)bytes[index + 8] << (index * 8);
		}
		low = (stdsint)low_word;
		high = (stdsint)high_word;
		return true;
	}

	bool ParseLongDoubleLiteral(const char* text, stdsint& low, stdsint& high) {
		if (!text) return false;
		stduint length = StrLength(text);
		return length && (text[length - 1] == 'l' || text[length - 1] == 'L') &&
			ParseLongDoubleConstant(text, low, high);
	}

	bool ParseCharacterLiteral(const char* text, const FrontendCTargetInfo& target,
		stdsint& value) {
		if (!text || !*text || !target.char_bits || target.char_bits > 8) return false;
		unsigned character;
		if (!DecodeCEscape(text, character) || *text) return false;
		uint64 character_limit = (uint64(1) << target.char_bits) - 1;
		if (character > character_limit) return false;
		value = (stdsint)character;
		if (target.char_is_signed &&
			(character & (uint64(1) << (target.char_bits - 1))))
			value -= (stdsint)(uint64(1) << target.char_bits);
		return true;
	}

	// Decode the ordinary C99 escape sequences needed by narrow string literals.
	bool DecodeCString(const char* text, uni::Vector<stdsint>& bytes) {
		if (!text) return true;
		while (*text) {
			unsigned value;
			if (!DecodeCEscape(text, value)) return false;
			bytes.Append((stdsint)value);
		}
		return true;
	}

	LoweredValue FunctionBuilder::LowerStringLiteral(uni::Nnode* node) {
		uni::Vector<stdsint> bytes;
		if (!DecodeCString(node->addr, bytes))
			return Error("invalid string literal escape", node);
		bytes.Append(0);

		Global global;
		global.name = ".L.clover.string.";
		AppendUnsigned(global.name, (*next_string_literal)++);
		global.type = Type::I8;
		global.element_count = (stdsint)bytes.Count();
		global.is_read_only = true;
		global.linkage = Linkage::Internal;
		global.initializer_values = bytes;
		module->globals.Append(global);

		FrontendCType type;
		type.integer_rank = FrontendCIntegerRank::Char;
		type.is_signed = target->char_is_signed;
		type.AppendArrayDimension(bytes.Count());
		return { EmitGlobalAddress(global.name), true, type, true, 1 };
	}

}

bool uni::lango::frontend_c_detail::ParseIntegerLiteral(const char* text,
	const FrontendCTargetInfo& target, stdsint& value, FrontendCType& type) {
	return clover::frontend_c_internal::ParseIntegerLiteral(
		text, target, value, type);
}

bool uni::lango::frontend_c_detail::ParseCharacterLiteral(const char* text,
	const FrontendCTargetInfo& target, stdsint& value) {
	return clover::frontend_c_internal::ParseCharacterLiteral(text, target, value);
}
