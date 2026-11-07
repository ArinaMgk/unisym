// ASCII CPP-ISO11 TAB4 LF
// Docutitle: LSI Logic Parallel SCSI controller (Fusion-MPT MPI 1.x)
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../../inc/cpp/Device/Storage/SCSI/LsiLogic-Para.hpp"
#if defined(_MCCA) && ((_MCCA & 0xFF00) == 0x8600)

namespace {
	// LSI MPI specification: mpi.h, mpi_ioc.h and mpi_init.h in Linux drivers/message/fusion/lsi/.
	enum : byte {
		Doorbell = 0x00, WriteSequence = 0x04, Diagnostic = 0x08,
		InterruptStatus = 0x30, InterruptMask = 0x34, RequestQueue = 0x40, ReplyQueue = 0x44,
		ScsiIo = 0x00, IocInit = 0x02, IocFacts = 0x03, PortFacts = 0x05, PortEnable = 0x06,
		MessageUnitReset = 0x40, HandshakeFunction = 0x42,
		ScsiPort = 0x01, SasPort = 0x30, InitiatorProtocol = 0x08, HostDriver = 0x04,
		AutosenseValid = 0x01, NoScsiStatus = 0x04, Terminated = 0x08, ResponseInfoValid = 0x10,
		ScsiCmdRequestSense = 0x03, ScsiCmdInquiry = 0x12, ScsiCmdReportLuns = 0xA0,
	};
	enum : uint32 {
		StateMask = 0xF0000000u, StateReady = 0x10000000u, StateOperational = 0x20000000u,
		DoorbellActive = 0x08000000u, DoorbellBusy = 0x80000000u, DoorbellInterrupt = 0x01u,
		InterruptsMasked = 0x09u, DiagnosticWritable = 0x80u, DiagnosticReset = 0x04u,
		DiagnosticDisableArm = 0x02u, DiagnosticBadFlash = 0x40u,
		AddressReply = 0x80000000u, ContextMask = 0x1FFFFFFFu, EmptyReply = 0xFFFFFFFFu,
		RequestBlockMask = 0x03u,
		ControlRead = 0x02000000u, ControlWrite = 0x01000000u, ControlUntagged = 0x00000500u,
		SgeSimple = 0x10000000u, SgeWrite = 0x04000000u, SgeLast = 0xC1000000u,
		SgeLengthMask = 0x00FFFFFFu,
	};
	enum : uint16 {
		IocStatusMask = 0x7FFF, IocSuccess = 0x0000, ScsiRecoveredError = 0x0040,
		ScsiDeviceAbsent = 0x0043, ScsiDataUnderrun = 0x0045,
	};

