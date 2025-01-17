// ASCII C++11 TAB4 CRLF
// Attribute: ArnCovenant yo Free 
// LastCheck: RFX18
// AllAuthor: @dosconio
// ModuTitle: ASCII String
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

namespace uni {
	String::String(const char* str) {
		allocated = str;
		if (allocated) {
			this->counts = StrLength(str);
			this->addr = (char*)malc(counts + 1);
			StrCopy(this->addr, str);
		}
		else {
			counts = 0;
			addr = NULL;
		}
		limits = 0;

	}
	String::String(const String& str) {
		this->addr = StrHeap(str.addr);
		this->counts = StrLength(this->addr);
		limits = 0;
		allocated = true;
	}

	String::String(char* str, stduint buffer_siz) {
		this->addr = str;
		this->counts = nil;
		limits = buffer_siz;
		allocated = nil == buffer_siz;
	}
	String::String(stduint buflen) {
		this->addr = (char*)zalc(buflen);
		this->counts = 0;
		limits = buflen;
		allocated = true;
	}

	String::~String() {
		if (allocated) memf(this->addr);
		this->addr = NULL;
		this->counts = 0;
		limits = 0;
		allocated = false;
	}

	size_t String::length() {
		if (!allocated) {
			Refresh();
			return counts;
		}
		else return counts = StrLength(addr);
	}

	char* String::reflect(size_t plus) {
		if (plus) {
			char* tmp = (char*)zalc(counts + plus + 1);
			StrCopy(tmp, addr);
			srs(addr, tmp);
			//ISSUE should not counts += plus;
		}
		return this->addr;
	}
	const char* String::reference() const {
		return this->addr;
	}

	// USYM's special.
	String& String::operator=(const String& str) {
		if (this != &str) {
			srs(this->addr, StrHeap(str.addr));
			this->counts = StrLength(this->addr);
		}
		#if !defined(_MCCA) && !defined(_PROPERTY_STRING_OFF)
		this->setthen(this);
		#endif
		return *this;
	}
	
	
	String& String::operator=(const char* addr) {
		if (this->addr != addr) {
			srs(this->addr, StrHeap(addr));
			this->counts = StrLength(this->addr);
		}
		#if !defined(_MCCA) && !defined(_PROPERTY_STRING_OFF)
		this->setthen(this);
		#endif
		return *this;
	}

	String& String::operator<< (char chr) {
		if (allocated) {
			srs(this->addr, StrHeapAppendChars(this->addr, chr, 1));
			this->counts = 0;
			//limits = limits;
		}
		else if (counts < limits) {
			this->addr[counts] = chr;
			this->counts++;
			if (counts != limits) this->addr[counts] = '\0';
		}
		return self;
	}


	int String::Format(const char* fmt, ...) {
		int ret = 0;
		Letpara(args, fmt);
		if (!allocated) ret = outsfmtlstbuf(addr, fmt, args);
		else {
			ret = this->counts = outsfmtlstlen(fmt, args);
			limits = ret + 1;
			srs(addr, salc(limits));
			outsfmtlstbuf(addr, fmt, args);
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
