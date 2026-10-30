// ASCII C/C++ TAB4 CRLF
// Docutitle: (Format) Filesys Partition
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect <Environment> <Reference/Dependence>
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
#ifndef _INCPP_FILESYS_PARTITION
#define _INCPP_FILESYS_PARTITION

#include "../../string"

enum class ClassicFilesysType : byte {
	None,
	FAT12 = 0x01,		// FAT12 (Usually for floppy disks)
	FAT16_CHS = 0x04,	// FAT16B (Partitions 32MB below)
	EXT = 0x05,			// Extended partition
	FAT16_CHSX = 0x06,	// FAT16B (Partitions 32MB to 2GB)
	NTFS = 0x07,		// Windows NTFS or exFAT
	FAT32_CHS = 0x0B,	// FAT32 using CHS addressing
	FAT32_LBA = 0x0C,	// FAT32 using Logical Block Addressing (LBA)
	FAT16_LBA = 0x0E,	//
	LINUX_SWAP = 0x82,	// Linux Swap partition
	LINUX_NATIVE = 0x83,// Linux Native (Ext2, Ext3, Ext4)
	GPT_PROTECTIVE = 0xEE,// GPT protective MBR entry
	EFI_SYS = 0xEF,		// EFI System Partition (ESP)
};

namespace uni {
	struct PartitionSlice : public Slice {
		ClassicFilesysType sys_id;
	};
}

_PACKED(struct) PartitionTableX86 {
	// boot indicator
	// bits 6-0 are zero: Bit 7 is the active partition flag
	// else: the drive number of the drive to boot so the active partition is always found on drive 80H, the first hdisk
	byte status;
	//
	byte head_start;
	word sector_start : 6;
	word cylinder_start : 10;
	ClassicFilesysType type;// system id, {} can list an enum class
	byte head_end;
	word sector_end : 6;
	word cylinder_end : 10;
	uint32 lba_start;
	uint32 lba_count;
};//{TODO} details

//

#define NR_PART_PER_DRIVE    4 // 4 primary partitions per drive
#define NR_SUB_PER_PART     16 // 16 logical partitions per primary partition
#define NR_SUB_PER_DRIVE    (NR_SUB_PER_PART * NR_PART_PER_DRIVE)// 64
#define NR_PRIM_PER_DRIVE    (NR_PART_PER_DRIVE + 1) // 5
#define MAX_PARTITIONS      128

enum class PartitionSchemeKind : byte {
	Unknown = 0,
	MBR,
	GPT,
	GPT_Backup,
};

struct GptPartitionMeta {
	byte type_guid[16];
	byte uniq_guid[16];
	uint64 attributes;
};

struct HD_Info {
	uni::PartitionSlice whole_disk;
	PartitionSchemeKind scheme_kind;
	uint32 part_count;
	uint32 part_overflow;
	uni::PartitionSlice parts[MAX_PARTITIONS];
	GptPartitionMeta gpt_meta[MAX_PARTITIONS];

	void ResetUnifiedParts() {
		part_count = 0;
		part_overflow = 0;
		MemSet(parts, 0, sizeof(parts));
		MemSet(gpt_meta, 0, sizeof(gpt_meta));
	}
	bool AppendPart(const uni::PartitionSlice& slice) {
		if (part_count < MAX_PARTITIONS) {
			parts[part_count++] = slice;
			return true;
		}
		part_overflow++;
		return false;
	}
};


inline uni::PartitionSlice GetPartitionSlice(const HD_Info& hdi, unsigned dev) {
	if (dev == 0) return hdi.whole_disk;
	if (dev <= hdi.part_count) return hdi.parts[dev - 1];
	uni::PartitionSlice slice = {};
	return slice;
}



#endif
