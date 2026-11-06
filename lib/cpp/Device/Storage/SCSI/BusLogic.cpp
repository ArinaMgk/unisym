// ASCII CPP-ISO11 TAB4 LF
// Docutitle: BusLogic MultiMaster SCSI controller
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../../inc/cpp/Device/Storage/SCSI/BusLogic.hpp"
#if defined(_MCCA) && ((_MCCA & 0xFF00) == 0x8600)

namespace {
	enum : byte {
		BLOGIC_CNTRL_REG = 0,
		BLOGIC_STATUS_REG = 0,
		BLOGIC_CMD_PARM_REG = 1,
		BLOGIC_DATAIN_REG = 1,
		BLOGIC_INT_REG = 2,
		BLOGIC_GET_BOARD_ID = 0x04,
		BLOGIC_INQ_DEV0TO7 = 0x0A,
		BLOGIC_INQ_CONFIG = 0x0B,
		BLOGIC_INQ_SETUPINFO = 0x0D,
		BLOGIC_INQ_DEV8TO15 = 0x23,
		BLOGIC_INQ_DEV = 0x24,
		BLOGIC_DISABLE_INT = 0x25,
		BLOGIC_EXEC_MBOX_CMD = 0x02,
		BLOGIC_INIT_EXT_MBOX = 0x81,
		BLOGIC_SETCCB_FMT = 0x96,
		BLOGIC_INQ_FWVER_D3 = 0x84,
		BLOGIC_INQ_FWVER_LETTER = 0x85,
		BLOGIC_INQ_PCI_INFO = 0x86,
		BLOGIC_INQ_MODELNO = 0x8B,
		BLOGIC_INQ_EXTSETUP = 0x8D,
	};

	enum : byte {
		BLOGIC_CTRL_INT_RESET = 1u << 5,
		BLOGIC_CTRL_SOFT_RESET = 1u << 6,
		BLOGIC_CTRL_HARD_RESET = 1u << 7,
	};

	enum : byte {
		BLOGIC_STAT_CMD_INVALID = 1u << 0,
		BLOGIC_STAT_DATAIN_READY = 1u << 2,
		BLOGIC_STAT_CMD_BUSY = 1u << 3,
		BLOGIC_STAT_ADAPTER_READY = 1u << 4,
		BLOGIC_STAT_INIT_REQUIRED = 1u << 5,
		BLOGIC_STAT_DIAG_FAILED = 1u << 6,
		BLOGIC_STAT_DIAG_ACTIVE = 1u << 7,
	};

	enum : byte {
		BLOGIC_INT_CMD_COMPLETE = 1u << 2,
	};

	enum : byte {
		BLOGIC_MBOX_START = 0x01,
		BLOGIC_INBOX_FREE = 0x00,
		BLOGIC_CMD_COMPLETE_GOOD = 0x01,
		BLOGIC_CMD_ABORT_BY_HOST = 0x02,
		BLOGIC_CMD_NOTFOUND = 0x03,
		BLOGIC_CMD_COMPLETE_ERROR = 0x04,
		BLOGIC_INVALID_CCB = 0x05,
		BLOGIC_CCB_INITIATOR = 0x00,
		BLOGIC_CCB_RESIDUAL = 0x03, // BLOGIC_INITIATOR_CCBB_RESIDUAL.
		BLOGIC_DATA_UNDERRUN = 0x0C,
		BLOGIC_DIR_DATAIN = 0x01,
		BLOGIC_DIR_DATAOUT = 0x02,
		BLOGIC_DIR_NOTX = 0x03,
		BLOGIC_LEGACY_LUN_CCB = 0x00,
		BLOGIC_EXT_LUN_CCB = 0x01,
		BLOGIC_ADAPTER_STATUS_OK = 0x00,
		BLOGIC_TARGET_STATUS_GOOD = 0x00,
	};

