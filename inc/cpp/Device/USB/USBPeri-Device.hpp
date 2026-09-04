// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: [Device.USB] Peripheral Device Stack
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

#ifndef _INC_DEVICE_USB_PERIPHERAL_DEVICE
#define _INC_DEVICE_USB_PERIPHERAL_DEVICE

#include "../../../c/stdinc.h"
#include "../../reference"
#include "../../interrupt"
#include "PCD.hpp"

#if defined(_MCU_STM32H7x)

namespace uni::device::SpaceUSB {

	// AKA USBD_StateTypeDef
	enum class PeripheralState : unsigned {
		Default = 1, Address = 2, Configured = 3, Suspended = 4
	};

	// AKA USBD_EP0 state machine
	enum class ControlState : unsigned {
		Idle = 0, Setup = 1, DataInn = 2, DataOut = 3,
		StatusInn = 4, StatusOut = 5, Stall = 6
	};

	// AKA USBD_SetupReqTypedef
	_PACKED(struct) SetupPacket {
		byte request_type;
		byte request;
		uint16 value;
		uint16 index;
		uint16 length;
	};

	// AKA USBD_SpeedTypeDef
	enum class PeripheralSpeed : byte {
		High = 0, Full = 1, Low = 2
	};

	// Device-level descriptors (AKA USBD_DescriptorsTypeDef / usbd_desc.c):
	// the device identity (VID/PID, strings) is independent of the class.
	struct PeripheralDescriptor {
		const byte* (*GetDeviceDescriptor)(PeripheralSpeed speed, uint16& length);
		const byte* (*GetLangIDStrDescriptor)(PeripheralSpeed speed, uint16& length);
		const byte* (*GetManufacturerStrDescriptor)(PeripheralSpeed speed, uint16& length);
		const byte* (*GetProductStrDescriptor)(PeripheralSpeed speed, uint16& length);
		const byte* (*GetSerialStrDescriptor)(PeripheralSpeed speed, uint16& length);
		const byte* (*GetConfigurationStrDescriptor)(PeripheralSpeed speed, uint16& length);
		const byte* (*GetInterfaceStrDescriptor)(PeripheralSpeed speed, uint16& length);
	};

	class ClassPeripheral;

	// Device-side USB stack (AKA USBD_HandleTypeDef): enumeration + standard
	// requests + EP0 control transfer, driven by PCD callbacks.
	class PeripheralDevice {
	public:
		// ---- endpoint state (AKA USBD_EndpointTypeDef) ----
		struct Endpoint {
			stduint status;
			stduint total_length;
			stduint rem_length;
			stduint maxpacket;
		};

		// ---- handle fields ----
		PCD* pcd = nullptr;              // bound device controller
		ClassPeripheral* pclass = nullptr;
		void* pclass_data = nullptr;
		PeripheralDescriptor* pdesc = nullptr;   // device identity (usbd_desc)
		SetupPacket request{};
		byte dev_address = 0;
		PeripheralState dev_state = PeripheralState::Default;
		PeripheralState dev_old_state = PeripheralState::Default;
		PeripheralSpeed dev_speed = PeripheralSpeed::High;
		ControlState ep0_state = ControlState::Idle;
		stduint ep0_data_len = 0;
		stduint dev_config = 0;
		stduint dev_config_status = 0;
		bool dev_remote_wakeup = false;
		byte dev_test_mode = 0;
		byte dev_connection_status = 0;
		Endpoint ep_in[15]{};
		Endpoint ep_out[15]{};

		// ---- lifecycle ----
		bool setMode(PCD& pcd, PeripheralDescriptor& desc);
		bool canMode();
		bool Start();
		bool Stop();

		// ---- class registration ----
		void RegisterClass(ClassPeripheral& cls);

		// ---- events (called by PCD callbacks) ----
		void HandleSetup();
		void HandleDataInn();
		void HandleDataOut();
		void HandleReset();
		void HandleSuspend();
		void HandleResume();
		void HandleSOF();
		void HandleConnect();
		void HandleDisconnect();

		// ---- EP0 control transfer primitives ----
		void SendControl(const byte* buf, uint16 len);
		void ReceiveControl(byte* buf, uint16 len);
		void SendStatus();
		void ReceiveStatus();
		void StallControl();

		// ---- endpoint ops (delegate to PCD) ----
		void OpenEndpoint(byte ep_addr, byte ep_type, uint16 ep_mps);
		void CloseEndpoint(byte ep_addr);
		void TransmitEndpoint(byte ep_addr, byte* buf, uint16 len);
		void ReceiveEndpoint(byte ep_addr, byte* buf, uint16 len);
		void ConfigStall(byte ep_addr, bool set_or_reset);
		void FlushEndpoint(byte ep_addr);
		void setAddress(byte address);
		uint16 getRxCount(byte ep_addr);

		// ---- standard request handlers ----
		void HandleStandardDeviceRequest();
		void HandleStandardInterfaceRequest();
		void HandleStandardEndpointRequest();

		// ---- descriptor dispatch ----
		void getDescriptor();
		void ParseSetup();

		// ---- class config ----
		bool SetClassConfiguration(byte cfgidx);
		bool ClearClassConfiguration(byte cfgidx);

		// state
		PeripheralState getState() const { return dev_state; }
		byte getDeviceAddress() const { return dev_address; }
		void setSpeed(PeripheralSpeed speed) { dev_speed = speed; }
		PeripheralSpeed getSpeed() const { return dev_speed; }
	};

}

#endif // _MCU_STM32H7x

#endif // _INC_DEVICE_USB_PERIPHERAL_DEVICE
