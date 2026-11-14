// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Storage) Harddisk - SCSI
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../inc/c/storage/harddisk.h"

#if defined(_MCCA) && ((_MCCA) == 0x8632 || _MCCA == 0x8664)
namespace uni {
	bool SCSI::CommandRequest::CopyResult(void* data, byte& completion, byte& host, byte& target_status) const {
		if (!IsDone()) return false;
		completion = result.completion;
		host = result.host_status;
		target_status = result.target_status;
		if (data_in && data_length && result.Succeeded() && data) {
			MemSet(data, 0, data_length);
			MemCopyN(data, Data(), getCompletion().transferred);
		}
		return received;
	}

	SCSI::CommandHandle SCSI::CommandQueue::Enqueue(byte bus, byte target, byte lun, const byte* cdb,
		byte cdb_length, const void* data, stduint bytes, bool data_in, stduint frame_bytes, uint32 timeout_ms, stduint origin) {
		if (!cdb || !cdb_length || cdb_length > 16 || (bytes && !data && !data_in) || !timeout_ms || timeout_ms > 0x7FFFFFFFu ||
			!services.AllocateDma || !services.ReleaseDma || !services.Milliseconds || frame_bytes > 4096 ||
			uint64(bytes) + frame_bytes + 47 > 0xFFFFFFFFull || uint64(bytes) + frame_bytes + 47 > ~stduint(0)) return {};
		const uint32 started = services.Milliseconds(services.context);
		// Failed probes must not leave a target-by-LUN-by-caller allocation matrix behind.
		for (stduint i = targets.Count(); i--;) {
			if (!targets[i]->tagged && targets[i].use_count() == 1) targets.Remove(i);
		}
		for (stduint i = sense_cache.Count(); i--;) {
			if (!sense_cache[i]->valid && sense_cache[i].use_count() == 1) sense_cache.Remove(i);
		}
		auto request = make_shared<CommandRequest>();
		if (!request) return {};
		request->services = services;
		request->bus = bus;
		request->target = target;
		request->lun = lun;
		request->cdb_length = cdb_length;
		MemCopyN(request->cdb, cdb, cdb_length);
		request->data_length = bytes;
		request->data_in = data_in;
		request->sense_offset = (frame_bytes + 7) & ~stduint(7);
		request->data_offset = request->sense_offset + 32;
		for (auto& state : targets) {
			if (state->bus == bus && state->target == target && state->lun == lun) request->target_state = state;
		}
		if (!request->target_state) {
			auto state = make_shared<TargetQueueState>();
			if (!state) return {};
			state->bus = bus;
			state->target = target;
			state->lun = lun;
			if (!targets.Append(state)) return {};
			request->target_state = state;
		}
		for (auto& cached : sense_cache) {
			if (cached->origin == origin && cached->bus == bus && cached->target == target && cached->lun == lun) request->cached_sense = cached;
		}
		if (!request->cached_sense) {
			auto cached = make_shared<CommandSense>();
			if (!cached) return {};
			cached->origin = origin;
			cached->bus = bus;
			cached->target = target;
			cached->lun = lun;
			if (!sense_cache.Append(cached)) return {};
			request->cached_sense = cached;
		}
		// Caller buffers (including REQUEST SENSE on the stack) are CPU addresses, never device addresses.
		if (!services.AllocateDma(services.context, request->data_offset + bytes, request->dma)) return {};
		if (!request->dma.virt || request->dma.size < request->data_offset + bytes || (request->dma.device_address & 7) ||
			uint64(request->dma.device_address) + request->dma.size > 0x100000000ull) return {};
		MemSet(request->dma.virt, 0, request->data_offset + bytes);
		if (bytes && !data_in) MemCopyN(request->Data(), data, bytes);
		if (!StorageRequestQueue::Enqueue(request, started, timeout_ms, origin, services.context, services.Completed)) return {};
		if (cdb[0] == 0x03 && cdb_length == 6 && !(cdb[1] & 1) && data_in && request->cached_sense->valid) {
			const stduint allocation = cdb[4] < 32 ? cdb[4] : 32;
			const stduint copied = bytes < allocation ? bytes : allocation;
			if (copied) MemCopyN(request->Data(), request->cached_sense->bytes, copied);
			request->cached_sense->valid = false;
			Complete(*request, true, {byte(Completion::Completed), byte(HostStatus::Good), byte(Status::Good)}, uint32(copied));
		}
		else request->cached_sense->valid = false;
		return request;
	}