	static void PauseIO() {
		for (volatile stduint spin = 0; spin < 256; ++spin) {
			_ASM volatile("" ::: "memory");
		}
	}

}

namespace uni::SCSI {
	bool BusLogic::Bind(word port, const Services& host_services) {
		if (faulted || !port || !host_services.EnsureDma || !host_services.DisableDma) return false;
		io_base = port;
		services = host_services;
		if (bool(services.LockInterrupt) != bool(services.UnlockInterrupt)) return false;
		queue.services = services.commands;
		if (!queue.services.AllocateDma || !queue.services.ReleaseDma || !queue.services.Milliseconds) return false;
		return true;
	}

	bool BusLogic::Initialize(bool pci_intx_masked) {
		if (faulted) return false;
		if (!HardwareReset(true)) {
			plogwarn("[SCSI] hard reset failed");
			return false;
		}
		mailbox_count = 0;
		next_outbox_index = 0;
		// Reset restores adapter IRQ generation; establish polling before any adapter/mailbox commands.
		if (!PreparePolling(pci_intx_masked)) return false;
		DumpInquiryResults();
		return IsReady();
	}

	bool BusLogic::EnsureRegion(DmaSlot slot, DmaBuffer& region, stduint bytes) {
		if (faulted) return false;
		if (region.virt && region.size >= bytes) return true;
		// Clear cached addresses even when host-side growth fails after releasing the previous region.
		DmaBuffer replacement{};
		const bool ready = services.EnsureDma(services.context, slot, bytes, replacement);
		region = replacement;
		return ready && region.virt && region.size >= bytes &&
			!(region.device_address & 3u) && uint64(region.device_address) + region.size <= 0x100000000ull;
	}

	bool BusLogic::ExecuteScsi(byte target, byte lun, const byte* cdb, byte cdb_length,
		void* data, stduint bytes, bool data_in, byte& completion, byte& host, byte& target_status) {
		completion = byte(Completion::Failed);
		host = byte(HostStatus::Error);
		target_status = 0xFF;
		if (bytes && !data) return false;
		auto request = SubmitScsi(target, lun, cdb, cdb_length, data, bytes, data_in, 60000, 0);
		if (!request) return false;
		// Compatibility for hosts without a scheduler; Mecocoa waits outside the command mutex.
		for0(attempt, 10000000u) {
			Service();
			if (request->IsDone()) return request->CopyResult(data, completion, host, target_status);
			_ASM volatile("pause" ::: "memory");
		}
		(void)Recover();
		return request->CopyResult(data, completion, host, target_status);
	}

	byte BusLogic::ReadStatus() const {
		return innpb(io_base + BLOGIC_STATUS_REG);
	}

	byte BusLogic::ReadInterrupt() const {
		const bool previous_if = services.LockInterrupt ? services.LockInterrupt(services.context) : false;
		const byte status = innpb(io_base + BLOGIC_INT_REG) | pending_interrupt.load(MemoryOrder_Acquire);
		if (services.UnlockInterrupt) services.UnlockInterrupt(services.context, previous_if);
		return status;
	}

	byte BusLogic::ReadDataIn() const {
		return innpb(io_base + BLOGIC_DATAIN_REG);
	}

	void BusLogic::WriteControl(byte value) const {
		outpb(io_base + BLOGIC_CNTRL_REG, value);
	}

	void BusLogic::WriteCommand(byte value) const {
		outpb(io_base + BLOGIC_CMD_PARM_REG, value);
	}

	void BusLogic::InterruptReset() const {
		const bool previous_if = services.LockInterrupt ? services.LockInterrupt(services.context) : false;
		pending_interrupt.store(0, MemoryOrder_Release);
		WriteControl(BLOGIC_CTRL_INT_RESET);
		if (services.UnlockInterrupt) services.UnlockInterrupt(services.context, previous_if);
	}

	void BusLogic::ClearAdapterInterruptState() const {
		byte intreg = ReadInterrupt();
		if (intreg & 0x80u) {
			InterruptReset();
		}
	}

