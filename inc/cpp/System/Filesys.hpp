// ASCII C/C++ TAB4 CRLF
// Docutitle: (Format) Filesys
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect <Environment> <Reference/Dependence>
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

#ifndef _INC_FILESYS
#define _INC_FILESYS

#include "../../cpp/trait/FilesysTrait.hpp"
#include "../../cpp/System/Filesys/Partition.hpp"
#include "../vector"
#include "../ISO_IEC_STD/memory"


class HD_Info;
namespace uni {

	// Views borrow the input path, are not necessarily terminated, and require the original string to remain alive.
	struct FilesysPathComponent {
		rostr data = nullptr;
		stduint length = 0;
	};

	struct FilesysPathParts {
		FilesysPathComponent parent;
		FilesysPathComponent name;
		bool absolute = false;
		bool trailing_separator = false;
	};

	// Emit one decoded entry and update logical pagination; the backend retains its physical cursor.
	bool EmitDirectoryEntry(FilesysEnumState* state, _tocall_ft callback, bool directory, rostr name);
	// Return a borrowed component, leaving dot names and backend naming rules uninterpreted.
	bool NextPathComponent(rostr& cursor, FilesysPathComponent& component);
	// Split the leaf from its parent; an empty parent view leaves the base choice to the caller.
	bool SplitParentAndName(rostr path, FilesysPathParts& result);


	enum class BlockCommitResult { Success, FailedRestored, FailedUncertain };

	// Callers serialize the entire preparation and commit; synchronous device calls must retire their I/O before returning.
	class BlockChangeSet {
		struct Change {
			stduint block = 0;
			stduint phase = 0;
			UniquePtr<byte[]> before;
			UniquePtr<byte[]> after;
		};
		BlockTrait& device;
		stduint block_size;
		stduint phase = 0;
		Vector<SharedPtr<Change>> changes;
		bool sealed = false;
		bool failed = false;
		BlockCommitResult result = BlockCommitResult::FailedRestored;
	public:
		explicit BlockChangeSet(BlockTrait& source) : device(source), block_size(source.Block_Size) {}
		BlockChangeSet(const BlockChangeSet&) = delete;
		BlockChangeSet& operator=(const BlockChangeSet&) = delete;
		// Preparation reads the latest staged image and never publishes bytes to the device.
		bool Read(stduint block, void* destination);
		// Edits within one phase share a stable buffer; callers stop editing it before advancing the phase.
		bool Edit(stduint block, byte*& image);
		// Preserve intermediate block images when the backend requires a later publication phase.
		bool NextPhase();
		// Stage a complete block and advance the phase to preserve backend write order.
		bool Write(stduint block, const void* source);
		// Commit once; failure restores every attempted block in reverse order without allocating memory.
		BlockCommitResult Commit();
	};

	#define	MBR_PARTITION_TABLE_OFFSET 0x1BE

	bool LoadGPTPartitions(StorageTrait& base, HD_Info& hdi, byte* block_buf);

	struct DiscPartition : public StorageTrait
	{
		using StorageTrait::getSlice;
		
		StorageTrait* base;
		stdsint device;
		stduint flag;
		PartitionSlice slice = {};
		//
		DiscPartition(StorageTrait& storage, int dev) : base(&storage), device(dev) { Block_Size = storage.Block_Size; }
		virtual bool Read(stduint BlockIden, void* Dest, stduint Times = 1) override;
		virtual bool Write(stduint BlockIden, const void* Sors, stduint Times = 1) override;
		virtual stduint getUnits() override {
			if (!slice.address && !slice.length) renew_slice();
			return slice.length;
		}
		PartitionSlice getSlice() {
			if (!slice.address && !slice.length) renew_slice();
			return slice;
		}
		virtual int operator[](uint64 bytid) override { _TODO return 0; }

		static void Partition(StorageTrait& base, HD_Info& hdi, byte* psector, unsigned device, bool primary_but_logical = true);
		//
	protected:
		void renew_slice() { if (base) self.slice = base->getSlice(self.device); }
	};


}

#endif
