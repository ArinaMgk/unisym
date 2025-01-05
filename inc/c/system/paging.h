// UTF-8 CPP-ISO11 TAB4 CRLF
// Docutitle: (System) Paging
// Codifiers: @dosconio: 20241209 ~ <Last-check> 
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

#include "../stdinc.h"

#if !defined(_INC_System_Paging) && defined(_INC_CPP)
#define _INC_System_Paging
// #include "../bitmap.h"

namespace uni {
#if defined(_ARC_x86)

// 0 .. 0x400
#define _NUM_pg_table_entries  (0x1000 / byteof(dword))
#define _NUM_pd_table_entries  (0x1000 / byteof(dword))

	struct Page {
		// return virtual linear address
		operator stduint() const {
			return _IMM(this) << 12;
		}
		//{} setMode ...
		//{} MapPhysical ...
	};

	// below are virtual entity

	extern void(*(*_physical_allocate)(stduint size));
	struct PageTable {
		Page& operator[](stduint pg_id) const {
			if (pg_id >= _NUM_pg_table_entries)
				return *(Page*)~_IMM0;
			return *(Page*)((_IMM(this) << 10) | pg_id);
		}
	};

	struct PageDirectory {
		PageTable& operator[](stduint pt_id) const {
			if (pt_id >= _NUM_pd_table_entries)
				return *(PageTable*)~_IMM0;
			return *(PageTable*)pt_id;
		}
	};

	struct Paging {
		PageDirectory* page_directory;
		// return phyical address
		void* operator[](stduint address) const {


			return (void*)0;
		}

		bool isMapped(stduint address) const {
			return false;//{TODO}
		}

		bool Map(stduint address, usize physical_address, bool writable) const{
			// assert !(address % 0x1000)
			// assert !(physical_address % 0x1000)
			_TODO
				return false;
		}

		void Reset();

	};


	
#endif

}

#endif