	void BusLogic::DisableAfterCommandFailure() {
		faulted = true;
		services.DisableDma(services.context);
		ClearAdapterInterruptState();
		// A late completion may still arrive; the host must retain every DMA region after failure.
		plogwarn("[SCSI] controller disabled after command failure; DMA buffers retained");
	}

	void BusLogic::ClearLocalCommandState() const {
		stduint drain_limit = 64;
		while (drain_limit--) {
			byte status = ReadStatus();
			if ((status & BLOGIC_STAT_DATAIN_READY) == 0) break;
			(void)ReadDataIn();
		}
		ClearAdapterInterruptState();
	}

	bool BusLogic::HardwareReset(bool hard_reset) {
		if (!HasIO()) return false;

		WriteControl(hard_reset ? BLOGIC_CTRL_HARD_RESET : BLOGIC_CTRL_SOFT_RESET);

		bool diagnostic_active = false;
		for0(spin, 50000) {
			byte status = ReadStatus();
			if (status & BLOGIC_STAT_DIAG_ACTIVE) {
				diagnostic_active = true;
				break;
			}
			PauseIO();
		}
		if (!diagnostic_active) return false;

		for0(spin, 100000) {
			byte status = ReadStatus();
			if ((status & BLOGIC_STAT_DIAG_ACTIVE) == 0) {
				diagnostic_active = false;
				break;
			}
			PauseIO();
		}
		if (diagnostic_active) return false;

		for0(spin, 10000) {
			byte status = ReadStatus();
			if (status & (BLOGIC_STAT_DIAG_FAILED | BLOGIC_STAT_ADAPTER_READY | BLOGIC_STAT_DATAIN_READY)) {
				if ((status & BLOGIC_STAT_DIAG_FAILED) || !(status & BLOGIC_STAT_ADAPTER_READY)) {
					return false;
				}
				if (status & BLOGIC_STAT_DATAIN_READY) {
					(void)ReadDataIn();
				}
				plogtrac("[SCSI] reset %s ok status=%[8H]",
					hard_reset ? "hard" : "soft", (unsigned)status);
				return true;
			}
			PauseIO();
		}
		return false;
	}

	bool BusLogic::WaitUntilReady(stduint limit) const {
		while (limit--) {
			byte status = ReadStatus();
			if ((status & (BLOGIC_STAT_ADAPTER_READY | BLOGIC_STAT_CMD_BUSY)) == BLOGIC_STAT_ADAPTER_READY) {
				return true;
			}
			PauseIO();
		}
		return false;
	}

	int BusLogic::Command(byte opcode, const void* param_data, stduint param_len, void* reply_data, stduint reply_len) const {
		if (!HasIO() || faulted) return -1;
		ClearLocalCommandState();
		if (!WaitUntilReady()) return -2;

		const byte* param = reinterpret_cast<const byte*>(param_data);
		byte* reply = reinterpret_cast<byte*>(reply_data);
		stduint reply_bytes = 0;
		WriteCommand(opcode);

		stduint param_timeout = 100000;
		while (param_len && param_timeout--) {
			PauseIO();
			byte intreg = ReadInterrupt();
			byte status = ReadStatus();
			if (intreg & BLOGIC_INT_CMD_COMPLETE) break;
			if (status & BLOGIC_STAT_DATAIN_READY) break;
			if (status & BLOGIC_STAT_CMD_BUSY) continue;
			WriteCommand(*param++);
			--param_len;
		}
		if (param_len) {
			ClearLocalCommandState();
			return -3;
		}

		stduint reply_timeout = 100000;
		if (opcode == BLOGIC_INQ_DEV0TO7 ||
			opcode == BLOGIC_INQ_DEV8TO15 ||
			opcode == BLOGIC_INQ_DEV) {
			reply_timeout = 6000000;
		}
		while (reply_timeout--) {
			byte intreg = ReadInterrupt();
			byte status = ReadStatus();
			if (status & BLOGIC_STAT_DATAIN_READY) {
				byte value = ReadDataIn();
				if (reply && reply_bytes < reply_len) {
					reply[reply_bytes] = value;
				}
				++reply_bytes;
				continue;
			}
			if (intreg & BLOGIC_INT_CMD_COMPLETE) {
				InterruptReset();
				if (status & BLOGIC_STAT_CMD_INVALID) {
					ClearLocalCommandState();
					return -4;
				}
				return int(reply_bytes);
			}
			PauseIO();
		}
		ClearLocalCommandState();
		return -5;
	}

