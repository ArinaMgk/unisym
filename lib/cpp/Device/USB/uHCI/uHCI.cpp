// ASCII CPP-ISO11 TAB4 LF
// Docutitle: (Device) USB uHCI Host Controller
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../../inc/cpp/Device/USB/uHCI/uHCI.hpp"

namespace uni::device::SpaceUSB1 {

	namespace {
		constexpr uint16 CommandOffset = 0x00u;
		constexpr uint16 StatusOffset = 0x02u;
		constexpr uint16 InterruptOffset = 0x04u;
		constexpr uint16 FrameNumberOffset = 0x06u;
		constexpr uint16 FrameListBaseOffset = 0x08u;
		constexpr uint16 SofModifyOffset = 0x0Cu;
		constexpr uint16 PortStatusOffset = 0x10u;

		constexpr uint16 CommandRun = 1u << 0;
		constexpr uint16 CommandHostReset = 1u << 1;
		constexpr uint16 CommandConfigure = 1u << 6;
		constexpr uint16 CommandMaxPacket64 = 1u << 7;

		constexpr uint16 StatusUsbInterrupt = 1u << 0;
		constexpr uint16 StatusUsbErrorInterrupt = 1u << 1;
		constexpr uint16 StatusResumeDetect = 1u << 2;
		constexpr uint16 StatusHostSystemError = 1u << 3;
		constexpr uint16 StatusHostProcessError = 1u << 4;
		constexpr uint16 StatusHalted = 1u << 5;
		constexpr uint16 StatusAcknowledgeMask = StatusUsbInterrupt |
			StatusUsbErrorInterrupt | StatusResumeDetect |
			StatusHostSystemError | StatusHostProcessError;
		constexpr uint16 StatusFatalMask = StatusHostSystemError |
			StatusHostProcessError;

		constexpr uint16 PortConnected = 1u << 0;
		constexpr uint16 PortConnectChange = 1u << 1;
		constexpr uint16 PortEnabled = 1u << 2;
		constexpr uint16 PortEnableChange = 1u << 3;
		constexpr uint16 PortAlwaysOne = 1u << 7;
		constexpr uint16 PortLowSpeed = 1u << 8;
		constexpr uint16 PortReset = 1u << 9;
		constexpr uint16 PortSuspend = 1u << 12;
		constexpr uint16 PortChangeMask = PortConnectChange | PortEnableChange;

		constexpr uint32 FrameListEntryTerminate = 1u;
		constexpr uint32 LinkTerminate = 1u;
		constexpr uint32 LinkQueueHead = 1u << 1;
		constexpr uint32 TransferActive = 1u << 23;
		constexpr uint32 TransferLowSpeed = 1u << 26;
		constexpr uint32 TransferErrorRetry = 3u << 27;
		constexpr uint32 TransferShortPacketDetect = 1u << 29;
		constexpr uint32 TransferStalled = 1u << 22;
		constexpr uint32 TransferDataBufferError = 1u << 21;
		constexpr uint32 TransferBabble = 1u << 20;
		constexpr uint32 TransferCrcTimeout = 1u << 18;
		constexpr uint32 TransferBitStuff = 1u << 17;
		constexpr uint32 TransferActualLengthMask = 0x07FFu;
		constexpr uint32 TokenDeviceAddressShift = 8;
		constexpr uint32 TokenEndpointShift = 15;
		constexpr uint32 TokenToggle = 1u << 19;
		constexpr uint32 TokenExpectedLengthShift = 21;
		constexpr uint32 TokenExpectedLengthMask = 0x07FFu;
		constexpr uint8 PacketSetup = 0x2Du;
		constexpr uint8 PacketIn = 0x69u;
		constexpr uint8 PacketOut = 0xE1u;

		struct alignas(16) QueueHead {
			volatile uint32 horizontal_link;
			volatile uint32 element_link;
			uint32 reserved[2];
		};

		struct alignas(16) TransferDescriptor {
			volatile uint32 link;
			volatile uint32 status;
			uint32 token;
			uint32 buffer;
		};