	_PACKED(struct) MessageRequest {
		byte reserved0[2], chain_offset, function;
		byte reserved4[2], port, flags;
		uint32 context;
	};
	_PACKED(struct) MessageReply {
		byte reserved0[2], length, function;
		byte reserved4[3], flags;
		uint32 context;
		uint16 reserved12, status;
		uint32 log_info;
	};
	_PACKED(struct) FactsReply {
		uint16 version;
		byte length, function;
		uint16 header_version;
		byte ioc_number, msg_flags;
		uint32 context;
		uint16 exceptions, status;
		uint32 log_info;
		byte max_chain_depth, who_init, block_size, flags;
		uint16 reply_depth, request_dwords, fw_version_old, product_id;
		uint32 current_mfa_high;
		uint16 global_credits;
		byte ports, event_state;
		uint32 current_sense_high;
		uint16 current_reply_size;
		byte max_devices, max_buses;
		uint32 fw_image_size, capabilities, fw_version;
		uint16 priority_queue_depth, reserved;
		byte host_page_sge[12];
		uint32 reply_signaling_address;
	};
	_PACKED(struct) PortReply {
		MessageReply header;
		byte reserved20, type;
		uint16 max_devices, scsi_id, protocols, posted_buffers, persistent_ids, lan_buckets;
		byte max_initiators, reserved35;
		uint32 reserved36;
	};
	_PACKED(struct) InitRequest {
		byte who_init, reserved1, chain_offset, function;
		byte flags, max_devices, max_buses, msg_flags;
		uint32 context;
		uint16 reply_frame_size, reserved14;
		uint32 mfa_high, sense_high, reply_signaling_address;
		byte host_page_sge[12];
		uint16 version, header_version;
	};
	_PACKED(struct) ScsiRequest {
		byte target, bus, chain_offset, function;
		byte cdb_length, sense_length, reserved, flags;
		uint32 context;
		byte lun[8];
		uint32 control;
		byte cdb[16];
		uint32 data_length, sense_address;
	};
	_PACKED(struct) SimpleSge {
		uint32 flags_length, address;
	};
	_PACKED(struct) ScsiReply {
		byte target, bus, length, function;
		byte cdb_length, sense_length, reserved, flags;
		uint32 context;
		byte scsi_status, scsi_state;
		uint16 status;
		uint32 log_info, transfer_count, sense_count, response_info;
		uint16 task_tag, reserved34;
	};
	static_assert(sizeof(MessageRequest) == 12 && sizeof(MessageReply) == 20);
	static_assert(sizeof(FactsReply) == 80 && sizeof(PortReply) == 40 && sizeof(InitRequest) == 44);
	static_assert(sizeof(ScsiRequest) == 48 && sizeof(SimpleSge) == 8 && sizeof(ScsiReply) == 36);
	static_assert(__builtin_offsetof(FactsReply, request_dwords) == 0x1A && __builtin_offsetof(FactsReply, global_credits) == 0x24);
	static_assert(__builtin_offsetof(PortReply, scsi_id) == 0x18 && __builtin_offsetof(InitRequest, reply_frame_size) == 0x0C);
	static_assert(__builtin_offsetof(ScsiRequest, cdb) == 0x18 && __builtin_offsetof(ScsiRequest, sense_address) == 0x2C);
	static_assert(__builtin_offsetof(ScsiReply, status) == 0x0E && __builtin_offsetof(ScsiReply, transfer_count) == 0x14);
}

namespace uni::SCSI {
	bool LsiLogicMPI::Bind(word port, const Services& host_services) {
		if (faulted || ready || !port || port > 0xFFB8u ||
			!host_services.EnsureDma || !host_services.DisableDma || !host_services.Milliseconds) return false;
		io_base = port;
		services = host_services;
		queue.services = services.commands;
		if (!queue.services.AllocateDma || !queue.services.ReleaseDma || !queue.services.Milliseconds) return false;
		return true;
	}

	uint32 LsiLogicMPI::ReadRegister(byte offset) const {
		return innpd(word(io_base + offset));
	}

	void LsiLogicMPI::WriteRegister(byte offset, uint32 value) const {
		outpd(word(io_base + offset), value);
	}

	bool LsiLogicMPI::WaitRegister(byte offset, uint32 mask, uint32 value, uint32 timeout_ms) const {
		const uint32 start = services.Milliseconds(services.context);
		// The iteration bound also terminates early-boot waits when timer interrupts are unavailable.
		for0(attempt, 10000000u) {
			const uint32 actual = ReadRegister(offset);
			if (actual == 0xFFFFFFFFu) return false;
			if ((actual & mask) == value) return true;
			if (!(attempt & 0xFFu) && uint32(services.Milliseconds(services.context) - start) >= timeout_ms) return false;
			_ASM volatile("pause" ::: "memory");
		}
		return false;
	}

	void LsiLogicMPI::DisableAfterFailure(const char* reason) {
		interrupts_enabled.store(0, MemoryOrder_Release);
		ready = false;
		faulted = true;
		WriteRegister(InterruptMask, InterruptsMasked);
		// A timed-out request may still own every DMA buffer; disable mastering and pin them until reboot.
		services.DisableDma(services.context);
		plogwarn("[%s] %s; controller disabled doorbell=%[32H]", log_name, reason, ReadRegister(Doorbell));
	}

