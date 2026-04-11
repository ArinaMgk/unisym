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

#include "../../../inc/c/format/filesys/FAT.h"
#include "../../../inc/c/proctrl/x86/inst_x86_64.h"

#include "../../../inc/c/consio.h"
namespace uni {



	bool FilesysFAT::makefs(rostr vol_label, void* moreinfo) {
		if (!storage) {
			plogerro("No storage"); return false;
		}
		if (_TEMP fat_type != 32) return false;
		bool state = false;
		ploginfo("Making FAT%u (buffer %[x])", fat_type, buffer_sector);
		stduint cluster_sz = moreinfo ? *(stduint*)(_IMM(moreinfo)) : nil;
		stduint secPerClus = (!cluster_sz) ? 8 : (cluster_sz / storage->Block_Size);
		// sector 0
		MemSet(buffer_sector, nil, storage->Block_Size);
		if (1) {
			FAT_BootSector32& bs = *(FAT_BootSector32*)buffer_sector;
			// Head
			bs.jmpBoot[0] = _JMP_imm_sid; bs.jmpBoot[1] = 0x58; bs.jmpBoot[2] = _NOP;// skip 0x58 bytes (to position +90)
			MemCopyN(bs.OEM, "MSWIN4.1", byteof(bs.OEM));
			bs.bytes_per_sector = storage->Block_Size;
			bs.sectors_per_cluster = secPerClus;
			bs.reserved_sectors = 32;// standard
			bs.fat_count = 2;// usual
			bs.root_entries = 0;// FAT32 0
			bs.total_sectors_16 = 0;// FAT32 0
			bs.media_type = 0xF8;// 0xF8 = fixed disk
			bs.sectors_per_fat_16 = 0;// FAT32 0
			bs.sectors_per_track = 0;
			bs.head_count = 0;
			bs.hidden_sectors = 0;
			bs.total_sectors_32 = storage->getUnits();
			// Tail
			bs.drive_number;
			bs.reserved1;
			bs.boot_signature = 0x29;
			bs.volume_id = 0x12345678;
			MemCopyN(bs.volume_label, vol_label, byteof(bs.volume_label));
			MemCopyN(bs.fs_type, "FAT32   ", byteof(bs.fs_type));
			// Body
			//   Calc FAT Table: FatSize = (TotalSectors / SecPerClus) * 4bytes / SectorSize
			uint32_t totalClusters = bs.total_sectors_32 / secPerClus;
			bs.sectors_per_fat_32 = (totalClusters * 4 + bs.bytes_per_sector - 1) / bs.bytes_per_sector;
			bs.fs_info_sector = 1;// usual
			bs.root_cluster = 2;// usual
			bs.backup_boot_sector = 6;// usual
			// Endian-independent
			treat<uint8>(buffer_sector + 510) = 0x55;
			treat<uint8>(buffer_sector + 511) = 0xAA;
			//! write the boot area code by the user
			storage->Write(0, buffer_sector);
		}
		// sector 1: FS Info
		MemSet(buffer_sector, nil, storage->Block_Size);
		storage->Write(1, buffer_sector);
		// Write FAT
		//   MemSet(buffer_sector, nil, storage->Block_Size);
		uint32_t* fatStart = (uint32_t*)buffer_sector;
		fatStart[0] = 0x0FFFFFF8; // Media type
		fatStart[1] = 0x0FFFFFFF; // EOC
		fatStart[2] = 0x0FFFFFFF; // Root Directory EOC (初始为空)
		storage->Write(((FAT_BootSector32*)buffer_sector)->reserved_sectors, buffer_sector); 
		return true;// loadfs();
	}

