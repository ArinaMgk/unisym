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

	static FAT_FileHandle* path_to_handle(FilesysFAT& fs, FAT_FileHandle& handle, rostr fullpath);

	bool FilesysFAT::create(rostr fullpath, stduint flags, void* exinfo, rostr linkdest) {
		if (!fs_loaded) return false;
		ploginfo("FAT: create %s", fullpath);
		
		// seek parent dir
		const char* filename = StrIndexCharRight(fullpath, '/');
		if (!filename) filename = fullpath;
		
		// allocate new cluster
		uint32_t new_cluster = find_free_cluster();
		if (new_cluster == 0) {
			error_number = 3; // part is full
			return false;
		}
		
		// mark of end
		uint32_t end_mark = (fat_type == 32) ? 0x0FFFFFFF : 
						   (fat_type == 16) ? 0xFFFF : 0xFFF;
		set_fat_entry(new_cluster, end_mark);
		
		// create dir entry
		FAT_DirEntry entry;
		MemSet(&entry, 0, sizeof(FAT_DirEntry));
		MemSet(entry.name, ' ', 8);
		MemSet(entry.ext, ' ', 3);
		
		// parse 8.3 filename
		const char* slash = StrIndexCharRight(filename, '/');
		if (slash) filename = slash + 1;
		
		const char* dot = StrIndexCharRight(filename, '.');
		if (dot) {
			int name_len = dot - filename;
			int ext_len = StrLength(dot + 1);
			if (name_len > 8) name_len = 8;
			if (ext_len > 3) ext_len = 3;
			MemCopyN(entry.name, filename, name_len);
			MemCopyN(entry.ext, dot + 1, ext_len);
		} else {
			int name_len = StrLength(filename);
			if (name_len > 8) name_len = 8;
			MemCopyN(entry.name, filename, name_len);
		}
		
		for (int i = 0; i < 8; i++) entry.name[i] = ascii_toupper(entry.name[i]);
		for (int i = 0; i < 3; i++) entry.ext[i] = ascii_toupper(entry.ext[i]);
		
		entry.attribute.attr = (flags & 1) ? 0x10 : 0x20; // dir or file
		entry.cluster_high = (uint16_t)(new_cluster >> 16);
		entry.cluster_low = (uint16_t)(new_cluster & 0xFFFF);
		
		FAT_FileHandle* p_fh = (FAT_FileHandle*)exinfo;
		uint32_t parent_cluster = p_fh ? p_fh->start_cluster : root_cluster;
		
		auto try_sector = [&](uint32_t sector) -> bool {
			ploginfo("FAT: scanning sector %u", sector);
			if (!storage->Read(sector, buffer_sector)) return false;
			for (int i = 0; i < 16; i++) {
				FAT_DirEntry* dir_entry = (FAT_DirEntry*)&buffer_sector[i * 32];
				if (dir_entry->name[0] == 0 || (byte)dir_entry->name[0] == 0xE5u) {
					MemCopyN(dir_entry, &entry, sizeof(FAT_DirEntry));
					ploginfo("FAT: sector %u writing entry...", sector);
					return storage->Write(sector, buffer_sector);
				}
			}
			return false;
		};

		if (parent_cluster == 0 && fat_type != 32) {
			// FAT12/16 Root Directory
			uint32_t start_sec = first_data_sector - root_dir_sectors;
			for (uint32_t s = 0; s < root_dir_sectors; s++) {
				if (try_sector(start_sec + s)) return true;
			}
		} else {
			// FAT32 or Subdirectory
			uint32_t cluster = parent_cluster;
			if (cluster == 0 && fat_type == 32) cluster = root_cluster;
			
			int limit = 10000;
			while (cluster >= 2 && cluster < 0xFFFFFFF8 && limit-- > 0) {
				uint32_t cluster_sec = getSector_foCluster(cluster);
				for (uint32_t s = 0; s < sectors_per_cluster; s++) {
					if (try_sector(cluster_sec + s)) return true;
				}
				cluster = get_fat_entry(cluster);
				//{} TODO: Allocate new cluster for dir if full
			}
		}

		error_number = 4; // Dir Full
		return false;
	}



}