		static_assert(sizeof(QueueHead) == 16 && sizeof(TransferDescriptor) == 16,
			"uHCI hardware schedule entries must be 16 bytes");
		static_assert(sizeof(SpaceUSB::SetupData) == 8,
			"USB setup packets must be 8 bytes");

		stduint AlignSchedule(stduint value) {
			return (value + 15u) & ~stduint(15u);
		}

		uint32 ExpectedLength(uint16 length) {
			return (uint32(length - 1u) & TokenExpectedLengthMask) <<
				TokenExpectedLengthShift;
		}

		uint32 TransferToken(uint8 packet_id, uint8 device_address,
			uint8 endpoint, bool data_one, uint16 length) {
			return uint32(packet_id) |
				(uint32(device_address) << TokenDeviceAddressShift) |
				(uint32(endpoint) << TokenEndpointShift) |
				(data_one ? TokenToggle : 0) | ExpectedLength(length);
		}
	}

	void HostController::Bind(const ControllerIO& io, uint16 io_length) {
		io_ = io;
		io_length_ = io_length;
		frame_list_ = nullptr;
		frame_list_physical_ = 0;
		root_port_count_ = 0;
		running_ = false;
		transfer_workspace_ = nullptr;
		transfer_workspace_physical_ = 0;
		transfer_workspace_bytes_ = 0;
		last_transfer_status_ = 0;
		last_transfer_descriptor_index_ = 0;
		interrupt_in_device_address_ = 0;
		interrupt_in_endpoint_ = 0;
		interrupt_in_data_length_ = 0;
		interrupt_in_transfer_status_ = 0;
		interrupt_in_data_one_ = false;
		interrupt_in_active_ = false;
	}

	bool HostController::IsBound() const {
		return io_.Read16 && io_.Write8 && io_.Write16 && io_.Write32 &&
			io_.DelayMilliseconds && io_.SynchronizeMemory;
	}

	uint16 HostController::Read16(uint16 offset) const {
		return io_.Read16(io_.context, offset);
	}

	void HostController::Write8(uint16 offset, uint8 value) const {
		io_.Write8(io_.context, offset, value);
	}

	void HostController::Write16(uint16 offset, uint16 value) const {
		io_.Write16(io_.context, offset, value);
	}

	void HostController::Write32(uint16 offset, uint32 value) const {
		io_.Write32(io_.context, offset, value);
	}

	void HostController::Delay(stduint milliseconds) const {
		io_.DelayMilliseconds(io_.context, milliseconds);
	}

	bool HostController::ResetController() {
		Write16(InterruptOffset, 0);
		Write16(CommandOffset, 0);
		Write16(StatusOffset, StatusAcknowledgeMask);
		Write16(CommandOffset, CommandHostReset);
		for (stduint i = 0; i < 100; ++i) {
			Delay(1);
			if (!(Read16(CommandOffset) & CommandHostReset)) return true;
		}
		return false;
	}

	uint8 HostController::DetectRootPorts() const {
		uint8 count = 0;
		for (uint8 index = 0; index < kMaximumRootPorts; ++index) {
			const uint16 offset = uint16(PortStatusOffset + 2u * index);
			if (stduint(offset) + sizeof(uint16) > io_length_) break;
			const uint16 status = Read16(offset);
			if (status == 0xFFFFu || !(status & PortAlwaysOne)) break;
			++count;
		}
		return count;
	}

	ControllerError HostController::Initialize(uint32* frame_list,
		uint32 frame_list_physical) {
		if (!IsBound() || !frame_list || !frame_list_physical ||
			(frame_list_physical & 0xFFFu) ||
			(stduint(frame_list) & (alignof(uint32) - 1))) {
			return ControllerError::InvalidParameter;
		}
		frame_list_ = frame_list;
		frame_list_physical_ = frame_list_physical;
		for (stduint i = 0; i < kFrameListEntries; ++i) {
			frame_list_[i] = FrameListEntryTerminate;
		}
		io_.SynchronizeMemory(io_.context);
		if (!ResetController()) return ControllerError::ResetTimeout;
		root_port_count_ = DetectRootPorts();
		if (!root_port_count_) return ControllerError::NoRootPorts;
		return ControllerError::Success;
	}

