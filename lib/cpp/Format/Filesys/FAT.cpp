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
#include "../../../../inc/cpp/ISO_IEC_STD/memory"
namespace uni {

	namespace {
		struct FAT_LfnState {
			uint16_t name_utf16[256];
			char utf8_name[768];
			uint8_t checksum;
			int expected_order;
			bool active;
		};

		static void fat_clear_lfn_sequence(FAT_LfnState& state) {
			MemSet(state.name_utf16, 0, byteof(state.name_utf16));
			state.checksum = 0;
			state.expected_order = 0;
			state.active = false;
		}

		static void fat_reset_lfn_state(FAT_LfnState& state) {
			fat_clear_lfn_sequence(state);
			MemSet(state.utf8_name, 0, byteof(state.utf8_name));
		}

		static uint8_t fat_short_name_checksum(const FAT_DirEntry& entry) {
			uint8_t sum = 0;
			for (int i = 0; i < 8; i++) sum = ((sum & 1u) ? 0x80u : 0) + (sum >> 1) + (uint8_t)entry.name[i];
			for (int i = 0; i < 3; i++) sum = ((sum & 1u) ? 0x80u : 0) + (sum >> 1) + (uint8_t)entry.ext[i];
			return sum;
		}

		static void fat_decode_short_name(const FAT_DirEntry& entry, char out_name[13]) {
			int name_len = 0;
			for (int k = 0; k < 8 && entry.name[k] != ' '; k++) {
				unsigned char ch = (unsigned char)entry.name[k];
				out_name[name_len++] = (ch == 0x05u) ? (char)0xE5u : (char)ch;
			}
			if (entry.ext[0] != ' ') {
				out_name[name_len++] = '.';
				for (int k = 0; k < 3 && entry.ext[k] != ' '; k++) out_name[name_len++] = entry.ext[k];
			}
			out_name[name_len] = 0;
		}

		static bool fat_append_lfn_piece(FAT_LfnState& state, const FAT_LongDirEntry& lfn) {
			if (lfn.type != 0 || lfn.cluster != 0) {
				fat_reset_lfn_state(state);
				return false;
			}

			int order = lfn.order & 0x1F;
			if (order == 0) {
				fat_reset_lfn_state(state);
				return false;
			}

			if (lfn.order & 0x40) {
				fat_clear_lfn_sequence(state);
				state.active = true;
				state.checksum = lfn.checksum;
				state.expected_order = order;
			} else {
				if (!state.active || lfn.checksum != state.checksum || order != state.expected_order - 1) {
					fat_reset_lfn_state(state);
					return false;
				}
				state.expected_order = order;
			}

			int piece_offset = (order - 1) * 13;
			if (piece_offset < 0 || piece_offset + 13 > 256) {
				fat_reset_lfn_state(state);
				return false;
			}

			int idx = 0;
			auto append_unit = [&](uint16_t c) {
				state.name_utf16[piece_offset + idx++] = (c == 0xFFFF) ? 0 : c;
			};
			for (int j = 0; j < 5; j++) append_unit(lfn.name1[j]);
			for (int j = 0; j < 6; j++) append_unit(lfn.name2[j]);
			for (int j = 0; j < 2; j++) append_unit(lfn.name3[j]);
			return true;
		}

		static const char* fat_finalize_lfn_name(FAT_LfnState& state) {
			state.utf8_name[0] = 0;
			String out(state.utf8_name, byteof(state.utf8_name));
			for (int i = 0; i < 255; i++) {
				uint16_t code = state.name_utf16[i];
				if (code == 0x0000) break;
				uint32_t rune = code;
				if (code >= 0xD800u && code <= 0xDBFFu) {
					const uint16_t low = (i + 1 < 255) ? state.name_utf16[i + 1] : 0;
					if (low >= 0xDC00u && low <= 0xDFFFu) {
						rune = 0x10000u + ((uint32_t)(code - 0xD800u) << 10) + (uint32_t)(low - 0xDC00u);
						i++;
					}
					else rune = 0xFFFDu;
				}
				else if (code >= 0xDC00u && code <= 0xDFFFu) {
					rune = 0xFFFDu;
				}
				if (!out.PushRune((char32_t)rune)) break;
			}
			return state.utf8_name;
		}