	bool FilesysFAT::loadfs(void* moreinfo) {
		// ploginfo("loadfs buffer_sector=%[x]", buffer_sector);
		if (!storage || !buffer_sector) {
			error_number = 1;
			return false;
		}
		if (!storage->Read(0, buffer_sector)) {
			error_number = 2;
			return false;
		}
		if (buffer_sector[510] != 0x55 || buffer_sector[511] != 0xAA) {
			error_number = 3;
			return false;
		}
		FAT_BootSector32& bs = *(FAT_BootSector32*)buffer_sector;
		if (!bs.sectors_per_cluster) {
			error_number = 4;
			plogerro("[%s] Zero sectors_per_cluster", __FILE__);
			return false;
		}
		self.total_sectors = bs.total_sectors_16 ? bs.total_sectors_16 : bs.total_sectors_32;

		sectors_per_fat = (fat_type == 32 || bs.sectors_per_fat_32) ? bs.sectors_per_fat_32 : bs.sectors_per_fat_16;
		if (!fat_type) {
			if (bs.sectors_per_fat_32) {
				fat_type = 32;
			} else if (bs.sectors_per_fat_16 != 0) {
				uint32_t root_dir_sectors = ((bs.root_entries * 32) + 
					(storage->Block_Size - 1)) / storage->Block_Size;
				uint32_t data_sectors = total_sectors - 
					(bs.reserved_sectors + bs.fat_count * sectors_per_fat + root_dir_sectors);
				total_clusters = data_sectors / bs.sectors_per_cluster;
				//
				if (total_clusters < 4085) fat_type = 12;
				else if (total_clusters < 65525) fat_type = 16;
				else fat_type = 32;
			}
		}

		first_fat_sector = bs.reserved_sectors;
		if (fat_type == 32) {
			root_dir_sectors = 0;
			first_data_sector = bs.reserved_sectors + (bs.fat_count * sectors_per_fat);
		} else {
			root_dir_sectors = ((bs.root_entries * 32) + (storage->Block_Size - 1)) / storage->Block_Size;
			first_data_sector = bs.reserved_sectors + (bs.fat_count * sectors_per_fat) + root_dir_sectors;
		}
		data_sectors = total_sectors - first_data_sector;
		total_clusters = data_sectors / bs.sectors_per_cluster;
		// storage->Block_Size is followed by hardware, not _bytesPerSector
		root_cluster = bs.root_cluster;
		sectors_per_cluster = bs.sectors_per_cluster;
		return fs_loaded = true;
	}

	enum DirWalkResult { WALK_CONTINUE, WALK_FOUND, WALK_END };

	bool FilesysFAT::find_entry_in_dir(uint32_t dir_cluster, const char* target_name, FAT_DirEntry* out_entry, uint32_t* out_sector, uint32_t* out_index) {
		char lfn_buf[256];
		int lfn_expected_order = 0;
		
		auto process_sector = [&](uint32_t sector) -> DirWalkResult {
			if (!storage->Read(sector, buffer_sector)) return WALK_END;
			for (int i = 0; i < 16; i++) {
				FAT_DirEntry* entry = (FAT_DirEntry*)&buffer_sector[i * 32];
				if ((byte)entry->name[0] == 0x00u) return WALK_END; // End of dir
				if ((byte)entry->name[0] == 0xE5u || (byte)entry->name[0] == 0x05u) {
					lfn_expected_order = 0; continue; // Deleted
				}
				
				if (entry->attribute.isLongName()) {
					FAT_LongDirEntry* lfn = (FAT_LongDirEntry*)entry;
					if (lfn->type != 0) continue;
					int order = lfn->order & 0x1F;
					if (lfn->order & 0x40) { // First logical piece (last physical)
						lfn_expected_order = order;
						MemSet(lfn_buf, 0, 256);
					} else if (order != lfn_expected_order - 1 || order == 0) {
						lfn_expected_order = 0;
						continue;
					}
					lfn_expected_order = order;
					int offset = (order - 1) * 13;
					if (offset < 0 || offset >= 256 - 14) { lfn_expected_order = 0; continue; }
					auto put_c = [&](uint16_t c) {
						if (c == 0x0000 || c == 0xFFFF) lfn_buf[offset++] = 0;
						else lfn_buf[offset++] = (char)(c & 0xFF);
					};
					for (int j = 0; j < 5; j++) put_c(lfn->name1[j]);
					for (int j = 0; j < 6; j++) put_c(lfn->name2[j]);
					for (int j = 0; j < 2; j++) put_c(lfn->name3[j]);
					continue;
				}
				
				if (entry->attribute.volume_id && !entry->attribute.directory) {
					lfn_expected_order = 0; continue;
				}
				
				char short_name[13];
				int name_len = 0;
				for (int k = 0; k < 8 && entry->name[k] != ' '; k++) short_name[name_len++] = entry->name[k];
				if (entry->ext[0] != ' ') {
					short_name[name_len++] = '.';
					for (int k = 0; k < 3 && entry->ext[k] != ' '; k++) short_name[name_len++] = entry->ext[k];
				}
				short_name[name_len] = 0;
				
				char* final_name = short_name;
				if (lfn_expected_order == 1) final_name = lfn_buf;
				
				// Sanity check to prevent explosive DFS traversal on garbage clusters
				bool is_valid_name = true;
				for (int n = 0; final_name[n]; n++) {
					unsigned char fn_c = (unsigned char)final_name[n];
					if (fn_c < 0x20 || fn_c == 0x7F) {
						is_valid_name = false; break;
					}
				}
				if (!is_valid_name) { lfn_expected_order = 0; continue; }

				if (!StrCompareInsensitive(final_name, target_name)) {
					if (out_entry) *out_entry = *entry;
					if (out_sector) *out_sector = sector;
					if (out_index) *out_index = i;
					return WALK_FOUND;
				}
				lfn_expected_order = 0;
			}
			return WALK_CONTINUE;
		};

		if (dir_cluster == 0 && fat_type != 32) {
			uint32_t start_sec = first_data_sector - root_dir_sectors;
			for (uint32_t s = 0; s < root_dir_sectors; s++) {
				DirWalkResult res = process_sector(start_sec + s);
				if (res == WALK_FOUND) return true;
				if (res == WALK_END) break;
			}
		} else {
			uint32_t cluster = dir_cluster;
			if (cluster == 0 && fat_type == 32) cluster = root_cluster;
			int limit = 100000;
			while (cluster >= 2 && cluster < 0xFFFFFFF8 && limit-- > 0) {
				uint32_t cluster_sec = getSector_foCluster(cluster);
				for (uint32_t s = 0; s < sectors_per_cluster; s++) {
					DirWalkResult res = process_sector(cluster_sec + s);
					if (res == WALK_FOUND) return true;
					if (res == WALK_END) return false;
				}
				cluster = get_fat_entry(cluster);
			}
		}
		return false;
	}

