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

#include "../../../../inc/c/format/filesys/FAT.h"
#include "../../../../inc/c/proctrl/x86/inst_x86_64.h"

namespace uni {

	bool FilesysFAT::rename(rostr src_path, rostr dst_path) {
		if (!fs_loaded) return false;

		stduint bytes_per_sector = storage->Block_Size;

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
			if (!this->search(src_parent_path, &src_search_args)) return false;
		}

		if (!src_parent_h.is_dir) return false;

		FAT_DirEntry entry;
		uint32_t src_entry_sector = 0;
		uint32_t src_entry_index = 0;
		if (!find_entry_in_dir(src_parent_h.start_cluster, src_filename, &entry, &src_entry_sector, &src_entry_index)) {
			return false; // Source file not found
		}

		// Format target filename into standard 8.3 format
		char formatted_name[8];
		char formatted_ext[3];
		MemSet(formatted_name, ' ', 8);
		MemSet(formatted_ext, ' ', 3);

		const char* dot_char = StrIndexCharRight(dst_filename, '.');
		if (dot_char) {
			int name_len = dot_char - dst_filename;
			int ext_len = StrLength(dot_char + 1);
			if (name_len > 8) name_len = 8;
			if (ext_len > 3) ext_len = 3;
			MemCopyN(formatted_name, dst_filename, name_len);
			MemCopyN(formatted_ext, dot_char + 1, ext_len);
		}
		else {
			int name_len = StrLength(dst_filename);
			if (name_len > 8) name_len = 8;
			MemCopyN(formatted_name, dst_filename, name_len);
		}

		for (int i = 0; i < 8; i++) formatted_name[i] = ascii_toupper(formatted_name[i]);
		for (int i = 0; i < 3; i++) formatted_ext[i] = ascii_toupper(formatted_ext[i]);

		// Compare parent paths to determine if it is a simple rename
		bool is_same_dir = StrCompare(src_parent_path, dst_parent_path) == 0;

		if (is_same_dir) {
			// Simply modify the directory entry's name and ext fields
			byte* sector_buf = new byte[bytes_per_sector];
			if (storage->Read(src_entry_sector, sector_buf)) {
				FAT_DirEntry* d_entry = (FAT_DirEntry*)&sector_buf[src_entry_index * 32];
				MemCopyN(d_entry->name, formatted_name, 8);
				MemCopyN(d_entry->ext, formatted_ext, 3);
				storage->Write(src_entry_sector, sector_buf);
			}
			delete[] sector_buf;
			return true;
		}

		// Cross-directory move: Locate target parent handle
		FAT_FileHandle dst_parent_h;
		FilesysSearchArgs dst_search_args = { &dst_parent_h, nullptr, nullptr, nullptr };
		if (!this->search(dst_parent_path, &dst_search_args)) {
			StrCopy(dst_parent_path, "/");
			if (!this->search(dst_parent_path, &dst_search_args)) return false;
		}

		if (!dst_parent_h.is_dir) return false;

		uint32_t dst_parent_cluster = dst_parent_h.start_cluster;

		// Check if destination directory already has an entry with the target name
		FAT_DirEntry existing_entry;
		if (find_entry_in_dir(dst_parent_cluster, dst_filename, &existing_entry, nullptr, nullptr)) {
			return false; // Target already exists
		}

		// Prepare new entry based on the source entry with new name
		FAT_DirEntry new_entry = entry;
		MemCopyN(new_entry.name, formatted_name, 8);
		MemCopyN(new_entry.ext, formatted_ext, 3);

		// Seek and write to an empty slot in destination parent directory
		bool written = false;
		uint32_t written_sector = 0;
		uint32_t written_index = 0;

		auto try_sector = [&](uint32_t sector) -> bool {
			if (!storage->Read(sector, buffer_sector)) return false;
			for (int i = 0; i < storage->Block_Size / 32; i++) {
				FAT_DirEntry* dir_entry = (FAT_DirEntry*)&buffer_sector[i * 32];
				if (dir_entry->name[0] == 0 || (byte)dir_entry->name[0] == 0xE5u) {
					MemCopyN(dir_entry, &new_entry, sizeof(FAT_DirEntry));
					if (storage->Write(sector, buffer_sector)) {
						written_sector = sector;
						written_index = i;
						return true;
					}
				}
			}
			return false;
		};

		if ((dst_parent_cluster == 0 || dst_parent_cluster == root_cluster) && fat_type != 32) {
			// FAT12/16 Root Directory
			uint32_t start_sec = first_data_sector - root_dir_sectors;
			for (uint32_t s = 0; s < root_dir_sectors; s++) {
				if (try_sector(start_sec + s)) { written = true; break; }
			}
		}
		else {
			// FAT32 Root Directory or Any Subdirectory
			uint32_t cluster = dst_parent_cluster;
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

		if (!written) return false; // Fail if no slot could be allocated

		// If the entry is a directory, update its ".." pointer to the new parent
		if (new_entry.attribute.directory) {
			uint32_t dir_start_cluster = new_entry.getFirstClusterNumber();
			if (fat_type != 32) dir_start_cluster &= 0xFFFF;

			if (dir_start_cluster >= 2) {
				uint32_t dir_sector = getSector_foCluster(dir_start_cluster);
				byte* temp_buf = new byte[bytes_per_sector];
				if (storage->Read(dir_sector, temp_buf)) {
					FAT_DirEntry* dotdot = (FAT_DirEntry*)&temp_buf[32];
					if (dotdot->name[0] == '.' && dotdot->name[1] == '.') {
						uint32_t pp_clus = (dst_parent_cluster == root_cluster && fat_type != 32) ? 0 : dst_parent_cluster;
						dotdot->cluster_low = (uint16_t)(pp_clus & 0xFFFF);
						dotdot->cluster_high = (uint16_t)(pp_clus >> 16);
						storage->Write(dir_sector, temp_buf);
					}
				}
				delete[] temp_buf;
			}
		}

		// 7. Mark the source entry as deleted
		byte* sector_buf = new byte[bytes_per_sector];
		if (storage->Read(src_entry_sector, sector_buf)) {
			FAT_DirEntry* d_entry = (FAT_DirEntry*)&sector_buf[src_entry_index * 32];
			d_entry->name[0] = 0xE5;
			storage->Write(src_entry_sector, sector_buf);
		}
		delete[] sector_buf;

		return true;
	}

}
