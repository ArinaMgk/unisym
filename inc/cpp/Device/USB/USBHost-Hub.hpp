// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: [Device.USB] Host Hub Class Driver
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

#ifndef _INC_DEVICE_USB_HOST_HUB
#define _INC_DEVICE_USB_HOST_HUB

#include "USB.hpp"

namespace uni::device::SpaceUSB {

	// AKA the hub class port feature selectors and the status bits the policy tests
	const uint16 kHubPortFeatureReset = 4;
	const uint16 kHubPortFeaturePower = 8;
	const uint16 kHubPortStatusConnect = 0x0001;
	const uint16 kHubPortStatusEnable = 0x0002;
	const uint16 kHubPortStatusLowSpeed = 0x0200;// PORT_LOW_SPEED of wPortStatus

	// AKA USBH_HUB_CLASS: reads the hub descriptor, then walks every port and hands
	// each port status change to the transport through OnHubPortStatusReceived().
	class USBHubDriver : public ClassDriver {
	public:
		explicit USBHubDriver(USBHostDevice* dev);

		void* operator new(size_t size);
		void operator delete(void* ptr) noexcept;

		Error Initialize() override;
		Error SetEndpoint(const EndpointConfig& config) override;
		Error OnEndpointsConfigured() override;
		Error OnControlCompleted(EndpointID ep_id, SetupData setup_data, const void* buf, int len) override;
		Error OnInterruptCompleted(EndpointID ep_id, const void* buf, int len) override;

		// AKA the port policy, ticked by the host transport about once per millisecond
		Error ProcessDelayed() override;
		Error RequestSetPortFeature(uint8 port_num, uint16 feature_selector);
		Error RecordPortStatus(uint8 port_num, const void* buf);

		// AKA the port state machine: poll a port, report the change, clear its change bits
		Error RequestPortStatus(uint8 port_num);
		Error RequestClearPortFeature(uint8 port_num, uint16 feature_selector);
		Error StartStatusChangePolling();
		Error ContinuePendingPortStatus();
		Error ClearNextPendingChange();

		uint8 NumPorts() const { return num_ports_; }

	private:
		// AKA the port policy state: it advances only while the transport ticks ProcessDelayed()
		uint16 port_status_[16] = {};
		uint16 port_change_[16] = {};
		uint8 pending_kind_ = 0;// 1 = SET_FEATURE in flight, 2 = GET_STATUS of the policy
		uint8 delayed_step_ = 0;
		uint8 port_cursor_ = 0;
		uint8 wait_ticks_ = 0;
		uint8 power_good_ticks_ = 0;
		uint16 ready_mask_ = 0;
		uint16 reset_mask_ = 0;// ports whose PORT_RESET is still settling
		uint8 reset_count_[16] = {};// resets left for a port that has not reached ENABLE yet
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
		uni::Array<uint8_t, sizeof(HubDescriptor)> buf_{};
		uni::Array<uint8_t, sizeof(HubPortStatus)> port_status_buf_{};
		uni::Array<uint8_t, 8> interrupt_buf_{};
		uni::Array<uint8_t, 32> pending_status_ports_{};
	};

}

#endif // _INC_DEVICE_USB_HOST_HUB