	bool LsiLogicMPI::Reset() {
		WriteRegister(InterruptMask, InterruptsMasked);
		const uint32 doorbell = ReadRegister(Doorbell);
		if (doorbell == 0xFFFFFFFFu) return false;
		if (!(doorbell & DoorbellActive)) {
			if ((doorbell & StateMask) == StateReady) return true;
			if ((doorbell & StateMask) == StateOperational) {
				WriteRegister(Doorbell, uint32(MessageUnitReset) << 24);
				if (WaitRegister(InterruptStatus, DoorbellBusy, 0, 5000) &&
					WaitRegister(Doorbell, StateMask | DoorbellActive, StateReady, 15000)) return true;
			}
			else if (!(doorbell & StateMask) && WaitRegister(Doorbell, StateMask, StateReady, 15000)) return true;
		}
		// MPI diagnostic write sequence permits adapter reset when a BIOS handshake or IOC fault remains.
		const byte keys[] = {0x04, 0x0B, 0x02, 0x07, 0x0D};
		WriteRegister(WriteSequence, 0xFF);
		for0(i, numsof(keys)) WriteRegister(WriteSequence, keys[i]);
		if (!WaitRegister(Diagnostic, DiagnosticWritable, DiagnosticWritable, 1000)) return false;
		const uint32 diagnostic = ReadRegister(Diagnostic) & ~DiagnosticDisableArm;
		WriteRegister(Diagnostic, diagnostic | DiagnosticDisableArm);
		const uint32 start = services.Milliseconds(services.context);
		for0(attempt, 1000000u) {
			if (uint32(services.Milliseconds(services.context) - start) >= 1) break;
			_ASM volatile("pause" ::: "memory");
		}
		WriteRegister(Diagnostic, diagnostic | DiagnosticReset);
		if (!WaitRegister(Diagnostic, DiagnosticReset, 0, 15000)) return false;
		WriteRegister(WriteSequence, 0xFFFFFFFFu);
		return WaitRegister(Doorbell, StateMask | DoorbellActive, StateReady, 60000) &&
			!(ReadRegister(Diagnostic) & (DiagnosticReset | DiagnosticDisableArm | DiagnosticBadFlash));
	}

