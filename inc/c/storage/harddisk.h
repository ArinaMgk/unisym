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
#include "AHCI.h"

#define ATA_READ     0x20
#define ATA_WRITE    0x30
#define ATA_READ_DMA  0xC8
#define ATA_WRITE_DMA 0xCA
#define ATA_IDENTIFY 0xEC

#define HD_TIMEOUT 10000
#define HD_WAITFOR_TIMEOUT (HD_TIMEOUT / 1000)

#define ATAPI_CMD_PACKET 0xA0
#define ATAPI_CMD_IDENTIFY 0xA1

namespace uni {

#if defined(_MCCA) && ((_MCCA & 0xFF00) == 0x8600)

	enum {
		BMIDE_REG_CMD = 0,
		BMIDE_REG_STATUS = 2,
		BMIDE_REG_PRDT = 4,
	};

	enum {
		BMIDE_CMD_START = 0x01,
		BMIDE_CMD_READ = 0x08,
		BMIDE_STATUS_ERROR = 0x02,
		BMIDE_STATUS_IRQ = 0x04,
	};

	struct HdiskCommand {
		byte feature;
		byte count;
		byte LBA[3];// LE
		byte device;
		byte command;
		#define	MAKE_DEVICE_REG(lba_mode,drv,lba_highest) (((lba_mode) << 6) | ((drv) << 4) | ((lba_highest) & 0xF) | 0xA0)
	};

	_PACKED(struct) PataBmidePrd {
		uint32 phys_base;
		uint16 byte_count;
		uint16 flags;
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
		bool (*fn_int_wait)() = 0;// true: interrupt received; false: timeout
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
		virtual bool Read(stduint BlockIden, void* Dest) override;
		virtual bool Write(stduint BlockIden, const void* Sors) override;
		virtual stduint getUnits() override;
		// byte read
		virtual int operator[](uint64 bytid) override { return _TODO 0; }
		//
		virtual void setInterrupt(Handler_t _func) const override;
		virtual void setInterruptPriority(byte preempt, byte sub_priority) const override;
		virtual void enInterrupt(bool enable = true) const override;

		virtual PartitionSlice getSlice(stduint dev) override;

		byte getID() const { return id; }
		byte getHigID() const { return id / 2; } // 0 = Primary, 1 = Secondary
		byte getLowID() const { return id % 2; } // 0 = Master, 1 = Slave
		
		byte ProbeDevice(); // Returns: 0 = None, 1 = HDD, 2 = CD

		// Initialize()
		void Reset();

		#if defined(_DEV_GCC) && defined(_MCCA) && ((_MCCA & 0xFF00) == 0x8600)
		bool Hdisk_OUT(HdiskCommand* hd_cmd);
		byte getStatus();
		#endif

	};

	#if defined(_DEV_GCC) && defined(_MCCA) && ((_MCCA & 0xFF00) == 0x8600)
	bool PataWaitRetry(Harddisk_PATA* hdd,
		bool (*fn_lup_wait)(Harddisk_PATA*, stduint mask, stduint val, stduint timeout_second),
		stduint mask, stduint val);
	#endif


	class xCD_ATAPI : public Harddisk_PATA {
	public:
		xCD_ATAPI(byte id = 0) : Harddisk_PATA(id) {
			Block_Size = 2048; // CD-ROM sector size is 2048 bytes
		}
		virtual bool Read(stduint BlockIden, void* Dest) override;
		virtual bool Write(stduint BlockIden, const void* Sors) override { return false; } // CD-ROM is read-only
		virtual stduint getUnits() override;
		virtual PartitionSlice getSlice(stduint dev) override;
	};

	class AHCI_Port_Base {
	public:
		volatile AHCI_MEM* abar = nullptr;
		int port_index = -1;
		byte* cmd_list = nullptr;
		byte* rx_fis = nullptr;
		byte* cmd_table = nullptr;
		bool (*fn_irq_wait)(AHCI_Port_Base* port, uint32 slot_mask) = nullptr;

	public:
		void Bind(volatile AHCI_MEM* mmio_base, int port_no);
		void SetWorkspace(void* clb, void* fb, void* ctba);

	protected:
		bool StopCommandEngine() const;
		void StartCommandEngine() const;
		bool PollCommandSlot(uint32 slot_mask) const;
		bool PrepareAtaCommand(byte ata_cmd, uint64 lba, byte* data_buf, stduint sector_count, bool is_write);
		bool PreparePacketCommand(const byte packet[12], byte* data_buf, stduint byte_count, bool is_write);
	};

	class Harddisk_SATA_AHCI : public StorageTrait, public AHCI_Port_Base {
	public:
		uint64 total_sectors = 0;
		HD_Info hd_info = {};
		bool hd_info_valid = false;

	public:
		Harddisk_SATA_AHCI() {
			Block_buffer = nullptr;
			Block_Size = 512;
		}

		void Bind(volatile AHCI_MEM* mmio_base, int port_no);

		static bool IsCandidatePort(volatile AHCI_MEM* mmio_base, uint32 port_no);
		static int SelectFirstPort(volatile AHCI_MEM* mmio_base);

		bool Identify(void* identify_buf);
		void UpdateIdentity(const void* identify_buf);
		void GetModel(char model[41], const void* identify_buf) const;
		bool ReadSectors(uint64 lba, void* dest, stduint sector_count);
		bool WriteSectors(uint64 lba, const void* src, stduint sector_count);

		virtual bool Read(stduint BlockIden, void* Dest) override;
		virtual bool Write(stduint BlockIden, const void* Sors) override;
		virtual stduint getUnits() override;
		virtual int operator[](uint64 bytid) override { return _TODO 0; }
		virtual PartitionSlice getSlice(stduint dev) override;
	};

	class CDROM_ATAPI_AHCI : public StorageTrait, public AHCI_Port_Base {
	public:
		uint32 total_blocks = 0;

	public:
		CDROM_ATAPI_AHCI() {
			Block_buffer = nullptr;
			Block_Size = 2048;
		}

		void Bind(volatile AHCI_MEM* mmio_base, int port_no);

		static bool IsCandidatePort(volatile AHCI_MEM* mmio_base, uint32 port_no);

		bool IdentifyPacket(void* identify_buf);
		bool ReadCapacity(void* capacity_buf);
		bool ReadBlocks(uint32 lba, void* dest, stduint block_count);
		void UpdateCapacity(const void* capacity_buf);

		virtual bool Read(stduint BlockIden, void* Dest) override;
		virtual bool Write(stduint BlockIden, const void* Sors) override { return false; }
		virtual stduint getUnits() override;
		virtual int operator[](uint64 bytid) override { return _TODO 0; }
		virtual PartitionSlice getSlice(stduint dev) override;
	};

}
#endif

#endif
