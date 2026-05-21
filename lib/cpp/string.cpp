// ASCII C++11 TAB4 CRLF
// Attribute: ArnCovenant yo Free 
// LastCheck: RFX18
// AllAuthor: @dosconio, @ArinaMgk
// ModuTitle: Data / Text String
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

#define _LIB_STRING_HEAP
#include "../../inc/cpp/string"
#include "../../inc/cpp/trait/XstreamTrait.hpp"

namespace uni {
	String::String(const char* str) {
		allocated = str;
		if (allocated) {
			this->counts = StrLength(str);
			limits = counts + 1;
			this->addr = (char*)malc(limits);
			StrCopy(this->addr, str);
		}
		else {
			counts = 0;
			addr = NULL;
			limits = 0;
		}
	}
	String::String(const String& str) {
		if (!str.addr) {
			counts = 0;
			addr = NULL;
			limits = 0;
			allocated = false;
			charset = str.charset;
			inn_direction = str.inn_direction;
			return;
		}
		
		// Synchronize all states
		this->counts = str.counts;
		this->limits = str.counts + 1; // Compact allocation to save memory
		this->allocated = true;        // Newly copied object always owns its heap memory (deep copy)
		this->charset = str.charset;
		this->inn_direction = str.inn_direction;
		
		this->addr = (char*)malc(this->limits);
		
		// Physical copy (Use MemCopyN to avoid truncation by embedded null characters)
		if (this->counts > 0) {
			MemCopyN(this->addr, str.addr, this->counts);
		}
		if (this->charset != Charset::Memory) {
			this->addr[this->counts] = '\0';
		}
	}

	String::String(char* str, stduint buffer_siz) {
		this->addr = str;
		this->counts = StrLength(this->addr);
		limits = buffer_siz;
		allocated = nil == buffer_siz;
	}
	String::String(stduint buflen) {
		this->addr = (char*)zalc(buflen);
		this->counts = 0;
		limits = buflen;
		allocated = true;
	}

	String::String(String&& str) noexcept {
		this->addr = str.addr;
		this->counts = str.counts;
		this->limits = str.limits;
		this->allocated = str.allocated;
		this->charset = str.charset;
		this->inn_direction = str.inn_direction;

		str.addr = nullptr;
		str.counts = 0;
		str.limits = 0;
		str.allocated = false;
	}

	String& String::operator=(String&& str) noexcept {
		if (this == &str) return self;

		if (this->allocated && this->addr != nullptr) {
			memf(this->addr);
		}

		this->addr = str.addr;
		this->counts = str.counts;
		this->limits = str.limits;
		this->allocated = str.allocated;
		this->charset = str.charset;
		this->inn_direction = str.inn_direction;

		str.addr = nullptr;
		str.counts = 0;
		str.limits = 0;
		str.allocated = false;

		#if defined(_USE_PROPERTY)
		this->setthen(this);
		#endif

		return self;
	}

