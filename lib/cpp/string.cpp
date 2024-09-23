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
		this->counts = StrLength(str);
		this->addr = (char*)malc(counts + 1);
		StrCopy(this->addr, str);
		limits = 0;
	}
	String::String(const String& str) {
		this->addr = StrHeap(str.addr);
		this->counts = StrLength(this->addr);
		limits = 0;
	}
	
	String::String(char* str) {
		this->counts = StrLength(str);
		this->addr = str;
		//str = 0;
	}
	String::String(stduint buflen) {
		this->addr = (char*)zalc(buflen);
		this->counts = 0;
		limits = buflen;
	}

	String::~String() {
		memf(this->addr);
		this->addr = NULL;
		this->counts = 0;
	}

	size_t String::length() {
		return this->counts;
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
		this->setthen(this);
		return *this;
	}
	
	
	String& String::operator=(const char* addr) {
		if (this->addr != addr) {
			srs(this->addr, StrHeap(addr));
			this->counts = StrLength(this->addr);
		}
		this->setthen(this);
		return *this;
	}
	/*
	std::ostream& operator<< (std::ostream& out, const String& str) {
		return out << str.addr;
	}
	*/
}