	ControllerError HostController::Run() {
		if (!frame_list_ || !frame_list_physical_ || !root_port_count_) {
			return ControllerError::InvalidParameter;
		}
		Write16(FrameNumberOffset, 0);
		Write32(FrameListBaseOffset, frame_list_physical_);
		Write8(SofModifyOffset, 0x40u);
		Write16(StatusOffset, StatusAcknowledgeMask);
		Write16(CommandOffset,
			CommandRun | CommandConfigure | CommandMaxPacket64);
		for (stduint i = 0; i < 20; ++i) {
			Delay(1);
			const uint16 status = Status();
			if (status & StatusFatalMask) return ControllerError::FatalStatus;
			if (!(status & StatusHalted)) {
				const uint16 before = FrameNumber();
				Delay(5);
				const uint16 after = FrameNumber();
				if (before != after) {
					const uint16 current = Status();
					if (current & StatusFatalMask) return ControllerError::FatalStatus;
					if (current & StatusHalted) return ControllerError::ScheduleHalted;
					running_ = true;
					return ControllerError::Success;
				}
			}
		}
		return (Status() & StatusHalted) ?
			ControllerError::ScheduleHalted : ControllerError::FrameNotAdvancing;
	}

	ControllerError HostController::ConfigureTransferWorkspace(void* workspace,
		uint32 workspace_physical, stduint workspace_bytes) {
		if (!workspace || !workspace_physical || workspace_bytes < 64 ||
			(stduint(workspace) & 0x0Fu) || (workspace_physical & 0x0Fu) ||
			workspace_bytes - 1u > 0xFFFFFFFFu - workspace_physical) {
			return ControllerError::InvalidParameter;
		}
		transfer_workspace_ = static_cast<uint8*>(workspace);
		transfer_workspace_physical_ = workspace_physical;
		transfer_workspace_bytes_ = workspace_bytes;
		interrupt_in_active_ = false;
		return ControllerError::Success;
	}

	ControllerError HostController::TransferStatusError(uint32 status) const {
		if (status & TransferStalled) return ControllerError::TransferStalled;
		if (status & TransferDataBufferError) {
			return ControllerError::TransferDataBufferError;
		}
		if (status & TransferBabble) return ControllerError::TransferBabble;
		if (status & TransferCrcTimeout) return ControllerError::TransferCrcTimeout;
		if (status & TransferBitStuff) return ControllerError::TransferBitStuff;
		return ControllerError::Success;
	}

	void HostController::DetachTransferSchedule() {
		for (stduint i = 0; i < kFrameListEntries; ++i) {
			frame_list_[i] = FrameListEntryTerminate;
		}
		io_.SynchronizeMemory(io_.context);
		Delay(2);
		Write16(StatusOffset, StatusAcknowledgeMask);
	}

