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
		this->addr = StrHeap(str.addr);
		this->counts = StrLength(this->addr);
		limits = 0;
		allocated = true;
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
	String::String(Charset charset, rostr str) {
		allocated = str;
		if (allocated) {
			this->counts = StrLength(str);
			this->limits = counts + 1;
			this->addr = (char*)malc(limits);
			StrCopy(this->addr, str);
		}
		this->charset = charset;
	}

	String::~String() {
		if (allocated) memf(this->addr);
		this->addr = NULL;
		this->counts = 0;
		limits = 0;
		allocated = false;
	}

	stduint String::getCharCount() const {
		switch (charset)
		{
		case Charset::ASCIZ:
		case Charset::Memory:
		default:
			return counts;
		}
	}

	char* String::reflect(size_t plus) {
		switch (charset)
		{
		case Charset::ASCIZ:
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
		case Charset::ASCIZ:
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
		return charset == Charset::ASCIZ ? 1 : 0;
	}


	// USYM's special.
	String& String::operator=(const String& str) {
		if (this == &str) return self;
		switch (charset)
		{
		case Charset::ASCIZ:
			if (allocated) {
				srs(this->addr, StrHeap(str.addr));
				this->counts = StrLength(this->addr);
				this->limits = this->counts + 1;
			}
			else {
				StrCopyN(this->addr, str.addr, this->limits);
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
	
	
	String& String::operator=(const char* addr) {
		if (this->addr == addr) return self;
		switch (charset)
		{
		case Charset::ASCIZ:
			if (allocated) {
				srs(this->addr, StrHeap(addr));
				this->counts = StrLength(this->addr);
				this->limits = this->counts + 1;
			}
			else {
				StrCopyN(this->addr, addr, this->limits);
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
			if (charset == Charset::ASCIZ)
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
		case Charset::ASCIZ:
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
			// ret.counts = OstreamTrait::CountFormat(fmt, args); -- C++ MSVC64 class static method -- paralist span a stduint, 20250329
			ret.limits = ret.counts + 1;
			srs(ret.addr, salc(ret.limits));
			para_ento(args, fmt);
			outsfmtlstbuf(ret.addr, fmt, args);
		}
		return ret;
	}


	/*[Abandoned]
	//{TODO} uni::stream
	std::ostream& operator<< (std::ostream& out, const String& str) {
		return out << str.addr;
	}
	*/
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

