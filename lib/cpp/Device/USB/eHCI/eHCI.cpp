// ASCII CPP-ISO11 TAB4 LF
// Docutitle: (Device) USB eHCI Host Controller
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../../inc/cpp/Device/USB/eHCI/eHCI.hpp"

namespace uni::device::SpaceUSB2 {

	namespace {
		constexpr uint16 CapabilityLengthOffset = 0x00u;
		constexpr uint16 InterfaceVersionOffset = 0x02u;
		constexpr uint16 StructuralParametersOffset = 0x04u;
		constexpr uint16 CapabilityParametersOffset = 0x08u;

		constexpr uint16 CommandOffset = 0x00u;
		constexpr uint16 StatusOffset = 0x04u;
		constexpr uint16 InterruptOffset = 0x08u;
		constexpr uint16 FrameIndexOffset = 0x0Cu;
		constexpr uint16 SegmentSelectorOffset = 0x10u;
		constexpr uint16 PeriodicListBaseOffset = 0x14u;
		constexpr uint16 AsyncListAddressOffset = 0x18u;
		constexpr uint16 ConfigureFlagOffset = 0x40u;
		constexpr uint16 PortStatusOffset = 0x44u;

		constexpr uint32 CommandRun = 1u << 0;
		constexpr uint32 CommandReset = 1u << 1;
		constexpr uint32 CommandPeriodicEnable = 1u << 4;
		constexpr uint32 CommandAsyncEnable = 1u << 5;
		constexpr uint32 CommandInterruptThreshold = 1u << 16;

		constexpr uint32 StatusHostSystemError = 1u << 4;
		constexpr uint32 StatusHalted = 1u << 12;
		constexpr uint32 StatusPeriodicSchedule = 1u << 14;
		constexpr uint32 StatusAsyncSchedule = 1u << 15;
		constexpr uint32 StatusAcknowledgeMask = 0x3Fu;

		constexpr uint32 StructuralPortCountMask = 0x0Fu;
		constexpr uint32 StructuralPortPowerControl = 1u << 4;
		constexpr uint32 StructuralCompanionPortCountShift = 8;
		constexpr uint32 StructuralCompanionCountShift = 12;
		constexpr uint32 StructuralCompanionCountMask = 0x0Fu;
		constexpr uint32 CapabilityExtendedCapabilitiesShift = 8;
		constexpr uint32 CapabilityExtendedCapabilitiesMask = 0xFFu;
		constexpr uint32 LegacyCapabilityId = 0x01u;
		constexpr uint32 LegacyBiosOwned = 1u << 16;
		constexpr uint32 LegacyOsOwned = 1u << 24;

		constexpr uint32 PortConnected = 1u << 0;
		constexpr uint32 PortConnectChange = 1u << 1;
		constexpr uint32 PortEnabled = 1u << 2;
		constexpr uint32 PortEnableChange = 1u << 3;
		constexpr uint32 PortOvercurrent = 1u << 4;
		constexpr uint32 PortOvercurrentChange = 1u << 5;
		constexpr uint32 PortReset = 1u << 8;
		constexpr uint32 PortLineStatusShift = 10;
		constexpr uint32 PortLineStatusMask = 3u << PortLineStatusShift;
		constexpr uint32 PortPower = 1u << 12;
		constexpr uint32 PortOwner = 1u << 13;
		constexpr uint32 PortChangeMask = PortConnectChange |
			PortEnableChange | PortOvercurrentChange;

		constexpr uint32 LinkTerminate = 1u;
		constexpr uint32 LinkQueueHead = 1u << 1;
		constexpr uint32 QueueHeadDeviceAddressMask = 0x7Fu;
		constexpr uint32 QueueHeadEndpointShift = 8;
		constexpr uint32 QueueHeadSpeedHigh = 2u << 12;
		constexpr uint32 QueueHeadDataToggleControl = 1u << 14;
		constexpr uint32 QueueHeadHeadOfReclamation = 1u << 15;
		constexpr uint32 QueueHeadMaxPacketShift = 16;
		constexpr uint32 QueueHeadNakReloadShift = 28;
		constexpr uint32 QueueHeadMultOne = 1u << 30;

		constexpr uint32 TransferMissedMicroframe = 1u << 2;
		constexpr uint32 TransferTransactionError = 1u << 3;
		constexpr uint32 TransferBabble = 1u << 4;
		constexpr uint32 TransferDataBufferError = 1u << 5;
		constexpr uint32 TransferHalted = 1u << 6;
		constexpr uint32 TransferActive = 1u << 7;
		constexpr uint32 TransferPidShift = 8;
		constexpr uint32 TransferErrorRetry = 3u << 10;
		constexpr uint32 TransferInterruptOnComplete = 1u << 15;
		constexpr uint32 TransferBytesShift = 16;
		constexpr uint32 TransferBytesMask = 0x7FFFu;
		constexpr uint32 TransferDataOne = 1u << 31;
		constexpr uint32 TransferErrorMask = TransferMissedMicroframe |
			TransferTransactionError | TransferBabble |
			TransferDataBufferError | TransferHalted;
		constexpr uint32 TransferPidOut = 0u;
		constexpr uint32 TransferPidIn = 1u;
		constexpr uint32 TransferPidSetup = 2u;