	String::String(Charset charset, rostr str) {
		allocated = str;
		if (allocated) {
			this->counts = StrLength(str);
			this->limits = counts + 1;
			this->addr = (char*)malc(limits);
			StrCopy(this->addr, str);
		}
		else {
			this->counts = 0;
			this->limits = 0;
			this->allocated = false;
		}
		this->charset = charset;
	}

// Construct from a UTF-32 string literal (C++11 U"...")
	String::String(const char32_t* str_utf32) {
		this->allocated = true;
		this->charset = Charset::UTF8;

		if (!str_utf32) {
			this->addr = nullptr;
			this->counts = 0;
			this->limits = 0;
			this->allocated = false;
			return;
		}

		// Step 1: Calculate the exact number of bytes required for UTF-8
		stduint utf8_bytes = 0;
		for (const char32_t* p = str_utf32; *p; ++p) {
			Rune cp = *p;
			if (cp <= 0x7F) utf8_bytes += 1;
			else if (cp <= 0x7FF) utf8_bytes += 2;
			else if (cp <= 0xFFFF) utf8_bytes += 3;
			else if (cp <= 0x10FFFF) utf8_bytes += 4;
			else utf8_bytes += 3; // Invalid char, will be replaced by U+FFFD (3 bytes)
		}

		// Step 2: Allocate memory
		this->counts = utf8_bytes;
		this->limits = utf8_bytes + 1;
		this->addr = (char*)malc(this->limits);

		// Step 3: Encode each Code Point into UTF-8 bytes
		stduint offset = 0;
		for (const char32_t* p = str_utf32; *p; ++p) {
			Rune cp = *p;
			if (cp <= 0x7F) {
				// 1-byte encoding (0xxxxxxx)
				addr[offset++] = static_cast<char>(cp);
			}
			else if (cp <= 0x7FF) {
				// 2-byte encoding (110xxxxx 10xxxxxx)
				addr[offset++] = static_cast<char>(0xC0 | ((cp >> 6) & 0x1F));
				addr[offset++] = static_cast<char>(0x80 | (cp & 0x3F));
			}
			else if (cp <= 0xFFFF) {
				// 3-byte encoding (1110xxxx 10xxxxxx 10xxxxxx)
				addr[offset++] = static_cast<char>(0xE0 | ((cp >> 12) & 0x0F));
				addr[offset++] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
				addr[offset++] = static_cast<char>(0x80 | (cp & 0x3F));
			}
			else if (cp <= 0x10FFFF) {
				// 4-byte encoding (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
				addr[offset++] = static_cast<char>(0xF0 | ((cp >> 18) & 0x07));
				addr[offset++] = static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
				addr[offset++] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
				addr[offset++] = static_cast<char>(0x80 | (cp & 0x3F));
			}
			else {
				// Replace invalid Code Point with U+FFFD (EF BF BD)
				addr[offset++] = static_cast<char>(0xEF);
				addr[offset++] = static_cast<char>(0xBF);
				addr[offset++] = static_cast<char>(0xBD);
			}
		}

		// Add null terminator
		addr[offset] = '\0';
	}

	// Construct from a single Unicode Code Point
	String::String(Rune cp) {
		this->allocated = true;
		this->charset = Charset::UTF8;

		// Maximum length for a single UTF-8 character is 4 bytes + 1 null terminator
		this->limits = 5;
		this->addr = (char*)malc(this->limits);

		stduint offset = 0;
		if (cp <= 0x7F) {
			addr[offset++] = static_cast<char>(cp);
		}
		else if (cp <= 0x7FF) {
			addr[offset++] = static_cast<char>(0xC0 | ((cp >> 6) & 0x1F));
			addr[offset++] = static_cast<char>(0x80 | (cp & 0x3F));
		}
		else if (cp <= 0xFFFF) {
			addr[offset++] = static_cast<char>(0xE0 | ((cp >> 12) & 0x0F));
			addr[offset++] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
			addr[offset++] = static_cast<char>(0x80 | (cp & 0x3F));
		}
		else if (cp <= 0x10FFFF) {
			addr[offset++] = static_cast<char>(0xF0 | ((cp >> 18) & 0x07));
			addr[offset++] = static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
			addr[offset++] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
			addr[offset++] = static_cast<char>(0x80 | (cp & 0x3F));
		}
		else {
			addr[offset++] = static_cast<char>(0xEF);
			addr[offset++] = static_cast<char>(0xBF);
			addr[offset++] = static_cast<char>(0xBD);
		}

		addr[offset] = '\0';
		this->counts = offset;
	}

	String::~String() {
		if (allocated) memf(this->addr);
		this->addr = NULL;
		this->counts = 0;
		limits = 0;
		allocated = false;
	}

