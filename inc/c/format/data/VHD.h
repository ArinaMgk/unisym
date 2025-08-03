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

#ifndef _INC_Format_Data_VHD
#define _INC_Format_Data_VHD

#include "../../stdinc.h"

typedef struct {
	char cookie[8];
	uint32 feature;
	uint32 filefmt_ver;
	uint64 data_offset;
	uint32 time_stamp;
	char creator_app[4];
	uint32 creator_ver;
	char creator_host[4];
	uint64 orig_size;
	uint64 curr_size;
	uint32 disk_geometry;
	uint32 disk_type;
	uint32 checksum;
	char unique_id[16];
	uint8 saved_state;
	uint8 RESERVED[427];
} VHD_Footer;

#ifdef _INC_CPP
#include "../../../cpp/trait/StorageTrait.hpp"

namespace uni {

	class VirtualDisk_VHD : public StorageTrait
	{
	public:
		virtual bool Read(stduint BlockIden, void* Dest);
		virtual bool Write(stduint BlockIden, const void* Sors) { return _TODO false; }
		virtual stduint getUnits() { return _TODO 0; }
		// byte read
		virtual byte operator[](uint64 bytid) { return _TODO 0; }
	public:


	};


}

#endif
#endif
