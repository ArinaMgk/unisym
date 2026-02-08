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

// - Harddisk
// - SDCard

#ifndef _INCPP_TRAIT_Storage
#define _INCPP_TRAIT_Storage
#include "../unisym"
#include "../string"
#include "../trait/BlockTrait.hpp"

// x86
_PACKED(struct) PartitionTableX86 {
	// boot indicator
	// bits 6-0 are zero: Bit 7 is the active partition flag
	// else: the drive number of the drive to boot so the active partition is always found on drive 80H, the first hdisk
	byte status;
	//
	byte head_start;
	word sector_start : 6;
	word cylinder_start : 10;
	byte type;// system id, {} can list an enum class
	byte head_end;
	word sector_end : 6;
	word cylinder_end : 10;
	uint32 lba_start;
	uint32 lba_count;
};//{TODO} details

// Part Type
#define Part_NO_PART 0x00
#define Part_EX_PART 0x05

#define NR_PART_PER_DRIVE    4 // 4 primary partitions per drive
#define NR_SUB_PER_PART     16 // 16 logical partitions per primary partition
#define NR_SUB_PER_DRIVE    (NR_SUB_PER_PART * NR_PART_PER_DRIVE)// 64
#define NR_PRIM_PER_DRIVE    (NR_PART_PER_DRIVE + 1) // 5
struct HD_Info {
    uni::Slice primary[NR_PRIM_PER_DRIVE];
    uni::Slice logical[NR_SUB_PER_DRIVE];
};

namespace uni {
	class StorageTrait : public BlockTrait {
	public:
		void* Block_buffer;// for byte access
		
		// byte read. -1 for error
		virtual int operator[](uint64 bytid) = 0;

	};

	struct DiscPartition : public StorageTrait
	{
		StorageTrait* base;
		stdsint device;
		stduint flag;
		Slice slice = {};
		//
		DiscPartition(StorageTrait& storage, int dev) : base(&storage), device(dev) { Block_Size = storage.Block_Size; }
		virtual bool Read(stduint BlockIden, void* Dest) override;
		virtual bool Write(stduint BlockIden, const void* Sors) override;
		virtual stduint getUnits() override {
			if (!slice.address && !slice.length) renew_slice();
			return slice.length;
		}
		Slice getSlice() {
			if (!slice.address && !slice.length) renew_slice();
			return slice;
		}
		virtual int operator[](uint64 bytid) override { _TODO return 0; }

		static void Partition(StorageTrait& base, HD_Info& hdi, byte* psector, unsigned device, bool primary_but_logical = true);
		//
	protected:
		void renew_slice();
	};


}


#endif
