// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Trait) Stroage
// Codifiers: @dosconio: 20250104
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

// a Block default { 512 bytes }

#ifndef _INCPP_TRAIT_Storage
#define _INCPP_TRAIT_Storage
#include "../unisym"
namespace uni {
	class StorageTrait {
	public:
		stduint Block_Size;
		virtual bool Read(stduint BlockIden, void* Dest) = 0;
		virtual bool Write(stduint BlockIden, const void* Sors) = 0;
	   // bytes
		virtual int out(const char* str, dword len) = 0;
	};

}


#endif
