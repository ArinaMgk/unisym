// ASCII C TAB4 CRLF
// Docutitle: (Format/FileSystem) FAT Deletion
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
#include "../../../../../inc/cpp/ISO_IEC_STD/memory"
#include "../../../../../inc/c/proctrl/x86/inst_x86_64.h"

namespace uni {

	bool FilesysFAT::remove(rostr pathname) {
		return unlink(pathname, nullptr);
	}

	bool FilesysFAT::unlink(rostr pathname, void* fil_handler) {
		if (!fs_loaded || getFaultState() != FilesysFaultState::Healthy) return false;
		Mutation transaction(*this);
		if (!transaction.IsReady()) return false;
		ploginfo("FATRemove: %s", pathname);
		FAT_DirEntry entry;
		uint32_t entry_sector, entry_index;

		// Split path to find parent directory
		char parent_path[256];
		FilesysPathParts path;
		if (!uni::SplitParentAndName(pathname, path) || path.trailing_separator || path.parent.length >= sizeof(parent_path)) return false;
		const char* filename = path.name.data;
		if (!path.parent.length) StrCopy(parent_path, "/");
		else {
			MemCopyN(parent_path, path.parent.data, path.parent.length);
			parent_path[path.parent.length] = '\0';
		}

		FAT_FileHandle parent_h;
		FilesysSearchArgs search_args = { &parent_h, nullptr, nullptr, nullptr };

		// A failed parent lookup must not redirect removal to an unrelated root entry.
		if (!this->search(parent_path, &search_args)) return false;

		if (!parent_h.is_dir) return false;

		FATBlockBuffer sector_owner(allow_allocate ? storage->Block_Size : 0);
		byte* sector_buffer = buffer_sector;
		FAT_TableScratch fat_table_cache = {};
		FAT_TableScratch* fat_cache = nullptr;
		FATBlockBuffer fat_owner(allow_allocate ? storage->Block_Size : 0);
		if (allow_allocate) {
			if (!sector_owner || !fat_owner) return false;
			sector_buffer = sector_owner.Data();
			fat_table_cache.buffer = fat_owner.Data();
			fat_table_cache.current_sector = 0xFFFFFFFF;
			fat_cache = &fat_table_cache;
		}
		if (!sector_buffer) return false;
		auto finish_remove = [&](bool ret) -> bool {
			ret = transaction.Finish(ret);
			return ret;
		};

		uint32_t lfn_sector = 0;
		uint32_t lfn_index = 0;
		stduint lfn_count = 0;
		// Find the entry in the parent directory together with the LFN pieces in front of it
		if (!find_entry_in_dir(parent_h.start_cluster, filename, &entry, &entry_sector, &entry_index, sector_buffer, fat_cache, &lfn_sector, &lfn_index, &lfn_count)) {
			return finish_remove(false);
		}
		if (entry.attribute.directory) return finish_remove(false);

		FAT_FileHandle* fh = (FAT_FileHandle*)fil_handler;
		if (fh && (fh->is_dir || fh->is_unlinked || fh->dir_sector != entry_sector ||
			fh->dir_index != entry_index)) return finish_remove(false);

		uint32_t cluster = entry.getFirstClusterNumber();
		if (this->fat_type != 32) cluster &= 0xFFFF;

		// Mark directory entry as deleted
		if (!ReadBlock(entry_sector, sector_buffer)) return finish_remove(false);
		FAT_DirEntry* d_entry = (FAT_DirEntry*)&sector_buffer[entry_index * 32];
		d_entry->name[0] = 0xE5;
		if (!WriteBlock(entry_sector, sector_buffer)) return finish_remove(false);
		if (lfn_count && !erase_dir_run(parent_h.start_cluster, lfn_sector, lfn_index, lfn_count)) return finish_remove(false);
		if (!finish_remove(true)) return false;
		if (fh) { fh->is_unlinked = true; return true; }
		// Namespace removal is committed; reclaim in bounded FAT-entry units without undoing unlink success.
		FAT_FileHandle retired{};
		retired.start_cluster = cluster;
		retired.is_unlinked = true;
		if (!reclaim(&retired)) plogwarn("FATRemove: allocation retained after unlink");
		return true;
	}