	bool BusLogic::QueryBoardId(BusLogicBoardId& out) const {
		MemSet(&out, 0, sizeof(out));
		return Command(BLOGIC_GET_BOARD_ID, nullptr, 0, &out, sizeof(out)) == sizeof(out);
	}

	bool BusLogic::PreparePolling(bool pci_intx_masked) const {
		if (pci_intx_masked) return true;
		// BusLogic MultiMaster Disable Host Adapter Interrupt (0x25): zero masks IRQ, but CMDC remains pollable.
		const byte enable = 0;
		if (Command(BLOGIC_DISABLE_INT, &enable, sizeof(enable), nullptr, 0) != 0) {
			plogwarn("[SCSI] cannot disable adapter IRQ for polling");
			return false;
		}
		ClearAdapterInterruptState();
		plogtrac("[SCSI] PCI INTx masking unavailable; adapter IRQ disabled for polling");
		return true;
	}

	bool BusLogic::QueryConfig(BusLogicConfig& out) const {
		MemSet(&out, 0, sizeof(out));
		return Command(BLOGIC_INQ_CONFIG, nullptr, 0, &out, sizeof(out)) == sizeof(out);
	}

	bool BusLogic::QuerySetupInfo(BusLogicSetupInfo& out) const {
		byte reply_len = sizeof(out.bytes);
		MemSet(&out, 0, sizeof(out));
		return Command(BLOGIC_INQ_SETUPINFO, &reply_len, 1, out.bytes, sizeof(out.bytes)) == sizeof(out.bytes);
	}

	bool BusLogic::QueryExtSetup(BusLogicExtSetup& out) const {
		byte reply_len = sizeof(out.bytes);
		MemSet(&out, 0, sizeof(out));
		return Command(BLOGIC_INQ_EXTSETUP, &reply_len, 1, out.bytes, sizeof(out.bytes)) == sizeof(out.bytes);
	}

	bool BusLogic::QueryPciInfo(BusLogicPciInfo& out) const {
		MemSet(&out, 0, sizeof(out));
		return Command(BLOGIC_INQ_PCI_INFO, nullptr, 0, &out, sizeof(out)) == sizeof(out);
	}

	bool BusLogic::QueryFirmwareExtra(byte& digit3, byte& letter) const {
		digit3 = 0;
		letter = 0;
		const bool ok3 = Command(BLOGIC_INQ_FWVER_D3, nullptr, 0, &digit3, 1) == 1;
		const bool ok4 = Command(BLOGIC_INQ_FWVER_LETTER, nullptr, 0, &letter, 1) == 1;
		return ok3 && ok4;
	}

	bool BusLogic::QueryModelNumber(char model[6]) const {
		MemSet(model, 0, 6);
		return Command(BLOGIC_INQ_MODELNO, nullptr, 0, model, 5) == 5;
	}

	bool BusLogic::QueryInstalledTargets(uint16& out_mask) const {
		out_mask = 0;
		return Command(BLOGIC_INQ_DEV, nullptr, 0, &out_mask, sizeof(out_mask)) == sizeof(out_mask);
	}