		struct alignas(32) QueueHead {
			volatile uint32 horizontal_link;
			volatile uint32 endpoint_characteristics;
			volatile uint32 endpoint_capabilities;
			volatile uint32 current_qtd;
			volatile uint32 next_qtd;
			volatile uint32 alternate_next_qtd;
			volatile uint32 token;
			volatile uint32 buffer[5];
			volatile uint32 extended_buffer[5];
		};

		struct alignas(32) TransferDescriptor {
			volatile uint32 next_qtd;
			volatile uint32 alternate_next_qtd;
			volatile uint32 token;
			volatile uint32 buffer[5];
			volatile uint32 extended_buffer[5];
		};

		static_assert(sizeof(QueueHead) == 96,
			"eHCI queue heads must preserve 32-byte hardware alignment");
		static_assert(sizeof(TransferDescriptor) == 64,
			"eHCI qTDs must preserve 32-byte hardware alignment");
		static_assert(sizeof(QueueHead) * 2 + sizeof(TransferDescriptor) ==
			HostController::kBulkScheduleBytes,
			"eHCI bulk workspace overhead must match its schedule layout");
		static_assert(sizeof(SpaceUSB::SetupData) == 8,
			"USB setup packets must be 8 bytes");
		stduint AlignSchedule(stduint value) {
			return (value + 31u) & ~stduint(31u);
		}

		uint32 TransferToken(uint32 pid, bool data_one, uint16 length,
			bool interrupt_on_complete = false) {
			return TransferActive | TransferErrorRetry |
				(pid << TransferPidShift) |
				(uint32(length) << TransferBytesShift) |
				(data_one ? TransferDataOne : 0) |
				(interrupt_on_complete ? TransferInterruptOnComplete : 0);
		}

		void SetTransferBuffer(TransferDescriptor& descriptor,
			uint32 physical) {
			if (!physical) {
				for (stduint index = 0; index < 5; ++index) {
					descriptor.buffer[index] = 0;
					descriptor.extended_buffer[index] = 0;
				}
				return;
			}
			const uint32 page = physical & ~0xFFFu;
			descriptor.buffer[0] = physical;
			for (stduint index = 1; index < 5; ++index) {
				descriptor.buffer[index] = page + uint32(index * 0x1000u);
			}
			for (stduint index = 0; index < 5; ++index) {
				descriptor.extended_buffer[index] = 0;
			}
		}
	}

	void HostController::Bind(const ControllerIO& io, uint16 mmio_length) {
		io_ = io;
		periodic_list_ = nullptr;
		periodic_list_physical_ = 0;
		transfer_workspace_ = nullptr;
		transfer_workspace_physical_ = 0;
		transfer_workspace_bytes_ = 0;
		mmio_length_ = mmio_length;
		capability_length_ = 0;
		root_port_count_ = 0;
		last_transfer_status_ = 0;
		last_transfer_descriptor_index_ = 0;
		port_power_control_ = false;
		running_ = false;
	}

	bool HostController::IsBound() const {
		return io_.Read8 && io_.Read16 && io_.Read32 && io_.Write32 &&
			io_.ReadPciConfig32 && io_.WritePciConfig32 &&
			io_.DelayMilliseconds && io_.SynchronizeMemory;
	}

	uint8 HostController::Read8(uint16 offset) const {
		return io_.Read8(io_.context, offset);
	}

	uint16 HostController::Read16(uint16 offset) const {
		return io_.Read16(io_.context, offset);
	}

	uint32 HostController::Read32(uint16 offset) const {
		return io_.Read32(io_.context, offset);
	}

	void HostController::Write32(uint16 offset, uint32 value) const {
		io_.Write32(io_.context, offset, value);
	}

	void HostController::Delay(stduint milliseconds) const {
		io_.DelayMilliseconds(io_.context, milliseconds);
	}

	uint16 HostController::OperationalOffset(uint16 offset) const {
		return uint16(capability_length_ + offset);
	}

	uint16 HostController::PortOffset(uint8 port_index) const {
		return OperationalOffset(uint16(PortStatusOffset + 4u * port_index));
	}

	uint16 HostController::InterfaceVersion() const {
		return capability_length_ ? Read16(InterfaceVersionOffset) : 0;
	}

	uint32 HostController::StructuralParameters() const {
		return capability_length_ ? Read32(StructuralParametersOffset) : 0;
	}

	uint32 HostController::CapabilityParameters() const {
		return capability_length_ ? Read32(CapabilityParametersOffset) : 0;
	}

	uint32 HostController::Command() const {
		return capability_length_ ? Read32(OperationalOffset(CommandOffset)) : 0;
	}

	uint32 HostController::Status() const {
		return capability_length_ ? Read32(OperationalOffset(StatusOffset)) :
			0xFFFFFFFFu;
	}

	uint32 HostController::FrameIndex() const {
		return capability_length_ ? Read32(OperationalOffset(FrameIndexOffset)) : 0;
	}

	stduint HostController::MaximumBulkTransferBytes() const {
		if (transfer_workspace_bytes_ <= kBulkScheduleBytes) return 0;
		const stduint available = transfer_workspace_bytes_ -
			kBulkScheduleBytes;
		return available < kMaximumBulkTransferBytes ?
			available : kMaximumBulkTransferBytes;
	}

	uint8 HostController::CompanionControllerCount() const {
		return uint8((StructuralParameters() >>
			StructuralCompanionCountShift) & StructuralCompanionCountMask);
	}

	uint8 HostController::CompanionPortsPerController() const {
		return uint8((StructuralParameters() >>
			StructuralCompanionPortCountShift) & StructuralCompanionCountMask);
	}

