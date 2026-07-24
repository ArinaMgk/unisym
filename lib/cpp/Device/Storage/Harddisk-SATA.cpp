// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Stroage) Harddisk - SATA / AHCI
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../inc/c/storage/harddisk.h"

#if defined(_MCCA) && (_MCCA == 0x8632)
namespace uni {

	void AHCI_Port_Base::Bind(volatile AHCI_MEM* mmio_base, int port_no) {
		abar = mmio_base;
		port_index = port_no;
	}

	void Harddisk_SATA_AHCI::Bind(volatile AHCI_MEM* mmio_base, int port_no) {
		AHCI_Port_Base::Bind(mmio_base, port_no);
		hd_info = {};
		hd_info_valid = false;
		total_sectors = 0;
	}

	void AHCI_Port_Base::SetWorkspace(void* clb, void* fb, void* ctba) {
		cmd_list = (byte*)clb;
		rx_fis = (byte*)fb;
		cmd_table = (byte*)ctba;
	}

	bool Harddisk_SATA_AHCI::IsCandidatePort(volatile AHCI_MEM* mmio_base, uint32 port_no) {
		if (!mmio_base || port_no >= 32) return false;
		if (((mmio_base->pi >> port_no) & 1u) == 0) return false;
		const volatile AHCI_PORT& regs = mmio_base->ports[port_no];
		const uint32 det = regs.ssts & AHCI_PxSSTS_DET_MASK;
		const uint32 ipm = regs.ssts & AHCI_PxSSTS_IPM_MASK;
		if (regs.sig != AHCI_SIG_ATA) return false;
		if (det != AHCI_PxSSTS_DET_OK) return false;
		if (ipm != AHCI_PxSSTS_IPM_ACTIVE) return false;
		return true;
	}

	int Harddisk_SATA_AHCI::SelectFirstPort(volatile AHCI_MEM* mmio_base) {
		for0(port, 32) {
			if (IsCandidatePort(mmio_base, port)) {
				return port;
			}
		}
		return -1;
	}

	bool AHCI_Port_Base::StopCommandEngine() const {
		if (!abar || port_index < 0 || port_index >= 32) return false;
		auto& port = abar->ports[port_index];
		port.cmd &= ~AHCI_PxCMD_ST;
		port.cmd &= ~AHCI_PxCMD_FRE;
		for (stduint spin = 0; spin < 1000000; ++spin) {
			if ((port.cmd & (AHCI_PxCMD_CR | AHCI_PxCMD_FR)) == 0) return true;
		}
		return false;
	}

	void AHCI_Port_Base::StartCommandEngine() const {
		if (!abar || port_index < 0 || port_index >= 32) return;
		auto& port = abar->ports[port_index];
		port.cmd |= AHCI_PxCMD_FRE;
		port.cmd |= AHCI_PxCMD_ST;
	}

