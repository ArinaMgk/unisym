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
#include "../atomic"
#include "../vector"
#include "../ISO_IEC_STD/memory"

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

	enum class StorageRequestState : byte { Created, Queued, Active, Completed };
	enum class StorageRequestStatus : byte { Success, Failed, TimedOut };
	struct StorageRequestResult {
		StorageRequestStatus status = StorageRequestStatus::Failed;
		stduint transferred = 0;
	};

	class StorageRequestQueue;
	// The host serializes state changes; readers acquire Completed before accessing the result or payload.
	class StorageRequest {
		friend class StorageRequestQueue;
		Atomic<byte> state{byte(StorageRequestState::Created)};
		StorageRequestResult completion{};
		StorageRequestQueue* owner = nullptr;
		uint32 started = 0, timeout_ms = 0;
		stduint origin = 0;
		bool pinned = false;
		void* completion_context = nullptr;
		void (*completed)(void*, stduint) = nullptr;
	public:
		StorageRequest() = default;
		virtual ~StorageRequest() = default;
		StorageRequest(const StorageRequest&) = delete;
		StorageRequest& operator=(const StorageRequest&) = delete;
		StorageRequestState getState() const { return StorageRequestState(state.load(MemoryOrder_Acquire)); }
		const StorageRequestResult& getCompletion() const { return completion; }
		stduint getOrigin() const { return origin; }
		bool IsDone() const { return getState() == StorageRequestState::Completed; }
		bool IsQueued() const { return getState() == StorageRequestState::Queued; }
		bool IsActive() const { return getState() == StorageRequestState::Active; }
		// Logical completion does not prove that hardware has stopped accessing request-owned resources.
		bool IsPinned() const { return pinned; }
		bool HasExpired(uint32 now) const;
		uint32 Remaining(uint32 now) const;
		bool Start(uint32 now);
	};
	using StorageRequestHandle = SharedPtr<StorageRequest>;

	/* The host serializes queue access and stops hardware before destroying the queue.
	   IRQ routing, scheduling, DMA allocation and protocol fields remain outside this layer. */
	class StorageRequestQueue {
		Vector<StorageRequestHandle> requests;
	public:
		StorageRequestQueue() = default;
		StorageRequestQueue(const StorageRequestQueue&) = delete;
		StorageRequestQueue& operator=(const StorageRequestQueue&) = delete;
		// origin is an opaque host token; completed only notifies and must not reenter or mutate this queue.
		bool Enqueue(const StorageRequestHandle& request, uint32 now, uint32 timeout_ms,
			stduint origin = 0, void* context = nullptr, void (*completed)(void*, stduint) = nullptr);
		bool Complete(StorageRequest& request, const StorageRequestResult& result, bool quiesced);
		void ExpireQueued(uint32 now);
		uint32 NextTimeout(uint32 now) const;
		bool HasExpired(uint32 now, bool active) const;
		stduint NextOrigin() const;
		void Prune();
		// quiesced must follow a confirmed stop/reset; otherwise active resources stay pinned even after failure.
		void FailAll(bool quiesced);
		stduint Count() const { return requests.Count(); }
		StorageRequestHandle getRequest(stduint index) const;
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
