// ASCII C TAB4 CRLF
// Docutitle: (Format/FileSystem) FAT Truncate
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

	bool FilesysFAT::truncate(void* fil_handler, stduint size) {
		FAT_FileHandle* fh = (FAT_FileHandle*)fil_handler;
		if (!fh || fh->is_dir) return false;

		stduint bytes_per_sector = storage->Block_Size;
		uint32_t cluster_size = sectors_per_cluster * bytes_per_sector;
		FAT_TableScratch fat_table_cache = {};
		FAT_TableScratch* fat_cache = nullptr;
		if (allow_allocate) {
			fat_table_cache.buffer = new byte[bytes_per_sector];
			if (!fat_table_cache.buffer) return false;
			fat_table_cache.current_sector = 0xFFFFFFFF;
			fat_cache = &fat_table_cache;
		}
		auto finish_truncate = [&](bool ret) -> bool {
			if (fat_cache) delete[] fat_table_cache.buffer;
			return ret;
		};

		// Dynamically determine EOC threshold and mark based on FAT version
		uint32_t eoc_threshold = 0x0FFFFFF8;
		uint32_t eoc_mark = 0x0FFFFFFF;
		if (this->fat_type == 12) {
			eoc_threshold = 0x0FF8;
			eoc_mark = 0x0FFF;
		}
		else if (this->fat_type == 16) {
			eoc_threshold = 0xFFF8;
			eoc_mark = 0xFFFF;
		}

		auto is_eof = [&](uint32_t c) {
			return c == 0 || c >= eoc_threshold;
		};

		if (size == fh->size) return finish_truncate(true);

		if (size < fh->size) {
			// Shrink/Truncate file
			if (size == 0) {
				// Free entire cluster chain
				uint32_t cluster = fh->start_cluster;
				while (cluster >= 2 && !is_eof(cluster)) {
					uint32_t next = get_fat_entry(cluster, fat_cache);
					set_fat_entry(cluster, 0, fat_cache); // Mark cluster as free
					cluster = next;
				}
				fh->start_cluster = 0;
				fh->current_cluster = 0;
			}
			else {
				uint32_t clusters_needed = (uint32_t)((size + cluster_size - 1) / cluster_size);
				uint32_t last = fh->start_cluster;
				for (uint32_t i = 1; i < clusters_needed; i++) {
					last = get_fat_entry(last, fat_cache);
					if (is_eof(last)) break;
				}

				if (!is_eof(last)) {
					uint32_t to_free = get_fat_entry(last, fat_cache);
					set_fat_entry(last, eoc_mark, fat_cache); // Mark new end of file

					// Free remaining clusters
					while (to_free >= 2 && !is_eof(to_free)) {
						uint32_t next = get_fat_entry(to_free, fat_cache);
						set_fat_entry(to_free, 0, fat_cache);
						to_free = next;
					}
				}
			}

			fh->size = (uint32_t)size;

			// Write updated size to directory entry
			byte* dir_buf = new byte[bytes_per_sector];
			if (storage->Read(fh->dir_sector, dir_buf)) {
				FAT_DirEntry* entry = (FAT_DirEntry*)&dir_buf[fh->dir_index * 32];
				entry->file_size = fh->size;
				entry->cluster_low = (uint16_t)(fh->start_cluster & 0xFFFF);
				entry->cluster_high = (uint16_t)(fh->start_cluster >> 16);
				storage->Write(fh->dir_sector, dir_buf);
			}
			delete[] dir_buf;
		}
		else {
			// Expand file
			uint32_t clusters_needed = (uint32_t)((size + cluster_size - 1) / cluster_size);
			uint32_t current_cluster_count = (fh->size + cluster_size - 1) / cluster_size;
			if (current_cluster_count == 0 && fh->size == 0 && fh->start_cluster != 0) current_cluster_count = 1;

			uint32_t cluster = fh->start_cluster;

			// If empty, allocate first cluster
			if (is_eof(cluster)) {
				cluster = find_free_cluster(fat_cache);
				if (cluster == 0) return finish_truncate(false);
				set_fat_entry(cluster, eoc_mark, fat_cache);
				fh->start_cluster = cluster;
				fh->current_cluster = cluster;

				byte* dir_buf = new byte[bytes_per_sector];
				if (storage->Read(fh->dir_sector, dir_buf)) {
					FAT_DirEntry* entry = (FAT_DirEntry*)&dir_buf[fh->dir_index * 32];
					entry->cluster_low = (uint16_t)(cluster & 0xFFFF);
					entry->cluster_high = (uint16_t)(cluster >> 16);
					storage->Write(fh->dir_sector, dir_buf);
				}
				delete[] dir_buf;
			}

			if (clusters_needed > current_cluster_count) {
				// Find last cluster
				uint32_t last = fh->start_cluster;
				while (true) {
					uint32_t next = get_fat_entry(last, fat_cache);
					if (is_eof(next)) break;
					last = next;
				}

				// Allocate more clusters
				for (uint32_t i = current_cluster_count; i < clusters_needed; i++) {
					uint32_t next = find_free_cluster(fat_cache);
					if (next == 0) return finish_truncate(false); // Disk full
					set_fat_entry(last, next, fat_cache);
					set_fat_entry(next, eoc_mark, fat_cache);
					last = next;
				}
			}

			fh->size = (uint32_t)size;

			byte* dir_buf = new byte[bytes_per_sector];
			if (storage->Read(fh->dir_sector, dir_buf)) {
				FAT_DirEntry* entry = (FAT_DirEntry*)&dir_buf[fh->dir_index * 32];
				entry->file_size = fh->size;
				storage->Write(fh->dir_sector, dir_buf);
			}
			delete[] dir_buf;
		}

		return finish_truncate(true);
	}

}