	bool LsiLogicMPI::Handshake(const void* request, stduint request_bytes, void* reply,
		stduint reply_bytes, stduint minimum_reply_bytes, uint32 timeout_ms, uint16* ioc_status) {
		if (ioc_status) *ioc_status = 0xFFFF;
		if (!request || request_bytes < sizeof(MessageRequest) || (request_bytes & 3) ||
			request_bytes / 4 > 255 || !reply || reply_bytes < minimum_reply_bytes ||
			minimum_reply_bytes < sizeof(MessageReply) || (ReadRegister(Doorbell) & DoorbellActive)) return false;
		MemSet(reply, 0, reply_bytes);
		WriteRegister(InterruptStatus, 0);
		WriteRegister(Doorbell, (uint32(HandshakeFunction) << 24) | (uint32(request_bytes / 4) << 16));
		if (!WaitRegister(InterruptStatus, DoorbellInterrupt, DoorbellInterrupt, 5000) ||
			!(ReadRegister(Doorbell) & DoorbellActive)) return false;
		WriteRegister(InterruptStatus, 0);
		if (!WaitRegister(InterruptStatus, DoorbellBusy, 0, 5000)) return false;
		const auto* input = static_cast<const byte*>(request);
		for0(i, request_bytes / 4) {
			const auto* p = input + i * 4;
			WriteRegister(Doorbell, uint32(p[0]) | (uint32(p[1]) << 8) | (uint32(p[2]) << 16) | (uint32(p[3]) << 24));
			if (!WaitRegister(InterruptStatus, DoorbellBusy, 0, 5000)) return false;
		}
		auto* output = static_cast<byte*>(reply);
		stduint words = 2;
		for (stduint i = 0; i < words; ++i) {
			if (!WaitRegister(InterruptStatus, DoorbellInterrupt, DoorbellInterrupt, i ? 5000 : timeout_ms)) return false;
			const uint32 value = ReadRegister(Doorbell);
			if (i * 2 + 1 < reply_bytes) {
				output[i * 2] = byte(value);
				output[i * 2 + 1] = byte(value >> 8);
			}
			WriteRegister(InterruptStatus, 0);
			if (i == 1) {
				words = stduint(output[2]) * 2;
				if (words * 2 < minimum_reply_bytes) return false;
			}
		}
		// The IOC raises one final interrupt after the last 16-bit reply word has been acknowledged.
		if (!WaitRegister(InterruptStatus, DoorbellInterrupt, DoorbellInterrupt, 5000)) return false;
		WriteRegister(InterruptStatus, 0);
		if (!WaitRegister(Doorbell, DoorbellActive, 0, 5000)) return false;
		MessageReply header{};
		MemCopyN(&header, reply, sizeof(header));
		uint32 context;
		MemCopyN(&context, input + 8, sizeof(context));
		if (header.function != input[3] || header.context != context) return false;
		// Config enumeration must distinguish a valid end-of-list reply from a broken handshake.
		if (ioc_status) {
			*ioc_status = header.status & IocStatusMask;
			return true;
		}
		if ((header.status & IocStatusMask) != IocSuccess) {
			plogwarn("[%s] handshake fn=%[8H] IOCStatus=%[16H] log=%[32H]",
				log_name, (unsigned)input[3], (unsigned)header.status, header.log_info);
			return false;
		}
		return true;
	}

	bool LsiLogicMPI::EnsureRegion(DmaSlot slot, DmaBuffer& region, stduint bytes) {
		DmaBuffer replacement{};
		const bool allocated = services.EnsureDma(services.context, slot, bytes, replacement);
		// A failed growth may already have released the previous allocation; always replace its metadata.
		region = replacement;
		return allocated && region.virt && region.size >= bytes && !(region.device_address & 7) &&
			uint64(region.device_address) + bytes <= 0x100000000ull;
	}