	stduint String::getCharCount() const {
		if (!addr || !counts) return 0;
		stduint real_char_count = 0;
		switch (charset)
		{
		case Charset::Memory:
			return counts;
		case Charset::UTF8:
			for0 (i, counts) {
				// Cast to unsigned to prevent sign extension bugs with char
				unsigned char byte_val = static_cast<unsigned char>(addr[i]);
				// A UTF-8 continuation byte starts with binary '10' (0x80 to 0xBF).
				// Masking with 0xC0 (binary 11000000) extracts the top two bits.
				// If the result is NOT 0x80 (binary 10000000), it's a leading byte.
				if ((byte_val & 0xC0) != 0x80) {
					real_char_count++;
				}
			}
			return real_char_count;
		default:
			return counts;
		}
	}

	char* String::reflect(size_t plus) {
		switch (charset)
		{
		case Charset::UTF8:
			if (plus) {
				limits = counts + plus + 1;
				char* tmp = (char*)zalc(limits);
				StrCopy(tmp, addr);
				srs(addr, tmp);
				Refresh();
			}
			break;
		case Charset::Memory:
		default:
			break;
		}
		return this->addr;
	}

	bool String::available() {
		if (allocated) return true;
		switch (charset)
		{
		case Charset::UTF8:
			// each char measures 1 byte
			return limits - counts > getTermicharCount();
		case Charset::Memory:
			return limits > counts;
		default:
			//{TODO}
			return limits - counts > getTermicharCount();
		}
	}

	byte String::getTermicharCount() {
		return charset == Charset::UTF8 ? 1 : 0;
	}


	// USYM's special.
	String& String::operator=(const String& str) {
		if (this == &str) return self;
		
		// If source object is empty
		if (!str.addr) {
			if (allocated && this->addr) memf(this->addr);
			this->addr = nullptr;
			this->counts = 0;
			this->allocated = false;
			this->limits = 0;
			return self;
		}

		// Handle assignment across different formats
		// (Unified logic: rely on counts instead of StrLength)
		
		if (allocated) {
			// If target is heap-allocated and capacity is insufficient, reallocate
			if (this->limits < str.counts + 1) {
				if (this->addr) memf(this->addr);
				this->limits = str.counts + 1;
				this->addr = (char*)malc(this->limits);
			}
		} else {
			// If target is an Alias Buffer (allocated == false)
			if (this->limits < str.counts + 1) {
				// Buffer too small to fit new data, discard alias role and promote to heap allocation
				this->limits = str.counts + 1;
				this->addr = (char*)malc(this->limits);
				this->allocated = true; 
			}
			// If buffer is large enough, reuse it without changing allocated status
		}

		// Safe physical copy of memory data
		this->counts = str.counts;
		this->charset = str.charset;
		this->inn_direction = str.inn_direction;
		
		if (this->counts > 0) {
			MemCopyN(this->addr, str.addr, this->counts);
		}
		
		if (this->charset != Charset::Memory) {
			this->addr[this->counts] = '\0';
		}

		#if defined(_USE_PROPERTY)
		this->setthen(this);
		#endif
		
		return self;
	}
	
	
	String& String::operator=(const char* addr) {
		if (this->addr == addr) return self;
		switch (charset)
		{
		case Charset::UTF8:
			if (allocated) {
				srs(this->addr, StrHeap(addr));
				this->counts = StrLength(this->addr);
				this->limits = this->counts + 1;
			}
			else if (limits) {
				StrCopyN(this->addr, addr, this->limits);
			}
			else {
				this->addr = StrHeap(addr);
				this->counts = StrLength(this->addr);
				this->limits = this->counts + 1;
				this->allocated = true;
			}
		case Charset::Memory:
			//{TODO}
		default:
			//{TODO}
			break;
		}
		#if defined(_USE_PROPERTY)
		this->setthen(this);
		#endif
		return self;
	}

	String& String::operator<< (char chr) {
		// If limits > old.counts + 1, need not re-allocate
		// for Memory and ASCIZ
		if (allocated) {
			if (limits <= counts + 1) {
				char* tmp = (char*)zalc(Enlarge(counts + 1));
				StrCopy(tmp, this->addr);
				srs(addr, tmp);
				Refresh();
			}
			this->addr[counts] = chr;
			if (charset == Charset::UTF8)
				this->addr[counts + 1] = '\0';
			this->counts++;
		}
		else if (counts < limits) {
			this->addr[counts] = chr;
			this->counts++;
			if (counts != limits) this->addr[counts] = '\0';
		}
		return self;
	}