		static const char* fat_pick_display_name(FAT_LfnState& state, const FAT_DirEntry& entry, char short_name[13]) {
			fat_decode_short_name(entry, short_name);
			bool use_lfn = state.active &&
				state.expected_order == 1 &&
				state.checksum == fat_short_name_checksum(entry);
			const char* final_name = short_name;
			if (use_lfn) {
				const char* lfn_name = fat_finalize_lfn_name(state);
				if (lfn_name[0]) final_name = lfn_name;
			}
			fat_clear_lfn_sequence(state);
			return final_name;
		}
	}



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
			bs.bytes_per_sector = (uint16_t)storage->Block_Size;
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
			(void)bs.drive_number;
			(void)bs.reserved1;
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
		fatStart[2] = 0x0FFFFFFF; // Root Directory EOC (initially empty)
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
		if ((buffer_sector[510] != 0x55 || buffer_sector[511] != 0xAA) &&
			(buffer_sector[storage->Block_Size - 2] != 0x55 || buffer_sector[storage->Block_Size - 1] != 0xAA)) {
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

		// If it's 0, then it must be FAT32, so we can safely read sectors_per_fat_32.
		sectors_per_fat = bs.sectors_per_fat_16 ? bs.sectors_per_fat_16 : bs.sectors_per_fat_32;

		if (!fat_type) {
			// Calculate cluster count to properly identify FAT type
			uint32_t root_dir_sectors_tmp = ((bs.root_entries * 32) + 
				(storage->Block_Size - 1)) / storage->Block_Size;
			uint32_t data_sectors_tmp = self.total_sectors - 
				(bs.reserved_sectors + bs.fat_count * sectors_per_fat + root_dir_sectors_tmp);
			total_clusters = data_sectors_tmp / bs.sectors_per_cluster;
			//
			if (total_clusters < 4085) fat_type = 12;
			else if (total_clusters < 65525) fat_type = 16;
			else fat_type = 32;
		}

		first_fat_sector = bs.reserved_sectors;
		if (fat_type == 32) {
			root_dir_sectors = 0;
			first_data_sector = bs.reserved_sectors + (bs.fat_count * sectors_per_fat);
		} else {
			root_dir_sectors = ((bs.root_entries * 32) + (storage->Block_Size - 1)) / storage->Block_Size;
			first_data_sector = bs.reserved_sectors + (bs.fat_count * sectors_per_fat) + root_dir_sectors;
		}
		
		data_sectors = self.total_sectors - first_data_sector;
		total_clusters = data_sectors / bs.sectors_per_cluster;
		
		root_cluster = (fat_type == 32) ? bs.root_cluster : 0;
		sectors_per_cluster = bs.sectors_per_cluster;
		fat_count = bs.fat_count;
		return fs_loaded = true;
	}

	enum DirWalkResult { WALK_CONTINUE, WALK_FOUND, WALK_END, WALK_LIMIT, WALK_ERROR };

	bool FilesysFAT::find_entry_in_dir(uint32_t dir_cluster, const char* target_name, FAT_DirEntry* out_entry, uint32_t* out_sector, uint32_t* out_index, byte* sector_buffer, FAT_TableScratch* fat_cache, uint32_t* out_lfn_sector, uint32_t* out_lfn_index, stduint* out_lfn_count) {
		if (!sector_buffer) sector_buffer = buffer_sector;
		if (!sector_buffer) return false;

		uni::UniquePtr<FAT_LfnState> lfn_state_holder(new FAT_LfnState);// ~1.3KB: keep it off the stack
		if (!lfn_state_holder) return false;
		FAT_LfnState& lfn_state = *lfn_state_holder;
		fat_reset_lfn_state(lfn_state);
		
		uint32_t lfn_run_sector = 0;
		uint32_t lfn_run_index = 0;
		stduint lfn_run_count = 0;
		auto process_sector = [&](uint32_t sector) -> DirWalkResult {
			if (!storage->Read(sector, sector_buffer)) return WALK_END;
			int entries_per_sector = storage->Block_Size / 32;
			for (int i = 0; i < entries_per_sector; i++) {
				FAT_DirEntry* entry = (FAT_DirEntry*)&sector_buffer[i * 32];
				if ((byte)entry->name[0] == 0x00u) return WALK_END; // End of dir
				if ((byte)entry->name[0] == 0xE5u) {
					fat_reset_lfn_state(lfn_state);
					lfn_run_count = 0;
					continue; // Deleted
				}
				
				if (entry->attribute.isLongName()) {
					if (!lfn_run_count) {
						lfn_run_sector = sector;
						lfn_run_index = (uint32_t)i;
					}
					lfn_run_count++;
					fat_append_lfn_piece(lfn_state, *(FAT_LongDirEntry*)entry);
					continue;
				}
				
				if (entry->attribute.volume_id && !entry->attribute.directory) {
					fat_reset_lfn_state(lfn_state);
					lfn_run_count = 0;
					continue;
				}
				
				char short_name[13];
				const char* final_name = fat_pick_display_name(lfn_state, *entry, short_name);
				
				// Sanity check to prevent explosive DFS traversal on garbage clusters
				bool is_valid_name = true;
				for (int n = 0; final_name[n]; n++) {
					unsigned char fn_c = (unsigned char)final_name[n];
					if (fn_c < 0x20 || fn_c == 0x7F) {
						is_valid_name = false; break;
					}
				}
				if (!is_valid_name) {
					lfn_run_count = 0;
					continue;
				}

				if (!StrCompareInsensitive(final_name, target_name)) {
					if (out_entry) *out_entry = *entry;
					if (out_sector) *out_sector = sector;
					if (out_index) *out_index = i;
					if (out_lfn_sector) *out_lfn_sector = lfn_run_sector;
					if (out_lfn_index) *out_lfn_index = lfn_run_index;
					if (out_lfn_count) *out_lfn_count = lfn_run_count;
					return WALK_FOUND;
				}
				lfn_run_count = 0;
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
				cluster = get_fat_entry(cluster, fat_cache);
			}
		}
		return false;
	}

