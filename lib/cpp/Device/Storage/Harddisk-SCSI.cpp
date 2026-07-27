// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Storage) Harddisk - SCSI
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../inc/c/storage/harddisk.h"

#if defined(_MCCA) && (_MCCA == 0x8632)
namespace uni {

	enum : byte {
		SCSI_CMD_REQUEST_SENSE = 0x03,
		SCSI_CMD_READ10 = 0x28,
		SCSI_CMD_WRITE10 = 0x2A,
		SCSI_CMD_COMPLETE_GOOD = 0x01,
		SCSI_ADAPTER_STATUS_OK = 0x00,
		SCSI_TARGET_STATUS_GOOD = 0x00,
		SCSI_TARGET_STATUS_CHECK_CONDITION = 0x02,
		SCSI_SENSE_NOT_READY = 0x02,
		SCSI_SENSE_UNIT_ATTENTION = 0x06,
	};

	_PACKED(struct) ScsiFixedSenseData {
		byte response_code;
		byte obsolete;
		byte sense_key;
		byte information[4];
		byte additional_length;
		byte command_specific[4];
		byte asc;
		byte ascq;
		byte fru;
		byte sense_key_specific[3];
	};

	static bool _LOCAL_RequestSense(Harddisk_SCSI& disk, byte sense_buf[18]) {
		if (!disk.fn_exec) return false;
		byte cdb[6] = {};
		cdb[0] = SCSI_CMD_REQUEST_SENSE;
		cdb[4] = 18;
		byte completion_code = 0xFF;
		byte adapter_status = 0xFF;
		byte target_status = 0xFF;
		MemSet(sense_buf, 0, 18);
		if (!disk.fn_exec(disk.exec_context, disk.target_id, disk.lun,
			cdb, 6, sense_buf, 18, true,
			completion_code, adapter_status, target_status)) return false;
		return completion_code == SCSI_CMD_COMPLETE_GOOD &&
			adapter_status == SCSI_ADAPTER_STATUS_OK &&
			target_status == SCSI_TARGET_STATUS_GOOD;
	}

	static bool _LOCAL_ShouldRetrySense(const byte sense_buf[18]) {
		const auto* sense = reinterpret_cast<const ScsiFixedSenseData*>(sense_buf);
		const byte key = sense->sense_key & 0x0Fu;
		if (key == SCSI_SENSE_UNIT_ATTENTION) return true;
		if (key == SCSI_SENSE_NOT_READY && sense->asc == 0x04 && sense->ascq == 0x01) return true;
		return false;
	}

	static void _LOCAL_LogSenseFailure(const char* op_name, uint64 lba, stduint block_count,
		byte completion_code, byte adapter_status, byte target_status,
		const byte sense_buf[18], bool have_sense) {
		if (have_sense) {
			const auto* sense = reinterpret_cast<const ScsiFixedSenseData*>(sense_buf);
			plogwarn("[SCSI] %s fail lba=%[64H] blocks=%u comp=%[8H] host=%[8H] tgt=%[8H] key=%[8H] asc=%[8H] ascq=%[8H]",
				op_name,
				lba,
				(unsigned)block_count,
				(unsigned)completion_code,
				(unsigned)adapter_status,
				(unsigned)target_status,
				(unsigned)(sense->sense_key & 0x0Fu),
				(unsigned)sense->asc,
				(unsigned)sense->ascq);
		}
		else {
			plogwarn("[SCSI] %s fail lba=%[64H] blocks=%u comp=%[8H] host=%[8H] tgt=%[8H] no-sense",
				op_name,
				lba,
				(unsigned)block_count,
				(unsigned)completion_code,
				(unsigned)adapter_status,
				(unsigned)target_status);
		}
	}