	ControllerError HostController::ControlIn(uint8 device_address,
		uint8 endpoint, bool low_speed, uint8 max_packet_size,
		const SpaceUSB::SetupData& setup, uint8* data, uint16 data_length,
		uint16& actual_length, stduint timeout_milliseconds) {
		actual_length = 0;
		last_transfer_status_ = 0;
		last_transfer_descriptor_index_ = 0;
		if (!IsRunning() || !transfer_workspace_ || interrupt_in_active_ ||
			!data || !data_length ||
			device_address > 0x7Fu || endpoint > 0x0Fu ||
			setup.length != data_length || !(setup.request_type.data & 0x80u) ||
			!timeout_milliseconds ||
			(max_packet_size != 8 && max_packet_size != 16 &&
			 max_packet_size != 32 && max_packet_size != 64) ||
			(low_speed && max_packet_size != 8)) {
			return ControllerError::InvalidParameter;
		}

		const stduint data_td_count =
			(data_length + max_packet_size - 1u) / max_packet_size;
		const stduint td_count = data_td_count + 2u;
		const stduint qh_offset = 0;
		const stduint td_offset = AlignSchedule(sizeof(QueueHead));
		const stduint setup_offset = AlignSchedule(
			td_offset + td_count * sizeof(TransferDescriptor));
		const stduint data_offset = AlignSchedule(setup_offset + sizeof(setup));
		const stduint required_bytes = data_offset + data_length;
		if (required_bytes > transfer_workspace_bytes_) {
			return ControllerError::TransferWorkspaceTooSmall;
		}

		for (stduint i = 0; i < required_bytes; ++i) {
			transfer_workspace_[i] = 0;
		}
		auto* queue_head = reinterpret_cast<QueueHead*>(
			transfer_workspace_ + qh_offset);
		auto* descriptors = reinterpret_cast<TransferDescriptor*>(
			transfer_workspace_ + td_offset);
		auto* setup_buffer = transfer_workspace_ + setup_offset;
		auto* data_buffer = transfer_workspace_ + data_offset;
		const auto* setup_source = reinterpret_cast<const uint8*>(&setup);
		for (stduint i = 0; i < sizeof(setup); ++i) {
			setup_buffer[i] = setup_source[i];
		}

		const uint32 descriptor_physical = transfer_workspace_physical_ +
			uint32(td_offset);
		const uint32 setup_physical = transfer_workspace_physical_ +
			uint32(setup_offset);
		const uint32 data_physical = transfer_workspace_physical_ +
			uint32(data_offset);
		const uint32 transfer_status = TransferActive | TransferErrorRetry |
			(low_speed ? TransferLowSpeed : 0);

		queue_head->horizontal_link = LinkTerminate;
		queue_head->element_link = descriptor_physical;
		queue_head->reserved[0] = 0;
		queue_head->reserved[1] = 0;

		descriptors[0].link = descriptor_physical + sizeof(TransferDescriptor);
		descriptors[0].status = transfer_status;
		descriptors[0].token = TransferToken(PacketSetup, device_address,
			endpoint, false, sizeof(setup));
		descriptors[0].buffer = setup_physical;

		uint16 remaining = data_length;
		uint16 data_position = 0;
		bool data_one = true;
		for (stduint i = 0; i < data_td_count; ++i) {
			const stduint descriptor_index = i + 1u;
			const uint16 packet_length = remaining < max_packet_size ?
				remaining : max_packet_size;
			descriptors[descriptor_index].link = descriptor_physical +
				uint32((descriptor_index + 1u) * sizeof(TransferDescriptor));
			descriptors[descriptor_index].status = transfer_status |
				(i + 1u < data_td_count ? TransferShortPacketDetect : 0);
			descriptors[descriptor_index].token = TransferToken(PacketIn,
				device_address, endpoint, data_one, packet_length);
			descriptors[descriptor_index].buffer = data_physical + data_position;
			data_position += packet_length;
			remaining -= packet_length;
			data_one = !data_one;
		}

		const stduint status_index = td_count - 1u;
		descriptors[status_index].link = LinkTerminate;
		descriptors[status_index].status = transfer_status;
		descriptors[status_index].token = TransferToken(PacketOut,
			device_address, endpoint, true, 0);
		descriptors[status_index].buffer = 0;

		io_.SynchronizeMemory(io_.context);
		const uint32 queue_head_link = transfer_workspace_physical_ |
			LinkQueueHead;
		for (stduint i = 0; i < kFrameListEntries; ++i) {
			frame_list_[i] = queue_head_link;
		}
		io_.SynchronizeMemory(io_.context);

		ControllerError result = ControllerError::TransferTimeout;
		for (stduint elapsed = 0; elapsed < timeout_milliseconds; ++elapsed) {
			for (stduint i = 0; i < td_count; ++i) {
				const uint32 status = descriptors[i].status;
				const auto error = TransferStatusError(status);
				if (error != ControllerError::Success) {
					last_transfer_status_ = status;
					last_transfer_descriptor_index_ = uint8(i);
					result = error;
					goto transfer_finished;
				}
			}
			if (!(descriptors[status_index].status & TransferActive)) {
				last_transfer_status_ = descriptors[status_index].status;
				last_transfer_descriptor_index_ = uint8(status_index);
				result = ControllerError::Success;
				goto transfer_finished;
			}
			if (!IsRunning()) {
				result = Status() & StatusFatalMask ?
					ControllerError::FatalStatus : ControllerError::ScheduleHalted;
				goto transfer_finished;
			}
			Delay(1);
		}
		for (stduint i = 0; i < td_count; ++i) {
			if (descriptors[i].status & TransferActive) {
				last_transfer_status_ = descriptors[i].status;
				last_transfer_descriptor_index_ = uint8(i);
				break;
			}
		}

	transfer_finished:
		DetachTransferSchedule();
		if (result != ControllerError::Success) return result;
		for (stduint i = 0; i < data_td_count; ++i) {
			const uint32 status = descriptors[i + 1u].status;
			actual_length += uint16((status + 1u) & TransferActualLengthMask);
		}
		if (actual_length > data_length) actual_length = data_length;
		for (uint16 i = 0; i < actual_length; ++i) {
			data[i] = data_buffer[i];
		}
		return ControllerError::Success;
	}

