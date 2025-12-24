// ASCII C++ TAB4 CRLF
// Docutitle: (Instance.Utility) Virtual-Disk-File/{VHD & FAT12} 
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
//{TEMP} DbgCommand: ~ D:/bin/fixed.vhd
#include <stdio.h>
#include <cpp/unisym>
#include <c/consio.h>
#include <c/datime.h>
#include <c/format/filesys/FAT12.h>
#include <c/format/data/VHD.h>

using namespace uni;


String text_size(stduint m) {
	char unit[]{ ' ', 'K', 'M', 'G', 'T' };
	int level = 0;
	double mem = m;
	while (mem > 1024) {
		level++;
		mem /= 1024;
	}
	return String::newFormat("%lf %cB", mem, unit[level]);
}



int print_vhd(OstreamTrait* cons, rostr filename, VirtualDisk_VHD& vhd)
{
	String str;
	if (!vhd) return -1;
	VHD_Footer& footer = *vhd.getFooter();

	cons->OutFormat("FIRST CREATOR ");
	for0(i, 8) if (!footer.cookie[i]) break; else cons->OutChar(footer.cookie[i]);
	cons->OutChar('\n');

	cons->OutFormat("FEATURES      ");// big endian
	uint32 feature = MemReverseB(footer.feature);
	if (feature & 0b10); else {
		plogerro("Bad feature in %s", filename);
		return -1;
	}
	feature &= ~_IMM(0b10);
	if (!feature) cons->OutFormat("None");
	if (feature & 0b01) cons->OutFormat("[Temp]");
	cons->OutChar('\n');


	uint64 data_offset = footer.data_offset;
#if !__ENDIAN__
	MemReverse((char*)&data_offset, byteof(data_offset));
#endif
	if (data_offset == ~(uint64)0) {
		//ploginfo("Fixed VHD.");
	}
	else {
		plogerro("Not support dynamic/differ VHD");
		return -1;
	}

	uint32 time_stamp = footer.time_stamp;
#if !__ENDIAN__
	MemReverse((char*)&data_offset, byteof(data_offset));
#endif

	cons->OutFormat("CREATE  APP   ");
	for0(i, 4) cons->OutChar(footer.creator_app[i]);
	cons->OutChar('\n');

	uint32 creator_ver = footer.creator_ver;
#if !__ENDIAN__
	MemReverse((char*)&creator_ver, byteof(creator_ver));
#endif
	cons->OutFormat("CREATE  VERS. 0x%[32H]\n", creator_ver);

	cons->OutFormat("CREATE  HOST  ");
	for0(i, 4) cons->OutChar(footer.creator_host[i]);
	cons->OutChar('\n');


	cons->OutFormat("CURRENT SIZE  %s (Origin %s) \n",
		text_size(vhd.getUnits() * vhd.Block_Size).reference(),
		text_size(MemReverseB(footer.orig_size)).reference()
	);

	word cylinder = *(word*)&footer.disk_geometry;
	byte heads = ((byte*)&footer.disk_geometry)[2];
	byte sectorspc = ((byte*)&footer.disk_geometry)[3];
	cons->OutFormat("GEOMETRY      CYLN=%u, HEAD=%u, SEC/C=%u\n",
		cylinder, heads, sectorspc);


	cons->OutFormat("DISK    TYPE  %s\n", vhd.getTypeIdentifier());

	cons->OutFormat("CHECK SUM     0x%[32H]\n", MemReverseB(footer.checksum));

	auto unique_id = footer.unique_id;
	cons->OutFormat("UUID          ");
	for0(i, 16) cons->OutFormat("%[8H] ", footer.unique_id[i]);
	cons->OutChar('\n');

	auto saved_state = footer.saved_state;
	cons->OutFormat("SAVED STATE   0x%[8H]\n", saved_state);
}

int main(int _c, char** _v)
{
	
	if (_c != 2) {
		plogerro("Bad command");
		return -1;
	}
	VirtualDisk_VHD vhd(_v[1]);
	if (!vhd) {
		plogerro("Bad file %s", _v[1]);
		return -1;
	}

	if (!print_vhd(&Console, _v[1], vhd));
	Console.OutFormat("0x%[8H]", vhd[0x1002]);

	vhd.~VirtualDisk_VHD();
	return malc_count;
}
