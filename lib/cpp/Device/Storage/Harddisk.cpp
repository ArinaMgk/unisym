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

namespace uni {

	bool Harddisk_PATA::Hdisk_OUT(HdiskCommand* hd_cmd) {
		if (fn_cmd_wait && !fn_cmd_wait(this)) return false;
		// Interrupt Enable (nIEN)
		outpb(ctrl_base, 0);
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
		if (id >= 4) return;// ide00 ide01 ide10 ide11
		// ploginfo("Harddisk_PATA::setInterrupt(%d)", id);
		i8259Slaver_Enable(_IMM(IRQ_ATA_DISK0) + (id >> 1) - PIC_SLV_IDSTART);
		i8259Master_Enable(2);
	}

	// __attribute__((optimize("O0")))// NO OPTIMIZE
	_WEAK bool Harddisk_PATA::Read(stduint BlockIden, void* Dest) {
		// ploginfo("Harddisk_PATA::Read %d(%d, %[32H])", _IMM(react_type), BlockIden, Dest);
		switch (react_type) {
		case ReactType::Loop:
		{
			outpb(io_base + REG_NSECTOR, 1);// 1 sector
			for0(i, 3) outpb(io_base + REG_LBA_LOW + i, (BlockIden >> (i * 8)));// [LiE]
			outpb(io_base + REG_DEVICE, MAKE_DEVICE_REG(1, getLowID(), (BlockIden >> 24) & 0xF));
			outpb(io_base + REG_CMD, 0x20);// read
			while ((innpb(io_base + REG_STATUS) & 0x88) != 0x08);
			IN_wn(io_base + REG_DATA, (word*)Dest, Block_Size);
			return true;
		}
		case ReactType::Rupt:
		{
			HdiskCommand cmd;
			cmd.feature = 0;
			cmd.count = 1;// number of sectors: slice.length
			for0(i, 3) cmd.LBA[i] = (BlockIden >> (i * 8));
			cmd.device = MAKE_DEVICE_REG(1, getLowID(), (BlockIden >> 24) & 0xF);
			cmd.command = ATA_READ;
			asserv(fn_feedback)();// foreback
			Harddisk_PATA::Hdisk_OUT(&cmd);
			if (fn_int_wait && fn_lup_wait) {
				fn_int_wait();
				if (!fn_lup_wait(this, STATUS_DRQ, STATUS_DRQ, HD_TIMEOUT)) {
					return false;
				}
				IN_wn(io_base + REG_DATA, (word*)Dest, Block_Size);
				return true;
				// repeat the block to RW multi-sectors
			}
			else return false;
		}
		default:
			return false;
		}
		return true;
	}
	// __attribute__((optimize("O0")))// NO OPTIMIZE
	_WEAK bool Harddisk_PATA::Write(stduint BlockIden, const void* Sors) {
		// ploginfo("Harddisk_PATA::Write(%d, %[32H])", BlockIden, Sors);
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
			for0(i, 3) cmd.LBA[i] = (BlockIden >> (i * 8));
			cmd.device = MAKE_DEVICE_REG(1, getLowID(), (BlockIden >> 24) & 0xF);
			cmd.command = ATA_WRITE;
			asserv(fn_feedback)();// foreback
			Harddisk_PATA::Hdisk_OUT(&cmd);
			if (fn_int_wait && fn_lup_wait) {
				if (!fn_lup_wait(this, STATUS_DRQ, STATUS_DRQ, HD_TIMEOUT)) {
					return false;
				}
				OUT_wn(io_base + REG_DATA, (word*)Sors, Block_Size);
				fn_int_wait();
				return true;
				// repeat the block to RW multi-sectors
			}
			else return false;
		}
		default:
			return false;
		}
		return true;
	}

	_WEAK stduint Harddisk_PATA::getUnits() {
		return _TODO 0;
	}

	#define ATA_CTRL_SRST  (1 << 2)
	#define ATA_CTRL_nIEN (1 << 1)
	void Harddisk_PATA::Reset() {
		outpb(ctrl_base, ATA_CTRL_SRST | ATA_CTRL_nIEN);
		for (volatile int i = 0; i < 1000; i = i + 1) _TEMP;//{} delay010us();
		outpb(ctrl_base, ATA_CTRL_nIEN);
		for (volatile int i = 0; i < 100000; i = i + 1) _TEMP;//{}
		//
		stduint timeout = 1000000;
		bool error = false;
		while (true) {
			byte status = innpb(io_base + REG_STATUS);
			if (!(status & STATUS_BSY)) {
				break;
			}
			timeout = timeout - 1;
			if (!timeout) {
				error = true;
				break;
			}
		}
		outpb(io_base + REG_DEVICE, 0xE0 | (getLowID() << 4));
	}

	

}
#endif
