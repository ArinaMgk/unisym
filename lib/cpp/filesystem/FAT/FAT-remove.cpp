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

namespace uni {

	static uint32_t cluster_to_sector(FilesysFAT& fs, uint32_t cluster);

	bool FilesysFAT::remove(rostr pathname) {
		// seek the entry and mark as 'removed'
		uint32_t parent_cluster = root_cluster ? root_cluster : 2;
		uint32_t sector = cluster_to_sector(self, parent_cluster);
		if (!storage->Read(sector, buffer_sector)) {
			return false;
		}
		const char* filename = StrIndexCharRight(pathname, '/');
		if (!filename) filename = pathname;
		for0 (i, 16) {
			FAT_DirEntry* entry = (FAT_DirEntry*)&buffer_sector[i * 32];
			char short_name[13]; MemSet(short_name, ' ', 12);
			int name_len = 0;
			while (name_len < 8 && entry->name[name_len] != ' ') {
				short_name[name_len] = entry->name[name_len];
				name_len++;
			}
			if (entry->ext[0] != ' ') {
				short_name[8] = '.';
				for (int j = 0; j < 3; j++) {
					short_name[9 + j] = entry->ext[j];
				}
			}
			short_name[12] = 0;
			if (!StrCompare(short_name, filename)) {
				entry->name[0] = 0xE5;
				return storage->Write(sector, buffer_sector);
			}
		}
		return false;
	}
	// ----

	static uint32_t cluster_to_sector(FilesysFAT& fs, uint32_t cluster)
	{
		if (cluster < 2) return 0;
		return ((cluster - 2) * fs.sectors_per_cluster) + fs.first_data_sector;
	}

}
