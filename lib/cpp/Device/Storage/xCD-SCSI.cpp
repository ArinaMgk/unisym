// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Storage) CD-ROM - SCSI
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../inc/c/storage/harddisk.h"

#if defined(_MCCA) && (_MCCA == 0x8632)
namespace uni {

	enum : byte {
		SCSI_CMD_READ10 = 0x28,
		SCSI_CMD_COMPLETE_GOOD = 0x01,
		SCSI_ADAPTER_STATUS_OK = 0x00,
		SCSI_TARGET_STATUS_GOOD = 0x00,
	};

	static bool _LOCAL_ExecuteRead10(CDROM_SCSI& cdrom, uint32 lba, void* data_buf, stduint block_count) {
		if (!cdrom.fn_exec || !data_buf || !block_count || !cdrom.Block_Size) return false;
		if (lba >= cdrom.total_blocks || uint64(lba) + block_count > cdrom.total_blocks) return false;
		if (block_count > 0xFFFFu) return false;

		byte cdb[10] = {};
		cdb[0] = SCSI_CMD_READ10;
		cdb[2] = byte(lba >> 24);
		cdb[3] = byte(lba >> 16);
		cdb[4] = byte(lba >> 8);
		cdb[5] = byte(lba);
		cdb[7] = byte(block_count >> 8);
		cdb[8] = byte(block_count);

		byte completion_code = 0xFF;
		byte adapter_status = 0xFF;
		byte target_status = 0xFF;
		const stduint data_len = cdrom.Block_Size * block_count;
		if (!cdrom.fn_exec(cdrom.exec_context, cdrom.target_id, cdrom.lun,
			cdb, 10, data_buf, data_len, true,
			completion_code, adapter_status, target_status)) return false;
		return completion_code == SCSI_CMD_COMPLETE_GOOD &&
			adapter_status == SCSI_ADAPTER_STATUS_OK &&
			target_status == SCSI_TARGET_STATUS_GOOD;
	}

	void CDROM_SCSI::Bind(byte target, byte lunx, void* context, ExecCommandFn exec_fn) {
		target_id = target;
		lun = lunx;
		exec_context = context;
		fn_exec = exec_fn;
		total_blocks = 0;
		Block_Size = 2048;
	}

	void CDROM_SCSI::UpdateCapacity(uint32 last_lba, uint32 block_size) {
		total_blocks = last_lba + 1;
		Block_Size = block_size ? block_size : 2048;
	}

	bool CDROM_SCSI::Read(stduint BlockIden, void* Dest, stduint Times) {
		return _LOCAL_ExecuteRead10(*this, (uint32)BlockIden, Dest, Times);
	}

	stduint CDROM_SCSI::getUnits() {
		return total_blocks;
	}

	PartitionSlice CDROM_SCSI::getSlice(stduint dev) {
		PartitionSlice slice = {};
		slice.address = 0;
		slice.length = dev ? 0 : getUnits();
		slice.sys_id = 0;
		return slice;
	}

}
#endif