	bool SCSI::CommandQueue::CanStart(const CommandRequest& request) const {
		if (request.target_state->tagged) return true;
		for0(i, Count()) {
			auto other = getCommand(i);
			if (other->IsActive() && other->target_state.get() == request.target_state.get()) return false;
		}
		return true;
	}

	void SCSI::CommandQueue::Complete(CommandRequest& request, bool received, const CommandResult& result,
		uint32 transferred, stduint sense_bytes) {
		if (request.IsDone()) return;
		request.result = result;
		request.received = received;
		if (sense_bytes) {
			MemSet(request.cached_sense->bytes, 0, sizeof(request.cached_sense->bytes));
			MemCopyN(request.cached_sense->bytes, request.Sense(), sense_bytes < 32 ? sense_bytes : 32);
			request.cached_sense->valid = true;
		}
		if (result.Succeeded() && request.cdb[0] == 0x12 && !(request.cdb[1] & 1) && transferred >= 8) {
			request.target_state->tagged = (request.Data()[7] & 2) != 0; // SPC INQUIRY byte 7, CmdQue.
		}
		StorageRequestQueue::Complete(request,
			{received && result.Succeeded() ? StorageRequestStatus::Success : StorageRequestStatus::Failed,
			transferred <= request.data_length ? stduint(transferred) : 0}, true);
	}

	void SCSI::CommandQueue::ExpireQueued() {
		StorageRequestQueue::ExpireQueued(services.Milliseconds(services.context));
	}

	uint32 SCSI::CommandQueue::NextTimeout() const {
		return StorageRequestQueue::NextTimeout(services.Milliseconds(services.context));
	}

	bool SCSI::CommandQueue::HasExpired(bool active) const {
		return StorageRequestQueue::HasExpired(services.Milliseconds(services.context), active);
	}


	enum : byte {
		SCSI_CMD_REQUEST_SENSE = 0x03,
		SCSI_CMD_INQUIRY = 0x12,
		SCSI_CMD_READ_CAPACITY10 = 0x25,
		SCSI_CMD_READ10 = 0x28,
		SCSI_CMD_WRITE10 = 0x2A,
		SCSI_CMD_ATA_PASS_THROUGH16 = 0x85, // SAT: ATA PASS-THROUGH(16).
		// SBC: READ(16), WRITE(16) and READ CAPACITY(16) operation and service-action codes.
		SCSI_CMD_READ16 = 0x88,
		SCSI_CMD_WRITE16 = 0x8A,
		SCSI_CMD_SERVICE_ACTION_IN16 = 0x9E,
		SCSI_SA_READ_CAPACITY16 = 0x10,
		SCSI_CMD_COMPLETE_GOOD = byte(SCSI::Completion::Completed),
		SCSI_ADAPTER_STATUS_OK = byte(SCSI::HostStatus::Good),
		SCSI_TARGET_STATUS_GOOD = byte(SCSI::Status::Good),
		SCSI_TARGET_STATUS_CHECK_CONDITION = byte(SCSI::Status::CheckCondition),
		SCSI_SENSE_NOT_READY = 0x02,
		SCSI_SENSE_UNIT_ATTENTION = 0x06,
	};

	bool SCSI::Inquiry(ExecuteCommandFn execute, void* context, stduint target, byte lun,
		InquiryReply& reply, CommandResult& result) {
		result = {};
		if (!execute) return false;
		byte cdb[6] = {};
		cdb[0] = SCSI_CMD_INQUIRY;
		cdb[4] = sizeof(reply);
		MemSet(&reply, 0, sizeof(reply));
		return execute(context, target, lun, cdb, sizeof(cdb), &reply, sizeof(reply), true,
			result.completion, result.host_status, result.target_status);
	}