	bool BusLogic::QueryInstalledTargetsLegacy(byte out_luns0to7[8]) const {
		MemSet(out_luns0to7, 0, 8);
		return Command(BLOGIC_INQ_DEV0TO7, nullptr, 0, out_luns0to7, 8) == 8;
	}

	bool BusLogic::InitializeMailbox(byte requested_count) {
		if (!HasIO()) return false;
		if (mailbox_region.virt && mailbox_count == requested_count) return true;

		const stduint total_size =
			stduint(requested_count) * (sizeof(BusLogicOutbox) + sizeof(BusLogicInbox));
		if (!EnsureRegion(DmaSlot::Mailbox, mailbox_region, total_size)) return false;

		MemSet(mailbox_region.virt, 0, total_size);
		outboxes = reinterpret_cast<BusLogicOutbox*>(mailbox_region.virt);
		inboxes = reinterpret_cast<BusLogicInbox*>(
			mailbox_region.virt + requested_count * sizeof(BusLogicOutbox));

		BusLogicExtMailboxInitReq req{};
		req.mbox_count = requested_count;
		req.base_mbox_addr = mailbox_region.device_address;
		if (Command(BLOGIC_INIT_EXT_MBOX, &req, sizeof(req), nullptr, 0) < 0) {
			DisableAfterCommandFailure();
			return false;
		}
		mailbox_count = requested_count;
		next_outbox_index = 0;
		plogtrac("[SCSI] mailbox init count=%u base=%[32H]",
			(unsigned)mailbox_count, req.base_mbox_addr);
		return true;
	}

	bool BusLogic::SetCcbFormat(byte fmt) {
		if (Command(BLOGIC_SETCCB_FMT, &fmt, 1, nullptr, 0) < 0) return false;
		extended_ccb = fmt == BLOGIC_EXT_LUN_CCB;
		return true;
	}

	CommandHandle BusLogic::SubmitScsi(stduint target, byte lun, const byte* cdb, byte cdb_length,
		const void* data, stduint bytes, bool data_in, uint32 timeout_ms, stduint origin) {
		if (!IsReady() || target >= MaxTargets || lun >= MaxLuns || cdb_length > 12) return {};
		auto request = queue.Enqueue(0, byte(target), lun, cdb, cdb_length, data, bytes, data_in,
			sizeof(BusLogicCcb32), timeout_ms, origin);
		if (request) StartQueued();
		queue.Prune();
		return request;
	}

	void BusLogic::StartQueued() {
		queue.ExpireQueued();
		for0(i, queue.Count()) {
			auto owner = queue.getCommand(i);
			auto& command = *owner;
			if (!command.IsQueued()) continue;
			if (active_count >= mailbox_count) break;
			if (!queue.CanStart(command)) continue;
			stduint index = next_outbox_index;
			for0(attempt, mailbox_count) {
				index = (next_outbox_index + attempt) % mailbox_count;
				if (!outboxes[index].action) break;
			}
			if (outboxes[index].action) break;
			MemSet(command.dma.virt, 0, sizeof(BusLogicCcb32));
			auto* ccb = reinterpret_cast<BusLogicCcb32*>(command.dma.virt);
			ccb->opcode = BLOGIC_CCB_RESIDUAL;
			ccb->control = byte((!command.data_length ? BLOGIC_DIR_NOTX :
				(command.data_in ? BLOGIC_DIR_DATAIN : BLOGIC_DIR_DATAOUT)) << 3);
			ccb->cdb_length = command.cdb_length;
			ccb->sense_length = 32;
			ccb->data_length = uint32(command.data_length);
			ccb->data_addr = command.data_length ? command.dma.device_address + uint32(command.data_offset) : 0;
			ccb->target_id = command.target;
			ccb->lun_legacy = command.lun;
			if (command.target_state->tagged) {
				// BusLogic.h: SIMPLE tag enable is byte 1 bit 5 in extended format, byte 17 bit 5 in legacy format.
				if (extended_ccb) ccb->control |= 0x20;
				else ccb->lun_legacy |= 0x20;
			}
			MemCopyN(ccb->cdb, command.cdb, command.cdb_length);
			ccb->sense_addr = command.dma.device_address + uint32(command.sense_offset);
			if (!command.Start(queue.services.Milliseconds(queue.services.context))) continue;
			++active_count;
			outboxes[index].ccb = command.dma.device_address;
			// Publish ownership only after the CCB, data and mailbox address are visible to the adapter.
			__sync_synchronize();
			outboxes[index].action = BLOGIC_MBOX_START;
			next_outbox_index = (index + 1) % mailbox_count;
			__sync_synchronize();
			WriteCommand(BLOGIC_EXEC_MBOX_CMD);
		}
	}

