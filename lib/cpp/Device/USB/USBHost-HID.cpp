// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: [Device.USB] Host Human Interface Device Class Driver
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
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

#if (defined(_MCCA) && _MCCA == 0x8664) || defined(_MCU_STM32H7x)

#include "../../../../inc/cpp/Device/USB/USBHost-HID.hpp"

namespace uni::device::SpaceUSB {

	HIDBaseDriver::HIDBaseDriver(USBHostDevice* dev, int interface_index, int in_packet_size)
		: ClassDriver{ dev }, interface_index_{ interface_index },
		in_packet_size_{ in_packet_size } {
	}

	Error HIDBaseDriver::Initialize() {
		return MAKE_ERROR(Error::kNotImplemented);
	}

	Error HIDBaseDriver::SetEndpoint(const EndpointConfig& config) {
		if (config.ep_type == EndpointType::kInterrupt && config.ep_id.IsIn()) {
			ep_interrupt_in_ = config.ep_id;
		}
		else if (config.ep_type == EndpointType::kInterrupt && !config.ep_id.IsIn()) {
			ep_interrupt_out_ = config.ep_id;
		}
		return MAKE_ERROR(Error::kSuccess);
	}

	// AKA USBH_HID_SetProtocol: boot protocol keeps every report at a fixed length
	Error HIDBaseDriver::OnEndpointsConfigured() {
		SetupData setup_data{};
		setup_data.request_type.bits.direction = request_type::kOut;
		setup_data.request_type.bits.type = request_type::kClass;
		setup_data.request_type.bits.recipient = request_type::kInterface;
		setup_data.request = request::kSetProtocol;
		setup_data.value = 0;// boot protocol
		setup_data.index = interface_index_;
		setup_data.length = 0;
		initialize_phase_ = 1;
		return ParentDevice()->ControlOut(kDefaultControlPipeID, setup_data, nullptr, 0, this);
	}

	Error HIDBaseDriver::OnControlCompleted(EndpointID ep_id, SetupData setup_data,
		const void* buf, int len) {
		(void)ep_id;
		(void)setup_data;
		(void)buf;
		(void)len;
		if (initialize_phase_ == 1) {
			initialize_phase_ = 2;
			return ParentDevice()->InterruptIn(ep_interrupt_in_, buf_.data(), in_packet_size_);
		}
		return MAKE_ERROR(Error::kNotImplemented);
	}

	Error HIDBaseDriver::OnInterruptCompleted(EndpointID ep_id, const void* buf, int len) {
		(void)buf;
		if (ep_id.IsIn()) {
			OnDataReceived();
			std::copy_n(buf_.begin(), len, previous_buf_.begin());
			return ParentDevice()->InterruptIn(ep_interrupt_in_, buf_.data(), in_packet_size_);
		}
		return MAKE_ERROR(Error::kNotImplemented);
	}

}

#endif // _MCU_STM32H7x
