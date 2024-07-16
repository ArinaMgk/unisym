// ASCII C/C++ TAB4 CRLF
// Docutitle: Node for Nested / Tree / Binary
// Codifiers: @dosconio: ~ 20240702
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

namespace uni
{

	class Mchain {
	protected:
		Dchain chn;
	public:
		Mnode() : chn() {}
		~Mnode() {}

	};


	
} // namespace uni

#endif
