// ASCII C/C++ TAB4 CRLF
// Docutitle: (Format: File-System) File Allocation Table
// Codifiers: @dosconio: 20240426 ~ <Last-check> 
// Attribute: <ArnCovenant> Any-Architect <Environment> <Reference/Dependence>
// Copyright: UNISYM, under Apache License 2.0
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

#ifndef _INC_FAT
#define _INC_FAT

// FAT12 FAT16 FAT32 exFAT

#include "../../../cpp/trait/StorageTrait.hpp"
#include "../../../cpp/trait/FilesysTrait.hpp"

// file:///E:/book/STANDARD/FAT32.pdf

// 40 bytes
#define FAT_BootSectorHead \
	uint8_t  jmpBoot[3]; \
	char     OEM[8]; \
	uint16_t bytes_per_sector; \
	uint8_t  sectors_per_cluster; \
	uint16_t reserved_sectors; \
	uint8_t  fat_count; \
	uint16_t root_entries; \
	uint16_t total_sectors_16; \
	uint8_t  media_type; \
	uint16_t sectors_per_fat_16; \
	uint16_t sectors_per_track; \
	uint16_t head_count; \
	uint32_t hidden_sectors; \
	uint32_t total_sectors_32

#define FAT_BootSectorTail \
	uint8_t  drive_number; \
	uint8_t  reserved1; \
	uint8_t  boot_signature; \
	uint32_t volume_id; \
	char     volume_label[11]; \
	char     fs_type[8]

_PACKED(struct) FAT_BootSector12 { // for FAT12/16
	FAT_BootSectorHead;
	FAT_BootSectorTail;
};
_PACKED(struct) FAT_BootSector32 {
	FAT_BootSectorHead;
	uint32_t sectors_per_fat_32;
	uint16_t flags;
	uint16_t fat_version;
	uint32_t root_cluster;
	uint16_t fs_info_sector;
	uint16_t backup_boot_sector;
	uint8_t  reserved[12];
	FAT_BootSectorTail;
	// uint8_t  boot_code[420];
	// uint16_t boot_signature_55aa;
};

_PACKED(struct) FAT_Attributes {
	byte read_only    : 1;
	byte hidden       : 1;
	byte system_file  : 1;
	byte volume_id    : 1;
	byte directory    : 1;
	byte archive      : 1;
	byte reserved1    : 1;
	byte reserved2    : 1;
};

_PACKED(struct) FAT_DirEntry {
	char     name[8];
	char     ext[3];
	union { uint8_t  attr; FAT_Attributes _attr; };
	uint8_t  nt_reserved;
	uint8_t  create_time_tenth;
	uint16_t create_time;
	uint16_t create_date;
	uint16_t access_date;
	uint16_t cluster_high;
	uint16_t write_time;
	uint16_t write_date;
	uint16_t cluster_low;
	uint32_t file_size;
};

_PACKED(struct) FAT_LongDirEntry {
	uint8_t  order;
	wchar_t  name1[5];
	uint8_t  attr;
	uint8_t  type;
	uint8_t  checksum;
	wchar_t  name2[6];
	uint16_t cluster;
	wchar_t  name3[2];
};


#ifdef _INC_CPP

namespace uni {

	struct FAT_FileHandle {
		uint32_t start_cluster;
		uint32_t current_cluster;
		uint32_t current_offset;
		uint32_t size;
		bool     is_dir;
		uint32_t dir_sector;
		uint32_t dir_index;
	};

	struct FAT_DirInfo {
		char     name[256];
		uint32_t size;
		uint32_t attr;
		uint32_t create_time;
		uint32_t access_time;
		uint32_t modify_time;
		uint32_t start_cluster;
	};

	// ----

	class FilesysFAT : public FilesysTrait
	{
	public:
		uint32_t fat_type;  // 12, 16, 32
		// FAT_BootSector bs;
		uint32_t total_sectors;
		uint32_t sectors_per_fat;
		uint32_t root_dir_sectors;
		uint32_t first_data_sector;
		uint32_t first_fat_sector;
		uint32_t data_sectors;
		bool     fs_loaded = false;
		//
		uint32_t root_cluster;
		uint32_t sectors_per_cluster;
		byte* buffer_sector;
		uint32_t current_sector;
		unsigned partid;
		// std::map<void*, FAT_FileHandle> open_files;
		uint32_t next_handle_id = 1;
		uint32_t total_clusters;// = data_sectors / sectors_per_cluster
		byte* buffer_fatable;
	public:
		FilesysFAT(uint32_t fatype, StorageTrait& s, byte* buffer, unsigned dev)
			: fat_type(fatype), buffer_sector(buffer), partid(dev)
		{
			storage = &s;
		}

		//{unchk}
		// [moreinfo]
		// - 0x00 stduint cluster_sz
		virtual bool makefs(rostr vol_label, void* moreinfo = 0) override;

		// []
		// - sectors_per_fat_32 may be 0
		// - root_dir_sectors may be 0
		virtual bool loadfs(void* moreinfo = 0) override;

		// create file/folder. usize{isfolder}
		virtual bool create(rostr fullpath, stduint flags, void* exinfo, rostr linkdest = 0) override;

		// remove file/folder
		virtual bool remove(rostr pathname) override;

		// return the handler of the path/file (nullptr for failure) , `moreinfo` will get the proper
		virtual void* search(rostr fullpath, void* moreinfo) override;

		virtual bool proper(void* handler, stduint cmd, const void* moreinfo = 0) override;// set proper

		virtual bool enumer(void* dir_handler, _tocall_ft _fn) override;

		virtual stduint readfl(void* fil_handler, Slice file_slice, byte*) override;// read file

		virtual stduint writfl(void* fil_handler, Slice file_slice, const byte*) override;// write file
		//
		virtual rostr nextlink(rostr fullpath) override { return NULL; }// if a hard/soft link

	public:
		uint32_t get_fat_entry(uint32_t cluster);
	};


	

}

#endif

#endif