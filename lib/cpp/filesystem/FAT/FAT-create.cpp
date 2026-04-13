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

#include "../../../../inc/c/format/filesys/FAT.h"
#include "../../../../inc/c/proctrl/x86/inst_x86_64.h"

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

		// 3. Resolve Flags (Ensure O_DIRECTORY is 0b1000 or your specific macro)
		bool create_as_dir = (flags & 0b1000) != 0;
		uint32_t new_cluster = 0;

		if (create_as_dir) {
			new_cluster = find_free_cluster();
			if (new_cluster == 0) {
				error_number = 3; // part is full
				return false;
			}
			uint32_t end_mark = (fat_type == 32) ? 0x0FFFFFFF :
				(fat_type == 16) ? 0xFFFF : 0xFFF;
			set_fat_entry(new_cluster, end_mark);

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

			// Setup ".." entry (Parent Directory)
			MemSet(dotdot->name, ' ', 11);
			dotdot->name[0] = '.'; dotdot->name[1] = '.';
			dotdot->attribute.attr = 0x10; // Directory attribute

			// In FAT12/16, if the parent is the root directory, the cluster number must be 0
			uint32_t pp_clus = (parent_cluster == root_cluster && fat_type != 32) ? 0 : parent_cluster;
			dotdot->cluster_low = (uint16_t)(pp_clus & 0xFFFF);
			dotdot->cluster_high = (uint16_t)(pp_clus >> 16);

			// Flush the initialization sector to disk
			storage->Write(getSector_foCluster(new_cluster), dir_init_buf);
			delete[] dir_init_buf;
			// ------------------------------------------------------------------
		}

		FAT_DirEntry entry;
		MemSet(&entry, 0, sizeof(FAT_DirEntry));
		MemSet(entry.name, ' ', 8);
		MemSet(entry.ext, ' ', 3);

		const char* dot_char = StrIndexCharRight(filename, '.');
		if (dot_char) {
			int name_len = dot_char - filename;
			int ext_len = StrLength(dot_char + 1);
			if (name_len > 8) name_len = 8;
			if (ext_len > 3) ext_len = 3;
			MemCopyN(entry.name, filename, name_len);
			MemCopyN(entry.ext, dot_char + 1, ext_len);
		}
		else {
			int name_len = StrLength(filename);
			if (name_len > 8) name_len = 8;
			MemCopyN(entry.name, filename, name_len);
		}

		for (int i = 0; i < 8; i++) entry.name[i] = ascii_toupper(entry.name[i]);
		for (int i = 0; i < 3; i++) entry.ext[i] = ascii_toupper(entry.ext[i]);

		entry.attribute.attr = create_as_dir ? 0x10 : 0x20;
		entry.cluster_high = (uint16_t)(new_cluster >> 16);
		entry.cluster_low = (uint16_t)(new_cluster & 0xFFFF);

		uint32_t entry_sector = 0;
		uint32_t entry_index = 0;
		bool written = false;

		auto try_sector = [&](uint32_t sector) -> bool {
			if (!storage->Read(sector, buffer_sector)) return false;
			for (int i = 0; i < storage->Block_Size / 32; i++) {
				FAT_DirEntry* dir_entry = (FAT_DirEntry*)&buffer_sector[i * 32];
				if (dir_entry->name[0] == 0 || (byte)dir_entry->name[0] == 0xE5u) {
					MemCopyN(dir_entry, &entry, sizeof(FAT_DirEntry));
					if (storage->Write(sector, buffer_sector)) {
						entry_sector = sector;
						entry_index = i;
						return true;
					}
				}
			}
			return false;
			};

		if ((parent_cluster == 0 || parent_cluster == root_cluster) && fat_type != 32) {
			// FAT12/16 Root Directory
			uint32_t start_sec = first_data_sector - root_dir_sectors;
			for (uint32_t s = 0; s < root_dir_sectors; s++) {
				if (try_sector(start_sec + s)) { written = true; break; }
			}
		}
		else {
			// FAT32 Root Directory or Any Subdirectory
			uint32_t cluster = parent_cluster;
			if (cluster == 0 && fat_type == 32) cluster = root_cluster;

			int limit = 10000;
			while (cluster >= 2 && cluster < 0xFFFFFFF8 && limit-- > 0) {
				uint32_t cluster_sec = getSector_foCluster(cluster);
				for (uint32_t s = 0; s < sectors_per_cluster; s++) {
					if (try_sector(cluster_sec + s)) { written = true; break; }
				}
				if (written) break;

				uint32_t next = get_fat_entry(cluster);
				if (next >= 0xFFFFFFF8) {
					uint32_t new_dir_cluster = find_free_cluster();
					if (new_dir_cluster == 0) break;

					uint32_t end_mark = (fat_type == 32) ? 0x0FFFFFFF :
						(fat_type == 16) ? 0xFFFF : 0xFFF;
					set_fat_entry(cluster, new_dir_cluster);
					set_fat_entry(new_dir_cluster, end_mark);

					uint32_t new_dir_sec = getSector_foCluster(new_dir_cluster);
					byte* clr_buf = new byte[storage->Block_Size];
					MemSet(clr_buf, 0, storage->Block_Size);
					for (uint32_t s = 0; s < sectors_per_cluster; s++) {
						storage->Write(new_dir_sec + s, clr_buf);
					}
					delete[] clr_buf;

					next = new_dir_cluster;
				}
				cluster = next;
			}
		}

		if (!written) {
			error_number = 4;
			return false;
		}

		// Output the new file handle back to VFS
		if (exinfo) {
			FAT_FileHandle* new_fh = new FAT_FileHandle();
			new_fh->start_cluster = new_cluster;
			new_fh->current_cluster = new_cluster;
			new_fh->current_offset = 0;
			new_fh->size = 0;
			new_fh->is_dir = create_as_dir;
			new_fh->dir_sector = entry_sector;
			new_fh->dir_index = entry_index;

			// Override the IN pointer with our newly created OUT pointer
			*(void**)exinfo = new_fh;
		}

		return true;
	}



}
