// UTF-8 CPP-ISO11 TAB4 CRLF
// Docutitle: (Device) USB
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
// Copyright: uchan-nos/mikanos, under Apache License 2.0
/*
	Copyright 2023 ArinaMgk

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0
	http://unisym.org/license.html

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/

/*
┌─────────────────────────────┐
│  Applications / OS Services │
├─────────────────────────────┤
│  USB Class Drivers          │  ← Device-specific functionality
├─────────────────────────────┤
│  USB Core Framework         │  ← Protocol abstraction layer
├─────────────────────────────┤
│  xHCI Driver                │  ← USB 3.0+ Host Controller Driver
├─────────────────────────────┤
│  PCI/PCIe Driver            │  ← Bus enumeration and communication (inc\cpp\Device\Bus\PCI.hpp)
├─────────────────────────────┤
│  xHCI Hardware Controller   │  ← Physical hardware
└─────────────────────────────┘
*/

#ifndef _INCPP_Device_USB
#define _INCPP_Device_USB

// x86_64 part of USB&xHCI Drivers are borrowed from uchan-nos/mikanos
// - https://github.com/uchan-nos/mikanos
// cited sincerely.
#if (defined(_MCCA) && ((_MCCA & 0xFF00)==0x8600))

#include "./USB-Header.hpp"
#include <algorithm>

#include "./USB-Device.hpp"

namespace uni::device::SpaceUSB {
	class DeviceUSB;
	using HubDescriptorCompleteHook = void (*)(DeviceUSB& dev);
	extern HubDescriptorCompleteHook g_hub_descriptor_complete_hook;
	using HubPortStatusHook = void (*)(DeviceUSB& dev, uint8 downstream_port, uint16 status, uint16 change);
	extern HubPortStatusHook g_hub_port_status_hook;

	class ClassDriver {
	public:
		ClassDriver(DeviceUSB* dev) : dev_{ dev } {}
		// virtual ~ClassDriver();

		virtual Error Initialize() = 0;
		virtual Error SetEndpoint(const EndpointConfig& config) = 0;
		virtual Error OnEndpointsConfigured() = 0;
		virtual Error OnControlCompleted(EndpointID ep_id, SetupData setup_data, const void* buf, int len) = 0;
		virtual Error OnInterruptCompleted(EndpointID ep_id, const void* buf, int len) = 0;

		/** このクラスドライバを保持する USB デバイスを返す． */
		DeviceUSB* ParentDevice() const { return dev_; }

	private:
		DeviceUSB* dev_;
	};

	class HIDBaseDriver : public ClassDriver {
	public:
		HIDBaseDriver(DeviceUSB* dev, int interface_index, int in_packet_size)
			: ClassDriver{ dev }, interface_index_{ interface_index },
			in_packet_size_{ in_packet_size }
		{ }
		Error Initialize() override { return MAKE_ERROR(Error::kNotImplemented); }
		Error SetEndpoint(const EndpointConfig& config) override {
			if (config.ep_type == EndpointType::kInterrupt && config.ep_id.IsIn()) {
				ep_interrupt_in_ = config.ep_id;
			}
			else if (config.ep_type == EndpointType::kInterrupt && !config.ep_id.IsIn()) {
				ep_interrupt_out_ = config.ep_id;
			}
			return MAKE_ERROR(Error::kSuccess);
		}
		Error OnEndpointsConfigured() override {
			SetupData setup_data{};
			setup_data.request_type.bits.direction = request_type::kOut;
			setup_data.request_type.bits.type = request_type::kClass;
			setup_data.request_type.bits.recipient = request_type::kInterface;
			setup_data.request = request::kSetProtocol;
			setup_data.value = 0; // boot protocol
			setup_data.index = interface_index_;
			setup_data.length = 0;
			initialize_phase_ = 1;
			return ParentDevice()->ControlOut(kDefaultControlPipeID, setup_data, nullptr, 0, this);
		}
		Error OnControlCompleted(EndpointID ep_id, SetupData setup_data, const void* buf, int len) override {
			// Log(kDebug, "HIDBaseDriver::OnControlCompleted: dev %[8H], phase = %d, len = %d", this, initialize_phase_, len);
			if (initialize_phase_ == 1) {
				initialize_phase_ = 2;
				return ParentDevice()->InterruptIn(ep_interrupt_in_, buf_.data(), in_packet_size_);
			}
			return MAKE_ERROR(Error::kNotImplemented);
		}
		Error OnInterruptCompleted(EndpointID ep_id, const void* buf, int len) override {
			if (ep_id.IsIn()) {
				OnDataReceived();
				std::copy_n(buf_.begin(), len, previous_buf_.begin());
				return ParentDevice()->InterruptIn(ep_interrupt_in_, buf_.data(), in_packet_size_);
			}
			return MAKE_ERROR(Error::kNotImplemented);
		}