	bool SCSI::ReadCapacity10(ExecuteCommandFn execute, void* context, stduint target, byte lun,
		ReadCapacity10Reply& reply, CommandResult& result) {
		result = {};
		if (!execute) return false;
		byte cdb[10] = {};
		cdb[0] = SCSI_CMD_READ_CAPACITY10;
		MemSet(&reply, 0, sizeof(reply));
		return execute(context, target, lun, cdb, sizeof(cdb), &reply, sizeof(reply), true,
			result.completion, result.host_status, result.target_status);
	}

	bool SCSI::ReadCapacity16(ExecuteCommandFn execute, void* context, stduint target, byte lun,
		ReadCapacity16Reply& reply, CommandResult& result) {
		result = {};
		if (!execute) return false;
		byte cdb[16] = {};
		cdb[0] = SCSI_CMD_SERVICE_ACTION_IN16;
		cdb[1] = SCSI_SA_READ_CAPACITY16;
		cdb[13] = sizeof(reply); // SBC READ CAPACITY(16): allocation length occupies bytes 10 through 13.
		MemSet(&reply, 0, sizeof(reply));
		return execute(context, target, lun, cdb, sizeof(cdb), &reply, sizeof(reply), true,
			result.completion, result.host_status, result.target_status);
	}

	bool SCSI::IdentifyAtaDevice(ExecuteCommandFn execute, void* context, stduint target, byte lun,
		byte* data, stduint bytes, AtaIdentifyInfo& info, CommandResult& result) {
		info = {};
		result = {};
		if (!execute || !data || bytes < AtaIdentifyDataSize) return false;
		byte cdb[16] = {};
		cdb[0] = SCSI_CMD_ATA_PASS_THROUGH16;
		cdb[1] = 4u << 1; // SAT PROTOCOL: PIO data-in.
		cdb[2] = (1u << 3) | (1u << 2) | 2u; // SAT: from device, 512-byte blocks, length from sector count; CK_COND clear.
		cdb[6] = 1;
		cdb[14] = ATA_IDENTIFY;
		MemSet(data, 0, AtaIdentifyDataSize);
		if (!execute(context, target, lun, cdb, sizeof(cdb), data, AtaIdentifyDataSize, true,
			result.completion, result.host_status, result.target_status) || !result.Succeeded()) return false;

		auto read_word = [data](stduint index) -> uint16 {
			return uint16(data[2 * index]) | (uint16(data[2 * index + 1]) << 8);
		};
		// ATA ACS IDENTIFY DEVICE: word 0 distinguishes ATA from packet devices; model is words 27 through 46.
		const uint16 config = read_word(0);
		if (!config || (config & 0x8000u)) return false;
		constexpr stduint model_bytes = sizeof(info.model) - 1;
		for0(i, model_bytes) {
			const byte ch = data[2 * 27 + (i ^ 1)];
			info.model[i] = ch == 0 ? ' ' : (ch >= 0x20 && ch <= 0x7E ? char(ch) : '?');
		}
		stduint model_len = model_bytes;
		while (model_len && info.model[model_len - 1] == ' ') --model_len;
		info.model[model_len] = 0;
		if (!model_len) return false;

		// ATA ACS: word 83 validates words 82 and 83; word 169 bit 0 advertises TRIM, not SAT UNMAP support.
		const uint16 command_sets = read_word(83);
		const uint16 major = read_word(80);
		const uint16 data_set_management = read_word(169);
		const bool command_sets_valid = (command_sets & 0xC000u) == 0x4000u;
		info.smart = command_sets_valid && (read_word(82) & 1u);
		info.lba48 = command_sets_valid && (command_sets & (1u << 10));
		info.trim = major != 0xFFFFu && (major & 0xFF80u) &&
			data_set_management != 0xFFFFu && (data_set_management & 1u);
		return true;
	}

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

