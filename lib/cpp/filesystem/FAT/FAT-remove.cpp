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

#include "../../../../inc/c/format/filesys/FAT.h"
#include "../../../../inc/c/proctrl/x86/inst_x86_64.h"

namespace uni {

	bool FilesysFAT::remove(rostr pathname) {
		if (!fs_loaded) return false;
		ploginfo("FATRemove: %s", pathname);
		FAT_DirEntry entry;
		uint32_t entry_sector, entry_index;

		// Split path to find parent directory
		char parent_path[256];
		const char* filename = StrIndexCharRight(pathname, '/');
		if (!filename) {
			StrCopy(parent_path, "/");
			filename = pathname;
		}
		else {
			stduint len = filename - pathname;
			if (len == 0) len = 1;
			MemCopyN(parent_path, pathname, len);
			parent_path[len] = '\0';
			filename++; // skip the slash
		}

		FAT_FileHandle parent_h;
		FilesysSearchArgs search_args = { &parent_h, nullptr, nullptr, nullptr };

		// Fallback to FAT root if VFS absolute path (e.g., /mnt) is not found internally
		if (!this->search(parent_path, &search_args)) {
			StrCopy(parent_path, "/");
			if (!this->search(parent_path, &search_args)) return false;
		}

		if (!parent_h.is_dir) return false;

		byte* sector_buffer = buffer_sector;
		bool allocated_sector_buffer = false;
		FAT_TableScratch fat_table_cache = {};
		FAT_TableScratch* fat_cache = nullptr;
		if (allow_allocate) {
			sector_buffer = new byte[storage->Block_Size];
			if (!sector_buffer) return false;
			allocated_sector_buffer = true;

			fat_table_cache.buffer = new byte[storage->Block_Size];
			if (!fat_table_cache.buffer) {
				delete[] sector_buffer;
				return false;
			}
			fat_table_cache.current_sector = 0xFFFFFFFF;
			fat_cache = &fat_table_cache;
		}
		if (!sector_buffer) return false;
		auto finish_remove = [&](bool ret) -> bool {
			if (fat_cache) delete[] fat_table_cache.buffer;
			if (allocated_sector_buffer) delete[] sector_buffer;
			return ret;
		};

		// Find the entry in the parent directory
		if (!find_entry_in_dir(parent_h.start_cluster, filename, &entry, &entry_sector, &entry_index, sector_buffer, fat_cache)) {
			return finish_remove(false);
		}

		// Free cluster chain if it's a file or non-empty directory
		uint32_t cluster = entry.getFirstClusterNumber();

		// In FAT12/16, cluster_high can contain garbage data
		if (this->fat_type != 32) cluster &= 0xFFFF;

		while (cluster >= 2 && cluster < 0xFFFFFFF8) {
			uint32_t next = get_fat_entry(cluster, fat_cache);
			set_fat_entry(cluster, 0, fat_cache); // Mark cluster as free
			cluster = next;
		}

		// Mark directory entry as deleted
		if (storage->Read(entry_sector, sector_buffer)) {
			FAT_DirEntry* d_entry = (FAT_DirEntry*)&sector_buffer[entry_index * 32];

			d_entry->name[0] = 0xE5;

			storage->Write(entry_sector, sector_buffer);
		}

		return finish_remove(true);
	}

}
