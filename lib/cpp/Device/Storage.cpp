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

#define DRV_OF_DEV
namespace uni {
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
		PartitionTableX86 part_tbl[4] = {};
		if (primary_but_logical) {
			// get_partition_table(drv, 0, part_tbl);
			base.Read(0, psector);
			MemCopyN(part_tbl, psector + MBR_PARTITION_TABLE_OFFSET, sizeof(part_tbl));
			int nr_prim_parts = 0;
			for0(i, NR_PART_PER_DRIVE) {
				if (part_tbl[i].type == Part_NO_PART)
					continue;
				nr_prim_parts++;
				hdi.primary[i + 1].address = part_tbl[i].lba_start;
				hdi.primary[i + 1].length = part_tbl[i].lba_count;
				// plogwarn("p %u: %u-%u", i + 1, part_tbl[i].lba_start, part_tbl[i].lba_count);
				if (part_tbl[i].type ==Part_EX_PART)
					Partition(base, hdi, psector, device + i + 1, false);
			}
			// assert(nr_prim_parts != 0);
		}
		else {
			int j = device % NR_PRIM_PER_DRIVE; /* 1~4 */
			int ext_start_sect = hdi.primary[j].address;
			int s = ext_start_sect;
			int nr_1st_sub = (j - 1) * NR_SUB_PER_PART; /* 0/16/32/48 */
			for0(i, NR_SUB_PER_PART) {
				// get_partition_table(drv, s, part_tbl);
				base.Read(s, psector);
				MemCopyN(part_tbl, psector + MBR_PARTITION_TABLE_OFFSET, sizeof(part_tbl));
				int dev_nr = nr_1st_sub + i;/* 0~15/16~31/32~47/48~63 */
				hdi.logical[dev_nr].address = s + part_tbl[0].lba_start;
				hdi.logical[dev_nr].length = part_tbl[0].lba_count;
				// plogwarn("l %u: %u-%u", dev_nr, hdi.logical[dev_nr].address, hdi.logical[dev_nr].address + hdi.logical[dev_nr].length);
				s = ext_start_sect + part_tbl[1].lba_start;
				if (part_tbl[1].type == Part_NO_PART) {
					break;
				}
			}
		}
	}

}

