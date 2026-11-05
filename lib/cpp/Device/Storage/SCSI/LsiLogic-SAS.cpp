// ASCII CPP-ISO11 TAB4 LF
// Docutitle: LSI Logic SAS1068 controller (Fusion-MPT MPI 1.5)
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../../inc/cpp/Device/Storage/SCSI/LsiLogic-SAS.hpp"
#if defined(_MCCA) && ((_MCCA & 0xFF00) == 0x8600)

namespace {
	// MPI 1.5 mpi_cnfg.h / mpi_sas.h: extended SAS Device Page 0 and GET_NEXT_HANDLE addressing.
	enum : byte {
		Config = 0x04, SasPort = 0x30, PageHeaderAction = 0x00, ReadCurrentAction = 0x01,
		ExtendedPage = 0x0F, SasDevicePage = 0x12, EndDevice = 0x01, DeviceTypeMask = 0x07,
	};
	enum : uint16 {
		ConfigInvalidPage = 0x0022,
		DevicePresent = 0x0001, DeviceMapped = 0x0002, UnsupportedDevice = 0x0100,
		NoHandle = 0xFFFF,
	};
	enum : uint32 {
		SspTarget = 0x00000400, StpTarget = 0x00000200, SataDevice = 0x00000080,
		GetNextHandle = 0x00000000, SimpleReadSge = 0xD1000000,
		MaxConfigBytes = 4096,
	};
	_PACKED(struct) PageHeader {
		byte version, length, number, type;
	};
	_PACKED(struct) ExtendedHeader {
		PageHeader header;
		uint16 words;
		byte type, reserved;
	};
	_PACKED(struct) ConfigRequest {
		byte action, reserved1, chain_offset, function;
		uint16 extended_words;
		byte extended_type, flags;
		uint32 context;
		byte reserved12[8];
		PageHeader header;
		uint32 address;
		uint32 sge_flags_length, sge_address, sge_reserved;
	};
	_PACKED(struct) ConfigReply {
		byte action, reserved1, length, function;
		uint16 extended_words;
		byte extended_type, flags;
		uint32 context;
		uint16 reserved12, status;
		uint32 log_info;
		PageHeader header;
	};
	_PACKED(struct) SasDevicePage0 {
		ExtendedHeader header;
		uint16 slot, enclosure_handle;
		uint64 sas_address;
		uint16 parent_handle;
		byte phy, access_status;
		uint16 handle;
		byte target_id, bus;
		uint32 device_info;
		uint16 flags;
		byte physical_port, reserved35;
	};
	static_assert(sizeof(ConfigRequest) == 40 && sizeof(ConfigReply) == 24 && sizeof(SasDevicePage0) == 36);
	static_assert(__builtin_offsetof(ConfigRequest, header) == 0x14 && __builtin_offsetof(ConfigRequest, sge_flags_length) == 0x1C);
	static_assert(__builtin_offsetof(SasDevicePage0, handle) == 0x18 && __builtin_offsetof(SasDevicePage0, device_info) == 0x1C);
}

namespace uni::SCSI {
	bool LsiLogicSAS::ReadNextTarget(uint16 previous_handle, Target& target, bool& found) {
		found = false;
		ConfigRequest request{};
		request.function = Config;
		request.action = PageHeaderAction;
		request.extended_type = SasDevicePage;
		request.header.type = ExtendedPage;
		request.address = GetNextHandle | previous_handle;
		request.sge_flags_length = SimpleReadSge;
		request.sge_address = 0xFFFFFFFFu;
		ConfigReply reply{};
		uint16 status;
		if (!Handshake(&request, sizeof(request), &reply, sizeof(reply), sizeof(reply), 10000, &status)) return false;
		if (status == ConfigInvalidPage) return true; // GET_NEXT_HANDLE has reached the end of the firmware table.
		if (status || reply.action != request.action || reply.extended_type != SasDevicePage ||
			(reply.header.type & ExtendedPage) != ExtendedPage || reply.header.number != 0) {
			plogwarn("[LSI-SAS] SAS device header failed IOCStatus=%[16H]", (unsigned)status);
			return false;
		}
		const stduint bytes = stduint(reply.extended_words) * 4;
		if (bytes < sizeof(SasDevicePage0) || bytes > MaxConfigBytes || !EnsureRegion(DmaSlot::Data, data_region, bytes)) return false;
		request.action = ReadCurrentAction;
		request.header = reply.header;
		request.header.type &= ExtendedPage;
		request.extended_words = reply.extended_words;
		request.sge_flags_length = SimpleReadSge | uint32(bytes);
		request.sge_address = data_region.device_address;
		MemSet(data_region.virt, 0, bytes);
		__sync_synchronize();
		// Startup is serialized; doorbell config replies cannot be confused with SCSI completions in the reply FIFO.
		if (!Handshake(&request, sizeof(request), &reply, sizeof(reply), sizeof(reply), 10000, &status)) return false;
		__sync_synchronize();
		// MPI PAGE_HEADER describes the layout; GET_NEXT_HANDLE may reach the end only on READ_CURRENT.
		if (status == ConfigInvalidPage) return true;
		if (status || reply.action != ReadCurrentAction || reply.extended_type != SasDevicePage ||
			(reply.header.type & ExtendedPage) != ExtendedPage || reply.header.number != 0 ||
			stduint(reply.extended_words) * 4 < sizeof(SasDevicePage0) || stduint(reply.extended_words) * 4 > bytes) {
			plogwarn("[LSI-SAS] SAS device page failed IOCStatus=%[16H]", (unsigned)status);
			return false;
		}
		SasDevicePage0 page{};
		MemCopyN(&page, data_region.virt, sizeof(page));
		if (page.header.header.number != 0 || (page.header.header.type & ExtendedPage) != ExtendedPage ||
			page.header.type != SasDevicePage || stduint(page.header.words) * 4 < sizeof(page) ||
			stduint(page.header.words) * 4 > bytes || page.handle == NoHandle || page.handle == previous_handle) return false;
		target.sas_address = page.sas_address;
		target.device_info = page.device_info;
		target.handle = page.handle;
		target.flags = page.flags;
		target.bus = page.bus;
		target.id = page.target_id;
		target.access_status = page.access_status;
		found = true;
		return true;
	}