	bool HostController::IsRunning() const {
		const uint32 status = Status();
		return running_ && !(status & (StatusHalted | StatusHostSystemError));
	}

	ControllerError HostController::TakeOwnership() {
		uint8 offset = uint8((CapabilityParameters() >>
			CapabilityExtendedCapabilitiesShift) &
			CapabilityExtendedCapabilitiesMask);
		for (stduint capability = 0; offset >= 0x40u && capability < 48;
			++capability) {
			const uint32 header = io_.ReadPciConfig32(io_.context, offset);
			const uint8 identifier = uint8(header);
			const uint8 next = uint8(header >> 8);
			if (identifier == LegacyCapabilityId) {
				io_.WritePciConfig32(io_.context, offset,
					header | LegacyOsOwned);
				for (stduint elapsed = 0; elapsed < 1000; ++elapsed) {
					const uint32 ownership = io_.ReadPciConfig32(
						io_.context, offset);
					if (!(ownership & LegacyBiosOwned)) {
						if (offset <= 0xF8u) {
							io_.WritePciConfig32(io_.context,
								uint8(offset + 4u), 0);
						}
						return ControllerError::Success;
					}
					Delay(1);
				}
				return ControllerError::OwnershipTimeout;
			}
			if (!next || next == offset) break;
			offset = next;
		}
		return ControllerError::Success;
	}

	bool HostController::ResetController() {
		Write32(OperationalOffset(InterruptOffset), 0);
		uint32 command = Command();
		command &= ~(CommandRun | CommandPeriodicEnable | CommandAsyncEnable);
		Write32(OperationalOffset(CommandOffset), command);
		for (stduint elapsed = 0; elapsed < 100; ++elapsed) {
			if (Status() & StatusHalted) break;
			Delay(1);
		}
		if (!(Status() & StatusHalted)) return false;
		Write32(OperationalOffset(CommandOffset), CommandReset);
		for (stduint elapsed = 0; elapsed < 100; ++elapsed) {
			Delay(1);
			if (!(Command() & CommandReset)) return true;
		}
		return false;
	}

	ControllerError HostController::Initialize(uint32* periodic_list,
		uint32 periodic_list_physical, void* transfer_workspace,
		uint32 transfer_workspace_physical, stduint transfer_workspace_bytes) {
		if (!IsBound() || !periodic_list || !periodic_list_physical ||
			(periodic_list_physical & 0xFFFu) ||
			(stduint(periodic_list) & 0xFFFu) || !transfer_workspace ||
			!transfer_workspace_physical ||
			(transfer_workspace_physical & 0xFFFu) ||
			(stduint(transfer_workspace) & 0xFFFu) ||
			transfer_workspace_bytes < kTransferWorkspaceBytes) {
			return ControllerError::InvalidParameter;
		}

		capability_length_ = Read8(CapabilityLengthOffset);
		if (capability_length_ < 0x10u ||
			stduint(capability_length_) + PortStatusOffset > mmio_length_) {
			capability_length_ = 0;
			return ControllerError::InvalidParameter;
		}
		const uint32 structural = StructuralParameters();
		root_port_count_ = uint8(structural & StructuralPortCountMask);
		if (!root_port_count_ || root_port_count_ > kMaximumRootPorts ||
			stduint(capability_length_) + PortStatusOffset +
				stduint(root_port_count_) * sizeof(uint32) > mmio_length_) {
			root_port_count_ = 0;
			return ControllerError::NoRootPorts;
		}
		port_power_control_ = structural & StructuralPortPowerControl;
		const auto ownership = TakeOwnership();
		if (ownership != ControllerError::Success) return ownership;
		if (!ResetController()) return ControllerError::ResetTimeout;

		periodic_list_ = periodic_list;
		periodic_list_physical_ = periodic_list_physical;
		transfer_workspace_ = static_cast<uint8*>(transfer_workspace);
		transfer_workspace_physical_ = transfer_workspace_physical;
		transfer_workspace_bytes_ = transfer_workspace_bytes;
		for (stduint index = 0; index < kPeriodicListEntries; ++index) {
			periodic_list_[index] = LinkTerminate;
		}
		for (stduint index = 0; index < transfer_workspace_bytes_; ++index) {
			transfer_workspace_[index] = 0;
		}
		auto* async_head = reinterpret_cast<QueueHead*>(transfer_workspace_);
		async_head->horizontal_link = transfer_workspace_physical_ |
			LinkQueueHead;
		async_head->endpoint_characteristics = QueueHeadSpeedHigh |
			QueueHeadDataToggleControl | QueueHeadHeadOfReclamation |
			(64u << QueueHeadMaxPacketShift) |
			(15u << QueueHeadNakReloadShift);
		async_head->endpoint_capabilities = QueueHeadMultOne;
		async_head->current_qtd = 0;
		async_head->next_qtd = LinkTerminate;
		async_head->alternate_next_qtd = LinkTerminate;
		async_head->token = TransferHalted;
		io_.SynchronizeMemory(io_.context);
		return ControllerError::Success;
	}