		virtual Error OnDataReceived() = 0;
		const static size_t kBufferSize = 1024;
		const std::array<uint8_t, kBufferSize>& Buffer() const { return buf_; }
		const std::array<uint8_t, kBufferSize>& PreviousBuffer() const { return previous_buf_; }

	private:
		EndpointID ep_interrupt_in_;
		EndpointID ep_interrupt_out_;
		const int interface_index_;
		int in_packet_size_;
		int initialize_phase_{ 0 };

		std::array<uint8_t, kBufferSize> buf_{}, previous_buf_{};
	};

	class USBHubDriver : public ClassDriver {
	public:
		explicit USBHubDriver(DeviceUSB* dev)
			: ClassDriver{ dev } {}

		void* operator new(size_t size) {
			(void)size;
			return uni_hostenv_allocator->allocate(sizeof(USBHubDriver));
		}
		void operator delete(void* ptr) noexcept {
			uni_hostenv_allocator->deallocate(ptr);
		}

		Error Initialize() override { return MAKE_ERROR(Error::kSuccess); }
		Error SetEndpoint(const EndpointConfig& config) override {
			if (config.ep_type == EndpointType::kInterrupt && config.ep_id.IsIn()) {
				ep_interrupt_in_ = config.ep_id;
			}
			return MAKE_ERROR(Error::kSuccess);
		}
		Error OnEndpointsConfigured() override {
			SetupData setup_data{};
			setup_data.request_type.bits.direction = request_type::kIn;
			setup_data.request_type.bits.type = request_type::kClass;
			setup_data.request_type.bits.recipient = request_type::kDevice;
			setup_data.request = request::kGetDescriptor;
			setup_data.value = static_cast<uint16>(descriptor_type::kHub) << 8;
			setup_data.index = 0;
			setup_data.length = sizeof(HubDescriptor);
			initialize_phase_ = 1;
			return ParentDevice()->ControlIn(kDefaultControlPipeID, setup_data,
				buf_.data(), sizeof(HubDescriptor), this);
		}
		Error StartStatusChangePolling() {
			if (ep_interrupt_in_.Number() == 0) {
				return MAKE_ERROR(Error::kSuccess);
			}
			const stduint bytes = status_change_bytes_ ? status_change_bytes_ : 1;
			return ParentDevice()->InterruptIn(ep_interrupt_in_, interrupt_buf_.data(), int(bytes));
		}
		Error RequestPortStatus(uint8 port_num) {
			SetupData setup_data{};
			setup_data.request_type.bits.direction = request_type::kIn;
			setup_data.request_type.bits.type = request_type::kClass;
			setup_data.request_type.bits.recipient = request_type::kOther;
			setup_data.request = request::kGetStatus;
			setup_data.value = 0;
			setup_data.index = port_num;
			setup_data.length = sizeof(HubPortStatus);
			return ParentDevice()->ControlIn(kDefaultControlPipeID, setup_data,
				port_status_buf_.data(), sizeof(HubPortStatus), this);
		}
		Error RequestClearPortFeature(uint8 port_num, uint16 feature_selector) {
			SetupData setup_data{};
			setup_data.request_type.bits.direction = request_type::kOut;
			setup_data.request_type.bits.type = request_type::kClass;
			setup_data.request_type.bits.recipient = request_type::kOther;
			setup_data.request = request::kClearFeature;
			setup_data.value = feature_selector;
			setup_data.index = port_num;
			setup_data.length = 0;
			return ParentDevice()->ControlOut(kDefaultControlPipeID, setup_data, nullptr, 0, this);
		}
		Error ContinuePendingPortStatus() {
			if (pending_status_index_ < pending_status_count_) {
				const auto next_port = pending_status_ports_[pending_status_index_++];
				initialize_phase_ = 4;
				return RequestPortStatus(next_port);
			}
			initialize_phase_ = 3;
			return StartStatusChangePolling();
		}
		Error ClearNextPendingChange() {
			while (clear_change_bit_ < 5) {
				const uint16 bit_mask = uint16(1u << clear_change_bit_);
				if ((pending_change_mask_ & bit_mask) != 0) {
					const auto feature_selector = uint16(16u + clear_change_bit_);
					++clear_change_bit_;
					initialize_phase_ = 5;
					return RequestClearPortFeature(clear_change_port_, feature_selector);
				}
				++clear_change_bit_;
			}
			pending_change_mask_ = 0;
			clear_change_port_ = 0;
			clear_change_bit_ = 0;
			return ContinuePendingPortStatus();
		}
		Error OnControlCompleted(EndpointID ep_id, SetupData setup_data, const void* buf, int len) override {
			(void)ep_id;
			(void)len;
			if (initialize_phase_ == 1) {
				auto hub_desc = DescriptorDynamicCast<HubDescriptor>(reinterpret_cast<const uint8*>(buf));
				if (hub_desc == nullptr) {
					return MAKE_ERROR(Error::kInvalidDescriptor);
				}
				num_ports_ = hub_desc->num_ports;
				ParentDevice()->SetHubNumPorts(num_ports_);
				if (g_hub_descriptor_complete_hook) {
					g_hub_descriptor_complete_hook(*ParentDevice());
				}
				if (num_ports_ == 0) {
					initialize_phase_ = 3;
					return StartStatusChangePolling();
				}
				status_change_bytes_ = uint8((num_ports_ + 8) / 8);
				scan_port_ = 1;
				initialize_phase_ = 2;
				return RequestPortStatus(scan_port_);
			}
			if (initialize_phase_ == 2) {
				auto port_status = reinterpret_cast<const HubPortStatus*>(buf);
				if (setup_data.request == request::kGetStatus &&
					setup_data.request_type.bits.recipient == request_type::kOther) {
					ParentDevice()->OnHubPortStatusReceived(uint8(setup_data.index), port_status->status, port_status->change);
					if (g_hub_port_status_hook) {
						g_hub_port_status_hook(*ParentDevice(), uint8(setup_data.index), port_status->status, port_status->change);
					}
				}
				pending_change_mask_ = uint16(port_status->change & 0x001fu);
				clear_change_port_ = uint8(setup_data.index);
				clear_change_bit_ = 0;
				if (pending_change_mask_ != 0) {
					return ClearNextPendingChange();
				}
				if (scan_port_ < num_ports_) {
					++scan_port_;
					return RequestPortStatus(scan_port_);
				}
				initialize_phase_ = 3;
				return StartStatusChangePolling();
			}
			if (initialize_phase_ == 4) {
				auto port_status = reinterpret_cast<const HubPortStatus*>(buf);
				if (setup_data.request == request::kGetStatus &&
					setup_data.request_type.bits.recipient == request_type::kOther) {
					ParentDevice()->OnHubPortStatusReceived(uint8(setup_data.index), port_status->status, port_status->change);
					if (g_hub_port_status_hook) {
						g_hub_port_status_hook(*ParentDevice(), uint8(setup_data.index), port_status->status, port_status->change);
					}
				}
				pending_change_mask_ = uint16(port_status->change & 0x001fu);
				clear_change_port_ = uint8(setup_data.index);
				clear_change_bit_ = 0;
				if (pending_change_mask_ != 0) {
					return ClearNextPendingChange();
				}
				return ContinuePendingPortStatus();
			}
			if (initialize_phase_ == 5) {
				if (setup_data.request == request::kClearFeature &&
					setup_data.request_type.bits.recipient == request_type::kOther) {
					return ClearNextPendingChange();
				}
				return MAKE_ERROR(Error::kInvalidPhase);
			}
			return MAKE_ERROR(Error::kNotImplemented);
		}
		Error OnInterruptCompleted(EndpointID ep_id, const void* buf, int len) override {
			(void)ep_id;
			if (initialize_phase_ != 3) {
				return MAKE_ERROR(Error::kSuccess);
			}
			pending_status_count_ = 0;
			pending_status_index_ = 0;
			const auto* bits = reinterpret_cast<const uint8_t*>(buf);
			const stduint bytes = stduint(len);
			for (uint8 port = 1; port <= num_ports_; ++port) {
				const stduint bit_index = port;
				const stduint byte_index = bit_index / 8;
				const uint8 bit_mask = uint8(1u << (bit_index % 8));
				if (byte_index < bytes && (bits[byte_index] & bit_mask) != 0) {
					if (pending_status_count_ < pending_status_ports_.size()) {
						pending_status_ports_[pending_status_count_++] = port;
					}
				}
			}
			if (pending_status_count_ == 0) {
				return StartStatusChangePolling();
			}
			return ContinuePendingPortStatus();
		}

		uint8 NumPorts() const { return num_ports_; }

	private:
		EndpointID ep_interrupt_in_{};
		int initialize_phase_ = 0;
		uint8 num_ports_ = 0;
		uint8 scan_port_ = 0;
		uint8 status_change_bytes_ = 1;
		uint8 pending_status_count_ = 0;
		uint8 pending_status_index_ = 0;
		uint8 clear_change_port_ = 0;
		uint8 clear_change_bit_ = 0;
		uint16 pending_change_mask_ = 0;
		std::array<uint8_t, sizeof(HubDescriptor)> buf_{};
		std::array<uint8_t, sizeof(HubPortStatus)> port_status_buf_{};
		std::array<uint8_t, 8> interrupt_buf_{};
		std::array<uint8_t, 32> pending_status_ports_{};
	};
}

#endif

#endif
