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

#include "StorageTrait.hpp"

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


	enum class FilesysCmd {
		FS_CMD_GET_SIZE = 0x01,  // moreinfo points to stduint for output
		FS_CMD_GET_ISDIR = 0x02, // moreinfo points to bool for output
	};

	struct FilesysSearchArgs {
		void* handle_buffer;     // formerly moreinfo[0]
		void* dir_info;          // formerly moreinfo[1]
		stduint (*on_segment)(void* handle, const char* name, stduint is_dir, stduint size, void* user_data);
		void* user_data;
	};

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

		// return the handler of the path/file (nullptr for failure) , `args` will contain results or callbacks
		virtual void* search(rostr fullpath, FilesysSearchArgs* args) = 0;

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


	// Loopback Device
	class FileBlockBridge : public BlockTrait {
	private:
		FilesysTrait* fs;
		void* file_handle;
		stduint file_size;

	public:
		FileBlockBridge(FilesysTrait* filesystem, void* handle, stduint f_size, stduint block_size = 512) {
			this->fs = filesystem;
			this->file_handle = handle;
			this->file_size = f_size;
			this->Block_Size = block_size;
			this->readable = true;
			this->writable = true;
		}

		using BlockTrait::Read;

		virtual bool Read(stduint BlockIden, void* Dest) override {
			if (!fs || !file_handle) return false;

			stduint offset = BlockIden * Block_Size;
			if (offset >= file_size) return false;

			stduint to_read = Block_Size;
			if (offset + to_read > file_size) {
				to_read = file_size - offset;
			}

			stduint bytes_read = fs->readfl(file_handle, Slice{ offset, to_read }, (byte*)Dest);

			// Safety Erase
			if (bytes_read < Block_Size) {
				MemSet((byte*)Dest + bytes_read, 0, Block_Size - bytes_read);
			}

			return bytes_read > 0;
		}

		virtual bool Write(stduint BlockIden, const void* Sors) override {
			if (!writable || !fs || !file_handle) return false;

			stduint offset = BlockIden * Block_Size;
			if (offset >= file_size) return false;

			stduint to_write = Block_Size;
			if (offset + to_write > file_size) {
				to_write = file_size - offset;
			}

			stduint bytes_written = fs->writfl(file_handle, Slice{ offset, to_write }, (const byte*)Sors);

			return bytes_written > 0;
		}

		virtual stduint getUnits() override {
			return vaultAlign(Block_Size, file_size);
		}
	};
}
#endif
