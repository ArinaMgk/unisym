// ASCII CPP-ISO11 TAB4 LF
// Docutitle: (Device) USB uHCI Host Controller
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#ifndef _INCPP_Device_USB_uHCI
#define _INCPP_Device_USB_uHCI

#include "../../../../c/stdinc.h"
#include "../USB-Header.hpp"

namespace uni::device::SpaceUSB1 {

	struct ControllerIO {
		void* context = nullptr;
		uint16 (*Read16)(void* context, uint16 offset) = nullptr;
		void (*Write8)(void* context, uint16 offset, uint8 value) = nullptr;
		void (*Write16)(void* context, uint16 offset, uint16 value) = nullptr;
		void (*Write32)(void* context, uint16 offset, uint32 value) = nullptr;
		void (*DelayMilliseconds)(void* context, stduint milliseconds) = nullptr;
		void (*SynchronizeMemory)(void* context) = nullptr;
	};

	enum class ControllerError : uint8 {
		Success,
		InvalidParameter,
		ResetTimeout,
		NoRootPorts,
		FatalStatus,
		ScheduleHalted,
		FrameNotAdvancing,
		TransferWorkspaceTooSmall,
		TransferTimeout,
		TransferStalled,
		TransferDataBufferError,
		TransferBabble,
		TransferCrcTimeout,
		TransferBitStuff,
	};

	struct RootPortStatus {
		uint16 raw = 0xFFFFu;
		bool valid = false;
		bool connected = false;
		bool connect_changed = false;
		bool enabled = false;
		bool enable_changed = false;
		bool low_speed = false;
	};

	class HostController {
	public:
		static constexpr stduint kFrameListEntries = 1024;
		static constexpr stduint kFrameListBytes = kFrameListEntries * sizeof(uint32);
		static constexpr stduint kTransferWorkspaceBytes = 0x1000;
		static constexpr uint8 kMaximumRootPorts = 7;

		HostController() = default;

		void Bind(const ControllerIO& io, uint16 io_length);
		ControllerError Initialize(uint32* frame_list, uint32 frame_list_physical);
		ControllerError Run();
		bool Stop();
		ControllerError ConfigureTransferWorkspace(void* workspace,
			uint32 workspace_physical, stduint workspace_bytes);
		ControllerError ControlIn(uint8 device_address, uint8 endpoint,
			bool low_speed, uint8 max_packet_size,
			const SpaceUSB::SetupData& setup, uint8* data, uint16 data_length,
			uint16& actual_length, stduint timeout_milliseconds);
		ControllerError ControlNoData(uint8 device_address, uint8 endpoint,
			bool low_speed, uint8 max_packet_size,
			const SpaceUSB::SetupData& setup, stduint timeout_milliseconds);
		ControllerError StartInterruptIn(uint8 device_address, uint8 endpoint,
			bool low_speed, uint16 max_packet_size, uint8 interval,
			uint16 data_length);
		ControllerError PollInterruptIn(uint8* data, uint16 data_capacity,
			uint16& actual_length, bool& completed);
		void StopInterruptIn();

		bool IsBound() const;
		bool IsRunning() const;
		uint16 Command() const;
		uint16 Status() const;
		uint16 FrameNumber() const;
		uint32 LastTransferStatus() const { return last_transfer_status_; }
		uint8 LastTransferDescriptorIndex() const {
			return last_transfer_descriptor_index_;
		}
		uint8 RootPortCount() const { return root_port_count_; }
		RootPortStatus RootPortAt(uint8 port_index) const;
		bool ResetRootPort(uint8 port_index);
		void AcknowledgeRootPortChanges(uint8 port_index);

		static const char* ErrorName(ControllerError error);

	private:
		bool ResetController();
		uint8 DetectRootPorts() const;
		uint16 Read16(uint16 offset) const;
		void Write8(uint16 offset, uint8 value) const;
		void Write16(uint16 offset, uint16 value) const;
		void Write32(uint16 offset, uint32 value) const;
		void Delay(stduint milliseconds) const;
		uint16 ReadRootPort(uint8 port_index) const;
		void WriteRootPortControl(uint8 port_index, uint16 set_bits,
			uint16 clear_bits, uint16 acknowledge_bits = 0) const;
		void DetachTransferSchedule();
		ControllerError TransferStatusError(uint32 status) const;

		ControllerIO io_{};
		uint32* frame_list_ = nullptr;
		uint32 frame_list_physical_ = 0;
		uint16 io_length_ = 0;
		uint8 root_port_count_ = 0;
		bool running_ = false;
		uint8* transfer_workspace_ = nullptr;
		uint32 transfer_workspace_physical_ = 0;
		stduint transfer_workspace_bytes_ = 0;
		uint32 last_transfer_status_ = 0;
		uint8 last_transfer_descriptor_index_ = 0;
		uint8 interrupt_in_device_address_ = 0;
		uint8 interrupt_in_endpoint_ = 0;
		uint16 interrupt_in_data_length_ = 0;
		uint32 interrupt_in_transfer_status_ = 0;
		bool interrupt_in_data_one_ = false;
		bool interrupt_in_active_ = false;
	};

}

#endif