	ControllerError HostController::ControlNoData(uint8 device_address,
		uint8 endpoint, bool low_speed, uint8 max_packet_size,
		const SpaceUSB::SetupData& setup, stduint timeout_milliseconds) {
		last_transfer_status_ = 0;
		last_transfer_descriptor_index_ = 0;
		if (!IsRunning() || !transfer_workspace_ || interrupt_in_active_ ||
			device_address > 0x7Fu || endpoint > 0x0Fu || setup.length ||
			(setup.request_type.data & 0x80u) || !timeout_milliseconds ||
			(max_packet_size != 8 && max_packet_size != 16 &&
			 max_packet_size != 32 && max_packet_size != 64) ||
			(low_speed && max_packet_size != 8)) {
			return ControllerError::InvalidParameter;
		}

		constexpr stduint td_count = 2;
		const stduint qh_offset = 0;
		const stduint td_offset = AlignSchedule(sizeof(QueueHead));
		const stduint setup_offset = AlignSchedule(
			td_offset + td_count * sizeof(TransferDescriptor));
		const stduint required_bytes = setup_offset + sizeof(setup);
		if (required_bytes > transfer_workspace_bytes_) {
			return ControllerError::TransferWorkspaceTooSmall;
		}

		for (stduint i = 0; i < required_bytes; ++i) {
			transfer_workspace_[i] = 0;
		}
		auto* queue_head = reinterpret_cast<QueueHead*>(
			transfer_workspace_ + qh_offset);
		auto* descriptors = reinterpret_cast<TransferDescriptor*>(
			transfer_workspace_ + td_offset);
		auto* setup_buffer = transfer_workspace_ + setup_offset;
		const auto* setup_source = reinterpret_cast<const uint8*>(&setup);
		for (stduint i = 0; i < sizeof(setup); ++i) {
			setup_buffer[i] = setup_source[i];
		}

		const uint32 descriptor_physical = transfer_workspace_physical_ +
			uint32(td_offset);
		const uint32 setup_physical = transfer_workspace_physical_ +
			uint32(setup_offset);
		const uint32 transfer_status = TransferActive | TransferErrorRetry |
			(low_speed ? TransferLowSpeed : 0);

		queue_head->horizontal_link = LinkTerminate;
		queue_head->element_link = descriptor_physical;
		queue_head->reserved[0] = 0;
		queue_head->reserved[1] = 0;

		descriptors[0].link = descriptor_physical + sizeof(TransferDescriptor);
		descriptors[0].status = transfer_status;
		descriptors[0].token = TransferToken(PacketSetup, device_address,
			endpoint, false, sizeof(setup));
		descriptors[0].buffer = setup_physical;

		descriptors[1].link = LinkTerminate;
		descriptors[1].status = transfer_status;
		descriptors[1].token = TransferToken(PacketIn, device_address,
			endpoint, true, 0);
		descriptors[1].buffer = 0;

		io_.SynchronizeMemory(io_.context);
		const uint32 queue_head_link = transfer_workspace_physical_ |
			LinkQueueHead;
		for (stduint i = 0; i < kFrameListEntries; ++i) {
			frame_list_[i] = queue_head_link;
		}
		io_.SynchronizeMemory(io_.context);

		ControllerError result = ControllerError::TransferTimeout;
		for (stduint elapsed = 0; elapsed < timeout_milliseconds; ++elapsed) {
			for (stduint i = 0; i < td_count; ++i) {
				const uint32 status = descriptors[i].status;
				const auto error = TransferStatusError(status);
				if (error != ControllerError::Success) {
					last_transfer_status_ = status;
					last_transfer_descriptor_index_ = uint8(i);
					result = error;
					goto transfer_finished;
				}
			}
			if (!(descriptors[1].status & TransferActive)) {
				last_transfer_status_ = descriptors[1].status;
				last_transfer_descriptor_index_ = 1;
				result = ControllerError::Success;
				goto transfer_finished;
			}
			if (!IsRunning()) {
				result = Status() & StatusFatalMask ?
					ControllerError::FatalStatus : ControllerError::ScheduleHalted;
				goto transfer_finished;
			}
			Delay(1);
		}
		for (stduint i = 0; i < td_count; ++i) {
			if (descriptors[i].status & TransferActive) {
				last_transfer_status_ = descriptors[i].status;
				last_transfer_descriptor_index_ = uint8(i);
				break;
			}
		}

	transfer_finished:
		DetachTransferSchedule();
		return result;
	}