	bool LsiLogicSAS::DiscoverTargets() {
		uint16 previous_handle = NoHandle;
		targets.Clear();
		// Handles are 16-bit; the bound also rejects cyclic or otherwise malformed firmware enumeration.
		for0(attempt, 0x10000u) {
			Target target{};
			bool found;
			if (!ReadNextTarget(previous_handle, target, found)) return false;
			if (!found) return true;
			previous_handle = target.handle;
			if ((target.device_info & DeviceTypeMask) != EndDevice ||
				!(target.device_info & (SspTarget | StpTarget | SataDevice))) continue;
			if ((target.flags & (DevicePresent | DeviceMapped)) != (DevicePresent | DeviceMapped) ||
				(target.flags & UnsupportedDevice) || target.access_status) {
				plogwarn("[LSI-SAS] skip handle=%[16H] bus=%u target=%u flags=%[16H] access=%[8H]",
					(unsigned)target.handle, (unsigned)target.bus, (unsigned)target.id,
					(unsigned)target.flags, (unsigned)target.access_status);
				continue;
			}
			bool duplicate = false;
			for (const auto& existing : targets) {
				if (existing->handle == target.handle) return false;
				if (existing->bus == target.bus && existing->id == target.id) duplicate = true;
			}
			if (duplicate) continue;
			auto owner = uni::make_shared<Target>();
			if (!owner) return false;
			*owner = target;
			const stduint slot = targets.Count();
			if (!targets.Append(owner)) return false;
			ploginfo("[LSI-SAS] slot=%u bus=%u target=%u handle=%[16H] SAS=%[64H] type=%s",
				(unsigned)slot, (unsigned)target.bus, (unsigned)target.id, (unsigned)target.handle,
				target.sas_address, (target.device_info & SataDevice) ? "SATA" : "SAS");
		}
		return false;
	}

	bool LsiLogicSAS::Initialize() {
		if (IsReady()) return true;
		if (!InitializePort(SasPort)) return false;
		// No hotplug rescans: published slots stay stable, and recovery separately validates the existing identities.
		if (!DiscoverTargets()) {
			DisableAfterFailure("initial SAS discovery failed");
			return false;
		}
		discovery_complete = true;
		ploginfo("[LSI-SAS] startup discovery complete: targets=%u", (unsigned)targets.Count());
		return true;
	}

	bool LsiLogicSAS::ExecuteScsi(stduint target_id, byte lun, const byte* cdb, byte cdb_length,
		void* data_buf, stduint data_length, bool data_in,
		byte& completion_code, byte& adapter_status, byte& target_status) {
		if (!IsTargetPresent(target_id)) {
			completion_code = byte(Completion::Failed);
			adapter_status = byte(HostStatus::Error);
			target_status = 0xFF;
			return false;
		}
		const auto& target = *targets[target_id];
		// SAS and firmware-translated SATA disks use the same MPI SCSI I/O request; no AHCI/ATA path is involved.
		return ExecuteAddressed(target.bus, target.id, lun, cdb, cdb_length,
			data_buf, data_length, data_in, completion_code, adapter_status, target_status);
	}

	CommandHandle LsiLogicSAS::SubmitScsi(stduint target_id, byte lun, const byte* cdb, byte cdb_length,
		const void* data, stduint bytes, bool data_in, uint32 timeout_ms, stduint origin) {
		if (!IsTargetPresent(target_id)) return {};
		const auto& target = *targets[target_id];
		return SubmitAddressed(target.bus, target.id, lun, cdb, cdb_length, data, bytes, data_in, timeout_ms, origin);
	}

	bool LsiLogicSAS::ValidateAfterReset() {
		Vector<SharedPtr<Target>> previous(targets);
		if (previous.Count() != targets.Count() || !DiscoverTargets() || targets.Count() != previous.Count()) return false;
		for (const auto& old : previous) {
			bool found = false;
			for (const auto& current : targets) {
				if (old->sas_address == current->sas_address && old->bus == current->bus && old->id == current->id) found = true;
			}
			if (!found) return false;
		}
		// Keep published logical slots stable; never redirect a mounted disk after a reset.
		targets = previous;
		return true;
	}
}
#endif
