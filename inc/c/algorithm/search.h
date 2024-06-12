// ASCII C TAB4 CRLF
// Docutitle: (Algorithm) Search
// Codifiers: @dosconio: 20240609
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

#ifndef _INC_Search
#define _INC_Search


//STYLE G-3
#ifdef _INC_CPP
#include "../stdinc.h"
#include "../../cpp/trait/ArrayTrait.hpp"
namespace uni {
	namespace SearchMode {
		enum SearchMode {
			Sequence = 0,
			Binary
		};
	}

	inline static stduint SearchSequence(const ArrayTrait& arr, pureptr_t p_val) {
		int (*cmp)(pureptr_t a, pureptr_t b) = arr.Compare_f ? arr.Compare_f : _Local_Compare;
		for0 (i, arr.Length()) if (!cmp(p_val, arr.Locate(i)))
			return i;
		return ~(stduint)0;
	}

	//{unchk} preset(ordered)
	inline static stduint SearchBinary(const ArrayTrait& arr, pureptr_t p_val) {
		stduint l = 0, r = arr.Length() - 1;
		stduint m;
		int state;
		int (*cmp)(pureptr_t a, pureptr_t b) = arr.Compare_f ? arr.Compare_f : _Local_Compare;
		while (l < r) {
			m = l + ((r - l) >> 1);
			state = cmp(p_val, arr.Locate(m));
			if (!state) return m;
			else if (state > 0) l = ++m;
			else r = --m;// yo left part
		}
		return ~(stduint)0;
	}
	
	// ~0 for failure
	inline static stduint Search(const ArrayTrait& arr, pureptr_t p_val, SearchMode::SearchMode sm = SearchMode::Sequence) {
		stduint(*sear_f)(const ArrayTrait & arr, pureptr_t p_val) = nullptr;
		switch (sm) {
		case SearchMode::Sequence: sear_f = SearchSequence; break;
		case SearchMode::Binary: sear_f = SearchBinary; break;
		default: sear_f = nullptr; break;
		}
		return sear_f ? sear_f(arr, p_val) : ~(stduint)0;
	}
}
#endif

#endif