	ControllerError HostController::Run() {
		if (!periodic_list_ || !periodic_list_physical_ ||
			!transfer_workspace_ || !transfer_workspace_physical_ ||
			!root_port_count_) return ControllerError::InvalidParameter;
		Write32(OperationalOffset(SegmentSelectorOffset), 0);
		Write32(OperationalOffset(PeriodicListBaseOffset),
			periodic_list_physical_);
		Write32(OperationalOffset(AsyncListAddressOffset),
			transfer_workspace_physical_);
		Write32(OperationalOffset(InterruptOffset), 0);
		Write32(OperationalOffset(StatusOffset), StatusAcknowledgeMask);
		Write32(OperationalOffset(ConfigureFlagOffset), 1);
		Write32(OperationalOffset(CommandOffset), CommandRun |
			CommandPeriodicEnable | CommandAsyncEnable |
			CommandInterruptThreshold);
		for (stduint elapsed = 0; elapsed < 100; ++elapsed) {
			const uint32 status = Status();
			if (status & StatusHostSystemError) {
				return ControllerError::FatalStatus;
			}
			if (!(status & StatusHalted)) {
				const uint32 before = FrameIndex();
				Delay(5);
				const uint32 after = FrameIndex();
				if (before != after) {
					running_ = true;
					if (port_power_control_) {
						for (uint8 port = 0; port < root_port_count_; ++port) {
							WritePortControl(port, PortPower, 0);
						}
						Delay(20);
					}
					return ControllerError::Success;
				}
			}
			Delay(1);
		}
		return ControllerError::FrameNotAdvancing;
	}

	bool HostController::Stop() {
		if (!capability_length_) return true;
		uint32 command = Command();
		command &= ~(CommandRun | CommandPeriodicEnable | CommandAsyncEnable);
		Write32(OperationalOffset(CommandOffset), command);
		for (stduint elapsed = 0; elapsed < 100; ++elapsed) {
			if (Status() & StatusHalted) {
				running_ = false;
				return true;
			}
			Delay(1);
		}
		return false;
	}

	bool HostController::SetAsyncScheduleEnabled(bool enabled) {
		uint32 command = Command();
		if (enabled) command |= CommandAsyncEnable;
		else command &= ~CommandAsyncEnable;
		Write32(OperationalOffset(CommandOffset), command);
		for (stduint elapsed = 0; elapsed < 100; ++elapsed) {
			const bool active = Status() & StatusAsyncSchedule;
			if (active == enabled) return true;
			Delay(1);
		}
		return false;
	}

	ControllerError HostController::TransferStatusError(uint32 status) const {
		if (status & TransferDataBufferError) {
			return ControllerError::TransferDataBufferError;
		}
		if (status & TransferBabble) return ControllerError::TransferBabble;
		if (status & TransferTransactionError) {
			return ControllerError::TransferTransactionError;
		}
		if (status & TransferMissedMicroframe) {
			return ControllerError::TransferMissedMicroframe;
		}
		if (status & TransferHalted) return ControllerError::TransferStalled;
		return ControllerError::Success;
	}