	bool LsiLogicMPI::InitializePort(byte port_type) {
		if (port_type != ScsiPort && port_type != SasPort) return false;
		if (faulted || !io_base) return false;
		if (ready) return true;
		this->port_type = port_type;
		if (!Reset()) {
			DisableAfterFailure("IOC reset failed");
			return false;
		}
		WriteRegister(InterruptMask, InterruptsMasked);
		if ((ReadRegister(InterruptMask) & InterruptsMasked) != InterruptsMasked) {
			DisableAfterFailure("cannot mask IOC interrupts for polling");
			return false;
		}
		MessageRequest request{};
		request.function = IocFacts;
		FactsReply facts{};
		if (!Handshake(&request, sizeof(request), &facts, sizeof(facts), 60, 10000)) {
			DisableAfterFailure("IOCFacts failed");
			return false;
		}
		const stduint frame_bytes = stduint(facts.request_dwords) * 4;
		if ((facts.version >> 8) != 1 || (port_type == SasPort && facts.version < 0x0105) ||
			!facts.ports || !facts.global_credits || !facts.reply_depth || !facts.block_size ||
			frame_bytes < sizeof(ScsiRequest) + sizeof(SimpleSge) || frame_bytes > 4096 || (facts.flags & 0x01)) {
			DisableAfterFailure("unsupported IOC facts or firmware upload required");
			return false;
		}
		// Firmware requiring a host-page buffer needs a separate lifetime contract; never reuse BIOS DMA addresses.
		if (facts.version >= 0x0105 && ((facts.flags & 0x04) ||
			facts.host_page_sge[0] || facts.host_page_sge[1] || facts.host_page_sge[2])) {
			DisableAfterFailure("IOC requires an unsupported host-page buffer");
			return false;
		}
		request.function = PortFacts;
		PortReply port{};
		if (!Handshake(&request, sizeof(request), &port, sizeof(port), sizeof(port), 10000) ||
			port.type != port_type || !(port.protocols & InitiatorProtocol) || !port.max_devices) {
			DisableAfterFailure("port 0 has an unsupported type or is not an initiator");
			return false;
		}
		target_count = port.max_devices;
		if (port_type == ScsiPort && target_count > LsiLogicParallel::MaxTargets) target_count = LsiLogicParallel::MaxTargets;
		initiator_id = port_type == ScsiPort && port.scsi_id < target_count ? byte(port.scsi_id) : 0xFF;
		request_frame_size = uint16(frame_bytes);
		// IOCFacts.BlockSize is in DWORDs; use its power-of-two fetch granularity (MPI Request Descriptor NB).
		request_block_bytes = 4;
		for (stduint size = facts.block_size; size > 1; size >>= 1) request_block_bytes <<= 1;
		if (sizeof(ScsiRequest) + sizeof(SimpleSge) > stduint(request_block_bytes) * 4) {
			DisableAfterFailure("IOC request fetch blocks are too small");
			return false;
		}
		// IOCFacts supplies the hardware queue bounds; request storage itself is allocated on demand.
		reply_count = facts.reply_depth;
		credits = facts.global_credits < reply_count ? facts.global_credits : reply_count;
		if (!EnsureRegion(DmaSlot::Reply, reply_region, ReplyFrameSize * reply_count)) {
			DisableAfterFailure("initial DMA allocation failed");
			return false;
		}
		MemSet(reply_region.virt, 0, ReplyFrameSize * reply_count);
		__sync_synchronize();
		for0(i, reply_count) WriteRegister(ReplyQueue, reply_region.device_address + uint32(i * ReplyFrameSize));
		InitRequest init{};
		init.who_init = HostDriver;
		init.function = IocInit;
		// SAS firmware addresses are independent of our compact host slots; zero MaxDevices means 256.
		init.max_devices = port_type == SasPort ? facts.max_devices : target_count;
		init.max_buses = port_type == SasPort && facts.max_buses ? facts.max_buses : 1;
		init.reply_frame_size = ReplyFrameSize;
		if (facts.version >= 0x0105) {
			init.version = 0x0105;
			init.header_version = 0x1300; // MPI 1.5 header unit 0x13, as defined by mpi.h.
		}
		MessageReply reply{};
		if (!Handshake(&init, sizeof(init), &reply, sizeof(reply), sizeof(reply), 10000)) {
			DisableAfterFailure("IOCInit failed");
			return false;
		}
		request.function = PortEnable;
		if (!Handshake(&request, sizeof(request), &reply, sizeof(reply), sizeof(reply), 60000) ||
			!WaitRegister(Doorbell, StateMask | DoorbellActive, StateOperational, 60000)) {
			DisableAfterFailure("PortEnable failed");
			return false;
		}
		ready = true;
		ploginfo("[%s] MPI=%[16H] port=0 targets=%u initiator=%u request=%u reply=%u credits=%u startup",
			log_name, (unsigned)facts.version, (unsigned)target_count, (unsigned)initiator_id,
			(unsigned)request_frame_size, (unsigned)reply_count, (unsigned)credits);
		return true;
	}

	bool LsiLogicParallel::Initialize() {
		return InitializePort(ScsiPort);
	}

	bool LsiLogicParallel::ExecuteScsi(stduint target_id, byte lun, const byte* cdb, byte cdb_length,
		void* data_buf, stduint data_length, bool data_in,
		byte& completion_code, byte& adapter_status, byte& target_status) {
		if (!IsTargetPresent(target_id)) {
			completion_code = byte(Completion::Failed);
			adapter_status = byte(HostStatus::Error);
			target_status = 0xFF;
			return false;
		}
		return ExecuteAddressed(0, byte(target_id), lun, cdb, cdb_length,
			data_buf, data_length, data_in, completion_code, adapter_status, target_status);
	}