	String& String::Refresh(char* str) {
		stduint newlen;
		switch (charset)
		{
		case Charset::UTF8:
			newlen = StrLength(str);
			if (newlen + 1 > limits) {
				// realloc
				char* tmp = (char*)zalc(Enlarge(newlen + 1));
				StrCopy(tmp, str);
				srs(addr, tmp);
				Refresh();
			}
			break;
		case Charset::Memory:
			//{TODO}
			break;
		default:
			break;
		}
		return self;
	}


	//{} ASCIZ + mem
	int String::Format(const char* fmt, ...) {
		int ret = 0;
		Letpara(args, fmt);
		if (!allocated) ret = outsfmtlstbuf(addr, fmt, args);
		else {
			ret = this->counts = outsfmtlstlen(fmt, args);
			// ret = this->counts = OstreamTrait::CountFormat(fmt, args);
			limits = ret + 1;
			srs(addr, salc(limits));
			para_ento(args, fmt);
			outsfmtlstbuf(addr, fmt, args);
		}
		return ret;
	}
	//{} ASCIZ + mem
	String String::newFormat(const char* fmt, ...) {
		String ret("");
		Letpara(args, fmt);
		{
			ret.counts = outsfmtlstlen(fmt, args);
			para_endo(args);
			// ret.counts = OstreamTrait::CountFormat(fmt, args); -- C++ MSVC64 class static method -- paralist span a stduint, 20250329
			ret.limits = ret.counts + 1;
			srs(ret.addr, salc(ret.limits));
			para_ento(args, fmt);
			outsfmtlstbuf(ret.addr, fmt, args);
			para_endo(args);
		}
		return ret;
	}


	/*[Abandoned]
	//{TODO} uni::stream
	std::ostream& operator<< (std::ostream& out, const String& str) {
		return out << str.addr;
	}
	*/

	// ===================================================================
	// 1. String -> String32 (UTF-8 to UTF-32)
	// ===================================================================
	String32 String::toUTF32() const {
		stduint char_count = this->getCharCount();
		
		// Fix: Handle completely empty strings safely
		if (char_count == 0) {
			return String32(1); // Return an empty but valid String32
		}
		
		String32 fixed_str(char_count + 1);
		
		stduint index = 0;
		for (auto it = this->chars_begin(); it != this->chars_end(); ++it) {
			fixed_str.addr[index++] = *it;
		}
		
		fixed_str.addr[index] = 0;
		fixed_str.counts = index;
		
		return fixed_str;
	}

