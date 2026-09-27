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

#ifndef _INC_DEVICE_USB_HOST_HID
#define _INC_DEVICE_USB_HOST_HID

#include "USB.hpp"

namespace uni::device::SpaceUSB {

	// AKA USBH_HID_CLASS: SET_PROTOCOL(boot) once the endpoints are configured, then
	// an interrupt-IN poll that re-arms itself after every completion (or failure),
	// handing each report to OnDataReceived().
	class HIDBaseDriver : public ClassDriver {
	public:
		HIDBaseDriver(USBHostDevice* dev, int interface_index, int in_packet_size);

		Error Initialize() override;
		Error SetEndpoint(const EndpointConfig& config) override;
		Error OnEndpointsConfigured() override;
		Error OnControlCompleted(EndpointID ep_id, SetupData setup_data, const void* buf, int len) override;
		Error OnInterruptCompleted(EndpointID ep_id, const void* buf, int len) override;

		virtual Error OnDataReceived() = 0;
		const static size_t kBufferSize = 1024;
		const uni::Array<uint8_t, kBufferSize>& Buffer() const { return buf_; }
		const uni::Array<uint8_t, kBufferSize>& PreviousBuffer() const { return previous_buf_; }

	private:
		EndpointID ep_interrupt_in_{};
		EndpointID ep_interrupt_out_{};
		const int interface_index_;
		int in_packet_size_;
		int initialize_phase_{ 0 };

		uni::Array<uint8_t, kBufferSize> buf_{}, previous_buf_{};
	};

}


#endif // _INC_DEVICE_USB_HOST_HID