	void* FilesysFAT::search(rostr fullpath, void* moreinfo) {
		// moreinfo[0] --> FAT_FileHandle
		// moreinfo[1] --> FAT_DirInfo? (? means optional, set nullptr if not used)
		//
		if (!storage || !buffer_sector) {
			error_number = 1;
			return nullptr;
		}
		FAT_FileHandle* handle = (FAT_FileHandle*)((stduint*)moreinfo)[0];

		*handle = FAT_FileHandle{};
		handle->is_dir = true;
		handle->start_cluster = (fat_type == 32) ? root_cluster : 0;
		handle->current_cluster = handle->start_cluster;

		if (!fullpath || !*fullpath || !StrCompare("/", fullpath)) {
			return handle;
		}

		const char* path_p = fullpath;
		if (*path_p == '/') path_p++;

		while (*path_p) {
			char segment[256];
			int seg_len = 0;
			while (*path_p && *path_p != '/' && seg_len < 255) {
				segment[seg_len++] = *path_p++;
			}
			segment[seg_len] = '\0';
			while (*path_p == '/') path_p++; // skip consecutive slashes

			if (seg_len == 0) break;

			FAT_DirEntry entry;
			uint32_t found_sector, found_index;
			if (!find_entry_in_dir(handle->start_cluster, segment, &entry, &found_sector, &found_index)) {
				return nullptr; // segment not found
			}

			handle->start_cluster = entry.getFirstClusterNumber();
			handle->current_cluster = handle->start_cluster;
			handle->size = entry.file_size;
			handle->is_dir = (entry.attribute.attr & 0x10) != 0;
			handle->dir_sector = found_sector;
			handle->dir_index = found_index;

			if (handle->is_dir) {
				bool is_dot = (segment[0] == '.' && (segment[1] == '\0' || (segment[1] == '.' && segment[2] == '\0')));
				uint32_t root_cl = (fat_type == 32) ? root_cluster : 0;
				if (!is_dot && handle->start_cluster == root_cl) {
					return nullptr; // Broken FAT structure (sub-directory pointing to root)
				}
			}

			if (*path_p && !handle->is_dir) return nullptr; // trying to traverse into file
		}

		if (((stduint*)moreinfo)[1]) {
			FAT_DirInfo* info = (FAT_DirInfo*)((stduint*)moreinfo)[1];
			// Populate directory info if requested
			info->size = handle->size;
			// Attr omitted since we don't carry full block here cleanly without refetch.
			info->start_cluster = handle->start_cluster;
		}

		return handle;
	}

