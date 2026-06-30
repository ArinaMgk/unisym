// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Storage) xCD_ATAPI CD-ROM Driver
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

#include "../../../../inc/c/storage/harddisk.h"
#include "../../../../inc/c/driver/i8259A.h"

#if defined(_MCCA) && ((_MCCA & 0xFF00) == 0x8600)
#include "../../../../inc/cpp/Device/Storage/HD-DEPEND.h"
#include "../../../../inc/c/board/IBM.h"
#pragma GCC optimize("O0")

#define ATA_CTRL_nIEN (1 << 1)

namespace uni {

	bool xCD_ATAPI::Read(stduint BlockIden, void* Dest) {
		outpb(ctrl_base, ATA_CTRL_nIEN);
		
		// Wait for the controller BSY to clear
		stduint timeout = 1000000;
		while ((innpb(io_base + REG_STATUS) & STATUS_BSY) && --timeout);
		if (!timeout) { outpb(ctrl_base, 0); return false; }
		
		// Select the drive (Master or Slave)
		outpb(io_base + REG_DEVICE, MAKE_DEVICE_REG(0, getLowID(), 0));
		
		// Wait for both BSY and DRQ to be cleared
		timeout = 1000000;
		while ((innpb(io_base + REG_STATUS) & (STATUS_BSY | STATUS_DRQ)) && --timeout);
		if (!timeout) { outpb(ctrl_base, 0); return false; }
		
		// Set feature register to 0 (PIO mode)
		outpb(io_base + REG_FEATURES, 0);
		
		// Write the expected transfer byte count (2048 bytes) to LBA Mid and LBA High
		outpb(io_base + REG_LBA_MID, 2048 & 0xFF);
		outpb(io_base + REG_LBA_HIGH, 2048 >> 8);
		
		// Send ATAPI Packet Command (0xA0)
		outpb(io_base + REG_CMD, ATAPI_CMD_PACKET);
		
		// Wait for BSY to clear and DRQ to set (device ready for packet transfer)
		timeout = 1000000;
		while (--timeout) {
			byte status = innpb(io_base + REG_STATUS);
			if (!(status & STATUS_BSY) && (status & STATUS_DRQ)) {
				break;
			}
		}
		if (!timeout) { outpb(ctrl_base, 0); return false; }
		
		// Construct the 12-byte SCSI READ (12) command packet
		byte packet[12] = {0};
		packet[0] = 0xA8; // SCSI READ (12) opcode
		packet[1] = 0;
		packet[2] = (BlockIden >> 24) & 0xFF; // LBA in Big-Endian format
		packet[3] = (BlockIden >> 16) & 0xFF;
		packet[4] = (BlockIden >> 8) & 0xFF;
		packet[5] = BlockIden & 0xFF;
		packet[6] = 0; // Sector count limit (transfer length) in Big-Endian
		packet[7] = 0;
		packet[8] = 0;
		packet[9] = 1; // Read 1 sector
		packet[10] = 0;
		packet[11] = 0;
		
		// Write the 12-byte command packet to the IDE data register (6 words)
		OUT_wn(io_base + REG_DATA, (word*)packet, 12);
		
		// Wait for the data transfer readiness (BSY clears, DRQ sets)
		timeout = 1000000;
		bool ok = false;
		while (--timeout) {
			byte status = innpb(io_base + REG_STATUS);
			if (!(status & STATUS_BSY)) {
				if (status & STATUS_DRQ) {
					ok = true;
					break; // Data is ready
				}
				if (status & STATUS_ERR) {
					break; // I/O Error occurred
				}
			}
		}
		if (!ok) { outpb(ctrl_base, 0); return false; }
		
		// Read 2048 bytes of sector data from the data register
		IN_wn(io_base + REG_DATA, (word*)Dest, 2048);
		
		outpb(ctrl_base, 0);
		return true;
	}

	stduint xCD_ATAPI::getUnits() {
		outpb(ctrl_base, ATA_CTRL_nIEN);
		
		// Wait for the controller BSY to clear
		stduint timeout = 1000000;
		while ((innpb(io_base + REG_STATUS) & STATUS_BSY) && --timeout);
		if (!timeout) { outpb(ctrl_base, 0); return 0; }
		
		// Select the drive (Master or Slave)
		outpb(io_base + REG_DEVICE, MAKE_DEVICE_REG(0, getLowID(), 0));
		
		// Wait a bit
		for (volatile int i = 0; i < 1000; i++);
		
		// Write expected return length (8 bytes) to Cylinder Low/High
		outpb(io_base + REG_LBA_MID, 8);
		outpb(io_base + REG_LBA_HIGH, 0);
		
		// Send ATAPI PACKET command
		outpb(io_base + REG_CMD, ATAPI_CMD_PACKET);
		
		// Wait for DRQ to be set, indicating ready to receive packet
		timeout = 1000000;
		while (!(innpb(io_base + REG_STATUS) & STATUS_DRQ) && --timeout);
		if (!timeout) { outpb(ctrl_base, 0); return 0; }
		
		// 12-byte SCSI command package (READ CAPACITY 10)
		byte packet[12] = {0};
		packet[0] = 0x25; // SCSI READ CAPACITY (10) command
		
		// Send the packet via Data register
		OUT_wn(io_base + REG_DATA, (word*)packet, 12);
		
		// Wait for completion (BSY clears, DRQ sets)
		timeout = 1000000;
		bool ok = false;
		while (--timeout) {
			byte status = innpb(io_base + REG_STATUS);
			if (!(status & STATUS_BSY)) {
				if (status & STATUS_DRQ) {
					ok = true;
					break; // Data is ready
				}
				if (status & STATUS_ERR) {
					break; // Capacity query failed (e.g. no disc)
				}
			}
		}
		if (!ok) { outpb(ctrl_base, 0); return 0; }
		
		// Read 8 bytes of capacity data
		uint32 buffer[2] = {0};
		IN_wn(io_base + REG_DATA, (word*)buffer, 8);
		
		// Parse results (Big Endian!)
		byte* p_lba = (byte*)&buffer[0];
		uint32 max_lba = ((uint32)p_lba[0] << 24) | ((uint32)p_lba[1] << 16) | ((uint32)p_lba[2] << 8) | p_lba[3];
		
		outpb(ctrl_base, 0);
		return max_lba + 1;
	}

	PartitionSlice xCD_ATAPI::getSlice(stduint dev) {
		outpb(ctrl_base, ATA_CTRL_nIEN);
		
		PartitionSlice slice;
		slice.address = 0;
		slice.length = getUnits();
		// ATAPI optical media are probed as whole-disk filesystems rather than
		// partition-typed block devices. Leave sys_id neutral here and let the
		// filesystem probe chain decide whether the medium is ISO9660/UDF/etc.
		slice.sys_id = 0;
		
		outpb(ctrl_base, 0);
		return slice;
	}

}
#endif
