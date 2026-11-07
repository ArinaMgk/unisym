// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Data) Token Node Parse Instructions and Directives
// Codifiers: @ArinaMgk 
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
/*
	Copyright 2023 ArinaMgk

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

#include "../../inc/cpp/parse.hpp"
#include "../../inc/c/multichar.h"


static toktype getctype(signed char chr)// Excluding Number
{
	if (ascii_isalnum(chr))
		return tok_identy;
	else if (ascii_isspace(chr))
		return tok_spaces;
	else if (ascii_ispunct(chr))
		return tok_symbol;// '_' may be of identifier
	return tok_others;
}

// It may be better to use Regular Expression
const char static EscSeq[] =
{
	'n','\n','r','\r','a','\a','b','\b','f','\f','t','\t','v','\v',
	'\\','\\','\'','\'','"','"','?','?'
};

namespace uni
{
	static int EscapeHexValue(char character) {
		if (character >= '0' && character <= '9') return character - '0';
		if (character >= 'A' && character <= 'F') return character - 'A' + 10;
		if (character >= 'a' && character <= 'f') return character - 'a' + 10;
		return -1;
	}

	EscapeSequenceRules EscapeSequenceRules::Permissive() {
		return EscapeSequenceRules {
			{1, 3, MissingDigitsAction::Reject},
			{0, 2, MissingDigitsAction::KeepPrefix},
			{0, 4, MissingDigitsAction::KeepPrefix},
			{0, 8, MissingDigitsAction::KeepPrefix},
			UnknownEscapeAction::KeepCharacter,
			true,
			false
		};
	}

	EscapeSequenceRules EscapeSequenceRules::AASM() {
		return Permissive();
	}

	EscapeSequenceRules EscapeSequenceRules::C() {
		return EscapeSequenceRules {
			{1, 3, MissingDigitsAction::Reject},
			{1, 0, MissingDigitsAction::Reject},
			{4, 4, MissingDigitsAction::Reject},
			{8, 8, MissingDigitsAction::Reject},
			UnknownEscapeAction::Reject,
			false,
			true
		};
	}

	EscapeSequenceRules EscapeSequenceRules::CWithExtensions() {
		EscapeSequenceRules rules = C();
		rules.allow_escape_character = true;
		return rules;
	}

	static EscapeSequenceResult DecodeNumericEscape(
		const char* text, stduint length, stduint prefix_length,
		unsigned base, EscapeSequenceType type,
		const NumericEscapeRule& rule, bool validate_unicode_scalar) {
		EscapeSequenceResult result {
			type,
			EscapeSequenceStatus::Success,
			EscapeSequenceError::None,
			0,
			prefix_length,
			0
		};
		bool overflow = false;
		uint64 maximum_value = ~(uint64)0;
		while (result.consumed < length &&
			(!rule.maximum_digits || result.digits < rule.maximum_digits)) {
			int digit = EscapeHexValue(text[result.consumed]);
			if (digit < 0 || (unsigned)digit >= base) break;
			if (result.value > (maximum_value - (unsigned)digit) / base)
				overflow = true;
			else if (!overflow)
				result.value = result.value * base + (unsigned)digit;
			result.consumed++;
			result.digits++;
		}

		if (!result.digits &&
			rule.missing_digits == MissingDigitsAction::KeepPrefix) {
			result.type = EscapeSequenceType::Byte;
			result.value = (byte)text[0];
			result.consumed = 1;
			return result;
		}
		if (result.digits < rule.minimum_digits) {
			result.status = result.consumed == length ?
				EscapeSequenceStatus::Incomplete : EscapeSequenceStatus::Invalid;
			result.error = result.digits ? EscapeSequenceError::InsufficientDigits :
				EscapeSequenceError::MissingDigits;
			return result;
		}
		if (overflow) {
			result.status = EscapeSequenceStatus::Invalid;
			result.error = EscapeSequenceError::NumericOverflow;
			return result;
		}
		if (type == EscapeSequenceType::Unicode && validate_unicode_scalar &&
			(result.value > 0x10FFFF ||
			(result.value >= 0xD800 && result.value <= 0xDFFF))) {
			result.status = EscapeSequenceStatus::Invalid;
			result.error = EscapeSequenceError::InvalidUnicodeScalar;
		}
		return result;
	}

	EscapeSequenceResult DecodeEscapeSequence(
		const char* text, stduint length,
		const EscapeSequenceRules& rules) {
		EscapeSequenceResult result {
			EscapeSequenceType::Byte,
			EscapeSequenceStatus::Incomplete,
			EscapeSequenceError::None,
			0,
			0,
			0
		};
		if (!text || !length) return result;

		char first = text[0];
		result.status = EscapeSequenceStatus::Success;
		result.value = (byte)first;
		result.consumed = 1;

		if (first >= '0' && first <= '7')
			return DecodeNumericEscape(text, length, 0, 8,
				EscapeSequenceType::Byte, rules.octal,
				rules.validate_unicode_scalar);
		if (first == 'x' || first == 'X')
			return DecodeNumericEscape(text, length, 1, 16,
				EscapeSequenceType::Byte, rules.hexadecimal,
				rules.validate_unicode_scalar);
		if (first == 'u')
			return DecodeNumericEscape(text, length, 1, 16,
				EscapeSequenceType::Unicode, rules.unicode_short,
				rules.validate_unicode_scalar);
		if (first == 'U')
			return DecodeNumericEscape(text, length, 1, 16,
				EscapeSequenceType::Unicode, rules.unicode_long,
				rules.validate_unicode_scalar);
		if (first == 'e' && rules.allow_escape_character) {
			result.value = 27;
			return result;
		}

		for (stduint i = 0; i < sizeof(EscSeq) / 2; i++) {
			if (EscSeq[i << 1] == first) {
				result.value = (byte)EscSeq[(i << 1) + 1];
				return result;
			}
		}
		if (rules.unknown_escape == UnknownEscapeAction::Reject) {
			result.status = EscapeSequenceStatus::Invalid;
			result.error = EscapeSequenceError::UnknownEscape;
		}
		return result;
	}

	LinearParser::LinearParser(IstreamTrait& lparser) {
		src = &lparser;
		buf = new String(String::Charset::Memory);
		tobuf = new String(String::Charset::Memory);
	}
	LinearParser::LinearParser(IstreamTrait& lparser, char* addr, stduint buflen) {
		src = &lparser;
		buf = NULL;// or static_lnbuf
		new (static_lnbuf) String(addr, buflen);
		_TODO tobuf = new String(String::Charset::Memory);
	}
	LinearParser::LinearParser(IstreamTrait& lparser, String& buf) {
		src = &lparser;
		this->buf = &buf;
		_TODO tobuf = new String(String::Charset::Memory);
	}

	static Tnode* TokenAppend(Dchain& chn,
		rostr content,
		size_t contlen,
		size_t ttype,
		size_t row,
		size_t col,
		bool allow_empty = false
	)
	{
		if (!contlen && ttype != tok_string && !allow_empty) return 0;
		Letvar(tn, Tnode*, chn.Append(StrHeapN(content, contlen), false));
		tn->type = ttype;
		tn->row = row;
		tn->col = col;
		return tn;
	}

	int LinearParser::getChar() {
		int ch = todobuf->inn();
		if (ch == EOF) ch = askChar();
		pos.x += 1;
		return ch;
	}

	#define apd() do { \
		if (!(method_omit_spaces && CrtTType == tok_spaces || method_omit_comment && CrtTType == tok_comment)) \
		{ TokenAppend(dc, linebuf->reference(), linebuf->getByteCount(), CrtTType, crtline_ento, crtcol_ento, YoString != 0); } \
		linebuf->Clear();\
		} while (0)

	// CRLF LFCR CR LF, c is the first char
	inline static void EatLinefeed(LinearParser& lp, signed char c) {
		int cc = lp.getChar();
		if (cc == '\n' || cc == '\r') {
			if (c == cc) lp.backChar(cc);
		}
		else if (cc != EOF) lp.backChar(cc);
	}

	stduint LineParse_NumChk_Default(LinearParser& lp) {
		size_t res = 0;
		int c, lastc = nil;
		int last_zo_num = 0;
		struct OnceOccur {
			unsigned bodx : 1;
			unsigned e : 1;
			unsigned dot : 1;
			unsigned sign : 1;
		} OnceO = {};
		while ((c = lp.getChar()) != EOF) {
			if (StrIndexChar("bodxij", c))// 'i' and 'j' for imagine, appended Haruno RFC05
				if (OnceO.bodx)
					break;
				else
				{
					last_zo_num = 0; OnceO.bodx = 1;
				}
			else if (c == 'e')
				if (OnceO.e || !last_zo_num)
					break;
				else
				{
					last_zo_num = 0; OnceO.e = 1;
				}
			else if (c == '.')
				if (OnceO.dot)
					break;
				else
				{
					last_zo_num = 0; OnceO.dot = 1;
				}
			else if (c == '+' || c == '-')
				if (OnceO.sign || !OnceO.e || lastc != 'e')
					break;
				else
				{
					last_zo_num = 0; OnceO.sign = 1;
				}
			else if (c >= '0' && c <= '9') last_zo_num = 1;
			else if (lp.method_line_continuation && c == '\\') {
				int ch0 = lp.getChar();
				if (ch0 == '\n' || ch0 == '\r') {
					EatLinefeed(lp, ch0); ++lp.pos.y; continue;
				}
				else {
					lp.backChar(ch0);
					lp.backChar('\\');
					return res;
				}
			}
			else break;
			res++;
			lp.setChar(c);
			lastc = c;
		}
		if (res > 1 && (lastc == '+' || lastc == '-' || lastc == '.'))
		{
			lp.backChar(lastc);
			res--;
		}
		if (c != EOF) lp.backChar(c);
		return res;
	}


	// return true for continue, false for break
	bool LinearParser::handler_escape_sequence() {
		int character = getChar();
		if (character == EOF) return false;
		String sequence(String::Charset::Memory);
		sequence << (char)character;
		const NumericEscapeRule* rule = nullptr;
		bool hexadecimal = false;
		if (character >= '0' && character <= '7')
			rule = &escape_sequence_rules.octal;
		else if (character == 'x' || character == 'X') {
			rule = &escape_sequence_rules.hexadecimal;
			hexadecimal = true;
		}
		else if (character == 'u') {
			rule = &escape_sequence_rules.unicode_short;
			hexadecimal = true;
		}
		else if (character == 'U') {
			rule = &escape_sequence_rules.unicode_long;
			hexadecimal = true;
		}

		stduint digits = character >= '0' && character <= '7' ? 1 : 0;
		while (rule &&
			(!rule->maximum_digits || digits < rule->maximum_digits)) {
			character = getChar();
			if (character == EOF) break;
			bool accepted = hexadecimal ? ascii_isxdigit(character) :
				(character >= '0' && character <= '7');
			if (!accepted) {
				backChar((char)character);
				break;
			}
			sequence << (char)character;
			digits++;
		}

		EscapeSequenceResult result = DecodeEscapeSequence(
			sequence.reference(), sequence.getByteCount(), escape_sequence_rules);
		if (result.status != EscapeSequenceStatus::Success) {
			if (handler_escape_diagnostic)
				handler_escape_diagnostic(*this, result);
			return false;
		}
		if (result.type == EscapeSequenceType::Byte) {
			setChar((char)result.value);
			return true;
		}

		byte encoded[6];
		pureptr_t destination = encoded;
		uint32 codepoint = (uint32)result.value;
		stduint length = result.value > 0xFFFFFFFF ? NONE :
			CscUTF(32, 8, &codepoint, byteof(codepoint), &destination);
		if (length == NONE) {
			result.status = EscapeSequenceStatus::Invalid;
			result.error = EscapeSequenceError::UnicodeConversionFailed;
			if (handler_escape_diagnostic)
				handler_escape_diagnostic(*this, result);
			return false;
		}
		for0(i, length) setChar((char)encoded[i]);
		return true;
	}

	Dchain& LinearParser::Parse(Dchain& dc) {
		stduint CrtTType = tok_any;
		stduint crtline_ento = pos.x, crtcol_ento = pos.y;
		int YoString = 0;
		bool YoStringRawEscape = false;
		signed char strtok;
		int c;

		this->linebuf = buf ? buf : (String*)&static_lnbuf;
		String& buffer = *linebuf;
		buffer.charset = String::Charset::Memory;
		buffer.inn_direction = false;

		this->todobuf = tobuf ? tobuf : (String*)&static_tobuf;
		String& tbuf = *todobuf;
		tbuf.charset = String::Charset::Memory;
		tbuf.inn_direction = false;

		dc.func_free = DnodeHeapFreeSimple;
		dc.setExtnField(sizeof(TnodeField));
		if (GHT) TokenAppend(dc, "GHTANIRA", 1, tok_any, 0, 0);// Traditional Header Guard
		while ((c = getChar()) != EOF)
		{
			Letvar(dbg_chr, char, c);
			if (YoString)
			{
				if (!method_string_escape_sequence && YoStringRawEscape) {
					setChar(c);
					YoStringRawEscape = false;
				}
				else if (strtok == c) {
					apd();// including terminated-symbol
					//pos.x += 2;
					crtline_ento = pos.y;
					crtcol_ento = pos.x - 1;
					YoString = 0;
					CrtTType = tok_any;
				}
				else if (method_string_escape_sequence && c == '\\') {
					if (handler_escape_sequence())
						continue; else break;
				}
				else if (c == '\\') {
					setChar(c);
					YoStringRawEscape = true;
				}
				else setChar(c);
			}
			else if (c == '\n' || c == '\r')
			{
				EatLinefeed(self, c);
				bool line_continuation = method_line_continuation &&
					linebuf->getByteCount() && (*linebuf)[linebuf->getByteCount() - 1] == '\\';
				// no consider: directive, comment
				++pos.y;
				// if (CrtTType == tok_direct); else
				if (!line_continuation) {
					if (linebuf->getByteCount()) apd();
					CrtTType = tok_any;
					crtline_ento = pos.y;
					crtcol_ento = pos.x = 1; //_Default_Row_or_Col;
				}
				else if (linebuf->getByteCount() > 1) {
					linebuf->inn_rear();
				}
				else if (dc.Count()) {
					linebuf->Clear();
					CrtTType = dc.Last()->type;
					auto tmpp = dc.Last()->addr;
					if (tmpp) while (*tmpp) {
						setChar(*tmpp++);
					}
					dc.Remove(dc.Last());
				}
			}
			else if (CrtTType == tok_direct || method_directive &&
				c == method_directive &&
				(CrtTType == tok_any || dc.Last()->GetTnodeField()->row != pos.y)
				) {
				apd();
				crtline_ento = pos.y;
				crtcol_ento = pos.x - 1;
				CrtTType = tok_direct;

				bool line_continuation;
				do {
					while ((c = getChar()) != EOF && c != '\n' && c != '\r')
						setChar(c);
					line_continuation = method_line_continuation &&
						linebuf->getByteCount() && (*linebuf)[linebuf->getByteCount() - 1] == '\\';
					if (line_continuation) linebuf->inn_rear();
					apd();
					if (line_continuation) {
						EatLinefeed(self, c);
						crtline_ento = ++pos.y;
						crtcol_ento = pos.x = 1; //_Default_Row_or_Col;
					}
				} while (line_continuation);
				crtline_ento = pos.y;
				crtcol_ento = pos.x - 1;
				CrtTType = tok_any;
				backChar(c);
			}

			else if (handler_comment && 1 &&
				backChar(c) && handler_comment(self, true)) {
				apd();
				crtline_ento = pos.y;
				crtcol_ento = pos.x - 1;
				handler_comment(self, false);
				CrtTType = tok_comment;
				apd();
				crtline_ento = pos.y;
				crtcol_ento = pos.x - 1;
				CrtTType = tok_any;
			}

			else if (method_string_double_quote && c == '\"' ||
				method_string_single_quote && c == '\'')// enter string
			{
				apd();
				crtline_ento = pos.y;
				crtcol_ento = pos.x - 1;
				CrtTType = c == '\'' ? method_string_single_quote_type : tok_string;
				YoString = 1;
				YoStringRawEscape = false;
				strtok = c;
			}
			else if (
				ascii_isdigit(c) && handler_numchk &&
				CrtTType != tok_identy)
			{
				if (CrtTType != tok_any) {
					apd();
					crtline_ento = pos.y;
					crtcol_ento = pos.x - 1;
				}
				backChar(c);
				stduint lens = handler_numchk(self);
				// ploginfo("%s handler_numchk> %[u]", __FUNCIDEN__, lens);
				//
				CrtTType = tok_number;
				apd();
				crtline_ento = pos.y;
				crtcol_ento = pos.x;
				CrtTType = tok_any;
				continue;
			}
			else if (ascii_isalnum(c) ||
				method_treatiden_underscore && c == '_')
			{
				if (CrtTType == tok_identy || CrtTType == tok_any)
				{
					setChar(c);
					if (CrtTType == tok_any) CrtTType = tok_identy;
				}
				else
				{
					apd();
					crtline_ento = pos.y;
					crtcol_ento = pos.x - 1;
					setChar(c);
					CrtTType = c == '_' && method_treatiden_underscore ?
						tok_identy : getctype(c);
				}
			}
			else if (CrtTType == getctype(c) || CrtTType == tok_any)
			{
				setChar(c);
				if (CrtTType == tok_any) CrtTType = getctype(c);
			}
			else {
				apd();
				crtline_ento = pos.y;
				crtcol_ento = pos.x - 1;
				setChar(c);
				CrtTType = getctype(c);
			}
		}
		if (linebuf->getByteCount()) apd();
		return dc;
	}

}