	ControllerError HostController::StartInterruptIn(uint8 device_address,
		uint8 endpoint, bool low_speed, uint16 max_packet_size,
		uint8 interval, uint16 data_length) {
		last_transfer_status_ = 0;
		last_transfer_descriptor_index_ = 0;
		if (!IsRunning() || !transfer_workspace_ || interrupt_in_active_ ||
			!device_address || device_address > 0x7Fu ||
			!endpoint || endpoint > 0x0Fu || !interval || !data_length ||
			max_packet_size > 64 || data_length > max_packet_size ||
			(low_speed && max_packet_size > 8)) {
			return ControllerError::InvalidParameter;
		}

		const stduint qh_offset = 0;
		const stduint td_offset = AlignSchedule(sizeof(QueueHead));
		const stduint data_offset = AlignSchedule(
			td_offset + sizeof(TransferDescriptor));
		const stduint required_bytes = data_offset + data_length;
		if (required_bytes > transfer_workspace_bytes_) {
			return ControllerError::TransferWorkspaceTooSmall;
		}

		for (stduint i = 0; i < required_bytes; ++i) {
			transfer_workspace_[i] = 0;
		}
		auto* queue_head = reinterpret_cast<QueueHead*>(
			transfer_workspace_ + qh_offset);
		auto* descriptor = reinterpret_cast<TransferDescriptor*>(
			transfer_workspace_ + td_offset);
		const uint32 descriptor_physical = transfer_workspace_physical_ +
			uint32(td_offset);
		interrupt_in_device_address_ = device_address;
		interrupt_in_endpoint_ = endpoint;
		interrupt_in_data_length_ = data_length;
		interrupt_in_transfer_status_ = TransferActive | TransferErrorRetry |
			TransferShortPacketDetect | (low_speed ? TransferLowSpeed : 0);
		interrupt_in_data_one_ = false;

		queue_head->horizontal_link = LinkTerminate;
		queue_head->element_link = descriptor_physical;
		queue_head->reserved[0] = 0;
		queue_head->reserved[1] = 0;
		descriptor->link = LinkTerminate;
		descriptor->status = interrupt_in_transfer_status_;
		descriptor->token = TransferToken(PacketIn, device_address, endpoint,
			interrupt_in_data_one_, data_length);
		descriptor->buffer = transfer_workspace_physical_ + uint32(data_offset);

		uint16 schedule_period = 1;
		while (uint16(schedule_period << 1) <= interval) {
			schedule_period = uint16(schedule_period << 1);
		}
		const uint32 queue_head_link = transfer_workspace_physical_ |
			LinkQueueHead;
		for (stduint i = 0; i < kFrameListEntries; ++i) {
			frame_list_[i] = (i % schedule_period) ?
				FrameListEntryTerminate : queue_head_link;
		}
		io_.SynchronizeMemory(io_.context);
		interrupt_in_active_ = true;
		return ControllerError::Success;
	}

