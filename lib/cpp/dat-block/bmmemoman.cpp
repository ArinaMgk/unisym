// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Module) Bitmap Memory Manager
// Codifiers: ArinaMgk
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

#include "../../../inc/c/bitmap.h"
#include "../../../inc/c/consio.h"

using namespace uni;

#define page_cnt 0x100000

static void update_avail_pointer(BmMemoman& bm, stduint head_pos, stduint last_pos, bool what) {
	// ploginfo("update_avail_pointer(0x%[32H], 0x%[32H], %d)", head_pos, last_pos, what);
	if (what) {
		MIN(bm.avail_pointer, head_pos);
	}
	else if (bm.avail_pointer == head_pos) {
		bm.avail_pointer = last_pos;
		while (!bm.bitof(bm.avail_pointer)) {
			bm.avail_pointer++;
			if (bm.avail_pointer >= (bm.size * _BYTE_BITS_)) {
				plogerro("Memory is all used");
				return;
			}
		}
	}
}

void BmMemoman::add_range(stduint head_pos, stduint last_pos, bool what) {
	// ploginfo("add_range(0x%[32H], 0x%[32H])", head_pos, last_pos);
	if (head_pos >= last_pos || head_pos >= page_cnt || last_pos >= page_cnt) return;
	stduint times = last_pos - head_pos;
	stduint curr_pos = head_pos;
	while (times && (curr_pos & 0b111)) {
		this->setof(curr_pos, what);
		curr_pos++;
		times--;
	}
	while (times >= _BYTE_BITS_) {
		cast<byte*>(this->offs)[curr_pos / _BYTE_BITS_] = what ? 0xFF : 0x00;
		curr_pos += _BYTE_BITS_;
		times -= _BYTE_BITS_;
	}
	while (times) {
		this->setof(curr_pos, what);
		curr_pos++;
		times--;
	}
	update_avail_pointer(self, head_pos, last_pos, what);
}
void BmMemoman::dump_avail_memory()
{
	bool last_stat = false;
	stduint last_index = 0;
	outsfmt("[Memoman] dump avail memory:\n\r");
	for0(i, page_cnt) {
		bool b = this->bitof(i);
		if (b != last_stat) {
			if (!last_stat) {
				last_index = i;
				last_stat = 1;
			}
			else {
				outsfmt("- 0x%[x]..0x%[x] \n\r", last_index * 0x1000, i * 0x1000);
				last_stat = 0;
			}
		}
	}
	if (last_stat == 1) {
		outsfmt("- 0x%[x]..0x%[x] \n\r", last_index * 0x1000, 0x100000000ull);
	}
}