	// Read raw entries instead of display enumeration so hidden or malformed entries cannot disappear from the test.
	FilesysDirectoryState FilesysFAT::CheckDirectoryContents(void* handler, byte* sector, FAT_TableScratch* fat_cache) {
		using State = FilesysDirectoryState;
		auto* fh = static_cast<const FAT_FileHandle*>(handler);
		if (!fs_loaded || !storage || !storage->Block_Size || storage->Block_Size % sizeof(FAT_DirEntry) || !sectors_per_cluster || !fh || !fh->is_dir || fh->is_unlinked) return State::Invalid;
		const bool fixed_root = fat_type != 32 && !fh->start_cluster && !fh->dir_sector;
		const bool root = fixed_root || (fat_type == 32 && fh->start_cluster == root_cluster);
		if ((fixed_root && !root_dir_sectors) || (!fixed_root && !IsDataCluster(fh->start_cluster))) return State::Invalid;
		FATBlockBuffer sector_owner(!sector && allow_allocate ? storage->Block_Size : 0);
		FATBlockBuffer fat_owner(!sector && allow_allocate ? storage->Block_Size : 0);
		FAT_TableScratch local_cache{};
		if (!sector) {
			if (allow_allocate) {
				if (!sector_owner || !fat_owner) return State::IoError;
				local_cache.buffer = fat_owner.Data();
				local_cache.current_sector = 0xFFFFFFFF;
				fat_cache = &local_cache;
				sector = sector_owner.Data();
			}
			else sector = buffer_sector;
		}
		byte* fat_buffer = fat_cache ? fat_cache->buffer : buffer_fatable;
		if (!sector || !fat_buffer || sector == fat_buffer) return State::IoError;
		bool ended = false, pending_lfn = false;
		auto inspect_sector = [&](uint32_t address) -> State {
			if (!ReadBlock(address, sector)) return State::IoError;
			for (stduint index = 0; index < storage->Block_Size / sizeof(FAT_DirEntry); ++index) {
				auto* entry = reinterpret_cast<FAT_DirEntry*>(sector) + index;
				const byte first = byte(entry->name[0]);
				if (!first) { if (pending_lfn) return State::Invalid; ended = true; break; }
				if (first == 0xE5) { if (pending_lfn) return State::Invalid; continue; }
				if (entry->attribute.isLongName()) { pending_lfn = true; continue; }
				// Root volume labels are metadata, while unexpected labels in a child directory remain occupied.
				if (root && entry->attribute.volume_id && !entry->attribute.directory) {
					if (pending_lfn) return State::Invalid;
					continue;
				}
				const byte* raw_name = reinterpret_cast<const byte*>(entry);
				stduint dot_length = first == '.' ? (raw_name[1] == '.' ? 2 : 1) : 0;
				bool dot = dot_length && entry->attribute.directory && !entry->attribute.volume_id;
				for (stduint n = dot_length; dot && n < 11; ++n) dot = raw_name[n] == ' ';
				if (dot) { if (pending_lfn) return State::Invalid; continue; }
				return State::NotEmpty;
			}
			return State::Empty;
		};
		if (fixed_root) {
			for (uint32_t offset = 0; !ended && offset < root_dir_sectors; ++offset) {
				const auto result = inspect_sector(first_data_sector - root_dir_sectors + offset);
				if (result != State::Empty) return result;
			}
			return pending_lfn ? State::Invalid : State::Empty;
		}
		uint32_t cluster = fh->start_cluster;
		for (uint32_t remaining = total_clusters; remaining; --remaining) {
			if (!IsDataCluster(cluster)) return State::Invalid;
			for (uint32_t offset = 0; !ended && offset < sectors_per_cluster; ++offset) {
				const auto result = inspect_sector(getSector_foCluster(cluster) + offset);
				if (result != State::Empty) return result;
			}
			uint32_t next = 0;
			if (!ReadFatEntry(cluster, next, fat_cache)) return State::IoError;
			if (next == 0xFFFFFFF8u) return pending_lfn ? State::Invalid : State::Empty;
			cluster = next;
		}
		return State::Invalid;
	}

