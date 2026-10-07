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

#include "./USB-Header.hpp"
#include "./USB-Device.hpp"

namespace uni::device::SpaceUSB {
	class USBHostDevice;
	using HubDescriptorCompleteHook = void (*)(USBHostDevice& dev);
	extern HubDescriptorCompleteHook g_hub_descriptor_complete_hook;
	using HubPortStatusHook = void (*)(USBHostDevice& dev, uint8 downstream_port, uint16 status, uint16 change);
	extern HubPortStatusHook g_hub_port_status_hook;
	// which port may be reset next (0 = none): only one device may sit at address 0 at a time
	using HubResetPortHook = uint8 (*)(USBHostDevice& dev);
	extern HubResetPortHook g_hub_reset_port_hook;

	// Base class of USB class drivers. Platform independent; the H7 host
	// bridge (OTGHostDevice) drives it over the OTG controller.
	class ClassDriver {
	public:
		ClassDriver(USBHostDevice* dev) : dev_{ dev } {}
		virtual ~ClassDriver() {}// the host device deletes its drivers through this base pointer

		virtual Error Initialize() = 0;
		virtual Error SetEndpoint(const EndpointConfig& config) = 0;
		virtual Error OnEndpointsConfigured() = 0;
		virtual Error OnControlCompleted(EndpointID ep_id, SetupData setup_data, const void* buf, int len) = 0;
		virtual Error OnInterruptCompleted(EndpointID ep_id, const void* buf, int len) = 0;
		// Bulk completion (AKA a host MSC disk); drivers without bulk endpoints keep this
		virtual Error OnBulkCompleted(EndpointID ep_id, const void* buf, int len) {
			(void)ep_id;
			(void)buf;
			(void)len;
			return MAKE_ERROR(Error::kNotImplemented);
		}
		virtual Error OnIsochronousCompleted(EndpointID ep_id, const void* buf,
			int len, uint16 frame_id, bool schedule_immediately, int completion_code) {
			(void)ep_id;
			(void)buf;
			(void)len;
			(void)frame_id;
			(void)schedule_immediately;
			(void)completion_code;
			return MAKE_ERROR(Error::kNotImplemented);
		}
		// AKA periodic service; the host transport ticks this about once per millisecond
		virtual Error ProcessDelayed() { return MAKE_ERROR(Error::kSuccess); }

		/** Returns the USB device that holds this class driver. */
		USBHostDevice* ParentDevice() const { return dev_; }

	private:
		USBHostDevice* dev_;
	};
}

#include "./USBHost-Hub.hpp"

#endif
