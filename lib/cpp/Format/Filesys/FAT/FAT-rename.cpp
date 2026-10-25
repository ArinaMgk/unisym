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
#include "../../../../../inc/c/proctrl/x86/inst_x86_64.h"

namespace uni {

	bool FilesysFAT::rename(rostr src_path, rostr dst_path) {
		if (!fs_loaded) return false;

		stduint bytes_per_sector = storage->Block_Size;
		byte* sector_buffer = buffer_sector;
		bool allocated_sector_buffer = false;
		FAT_TableScratch fat_table_cache = {};
		FAT_TableScratch* fat_cache = nullptr;
		if (allow_allocate) {
			sector_buffer = new byte[bytes_per_sector];
			if (!sector_buffer) return false;
			allocated_sector_buffer = true;

			fat_table_cache.buffer = new byte[bytes_per_sector];
			if (!fat_table_cache.buffer) {
				delete[] sector_buffer;
				return false;
			}
			fat_table_cache.current_sector = 0xFFFFFFFF;
			fat_cache = &fat_table_cache;
		}
		if (!sector_buffer) return false;
		auto finish_rename = [&](bool ret) -> bool {
			if (fat_cache) delete[] fat_table_cache.buffer;
			if (allocated_sector_buffer) delete[] sector_buffer;
			return ret;
		};

		// Split src_path into parent path and filename
		char src_parent_path[256];
		const char* src_filename = StrIndexCharRight(src_path, '/');
		if (!src_filename) {
			StrCopy(src_parent_path, "/");
			src_filename = src_path;
		}
		else {
			stduint len = src_filename - src_path;
			if (len == 0) len = 1;
			MemCopyN(src_parent_path, src_path, len);
			src_parent_path[len] = '\0';
			src_filename++;
		}

		// Split dst_path into parent path and filename
		char dst_parent_path[256];
		const char* dst_filename = StrIndexCharRight(dst_path, '/');
		if (!dst_filename) {
			StrCopy(dst_parent_path, "/");
			dst_filename = dst_path;
		}
		else {
			stduint len = dst_filename - dst_path;
			if (len == 0) len = 1;
			MemCopyN(dst_parent_path, dst_path, len);
			dst_parent_path[len] = '\0';
			dst_filename++;
		}

		// Locate source parent handle and entry
		FAT_FileHandle src_parent_h;
		FilesysSearchArgs src_search_args = { &src_parent_h, nullptr, nullptr, nullptr };
		if (!this->search(src_parent_path, &src_search_args)) {
			StrCopy(src_parent_path, "/");
			if (!this->search(src_parent_path, &src_search_args)) return finish_rename(false);
		}

		if (!src_parent_h.is_dir) return finish_rename(false);

		FAT_DirEntry entry;
		uint32_t src_entry_sector = 0;
		uint32_t src_entry_index = 0;
		uint32_t src_lfn_sector = 0;
		uint32_t src_lfn_index = 0;
		stduint src_lfn_count = 0;
		if (!find_entry_in_dir(src_parent_h.start_cluster, src_filename, &entry, &src_entry_sector, &src_entry_index, sector_buffer, fat_cache, &src_lfn_sector, &src_lfn_index, &src_lfn_count)) {
			return finish_rename(false); // Source file not found
		}

		const bool is_same_dir = StrCompare(src_parent_path, dst_parent_path) == 0;
		uint32_t dst_parent_cluster = src_parent_h.start_cluster;
		if (!is_same_dir) {
			FAT_FileHandle dst_parent_h;
			FilesysSearchArgs dst_search_args = { &dst_parent_h, nullptr, nullptr, nullptr };
			if (!this->search(dst_parent_path, &dst_search_args)) {
				StrCopy(dst_parent_path, "/");
				if (!this->search(dst_parent_path, &dst_search_args)) return finish_rename(false);
			}
			if (!dst_parent_h.is_dir) return finish_rename(false);
			dst_parent_cluster = dst_parent_h.start_cluster;
			FAT_DirEntry existing_entry;
			if (find_entry_in_dir(dst_parent_cluster, dst_filename, &existing_entry, nullptr, nullptr, sector_buffer, fat_cache)) {
				return finish_rename(false); // Target already exists
			}
		}

		// Prepare new entry based on the source entry with the destination name
		FAT_DirEntry new_entry = entry;
		FAT_DirEntry shaped;
		bool needs_lfn = false;
		if (!build_short_entry(dst_filename, dst_parent_cluster, sector_buffer, fat_cache, shaped, needs_lfn)) {
			return finish_rename(false);
		}
		MemCopyN(new_entry.name, shaped.name, 8);
		MemCopyN(new_entry.ext, shaped.ext, 3);

		constexpr stduint lfn_capacity = 20;
		stduint new_count = 0;
		byte* run = new byte[(lfn_capacity + 2) * 32];
		if (!run) return finish_rename(false);
		auto drop_run = [&]() {
			delete[] run;
			run = nullptr;
		};
		if (needs_lfn &&
			(!build_lfn_entries(dst_filename, new_entry, run, lfn_capacity, new_count) || !new_count)) {
			drop_run();
			return finish_rename(false);
		}
		MemCopyN(run + new_count * 32, &new_entry, 32);
		MemSet(run + (new_count + 1) * 32, 0, 32);

		const uint32_t old_block_sector = src_lfn_count ? src_lfn_sector : src_entry_sector;
		const uint32_t old_block_index = src_lfn_count ? src_lfn_index : src_entry_index;
		const stduint old_block_count = src_lfn_count + 1;

		if (is_same_dir && src_lfn_count >= new_count) {
			// In-place: clear the old block, then lay the new pieces and short entry over its front
			if (!erase_dir_run(src_parent_h.start_cluster, old_block_sector, old_block_index, old_block_count) ||
				!write_dir_run(src_parent_h.start_cluster, old_block_sector, old_block_index, run, new_count + 1)) {
				drop_run();
				return finish_rename(false);
			}
			drop_run();
			return finish_rename(true);
		}

		uint32_t written_sector = 0;
		uint32_t written_index = 0;
		if (!alloc_dir_run(dst_parent_cluster, new_count + 2, written_sector, written_index, sector_buffer, fat_cache) ||
			!write_dir_run(dst_parent_cluster, written_sector, written_index, run, new_count + 2)) {
			drop_run();
			return finish_rename(false);
		}
		drop_run();

		// If the entry is a directory, update its ".." pointer to the new parent
		if (new_entry.attribute.directory) {
			uint32_t dir_start_cluster = new_entry.getFirstClusterNumber();
			if (fat_type != 32) dir_start_cluster &= 0xFFFF;

			if (dir_start_cluster >= 2) {
				uint32_t dir_sector = getSector_foCluster(dir_start_cluster);
				if (storage->Read(dir_sector, sector_buffer)) {
					FAT_DirEntry* dotdot = (FAT_DirEntry*)&sector_buffer[32];
					if (dotdot->name[0] == '.' && dotdot->name[1] == '.') {
						uint32_t pp_clus = (dst_parent_cluster == root_cluster && fat_type != 32) ? 0 : dst_parent_cluster;
						dotdot->cluster_low = (uint16_t)(pp_clus & 0xFFFF);
						dotdot->cluster_high = (uint16_t)(pp_clus >> 16);
						storage->Write(dir_sector, sector_buffer);
					}
				}
			}
		}

		// Mark the whole source block, LFN pieces and short entry, as deleted
		if (!erase_dir_run(src_parent_h.start_cluster, old_block_sector, old_block_index, old_block_count)) {
			return finish_rename(false);
		}

		return finish_rename(true);
	}

}
