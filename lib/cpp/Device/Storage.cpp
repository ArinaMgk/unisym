// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Stroage)
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
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

#include "../../../inc/cpp/unisym"
#include "../../../inc/cpp/string"
#include "../../../inc/cpp/trait/StorageTrait.hpp"

#pragma GCC optimize("O0")
#define DRV_OF_DEV
namespace uni {

	stduint BlockTrait::Read(stduint linear_offset, void* dest, stduint length, byte* buffer) {
		stduint block_size = Block_Size ? Block_Size : 512;
		stduint crt_block = linear_offset / block_size;
		stduint block_offset = linear_offset % block_size;
		byte* out = cast<byte*>(dest);
		stduint ret = 0;
		while (length) {
			bool state = Read(crt_block, buffer);
			if (!state) return ret;
			stduint copy_size = block_size - block_offset;
			if (copy_size > length) {
				copy_size = length;
			}
			MemCopyN(out, buffer + block_offset, copy_size);
			out += copy_size;
			ret += copy_size;
			length -= copy_size;
			crt_block++;
			block_offset = 0; // read from 0 at 
		}
		return ret;
	}

	bool DiscPartition::Read(stduint BlockIden, void* Dest) {
		if (!slice.address && !slice.length) renew_slice();
		// ploginfo("DiscPartition::Read %u:%u -> %[x]", DRV_OF_DEV(self.device), BlockIden + slice.address, Dest);
		return base->Read(BlockIden + slice.address, Dest);
	}

	bool DiscPartition::Write(stduint BlockIden, const void* Sors) {
		if (!slice.address && !slice.length) renew_slice();
		return base->Write(BlockIden + slice.address, Sors);
	}

	void DiscPartition::Partition(StorageTrait& base, HD_Info& hdi, byte* psector, unsigned device, bool primary_but_logical)
	{
		unsigned drive = DRV_OF_DEV(device);
		// ploginfo("%s: drive%u", __FUNCIDEN__, drive);
		alignas(16) PartitionTableX86 part_tbl[4] = {};
		if (primary_but_logical) {
			if (LoadGPTPartitions(base, hdi, psector)) return;
			hdi.scheme_kind = PartitionSchemeKind::MBR;
			hdi.ResetUnifiedParts();
			// get_partition_table(drv, 0, part_tbl);
			base.Read(0, psector);
			MemCopyN(part_tbl, psector + MBR_PARTITION_TABLE_OFFSET, sizeof(part_tbl));
			for0(i, NR_PART_PER_DRIVE) {
				if (part_tbl[i].type == Part_NO_PART)
					continue;
				PartitionSlice prim = {};
				prim.address = part_tbl[i].lba_start;
				prim.length = part_tbl[i].lba_count;
				prim.sys_id = part_tbl[i].type;
				if (part_tbl[i].type != Part_EX_PART) {
					hdi.AppendPart(prim);
				}
				// plogwarn("p %u: %u-%u", i + 1, part_tbl[i].lba_start, part_tbl[i].lba_count);
				if (part_tbl[i].type ==Part_EX_PART)
					Partition(base, hdi, psector, prim.address, false);
			}
		}
		else {
			int ext_start_sect = device;
			int s = ext_start_sect;
			for0(i, NR_SUB_PER_PART) {
				// get_partition_table(drv, s, part_tbl);
				base.Read(s, psector);
				MemCopyN(part_tbl, psector + MBR_PARTITION_TABLE_OFFSET, sizeof(part_tbl));
				PartitionSlice logi = {};
				logi.address = s + part_tbl[0].lba_start;
				logi.length = part_tbl[0].lba_count;
				logi.sys_id = part_tbl[0].type;
				if (logi.sys_id != Part_NO_PART && logi.length) {
					hdi.AppendPart(logi);
				}
				// plogwarn("l %u: %u-%u", dev_nr, logi.address, logi.address + logi.length);
				s = ext_start_sect + part_tbl[1].lba_start;
				if (part_tbl[1].type == Part_NO_PART) {
					break;
				}
			}
		}
	}

}