	ControllerError HostController::ExecuteControlTransfer(
		uint8 device_address, uint8 endpoint, uint16 max_packet_size,
		const SpaceUSB::SetupData& setup, uint8* data, uint16 data_length,
		uint16& actual_length, stduint timeout_milliseconds,
		bool direction_in) {
		actual_length = 0;
		last_transfer_status_ = 0;
		last_transfer_descriptor_index_ = 0;
		if (!IsRunning() || device_address > 0x7Fu || endpoint > 0x0Fu ||
			max_packet_size != 64 || !timeout_milliseconds ||
			setup.length != data_length ||
			(data_length && !data) ||
			(bool(setup.request_type.data & 0x80u) != direction_in)) {
			return ControllerError::InvalidParameter;
		}

		const stduint qtd_count = data_length ? 3u : 2u;
		const stduint head_offset = 0;
		const stduint queue_head_offset = AlignSchedule(sizeof(QueueHead));
		const stduint qtd_offset = AlignSchedule(
			queue_head_offset + sizeof(QueueHead));
		const stduint setup_offset = AlignSchedule(
			qtd_offset + qtd_count * sizeof(TransferDescriptor));
		const stduint data_offset = AlignSchedule(
			setup_offset + sizeof(SpaceUSB::SetupData));
		const stduint required_bytes = data_offset + data_length;
		if (required_bytes > transfer_workspace_bytes_) {
			return ControllerError::TransferWorkspaceTooSmall;
		}
		for (stduint index = queue_head_offset; index < required_bytes; ++index) {
			transfer_workspace_[index] = 0;
		}

		auto* async_head = reinterpret_cast<QueueHead*>(
			transfer_workspace_ + head_offset);
		auto* queue_head = reinterpret_cast<QueueHead*>(
			transfer_workspace_ + queue_head_offset);
		auto* descriptors = reinterpret_cast<TransferDescriptor*>(
			transfer_workspace_ + qtd_offset);
		auto* setup_buffer = transfer_workspace_ + setup_offset;
		auto* data_buffer = transfer_workspace_ + data_offset;
		const auto* setup_source = reinterpret_cast<const uint8*>(&setup);
		for (stduint index = 0; index < sizeof(setup); ++index) {
			setup_buffer[index] = setup_source[index];
		}
		if (data_length && !direction_in) {
			for (uint16 index = 0; index < data_length; ++index) {
				data_buffer[index] = data[index];
			}
		}

		const uint32 queue_head_physical = transfer_workspace_physical_ +
			uint32(queue_head_offset);
		const uint32 qtd_physical = transfer_workspace_physical_ +
			uint32(qtd_offset);
		const uint32 setup_physical = transfer_workspace_physical_ +
			uint32(setup_offset);
		const uint32 data_physical = transfer_workspace_physical_ +
			uint32(data_offset);
		const stduint status_index = data_length ? 2u : 1u;
		const uint32 status_physical = qtd_physical +
			uint32(status_index * sizeof(TransferDescriptor));

		descriptors[0].next_qtd = data_length ?
			qtd_physical + sizeof(TransferDescriptor) : status_physical;
		descriptors[0].alternate_next_qtd = LinkTerminate;
		descriptors[0].token = TransferToken(TransferPidSetup, false,
			sizeof(SpaceUSB::SetupData));
		SetTransferBuffer(descriptors[0], setup_physical);

		if (data_length) {
			descriptors[1].next_qtd = status_physical;
			descriptors[1].alternate_next_qtd = status_physical;
			descriptors[1].token = TransferToken(
				direction_in ? TransferPidIn : TransferPidOut,
				true, data_length);
			SetTransferBuffer(descriptors[1], data_physical);
		}

		descriptors[status_index].next_qtd = LinkTerminate;
		descriptors[status_index].alternate_next_qtd = LinkTerminate;
		descriptors[status_index].token = TransferToken(
			direction_in ? TransferPidOut : TransferPidIn,
			true, 0, true);
		SetTransferBuffer(descriptors[status_index], 0);

		queue_head->horizontal_link = transfer_workspace_physical_ |
			LinkQueueHead;
		queue_head->endpoint_characteristics =
			(uint32(device_address) & QueueHeadDeviceAddressMask) |
			(uint32(endpoint) << QueueHeadEndpointShift) |
			QueueHeadSpeedHigh | QueueHeadDataToggleControl |
			(uint32(max_packet_size) << QueueHeadMaxPacketShift) |
			(15u << QueueHeadNakReloadShift);
		queue_head->endpoint_capabilities = QueueHeadMultOne;
		queue_head->current_qtd = 0;
		queue_head->next_qtd = qtd_physical;
		queue_head->alternate_next_qtd = LinkTerminate;
		queue_head->token = 0;

		io_.SynchronizeMemory(io_.context);
		if (!SetAsyncScheduleEnabled(false)) {
			return ControllerError::ScheduleTimeout;
		}
		async_head->horizontal_link = queue_head_physical | LinkQueueHead;
		io_.SynchronizeMemory(io_.context);
		if (!SetAsyncScheduleEnabled(true)) {
			async_head->horizontal_link = transfer_workspace_physical_ |
				LinkQueueHead;
			io_.SynchronizeMemory(io_.context);
			return ControllerError::ScheduleTimeout;
		}

		ControllerError result = ControllerError::TransferTimeout;
		for (stduint elapsed = 0; elapsed < timeout_milliseconds; ++elapsed) {
			for (stduint index = 0; index < qtd_count; ++index) {
				const uint32 status = descriptors[index].token;
				if (status & TransferErrorMask) {
					last_transfer_status_ = status;
					last_transfer_descriptor_index_ = uint8(index);
					result = TransferStatusError(status);
					goto transfer_finished;
				}
			}
			if (!(descriptors[status_index].token & TransferActive)) {
				last_transfer_status_ = descriptors[status_index].token;
				last_transfer_descriptor_index_ = uint8(status_index);
				result = ControllerError::Success;
				goto transfer_finished;
			}
			if (Status() & StatusHostSystemError) {
				result = ControllerError::FatalStatus;
				goto transfer_finished;
			}
			if (Status() & StatusHalted) {
				result = ControllerError::HaltTimeout;
				goto transfer_finished;
			}
			Delay(1);
		}
		for (stduint index = 0; index < qtd_count; ++index) {
			if (descriptors[index].token & TransferActive) {
				last_transfer_status_ = descriptors[index].token;
				last_transfer_descriptor_index_ = uint8(index);
				break;
			}
		}

	transfer_finished:
		if (!SetAsyncScheduleEnabled(false)) {
			return ControllerError::ScheduleTimeout;
		}
		async_head->horizontal_link = transfer_workspace_physical_ |
			LinkQueueHead;
		io_.SynchronizeMemory(io_.context);
		if (!SetAsyncScheduleEnabled(true)) {
			return ControllerError::ScheduleTimeout;
		}
		Write32(OperationalOffset(StatusOffset), StatusAcknowledgeMask);
		if (result != ControllerError::Success) return result;
		if (data_length) {
			const uint16 remaining = uint16(
				(descriptors[1].token >> TransferBytesShift) &
				TransferBytesMask);
			actual_length = remaining <= data_length ?
				uint16(data_length - remaining) : 0;
			if (direction_in) {
				for (uint16 index = 0; index < actual_length; ++index) {
					data[index] = data_buffer[index];
				}
			}
		}
		return ControllerError::Success;
	}

	ControllerError HostController::ControlIn(uint8 device_address,
		uint8 endpoint, uint16 max_packet_size,
		const SpaceUSB::SetupData& setup, uint8* data, uint16 data_length,
		uint16& actual_length, stduint timeout_milliseconds) {
		if (!data_length || !(setup.request_type.data & 0x80u)) {
			actual_length = 0;
			return ControllerError::InvalidParameter;
		}
		return ExecuteControlTransfer(device_address, endpoint,
			max_packet_size, setup, data, data_length, actual_length,
			timeout_milliseconds, true);
	}