	void BusLogic::SetInterrupts(bool enabled) {
		interrupts_enabled.store(0, MemoryOrder_Release);
		if (!IsReady()) return;
		if (enabled && (!services.LockInterrupt || !services.UnlockInterrupt)) return;
		const byte value = enabled ? 1 : 0;
		if (Command(BLOGIC_DISABLE_INT, &value, 1, nullptr, 0) != 0) {
			plogwarn("[SCSI/BusLogic] cannot configure adapter interrupts");
			return;
		}
		ClearAdapterInterruptState();
		interrupts_enabled.store(enabled, MemoryOrder_Release);
	}

	bool BusLogic::OnInterrupt() {
		const bool previous_if = services.LockInterrupt ? services.LockInterrupt(services.context) : false;
		const byte status = innpb(io_base + BLOGIC_INT_REG);
		// Preserve CMDC for polling administrative commands too; otherwise an IRQ can steal their completion during reset.
		if (status != 0xFF && (status & 0x80u)) {
			WriteControl(BLOGIC_CTRL_INT_RESET);
			pending_interrupt.store(byte(pending_interrupt.load(MemoryOrder_Relaxed) | status), MemoryOrder_Release);
		}
		if (services.UnlockInterrupt) services.UnlockInterrupt(services.context, previous_if);
		return status != 0xFF && (status & 0x80u) != 0;
	}

	bool BusLogic::Recover() {
		const bool restore_interrupts = interrupts_enabled.load(MemoryOrder_Acquire);
		SetInterrupts(false);
		plogwarn("[SCSI/BusLogic] resetting adapter; outstanding commands fail without replay");
		if (!HardwareReset(true)) {
			DisableAfterCommandFailure();
			queue.FailAll(false);
			return false;
		}
		active_count = 0;
		queue.FailAll(true);
		const byte count = byte(mailbox_count);
		mailbox_count = 0;
		faulted = false;
		if (!PreparePolling(false) || !InitializeMailbox(count) ||
			(extended_ccb && !SetCcbFormat(BLOGIC_EXT_LUN_CCB))) {
			DisableAfterCommandFailure();
			return false;
		}
		SetInterrupts(restore_interrupts);
		return true;
	}

