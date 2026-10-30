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
#include "../trait/BlockTrait.hpp"
#include "../System/Filesys/Partition.hpp"

namespace uni {
	class StorageTrait : public BlockTrait {
	public:
		virtual ~StorageTrait() = default;
		void* Block_buffer;// for byte access
		
		// byte read. -1 for error
		virtual int operator[](uint64 bytid) = 0;

		// for storage device with Partitions
		virtual PartitionSlice getSlice(stduint dev) {
			PartitionSlice default_slice;
			default_slice.address = 0;
			default_slice.length = dev ? 0 : this->getUnits();
			default_slice.sys_id = ClassicFilesysType::None;
			return default_slice;
		}
	};

	class MemoryBlockDevice : public StorageTrait {
	protected:
		byte* address;
		stduint total_units;
	public:
		MemoryBlockDevice(Slice src, byte* block_buffer, stduint unit = 512) {
			Block_Size = unit;
			// Block_buffer = block_buffer;
			address = (byte*)src.address;
			total_units = src.length / unit;
		}
	public:
		using BlockTrait::Read;
		using BlockTrait::Write;

		virtual bool Read(stduint BlockIden, void* Dest, stduint Times = 1) override {
			if (BlockIden >= total_units || BlockIden + Times > total_units) return false;
			MemCopyN(Dest, address + BlockIden * Block_Size, Block_Size * Times);
			return true;
		}
		virtual bool Write(stduint BlockIden, const void* Sors, stduint Times = 1) override {
			if (BlockIden >= total_units || BlockIden + Times > total_units) return false;
			MemCopyN(address + BlockIden * Block_Size, Sors, Block_Size * Times);
			return true;
		}
		virtual stduint getUnits() override {
			return total_units;
		}
		virtual int operator[](uint64 bytid) override {
			if (bytid >= total_units * Block_Size) return -1;
			return address[bytid];
		}
	};

}


#endif
