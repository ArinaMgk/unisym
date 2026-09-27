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

#include "../../../../inc/cpp/Device/USB/USBHost-HID.hpp"
#include "../../../../inc/cpp/ISO_IEC_STD/algorithm"

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
			// AKA USBH_HID_SetProtocol done: arm the poll, then sync the device LED state
			initialize_phase_ = 2;
			Error armed = ParentDevice()->InterruptIn(ep_interrupt_in_, buf_.data(), in_packet_size_);
			if (HasLedReport()) SendLed();
			return armed;
		}
		if (initialize_phase_ == 2) {
			// the output report completed (or the device refused it): release the pipe
			led_pending_ = false;
			if (led_sent_ != led_) return SendLed();// a newer state waited for this completion
			return MAKE_ERROR(Error::kSuccess);
		}
		return MAKE_ERROR(Error::kNotImplemented);
	}

	// AKA USBH_HID_SetReport: the LED state is remembered; the pipe sends one report at a time
	Error HIDBaseDriver::SetLed(byte leds) {
		led_ = leds;
		if (led_pending_ || led_sent_ == led_) return MAKE_ERROR(Error::kSuccess);
		return SendLed();
	}

	// one byte of LED state, report id 0 so wValue is 0x0200
	Error HIDBaseDriver::SendLed() {
		led_sent_ = led_;
		led_pending_ = true;
		SetupData setup_data{};
		setup_data.request_type.bits.direction = request_type::kOut;
		setup_data.request_type.bits.type = request_type::kClass;
		setup_data.request_type.bits.recipient = request_type::kInterface;
		setup_data.request = request::kSetReport;
		setup_data.value = 0x0200;// output report, report id 0
		setup_data.index = interface_index_;
		setup_data.length = 1;
		return ParentDevice()->ControlOut(kDefaultControlPipeID, setup_data, &led_sent_, 1, this);
	}

	Error HIDBaseDriver::OnInterruptCompleted(EndpointID ep_id, const void* buf, int len) {
		(void)buf;
		if (ep_id.IsIn()) {
			// a NAK or a failed poll carries no bytes: only a real report reaches the class
			if (len > 0) {
				OnDataReceived();
				std::copy_n(buf_.begin(), len, previous_buf_.begin());
			}
			return ParentDevice()->InterruptIn(ep_interrupt_in_, buf_.data(), in_packet_size_);
		}
		return MAKE_ERROR(Error::kNotImplemented);
	}

}