	void* FilesysFAT::search(rostr fullpath, FilesysSearchArgs* args) {
		// args->handle_buffer --> FAT_FileHandle
		// args->dir_info      --> FAT_DirInfo? (? means optional, set nullptr if not used)
		//
		if (!storage || !args || (!buffer_sector && !allow_allocate)) {
			error_number = 1;
			return nullptr;
		}
		FAT_FileHandle* handle = (FAT_FileHandle*)args->handle_buffer;

		*handle = FAT_FileHandle{};
		handle->is_dir = true;
		handle->start_cluster = (fat_type == 32) ? root_cluster : 0;
		handle->current_cluster = handle->start_cluster;

		if (!fullpath || !*fullpath || !StrCompare("/", fullpath)) {
			return handle;
		}

		byte* sector_buffer = buffer_sector;
		bool allocated_sector_buffer = false;
		if (allow_allocate) {
			sector_buffer = new byte[storage->Block_Size];
			if (!sector_buffer) return nullptr;
			allocated_sector_buffer = true;
		}
		FAT_TableScratch fat_table_cache = {};
		FAT_TableScratch* fat_cache = nullptr;
		if (allow_allocate) {
			fat_table_cache.buffer = new byte[storage->Block_Size];
			if (!fat_table_cache.buffer) {
				if (allocated_sector_buffer) delete[] sector_buffer;
				return nullptr;
			}
			fat_table_cache.current_sector = 0xFFFFFFFF;
			fat_cache = &fat_table_cache;
		}
		auto finish_search = [&](void* ret) -> void* {
			if (fat_cache) delete[] fat_table_cache.buffer;
			if (allocated_sector_buffer) delete[] sector_buffer;
			return ret;
		};

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
			if (!find_entry_in_dir(handle->start_cluster, segment, &entry, &found_sector, &found_index, sector_buffer, fat_cache)) {
				return finish_search(nullptr); // segment not found
			}

			handle->start_cluster = entry.getFirstClusterNumber();
			// FIX: FAT12/16 cluster_high might have garbage (EAs). Only keep LOW bits!
			if (fat_type != 32) handle->start_cluster &= 0xFFFF;
			
			handle->current_cluster = handle->start_cluster;
			handle->size = entry.file_size;
			handle->is_dir = (entry.attribute.attr & 0x10) != 0;
			handle->dir_sector = found_sector;
			handle->dir_index = found_index;


			if (handle->is_dir) {
				bool is_dot = (segment[0] == '.' && (segment[1] == '\0' || (segment[1] == '.' && segment[2] == '\0')));
				uint32_t root_cl = (fat_type == 32) ? root_cluster : 0;
				if (!is_dot && handle->start_cluster == root_cl) {
					return finish_search(nullptr); // Broken FAT structure (sub-directory pointing to root)
				}
			}

			// Invoke callback for VFS caching if provided
			if (args->on_segment) {
				if (!args->on_segment(handle, segment, (stduint)handle->is_dir, (stduint)handle->size, args->user_data)) {
					// Callback requested to stop the search (e.g., crossed mount point)
					return finish_search(handle);
				}
			}

			if (*path_p && !handle->is_dir) return finish_search(nullptr); // trying to traverse into file
		}

		if (args->dir_info) {
			FAT_DirInfo* info = (FAT_DirInfo*)args->dir_info;
			// Populate directory info if requested
			info->size = handle->size;
			// Attr omitted since we don't carry full block here cleanly without refetch.
			info->start_cluster = handle->start_cluster;
		}

