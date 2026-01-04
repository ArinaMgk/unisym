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
		ploginfo("Making FAT%u on part %u (buffer %[x])", fat_type, partid, buffer_sector);
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
		ploginfo("loadfs buffer_sector=%[x]", buffer_sector);
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
		self.total_sectors = bs.total_sectors_16 ? bs.total_sectors_16 : bs.total_sectors_32;

		sectors_per_fat = (fat_type == 32 || bs.sectors_per_fat_32) ? bs.sectors_per_fat_32 : bs.sectors_per_fat_16;
		if (!fat_type) {
			if (bs.sectors_per_fat_32) {
				fat_type = 32;
			} else if (bs.sectors_per_fat_16 != 0) {
				uint32_t root_dir_sectors = ((bs.root_entries * 32) + 
					(bs.bytes_per_sector - 1)) / bs.bytes_per_sector;
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
			root_dir_sectors = ((bs.root_entries * 32) + (bs.bytes_per_sector - 1)) / bs.bytes_per_sector;
			first_data_sector = bs.reserved_sectors + (bs.fat_count * sectors_per_fat) + root_dir_sectors;
		}
		data_sectors = total_sectors - first_data_sector;
		total_clusters = data_sectors / bs.sectors_per_cluster;
		// storage->Block_Size is followed by hardware, not _bytesPerSector
		root_cluster = bs.root_cluster;
		sectors_per_cluster = bs.sectors_per_cluster;
		return fs_loaded = true;
	}

	bool FilesysFAT::create(rostr fullpath, stduint flags, void* exinfo, rostr linkdest) {
		
	}

	bool FilesysFAT::remove(rostr pathname) {
		
	}

	static FAT_FileHandle* path_to_handle(FilesysFAT& fs, FAT_FileHandle& handle, rostr fullpath);
	static uint32_t cluster_to_sector(FilesysFAT& fs, uint32_t cluster);
	void* FilesysFAT::search(rostr fullpath, void* moreinfo) {
		// moreinfo[0] --> FAT_FileHandle
		// moreinfo[1] --> FAT_DirInfo? (? means optional, set nullptr if not used)
		//
		if (fat_type != 32) return nullptr;
		if (!storage || !buffer_sector) {
			error_number = 1;
			return nullptr;
		}
		FAT_FileHandle* handle = (FAT_FileHandle*)((stduint*)moreinfo)[0];
		FAT_FileHandle hand_body;
		FAT_FileHandle* phandle = path_to_handle(self, hand_body, fullpath);
		*handle = *phandle;
		if (!StrCompare("/", fullpath)) {
			return handle;
		}
		// Seek the file
		uint32_t cluster = handle->start_cluster;
		while (cluster < 0xFFFFFFF8) {
			uint32_t sector = cluster_to_sector(self, cluster);
			if (!storage->Read(sector, buffer_sector)) {
				error_number = 2;
				return nullptr;
			}
			for (int i = 0; i < 16; i++) {
				FAT_DirEntry* entry = (FAT_DirEntry*)&buffer_sector[i * 32];
				if (entry->name[0] == 0) { // end of dir
					return nullptr;
				}
				if (entry->name[0] == 0xE5) continue; // deleted
				// ploginfo("%x %s", entry->name, entry->name);
				// cmp name
				char short_name[13];
				MemSet(short_name, ' ', 12);
				//
				int name_len = 0;
				while (name_len < 8 && entry->name[name_len] != ' ') {
					short_name[name_len] = entry->name[name_len];
					name_len++;
				}
				
				if (entry->ext[0] != ' ') {
					short_name[name_len++] = '.';
					for (int j = 0; j < 3; j++) {
						short_name[name_len++] = entry->ext[j];
					}
				}
				short_name[name_len++] = 0;
				
				const char* target = StrIndexCharRight(fullpath, '/');
				if (!target) target = fullpath;
				if (!StrCompareInsensitive(short_name, target)) {
					handle->start_cluster = (entry->cluster_high << 16) | entry->cluster_low;
					handle->current_cluster = handle->start_cluster;
					handle->size = entry->file_size;
					handle->is_dir = (entry->attr & 0x10) != 0;
					handle->dir_sector = sector;
					handle->dir_index = i;
					
					if (((stduint*)moreinfo)[1]) {
						FAT_DirInfo* info = (FAT_DirInfo*)((stduint*)moreinfo)[1];
						StrCopy(info->name, short_name);
						info->size = entry->file_size;
						info->attr = entry->attr;
						info->start_cluster = handle->start_cluster;
					}
					return handle;
				}
			}
			cluster = get_fat_entry(cluster);
		}
		return nullptr;
	}

	bool FilesysFAT::proper(void* handler, stduint cmd, const void* moreinfo) {
		_TODO
	}

	bool FilesysFAT::enumer(void* dir_handler, _tocall_ft _fn) {
		FAT_FileHandle* fh = (FAT_FileHandle*)dir_handler;
		if (!fh->is_dir) return false;
		uint32_t cluster = fh->start_cluster;
		uint32_t count = 0;
		while (cluster < 0xFFFFFFF8) {
			uint32_t sector = cluster_to_sector(self, cluster);	
			if (!storage->Read(sector, buffer_sector)) return false;
			for0 (i, 16) {
				FAT_DirEntry* entry = &cast<FAT_DirEntry*>(buffer_sector)[i];
				if (entry->name[0] == 0) return true; // end of dir
				if (entry->name[0] == 0xE5) continue; // removed
				if (entry->attr == 0x0F) continue; //{} FAT_LongDirEntry
				ploginfo("FilesysFAT::enumer %s", entry->name);
				if (_fn) _fn((void*)_IMM(entry->_attr.directory), entry);
				count++;
			}
			cluster = get_fat_entry(cluster);
		}
		return true;
	}

	stduint FilesysFAT::readfl(void* fil_handler, Slice file_slice, byte* dest) {
		FAT_FileHandle* fh = (FAT_FileHandle*)fil_handler;
		if (fh->is_dir) return 0;
		
		uint64_t offset = file_slice.address;
		uint64_t to_read = file_slice.length;
		uint64_t total_read = 0;// ret
		MIN(to_read, offset + fh->size);
		// ploginfo("FilesysFAT::readfl %uBlks(0x%[64H],0x%[64H])", sectors_per_cluster, offset, to_read);

		// evaluate the start cluster
		uint32_t cluster = fh->start_cluster;
		uint32_t cluster_size = sectors_per_cluster * storage->Block_Size;

		// skip the start cluster
		while (offset >= cluster_size) {
			ploginfo("offset >= cluster_size");
			offset -= cluster_size;
			cluster = get_fat_entry(cluster);
			if (cluster >= 0xFFFFFFF8) return total_read; // over the file
		}
		
		while (to_read > 0) {
			uint32_t sector = cluster_to_sector(self, cluster);
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

	stduint FilesysFAT::writfl(void* fil_handler, Slice file_slice, const byte*) {
		
	}

	// ----

	static FAT_FileHandle* path_to_handle(FilesysFAT& fs, FAT_FileHandle& handle, rostr fullpath)
	{
		_TEMP; // ploginfo("path_to_handle %[x]", &handle);
		handle.start_cluster = fs.root_cluster ? fs.root_cluster : 2;
		handle.current_cluster = handle.start_cluster;
		handle.current_offset = 0;
		handle.is_dir = true;
		return &handle;
	}

	static uint32_t cluster_to_sector(FilesysFAT& fs, uint32_t cluster)
	{
		if (cluster < 2) return 0;
		return ((cluster - 2) * fs.sectors_per_cluster) + fs.first_data_sector;
	}

	uint32_t FilesysFAT::get_fat_entry(uint32_t cluster)
	{
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
			return *(uint16_t*)&buffer_fatable[ent_offset];
		} else {// 12
			uint16_t value = *(uint16_t*)&buffer_fatable[ent_offset];
			if (cluster & 0x01)
				return value >> 4;
			else
				return value & 0x0FFF;
		}
	}


}