	void BusLogic::Service() {
		if (!IsReady()) return;
		for (;;) {
			for0(i, mailbox_count) {
				auto* inbox = &inboxes[i];
				const byte completion = inbox->completion_code;
				if (!completion) continue;
				__sync_synchronize();
				const uint32 address = inbox->ccb;
				const byte host = inbox->adapter_status, target_status = inbox->target_status;
				CommandHandle command;
				for0(index, queue.Count()) {
					auto pending = queue.getCommand(index);
					if (pending->IsActive() && pending->dma.device_address == address) {
						command = pending;
						break;
					}
				}
				if (!command) {
					(void)Recover();
					return;
				}
				auto* ccb = reinterpret_cast<BusLogicCcb32*>(command->dma.virt);
				const uint32 residual = ccb->data_length;
				if (residual > command->data_length) {
					(void)Recover();
					return;
				}
				const uint32 transferred = uint32(command->data_length) - residual;
				const byte opcode = command->cdb[0];
				const bool short_reply = command->data_in && (opcode == 0x12 || opcode == 0x03 || opcode == 0xA0);
				bool good = host == BLOGIC_ADAPTER_STATUS_OK || (host == BLOGIC_DATA_UNDERRUN && short_reply);
				if (target_status == BLOGIC_TARGET_STATUS_GOOD && residual && !short_reply) good = false;
				if (target_status == BLOGIC_TARGET_STATUS_GOOD && opcode == 0xA0 && command->data_in) {
					if (transferred < 8) good = false;
					else {
						const byte* p = command->Data();
						const uint64 needed = uint64((uint32(p[0]) << 24) | (uint32(p[1]) << 16) | (uint32(p[2]) << 8) | p[3]) + 8;
						if (transferred < (needed < command->data_length ? needed : command->data_length)) good = false;
					}
				}
				__sync_synchronize();
				inbox->completion_code = BLOGIC_INBOX_FREE;
				--active_count;
				const bool received = completion == BLOGIC_CMD_COMPLETE_GOOD || completion == BLOGIC_CMD_COMPLETE_ERROR;
				const CommandResult result{byte(received ? Completion::Completed : Completion::Failed),
					byte(good ? HostStatus::Good : HostStatus::Error), target_status};
				queue.Complete(*command, received, result, transferred,
					target_status == byte(Status::CheckCondition) && (command->Sense()[0] & 0x70) == 0x70 ? 32 : 0);
			}
			ClearAdapterInterruptState();
			// A completion racing interrupt acknowledgement remains visible in its inbox.
			__sync_synchronize();
			bool pending = false;
			for0(i, mailbox_count) if (inboxes[i].completion_code) pending = true;
			if (!pending) break;
		}
		if (queue.HasExpired(true)) {
			const uint32 now = queue.services.Milliseconds(queue.services.context);
			for0(i, queue.Count()) {
				auto request = queue.getCommand(i);
				if (request->IsActive() && request->HasExpired(now)) {
					plogwarn("[SCSI/BusLogic] request timeout CCB=%[32H] t%ul%u origin=%u",
						request->dma.device_address, (unsigned)request->target, (unsigned)request->lun, request->getOrigin());
				}
			}
			(void)Recover();
			return;
		}
		queue.ExpireQueued();
		queue.Prune();
		StartQueued();
		queue.Prune();
	}

