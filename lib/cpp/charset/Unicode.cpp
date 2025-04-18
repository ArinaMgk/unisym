// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Charset) UTF-Series
// Codifiers: @dosconio: 20240422 ~ <Last-check> 
// Attribute: Endian(Current) Any-Architect Env-Freestanding Non-Dependence
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

#define _OPT_CHARSET_UTF8
#define _OPT_CHARSET_UTF16
#define _OPT_CHARSET_UTF32
#include "../../../inc/c/stdinc.h"
#include "../../../inc/c/widechar.h"
#include "../../../inc/c/multichar.h"
#include "../../../inc/cpp/string"

// Csc: Charset Conversion


static stduint hand_8to16(stduint& dlen, dword& v, dword& vmin, unit_utf16_t*& des) {
	if (v < vmin ||
		v > 0x10FFFF ||
		(v >= 0xD800 && v <= 0xDFFF)) {
		return NONE;
	}
	else if (v > 0xFFFF) {
		v -= 0x10000;
		asrtinc(des) = (v >> 10) | 0xD800;
		asrtinc(des) = (v & 0x3FF) | 0xDC00;
		dlen += 2 * byteof(unit_utf16_t);
	}
	else {
		asrtinc(des) = v;
		dlen += byteof(unit_utf16_t);
	}
	return 0;
}

static stduint hand_8to32(stduint& dlen, dword& v, dword& vmin, char_utf32_t*& des) {
	if (v < vmin || (v >= 0xD800 && v <= 0xDfff))
		return NONE;
	else {
		asrtinc(des) = v;
		dlen += byteof(char_utf32_t);
	}
	return 0;
}

static stduint CscFromUTF8(const byte* src, stduint slen, pureptr_t des, byte to) {
	typedef stduint(*fn_t)(stduint&, dword&, dword&, pureptr_t&);
	fn_t fn = nullptr;
	if (to == 16) fn = (fn_t)hand_8to16;
	else if (to == 32) fn = (fn_t)hand_8to32;
	if (!fn) return NONE;
	stduint dlen = 0;
	byte expected = 0;
	byte ch;
	dword v = 0, vmin = 0;
	unit_utf16_t*& dlen_16 = *(unit_utf16_t**)&des;
	char_utf32_t*& dlen_32 = *(char_utf32_t**)&des;
	for0(i, slen) {
		ch = *src++;
		if (expected) {
			if ((ch & 0xC0) == 0x80) {
				stduint state;
				v = (v << 6) | (ch & 0x3F);
				if (!--expected && (NONE == fn(dlen, v, vmin, des)))
					return NONE;
				continue;
			}
			else {
				expected = 0; // false;
				return NONE;
			}
		} // !expected below
		if (ch < 0x80) {
			if (to == 16) {
				asrtinc(dlen_16) = ch; dlen+= byteof(unit_utf16_t);
			}
			else if (to == 32) {
				asrtinc(dlen_32) = ch; dlen += byteof(char_utf32_t);
			}
		}
		else if (ch < 0xC0 || ch >= 0xFE) { return NONE; } // invalid UTF-8
		else if (ch < 0xE0) {
			v = ch & 0x1F;
			expected = 1;
			vmin = 0x80;
		}
		else if (ch < 0xF0) {
			v = ch & 0x0F;
			expected = 2;
			vmin = 0x800;
		}
		else if (ch < 0xF8) {
			v = ch & 0x07;
			expected = 3;
			vmin = 0x10000;
		}
		else if (ch < 0xFC) {
		    v = ch & 0x03;
			expected = 4;
			vmin = 0x200000;
		}
		else {
			v = ch & 0x01;
			expected = 5;
			vmin = 0x4000000;
		}
	}
	return expected ? NONE : dlen;
}

#define UNICHAR_LEN_CRTMAX 4

// Result in Heap
// No prefix here: BOM: {0xFF, 0xFE} for little endian, {0xFE, 0xFF} for big endian
stduint CscUTF(byte from, byte to, const pureptr_t src, stduint slen, pureptr_t* des) {
	if (!src || !des) return NONE;
	if (from == to) return slen;
	if (from == 8) {
		stduint res = CscFromUTF8((const byte*)src, slen, *des, to);
		if (res == NONE) return NONE;
		if (!*des) *des = zalc(res + UNICHAR_LEN_CRTMAX);
		MemSet(*des, nil, UNICHAR_LEN_CRTMAX);
		return CscFromUTF8((const byte*)src, slen, *des, to);
	}
	return NONE;
}

stduint ScanUTF8(byte* src) {
	if (!src) return NONE;
	if (*src < 0x80) return 1;
	else if (*src < 0xC0 || *src >= 0xFE) return NONE;
	else if (*src < 0xE0) return 2;// 110xxxxx 10xxxxxx
	else if (*src < 0xF0) return 3;// 1110xxxx 10xxxxxx 10xxxxxx
	else if (*src < 0xF8) return 4;// 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
	else if (*src < 0xFC) return 5;
	else if (*src < 0xFE) return 6;
	return NONE;
}