	ControllerError HostController::ControlOut(uint8 device_address,
		uint8 endpoint, uint16 max_packet_size,
		const SpaceUSB::SetupData& setup, const uint8* data,
		uint16 data_length, stduint timeout_milliseconds) {
		if (!data_length || (setup.request_type.data & 0x80u)) {
			return ControllerError::InvalidParameter;
		}
		uint16 actual_length = 0;
		return ExecuteControlTransfer(device_address, endpoint,
			max_packet_size, setup, const_cast<uint8*>(data), data_length,
			actual_length, timeout_milliseconds, false);
	}

	ControllerError HostController::ControlNoData(uint8 device_address,
		uint8 endpoint, uint16 max_packet_size,
		const SpaceUSB::SetupData& setup, stduint timeout_milliseconds) {
		uint16 actual_length = 0;
		if (setup.length) return ControllerError::InvalidParameter;
		return ExecuteControlTransfer(device_address, endpoint,
			max_packet_size, setup, nullptr, 0, actual_length,
			timeout_milliseconds, false);
	}

	ControllerError HostController::BulkTransfer(uint8 device_address,
		uint8 endpoint, bool direction_in, uint16 max_packet_size,
		void* data, uint16 data_length, uint16& actual_length,
		bool& data_toggle, stduint timeout_milliseconds) {
		actual_length = 0;
		last_transfer_status_ = 0;
		last_transfer_descriptor_index_ = 0;
		if (!IsRunning() || device_address > 0x7Fu || !endpoint ||
			endpoint > 0x0Fu || !max_packet_size || max_packet_size > 512u ||
			!data || !data_length || !timeout_milliseconds) {
			return ControllerError::InvalidParameter;
		}

		const stduint head_offset = 0;
		const stduint queue_head_offset = AlignSchedule(sizeof(QueueHead));
		const stduint qtd_offset = AlignSchedule(
			queue_head_offset + sizeof(QueueHead));
		const stduint data_offset = AlignSchedule(
			qtd_offset + sizeof(TransferDescriptor));
		const stduint required_bytes = data_offset + data_length;
		if (required_bytes > transfer_workspace_bytes_) {
			return ControllerError::TransferWorkspaceTooSmall;
		}
		for (stduint index = queue_head_offset; index < required_bytes; ++index) {
			transfer_workspace_[index] = 0;
		}

		auto* async_head = reinterpret_cast<QueueHead*>(
			transfer_workspace_ + head_offset);
		auto* queue_head = reinterpret_cast<QueueHead*>(
			transfer_workspace_ + queue_head_offset);
		auto* descriptor = reinterpret_cast<TransferDescriptor*>(
			transfer_workspace_ + qtd_offset);
		auto* data_buffer = transfer_workspace_ + data_offset;
		if (!direction_in) {
			const auto* source = static_cast<const uint8*>(data);
			for (uint16 index = 0; index < data_length; ++index) {
				data_buffer[index] = source[index];
			}
		}

		const uint32 queue_head_physical = transfer_workspace_physical_ +
			uint32(queue_head_offset);
		const uint32 qtd_physical = transfer_workspace_physical_ +
			uint32(qtd_offset);
		const uint32 data_physical = transfer_workspace_physical_ +
			uint32(data_offset);

		descriptor->next_qtd = LinkTerminate;
		descriptor->alternate_next_qtd = LinkTerminate;
		descriptor->token = TransferToken(
			direction_in ? TransferPidIn : TransferPidOut,
			data_toggle, data_length, true);
		SetTransferBuffer(*descriptor, data_physical);

		queue_head->horizontal_link = transfer_workspace_physical_ |
			LinkQueueHead;
		queue_head->endpoint_characteristics =
			(uint32(device_address) & QueueHeadDeviceAddressMask) |
			(uint32(endpoint) << QueueHeadEndpointShift) |
			QueueHeadSpeedHigh | QueueHeadDataToggleControl |
			(uint32(max_packet_size) << QueueHeadMaxPacketShift) |
			(15u << QueueHeadNakReloadShift);
		queue_head->endpoint_capabilities = QueueHeadMultOne;
		queue_head->current_qtd = 0;
		queue_head->next_qtd = qtd_physical;
		queue_head->alternate_next_qtd = LinkTerminate;
		queue_head->token = 0;

		io_.SynchronizeMemory(io_.context);
		if (!SetAsyncScheduleEnabled(false)) {
			return ControllerError::ScheduleTimeout;
		}
		async_head->horizontal_link = queue_head_physical | LinkQueueHead;
		io_.SynchronizeMemory(io_.context);
		if (!SetAsyncScheduleEnabled(true)) {
			async_head->horizontal_link = transfer_workspace_physical_ |
				LinkQueueHead;
			io_.SynchronizeMemory(io_.context);
			return ControllerError::ScheduleTimeout;
		}

		ControllerError result = ControllerError::TransferTimeout;
		for (stduint elapsed = 0; elapsed < timeout_milliseconds; ++elapsed) {
			const uint32 transfer_status = descriptor->token;
			if (transfer_status & TransferErrorMask) {
				last_transfer_status_ = transfer_status;
				result = TransferStatusError(transfer_status);
				break;
			}
			if (!(transfer_status & TransferActive)) {
				last_transfer_status_ = transfer_status;
				result = ControllerError::Success;
				break;
			}
			if (Status() & StatusHostSystemError) {
				result = ControllerError::FatalStatus;
				break;
			}
			if (Status() & StatusHalted) {
				result = ControllerError::HaltTimeout;
				break;
			}
			Delay(1);
		}
		if (result == ControllerError::TransferTimeout) {
			last_transfer_status_ = descriptor->token;
		}

		if (!SetAsyncScheduleEnabled(false)) {
			return ControllerError::ScheduleTimeout;
		}
		async_head->horizontal_link = transfer_workspace_physical_ |
			LinkQueueHead;
		io_.SynchronizeMemory(io_.context);
		if (!SetAsyncScheduleEnabled(true)) {
			return ControllerError::ScheduleTimeout;
		}
		Write32(OperationalOffset(StatusOffset), StatusAcknowledgeMask);
		if (result != ControllerError::Success) return result;

		const uint16 remaining = uint16(
			(descriptor->token >> TransferBytesShift) & TransferBytesMask);
		actual_length = remaining <= data_length ?
			uint16(data_length - remaining) : 0;
		if (direction_in) {
			auto* destination = static_cast<uint8*>(data);
			for (uint16 index = 0; index < actual_length; ++index) {
				destination[index] = data_buffer[index];
			}
		}
		const stduint packet_count = actual_length ?
			(stduint(actual_length) + max_packet_size - 1u) /
				max_packet_size : 1u;
		if (packet_count & 1u) data_toggle = !data_toggle;
		return ControllerError::Success;
	}