	bool AHCI_Port_Base::PrepareAtaCommand(byte ata_cmd, uint64 lba, byte* data_buf, stduint sector_count, bool is_write) {
		if (!abar || port_index < 0 || port_index >= 32) return false;
		if (!cmd_list || !rx_fis || !cmd_table || !data_buf || sector_count == 0) return false;
		auto& port = abar->ports[port_index];
		if (!StopCommandEngine()) return false;

		MemSet(cmd_list, 0, 1024);
		MemSet(rx_fis, 0, 256);
		MemSet(cmd_table, 0, 256);
		if (!is_write) {
			MemSet(data_buf, 0, 512 * sector_count);
		}

		port.clb = _IMM(cmd_list);
		port.clbu = 0;
		port.fb = _IMM(rx_fis);
		port.fbu = 0;
		port.is = 0xFFFFFFFFu;
		port.serr = 0xFFFFFFFFu;

		auto* cmd_header = reinterpret_cast<AHCI_CMD_HEADER*>(cmd_list);
		auto* cmd_table_ptr = reinterpret_cast<AHCI_CMD_TBL*>(cmd_table);
		auto* fis = reinterpret_cast<AHCI_FIS_REG_H2D*>(cmd_table_ptr->cfis);

		cmd_header[0].cfl = sizeof(AHCI_FIS_REG_H2D) / sizeof(uint32);
		cmd_header[0].w = is_write ? 1 : 0;
		cmd_header[0].prdtl = 1;
		cmd_header[0].prdbc = 0;
		cmd_header[0].ctba = _IMM(cmd_table);
		cmd_header[0].ctbau = 0;

		cmd_table_ptr->prdt_entry[0].dba = _IMM(data_buf);
		cmd_table_ptr->prdt_entry[0].dbau = 0;
		cmd_table_ptr->prdt_entry[0].dbc = 512 * sector_count - 1;
		cmd_table_ptr->prdt_entry[0].i = 1;

		fis->fis_type = AHCI_FIS_TYPE_REG_H2D;
		fis->flags = AHCI_FIS_H2D_C;
		fis->command = ata_cmd;
		fis->device = (ata_cmd == AHCI_ATA_CMD_IDENTIFY || ata_cmd == ATAPI_CMD_IDENTIFY) ? 0 : (1u << 6);
		fis->lba0 = byte(lba);
		fis->lba1 = byte(lba >> 8);
		fis->lba2 = byte(lba >> 16);
		fis->lba3 = byte(lba >> 24);
		fis->lba4 = byte(lba >> 32);
		fis->lba5 = byte(lba >> 40);
		fis->countl = (ata_cmd == AHCI_ATA_CMD_IDENTIFY || ata_cmd == ATAPI_CMD_IDENTIFY) ? 0 : byte(sector_count);
		fis->counth = (ata_cmd == AHCI_ATA_CMD_IDENTIFY || ata_cmd == ATAPI_CMD_IDENTIFY) ? 0 : byte(sector_count >> 8);

		StartCommandEngine();
		return true;
	}

	bool AHCI_Port_Base::PreparePacketCommand(const byte packet[12], byte* data_buf, stduint byte_count, bool is_write) {
		if (!abar || port_index < 0 || port_index >= 32) return false;
		if (!cmd_list || !rx_fis || !cmd_table || !packet || !data_buf || byte_count == 0) return false;
		auto& port = abar->ports[port_index];
		if (!StopCommandEngine()) return false;

		MemSet(cmd_list, 0, 1024);
		MemSet(rx_fis, 0, 256);
		MemSet(cmd_table, 0, 256);
		if (!is_write) {
			MemSet(data_buf, 0, byte_count);
		}

		port.clb = _IMM(cmd_list);
		port.clbu = 0;
		port.fb = _IMM(rx_fis);
		port.fbu = 0;
		port.is = 0xFFFFFFFFu;
		port.serr = 0xFFFFFFFFu;

		auto* cmd_header = reinterpret_cast<AHCI_CMD_HEADER*>(cmd_list);
		auto* cmd_table_ptr = reinterpret_cast<AHCI_CMD_TBL*>(cmd_table);
		auto* fis = reinterpret_cast<AHCI_FIS_REG_H2D*>(cmd_table_ptr->cfis);

		cmd_header[0].cfl = sizeof(AHCI_FIS_REG_H2D) / sizeof(uint32);
		cmd_header[0].a = 1;
		cmd_header[0].w = is_write ? 1 : 0;
		cmd_header[0].prdtl = 1;
		cmd_header[0].prdbc = 0;
		cmd_header[0].ctba = _IMM(cmd_table);
		cmd_header[0].ctbau = 0;

		MemCopyN(cmd_table_ptr->acmd, packet, 12);
		cmd_table_ptr->prdt_entry[0].dba = _IMM(data_buf);
		cmd_table_ptr->prdt_entry[0].dbau = 0;
		cmd_table_ptr->prdt_entry[0].dbc = byte_count - 1;
		cmd_table_ptr->prdt_entry[0].i = 1;

		fis->fis_type = AHCI_FIS_TYPE_REG_H2D;
		fis->flags = AHCI_FIS_H2D_C;
		fis->command = ATAPI_CMD_PACKET;
		fis->featurel = 1;
		fis->lba1 = byte(byte_count);
		fis->lba2 = byte(byte_count >> 8);

		StartCommandEngine();
		return true;
	}

