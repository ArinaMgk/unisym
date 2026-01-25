// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Module) Bitmap, not a picture format
// Codifiers: @dosconio: 20240422 ~ <Last-check> 
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

#ifndef _INC_BITMAP
#define _INC_BITMAP
#include "stdinc.h"
#ifdef _INC_CPP

namespace uni {
	class Bitmap {
	public:
		pureptr_t offs;// align by byte
		stduint size;
	protected:
		bool alloc;
	public:
		Bitmap(pureptr_t offs, stduint size) : offs(offs), size(size), alloc(false) {}
		// cnts > 0
		Bitmap(stduint cnts) : size((cnts - _BYTE_BITS_ + 1) / _BYTE_BITS_), alloc(true) { offs = zalc(size); }
		~Bitmap() { if (alloc) mfree(offs); }
		bool bitof(stduint idx) const {
			const byte& byt = ((byte*)offs)[idx / _BYTE_BITS_];
			const stduint bitidx = idx % _BYTE_BITS_;
			return (byt >> bitidx) & 1;
		}
		bool setof(stduint idx, bool src = true) {
			byte& byt = ((byte*)offs)[idx / _BYTE_BITS_];
			const stduint bitidx = idx % _BYTE_BITS_;
			bool val = (byt >> bitidx) & 1;
			if (val ^ src) byt ^= 1 << bitidx;
			return val;
		}
		//
		bool operator[](stduint idx) const { return bitof(idx); }
	};

	struct BmMemoman : public Bitmap {
		stduint avail_pointer = ~_IMM0;// to the lowest available page
		BmMemoman(pureptr_t offs, stduint size) : Bitmap(offs, size) {}
		//
		// e.g. add_range(0x1, 0x2) for 0x1000 ~ 0x1FFF
		void add_range(stduint head_pos, stduint last_pos, bool what);
		//
		void dump_avail_memory();
	};
}

#else

//{TODO}:

typedef struct Bitmap_t {
	pureptr_t offs;
	stduint size;
	byte alloc : 1;
} Bitmap_t;

Bitmap_t* BitmapNew(stduint cnts);

#endif
#endif