	bool FilesysFAT::proper(void* handler, stduint cmd, const void* moreinfo) {
		_TODO return false;
	}

	bool FilesysFAT::enumer(void* dir_handler, _tocall_ft _fn) {
		FAT_FileHandle* fh = (FAT_FileHandle*)dir_handler;
		if (!fh->is_dir) return false;

		char lfn_buf[256];
		int lfn_expected_order = 0;

		auto process_sector = [&](uint32_t sector) -> bool {
			if (!storage->Read(sector, buffer_sector)) return false;
			for (int i = 0; i < 16; i++) {
				FAT_DirEntry* entry = &cast<FAT_DirEntry*>(buffer_sector)[i];
				if ((byte)entry->name[0] == 0x00u) return false; // end of dir
				if ((byte)entry->name[0] == 0xE5u || (byte)entry->name[0] == 0x05u) {
					lfn_expected_order = 0; continue;
				}

				if (entry->attribute.isLongName()) {
					FAT_LongDirEntry* lfn = (FAT_LongDirEntry*)entry;
					if (lfn->type != 0) continue;
					int order = lfn->order & 0x1F;
					if (lfn->order & 0x40) {
						lfn_expected_order = order;
						MemSet(lfn_buf, 0, 256);
					}
					else if (order != lfn_expected_order - 1 || order == 0) {
						lfn_expected_order = 0; continue;
					}
					lfn_expected_order = order;
					int offset = (order - 1) * 13;
					if (offset < 0 || offset >= 256 - 14) { lfn_expected_order = 0; continue; }
					auto put_c = [&](uint16_t c) {
						if (c == 0x0000 || c == 0xFFFF) lfn_buf[offset++] = 0;
						else lfn_buf[offset++] = (char)(c & 0xFF);
					};
					for (int j = 0; j < 5; j++) put_c(lfn->name1[j]);
					for (int j = 0; j < 6; j++) put_c(lfn->name2[j]);
					for (int j = 0; j < 2; j++) put_c(lfn->name3[j]);
					continue;
				}

				if (entry->attribute.volume_id && !entry->attribute.directory) {
					lfn_expected_order = 0; continue;
				}

				char short_name[13];
				int name_len = 0;
				for (int k = 0; k < 8 && entry->name[k] != ' '; k++) short_name[name_len++] = entry->name[k];
				if (entry->ext[0] != ' ') {
					short_name[name_len++] = '.';
					for (int k = 0; k < 3 && entry->ext[k] != ' '; k++) short_name[name_len++] = entry->ext[k];
				}
				short_name[name_len] = 0;

				char* final_name = short_name;
				if (lfn_expected_order == 1) final_name = lfn_buf;

				bool is_valid_name = true;
				for (int n = 0; final_name[n]; n++) {
					unsigned char fn_c = (unsigned char)final_name[n];
					if (fn_c < 0x20 || fn_c == 0x7F) {
						is_valid_name = false; break;
					}
				}
				if (!is_valid_name) { lfn_expected_order = 0; continue; }

				ploginfo("FilesysFAT::enumer %s", final_name);
				if (_fn) _fn((void*)_IMM(entry->attribute.directory), (void*)final_name);

				lfn_expected_order = 0;
			}
			return true;
			};

		if (fh->start_cluster == 0 && fat_type != 32) {
			uint32_t start_sec = first_data_sector - root_dir_sectors;
			for (uint32_t s = 0; s < root_dir_sectors; s++) {
				if (!process_sector(start_sec + s)) break;
			}
		}
		else {
			uint32_t cluster = fh->start_cluster;
			if (cluster == 0 && fat_type == 32) cluster = root_cluster;
			int limit = 100000;
			while (cluster >= 2 && cluster < 0xFFFFFFF8 && limit-- > 0) {
				uint32_t cluster_sec = getSector_foCluster(cluster);
				bool cont = true;
				for (uint32_t s = 0; s < sectors_per_cluster; s++) {
					if (!process_sector(cluster_sec + s)) { cont = false; break; }
				}
				if (!cont) break;
				cluster = get_fat_entry(cluster);
			}
		}
		return true;
	}