		return finish_search(handle);
	}

	bool FilesysFAT::proper(void* handler, stduint cmd, const void* moreinfo) {
		FAT_FileHandle* fh = (FAT_FileHandle*)handler;
		if (!fh || !moreinfo) return false;

		switch ((FilesysCmd)cmd) {
		case FilesysCmd::FS_CMD_GET_SIZE:
			*(stduint*)moreinfo = fh->size;
			return true;
		case FilesysCmd::FS_CMD_GET_ISDIR:
			*(bool*)moreinfo = fh->is_dir;
			return true;
		default:
			return false;
		}
	}

	bool FilesysFAT::enumer(void* dir_handler, _tocall_ft _fn, FilesysEnumState* state) {
		FAT_FileHandle* fh = (FAT_FileHandle*)dir_handler;
		if (!fh || !fh->is_dir) return false;
		if (!_fn) return true;
		if (state && (state->finished || state->full())) return true;

		byte* sector_buffer = buffer_sector;
		bool allocated_sector_buffer = false;
		if (allow_allocate) {
			sector_buffer = new byte[storage->Block_Size];
			if (!sector_buffer) return false;
			allocated_sector_buffer = true;
		}
		if (!sector_buffer) return false;
		FAT_TableScratch fat_table_cache = {};
		FAT_TableScratch* fat_cache = nullptr;
		if (allow_allocate) {
			fat_table_cache.buffer = new byte[storage->Block_Size];
			if (!fat_table_cache.buffer) {
				if (allocated_sector_buffer) delete[] sector_buffer;
				return false;
			}
			fat_table_cache.current_sector = 0xFFFFFFFF;
			fat_cache = &fat_table_cache;
		}

		uni::UniquePtr<FAT_LfnState> lfn_state_holder(new FAT_LfnState);// ~1.3KB: keep it off the stack
		if (!lfn_state_holder) return false;
		FAT_LfnState& lfn_state = *lfn_state_holder;
		fat_reset_lfn_state(lfn_state);

		enum {
			FAT_ENUM_CLUSTER = 0,
			FAT_ENUM_SECTOR_OFFSET = 1,
			FAT_ENUM_ENTRY_INDEX = 2,
			FAT_ENUM_STARTED = 3,
		};

		uint32_t cursor_cluster = fh->start_cluster;
		if (cursor_cluster == 0 && fat_type == 32) cursor_cluster = root_cluster;
		uint32_t cursor_sector_offset = 0;
		uint32_t cursor_entry_index = 0;
		if (state) {
			state->current = state->position;
			if (state->opaque[FAT_ENUM_STARTED]) {
				cursor_cluster = (uint32_t)state->opaque[FAT_ENUM_CLUSTER];
				cursor_sector_offset = (uint32_t)state->opaque[FAT_ENUM_SECTOR_OFFSET];
				cursor_entry_index = (uint32_t)state->opaque[FAT_ENUM_ENTRY_INDEX];
			}
			else {
				state->opaque[FAT_ENUM_CLUSTER] = cursor_cluster;
				state->opaque[FAT_ENUM_SECTOR_OFFSET] = 0;
				state->opaque[FAT_ENUM_ENTRY_INDEX] = 0;
				state->opaque[FAT_ENUM_STARTED] = 1;
			}
		}

		auto save_cursor = [&](uint32_t cluster, uint32_t sector_offset, uint32_t entry_index) {
			if (!state) return;
			state->opaque[FAT_ENUM_CLUSTER] = cluster;
			state->opaque[FAT_ENUM_SECTOR_OFFSET] = sector_offset;
			state->opaque[FAT_ENUM_ENTRY_INDEX] = entry_index;
			state->opaque[FAT_ENUM_STARTED] = 1;
		};

		auto process_sector = [&](uint32_t sector, uint32_t cluster, uint32_t sector_offset, uint32_t start_index) -> DirWalkResult {
			if (!storage->Read(sector, sector_buffer)) return WALK_ERROR;
			int entries_per_sector = storage->Block_Size / 32;
			for (int i = (int)start_index; i < entries_per_sector; i++) {
				FAT_DirEntry* entry = &cast<FAT_DirEntry*>(sector_buffer)[i];
				if ((byte)entry->name[0] == 0x00u) {
					if (state) state->mark_finished();
					return WALK_END;
				}
				if ((byte)entry->name[0] == 0xE5u) {
					fat_reset_lfn_state(lfn_state); continue;
				}

				if (entry->attribute.isLongName()) {
					fat_append_lfn_piece(lfn_state, *(FAT_LongDirEntry*)entry);
					continue;
				}

				if (entry->attribute.volume_id && !entry->attribute.directory) {
					fat_reset_lfn_state(lfn_state); continue;
				}

				char short_name[13];
				const char* final_name = fat_pick_display_name(lfn_state, *entry, short_name);

				bool is_valid_name = true;
				for (int n = 0; final_name[n]; n++) {
					unsigned char fn_c = (unsigned char)final_name[n];
					if (fn_c < 0x20 || fn_c == 0x7F) {
						is_valid_name = false; break;
					}
				}
				if (!is_valid_name) continue;

				// ploginfo("FilesysFAT::enumer %s", final_name);
				if (_fn) _fn((void*)_IMM(entry->attribute.directory), (void*)final_name);
				if (state) {
					state->emit_one();
					uint32_t next_sector_offset = sector_offset;
					uint32_t next_entry_index = (uint32_t)i + 1;
					if (next_entry_index >= (uint32_t)entries_per_sector) {
						next_sector_offset++;
						next_entry_index = 0;
					}
					save_cursor(cluster, next_sector_offset, next_entry_index);
					if (state->full()) return WALK_LIMIT;
				}
			}
			return WALK_CONTINUE;
			};

		if (fh->start_cluster == 0 && fat_type != 32) {
			uint32_t start_sec = first_data_sector - root_dir_sectors;
			for (uint32_t s = cursor_sector_offset; s < root_dir_sectors; s++) {
				DirWalkResult res = process_sector(start_sec + s, 0, s, (s == cursor_sector_offset) ? cursor_entry_index : 0);
				if (res == WALK_ERROR) {
					if (fat_cache) delete[] fat_table_cache.buffer;
					if (allocated_sector_buffer) delete[] sector_buffer;
					return false;
				}
				if (res == WALK_END || res == WALK_LIMIT) break;
				save_cursor(0, s + 1, 0);
			}
			if (state && !state->full()) state->mark_finished();
		}
		else {
			uint32_t cluster = cursor_cluster;
			int limit = 100000;
			while (cluster >= 2 && cluster < 0xFFFFFFF8 && limit-- > 0) {
				uint32_t cluster_sec = getSector_foCluster(cluster);
				DirWalkResult res = WALK_CONTINUE;
				for (uint32_t s = cursor_sector_offset; s < sectors_per_cluster; s++) {
					res = process_sector(cluster_sec + s, cluster, s, (s == cursor_sector_offset) ? cursor_entry_index : 0);
					if (res == WALK_ERROR) {
						if (fat_cache) delete[] fat_table_cache.buffer;
						if (allocated_sector_buffer) delete[] sector_buffer;
						return false;
					}
					if (res == WALK_END || res == WALK_LIMIT) break;
					save_cursor(cluster, s + 1, 0);
				}
				if (res == WALK_END || res == WALK_LIMIT) break;
				cluster = get_fat_entry(cluster, fat_cache);
				cursor_sector_offset = 0;
				cursor_entry_index = 0;
				save_cursor(cluster, 0, 0);
			}
			if ((cluster < 2 || cluster >= 0xFFFFFFF8 || limit <= 0) && state) state->mark_finished();
		}
		if (fat_cache) delete[] fat_table_cache.buffer;
		if (allocated_sector_buffer) delete[] sector_buffer;
		return true;
	}

	stduint FilesysFAT::readfl(void* fil_handler, Slice file_slice, byte* dest) {
		FAT_FileHandle* fh = (FAT_FileHandle*)fil_handler;
		if (fh->is_dir) return 0;
		auto [offset, to_read] = file_slice; // C++17 structured binding
		uint64_t total_read = 0; // ret

		if (offset >= fh->size) {
			// plogerro("[%s:%u] read beyond EOF off=%u size=%u", __FILE__, __LINE__, offset, fh->size);
			return 0;
		}
		MIN(to_read, fh->size - offset);

		byte* sector_buffer = buffer_sector;
		bool allocated_sector_buffer = false;
		FAT_TableScratch fat_table_cache = {};
		FAT_TableScratch* fat_cache = nullptr;
		if (allow_allocate) {
			sector_buffer = new byte[storage->Block_Size];
			if (!sector_buffer) return total_read;
			allocated_sector_buffer = true;

			fat_table_cache.buffer = new byte[storage->Block_Size];
			if (!fat_table_cache.buffer) {
				delete[] sector_buffer;
				return total_read;
			}
			fat_table_cache.current_sector = 0xFFFFFFFF;
			fat_cache = &fat_table_cache;
		}
		if (!sector_buffer) return total_read;
		auto finish_read = [&](stduint ret) -> stduint {
			if (fat_cache) delete[] fat_table_cache.buffer;
			if (allocated_sector_buffer) delete[] sector_buffer;
			return ret;
		};

		// evaluate the start cluster
		uint32_t cluster_size = sectors_per_cluster * storage->Block_Size;
		uint32_t cluster;
		uint32_t cluster_file_offset; // file offset of current cluster's start

		// Use cached position to avoid O(n) FAT walk from start on forward seeks.
		// current_cluster/current_offset record where the last readfl ended.
		if (fh->current_cluster >= 2 && offset >= fh->current_offset) {
			cluster = fh->current_cluster;
			cluster_file_offset = fh->current_offset;
			uint32_t skip = offset - fh->current_offset;
			while (skip >= cluster_size) {
				skip -= cluster_size;
				cluster_file_offset += cluster_size;
				cluster = get_fat_entry(cluster, fat_cache);
				if (cluster >= 0xFFFFFFF8) return finish_read(total_read);
			}
			offset = skip; // cluster-local offset
		} else {
			cluster = fh->start_cluster;
			cluster_file_offset = 0;
			while (offset >= cluster_size) {
				offset -= cluster_size;
				cluster_file_offset += cluster_size;
				cluster = get_fat_entry(cluster, fat_cache);
				if (cluster >= 0xFFFFFFF8) return finish_read(total_read); // over the file
			}
		}

		while (to_read > 0) {
			uint32_t sector = getSector_foCluster(cluster);
			uint32_t sector_offset = offset % storage->Block_Size;
			uint32_t sector_index = offset / storage->Block_Size;
			if (sector + sector_index == 0) {
				plogerro("[%s:%u] Bad sec id", __FILE__, __LINE__);
				return finish_read(total_read);
			}

			if (!storage->Read(sector + sector_index, sector_buffer)) {
				plogerro("[%s:%u] storage read fail sector=%u off=%u", __FILE__, __LINE__, sector + sector_index, offset);
				return finish_read(total_read);
			}
			uint32_t can_read = storage->Block_Size - sector_offset;
			if (can_read > to_read) can_read = (uint32_t)to_read;

			MemCopyN(dest + total_read, sector_buffer + sector_offset, can_read);

			total_read += can_read;
			to_read -= can_read;
			offset += can_read;

			if (to_read == 0) break;

			if (offset >= cluster_size) {
				offset = 0;
				cluster_file_offset += cluster_size;
				auto old = cluster;
				cluster = get_fat_entry(cluster, fat_cache);
				if (cluster >= 0xFFFFFFF8) {
					// Fix 3: Return the total accumulated bytes, not the last chunk
					fh->current_cluster = old;
					fh->current_offset = cluster_file_offset - cluster_size;
					return finish_read(total_read);
				}
			}
		}
		// Cache current position for future forward seeks
		fh->current_cluster = cluster;
		fh->current_offset = cluster_file_offset;
		return finish_read(total_read);
	}


	// ----

	uint32_t FilesysFAT::get_fat_entry(uint32_t cluster, FAT_TableScratch* fat_cache)
	{
		byte* fat_buffer = fat_cache ? fat_cache->buffer : buffer_fatable;
		uint32_t* cached_sector = fat_cache ? &fat_cache->current_sector : &current_sector;
		if (!fat_buffer) {
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
		}
		else if (fat_type == 16) {
			fat_offset = cluster * 2;
			fat_sector = first_fat_sector + (fat_offset / bytes_per_sector);
			ent_offset = fat_offset % bytes_per_sector;
		}
		else { // FAT12
			fat_offset = cluster * 3 / 2;
			fat_sector = first_fat_sector + (fat_offset / bytes_per_sector);
			ent_offset = fat_offset % bytes_per_sector;
		}

		if (*cached_sector != fat_sector) {
			storage->Read(fat_sector, fat_buffer);
			*cached_sector = fat_sector;
		}

		if (fat_type == 32) {
			uint32_t val = *(uint32_t*)&fat_buffer[ent_offset] & 0x0FFFFFFF;
			if (val >= 0x0FFFFFF8) return 0xFFFFFFF8;
			return val;
		}
		else if (fat_type == 16) {
			uint16_t val = *(uint16_t*)&fat_buffer[ent_offset];
			if (val >= 0xFFF8) return 0xFFFFFFF8;
			return val;
		}
		else { // FAT12
			uint16_t value;
			if (ent_offset == bytes_per_sector - 1) {
				uint8_t byte_low = fat_buffer[ent_offset];
				// Read the next sector to fetch the higher byte
				storage->Read(fat_sector + 1, fat_buffer);
				*cached_sector = fat_sector + 1; // Update sector state for cache coherency
				uint8_t byte_high = fat_buffer[0];
				value = ((uint16_t)byte_low) | ((uint16_t)byte_high << 8);
			}
			else {
				value = ((uint16_t)fat_buffer[ent_offset]) |
					((uint16_t)fat_buffer[ent_offset + 1] << 8);
			}

			uint16_t val = (cluster & 0x01) ? (value >> 4) : (value & 0x0FFF);
			if (val >= 0x0FF8) return 0xFFFFFFF8;
			return val;
		};
	}

	bool FilesysFAT::set_fat_entry(uint32_t cluster, uint32_t value, FAT_TableScratch* fat_cache) {
		if (cluster < 2) return false;
		byte* fat_buffer = fat_cache ? fat_cache->buffer : buffer_fatable;
		uint32_t* cached_sector = fat_cache ? &fat_cache->current_sector : &current_sector;
		if (!fat_buffer) return false;
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
		
		if (*cached_sector != fat_sector) {
			storage->Read(fat_sector, fat_buffer);
			*cached_sector = fat_sector;
		}
		
		if (fat_type == 32) {
			value &= 0x0FFFFFFF;
			fat_buffer[ent_offset] = (byte)(value & 0xFF);
			fat_buffer[ent_offset + 1] = (byte)((value >> 8) & 0xFF);
			fat_buffer[ent_offset + 2] = (byte)((value >> 16) & 0xFF);
			fat_buffer[ent_offset + 3] = (byte)((fat_buffer[ent_offset + 3] & 0xF0) | ((value >> 24) & 0x0F));
		} else if (fat_type == 16) {
			fat_buffer[ent_offset] = (byte)(value & 0xFF);
			fat_buffer[ent_offset + 1] = (byte)((value >> 8) & 0xFF);
		} else {// 12
			uint32_t cur = ((uint32_t)fat_buffer[ent_offset]) | ((uint32_t)fat_buffer[ent_offset + 1] << 8);
			if (cluster & 0x01) {
				value = value << 4;
				cur = (cur & 0x000F) | (value & 0xFFF0);
			} else {
				cur = (cur & 0xF000) | (value & 0x0FFF);
			}
			fat_buffer[ent_offset] = (byte)(cur & 0xFF);
			fat_buffer[ent_offset + 1] = (byte)((cur >> 8) & 0xFF);
		}
		
		for (uint32_t i = 0; i < fat_count; i++) {
			uint32_t sec = fat_sector + (i * sectors_per_fat);
			storage->Write(sec, fat_buffer);
		}
		return true;
	}

	namespace {
		// One code point to UTF-16 units, a surrogate pair above the BMP
		static stduint fat_utf16_units_of(uint32_t rune, uint16_t* out_units) {
			if (rune >= 0x10000u && rune <= 0x10FFFFu) {
				const uint32_t offset = rune - 0x10000u;
				out_units[0] = (uint16_t)(0xD800u | (offset >> 10));
				out_units[1] = (uint16_t)(0xDC00u | (offset & 0x3FFu));
				return 2;
			}
			out_units[0] = (uint16_t)(rune > 0xFFFFu ? 0xFFFDu : rune);
			return 1;
		}

		// Characters allowed in an 8.3 field: uppercase ASCII, every other byte becomes '_'
		static char fat_short_name_char(char c) {
			const unsigned char raw = (unsigned char)c;
			if (raw < 0x20u || raw >= 0x7Fu) return '_';
			if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
			switch (c) {
			case ' ': case '"': case '*': case '+': case ',': case '/': case ':':
			case ';': case '<': case '=': case '>': case '?': case '[': case '\\':
			case ']': case '|':
				return '_';
			default:
				return c;
			}
		}
	}

	bool FilesysFAT::build_lfn_entries(const char* name, const FAT_DirEntry& short_entry,
		byte* out_entries, stduint capacity, stduint& out_count) {
		out_count = 0;
		if (!name || !out_entries) return false;
		String source(name);
		stduint unit_count = 0;
		for (auto rune : source.chars()) {
			unit_count += (rune >= 0x10000u) ? 2 : 1;
		}
		if (!unit_count || unit_count > 255) return false;
		const stduint piece_count = (unit_count + 12) / 13;
		if (piece_count > capacity) return false;
		const uint8_t checksum = fat_short_name_checksum(short_entry);
		for (stduint slot = 0; slot < piece_count; slot++) {
			const stduint order = piece_count - slot;
			FAT_LongDirEntry* entry = (FAT_LongDirEntry*)(out_entries + slot * 32);
			MemSet(entry, 0, 32);
			entry->order = (uint8_t)(order | (slot == 0 ? 0x40 : 0x00));
			entry->attr = 0x0F;
			entry->type = 0;
			entry->checksum = checksum;
			entry->cluster = 0;
			uint16_t units[13];
			stduint filled = 0;
			const stduint first_unit = (order - 1) * 13;
			const stduint last_unit = first_unit + 13;
			stduint position = 0;
			for (auto rune = source.chars_begin(); position < last_unit; ++rune) {
				if (rune == source.chars_end()) break;
				uint16_t produced_units[2];
				const stduint produced = fat_utf16_units_of(*rune, produced_units);
				for (stduint i = 0; i < produced && position < last_unit; i++, position++) {
					if (position < first_unit) continue;
					units[filled++] = produced_units[i];
				}
			}
			for (stduint i = 0; i < 13; i++) {
				const uint16_t unit = i < filled ? units[i] : (uint16_t)0xFFFF;
				if (i < 5) entry->name1[i] = unit;
				else if (i < 11) entry->name2[i - 5] = unit;
				else entry->name3[i - 11] = unit;
			}
		}
		out_count = piece_count;
		return true;
	}

	bool FilesysFAT::short_name_exists(uint32_t dir_cluster, const FAT_DirEntry& candidate,
		byte* sector_buffer, FAT_TableScratch* fat_cache) {
		if (!sector_buffer) sector_buffer = buffer_sector;
		if (!sector_buffer) return false;
		auto matches = [&](uint32_t sector) -> bool {
			if (!storage->Read(sector, sector_buffer)) return true;
			const int entries_per_sector = storage->Block_Size / 32;
			for (int i = 0; i < entries_per_sector; i++) {
				FAT_DirEntry* entry = (FAT_DirEntry*)&sector_buffer[i * 32];
				if ((byte)entry->name[0] == 0x00u) return false;
				if ((byte)entry->name[0] == 0xE5u) continue;
				if (entry->attribute.isLongName()) continue;
				if (entry->attribute.volume_id && !entry->attribute.directory) continue;
				bool same = true;
				for (int k = 0; k < 8 && same; k++) same = entry->name[k] == candidate.name[k];
				for (int k = 0; k < 3 && same; k++) same = entry->ext[k] == candidate.ext[k];
				if (same) return true;
			}
			return false;
		};
		if (dir_cluster == 0 && fat_type != 32) {
			const uint32_t start_sec = first_data_sector - root_dir_sectors;
			for (uint32_t s = 0; s < root_dir_sectors; s++) {
				if (matches(start_sec + s)) return true;
			}
			return false;
		}
		uint32_t cluster = dir_cluster;
		if (cluster == 0) cluster = root_cluster;
		int limit = 100000;
		while (cluster >= 2 && cluster < 0xFFFFFFF8 && limit-- > 0) {
			const uint32_t cluster_sec = getSector_foCluster(cluster);
			for (uint32_t s = 0; s < sectors_per_cluster; s++) {
				if (matches(cluster_sec + s)) return true;
			}
			cluster = get_fat_entry(cluster, fat_cache);
		}
		return false;
	}

	bool FilesysFAT::build_short_entry(const char* name, uint32_t dir_cluster,
		byte* sector_buffer, FAT_TableScratch* fat_cache, FAT_DirEntry& out_entry,
		bool& out_needs_lfn) {
		out_needs_lfn = true;
		if (!name || !*name) return false;
		const char* base = name;
		while (*base == '.') base++;
		if (!*base) return false;
		const char* dot = nullptr;
		for (const char* p = base; *p; p++) {
			if (*p == '.') dot = p;
		}
		char stem[8] = {};
		char ext[3] = {};
		stduint stem_len = 0;
		stduint ext_len = 0;
		if (dot) {
			for (const char* p = base; p < dot && stem_len < 8; p++) {
				const char ch = fat_short_name_char(*p);
				if (ch) stem[stem_len++] = ch;
			}
			for (const char* p = dot + 1; *p && ext_len < 3; p++) {
				const char ch = fat_short_name_char(*p);
				if (ch) ext[ext_len++] = ch;
			}
		}
		else {
			for (const char* p = base; *p && stem_len < 8; p++) {
				const char ch = fat_short_name_char(*p);
				if (ch) stem[stem_len++] = ch;
			}
		}
		if (!stem_len) return false;

		FAT_DirEntry candidate;
		auto compose = [&](const char* tail, stduint tail_len) {
			MemSet(&candidate, 0, sizeof(FAT_DirEntry));
			MemSet(candidate.name, ' ', 8);
			MemSet(candidate.ext, ' ', 3);
			stduint usable = 8 - tail_len;
			if (usable > stem_len) usable = stem_len;
			for (stduint i = 0; i < usable; i++) candidate.name[i] = stem[i];
			for (stduint i = 0; i < tail_len; i++) candidate.name[usable + i] = tail[i];
			for (stduint i = 0; i < ext_len; i++) candidate.ext[i] = ext[i];
			if ((byte)candidate.name[0] == 0xE5u) candidate.name[0] = (char)0x05;
		};
		auto accept = [&]() {
			MemSet(&out_entry, 0, sizeof(FAT_DirEntry));
			MemCopyN(out_entry.name, candidate.name, 8);
			MemCopyN(out_entry.ext, candidate.ext, 3);
			char rebuilt[13];
			fat_decode_short_name(out_entry, rebuilt);
			out_needs_lfn = StrCompare(rebuilt, base) != 0;
		};

		compose(nullptr, 0);
		if (!short_name_exists(dir_cluster, candidate, sector_buffer, fat_cache)) {
			accept();
			return true;
		}
		for (uint32_t serial = 1; serial < 1000000u; serial++) {
			char tail[8] = { '~' };
			char digits[8];
			stduint digit_count = 0;
			uint32_t value = serial;
			do {
				digits[digit_count++] = (char)('0' + (value % 10));
				value /= 10;
			} while (value && digit_count < 7);
			for (stduint i = 0; i < digit_count; i++) tail[1 + i] = digits[digit_count - 1 - i];
			compose(tail, 1 + digit_count);
			if (!short_name_exists(dir_cluster, candidate, sector_buffer, fat_cache)) {
				accept();
				out_needs_lfn = true;
				return true;
			}
		}
		return false;
	}

	uint32_t FilesysFAT::dir_next_sector(uint32_t dir_cluster, uint32_t sector,
		FAT_TableScratch* fat_cache) {
		if (!sector) return 0;
		if (dir_cluster == 0 && fat_type != 32) {
			const uint32_t start_sec = first_data_sector - root_dir_sectors;
			if (sector + 1 >= start_sec + root_dir_sectors) return 0;
			return sector + 1;
		}
		if (sector < first_data_sector) return 0;
		const uint32_t cluster_of_sector = (sector - first_data_sector) / sectors_per_cluster + 2;
		const uint32_t cluster_first = getSector_foCluster(cluster_of_sector);
		if (sector + 1 < cluster_first + sectors_per_cluster) return sector + 1;
		const uint32_t next_cluster = get_fat_entry(cluster_of_sector, fat_cache);
		if (next_cluster < 2 || next_cluster >= 0xFFFFFFF8) return 0;
		return getSector_foCluster(next_cluster);
	}

	bool FilesysFAT::dir_grow_chain(uint32_t dir_cluster, FAT_TableScratch* fat_cache) {
		if (dir_cluster == 0 && fat_type != 32) return false;
		uint32_t cluster = dir_cluster ? dir_cluster : root_cluster;
		if (cluster < 2) return false;
		int limit = 10000;
		while (limit-- > 0) {
			const uint32_t next = get_fat_entry(cluster, fat_cache);
			if (next >= 0xFFFFFFF8) break;
			if (next < 2) return false;
			cluster = next;
		}
		if (limit <= 0) return false;
		const uint32_t grown = find_free_cluster(fat_cache);
		if (!grown) return false;
		const uint32_t end_mark = (fat_type == 32) ? 0x0FFFFFFF :
			(fat_type == 16) ? 0xFFFF : 0xFFF;
		if (!set_fat_entry(cluster, grown, fat_cache)) return false;
		if (!set_fat_entry(grown, end_mark, fat_cache)) return false;
		byte* cleared = new byte[storage->Block_Size];
		if (!cleared) return false;
		MemSet(cleared, 0, storage->Block_Size);
		const uint32_t grown_sec = getSector_foCluster(grown);
		for (uint32_t s = 0; s < sectors_per_cluster; s++) {
			storage->Write(grown_sec + s, cleared);
		}
		delete[] cleared;
		return true;
	}

	bool FilesysFAT::alloc_dir_run(uint32_t dir_cluster, stduint count,
		uint32_t& out_sector, uint32_t& out_index, byte* sector_buffer,
		FAT_TableScratch* fat_cache) {
		out_sector = 0;
		out_index = 0;
		if (!count) return false;
		if (!sector_buffer) sector_buffer = buffer_sector;
		if (!sector_buffer) return false;
		const stduint slots_per_sector = storage->Block_Size / 32;
		const bool fixed_root = (dir_cluster == 0 && fat_type != 32);
		uint32_t sector = fixed_root ?
			(first_data_sector - root_dir_sectors) :
			getSector_foCluster(dir_cluster ? dir_cluster : root_cluster);
		if (!sector) return false;
		uint32_t run_sector = 0;
		uint32_t run_index = 0;
		stduint run_count = 0;
		for (int guard = 0; sector && guard < 100000; guard++) {
			if (!storage->Read(sector, sector_buffer)) return false;
			for (stduint i = 0; i < slots_per_sector; i++) {
				FAT_DirEntry* entry = (FAT_DirEntry*)&sector_buffer[i * 32];
				const bool free_slot = (byte)entry->name[0] == 0x00u ||
					(byte)entry->name[0] == 0xE5u;
				if (!free_slot) {
					run_count = 0;
					continue;
				}
				if (!run_count) {
					run_sector = sector;
					run_index = (uint32_t)i;
				}
				if (++run_count >= count) {
					out_sector = run_sector;
					out_index = run_index;
					return true;
				}
			}
			const uint32_t next = dir_next_sector(dir_cluster, sector, fat_cache);
			if (!next) {
				if (fixed_root) return false;
				if (!dir_grow_chain(dir_cluster, fat_cache)) return false;
				sector = dir_next_sector(dir_cluster, sector, fat_cache);
				continue;
			}
			sector = next;
		}
		return false;
	}

	bool FilesysFAT::write_dir_run(uint32_t dir_cluster, uint32_t start_sector,
		uint32_t start_index, const byte* entries, stduint count) {
		if (!start_sector || !entries || !count) return false;
		byte* buffer = new byte[storage->Block_Size];
		if (!buffer) return false;
		const stduint slots_per_sector = storage->Block_Size / 32;
		uint32_t sector = start_sector;
		stduint index = start_index;
		stduint done = 0;
		bool ok = true;
		while (done < count && ok) {
			if (!storage->Read(sector, buffer)) {
				ok = false;
				break;
			}
			while (index < slots_per_sector && done < count) {
				MemCopyN(&buffer[index * 32], entries + done * 32, 32);
				index++;
				done++;
			}
			ok = storage->Write(sector, buffer);
			if (done < count) {
				sector = dir_next_sector(dir_cluster, sector, nullptr);
				if (!sector) ok = false;
				index = 0;
			}
		}
		delete[] buffer;
		return ok && done == count;
	}

	bool FilesysFAT::erase_dir_run(uint32_t dir_cluster, uint32_t start_sector,
		uint32_t start_index, stduint count) {
		if (!start_sector || !count) return false;
		byte* buffer = new byte[storage->Block_Size];
		if (!buffer) return false;
		const stduint slots_per_sector = storage->Block_Size / 32;
		uint32_t sector = start_sector;
		stduint index = start_index;
		stduint done = 0;
		bool ok = true;
		while (done < count && ok) {
			if (!storage->Read(sector, buffer)) {
				ok = false;
				break;
			}
			while (index < slots_per_sector && done < count) {
				buffer[index * 32] = 0xE5u;
				index++;
				done++;
			}
			ok = storage->Write(sector, buffer);
			if (done < count) {
				sector = dir_next_sector(dir_cluster, sector, nullptr);
				if (!sector) ok = false;
				index = 0;
			}
		}
		delete[] buffer;
		return ok && done == count;
	}
}
