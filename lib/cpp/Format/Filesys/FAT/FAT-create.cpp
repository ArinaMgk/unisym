// ASCII C TAB4 CRLF
// Docutitle: (Format/FileSystem) FAT12 
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
#include "../../../../../inc/c/API-POSIX/fcntl.h"

namespace uni {

	bool FilesysFAT::create(rostr fullpath, stduint flags, void* exinfo, rostr linkdest) {
		if (!fs_loaded) return false;

		// Extract pure filename (Isolate from VFS absolute path)
		const char* filename = StrIndexCharRight(fullpath, '/');
		if (!filename) filename = fullpath;
		else filename++; // skip the slash

		// Retrieve Parent Cluster from VFS IN/OUT pointer
		FAT_FileHandle* parent_h = nullptr;
		if (exinfo) {
			// Read the dereferenced void** as the parent handle
			parent_h = (FAT_FileHandle*)(*(void**)exinfo);
		}
		// If VFS passed a valid parent, use its cluster. Otherwise fallback to root.
		uint32_t parent_cluster = parent_h ? parent_h->start_cluster : root_cluster;

		// 3. Resolve Flags
		bool create_as_dir = (flags & O_DIRECTORY) != 0;
		uint32_t new_cluster = 0;
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
		auto finish_create = [&](bool ret) -> bool {
			if (fat_cache) delete[] fat_table_cache.buffer;
			if (allocated_sector_buffer) delete[] sector_buffer;
			return ret;
		};

#ifdef _FILESYS_DATETIME
		uint16_t fat_date = 0;
		uint16_t fat_time = 0;
		uint8_t fat_hundredths = 0;
		bool has_fat_time = getFatDateTime(fat_date, fat_time, fat_hundredths);
#endif

		if (create_as_dir) {
			new_cluster = find_free_cluster(fat_cache);
			if (new_cluster == 0) {
				error_number = 3; // part is full
				return finish_create(false);
			}
			uint32_t end_mark = (fat_type == 32) ? 0x0FFFFFFF :
				(fat_type == 16) ? 0xFFFF : 0xFFF;
			set_fat_entry(new_cluster, end_mark, fat_cache);

			// --- [NEW LOGIC]: Initialize "." and ".." for the new directory ---
			byte* dir_init_buf = new byte[storage->Block_Size];
			MemSet(dir_init_buf, 0, storage->Block_Size);

			FAT_DirEntry* dot = (FAT_DirEntry*)&dir_init_buf[0];
			FAT_DirEntry* dotdot = (FAT_DirEntry*)&dir_init_buf[32];

			// Setup "." entry (Current Directory)
			MemSet(dot->name, ' ', 11);
			dot->name[0] = '.';
			dot->attribute.attr = 0x10; // Directory attribute
			dot->cluster_low = (uint16_t)(new_cluster & 0xFFFF);
			dot->cluster_high = (uint16_t)(new_cluster >> 16);
#ifdef _FILESYS_DATETIME
			if (has_fat_time) setFatCreationTime(*dot, fat_date, fat_time, fat_hundredths);
#endif

			// Setup ".." entry (Parent Directory)
			MemSet(dotdot->name, ' ', 11);
			dotdot->name[0] = '.'; dotdot->name[1] = '.';
			dotdot->attribute.attr = 0x10; // Directory attribute

			// In FAT12/16, if the parent is the root directory, the cluster number must be 0
			uint32_t pp_clus = (parent_cluster == root_cluster && fat_type != 32) ? 0 : parent_cluster;
			dotdot->cluster_low = (uint16_t)(pp_clus & 0xFFFF);
			dotdot->cluster_high = (uint16_t)(pp_clus >> 16);
#ifdef _FILESYS_DATETIME
			if (has_fat_time) setFatCreationTime(*dotdot, fat_date, fat_time, fat_hundredths);
#endif

			// Flush the initialization sector to disk
			storage->Write(getSector_foCluster(new_cluster), dir_init_buf);
			delete[] dir_init_buf;
			// ------------------------------------------------------------------
		}

		FAT_DirEntry entry;
		bool needs_lfn = false;
		if (!build_short_entry(filename, parent_cluster, sector_buffer, fat_cache, entry, needs_lfn)) {
			error_number = 4;
			return finish_create(false);
		}
		entry.attribute.attr = create_as_dir ? 0x10 : 0x20;
		entry.cluster_high = (uint16_t)(new_cluster >> 16);
		entry.cluster_low = (uint16_t)(new_cluster & 0xFFFF);
#ifdef _FILESYS_DATETIME
		if (has_fat_time) setFatCreationTime(entry, fat_date, fat_time, fat_hundredths);
#endif

		stduint lfn_count = 0;
		constexpr stduint lfn_capacity = 20;
		byte* run = new byte[(lfn_capacity + 2) * 32];
		if (!run) {
			error_number = 4;
			return finish_create(false);
		}
		if (needs_lfn &&
			(!build_lfn_entries(filename, entry, run, lfn_capacity, lfn_count) || !lfn_count)) {
			delete[] run;
			error_number = 4;
			return finish_create(false);
		}
		// LFN pieces first, then the short entry, then a zero terminator
		MemCopyN(run + lfn_count * 32, &entry, 32);
		MemSet(run + (lfn_count + 1) * 32, 0, 32);

		uint32_t run_sector = 0;
		uint32_t run_index = 0;
		if (!alloc_dir_run(parent_cluster, lfn_count + 2, run_sector, run_index, sector_buffer, fat_cache) ||
			!write_dir_run(parent_cluster, run_sector, run_index, run, lfn_count + 2)) {
			delete[] run;
			error_number = 4;
			return finish_create(false);
		}
		delete[] run;

		uint32_t entry_sector = 0;
		uint32_t entry_index = 0;
		if (!find_entry_in_dir(parent_cluster, filename, &entry, &entry_sector, &entry_index, sector_buffer, fat_cache)) {
			error_number = 4;
			return finish_create(false);
		}

		// Output the new file handle back to VFS
		if (exinfo) {
			FAT_FileHandle* new_fh = new FAT_FileHandle();
			new_fh->start_cluster = new_cluster;
			new_fh->current_cluster = new_cluster;
			new_fh->current_offset = 0;
			new_fh->size = 0;
			new_fh->is_dir = create_as_dir;
			new_fh->is_unlinked = false;
			new_fh->dir_sector = entry_sector;
			new_fh->dir_index = entry_index;

			// Override the IN pointer with our newly created OUT pointer
			*(void**)exinfo = new_fh;
		}

		return finish_create(true);
	}



}
