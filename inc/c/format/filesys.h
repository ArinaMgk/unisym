// ASCII C/C++ TAB4 CRLF
// Docutitle: (Format) Filesys
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

#ifndef _INC_FILESYS
#define _INC_FILESYS

#ifdef _INC_CPP
#include "../../cpp/trait/FilesysTrait.hpp"
#endif

enum FilesysType {
	FILESYS_FAT12		= 0x01,		// FAT12 (Usually for floppy disks)
	FILESYS_FAT16_CHS	= 0x04,		// FAT16B (Partitions 32MB below)
	FILESYS_EXT         = 0x05,		// Extended partition
	FILESYS_FAT16_CHSX  = 0x06,		// FAT16B (Partitions 32MB to 2GB)
	FILESYS_NTFS		= 0x07,		// Windows NTFS or exFAT
	FILESYS_FAT32_CHS	= 0x0B,		// FAT32 using CHS addressing
	FILESYS_FAT32_LBA	= 0x0C,		// FAT32 using Logical Block Addressing (LBA)
	FILESYS_FAT16_LBA	= 0x0E,		//
	FILESYS_LINUX_SWAP	= 0x82,		// Linux Swap partition
	FILESYS_LINUX_NATIVE= 0x83,		// Linux Native (Ext2, Ext3, Ext4)
	FILESYS_EFI_SYS		= 0xEF,		// EFI System Partition (ESP)
};



#endif