	// ===================================================================
	// 2. String32 -> String (UTF-32 to UTF-8)
	// ===================================================================
	String::String(const String32& str32) {
		this->allocated = true;
		this->charset = Charset::UTF8;

		if (!str32.reference() || !str32.length()) {
			this->addr = nullptr;
			this->counts = 0;
			this->limits = 0;
			this->allocated = false;
			return;
		}

		// Step 1: Calculate the exact number of bytes required for UTF-8
		stduint utf8_bytes = 0;
		for (stduint i = 0; i < str32.length(); ++i) {
			Rune cp = str32[i]; // O(1) direct access
			if (cp <= 0x7F) utf8_bytes += 1;
			else if (cp <= 0x7FF) utf8_bytes += 2;
			else if (cp <= 0xFFFF) utf8_bytes += 3;
			else if (cp <= 0x10FFFF) utf8_bytes += 4;
			else utf8_bytes += 3; // Invalid char, U+FFFD
		}

		// Step 2: Allocate memory for UTF-8 bytes
		this->counts = utf8_bytes;
		this->limits = utf8_bytes + 1;
		this->addr = (char*)malc(this->limits);

		// Step 3: Encode each UTF-32 Code Point back into UTF-8
		stduint offset = 0;
		for (stduint i = 0; i < str32.length(); ++i) {
			Rune cp = str32[i];
			if (cp <= 0x7F) {
				addr[offset++] = static_cast<char>(cp);
			}
			else if (cp <= 0x7FF) {
				addr[offset++] = static_cast<char>(0xC0 | ((cp >> 6) & 0x1F));
				addr[offset++] = static_cast<char>(0x80 | (cp & 0x3F));
			}
			else if (cp <= 0xFFFF) {
				addr[offset++] = static_cast<char>(0xE0 | ((cp >> 12) & 0x0F));
				addr[offset++] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
				addr[offset++] = static_cast<char>(0x80 | (cp & 0x3F));
			}
			else if (cp <= 0x10FFFF) {
				addr[offset++] = static_cast<char>(0xF0 | ((cp >> 18) & 0x07));
				addr[offset++] = static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
				addr[offset++] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
				addr[offset++] = static_cast<char>(0x80 | (cp & 0x3F));
			}
			else {
				// Replace invalid Code Point with U+FFFD (EF BF BD)
				addr[offset++] = static_cast<char>(0xEF);
				addr[offset++] = static_cast<char>(0xBF);
				addr[offset++] = static_cast<char>(0xBD);
			}
		}

		// Null terminate the UTF-8 string
		addr[offset] = '\0';
	}
}


const char* StrIndexString_KMP(const char* dest, const char* sub)
{
	stduint sublen = StrLength(sub);
	if (!*dest || !sublen) return NULL;
	uni::String str_next((sublen + 1) * byteof(stduint)); auto pnext = str_next.reflect();// or Letvar
	stduint crt_pref = 0, las_preflen = 0, p = 0;
	bool good = false;
	for (stduint i = 1; i < sublen; (good || !las_preflen) ? (pnext[1 + i++] = crt_pref) : 0)
		crt_pref = (good = sub[las_preflen = crt_pref] == sub[i]) ? (crt_pref + 1) : pnext[crt_pref];
	// if (1) { puts(""); for0(i, sublen + 1) printf("%d ", pnext[i]); puts(""); }
	do dest += (good = *dest == sub[p]) || !p; while (sub[p = good ? p + 1 : pnext[p]] && *dest);
	return sub[p] ? NULL : dest - sublen;
}
/*20250403
	// TEST
	// ABABABCAA
	// ABABC
	// 00120 [next]
	// 00012 [AzsNext]
	// stduint next[] = { _Comment(Occupy) nil, _Comment(Occupy) nil, 0,1,2 };
	////
	for (stduint i = 1; i < sublen;) {
		if (sub[crt_pref] == sub[i]) {
			crt_pref++;
			pnext[1 + i] = crt_pref;
		}
		else if (crt_pref) {
			crt_pref = pnext[crt_pref];// pnext[crt_pref - 1];
			continue;
		}
		else { // crt_pref=0, and pnext[0] == 0
			pnext[1 + i] = 0;
		}
		i++;
	}
	////
	if (!*dest || !sublen) return NULL;
	do {
		bool good = *dest == sub[p];
		dest += (!p || good);
		p = good ? p + 1 : next[p];
	} while (sub[p] && *dest);
	////
	Console.OutFormat("I find substr at %s\n", StrIndexString_KMP("ABABABCAA", "AB"));
	Console.OutFormat("I find substr at %s\n", StrIndexString_KMP("ABABABCAA", "ABABZ"));
	Console.OutFormat("I find substr at %s\n", StrIndexString_KMP("ABABABCAA", "ABABC"));
	Console.OutFormat("I find substr at %s\n", StrIndexString_KMP("ABABABCAA", "BCAA"));
	Console.OutFormat("I find substr at %s\n", StrIndexString_KMP("ABABABCAA", "ABCDABCABC"));
	Console.OutFormat("I find substr at %s\n", StrIndexString_KMP("ABABABCAAAAAA", "ABACABAB"));// 000101232
*/

