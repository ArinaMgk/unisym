// ASCII CPP-ISO11 TAB4 LF
// Docutitle: (Device) USB eHCI Host Controller
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#ifndef _INCPP_Device_USB_eHCI
#define _INCPP_Device_USB_eHCI

#include "../../../../c/stdinc.h"
#include "../USB.hpp"

namespace uni::device::SpaceUSB2 {

	struct ControllerIO {
		void* context = nullptr;
		uint8 (*Read8)(void* context, uint16 offset) = nullptr;
		uint16 (*Read16)(void* context, uint16 offset) = nullptr;
		uint32 (*Read32)(void* context, uint16 offset) = nullptr;
		void (*Write32)(void* context, uint16 offset, uint32 value) = nullptr;
		uint32 (*ReadPciConfig32)(void* context, uint8 offset) = nullptr;
		void (*WritePciConfig32)(void* context, uint8 offset, uint32 value) = nullptr;
		void (*DelayMilliseconds)(void* context, stduint milliseconds) = nullptr;
		void (*SynchronizeMemory)(void* context) = nullptr;
	};

	enum class ControllerError : uint8 {
		Success,
		InvalidParameter,
		OwnershipTimeout,
		HaltTimeout,
		ResetTimeout,
		NoRootPorts,
		FatalStatus,
		ScheduleTimeout,
		FrameNotAdvancing,
		TransferWorkspaceTooSmall,
		TransferTimeout,
		TransferStalled,
		TransferDataBufferError,
		TransferBabble,
		TransferTransactionError,
		TransferMissedMicroframe,
	};

	struct RootPortStatus {
		uint32 raw = 0xFFFFFFFFu;
		bool valid = false;
		bool connected = false;
		bool connect_changed = false;
		bool enabled = false;
		bool enable_changed = false;
		bool overcurrent = false;
		bool overcurrent_changed = false;
		bool powered = false;
		bool owned_by_companion = false;
		bool high_speed = false;
		uint8 line_status = 0;
	};

	class HostController {
	public:
		static constexpr stduint kPeriodicListEntries = 1024;
		static constexpr stduint kPeriodicListBytes =
			kPeriodicListEntries * sizeof(uint32);
		static constexpr stduint kTransferWorkspaceBytes = 0x1000;
		static constexpr stduint kBulkScheduleBytes = 0x100;
		static constexpr stduint kMaximumBulkTransferBytes =
			kTransferWorkspaceBytes - kBulkScheduleBytes;
		static constexpr uint8 kMaximumRootPorts = 15;

		HostController() = default;

		void Bind(const ControllerIO& io, uint16 mmio_length);
		ControllerError Initialize(uint32* periodic_list,
			uint32 periodic_list_physical, void* transfer_workspace,
			uint32 transfer_workspace_physical, stduint transfer_workspace_bytes);
		ControllerError Run();
		bool Stop();
		ControllerError ControlIn(uint8 device_address, uint8 endpoint,
			uint16 max_packet_size, const SpaceUSB::SetupData& setup,
			uint8* data, uint16 data_length, uint16& actual_length,
			stduint timeout_milliseconds);
		ControllerError ControlOut(uint8 device_address, uint8 endpoint,
			uint16 max_packet_size, const SpaceUSB::SetupData& setup,
			const uint8* data, uint16 data_length,
			stduint timeout_milliseconds);
		ControllerError ControlNoData(uint8 device_address, uint8 endpoint,
			uint16 max_packet_size, const SpaceUSB::SetupData& setup,
			stduint timeout_milliseconds);
		ControllerError BulkTransfer(uint8 device_address, uint8 endpoint,
			bool direction_in, uint16 max_packet_size, void* data,
			uint16 data_length, uint16& actual_length, bool& data_toggle,
			stduint timeout_milliseconds);