	bool AHCI_Port_Base::PollCommandSlot(uint32 slot_mask) const {
		if (!abar || port_index < 0 || port_index >= 32) return false;
		if (fn_irq_wait) {
			if (!fn_irq_wait(const_cast<AHCI_Port_Base*>(this), slot_mask)) return false;
		}
		auto& port = abar->ports[port_index];
		for (stduint spin = 0; spin < 5000000; ++spin) {
			if ((port.ci & slot_mask) == 0) {
				if (port.is & AHCI_PxIS_TFES) return false;
				return true;
			}
			if (port.is & AHCI_PxIS_TFES) return false;
		}
		return false;
	}

	bool Harddisk_SATA_AHCI::Identify(void* identify_buf) {
		if (!identify_buf) return false;
		if (!PrepareAtaCommand(AHCI_ATA_CMD_IDENTIFY, 0, (byte*)identify_buf, 1, false)) return false;
		auto& port = abar->ports[port_index];
		while (port.tfd & (0x80u | 0x08u)) {}
		port.ci = 1u;
		if (!PollCommandSlot(1u)) return false;
		return true;
	}

	void Harddisk_SATA_AHCI::UpdateIdentity(const void* identify_buf) {
		if (!identify_buf) return;
		auto* hdinfo = reinterpret_cast<const uint16*>(identify_buf);
		uint32 sectors28 = (uint32(hdinfo[61]) << 16) | hdinfo[60];
		uint64 sectors48 =
			(uint64(hdinfo[103]) << 48) |
			(uint64(hdinfo[102]) << 32) |
			(uint64(hdinfo[101]) << 16) |
			uint64(hdinfo[100]);
		total_sectors = sectors48 ? sectors48 : sectors28;
		hd_info.whole_disk.address = 0;
		hd_info.whole_disk.length = (stduint)total_sectors;
		hd_info.whole_disk.sys_id = 0;
		hd_info.scheme_kind = PartitionSchemeKind::Unknown;
	}

	void Harddisk_SATA_AHCI::GetModel(char model[41], const void* identify_buf) const {
		if (!model) return;
		MemSet(model, 0, 41);
		if (!identify_buf) return;
		auto* hdinfo = reinterpret_cast<const uint16*>(identify_buf);
		for0(i, 20) {
			const char* src = reinterpret_cast<const char*>(&hdinfo[27 + i]);
			model[i * 2 + 0] = src[1];
			model[i * 2 + 1] = src[0];
		}
	}

	bool Harddisk_SATA_AHCI::ReadSectors(uint64 lba, void* dest, stduint sector_count) {
		if (!dest) return false;
		if (!PrepareAtaCommand(AHCI_ATA_CMD_READ_DMA_EXT, lba, (byte*)dest, sector_count, false)) return false;
		auto& port = abar->ports[port_index];
		while (port.tfd & (0x80u | 0x08u)) {}
		port.ci = 1u;
		if (!PollCommandSlot(1u)) return false;
		return true;
	}

	bool Harddisk_SATA_AHCI::WriteSectors(uint64 lba, const void* src, stduint sector_count) {
		if (!src) return false;
		if (!PrepareAtaCommand(AHCI_ATA_CMD_WRITE_DMA_EXT, lba, (byte*)src, sector_count, true)) return false;
		auto& port = abar->ports[port_index];
		while (port.tfd & (0x80u | 0x08u)) {}
		port.ci = 1u;
		if (!PollCommandSlot(1u)) return false;
		return true;
	}

	bool Harddisk_SATA_AHCI::Read(stduint BlockIden, void* Dest) {
		return ReadSectors(BlockIden, Dest, 1);
	}

	bool Harddisk_SATA_AHCI::Write(stduint BlockIden, const void* Sors) {
		return WriteSectors(BlockIden, Sors, 1);
	}

	stduint Harddisk_SATA_AHCI::getUnits() {
		return (stduint)total_sectors;
	}

