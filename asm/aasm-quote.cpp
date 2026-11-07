// ASCII C TAB4 CRLF
// Docutitle: (Module) 
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

#include "inc/aasm.h"
#include "../inc/c/stdinc.h"
#include "../inc/c/multichar.h"
#include "../inc/cpp/parse.hpp"


char *aasm_quote(char *str, size_t len)
{
	char c, c1, *p, *q, *nstr, *ep;
	unsigned char uc;
	bool sq_ok, dq_ok;
	size_t qlen;

	sq_ok = dq_ok = true;
	ep = str+len;
	qlen = 0;			/* Length if we need `...` quotes */
	for (p = str; p < ep; p++) {
	c = *p;
	switch (c) {
	case '\'':
		sq_ok = false;
		qlen++;
		break;
	case '\"':
		dq_ok = false;
		qlen++;
		break;
	case '`':
	case '\\':
		qlen += 2;
		break;
	default:
		if (c < ' ' || c > '~') {
		sq_ok = dq_ok = false;
		switch (c) {
		case '\a':
		case '\b':
		case '\t':
		case '\n':
		case '\v':
		case '\f':
		case '\r':
		case 27:
			qlen += 2;
			break;
		default:
			c1 = (p+1 < ep) ? p[1] : 0;
			if (c1 >= '0' && c1 <= '7')
			uc = 0377; /* Must use the full form */
			else
			uc = c;
			if (uc > 077)
			qlen++;
			if (uc > 07)
			qlen++;
			qlen += 2;
			break;
		}
		} else {
		qlen++;
		}
		break;
	}
	}

	if (sq_ok || dq_ok) {
	/* Use '...' or "..." */
	nstr = (char*)malc(len+3);
	nstr[0] = nstr[len+1] = sq_ok ? '\'' : '\"';
	nstr[len+2] = '\0';
	MemCopyN(nstr+1, str, len);
	} else {
	/* Need to use `...` quoted syntax */
	nstr = (char*)malc(qlen+3);
	q = nstr;
	*q++ = '`';
	for (p = str; p < ep; p++) {
		c = *p;
		switch (c) {
		case '`':
		case '\\':
		*q++ = '\\';
		*q++ = c;
		break;
		case 7:
		*q++ = '\\';
		*q++ = 'a';
		break;
		case 8:
		*q++ = '\\';
		*q++ = 'b';
		break;
		case 9:
		*q++ = '\\';
		*q++ = 't';
		break;
		case 10:
		*q++ = '\\';
		*q++ = 'n';
		break;
		case 11:
		*q++ = '\\';
		*q++ = 'v';
		break;
		case 12:
		*q++ = '\\';
		*q++ = 'f';
		break;
		case 13:
		*q++ = '\\';
		*q++ = 'r';
		break;
		case 27:
		*q++ = '\\';
		*q++ = 'e';
		break;
		default:
		if (c < ' ' || c > '~') {
			c1 = (p+1 < ep) ? p[1] : 0;
			if (c1 >= '0' && c1 <= '7')
			uc = 0377; /* Must use the full form */
			else
			uc = c;
			*q++ = '\\';
			if (uc > 077)
			*q++ = ((unsigned char)c >> 6) + '0';
			if (uc > 07)
			*q++ = (((unsigned char)c >> 3) & 7) + '0';
			*q++ = ((unsigned char)c & 7) + '0';
			break;
		} else {
			*q++ = c;
		}
		break;
		}
	}
	*q++ = '`';
	*q++ = '\0';
	aasm_assert((size_t)(q-nstr) == qlen+3);
	}
	return nstr;
}

static bool append_utf8(char*& output, uint64 value)
{
	if (value > 0xFFFFFFFF) {
		aasm_log(_LOG_ERROR, "Unicode code point is out of range: U+%" PRIX64,
			value);
		return false;
	}
	uint32 codepoint = (uint32)value;
	pureptr_t destination = output;
	stduint length = CscUTF(32, 8, &codepoint,
		byteof(codepoint), &destination);
	if (length == NONE) {
		aasm_log(_LOG_ERROR, "invalid Unicode code point: U+%" PRIX64, value);
		return false;
	}
	output += length;
	return true;
}

/*
 * Do an *in-place* dequoting of the specified string, returning the
 * resulting length (which may be containing embedded nulls.)
 *
 * In-place replacement is possible since the unquoted length is always
 * shorter than or equal to the quoted length.
 *
 * *ep points to the final quote, or to the null if improperly quoted.
 */
size_t aasm_unquote(char* str, char** ep)
{
	char bq;
	char* p, * q;
	char c;

	p = q = str;

	bq = *p++;
	if (!bq)
		return 0;

	switch (bq) {
	case '\'':
	case '\"':
	/* '...' or "..." string */
		while ((c = *p) && c != bq) {
			p++;
			*q++ = c;
		}
		*q = '\0';
		break;

	case '`': {
	/* `...` string */
		static const uni::EscapeSequenceRules rules =
			uni::EscapeSequenceRules::AASM();
		const char* end = (const char*)StrIndexChar(p, '\0');
		while ((c = *p)) {
			p++;
			if (c == '`') {
				p--;
				break;
			}
			if (c != '\\') {
				*q++ = c;
				continue;
			}

			stduint remaining = end - p;
			uni::EscapeSequenceResult result =
				uni::DecodeEscapeSequence(p, remaining, rules);
			if (result.status != uni::EscapeSequenceStatus::Success) {
				if (result.status == uni::EscapeSequenceStatus::Invalid)
					aasm_log(_LOG_ERROR, "invalid escape sequence");
				break;
			}
			p += result.consumed;
			if (result.type == uni::EscapeSequenceType::Byte)
				*q++ = (char)result.value;
			else
				append_utf8(q, result.value);
		}
		break;
	}

	default:
	/* Not a quoted string, just return the input... */
		p = q = (char*)StrIndexChar(str, '\0');
		break;
	}

	if (ep)
		*ep = p;
	return q - str;
}

/*
 * Find the end of a quoted string; returns the pointer to the terminating
 * character (either the ending quote or the null character, if unterminated.)
 */
char* aasm_skip_string(char* str)
{
	char bq;
	char* p;
	char c;
	enum unq_state {
		st_start,
		st_backslash,
	} state;

	bq = str[0];
	if (bq == '\'' || bq == '\"') {
	/* '...' or "..." string */
		for (p = str + 1; *p && *p != bq; p++)
			;
		return p;
	}
	else if (bq == '`') {
		/* `...` string */
		p = str + 1;
		state = st_start;

		while ((c = *p++)) {
			switch (state) {
			case st_start:
				switch (c) {
				case '\\':
					state = st_backslash;
					break;
				case '`':
					return p - 1;	/* Found the end */
				default:
					break;
				}
				break;

			case st_backslash:
			/*
			 * Note: for the purpose of finding the end of the string,
			 * all successor states to st_backslash are functionally
			 * equivalent to st_start, since either a backslash or
			 * a backquote will force a return to the st_start state.
			 */
				state = st_start;
				break;
			}
		}
		return p;		/* Unterminated string... */
	}
	else {
		return str;		/* Not a string... */
	}
}

