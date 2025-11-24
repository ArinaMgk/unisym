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
//{TODO} ! Below are included by HEREPIC ! //

#include "../trait/BlockTrait.hpp"

#ifndef _INCPP_TRAIT_FILESYSTEM
#define _INCPP_TRAIT_FILESYSTEM
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
		virtual bool makefs() = 0;
		virtual bool loadfs() { return false; }
		virtual bool create(rostr fullpath, stduint flags, stduint* exinfo, rostr linkdest = 0) = 0;// create file/folder. usize{isfolder}
		virtual bool remove() = 0;// remove file/folder
		virtual bool search(rostr fullpath, stduint* retback) = 0;// get proper
		virtual bool proper() = 0;// set proper
		virtual bool enumer() = 0;
		virtual bool readfl() = 0;// read file
		virtual bool writfl() = 0;// write file
		//
		virtual rostr nextlink() { return NULL; }
	};


}
#endif