	PartitionSlice Harddisk_SATA_AHCI::getSlice(stduint dev) {
		return GetPartitionSlice(hd_info, dev);
	}

	void CDROM_ATAPI_AHCI::Bind(volatile AHCI_MEM* mmio_base, int port_no) {
		AHCI_Port_Base::Bind(mmio_base, port_no);
		total_blocks = 0;
	}

	bool CDROM_ATAPI_AHCI::IsCandidatePort(volatile AHCI_MEM* mmio_base, uint32 port_no) {
		if (!mmio_base || port_no >= 32) return false;
		if (((mmio_base->pi >> port_no) & 1u) == 0) return false;
		const volatile AHCI_PORT& regs = mmio_base->ports[port_no];
		const uint32 det = regs.ssts & AHCI_PxSSTS_DET_MASK;
		const uint32 ipm = regs.ssts & AHCI_PxSSTS_IPM_MASK;
		if (regs.sig != AHCI_SIG_ATAPI) return false;
		if (det != AHCI_PxSSTS_DET_OK) return false;
		if (ipm != AHCI_PxSSTS_IPM_ACTIVE) return false;
		return true;
	}

	bool CDROM_ATAPI_AHCI::IdentifyPacket(void* identify_buf) {
		if (!identify_buf) return false;
		if (!PrepareAtaCommand(ATAPI_CMD_IDENTIFY, 0, (byte*)identify_buf, 1, false)) return false;
		auto& port = abar->ports[port_index];
		while (port.tfd & (0x80u | 0x08u)) {}
		port.ci = 1u;
		if (!PollCommandSlot(1u)) return false;
		return true;
	}

	bool CDROM_ATAPI_AHCI::ReadCapacity(void* capacity_buf) {
		if (!capacity_buf) return false;
		byte packet[12] = {};
		packet[0] = 0x25;
		if (!PreparePacketCommand(packet, (byte*)capacity_buf, 8, false)) return false;
		auto& port = abar->ports[port_index];
		while (port.tfd & (0x80u | 0x08u)) {}
		port.ci = 1u;
		if (!PollCommandSlot(1u)) return false;
		return true;
	}

	bool CDROM_ATAPI_AHCI::ReadBlocks(uint32 lba, void* dest, stduint block_count) {
		if (!dest || block_count == 0) return false;
		byte packet[12] = {};
		packet[0] = 0xA8;
		packet[2] = byte(lba >> 24);
		packet[3] = byte(lba >> 16);
		packet[4] = byte(lba >> 8);
		packet[5] = byte(lba);
		packet[6] = byte(block_count >> 24);
		packet[7] = byte(block_count >> 16);
		packet[8] = byte(block_count >> 8);
		packet[9] = byte(block_count);
		if (!PreparePacketCommand(packet, (byte*)dest, Block_Size * block_count, false)) return false;
		auto& port = abar->ports[port_index];
		while (port.tfd & (0x80u | 0x08u)) {}
		port.ci = 1u;
		if (!PollCommandSlot(1u)) return false;
		return true;
	}

	void CDROM_ATAPI_AHCI::UpdateCapacity(const void* capacity_buf) {
		if (!capacity_buf) return;
		auto* data = reinterpret_cast<const byte*>(capacity_buf);
		uint32 max_lba =
			(uint32(data[0]) << 24) |
			(uint32(data[1]) << 16) |
			(uint32(data[2]) << 8) |
			uint32(data[3]);
		total_blocks = max_lba + 1;
	}

	bool CDROM_ATAPI_AHCI::Read(stduint BlockIden, void* Dest) {
		return ReadBlocks((uint32)BlockIden, Dest, 1);
	}

	stduint CDROM_ATAPI_AHCI::getUnits() {
		return total_blocks;
	}

	PartitionSlice CDROM_ATAPI_AHCI::getSlice(stduint dev) {
		PartitionSlice slice = {};
		if (dev != 0) return slice;
		slice.address = 0;
		slice.length = total_blocks;
		slice.sys_id = 0;
		return slice;
	}
}
#endif