	ControllerError HostController::PollInterruptIn(uint8* data,
		uint16 data_capacity, uint16& actual_length, bool& completed) {
		actual_length = 0;
		completed = false;
		if (!interrupt_in_active_ || !data ||
			data_capacity < interrupt_in_data_length_) {
			return ControllerError::InvalidParameter;
		}
		if (!IsRunning()) {
			return Status() & StatusFatalMask ?
				ControllerError::FatalStatus : ControllerError::ScheduleHalted;
		}

		const stduint td_offset = AlignSchedule(sizeof(QueueHead));
		const stduint data_offset = AlignSchedule(
			td_offset + sizeof(TransferDescriptor));
		auto* queue_head = reinterpret_cast<QueueHead*>(transfer_workspace_);
		auto* descriptor = reinterpret_cast<TransferDescriptor*>(
			transfer_workspace_ + td_offset);
		io_.SynchronizeMemory(io_.context);
		const uint32 status = descriptor->status;
		if (status & TransferActive) return ControllerError::Success;
		const auto error = TransferStatusError(status);
		if (error != ControllerError::Success) {
			last_transfer_status_ = status;
			last_transfer_descriptor_index_ = 0;
			return error;
		}

		actual_length = uint16((status + 1u) & TransferActualLengthMask);
		if (actual_length > interrupt_in_data_length_) {
			actual_length = interrupt_in_data_length_;
		}
		for (uint16 i = 0; i < actual_length; ++i) {
			data[i] = transfer_workspace_[data_offset + i];
		}
		completed = true;
		last_transfer_status_ = status;
		last_transfer_descriptor_index_ = 0;
		interrupt_in_data_one_ = !interrupt_in_data_one_;
		descriptor->link = LinkTerminate;
		descriptor->status = interrupt_in_transfer_status_;
		descriptor->token = TransferToken(PacketIn,
			interrupt_in_device_address_, interrupt_in_endpoint_,
			interrupt_in_data_one_, interrupt_in_data_length_);
		descriptor->buffer = transfer_workspace_physical_ + uint32(data_offset);
		io_.SynchronizeMemory(io_.context);
		queue_head->element_link = transfer_workspace_physical_ + uint32(td_offset);
		io_.SynchronizeMemory(io_.context);
		return ControllerError::Success;
	}

	void HostController::StopInterruptIn() {
		if (!interrupt_in_active_) return;
		interrupt_in_active_ = false;
		DetachTransferSchedule();
	}

	bool HostController::Stop() {
		if (!IsBound()) return true;
		Write16(InterruptOffset, 0);
		Write16(CommandOffset, 0);
		for (stduint i = 0; i < 100; ++i) {
			if (Status() & StatusHalted) break;
			Delay(1);
		}
		const bool halted = Status() & StatusHalted;
		Write16(StatusOffset, StatusAcknowledgeMask);
		running_ = false;
		interrupt_in_active_ = false;
		return halted;
	}

	bool HostController::IsRunning() const {
		return running_ && !(Status() &
			(StatusFatalMask | StatusHalted));
	}

