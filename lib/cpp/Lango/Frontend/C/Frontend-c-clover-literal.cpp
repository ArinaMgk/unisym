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

namespace clover::frontend_c_internal {

	bool ParseIntegerLiteral(const char* text, stdsint& value) {
		if (!text || !*text) return false;
		unsigned base = 10;
		if (text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
			base = 16;
			text += 2;
		}
		else if (text[0] == '0' && text[1]) base = 8;
		if (!*text) return false;
		stdsint result = 0;
		for (; *text; text++) {
			unsigned digit;
			if (*text >= '0' && *text <= '9') digit = (unsigned)(*text - '0');
			else if (*text >= 'a' && *text <= 'f') digit = (unsigned)(*text - 'a' + 10);
			else if (*text >= 'A' && *text <= 'F') digit = (unsigned)(*text - 'A' + 10);
			else return false;
			if (digit >= base) return false;
			result = result * (stdsint)base + (stdsint)digit;
		}
		value = result;
		return true;
	}

	// Decode the ordinary C99 escape sequences needed by narrow string literals.
	bool DecodeCString(const char* text, uni::Vector<stdsint>& bytes) {
		if (!text) return true;
		while (*text) {
			unsigned value = (unsigned char)*text++;
			if (value != '\\') {
				bytes.Append((stdsint)value);
				continue;
			}
			if (!*text) return false;
			char escaped = *text++;
			if (escaped >= '0' && escaped <= '7') {
				value = (unsigned)(escaped - '0');
				for (unsigned count = 1; count < 3 && *text >= '0' && *text <= '7'; count++)
					value = value * 8 + (unsigned)(*text++ - '0');
			}
			else if (escaped == 'x') {
				if (!ascii_isxdigit(*text)) return false;
				value = 0;
				while (ascii_isxdigit(*text)) {
					char digit = *text++;
					value = value * 16 + (unsigned)(digit >= '0' && digit <= '9' ?
						digit - '0' : digit >= 'A' && digit <= 'F' ?
						digit - 'A' + 10 : digit - 'a' + 10);
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
			bytes.Append((stdsint)(value & 0xFF));
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
		global.is_internal = true;
		global.initializer_values = bytes;
		module->globals.Append(global);

		FrontendCType type;
		type.integer_rank = FrontendCIntegerRank::Char;
		type.is_signed = target->char_is_signed;
		type.AppendArrayDimension(bytes.Count());
		return { EmitGlobalAddress(global.name), true, type, true, 1 };
	}

}