	CommandHandle LsiLogicParallel::SubmitScsi(stduint target, byte lun, const byte* cdb, byte cdb_length,
		const void* data, stduint bytes, bool data_in, uint32 timeout_ms, stduint origin) {
		if (!IsTargetPresent(target)) return {};
		return SubmitAddressed(0, byte(target), lun, cdb, cdb_length, data, bytes, data_in, timeout_ms, origin);
	}

	CommandHandle LsiLogicMPI::SubmitAddressed(byte bus, byte target, byte lun, const byte* cdb, byte cdb_length,
		const void* data, stduint bytes, bool data_in, uint32 timeout_ms, stduint origin) {
		if (!IsReady() || !cdb_length || cdb_length > 16) return {};
		const stduint sges = bytes ? (bytes - 1) / SgeLengthMask + 1 : 1;
		// This first implementation has no chain SGEs; NB can describe at most four initial fetch blocks.
		if (sges > (request_frame_size - sizeof(ScsiRequest)) / sizeof(SimpleSge) ||
			sizeof(ScsiRequest) + sges * sizeof(SimpleSge) > stduint(request_block_bytes) * 4) return {};
		auto request = queue.Enqueue(bus, target, lun, cdb, cdb_length, data, bytes, data_in, request_frame_size, timeout_ms, origin);
		if (request) StartQueued();
		queue.Prune();
		return request;
	}

	void LsiLogicMPI::StartQueued() {
		queue.ExpireQueued();
		for0(i, queue.Count()) {
			auto owner = queue.getCommand(i);
			auto& command = *owner;
			if (!command.IsQueued()) continue;
			if (active_count >= credits) break;
			if (!queue.CanStart(command)) continue;
			bool collision;
			do {
				if (++next_context > ContextMask) next_context = 1;
				collision = false;
				for0(index, queue.Count()) {
					auto other = queue.getCommand(index);
					if (other->IsActive() && other->context == next_context) collision = true;
				}
			} while (collision);
			command.context = next_context;
			MemSet(command.dma.virt, 0, request_frame_size);
			MemSet(command.Sense(), 0, SenseStride);
			auto* request = reinterpret_cast<ScsiRequest*>(command.dma.virt);
			request->target = command.target;
			request->bus = command.bus;
			request->function = ScsiIo;
			request->cdb_length = command.cdb_length;
			request->sense_length = SenseStride;
			request->context = command.context;
			request->lun[1] = command.lun; // SAM peripheral-device addressing.
			request->control = (command.target_state->tagged ? 0u : ControlUntagged) |
				(command.data_length ? (command.data_in ? ControlRead : ControlWrite) : 0u);
			MemCopyN(request->cdb, command.cdb, command.cdb_length);
			request->data_length = uint32(command.data_length);
			request->sense_address = command.dma.device_address + uint32(command.sense_offset);
			auto* sge = reinterpret_cast<SimpleSge*>(command.dma.virt + sizeof(ScsiRequest));
			const stduint sge_count = command.data_length ? (command.data_length - 1) / SgeLengthMask + 1 : 1;
			stduint remaining = command.data_length, offset = 0;
			for0(i, sge_count) {
				const uint32 chunk = uint32(remaining < SgeLengthMask ? remaining : SgeLengthMask);
				sge[i].flags_length = SgeSimple | (command.data_length && !command.data_in ? SgeWrite : 0) | chunk;
				if (i + 1 == sge_count) sge[i].flags_length |= SgeLast;
				sge[i].address = command.data_length ? command.dma.device_address + uint32(command.data_offset + offset) : 0xFFFFFFFFu;
				offset += chunk;
				remaining -= chunk;
			}
			const stduint request_bytes = sizeof(ScsiRequest) + sge_count * sizeof(SimpleSge);
			const uint32 blocks = uint32((request_bytes - 1) / request_block_bytes + 1) & RequestBlockMask;
			if (!command.Start(services.Milliseconds(services.context))) continue;
			++active_count;
			__sync_synchronize();
			WriteRegister(RequestQueue, command.dma.device_address | blocks);
		}
	}