	stduint FilesysFAT::readfl(void* fil_handler, Slice file_slice, byte* dest) {
		FAT_FileHandle* fh = (FAT_FileHandle*)fil_handler;
		if (fh->is_dir) return 0;
		auto [offset, to_read] = file_slice;// C++17 structured binding
		uint64_t total_read = 0;// ret
		MIN(to_read, offset + fh->size);
		// ploginfo("FilesysFAT::readfl %uBlks(0x%[64H],0x%[64H])", sectors_per_cluster, offset, to_read);

		// evaluate the start cluster
		uint32_t cluster = fh->start_cluster;
		uint32_t cluster_size = sectors_per_cluster * storage->Block_Size;

		// skip the start cluster
		while (offset >= cluster_size) {
			// ploginfo("offset >= cluster_size");
			offset -= cluster_size;
			cluster = get_fat_entry(cluster);
			if (cluster >= 0xFFFFFFF8) return total_read; // over the file
		}
		
		while (to_read > 0) {
			uint32_t sector = getSector_foCluster(cluster);
			uint32_t sector_offset = offset % storage->Block_Size;
			uint32_t sector_index = offset / storage->Block_Size;
			if (sector + sector_index == 0) {
				plogerro("[%s:%u] Bad sec id", __FILE__, __LINE__);
				return total_read;
			}

			if (!storage->Read(sector + sector_index, buffer_sector)) return 0;
			uint32_t can_read = storage->Block_Size - sector_offset;
			if (can_read > to_read) can_read = (uint32_t)to_read;
			
			MemCopyN(dest + total_read, buffer_sector + sector_offset, can_read);
			
			total_read += can_read;
			to_read -= can_read;
			offset += can_read;
			
			if (offset >= cluster_size) {
				offset = 0;
				auto old = cluster;
				cluster = get_fat_entry(cluster);
				if (cluster >= 0xFFFFFFF8 && to_read > 0) {
					return can_read;// not a error
				}
			}
		}
		return total_read;
	}


	// ----




	uint32_t FilesysFAT::get_fat_entry(uint32_t cluster)
	{
		if (!buffer_fatable) {
			plogerro("[%s:%u] buffer_fatable is null", __FILE__, __LINE__);
		}
		uint32_t fat_offset = 0;
		uint32_t fat_sector = 0;
		uint32_t ent_offset = 0;
		const uint32_t bytes_per_sector = storage->Block_Size;
		
		if (fat_type == 32) {
			fat_offset = cluster * 4;
			fat_sector = first_fat_sector + (fat_offset / bytes_per_sector);
			ent_offset = fat_offset % bytes_per_sector;
		} else if (fat_type == 16) {
			fat_offset = cluster * 2;
			fat_sector = first_fat_sector + (fat_offset / bytes_per_sector);
			ent_offset = fat_offset % bytes_per_sector;
		} else { // FAT12
			fat_offset = cluster * 3 / 2;
			fat_sector = first_fat_sector + (fat_offset / bytes_per_sector);
			ent_offset = fat_offset % bytes_per_sector;
		}
		
		if (current_sector != fat_sector) {
			storage->Read(fat_sector, buffer_fatable);
			current_sector = fat_sector;
		}
		
		if (fat_type == 32) {
			return *(uint32_t*)&buffer_fatable[ent_offset] & 0x0FFFFFFF;
		} else if (fat_type == 16) {
			uint16_t val = *(uint16_t*)&buffer_fatable[ent_offset];
			if (val >= 0xFFF8) return 0xFFFFFFF8;
			return val;
		} else {// 12
			uint16_t value = *(uint16_t*)&buffer_fatable[ent_offset];
			uint16_t val = (cluster & 0x01) ? (value >> 4) : (value & 0x0FFF);
			if (val >= 0x0FF8) return 0xFFFFFFF8;
			return val;
		}
	}


}
