// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Device.USB) OTG Host Device Bridge
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

#ifndef _INC_DEVICE_USB_OTGHOSTDEVICE
#define _INC_DEVICE_USB_OTGHOSTDEVICE

#include "../../unisym"
#if defined(_MCU_STM32)
#include "USB-Header.hpp"
#include "USB-Device.hpp"
#include "HCD.hpp"
#endif

#if defined(_MCU_STM32H7x)

#include <array>
#include <optional>

namespace uni::device::SpaceUSB {

	// AKA USBHostDevice_v3 (xHCI) but on the H7 OTG host controller: bridges the
	// USBHostDevice protocol stack (enumeration + class drivers) onto HCD channels.
	// Channel 0 is the default control pipe; other endpoints use their number.
	class OTGHostDevice : public USBHostDevice {
	public:
		OTGHostDevice(HCD& hcd, byte dev_address, byte speed);

		// transport backends (called by the USBHostDevice protocol stack)
		Error ControlIn(EndpointID ep_id, SetupData setup_data,
			void* buf, int len, ClassDriver* issuer) override;
		Error ControlOut(EndpointID ep_id, SetupData setup_data,
			const void* buf, int len, ClassDriver* issuer) override;
		Error InterruptIn(EndpointID ep_id, void* buf, int len) override;
		Error InterruptOut(EndpointID ep_id, void* buf, int len) override;
		Error OnHubPortStatusReceived(uint8 port_num, uint16 status, uint16 change) override;

		// URB completion dispatcher; called from HCD.NotifyURBChangeHandler
		void OnChannelURBCompleted(byte ch_num, URBState urb_state);

		// configure all non-control endpoints from the parsed configuration
		Error ConfigureEndpoints();

		HCD& Controller() { return hcd_; }
		byte DeviceAddress() const { return dev_address_; }

	private:
		// control transfer stage machine (AKA HCD control pipe state)
		enum class ControlStage : byte {
			Idle, Setup, DataIn, DataOut, StatusIn, StatusOut, Done
		};
		ControlStage ctrl_stage_ = ControlStage::Idle;
		EndpointID ctrl_ep_id_{};
		SetupData ctrl_setup_{};
		void* ctrl_buf_ = nullptr;
		int ctrl_len_ = 0;

		HCD& hcd_;
		const byte dev_address_;
		const byte speed_;
	};

}

#endif // _MCU_STM32H7x

#endif // _INC_DEVICE_USB_OTGHOSTDEVICE
