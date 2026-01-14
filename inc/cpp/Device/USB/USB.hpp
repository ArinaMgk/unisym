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

// x86_64 part of USB Driver is borrowed from uchan-nos/mikanos
// - https://github.com/uchan-nos/mikanos
// cited sincerely.
#if (defined(_MCCA) && ((_MCCA & 0xFF00)==0x8600))

#include "./USB-Header.hpp"
#include <algorithm>

namespace usb {
	class Device;

	class ClassDriver {
	public:
		ClassDriver(Device* dev) : dev_{ dev } {}
		// virtual ~ClassDriver();

		virtual Error Initialize() = 0;
		virtual Error SetEndpoint(const EndpointConfig& config) = 0;
		virtual Error OnEndpointsConfigured() = 0;
		virtual Error OnControlCompleted(EndpointID ep_id, SetupData setup_data, const void* buf, int len) = 0;
		virtual Error OnInterruptCompleted(EndpointID ep_id, const void* buf, int len) = 0;

		/** このクラスドライバを保持する USB デバイスを返す． */
		Device* ParentDevice() const { return dev_; }

	private:
		Device* dev_;
	};
}

namespace usb {
	class HIDBaseDriver : public ClassDriver {
	public:
		HIDBaseDriver(Device* dev, int interface_index, int in_packet_size)
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
}

#endif

#endif
