// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Stroage)
// Codifiers: @ArinaMgk
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

#include "../../../inc/cpp/unisym"
#include "../../../inc/cpp/string"
#include "../../../inc/cpp/trait/StorageTrait.hpp"
#include "../../../inc/cpp/System/Filesys.hpp"

#pragma GCC push_options
#pragma GCC optimize("O0")
#define DRV_OF_DEV
namespace uni {

	stduint BlockTrait::Read(stduint linear_offset, void* dest, stduint length, byte* buffer) {
		stduint block_size = Block_Size ? Block_Size : 512;
		stduint crt_block = linear_offset / block_size;
		stduint block_offset = linear_offset % block_size;
		byte* out = cast<byte*>(dest);
		stduint ret = 0;

		// Partial head block
		if (block_offset != 0 && length > 0) {
			if (!Read(crt_block, buffer, 1)) return ret;
			stduint copy_size = block_size - block_offset;
			if (copy_size > length) copy_size = length;
			MemCopyN(out, buffer + block_offset, copy_size);
			out += copy_size;
			ret += copy_size;
			length -= copy_size;
			crt_block++;
			block_offset = 0;
		}

		// Multi-block aligned body
		stduint full_blocks = length / block_size;
		if (full_blocks > 0) {
			if (!Read(crt_block, out, full_blocks)) return ret;
			stduint trans_bytes = full_blocks * block_size;
			out += trans_bytes;
			ret += trans_bytes;
			length -= trans_bytes;
			crt_block += full_blocks;
		}

		// Partial tail block
		if (length > 0) {
			if (!Read(crt_block, buffer, 1)) return ret;
			MemCopyN(out, buffer, length);
			ret += length;
		}

		return ret;
	}

	stduint BlockTrait::Write(stduint linear_offset, const void* src, stduint length, byte* buffer) {
		stduint block_size = Block_Size ? Block_Size : 512;
		stduint crt_block = linear_offset / block_size;
		stduint block_offset = linear_offset % block_size;
		const byte* in = cast<const byte*>(src);
		stduint ret = 0;

		// Partial head block (read-modify-write)
		if (block_offset != 0 && length > 0) {
			if (!Read(crt_block, buffer, 1)) return ret;
			stduint copy_size = block_size - block_offset;
			if (copy_size > length) copy_size = length;
			MemCopyN(buffer + block_offset, in, copy_size);
			if (!Write(crt_block, buffer, 1)) return ret;
			in += copy_size;
			ret += copy_size;
			length -= copy_size;
			crt_block++;
			block_offset = 0;
		}

		// Multi-block aligned body (direct multi-block write)
		stduint full_blocks = length / block_size;
		if (full_blocks > 0) {
			if (!Write(crt_block, in, full_blocks)) return ret;
			stduint trans_bytes = full_blocks * block_size;
			in += trans_bytes;
			ret += trans_bytes;
			length -= trans_bytes;
			crt_block += full_blocks;
		}

		// Partial tail block (read-modify-write)
		if (length > 0) {
			if (!Read(crt_block, buffer, 1)) return ret;
			MemCopyN(buffer, in, length);
			if (!Write(crt_block, buffer, 1)) return ret;
			ret += length;
		}

		return ret;
	}

	bool DiscPartition::Read(stduint BlockIden, void* Dest, stduint Times) {
		if (!slice.address && !slice.length) renew_slice();
		// ploginfo("DiscPartition::Read %u:%u -> %[x]", DRV_OF_DEV(self.device), BlockIden + slice.address, Dest);
		return base->Read(BlockIden + slice.address, Dest, Times);
	}

	bool DiscPartition::Write(stduint BlockIden, const void* Sors, stduint Times) {
		if (!slice.address && !slice.length) renew_slice();
		return base->Write(BlockIden + slice.address, Sors, Times);
	}

	void DiscPartition::Partition(StorageTrait& base, HD_Info& hdi, byte* psector, unsigned device, bool primary_but_logical)
	{
		unsigned drive = DRV_OF_DEV(device);
		// ploginfo("%s: drive%u", __FUNCIDEN__, drive);
		alignas(16) PartitionTableX86 part_tbl[4] = {};
		if (primary_but_logical) {
			if (LoadGPTPartitions(base, hdi, psector)) return;
			hdi.scheme_kind = PartitionSchemeKind::MBR;
			hdi.ResetUnifiedParts();
			// get_partition_table(drv, 0, part_tbl);
			base.Read(0, psector);
			MemCopyN(part_tbl, psector + MBR_PARTITION_TABLE_OFFSET, sizeof(part_tbl));
			for0(i, NR_PART_PER_DRIVE) {
				if (part_tbl[i].type == ClassicFilesysType::None)
					continue;
				PartitionSlice prim = {};
				prim.address = part_tbl[i].lba_start;
				prim.length = part_tbl[i].lba_count;
				prim.sys_id = part_tbl[i].type;
				if (part_tbl[i].type != ClassicFilesysType::EXT) {
					hdi.AppendPart(prim);
				}
				// plogwarn("p %u: %u-%u", i + 1, part_tbl[i].lba_start, part_tbl[i].lba_count);
				if (part_tbl[i].type == ClassicFilesysType::EXT)
					Partition(base, hdi, psector, prim.address, false);
			}
		}
		else {
			int ext_start_sect = device;
			int s = ext_start_sect;
			for0(i, NR_SUB_PER_PART) {
				// get_partition_table(drv, s, part_tbl);
				base.Read(s, psector);
				MemCopyN(part_tbl, psector + MBR_PARTITION_TABLE_OFFSET, sizeof(part_tbl));
				PartitionSlice logi = {};
				logi.address = s + part_tbl[0].lba_start;
				logi.length = part_tbl[0].lba_count;
				logi.sys_id = part_tbl[0].type;
				if (logi.sys_id != ClassicFilesysType::None && logi.length) {
					hdi.AppendPart(logi);
				}
				// plogwarn("l %u: %u-%u", dev_nr, logi.address, logi.address + logi.length);
				s = ext_start_sect + part_tbl[1].lba_start;
				if (part_tbl[1].type == ClassicFilesysType::None) {
					break;
				}
			}
		}
	}

}

