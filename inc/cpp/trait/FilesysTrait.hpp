// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Trait) File System
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
#include "../unisym"
#include "../string"
//{TODO} ! Below are included by HEREPIC ! //

#include "../trait/BlockTrait.hpp"

#ifndef _INCPP_TRAIT_FILESYSTEM
#define _INCPP_TRAIT_FILESYSTEM

// 20251121 Phina: /http -> /lnk/http, use /http/phina.net for http(s)://phina.net


namespace uni {

/*
20251121 PhinaThink:
fileman --- FilesysT(in: StorageT<Partition>)
        --- StorageT<Device>
(v)disk[BUSLINES/HOSTFILE]   drive   {
	partitions[] each device {
		interf fs {
			makefs();
			create();
			delete();
			search();// get proper
			proper();// set proper
			enumer();
			readfl();
			writfl();
		}
	}
	prim[4]// h1~h4 h6~h9 ...
	subp[16 * 4]// h16...
}
Harddisk_PATA IDE0_0(0x00),// h0
Harddisk_PATA IDE0_1(0x01),// h5
Harddisk_PATA IDE1_0(0x10),// ... CD-ROM
Harddisk_PATA IDE1_1(0x11);// ... XXX
* MBR */


	class FilesysTrait
	{
	public:
		StorageTrait* storage;// set a partition(device)
		stduint error_number = 0;

		virtual bool makefs(rostr vol_label, void* moreinfo = 0) = 0;

		virtual bool loadfs(void* moreinfo = 0) { return false; }

		// create file/folder. usize{isfolder}
		virtual bool create(rostr fullpath, stduint flags, void* exinfo, rostr linkdest = 0) = 0;

		// remove file/folder
		virtual bool remove(rostr pathname) = 0;

		// return the handler of the path/file (nullptr for failure) , `moreinfo` will get the proper
		virtual void* search(rostr fullpath, void* moreinfo) = 0;

		virtual bool proper(void* handler, stduint cmd, const void* moreinfo = 0) = 0;// set proper

		// _fn(is_folder, flinfo)
		virtual bool enumer(void* dir_handler, _tocall_ft _fn) = 0;

		// return the bytes read
		// limit(<= ~0, <= ~0)
		virtual stduint readfl(void* fil_handler, Slice file_slice, byte*) = 0;// read file

		// return the bytes written
		virtual stduint writfl(void* fil_handler, Slice file_slice, const byte*) = 0;// write file
		//
		virtual rostr nextlink(rostr fullpath) { return NULL; }// if a hard/soft link
	};


}
#endif