	// Unlink the verified empty directory while retaining its allocation until reclaim.
	FilesysDirectoryRemoveResult FilesysFAT::rmdir(rostr pathname, void* handler) {
		using Result = FilesysDirectoryRemoveResult;
		if (getFaultState() != FilesysFaultState::Healthy) return Result::IoError;
		auto* fh = static_cast<FAT_FileHandle*>(handler);
		if (!fs_loaded || !storage || !storage->Block_Size || storage->Block_Size % sizeof(FAT_DirEntry) || !sectors_per_cluster || !pathname || !fh || !fh->is_dir || fh->is_unlinked || !IsDataCluster(fh->start_cluster)) return Result::Invalid;
		if (fat_type == 32 && fh->start_cluster == root_cluster) return Result::Invalid;
		struct Scratch {
			char parent_path[256];
			FAT_FileHandle parent{};
			FAT_DirEntry entry{};
		};
		auto work = uni::make_unique<Scratch>();
		if (!work) return Result::IoError;
		FilesysPathParts path;
		if (!uni::SplitParentAndName(pathname, path) || path.trailing_separator || path.parent.length >= sizeof(work->parent_path)) return Result::Invalid;
		const char* name = path.name.data;
		if (!path.parent.length) StrCopy(work->parent_path, "/");
		else {
			MemCopyN(work->parent_path, path.parent.data, path.parent.length);
			work->parent_path[path.parent.length] = 0;
		}
		if (!*name || !StrCompare(name, ".") || !StrCompare(name, "..")) return Result::Invalid;
		FilesysSearchArgs args = { &work->parent, nullptr, nullptr, nullptr };
		if (!search(work->parent_path, &args) || !work->parent.is_dir) return Result::IoError;
		FATBlockBuffer sector_owner(allow_allocate ? storage->Block_Size : 0);
		FATBlockBuffer fat_owner(allow_allocate ? storage->Block_Size : 0);
		if (allow_allocate) {
			if (!sector_owner || !fat_owner) return Result::IoError;
		}
		byte* sector = allow_allocate ? sector_owner.Data() : buffer_sector;
		FAT_TableScratch cache{};
		cache.buffer = allow_allocate ? fat_owner.Data() : buffer_fatable;
		cache.current_sector = 0xFFFFFFFF;
		FAT_TableScratch* fat_cache = allow_allocate ? &cache : nullptr;
		if (!sector || !cache.buffer || sector == cache.buffer) return Result::IoError;
		uint32_t entry_sector = 0, entry_index = 0, lfn_sector = 0, lfn_index = 0;
		stduint lfn_count = 0;
		if (!find_entry_in_dir(work->parent.start_cluster, name, &work->entry, &entry_sector, &entry_index, sector, fat_cache, &lfn_sector, &lfn_index, &lfn_count)) return Result::IoError;
		const uint32_t mask = fat_type == 32 ? 0x0FFFFFFFu : 0xFFFFu;
		if (!work->entry.attribute.directory || entry_sector != fh->dir_sector || entry_index != fh->dir_index ||
			(work->entry.getFirstClusterNumber() & mask) != fh->start_cluster) return Result::Invalid;

		switch (CheckDirectoryContents(handler, sector, fat_cache)) {
		case FilesysDirectoryState::Empty: break;
		case FilesysDirectoryState::NotEmpty: return Result::NotEmpty;
		case FilesysDirectoryState::Unsupported: return Result::Unsupported;
		case FilesysDirectoryState::IoError: return Result::IoError;
		default: return Result::Invalid;
		}

		// Stage the short entry and its LFN run together before publishing handle state.
		Mutation transaction(*this);
		if (!transaction.IsReady()) return Result::IoError;
		if (!ReadBlock(entry_sector, sector)) return Result::IoError;
		auto* entry = reinterpret_cast<FAT_DirEntry*>(sector) + entry_index;
		if (MemCompare(reinterpret_cast<const char*>(entry), reinterpret_cast<const char*>(&work->entry), sizeof(FAT_DirEntry))) return Result::Invalid;
		entry->name[0] = char(0xE5);
		if (!WriteBlock(entry_sector, sector)) return Result::IoError;
		if (lfn_count && !erase_dir_run(work->parent.start_cluster, lfn_sector, lfn_index, lfn_count)) return Result::IoError;
		if (!transaction.Finish(true)) return Result::IoError;
		fh->is_unlinked = true;
		return Result::Success;
	}

	// Validate the complete chain before freeing any cluster; the handle records partial reclamation progress.
	bool FilesysFAT::reclaim(void* fil_handler) {
		if (!fs_loaded || getFaultState() != FilesysFaultState::Healthy) return false;
		auto* fh = static_cast<FAT_FileHandle*>(fil_handler);
		if (!fh || !fh->is_unlinked || (fh->is_dir && (!fh->start_cluster || (fat_type == 32 && fh->start_cluster == root_cluster)))) return false;
		FATBlockBuffer owner(allow_allocate ? storage->Block_Size : 0);
		if (allow_allocate && !owner) return false;
		FAT_TableScratch cache{};
		cache.buffer = allow_allocate ? owner.Data() : buffer_fatable;
		cache.current_sector = 0xFFFFFFFF;
		FAT_TableScratch* fat_cache = allow_allocate ? &cache : nullptr;
		if (!cache.buffer) return false;
		uint32_t cluster = fh->start_cluster;
		bool complete = cluster == 0;
		for (uint32_t remaining = total_clusters; !complete && remaining; --remaining) {
			if (!IsDataCluster(cluster)) return false;
			uint32_t next = 0;
			if (!ReadFatEntry(cluster, next, fat_cache)) return false;
			if (next == 0xFFFFFFF8u) complete = true;
			else cluster = next;
		}
		if (!complete) return false;
		cluster = fh->start_cluster;
		while (cluster) {
			uint32_t next = 0;
			if (!ReadFatEntry(cluster, next, fat_cache) || !set_fat_entry(cluster, 0, fat_cache)) return false;
			cluster = next == 0xFFFFFFF8u ? 0 : next;
			fh->start_cluster = cluster;
		}
		fh->current_cluster = 0;
		fh->current_offset = 0;
		fh->size = 0;
		fh->is_unlinked = false;
		return true;
	}

}
