// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Stroage) Harddisk
// Codifiers: @dosconio: 20250107
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

#ifndef _INCPP_Storage_Harddisk
#define _INCPP_Storage_Harddisk

#include "../stdinc.h"

#ifdef _INC_CPP

#include "../../cpp/trait/StorageTrait.hpp"

namespace uni {
	class Harddisk_t : public StorageTrait {
	public:
		enum class HarddiskType {
			LBA28
		};
	public:
		// heritance
		// - stduint Block_Size;
		// - void* Block_buffer;
		HarddiskType type;
		Harddisk_t(HarddiskType type) : type(type) {}
		virtual bool Read(stduint BlockIden, void* Dest);
		virtual bool Write(stduint BlockIden, const void* Sors) { return _TODO false; }
		// byte read
		virtual byte operator[](uint64 bytid) { return _TODO 0; }

	};

}
#endif

#endif