	static bool _LOCAL_ExecuteRw(Harddisk_SCSI& disk, uint64 lba,
		void* data_buf, stduint block_count, bool data_in) {
		if (!disk.fn_exec || !data_buf || !block_count || !disk.Block_Size) return false;
		if (lba >= disk.total_blocks || block_count > disk.total_blocks - lba) return false;
		// Large LBAs do not change the existing per-command transfer limits.
		if (block_count > 0xFFFFu || block_count > 0xFFFFFFFFull / disk.Block_Size) return false;
		const bool wide = lba + (block_count - 1) > 0xFFFFFFFFull;
		const byte cdb_length = wide ? 16 : 10;
		const char* op_name = wide ? (data_in ? "READ(16)" : "WRITE(16)") : (data_in ? "READ(10)" : "WRITE(10)");
		byte cdb[16] = {};
		if (wide) {
			// SBC READ/WRITE(16): bytes 2..9 hold the LBA and bytes 10..13 hold the block count.
			cdb[0] = data_in ? SCSI_CMD_READ16 : SCSI_CMD_WRITE16;
			for0(i, 8) cdb[2 + i] = byte(lba >> (56 - 8 * i));
			for0(i, 4) cdb[10 + i] = byte(block_count >> (24 - 8 * i));
		}
		else {
			cdb[0] = data_in ? SCSI_CMD_READ10 : SCSI_CMD_WRITE10;
			cdb[2] = byte(lba >> 24);
			cdb[3] = byte(lba >> 16);
			cdb[4] = byte(lba >> 8);
			cdb[5] = byte(lba);
			cdb[7] = byte(block_count >> 8);
			cdb[8] = byte(block_count);
		}

		const stduint data_len = stduint(uint64(disk.Block_Size) * block_count);
		for (int attempt = 0; attempt < 3; ++attempt) {
			byte completion_code = 0xFF;
			byte adapter_status = 0xFF;
			byte target_status = 0xFF;
			if (!disk.fn_exec(disk.exec_context, disk.target_id, disk.lun,
				cdb, cdb_length, data_buf, data_len, data_in,
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

	void Harddisk_SCSI::Bind(stduint target, byte lunx, void* context, ExecCommandFn exec_fn) {
		target_id = target;
		lun = lunx;
		exec_context = context;
		fn_exec = exec_fn;
		hd_info = {};
		hd_info_valid = false;
		total_blocks = 0;
	}

	void Harddisk_SCSI::UpdateCapacity(uint64 last_lba, uint32 block_size) {
		total_blocks = last_lba + 1;
		Block_Size = block_size ? block_size : 512;
		hd_info.whole_disk.address = 0;
		hd_info.whole_disk.length = (stduint)total_blocks;
		hd_info.whole_disk.sys_id = ClassicFilesysType::None;
		hd_info.scheme_kind = PartitionSchemeKind::Unknown;
		hd_info.ResetUnifiedParts();
		hd_info_valid = false;
	}

	bool Harddisk_SCSI::ReadBlocks(uint64 lba, void* dest, stduint block_count) {
		return _LOCAL_ExecuteRw(*this, lba, dest, block_count, true);
	}

	bool Harddisk_SCSI::WriteBlocks(uint64 lba, const void* src, stduint block_count) {
		return _LOCAL_ExecuteRw(*this, lba, const_cast<void*>(src), block_count, false);
	}

	bool Harddisk_SCSI::Read(stduint BlockIden, void* Dest, stduint Times) {
		return ReadBlocks(BlockIden, Dest, Times);
	}

	bool Harddisk_SCSI::Write(stduint BlockIden, const void* Sors, stduint Times) {
		return WriteBlocks(BlockIden, Sors, Times);
	}

	stduint Harddisk_SCSI::getUnits() {
		return (stduint)total_blocks;
	}

	PartitionSlice Harddisk_SCSI::getSlice(stduint dev) {
		return GetPartitionSlice(hd_info, dev);
	}

}
#endif