	void BusLogic::DumpInquiryResults() {
		if (!HasIO()) return;

		BusLogicBoardId board_id{};
		if (QueryBoardId(board_id)) {
			plogtrac("[SCSI] board id type=%[8H] fw=%c.%c custom=%[8H]",
				(unsigned)board_id.type,
				board_id.fw_ver_digit1 ? board_id.fw_ver_digit1 : '?',
				board_id.fw_ver_digit2 ? board_id.fw_ver_digit2 : '?',
				(unsigned)board_id.custom_features);
			if (board_id.fw_ver_digit1 == '4') {
				ploginfo("[SCSI] family=BusLogic MultiMaster C-series (BT-946C/956C/956CD class)");
			}
			else if (board_id.fw_ver_digit1 == '5') {
				ploginfo("[SCSI] family=BusLogic MultiMaster W-series (BT-948/958/958D class)");
			}
		}
		else {
			plogwarn("[SCSI] GET_BOARD_ID failed");
		}

		BusLogicConfig config{};
		if (QueryConfig(config)) {
			// ploginfo("[SCSI] config dma=%[8H] irq=%[8H] hostid=%u",
			// 	(unsigned)config.byte0,
			// 	(unsigned)config.byte1,
			// 	(unsigned)(config.byte2 & 0x0Fu));
		}
		else {
			plogwarn("[SCSI] INQ_CONFIG failed");
		}

		BusLogicSetupInfo setup{};
		if (QuerySetupInfo(setup)) {
			(void)setup;
			// ploginfo("[SCSI] setup parity=%u sync=%u tx=%u preempt=%u timeoff=%u mbox=%u base=%[32H]",
			// 	(unsigned)((flags >> 1) & 1u),
			// 	(unsigned)(flags & 1u),
			// 	(unsigned)setup.bytes[1],
			// 	(unsigned)setup.bytes[2],
			// 	(unsigned)setup.bytes[3],
			// 	(unsigned)setup.bytes[4],
			// 	mbox_addr);
		}
		else {
			plogwarn("[SCSI] INQ_SETUPINFO failed");
		}

		BusLogicExtSetup ext{};
		byte ext_mbox_count = 0;
		if (QueryExtSetup(ext)) {
			ext_mbox_count = ext.bytes[4];
			(void)ext;
			// ploginfo("[SCSI] extsetup bus=%[8H] bios=%[8H] sg-limit=%u mbox=%u fw=%c%c%c features=%[8H]",
			// 	(unsigned)ext.bytes[0],
			// 	(unsigned)ext.bytes[1],
			// 	(unsigned)(uint16(ext.bytes[2]) | (uint16(ext.bytes[3]) << 8)),
			// 	(unsigned)ext.bytes[4],
			// 	ext.bytes[10] ? ext.bytes[10] : '?',
			// 	ext.bytes[11] ? ext.bytes[11] : '?',
			// 	ext.bytes[12] ? ext.bytes[12] : '?',
			// 	(unsigned)feature);
			// ploginfo("[SCSI] caps wide=%u differential=%u scam=%u ultra=%u smart-term=%u",
			// 	(unsigned)((feature >> 0) & 1u),
			// 	(unsigned)((feature >> 1) & 1u),
			// 	(unsigned)((feature >> 2) & 1u),
			// 	(unsigned)((feature >> 3) & 1u),
			// 	(unsigned)((feature >> 4) & 1u));
		}
		else {
			plogwarn("[SCSI] INQ_EXTSETUP failed");
		}

		if (ext_mbox_count) {
			const byte init_mbox_count = ext_mbox_count;
			if (!InitializeMailbox(init_mbox_count)) {
				plogwarn("[SCSI] INIT_EXT_MBOX failed");
			}
			else {
				if (SetCcbFormat(BLOGIC_EXT_LUN_CCB)) {
					plogtrac("[SCSI] ccb format=extended");
				}
				else {
					plogwarn("[SCSI] SETCCB_FMT failed");
				}
				BusLogicSetupInfo setup_after_mbox{};
				if (QuerySetupInfo(setup_after_mbox)) {
					(void)setup_after_mbox;
					// ploginfo("[SCSI] setup-after-mbox mbox=%u base=%[32H]",
					// 	(unsigned)setup_after_mbox.bytes[4], mbox_addr_after);
				}
			}
		}

		BusLogicPciInfo pci{};
		if (QueryPciInfo(pci)) {
			(void)pci;
			// ploginfo("[SCSI] pci-info isa-port=%u irq=%u low-term=%u high-term=%u valid=%u",
			// 	(unsigned)pci.isa_port,
			// 	(unsigned)pci.irq_channel,
			// 	(unsigned)(pci.termination & 0x01u),
			// 	(unsigned)((pci.termination >> 1) & 0x01u),
			// 	(unsigned)((pci.termination >> 7) & 0x01u));
		}
		else {
			plogwarn("[SCSI] INQ_PCI_INFO failed");
		}

		byte fw_digit3 = 0;
		byte fw_letter = 0;
		if (QueryFirmwareExtra(fw_digit3, fw_letter)) {
			(void)fw_digit3;
			(void)fw_letter;
			// ploginfo("[SCSI] firmware extra d3=%c letter=%c",
			// 	fw_digit3 ? fw_digit3 : '?',
			// 	fw_letter ? fw_letter : '?');
		}
		else {
			plogwarn("[SCSI] INQ_FWVER extra failed");
		}
	}
}
#endif