	void LsiLogicMPI::SetInterrupts(bool enabled) {
		interrupts_enabled.store(enabled && IsReady(), MemoryOrder_Release);
		// MPI HIM: leave doorbell interrupts masked; reply-message interrupts use bit 3.
		WriteRegister(InterruptMask, interrupts_enabled.load(MemoryOrder_Acquire) ? 0x01u : InterruptsMasked);
	}

	bool LsiLogicMPI::OnInterrupt() {
		if (!interrupts_enabled.load(MemoryOrder_Acquire) || !(ReadRegister(InterruptStatus) & 0x08u)) return false;
		// Mask before EOI; draining/reposting reply frames is deferred to host-serialized thread context.
		WriteRegister(InterruptMask, InterruptsMasked);
		return true;
	}

	bool LsiLogicMPI::Recover() {
		return RecoverDetailed().can_submit;
	}

	StorageRecoveryResult LsiLogicMPI::RecoverDetailed() {
		const bool restore_interrupts = interrupts_enabled.load(MemoryOrder_Acquire);
		SetInterrupts(false);
		plogwarn("[%s] resetting IOC; outstanding commands fail without replay", log_name);
		if (!Reset()) {
			DisableAfterFailure("recovery reset failed");
			queue.FailAll(false); // An unconfirmed reset cannot release device-reachable request buffers.
			return {};
		}
		active_count = 0;
		queue.FailAll(true);
		ready = false;
		faulted = false;
		if (!InitializePort(port_type) || !ValidateAfterReset()) {
			DisableAfterFailure("recovery initialization or target identity validation failed");
			return { false, true, false };
		}
		SetInterrupts(restore_interrupts);
		return { true, true, false };
	}