	static bool _LOCAL_ExecuteRw10(Harddisk_SCSI& disk, byte opcode, uint64 lba,
		void* data_buf, stduint block_count, bool data_in, const char* op_name) {
		if (!disk.fn_exec || !data_buf || !block_count || !disk.Block_Size) return false;
		if (lba >= disk.total_blocks || lba + block_count > disk.total_blocks) return false;
		if (lba > 0xFFFFFFFFull || block_count > 0xFFFFu) return false;

		byte cdb[10] = {};
		cdb[0] = opcode;
		cdb[2] = byte(lba >> 24);
		cdb[3] = byte(lba >> 16);
		cdb[4] = byte(lba >> 8);
		cdb[5] = byte(lba);
		cdb[7] = byte(block_count >> 8);
		cdb[8] = byte(block_count);

		const stduint data_len = disk.Block_Size * block_count;
		for (int attempt = 0; attempt < 3; ++attempt) {
			byte completion_code = 0xFF;
			byte adapter_status = 0xFF;
			byte target_status = 0xFF;
			if (!disk.fn_exec(disk.exec_context, disk.target_id, disk.lun,
				cdb, 10, data_buf, data_len, data_in,
				completion_code, adapter_status, target_status)) {
				_LOCAL_LogSenseFailure(op_name, lba, block_count,
					completion_code, adapter_status, target_status,
					nullptr, false);
				return false;
			}
			if (completion_code == SCSI_CMD_COMPLETE_GOOD &&
				adapter_status == SCSI_ADAPTER_STATUS_OK &&
				target_status == SCSI_TARGET_STATUS_GOOD) {
				return true;
			}

			byte sense_buf[18] = {};
			const bool have_sense =
				(target_status == SCSI_TARGET_STATUS_CHECK_CONDITION) &&
				_LOCAL_RequestSense(disk, sense_buf);
			if (have_sense && _LOCAL_ShouldRetrySense(sense_buf)) {
				continue;
			}
			_LOCAL_LogSenseFailure(op_name, lba, block_count,
				completion_code, adapter_status, target_status,
				sense_buf, have_sense);
			return false;
		}

		byte sense_buf[18] = {};
		const bool have_sense = _LOCAL_RequestSense(disk, sense_buf);
		_LOCAL_LogSenseFailure(op_name, lba, block_count,
			0xFF, 0xFF, SCSI_TARGET_STATUS_CHECK_CONDITION,
			sense_buf, have_sense);
		return false;
	}

	void Harddisk_SCSI::Bind(byte target, byte lunx, void* context, ExecCommandFn exec_fn) {
		target_id = target;
		lun = lunx;
		exec_context = context;
		fn_exec = exec_fn;
		hd_info = {};
		hd_info_valid = false;
		total_blocks = 0;
	}

	void Harddisk_SCSI::UpdateCapacity(uint32 last_lba, uint32 block_size) {
		total_blocks = uint64(last_lba) + 1;
		Block_Size = block_size ? block_size : 512;
		hd_info.whole_disk.address = 0;
		hd_info.whole_disk.length = (stduint)total_blocks;
		hd_info.whole_disk.sys_id = 0;
		hd_info.scheme_kind = PartitionSchemeKind::Unknown;
		hd_info.ResetUnifiedParts();
		hd_info_valid = false;
	}

	bool Harddisk_SCSI::ReadBlocks(uint64 lba, void* dest, stduint block_count) {
		return _LOCAL_ExecuteRw10(*this, SCSI_CMD_READ10, lba, dest, block_count, true, "READ(10)");
	}

	bool Harddisk_SCSI::WriteBlocks(uint64 lba, const void* src, stduint block_count) {
		return _LOCAL_ExecuteRw10(*this, SCSI_CMD_WRITE10, lba,
			const_cast<void*>(src), block_count, false, "WRITE(10)");
	}

	bool Harddisk_SCSI::Read(stduint BlockIden, void* Dest) {
		return ReadBlocks(BlockIden, Dest, 1);
	}

	bool Harddisk_SCSI::Write(stduint BlockIden, const void* Sors) {
		return WriteBlocks(BlockIden, Sors, 1);
	}

	stduint Harddisk_SCSI::getUnits() {
		return (stduint)total_blocks;
	}

	PartitionSlice Harddisk_SCSI::getSlice(stduint dev) {
		return GetPartitionSlice(hd_info, dev);
	}

}
#endif
