// ASCII C TAB4 CRLF
// Docutitle: (Format/FileSystem) FAT Rename and Move
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Bit-32mode Non-Dependence
// Copyright: UNISYM, under Apache License 2.0; Dosconio Mecocoa, BSD 3-Clause License
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

#include "../../../../../inc/c/format/filesys/FAT.h"
#include "../../../../../inc/cpp/System/Filesys.hpp"
#include "../../../../../inc/c/proctrl/x86/inst_x86_64.h"

namespace uni {

	bool FilesysFAT::rename(rostr src_path, rostr dst_path, FilesysRenameParameter& parameter) {
		if (!fs_loaded || !storage || storage->Block_Size < 64 || storage->Block_Size % sizeof(FAT_DirEntry) ||
			!sectors_per_cluster || getFaultState() != FilesysFaultState::Healthy) return false;
		auto* source = static_cast<FAT_FileHandle*>(parameter.handler);
		auto* target = static_cast<FAT_FileHandle*>(parameter.replaced_handler);
		if (!source || source->is_unlinked || !source->dir_sector || (target && target->is_unlinked)) return false;
		Mutation transaction(*this);
		if (!transaction.IsReady()) return false;
		// Keep path, parent handles and entry snapshots off embedded service stacks.
		struct Scratch {
			char source_parent_path[256];
			char target_parent_path[256];
			FAT_FileHandle source_parent{};
			FAT_FileHandle target_parent{};
			FAT_DirEntry source_entry{};
			FAT_DirEntry target_entry{};
		};
		auto work = make_unique<Scratch>();
		if (!work) return false;
		UniquePtr<byte[]> sector_owner;
		UniquePtr<byte[]> fat_owner;
		if (allow_allocate) {
			sector_owner = make_unique<byte[]>(storage->Block_Size);
			fat_owner = make_unique<byte[]>(storage->Block_Size);
			if (!sector_owner || !fat_owner) return false;
		}
		byte* sector = allow_allocate ? sector_owner.get() : buffer_sector;
		FAT_TableScratch cache{};
		cache.buffer = allow_allocate ? fat_owner.get() : buffer_fatable;
		cache.current_sector = 0xFFFFFFFF;
		FAT_TableScratch* fat_cache = allow_allocate ? &cache : nullptr;
		if (!sector || !cache.buffer || sector == cache.buffer) return false;
		FilesysPathParts source_parts, target_parts;
		if (!SplitParentAndName(src_path, source_parts) || source_parts.trailing_separator ||
			!SplitParentAndName(dst_path, target_parts) || target_parts.trailing_separator ||
			source_parts.parent.length >= sizeof(work->source_parent_path) || target_parts.parent.length >= sizeof(work->target_parent_path)) return false;
		auto parent_path = [](const FilesysPathComponent& part, char* path) {
			if (!part.length) StrCopy(path, "/");
			else { MemCopyN(path, part.data, part.length); path[part.length] = 0; }
		};
		parent_path(source_parts.parent, work->source_parent_path);
		parent_path(target_parts.parent, work->target_parent_path);
		FilesysSearchArgs args = { &work->source_parent, nullptr, nullptr, nullptr };
		if (!search(work->source_parent_path, &args) || !work->source_parent.is_dir) return false;
		args.handle_buffer = &work->target_parent;
		if (!search(work->target_parent_path, &args) || !work->target_parent.is_dir) return false;
		const uint32_t source_parent = work->source_parent.start_cluster;
		const uint32_t target_parent = work->target_parent.start_cluster;
		const bool same_parent = source_parent == target_parent;
		uint32_t source_sector = 0, source_index = 0, lfn_sector = 0, lfn_index = 0;
		stduint lfn_count = 0;
		if (!find_entry_in_dir(source_parent, source_parts.name.data, &work->source_entry, &source_sector, &source_index, sector, fat_cache, &lfn_sector, &lfn_index, &lfn_count) || lfn_count > 20) return false;
		const uint32_t cluster_mask = fat_type == 32 ? 0x0FFFFFFFu : 0xFFFFu;
		auto matches = [&](const FAT_FileHandle& handle, const FAT_DirEntry& entry, uint32_t address, uint32_t index) {
			return !handle.is_unlinked && handle.dir_sector == address && handle.dir_index == index &&
				handle.is_dir == bool(entry.attribute.directory) && (handle.start_cluster & cluster_mask) == (entry.getFirstClusterNumber() & cluster_mask);
		};
		if (!matches(*source, work->source_entry, source_sector, source_index)) return false;
		uint32_t target_sector = 0, target_index = 0;
		bool lookup_complete = false;
		const bool exists = find_entry_in_dir(target_parent, target_parts.name.data, &work->target_entry, &target_sector, &target_index, sector, fat_cache, nullptr, nullptr, nullptr, &lookup_complete);
		if (!lookup_complete) return false;
		if (exists && source_sector == target_sector && source_index == target_index) return transaction.Finish(true);
		if (exists != bool(target)) return false;
		if (target) {
			// FAT has no hard links; distinct entries sharing a data start must not reclaim each other.
			if (source->start_cluster && (source->start_cluster & cluster_mask) == (target->start_cluster & cluster_mask)) return false;
			if (!matches(*target, work->target_entry, target_sector, target_index) || source->is_dir != target->is_dir) return false;
			if (target->is_dir && CheckDirectoryContents(target, sector, fat_cache) != FilesysDirectoryState::Empty) return false;
		}
		const uint32_t old_sector = lfn_count ? lfn_sector : source_sector;
		const uint32_t old_index = lfn_count ? lfn_index : source_index;
		uint32_t updated_sector = 0, updated_index = 0;
		auto finish = [&]() {
			if (!transaction.Finish(true)) return false;
			// Publish only namespace locations; open descriptors retain allocation, size and stream position.
			source->dir_sector = updated_sector;
			source->dir_index = updated_index;
			if (target) target->is_unlinked = true;
			return true;
		};
		// A directory's parent link belongs to the same change set as both namespace entries.
		if (source->is_dir && !same_parent) {
			if (!IsDataCluster(source->start_cluster) || !ReadBlock(getSector_foCluster(source->start_cluster), sector)) return false;
			auto* dotdot = reinterpret_cast<FAT_DirEntry*>(sector) + 1;
			if (dotdot->name[0] != '.' || dotdot->name[1] != '.' || !dotdot->attribute.directory) return false;
			const uint32_t parent = fat_type != 32 && target_parent == root_cluster ? 0 : target_parent;
			dotdot->cluster_low = uint16_t(parent);
			dotdot->cluster_high = uint16_t(parent >> 16);
			if (!WriteBlock(getSector_foCluster(source->start_cluster), sector)) return false;
		}
		if (target) {
			// Retain the target name and LFN checksum while replacing its allocation and metadata with the source's.
			FAT_DirEntry replacement = work->source_entry;
			MemCopyN(replacement.name, work->target_entry.name, sizeof(replacement.name));
			MemCopyN(replacement.ext, work->target_entry.ext, sizeof(replacement.ext));
			replacement.nt_reserved = work->target_entry.nt_reserved;
			if (!ReadBlock(target_sector, sector)) return false;
			auto* slot = reinterpret_cast<FAT_DirEntry*>(sector) + target_index;
			if (MemCompare(reinterpret_cast<const char*>(slot), reinterpret_cast<const char*>(&work->target_entry), sizeof(FAT_DirEntry))) return false;
			MemCopyN(slot, &replacement, sizeof(replacement));
			if (!WriteBlock(target_sector, sector) || !erase_dir_run(source_parent, old_sector, old_index, lfn_count + 1)) return false;
			updated_sector = target_sector;
			updated_index = target_index;
			return finish();
		}
		// An absent target retains the existing short-name and LFN construction path.
		FAT_DirEntry shaped;
		bool needs_lfn = false;
		if (!build_short_entry(target_parts.name.data, target_parent, sector, fat_cache, shaped, needs_lfn)) return false;
		MemCopyN(work->source_entry.name, shaped.name, sizeof(shaped.name));
		MemCopyN(work->source_entry.ext, shaped.ext, sizeof(shaped.ext));
		constexpr stduint capacity = 20;
		auto run = make_unique<byte[]>((capacity + 2) * sizeof(FAT_DirEntry));
		if (!run) return false;
		stduint count = 0;
		if (needs_lfn && (!build_lfn_entries(target_parts.name.data, work->source_entry, run.get(), capacity, count) || !count)) return false;
		MemCopyN(run.get() + count * sizeof(FAT_DirEntry), &work->source_entry, sizeof(FAT_DirEntry));
		auto locate_short = [&](uint32_t address, uint32_t index) {
			const stduint slots = storage->Block_Size / sizeof(FAT_DirEntry);
			stduint offset = index + count;
			while (offset >= slots) {
				address = dir_next_sector(target_parent, address, fat_cache);
				if (!address) return false;
				offset -= slots;
			}
			updated_sector = address;
			updated_index = uint32_t(offset);
			return true;
		};
		if (same_parent && lfn_count >= count) {
			if (!locate_short(old_sector, old_index) || !erase_dir_run(source_parent, old_sector, old_index, lfn_count + 1) ||
				!write_dir_run(target_parent, old_sector, old_index, run.get(), count + 1)) return false;
			return finish();
		}
		uint32_t address = 0, index = 0;
		if (!alloc_dir_run(target_parent, count + 2, address, index, sector, fat_cache) || !locate_short(address, index) ||
			!write_dir_run(target_parent, address, index, run.get(), count + 2) ||
			!erase_dir_run(source_parent, old_sector, old_index, lfn_count + 1)) return false;
		return finish();
	}

}