	void LsiLogicMPI::Service() {
		if (!IsReady()) return;
		if ((ReadRegister(Doorbell) & StateMask) != StateOperational) {
			(void)Recover();
			return;
		}
		for (;;) {
			const uint32 descriptor = ReadRegister(ReplyQueue);
			if (descriptor == EmptyReply) break;
			__sync_synchronize();
			ScsiReply reply{};
			bool address_reply = (descriptor & AddressReply) != 0;
			uint32 address = 0;
			if (address_reply) {
				address = descriptor << 1;
				const uint32 offset = address - reply_region.device_address;
				if (address < reply_region.device_address || offset >= ReplyFrameSize * reply_count || offset % ReplyFrameSize) {
					(void)Recover();
					return;
				}
				MemCopyN(&reply, reply_region.virt + offset, sizeof(reply));
				// MPI 1.2 error replies may end at ResponseInfo (32 bytes), without the later TaskTag fields.
				if (reply.length * 4 < 32 || reply.length * 4 > ReplyFrameSize || reply.function != ScsiIo) {
					(void)Recover();
					return;
				}
			}
			else reply.context = descriptor;
			CommandHandle command;
			for0(i, queue.Count()) {
				auto pending = queue.getCommand(i);
				if (pending->IsActive() && pending->context == reply.context) {
					command = pending;
					break;
				}
			}
			if (!command || (address_reply && (reply.target != command->target || reply.bus != command->bus))) {
				(void)Recover();
				return;
			}
			if (address_reply) {
				__sync_synchronize();
				WriteRegister(ReplyQueue, address);
			}
			// MPI SCSI-init context replies imply IOC success, GOOD status and a full transfer.
			else reply.transfer_count = uint32(command->data_length);
			if (reply.transfer_count > command->data_length) {
				(void)Recover();
				return;
			}
			const uint16 status = reply.status & IocStatusMask;
			const byte opcode = command->cdb[0];
			const bool short_reply = command->data_in &&
				(opcode == ScsiCmdInquiry || opcode == ScsiCmdRequestSense || opcode == ScsiCmdReportLuns);
			bool complete_report = true;
			if (command->data_in && opcode == ScsiCmdReportLuns && reply.scsi_status == byte(Status::Good)) {
				complete_report = command->data_length >= 8 && reply.transfer_count >= 8;
				if (complete_report) {
					const byte* p = command->Data();
					const uint64 needed = uint64((uint32(p[0]) << 24) | (uint32(p[1]) << 16) | (uint32(p[2]) << 8) | p[3]) + 8;
					complete_report = reply.transfer_count >= (needed < command->data_length ? needed : command->data_length);
				}
			}
			const bool good = (status == IocSuccess || status == ScsiRecoveredError || (status == ScsiDataUnderrun && short_reply)) &&
				complete_report && !(reply.scsi_state & (NoScsiStatus | Terminated)) &&
				(!(reply.scsi_state & ResponseInfoValid) || !reply.response_info) &&
				(reply.transfer_count == command->data_length || short_reply || reply.scsi_status != byte(Status::Good));
			--active_count;
			const CommandResult result{byte(Completion::Completed), byte(good ? HostStatus::Good : HostStatus::Error), reply.scsi_status};
			queue.Complete(*command, true, result, reply.transfer_count, (reply.scsi_state & AutosenseValid) ? reply.sense_count : 0);
			if ((!good || reply.scsi_status != byte(Status::Good)) && status != ScsiDeviceAbsent) {
				plogtrac("[%s] b%ut%ul%u ctx=%[32H] IOCStatus=%[16H] SCSI=%[8H] transferred=%u/%u",
					log_name, (unsigned)command->bus, (unsigned)command->target, (unsigned)command->lun,
					command->context, (unsigned)reply.status, (unsigned)reply.scsi_status,
					(unsigned)reply.transfer_count, (unsigned)command->data_length);
			}
		}
		if (queue.HasExpired(true)) {
			const uint32 now = services.Milliseconds(services.context);
			for0(i, queue.Count()) {
				auto request = queue.getCommand(i);
				if (request->IsActive() && request->HasExpired(now)) {
					plogwarn("[%s] request timeout ctx=%[32H] b%ut%ul%u origin=%u", log_name,
						request->context, (unsigned)request->bus, (unsigned)request->target, (unsigned)request->lun, request->getOrigin());
				}
			}
			(void)Recover();
			return;
		}
		queue.ExpireQueued();
		queue.Prune();
		StartQueued();
		queue.Prune();
		if (interrupts_enabled.load(MemoryOrder_Acquire)) WriteRegister(InterruptMask, 0x01u);
	}

	bool LsiLogicMPI::ExecuteAddressed(byte bus, byte target, byte lun, const byte* cdb, byte cdb_length,
		void* data, stduint bytes, bool data_in, byte& completion, byte& host, byte& target_status) {
		completion = byte(Completion::Failed);
		host = byte(HostStatus::Error);
		target_status = 0xFF;
		if (bytes && !data) return false;
		auto request = SubmitAddressed(bus, target, lun, cdb, cdb_length, data, bytes, data_in, 60000, 0);
		if (!request) return false;
		// Compatibility for hosts without a scheduler; Mecocoa uses SubmitScsi plus interrupt-driven waits.
		for0(attempt, 10000000u) {
			Service();
			if (request->IsDone()) return request->CopyResult(data, completion, host, target_status);
			_ASM volatile("pause" ::: "memory");
		}
		(void)Recover(); // Also bounds early-boot waits when the host clock cannot advance.
		return request->CopyResult(data, completion, host, target_status);
	}
}
#endif
