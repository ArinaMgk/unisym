// ASCII C/C++ TAB4 CRLF
// Docutitle: Map Node
// Codifiers: @dosconio: ~ 20240716
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
// Descripts: 20240630 new design: left of eldest child points to parent, aka this->subf==this->subf->left for parent, this->left->subf==this for the eldest.
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

#ifndef _INC_MNODE
#define _INC_MNODE

#include "stdinc.h"
#include "dnode.h"

//{TODO} for CPL
#ifdef _INC_CPP

namespace uni
{

	struct Mnode : public Dnode {
		pureptr_t operator= (pureptr_t val) { type = (stduint)val; }
	};
	
	class Mchain {
	protected:
		Dchain chn;
	public:
		_tocomp_ft func_comp;
		Mchain() : chn() { func_comp = nullptr; }
		~Mchain() {}
		
		// Auto Map
		Mnode& operator[] (pureptr_t key) { 
			Letvar(res, Mnode*, chn.LocateNode(key, func_comp));
			return *(res ? res : Map(key, nullptr));
		}

		Dchain& refChain() { return chn; }
		bool isExist(pureptr_t key, Dnode** val_ref = 0) {
			Dnode* res = chn.LocateNode(key, func_comp);
			asserv(val_ref)[0] = res;
			return res != nullptr;
		}
		Mnode* Map(pureptr_t key, pureptr_t val) {
			Dnode* val_node;
			if (isExist(key, &val_node)) {
				chn.Remove(val_node);
			}
			val_node = chn.Append(key, false);
			val_node->type = (stduint)val;
			return (Mnode*)val_node;
		}
		void unMap(pureptr_t key) {
			chn.Remove(chn.LocateNode(key, func_comp));
		}
	};


	
} // namespace uni

#endif

#endif
