// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: [Device.USB] Peripheral Class Base
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

#ifndef _INC_DEVICE_USB_PERIPHERAL_CLASS
#define _INC_DEVICE_USB_PERIPHERAL_CLASS

#include "../../../c/stdinc.h"
#include "../../interrupt"

#if defined(_MCU_STM32H7x)

namespace uni::device::SpaceUSB {

	class PeripheralDevice;

	// Base class of device-side USB class drivers (AKA USBD_ClassTypeDef).
	// A concrete class (e.g. USBPeri_MSC) implements these callbacks and is
	// linked to a PeripheralDevice via RegisterClass().
	class ClassPeripheral {
	public:
		virtual ~ClassPeripheral() = default;

		// class configuration set / cleared (AKA USBD_Class.Init/DeInit)
		virtual bool setMode(byte cfgidx) = 0;
		virtual bool canMode(byte cfgidx) = 0;
		// class-specific setup request (AKA USBD_Class.Setup)
		virtual bool Setup() = 0;
		// EP0 tx/rx completed (optional)
		virtual bool EP0TxSent() { return true; }
		virtual bool EP0RxReady() { return true; }
		// class data stage on non-EP0 endpoint
		virtual bool inn(byte epnum) { return true; }
		virtual bool out(byte epnum) { return true; }
		virtual bool SOF() { return true; }

		// configuration descriptors (AKA GetHS/FS/OtherSpeedConfigDescriptor)
		virtual const byte* getConfigurationDescriptor(uint16& len) = 0;
		virtual const byte* getOtherSpeedConfigurationDescriptor(uint16& len) = 0;
		virtual const byte* getDeviceQualifierDescriptor(uint16& len) = 0;

		// bound device core
		PeripheralDevice* Parent() const { return parent_; }
		void setParent(PeripheralDevice* dev) { parent_ = dev; }

	private:
		PeripheralDevice* parent_ = nullptr;
	};

}

#endif // _MCU_STM32H7x

#endif // _INC_DEVICE_USB_PERIPHERAL_CLASS