	RootPortStatus HostController::RootPortAt(uint8 port_index) const {
		RootPortStatus result{};
		if (port_index >= root_port_count_) return result;
		result.raw = Read32(PortOffset(port_index));
		result.valid = result.raw != 0xFFFFFFFFu;
		result.connected = result.raw & PortConnected;
		result.connect_changed = result.raw & PortConnectChange;
		result.enabled = result.raw & PortEnabled;
		result.enable_changed = result.raw & PortEnableChange;
		result.overcurrent = result.raw & PortOvercurrent;
		result.overcurrent_changed = result.raw & PortOvercurrentChange;
		result.powered = !port_power_control_ || (result.raw & PortPower);
		result.owned_by_companion = result.raw & PortOwner;
		result.line_status = uint8((result.raw & PortLineStatusMask) >>
			PortLineStatusShift);
		result.high_speed = result.connected && result.enabled &&
			!result.owned_by_companion;
		return result;
	}

	void HostController::WritePortControl(uint8 port_index, uint32 set_bits,
		uint32 clear_bits, uint32 acknowledge_bits) const {
		if (port_index >= root_port_count_) return;
		uint32 value = Read32(PortOffset(port_index));
		value &= ~PortChangeMask;
		value &= ~clear_bits;
		value |= set_bits;
		value |= acknowledge_bits & PortChangeMask;
		Write32(PortOffset(port_index), value);
	}

	void HostController::AcknowledgeRootPortChanges(uint8 port_index) {
		if (port_index >= root_port_count_) return;
		const uint32 status = Read32(PortOffset(port_index));
		WritePortControl(port_index, 0, 0, status & PortChangeMask);
	}

	bool HostController::RouteRootPortToCompanion(uint8 port_index) {
		if (!IsRunning() || !CompanionControllerCount() ||
			port_index >= root_port_count_) return false;
		const auto status = RootPortAt(port_index);
		if (!status.valid || !status.connected) return false;
		if (status.owned_by_companion) return true;
		AcknowledgeRootPortChanges(port_index);
		WritePortControl(port_index, PortOwner, PortReset | PortEnabled);
		return RootPortAt(port_index).owned_by_companion;
	}

	bool HostController::ReclaimRootPortFromCompanion(uint8 port_index) {
		if (!IsRunning() || port_index >= root_port_count_) return false;
		const auto status = RootPortAt(port_index);
		if (!status.valid || status.connected || !status.owned_by_companion) {
			return false;
		}
		AcknowledgeRootPortChanges(port_index);
		WritePortControl(port_index, 0, PortOwner | PortReset | PortEnabled);
		return !RootPortAt(port_index).owned_by_companion;
	}

	bool HostController::ResetRootPort(uint8 port_index) {
		if (!IsRunning() || port_index >= root_port_count_) return false;
		auto status = RootPortAt(port_index);
		if (!status.valid || !status.connected || status.owned_by_companion) {
			return false;
		}
		AcknowledgeRootPortChanges(port_index);
		if (port_power_control_ && !status.powered) {
			WritePortControl(port_index, PortPower, 0);
			Delay(20);
			status = RootPortAt(port_index);
			if (!status.connected) return false;
		}
		WritePortControl(port_index, PortReset, PortEnabled);
		Delay(50);
		WritePortControl(port_index, 0, PortReset);
		bool reset_cleared = false;
		for (stduint elapsed = 0; elapsed < 50; ++elapsed) {
			Delay(1);
			status = RootPortAt(port_index);
			if (!(status.raw & PortReset)) {
				reset_cleared = true;
				break;
			}
		}
		if (!reset_cleared) return false;
		for (stduint elapsed = 0; elapsed < 20; ++elapsed) {
			status = RootPortAt(port_index);
			if (!status.connected || status.enabled ||
				status.owned_by_companion) break;
			Delay(1);
		}
		AcknowledgeRootPortChanges(port_index);
		status = RootPortAt(port_index);
		return status.valid && status.connected && status.high_speed;
	}

	USBHostDevice_v2::USBHostDevice_v2(HostController& host,
		uint8 assigned_address) : host_{ host } {
		SetAssignedAddress(assigned_address);
	}

	void USBHostDevice_v2::OnDeviceAddressChanged(uint8 address) {
		device_address_ = address;
	}

