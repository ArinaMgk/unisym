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
		stduint _crt_out_cnt = 0;
		// bytes
		virtual int out(const char* str, stduint len) = 0;
		void OutInteger(uint64 val, ...) {}
		//
		void outfloat(double) {}
		void outi64(int64, int base, int sign_show) {}
		void outi8hex(int8) {}
		void outi16hex(int16) {}
		void outi32hex(int32) {}
		void outi64hex(int64) {}
		inline void OutChar(char ch) { out(&ch, 1); }
	
	public:
		int FormatOut(const char* fmt, ...);
		int operator<< (const Slice& slice) { return out((char*)slice.address, slice.length); }

	};
	class IstreamTrait {
		// byte
		virtual int inn() = 0;
	public:
		int FormatInn(const char* fmt, ...);
	};
}


#endif
