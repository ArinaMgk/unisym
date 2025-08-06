// ASCII C TAB4 CRLF
// Docutitle: (Format.Data) Virtual-Disk-File
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


#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include "../../../../inc/c/format/data/VHD.h"
#include "../../../../inc/c/ustring.h"

namespace uni {
	VirtualDisk_VHD::VirtualDisk_VHD(rostr hostfile)
	{
		Block_buffer = nullptr;
		Block_Size = 512;
		footer = nullptr;
		fp = fopen(hostfile, "rb+");
		if (!fp) {
			valid = false;
			return;
		}
		Letvar(pf, FILE*, fp);
		fseek(pf, 0, SEEK_END);
		stduint total_size = ftell(pf);
		if (total_size < 512) {
			valid = false;
			return;
		}
		stduint avail_size = (total_size - 511) & ~_IMM(512);
		// cons->OutFormat("AVAIL   SIZE  %[u]B (%s)\n", avail_size, text_size(str, avail_size));

		fseek(pf, -long(total_size - avail_size + 1), SEEK_END);
		footer = malcof(VHD_Footer);
		if (!footer) {
			valid = false;
			return;
		}
		(void)fread(footer, sizeof(VHD_Footer), 1, pf);
		//
		bool want_fixed = MemReverseL(footer->disk_type) == 2;// FIXED
		//
		if (0 && MemReverseL(footer->filefmt_ver) != 0x00010000) {
			//plogerro("Not a supported VHD version");
			return;
		}
		//
		valid = true;
	}

	VirtualDisk_VHD::~VirtualDisk_VHD() {
		if (fp) fclose((FILE*)fp);
		fp = nullptr;
		if (footer) mfree(footer);
		//if (Block_buffer) mfree(Block_buffer);
	}

	stduint VirtualDisk_VHD::getUnits() {
		if (!valid) return 0;
		return MemReverseL(footer->curr_size) / Block_Size;
	}
	static rostr vhd_disk_type[] = {
	"None",
	"Reserved (deprecated)",
	"Fixed",
	"Dynamic",
	"Differencing",
	"Reserved (deprecated)",
	"Reserved (deprecated)"
	};
	rostr VirtualDisk_VHD::getTypeIdentifier() {
		if (!valid || MemReverseL(footer->disk_type) >= numsof(vhd_disk_type)) return 0;
		return vhd_disk_type[MemReverseL(footer->disk_type)];
	}

	bool VirtualDisk_VHD::Read(stduint BlockIden, void* Dest)//{TEMP} only for FIXED
	{
		if (BlockIden >= getUnits()) return false;
		fseek((FILE*)fp, Block_Size * BlockIden, SEEK_SET);//{stdc} long-limit
		(void)fread(Dest, 512, 1, (FILE*)fp);
		return false;
	}


	bool VirtualDisk_VHD::Write(stduint BlockIden, const void* Sors)//{TEMP} only for FIXED
	{
		if (BlockIden >= getUnits()) return false;
		fseek((FILE*)fp, Block_Size * BlockIden, SEEK_SET);//{stdc} long-limit
		fwrite(Sors, 512, 1, (FILE*)fp);
		return false;
	}

	int VirtualDisk_VHD::operator[](uint64 bytid)
	{
		bool alloced = false;
		if (!valid) return -1;
		if (!Block_buffer) 
		{
			alloced = true;
			Block_buffer = malc(512);
		}
		Read(bytid / Block_Size, Block_buffer);
		char ch = ((char*)Block_buffer)[bytid % Block_Size];
		if (alloced) mfree(Block_buffer);
		return ch;
	}

}



