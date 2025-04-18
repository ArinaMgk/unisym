// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Trait) Array
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
#include "../unisym"
// ! Below are included by HEREPIC ! //

// Sort ArrayTrait Request Mutable
// - Chain
// - Dchain
// - Vector

#ifndef _INCPP_TRAIT_ARRAY
#define _INCPP_TRAIT_ARRAY
namespace uni {

class ArrayTrait {
public:
	virtual pureptr_t Locate(stduint idx) const = 0; //  operator[]

	// Index One. `~0' as not-found
	virtual stduint   Locate(pureptr_t p_val, bool fromRight) const = 0;

	virtual stduint   Length() const = 0;

	// After, the new item will be at idx.
	// For example, if you have {A,B}, then insert(2,x) will result in: {A,B,x}
	virtual bool      Insert(stduint idx, pureptr_t dat) = 0;

	// You may need destructure them before calling this.
	virtual bool      Remove(stduint idx, stduint times) = 0;
	
	virtual bool      Exchange(stduint idx1, stduint idx2) = 0;

	_tocomp_ft Compare_f;
	
	ArrayTrait() : Compare_f(nullptr) {}

};
}
#endif
