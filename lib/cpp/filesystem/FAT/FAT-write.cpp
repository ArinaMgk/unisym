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

	stduint FilesysFAT::writfl(void* fil_handler, Slice file_slice, const byte* sors) {
		FAT_FileHandle* fh = (FAT_FileHandle*)fil_handler;
		if (fh->is_dir) return 0;
		
		uint64_t start_offset = file_slice.address;
		uint64_t to_write = file_slice.length;
		uint64_t total_written = 0;
		stduint bytes_per_sector = storage->Block_Size;
		uint32_t cluster_size = sectors_per_cluster * bytes_per_sector;

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

		// Helper lambda to check if a cluster is an End-Of-Cluster (EOC) marker
		auto is_eof = [&](uint32_t c) {
			return c == 0 || c >= eoc_threshold;
			};

		// Ensure enough clusters are allocated
		uint64_t end_pos = start_offset + to_write;
		uint32_t cluster = fh->start_cluster;

		// If it's a new empty file, allocate first cluster
		if (is_eof(cluster)) {
			cluster = find_free_cluster();
			if (cluster == 0) return 0;
			set_fat_entry(cluster, eoc_mark);
			fh->start_cluster = cluster;
			fh->current_cluster = cluster;

			// Update directory entry immediately for the new start cluster
			// Use dynamically allocated buffer to prevent stack overflow on large sector sizes
			byte* dir_buf = new byte[bytes_per_sector];
			if (storage->Read(fh->dir_sector, dir_buf)) {
				FAT_DirEntry* entry = (FAT_DirEntry*)&dir_buf[fh->dir_index * 32];
				entry->cluster_low = (uint16_t)(cluster & 0xFFFF);
				entry->cluster_high = (uint16_t)(cluster >> 16);
				storage->Write(fh->dir_sector, dir_buf);
			}
			delete[] dir_buf;
		}

		// Calculate how many clusters we need in total
		uint32_t clusters_needed = (uint32_t)((end_pos + cluster_size - 1) / cluster_size);
		uint32_t current_cluster_count = (fh->size + cluster_size - 1) / cluster_size;
		if (current_cluster_count == 0 && fh->size == 0 && fh->start_cluster != 0) current_cluster_count = 1;

		if (clusters_needed > current_cluster_count) {
			// Find the last cluster of the current chain safely
			uint32_t last = fh->start_cluster;
			while (true) {
				uint32_t next = get_fat_entry(last);
				if (is_eof(next)) break;
				last = next;
			}
			// Allocate more
			for (uint32_t i = current_cluster_count; i < clusters_needed; i++) {
				uint32_t next = find_free_cluster();
				if (next == 0) break; // disk full
				set_fat_entry(last, next);
				set_fat_entry(next, eoc_mark);
				last = next;
			}
		}

		// Start writing
		uint64_t current_pos = start_offset;

		// Seek to the correct cluster for start_offset
		cluster = fh->start_cluster;
		uint64_t seek_pos = current_pos;
		while (seek_pos >= cluster_size) {
			cluster = get_fat_entry(cluster);
			if (is_eof(cluster)) break;
			seek_pos -= cluster_size;
		}

		while (total_written < to_write && !is_eof(cluster)) {
			uint32_t cluster_offset = (uint32_t)(current_pos % cluster_size);
			uint32_t sector_in_cluster = cluster_offset / bytes_per_sector;
			uint32_t offset_in_sector = cluster_offset % bytes_per_sector;

			uint32_t target_sector = getSector_foCluster(cluster) + sector_in_cluster;
			uint32_t can_write_in_sector = bytes_per_sector - offset_in_sector;
			if (can_write_in_sector > (to_write - total_written))
				can_write_in_sector = (uint32_t)(to_write - total_written);

			if (offset_in_sector == 0 && can_write_in_sector == bytes_per_sector) {
				// Full sector write, no need to read first
				MemCopyN(buffer_sector, sors + total_written, bytes_per_sector);
			}
			else {
				storage->Read(target_sector, buffer_sector);
				MemCopyN(buffer_sector + offset_in_sector, sors + total_written, can_write_in_sector);
			}
			// plogwarn("FATWrite %u %s", target_sector, buffer_sector);
			if (!storage->Write(target_sector, buffer_sector)) break;

			total_written += can_write_in_sector;
			current_pos += can_write_in_sector;

			// If we crossed a cluster boundary, move to the next cluster
			if ((current_pos % cluster_size) == 0 && total_written < to_write) {
				cluster = get_fat_entry(cluster);
			}
		}

		// Update size if expanded
		if (current_pos > fh->size) {
			fh->size = (uint32_t)current_pos;
			byte* dir_buf = new byte[bytes_per_sector];
			if (storage->Read(fh->dir_sector, dir_buf)) {
				FAT_DirEntry* entry = (FAT_DirEntry*)&dir_buf[fh->dir_index * 32];
				entry->file_size = fh->size;
				storage->Write(fh->dir_sector, dir_buf);
			}
			delete[] dir_buf;
		}

		return (stduint)total_written;
	}

}