#pragma GCC pop_options

namespace uni {

	bool StorageRequest::HasExpired(uint32 now) const {
		return (IsQueued() || IsActive()) && uint32(now - started) >= timeout_ms;
	}

	uint32 StorageRequest::Remaining(uint32 now) const {
		if (!IsQueued() && !IsActive()) return 0xFFFFFFFFu;
		const uint32 elapsed = now - started;
		return elapsed >= timeout_ms ? 1 : timeout_ms - elapsed;
	}

	bool StorageRequest::Start(uint32 now) {
		if (!owner || !IsQueued()) return false;
		if (HasExpired(now)) {
			owner->Complete(*this, {StorageRequestStatus::TimedOut, 0}, true);
			return false;
		}
		state.store(byte(StorageRequestState::Active), MemoryOrder_Release);
		return true;
	}

	bool StorageRequestQueue::Enqueue(const StorageRequestHandle& request, uint32 now, uint32 timeout_ms,
		stduint origin, void* context, void (*completed)(void*, stduint)) {
		if (!request || request->owner || request->getState() != StorageRequestState::Created ||
			!timeout_ms || timeout_ms > 0x7FFFFFFFu) return false;
		if (!requests.Append(request)) return false;
		// Publish ownership only after vector growth succeeds; rejected requests remain reusable and unowned.
		request->owner = this;
		request->started = now;
		request->timeout_ms = timeout_ms;
		request->origin = origin;
		request->completion_context = context;
		request->completed = completed;
		request->state.store(byte(StorageRequestState::Queued), MemoryOrder_Release);
		return true;
	}

	bool StorageRequestQueue::Complete(StorageRequest& request, const StorageRequestResult& result, bool quiesced) {
		if (request.owner != this || request.IsDone()) return false;
		request.pinned = request.IsActive() && !quiesced;
		request.completion = result;
		request.state.store(byte(StorageRequestState::Completed), MemoryOrder_Release);
		if (request.completed) request.completed(request.completion_context, request.origin);
		return true;
	}

	void StorageRequestQueue::ExpireQueued(uint32 now) {
		for (auto& request : requests) {
			if (request->IsQueued() && request->HasExpired(now)) {
				Complete(*request, {StorageRequestStatus::TimedOut, 0}, true);
			}
		}
	}

	uint32 StorageRequestQueue::NextTimeout(uint32 now) const {
		uint32 remaining = 0xFFFFFFFFu;
		for (const auto& request : requests) {
			const uint32 wait = request->Remaining(now);
			if (wait < remaining) remaining = wait;
		}
		return remaining;
	}

	bool StorageRequestQueue::HasExpired(uint32 now, bool active) const {
		for (const auto& request : requests) {
			if ((active ? request->IsActive() : request->IsQueued()) && request->HasExpired(now)) return true;
		}
		return false;
	}

	stduint StorageRequestQueue::NextOrigin() const {
		for (const auto& request : requests) {
			if (!request->IsDone()) return request->getOrigin();
		}
		return ~stduint(0);
	}

	void StorageRequestQueue::Prune() {
		for (stduint i = requests.Count(); i--;) {
			if (!requests[i]->IsDone() || requests[i]->IsPinned()) continue;
			requests[i]->owner = nullptr;
			requests.Remove(i);
		}
	}

	void StorageRequestQueue::FailAll(bool quiesced) {
		for (auto& request : requests) {
			// A later confirmed reset may also retire resources retained by an earlier failed reset.
			if (quiesced) request->pinned = false;
			if (!request->IsDone()) Complete(*request, {StorageRequestStatus::Failed, 0}, quiesced);
		}
		Prune();
	}

	StorageRequestHandle StorageRequestQueue::getRequest(stduint index) const {
		return index < requests.Count() ? requests.begin()[index] : StorageRequestHandle{};
	}
}