	const SpaceUSB::EndpointConfig* USBHostDevice_v2::EndpointConfigOf(
		SpaceUSB::EndpointID ep_id) {
		for (int index = 0; index < NumEndpointConfigs(); ++index) {
			const auto& config = EndpointConfigs()[index];
			if (config.ep_id.Address() == ep_id.Address()) return &config;
		}
		return nullptr;
	}

	Error USBHostDevice_v2::ControlIn(SpaceUSB::EndpointID ep_id,
		SpaceUSB::SetupData setup_data, void* buf, int len,
		SpaceUSB::ClassDriver* issuer) {
		if (ep_id.Number() != 0 || !buf || len <= 0 || len > 0xFFFF) {
			return MAKE_ERROR(Error::kInvalidEndpointNumber);
		}
		if (auto error = SpaceUSB::USBHostDevice::ControlIn(ep_id,
			setup_data, buf, len, issuer)) return error;
		uint16 actual_length = 0;
		const auto result = host_.ControlIn(device_address_, 0,
			control_max_packet_size_, setup_data, static_cast<uint8*>(buf),
			uint16(len), actual_length, 500);
		return OnControlCompleted(ep_id, setup_data, buf,
			result == ControllerError::Success ? int(actual_length) : -1);
	}

	Error USBHostDevice_v2::ControlOut(SpaceUSB::EndpointID ep_id,
		SpaceUSB::SetupData setup_data, const void* buf, int len,
		SpaceUSB::ClassDriver* issuer) {
		if (ep_id.Number() != 0 || len < 0 || len > 0xFFFF ||
			(len && !buf)) return MAKE_ERROR(Error::kInvalidEndpointNumber);
		if (auto error = SpaceUSB::USBHostDevice::ControlOut(ep_id,
			setup_data, buf, len, issuer)) return error;
		const auto result = len ? host_.ControlOut(device_address_, 0,
			control_max_packet_size_, setup_data,
			static_cast<const uint8*>(buf), uint16(len), 500) :
			host_.ControlNoData(device_address_, 0,
				control_max_packet_size_, setup_data, 500);
		if (result == ControllerError::Success &&
			setup_data.request == SpaceUSB::request::kClearFeature &&
			setup_data.request_type.bits.recipient ==
				SpaceUSB::request_type::kEndpoint) {
			const SpaceUSB::EndpointID endpoint{
				int(setup_data.index & 0x0Fu), bool(setup_data.index & 0x80u) };
			data_toggle_[endpoint.Address()] = false;
		}
		return OnControlCompleted(ep_id, setup_data, buf,
			result == ControllerError::Success ? len : -1);
	}

	Error USBHostDevice_v2::InterruptIn(SpaceUSB::EndpointID ep_id,
		void* buf, int len) {
		(void)ep_id;
		(void)buf;
		(void)len;
		return MAKE_ERROR(Error::kNotImplemented);
	}

	Error USBHostDevice_v2::InterruptOut(SpaceUSB::EndpointID ep_id,
		void* buf, int len) {
		(void)ep_id;
		(void)buf;
		(void)len;
		return MAKE_ERROR(Error::kNotImplemented);
	}

	Error USBHostDevice_v2::BulkTransfer(SpaceUSB::EndpointID ep_id,
		bool direction_in, void* buf, int len) {
		const auto* config = EndpointConfigOf(ep_id);
		if (!config || config->ep_type != SpaceUSB::EndpointType::kBulk ||
			ep_id.IsIn() != direction_in || !buf || len <= 0 ||
			stduint(len) > host_.MaximumBulkTransferBytes() || len > 0xFFFF) {
			return MAKE_ERROR(Error::kInvalidEndpointNumber);
		}
		uint16 actual_length = 0;
		const auto result = host_.BulkTransfer(device_address_,
			uint8(ep_id.Number()), direction_in,
			uint16(config->max_packet_size), buf, uint16(len), actual_length,
			data_toggle_[ep_id.Address()], 2000);
		return OnBulkCompleted(ep_id,
			result == ControllerError::Success ? buf : nullptr,
			result == ControllerError::Success ? int(actual_length) : 0);
	}

	Error USBHostDevice_v2::ConfigureTransportEndpoints() {
		return MAKE_ERROR(Error::kSuccess);
	}

	const char* HostController::ErrorName(ControllerError error) {
		switch (error) {
		case ControllerError::Success: return "success";
		case ControllerError::InvalidParameter: return "invalid-parameter";
		case ControllerError::OwnershipTimeout: return "ownership-timeout";
		case ControllerError::HaltTimeout: return "halt-timeout";
		case ControllerError::ResetTimeout: return "reset-timeout";
		case ControllerError::NoRootPorts: return "no-root-ports";
		case ControllerError::FatalStatus: return "fatal-status";
		case ControllerError::ScheduleTimeout: return "schedule-timeout";
		case ControllerError::FrameNotAdvancing: return "frame-not-advancing";
		case ControllerError::TransferWorkspaceTooSmall:
			return "transfer-workspace-too-small";
		case ControllerError::TransferTimeout: return "transfer-timeout";
		case ControllerError::TransferStalled: return "transfer-stalled";
		case ControllerError::TransferDataBufferError:
			return "transfer-data-buffer-error";
		case ControllerError::TransferBabble: return "transfer-babble";
		case ControllerError::TransferTransactionError:
			return "transfer-transaction-error";
		case ControllerError::TransferMissedMicroframe:
			return "transfer-missed-microframe";
		}
		return "unknown";
	}

}
