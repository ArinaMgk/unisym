// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Trait) Iterate
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

// Store as array in heap
// avoid nodes and others, remove crt and then `crt->next` is invalid

#ifndef _INCPP_TRAIT_Iterate
#define _INCPP_TRAIT_Iterate
#include "../unisym"
namespace uni {
	class IterateTrait {
	protected:
		stduint _iterate_crt;
		stduint _iterate_lim;
		pureptr_t* _iterate_datas;
	public:
		IterateTrait() : _iterate_crt(~(stduint)0), _iterate_datas(nullptr) {}
		~IterateTrait() {
			if (_iterate_datas) delete[] _iterate_datas;
			_iterate_datas = nullptr;
		}
		virtual void Iterate() = 0;
		pureptr_t Iter() {
			return _iterate_crt >= _iterate_lim ? nullptr : _iterate_datas[_iterate_crt++];
		}
	};
}
#define fori(i,obj) (obj).Iterate();for(pureptr_t p_##i; p_##i=(obj).Iter();)
#define forhead(i,typ) Letvar(i,typ&,*(typ*)p_##i)

#endif // _INCPP_TRAIT_Iterate
