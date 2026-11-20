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

#include "../../../../inc/c/storage/harddisk.h"
#include "../../../../inc/c/driver/i8259A.h"

#if defined(_MCCA) && ((_MCCA & 0xFF00) == 0x8600)
#include "../../../../inc/cpp/Device/Storage/HD-DEPEND.h"
#include "../../../../inc/c/board/IBM.h"
#pragma GCC optimize("O0")
#define ATA_CTRL_nIEN (1 << 1)
namespace uni {

	bool Harddisk_PATA::Hdisk_OUT(HdiskCommand* hd_cmd) {
		if (fn_cmd_wait && !fn_cmd_wait(this)) return false;
		// Interrupt Enable (nIEN)
		outpb(ctrl_base, react_type == ReactType::Loop ? ATA_CTRL_nIEN : 0);
		// Load required parameters in the Command Block Registers
		outpb(io_base + REG_FEATURES, hd_cmd->feature);
		outpb(io_base + REG_NSECTOR, hd_cmd->count);
		for0(i, 3) outpb(io_base + REG_LBA_LOW + i, hd_cmd->LBA[i]);
		outpb(io_base + REG_DEVICE, hd_cmd->device);
		// Write the command code to the Command Register
		outpb(io_base + REG_CMD, hd_cmd->command);
		return true;
	}
	byte Harddisk_PATA::getStatus() {
		return innpb(io_base + REG_STATUS);
	}

	void Harddisk_PATA::setInterruptPriority(byte preempt, byte sub_priority) const {
		// EMPTY
	}
	void Harddisk_PATA::enInterrupt(bool enable) const {
		// EMPTY
	}
	void Harddisk_PATA::setInterrupt(Handler_t unused_func) const {
		#if _MCCA == 0x8632
		if (id >= 4) return;// ide00 ide01 ide10 ide11
		// ploginfo("Harddisk_PATA::setInterrupt(%d)", id);
		i8259Slaver_Enable(_IMM(IRQ_ATA_DISK0) + (id >> 1) - PIC_SLV_IDSTART);
		i8259Master_Enable(2);
		#endif
	}

	// __attribute__((optimize("O0")))// NO OPTIMIZE
	_WEAK bool Harddisk_PATA::Read(stduint BlockIden, void* Dest, stduint Times) {
		// ploginfo("Harddisk_PATA::Read %d(%d, %[32H])", _IMM(react_type), BlockIden, Dest);
		if (BlockIden + Times > getUnits()) return false;
		for0(t, Times) {
			stduint blk = BlockIden + t;
			byte* dst = (byte*)Dest + t * Block_Size;
			switch (react_type) {
			case ReactType::Loop:
			{
				outpb(io_base + REG_NSECTOR, 1);// 1 sector
				for0(i, 3) outpb(io_base + REG_LBA_LOW + i, (blk >> (i * 8)));// [LiE]
				outpb(io_base + REG_DEVICE, MAKE_DEVICE_REG(1, getLowID(), (blk >> 24) & 0xF));
				outpb(io_base + REG_CMD, 0x20);// read
				while ((innpb(io_base + REG_STATUS) & 0x88) != 0x08);
				IN_wn(io_base + REG_DATA, (word*)dst, Block_Size);
				break;
			}
			case ReactType::Rupt:
			{
				HdiskCommand cmd;
				cmd.feature = 0;
				cmd.count = 1;// number of sectors: slice.length
				for0(i, 3) cmd.LBA[i] = (blk >> (i * 8));
				cmd.device = MAKE_DEVICE_REG(1, getLowID(), (blk >> 24) & 0xF);
				cmd.command = ATA_READ;
				asserv(fn_feedback)();// foreback
				Harddisk_PATA::Hdisk_OUT(&cmd);
				if (fn_int_wait && fn_lup_wait) {
					const bool use_loop_fallback = !fn_int_wait();
					if (use_loop_fallback) {
						if (!PataWaitRetry(this, fn_lup_wait, STATUS_DRQ, STATUS_DRQ)) return false;
					}
					else if (!fn_lup_wait(this, STATUS_DRQ, STATUS_DRQ, HD_TIMEOUT / 1000)) {
						return false;
					}
					IN_wn(io_base + REG_DATA, (word*)dst, Block_Size);
					// repeat the block to RW multi-sectors
				}
				else return false;
				break;
			}
			default:
				return false;
			}
		}
		return true;
	}
	// __attribute__((optimize("O0")))// NO OPTIMIZE
	_WEAK bool Harddisk_PATA::Write(stduint BlockIden, const void* Sors, stduint Times) {
		// ploginfo("Harddisk_PATA::Write(%d, %[32H])", BlockIden, Sors);
		if (BlockIden + Times > getUnits()) return false;
		for0(t, Times) {
			stduint blk = BlockIden + t;
			const byte* src = (const byte*)Sors + t * Block_Size;
			switch (react_type) {
			case ReactType::Loop:
			{
				return _TODO false;
			}
			case ReactType::Rupt:
			{
				HdiskCommand cmd;
				cmd.feature = 0;
				cmd.count = 1;// number of sectors: slice.length
				for0(i, 3) cmd.LBA[i] = (blk >> (i * 8));
				cmd.device = MAKE_DEVICE_REG(1, getLowID(), (blk >> 24) & 0xF);
				cmd.command = ATA_WRITE;
				asserv(fn_feedback)();// foreback
				Harddisk_PATA::Hdisk_OUT(&cmd);
				if (fn_int_wait && fn_lup_wait) {
					if (!fn_lup_wait(this, STATUS_DRQ, STATUS_DRQ, HD_TIMEOUT / 1000)) return false;
					OUT_wn(io_base + REG_DATA, (word*)src, Block_Size);
					const bool use_loop_fallback = !fn_int_wait();
					if (use_loop_fallback) {
						if (!PataWaitRetry(this, fn_lup_wait, STATUS_BSY, 0)) return false;
					}
					// repeat the block to RW multi-sectors
				}
				else return false;
				break;
			}
			default:
				return false;
			}
		}
		return true;
	}