		bool IsBound() const;
		bool IsRunning() const;
		uint8 CapabilityLength() const { return capability_length_; }
		uint16 InterfaceVersion() const;
		uint32 StructuralParameters() const;
		uint32 CapabilityParameters() const;
		uint32 Command() const;
		uint32 Status() const;
		uint32 FrameIndex() const;
		stduint MaximumBulkTransferBytes() const;
		uint8 CompanionControllerCount() const;
		uint8 CompanionPortsPerController() const;
		uint32 LastTransferStatus() const { return last_transfer_status_; }
		uint8 LastTransferDescriptorIndex() const {
			return last_transfer_descriptor_index_;
		}
		uint8 RootPortCount() const { return root_port_count_; }
		RootPortStatus RootPortAt(uint8 port_index) const;
		bool ResetRootPort(uint8 port_index);
		bool RouteRootPortToCompanion(uint8 port_index);
		bool ReclaimRootPortFromCompanion(uint8 port_index);
		void AcknowledgeRootPortChanges(uint8 port_index);

		static const char* ErrorName(ControllerError error);

	private:
		ControllerError TakeOwnership();
		bool ResetController();
		bool SetAsyncScheduleEnabled(bool enabled);
		ControllerError ExecuteControlTransfer(uint8 device_address,
			uint8 endpoint, uint16 max_packet_size,
			const SpaceUSB::SetupData& setup, uint8* data,
			uint16 data_length, uint16& actual_length,
			stduint timeout_milliseconds, bool direction_in);
		ControllerError TransferStatusError(uint32 status) const;
		uint8 Read8(uint16 offset) const;
		uint16 Read16(uint16 offset) const;
		uint32 Read32(uint16 offset) const;
		void Write32(uint16 offset, uint32 value) const;
		void Delay(stduint milliseconds) const;
		uint16 OperationalOffset(uint16 offset) const;
		uint16 PortOffset(uint8 port_index) const;
		void WritePortControl(uint8 port_index, uint32 set_bits,
			uint32 clear_bits, uint32 acknowledge_bits = 0) const;

		ControllerIO io_{};
		uint32* periodic_list_ = nullptr;
		uint32 periodic_list_physical_ = 0;
		uint8* transfer_workspace_ = nullptr;
		uint32 transfer_workspace_physical_ = 0;
		stduint transfer_workspace_bytes_ = 0;
		uint16 mmio_length_ = 0;
		uint8 capability_length_ = 0;
		uint8 root_port_count_ = 0;
		uint32 last_transfer_status_ = 0;
		uint8 last_transfer_descriptor_index_ = 0;
		bool port_power_control_ = false;
		bool running_ = false;
	};

	class USBHostDevice_v2 : public SpaceUSB::USBHostDevice {
	public:
		USBHostDevice_v2(HostController& host,
			uint8 assigned_address = SpaceUSB::kDefaultDeviceAddress);
		stduint Speed() const override { return 0; }
		bool RequiresSetAddressRequest() const override { return true; }
		void OnDeviceAddressChanged(uint8 address) override;
		Error ControlIn(SpaceUSB::EndpointID ep_id,
			SpaceUSB::SetupData setup_data, void* buf, int len,
			SpaceUSB::ClassDriver* issuer) override;
		Error ControlOut(SpaceUSB::EndpointID ep_id,
			SpaceUSB::SetupData setup_data, const void* buf, int len,
			SpaceUSB::ClassDriver* issuer) override;
		Error InterruptIn(SpaceUSB::EndpointID ep_id,
			void* buf, int len) override;
		Error InterruptOut(SpaceUSB::EndpointID ep_id,
			void* buf, int len) override;
		Error BulkTransfer(SpaceUSB::EndpointID ep_id,
			bool direction_in, void* buf, int len) override;
		Error ConfigureTransportEndpoints() override;
		HostController& Controller() { return host_; }
		uint8 DeviceAddress() const { return device_address_; }

	private:
		const SpaceUSB::EndpointConfig* EndpointConfigOf(
			SpaceUSB::EndpointID ep_id);
		HostController& host_;
		uint8 device_address_ = 0;
		uint16 control_max_packet_size_ = 64;
		bool data_toggle_[32]{};
	};

}

#endif
