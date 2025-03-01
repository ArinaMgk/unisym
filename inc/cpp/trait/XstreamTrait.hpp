// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Trait) Stream
// Codifiers: @dosconio: 20240603
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

// for Console, File, XART(...)

#ifndef _INCPP_TRAIT_Stream
#define _INCPP_TRAIT_Stream
#include "../unisym"
#include "../string"
//included "../../c/consio.h"
namespace uni {
	class OstreamTrait {
	protected:
		// below: clear by OutFormat
		stduint out_count = 0;
		stduint inn_count = 0;
		bool count_mode = false;
		// bytes
		virtual int out(const char* str, stduint len) = 0;
	public:
		// `base`: `-16` for lower-case, else this should greater than 1
		// `sign_show`: whether show sign if sign_have is true
		// `sign_have`: whether it is signed number
		// `zero_padding`: pad with 0 or space
		// e.g. *(-123, 10, true, true, 8, true) => -00000123
		bool OutInteger(uint64 val, int base, bool sign_show = false,
			bool sign_have = true, byte least_digits = 0, bool zero_padding = false, byte bytexpo = 2);
		//{} OutCoearNumber
		//
		void OutFloating(double);
		inline void OutChar(char ch) { out(&ch, 1); }

	public:
		int OutFormat(const char* fmt, para_list plst);
		int OutFormat(const char* fmt, ...);
		stduint CountFormat(const char* fmt, ...);
		int operator<< (const Slice& slice) { return out((char*)slice.address, slice.length); }

	};
	class IstreamTrait {
		// byte
		virtual int inn() = 0;
	public:
		int InnFormat(const char* fmt, ...);
	};
}


#endif
