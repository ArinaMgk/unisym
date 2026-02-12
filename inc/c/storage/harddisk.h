// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Stroage) Harddisk
// Codifiers: @dosconio: 20250107
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

#ifndef _INCPP_Storage_Harddisk
#define _INCPP_Storage_Harddisk

#include "../stdinc.h"

#ifdef _INC_CPP

#include "../../cpp/trait/StorageTrait.hpp"
#include "../../cpp/interrupt"

#define ATA_READ     0x20
#define ATA_WRITE    0x30
#define ATA_IDENTIFY 0xEC

namespace uni {

#if defined(_MCCA) && ((_MCCA & 0xFF00) == 0x8600)

	struct HdiskCommand {
		byte feature;
		byte count;
		byte LBA[3];// LE
		byte device;
		byte command;
		#define	MAKE_DEVICE_REG(lba_mode,drv,lba_highest) (((lba_mode) << 6) | ((drv) << 4) | ((lba_highest) & 0xF) | 0xA0)
	};


#endif


	class Harddisk_PATA : public StorageTrait, public RuptTrait
	{
	public:
		enum class HarddiskType {
			ATA// LBA28
		};//{} ControllerType. Move out and into class `Harddisk`
		enum class ReactType {
			None,// invalid mode
			Loop,// Polling
			Rupt,// ! need bind 3 functions
		};
		byte id;// 0x00=0:0(primary master), 0x11=1:1(secondary slave) ...
	protected:
		word io_base;   // e.g., 0x1F0 or 0x170
		word ctrl_base; // e.g., 0x3F6 or 0x376
	public:
		// heritance
		// - stduint Block_Size;
		// - void* Block_buffer;
		HarddiskType type;
		ReactType react_type = ReactType::Loop;
		bool (*fn_cmd_wait)(Harddisk_PATA* hdd) = 0;
		void (*fn_int_wait)() = 0;
		bool (*fn_lup_wait)(Harddisk_PATA*, stduint mask, stduint val, stduint timeout_second) = 0;
		void (*fn_feedback)();// optional RW precalling
		stdsint units = -1;// -1 for uninit, 0 for inf
	public:
		
		Harddisk_PATA(byte id = 0, HarddiskType type = HarddiskType::ATA) : id(id), type(type) {
			Block_buffer = nullptr;
			Block_Size = 512;
			#if defined(_MCCA) && ((_MCCA & 0xFF00) == 0x8600)
			io_base = id < 2 ? PORT_IDE_CommandBlock_0 : PORT_IDE_CommandBlock_1;
			ctrl_base = id < 2 ? PORT_IDE_ControlBlock_0 : PORT_IDE_ControlBlock_1;
			#endif
		}
		virtual bool Read(stduint BlockIden, void* Dest);
		virtual bool Write(stduint BlockIden, const void* Sors);
		virtual stduint getUnits();
		// byte read
		virtual int operator[](uint64 bytid) { return _TODO 0; }
		//
		virtual void setInterrupt(Handler_t _func) const;
		virtual void setInterruptPriority(byte preempt, byte sub_priority) const;
		virtual void enInterrupt(bool enable = true) const;

		byte getID() const { return id; }
		byte getHigID() const { return id >> 4; }
		byte getLowID() const { return id & 0x0F; }

		// Initialize()
		void Reset();

		#if defined(_DEV_GCC) && defined(_MCCA) && ((_MCCA & 0xFF00) == 0x8600)
		bool Hdisk_OUT(HdiskCommand* hd_cmd);
		byte getStatus();
		#endif

	};




}
#endif

#endif
