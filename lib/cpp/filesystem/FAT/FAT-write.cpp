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

	static uint32_t cluster_to_sector(FilesysFAT& fs, uint32_t cluster);

	stduint FilesysFAT::writfl(void* fil_handler, Slice file_slice, const byte* sors) {
		FAT_FileHandle* fh = (FAT_FileHandle*)fil_handler;
		if (fh->is_dir) return false;
		uint64_t offset = file_slice.address;
		uint64_t to_write = file_slice.length;
		uint64_t total_written = 0;
		stduint& bytes_per_sector = storage->Block_Size;
		uint32_t cluster = fh->start_cluster;
		uint32_t cluster_size = sectors_per_cluster * bytes_per_sector;
		
		// Ext file
		uint64_t needed_clusters = (offset + to_write + cluster_size - 1) / cluster_size;
		uint64_t current_clusters = (fh->size + cluster_size - 1) / cluster_size;
		
		if (needed_clusters > current_clusters) {
			// 分配新簇
			uint32_t last_cluster = cluster;
			for (uint32_t i = 1; i < current_clusters; i++) {
				last_cluster = get_fat_entry(last_cluster);
			}
			
			for (uint64_t i = current_clusters; i < needed_clusters; i++) {
				uint32_t new_cluster = find_free_cluster();
				if (new_cluster == 0) {
					error_number = 3; // part is full
					return false;
				}
				set_fat_entry(last_cluster, new_cluster);
				set_fat_entry(new_cluster, 0xFFFFFFF);
				last_cluster = new_cluster;
			}
		}
		
		while (to_write > 0) {
			uint32_t sector = cluster_to_sector(self, cluster);
			uint32_t sector_offset = offset % bytes_per_sector;
			uint32_t sector_index = offset / bytes_per_sector;
			uint32_t can_write = bytes_per_sector - sector_offset;
			storage->Read(sector + sector_index, buffer_sector);
			if (can_write > to_write) can_write = (uint32_t)to_write;
			MemCopyN(buffer_sector + sector_offset, sors + total_written, can_write);
			if (!storage->Write(sector + sector_index, buffer_sector))
				return false;

			total_written += can_write;
			to_write -= can_write;
			offset += can_write;
			
			if (offset >= cluster_size) {
				offset = 0;
				cluster = get_fat_entry(cluster);
			}
		}
		
		// update size of the file
		if (offset + total_written > fh->size) {
			fh->size = (uint32_t)(offset + total_written);
			// update new entry
			uint8_t dir_buffer[512];
			if (storage->Read(fh->dir_sector, dir_buffer)) {
				FAT_DirEntry* entry = (FAT_DirEntry*)&dir_buffer[fh->dir_index * 32];
				entry->file_size = fh->size;
				storage->Write(fh->dir_sector, dir_buffer);
			}
		}
		
		return true;
	}



	// ----

	static uint32_t cluster_to_sector(FilesysFAT& fs, uint32_t cluster)
	{
		if (cluster < 2) return 0;
		return ((cluster - 2) * fs.sectors_per_cluster) + fs.first_data_sector;
	}

}