	uint16 HostController::Command() const {
		return IsBound() ? Read16(CommandOffset) : 0xFFFFu;
	}

	uint16 HostController::Status() const {
		return IsBound() ? Read16(StatusOffset) : 0xFFFFu;
	}

	uint16 HostController::FrameNumber() const {
		return IsBound() ? uint16(Read16(FrameNumberOffset) & 0x07FFu) : 0;
	}

	uint16 HostController::ReadRootPort(uint8 port_index) const {
		if (port_index >= root_port_count_) return 0xFFFFu;
		return Read16(uint16(PortStatusOffset + 2u * port_index));
	}

	RootPortStatus HostController::RootPortAt(uint8 port_index) const {
		RootPortStatus result{};
		result.raw = ReadRootPort(port_index);
		result.valid = port_index < root_port_count_ && result.raw != 0xFFFFu &&
			(result.raw & PortAlwaysOne);
		if (!result.valid) return result;
		result.connected = result.raw & PortConnected;
		result.connect_changed = result.raw & PortConnectChange;
		result.enabled = result.raw & PortEnabled;
		result.enable_changed = result.raw & PortEnableChange;
		result.low_speed = result.raw & PortLowSpeed;
		return result;
	}

	void HostController::WriteRootPortControl(uint8 port_index, uint16 set_bits,
		uint16 clear_bits, uint16 acknowledge_bits) const {
		if (port_index >= root_port_count_) return;
		uint16 value = ReadRootPort(port_index);
		value &= uint16(~PortChangeMask);
		value &= uint16(~clear_bits);
		value |= uint16(set_bits | (acknowledge_bits & PortChangeMask));
		Write16(uint16(PortStatusOffset + 2u * port_index), value);
	}

	bool HostController::ResetRootPort(uint8 port_index) {
		if (port_index >= root_port_count_) return false;
		uint16 status = ReadRootPort(port_index);
		if (!(status & PortConnected)) return true;
		WriteRootPortControl(port_index, PortReset,
			PortEnabled | PortSuspend, PortChangeMask);
		Delay(50);
		WriteRootPortControl(port_index, 0, PortReset, PortChangeMask);
		Delay(10);
		for (stduint i = 0; i < 10; ++i) {
			status = ReadRootPort(port_index);
			if (!(status & PortConnected) || (status & PortEnabled)) break;
			WriteRootPortControl(port_index, PortEnabled, 0, PortChangeMask);
			Delay(1);
		}
		AcknowledgeRootPortChanges(port_index);
		status = ReadRootPort(port_index);
		return (status & (PortConnected | PortEnabled)) ==
			(PortConnected | PortEnabled);
	}

	void HostController::AcknowledgeRootPortChanges(uint8 port_index) {
		if (port_index >= root_port_count_) return;
		const uint16 status = ReadRootPort(port_index);
		WriteRootPortControl(port_index, 0, 0, status & PortChangeMask);
	}

	const char* HostController::ErrorName(ControllerError error) {
		switch (error) {
		case ControllerError::Success: return "success";
		case ControllerError::InvalidParameter: return "invalid parameter";
		case ControllerError::ResetTimeout: return "reset timeout";
		case ControllerError::NoRootPorts: return "no root ports";
		case ControllerError::FatalStatus: return "fatal controller status";
		case ControllerError::ScheduleHalted: return "schedule halted";
		case ControllerError::FrameNotAdvancing: return "frame number not advancing";
		case ControllerError::TransferWorkspaceTooSmall: return "transfer workspace too small";
		case ControllerError::TransferTimeout: return "transfer timeout";
		case ControllerError::TransferStalled: return "transfer stalled";
		case ControllerError::TransferDataBufferError: return "transfer data-buffer error";
		case ControllerError::TransferBabble: return "transfer babble";
		case ControllerError::TransferCrcTimeout: return "transfer CRC/timeout";
		case ControllerError::TransferBitStuff: return "transfer bit-stuff error";
		default: return "unknown";
		}
	}

}