	_WEAK stduint Harddisk_PATA::getUnits() {
		return _TODO 0;
	}

	#define ATA_CTRL_SRST  (1 << 2)
	// Alternate status avoids acknowledging an interrupt during reset and device selection.
	static bool WaitPataResetReady(word control, byte& status) {
		for (stduint remaining = 1000000; remaining; --remaining) {
			status = innpb(control);
			if (status == 0xFF || !(status & STATUS_BSY)) return true;
		}
		return false;
	}

	static bool ResetPataDevice(word io, word control, byte device, byte& status, bool& selected) {
		selected = false;
		outpb(control, ATA_CTRL_SRST | ATA_CTRL_nIEN);
		for (volatile int i = 0; i < 1000; i = i + 1) _TEMP;//{} delay010us();
		outpb(control, ATA_CTRL_nIEN);
		for (volatile int i = 0; i < 100000; i = i + 1) _TEMP;//{}

		// Wait for BSY to clear on default device before writing to REG_DEVICE.
		if (!WaitPataResetReady(control, status)) return false;
		outpb(io + REG_DEVICE, 0xE0 | (device << 4));
		selected = true;

		// Mandatory 400ns delay after selecting drive.
		innpb(control);
		innpb(control);
		innpb(control);
		innpb(control);
		return WaitPataResetReady(control, status);
	}

	void Harddisk_PATA::Reset() {
		byte status = 0;
		bool selected = false;
		(void)ResetPataDevice(io_base, ctrl_base, getLowID(), status, selected);
	}

	_WEAK PartitionSlice Harddisk_PATA::getSlice(stduint dev) {
		plogerro("using empty Harddisk_PATA::getSlice(%d)", dev);
		PartitionSlice slice;
		return slice;
	}

	byte Harddisk_PATA::ProbeDevice() {
		byte status = 0;
		bool selected = false;
		if (!ResetPataDevice(io_base, ctrl_base, getLowID(), status, selected)) {
			plogwarn("[PATA] ide%u:%u %s busy timeout status=%x", stduint(getHigID()), stduint(getLowID()),
				selected ? "device select" : "channel reset", stduint(status));
			return 0;
		}

		// ATAPI can report zero status with a valid signature, including on an empty drive.
		byte mid = 0, high = 0;
		for (stduint remaining = 1024; remaining; --remaining) {
			status = innpb(ctrl_base);
			if (status == 0xFF || (status & STATUS_BSY)) continue;
			mid = innpb(io_base + REG_LBA_MID);
			high = innpb(io_base + REG_LBA_HIGH);
			if (mid == 0x14 && high == 0xEB) return 2;
			if (status) break;
		}
		if (status == 0 || (status & STATUS_BSY)) {
			if (!getLowID()) plogwarn("[PATA] ide%u:%u no probe response status=%x signature=%x:%x",
				stduint(getHigID()), stduint(getLowID()), stduint(status), stduint(mid), stduint(high));
			return 0;
		}

		// Write pattern to Sector Count and LBA Low registers (for ATA Hard Disk validation).
		outpb(io_base + REG_NSECTOR, 0x55);
		outpb(io_base + REG_LBA_LOW, 0xAA);

		// Read back the pattern to verify device presence.
		const byte nsector = innpb(io_base + REG_NSECTOR);
		const byte lba_low = innpb(io_base + REG_LBA_LOW);
		if (nsector == 0x55 && lba_low == 0xAA) return 1;
		if (!getLowID()) plogwarn("[PATA] ide%u:%u probe pattern mismatch status=%x count=%x lba-low=%x",
			stduint(getHigID()), stduint(getLowID()), stduint(status), stduint(nsector), stduint(lba_low));
		return 0;
	}

}
#endif

