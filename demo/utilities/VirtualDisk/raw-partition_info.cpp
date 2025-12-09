// ASCII C++ TAB4 CRLF
// Docutitle: (Instance.Utility) Virtual-Disk-File(RAW)/Partition Lister
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
//{TEMP} Fixed Size VHD
//{TEMP} DbgCommand: ~ D:/bin/fixedz.img
#include <stdio.h>
#include <cpp/unisym>
#include <c/consio.h>
#include <c/datime.h>
#include <c/format/filesys/FAT12.h>
#include <c/format/data/VHD.h>

using namespace uni;

PartitionTableX86 pt[4];
static unsigned logi_part_id = 5;

FILE* f;
void read_logi(stduint fposi, stduint ext_start, stduint par_start)
{
	PartitionTableX86 loc_pt[2];
	fseek(f, fposi + 0x1BE, SEEK_SET);
	fread(loc_pt, sizeof(PartitionTableX86), 2, f);
	for0a(i, loc_pt) {
		//ploginfo("%d", i);
		auto p = loc_pt[i];
		bool valid = p.status == 0x80 || p.status == 0;
		bool bootable = p.status == 0x80;
		stduint lba_start = p.lba_start + par_start;
		if (!p.lba_count) {
			//ploginfo("End", i);
			//if (!i) return false;
			break;
		}
		if (p.type != 0x05) {
			printf("%02hhu %s %02hhxH~%02hhxH %02hhxH~%02hhxH %03hxH~%03hxH 0x%02hhx %u..%u", (byte)logi_part_id++,
				valid ? bootable ? "[boot]" : "[yes ]" : "[ no ]",
				p.head_start, p.head_end,
				p.sector_start, p.sector_end,
				p.cylinder_start, p.cylinder_end,
				p.type,
				lba_start, lba_start + p.lba_count
			);
		}
		if (i && p.type == 0x05) {
			//ploginfo("Cont.");
			lba_start = p.lba_start + ext_start;
			read_logi(lba_start * 512, ext_start, lba_start);
		}

		outsfmt("\n");

	}
	int end = 0;
	//return true; // if not end;
}

int main(int _c, char** _v)
{
	
	if (_c != 2) {
		plogerro("Bad command");
		return -1;
	}
	
	f = fopen(_v[1], "r");
	if (!f) {
		plogerro("Cannot open file");
		return -1;
	}

	fseek(f, 0x1BE, SEEK_SET);
	fread(pt, sizeof(PartitionTableX86), 4, f);
	outsfmt("no proper HEAD    SECTOR  CYLINDER  TYPE LBA\n");
	for0(i, 4) {
		auto p = pt[i];
		bool valid = p.status == 0x80 || p.status == 0;
		bool bootable = p.status == 0x80;
		if (!p.lba_count) break;
		printf("%02hhu %s %02hhxH~%02hhxH %02hhxH~%02hhxH %03hxH~%03hxH 0x%02hhx %u..%u", (byte)i + 1,
			valid ? bootable ? "[boot]" : "[yes ]" : "[ no ]",
			p.head_start, p.head_end,
			p.sector_start, p.sector_end,
			p.cylinder_start, p.cylinder_end,
			p.type,
			p.lba_start, p.lba_start + p.lba_count
		);
		outsfmt("\n");
		if (p.type == 0x05) {
			read_logi(p.lba_start * 512, p.lba_start, p.lba_start);
		}
	}


	fclose(f); f = nullptr;
	return malc_count;
}
