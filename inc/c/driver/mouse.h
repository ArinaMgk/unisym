// ASCII CPL TAB4 CRLF
// Docutitle: (Device) Mouse / マウス
// Codifiers: @ArinaMgk
// Attribute: 
// Copyright: UNISYM, under Apache License 2.0; Dosconio Mecocoa, BSD 3-Clause License
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

#ifndef _INC_DEVICE_Mouse
#define _INC_DEVICE_Mouse



#include "../stdinc.h"

// x86
#include "./keyboard.h"

_PACKED(struct) MouseMessage {
	byte BtnLeft : 1;
	byte BtnRight : 1;
	byte BtnMiddle : 1;
	byte HIGH : 1;// always 1
	byte DirX : 1;
	byte DirY : 1;
	byte OvfX : 1;
	byte OvfY : 1;
	int8 X;
	int8 Y;
};

// pres: init keyboard
_ESYM_C void Mouse_Init();

#if defined(_INC_CPP) && (defined(_UEFI))
#include "../../../inc/c/msgface.h"
#include <functional>
#include "../../cpp/Device/USB/USB.hpp"
#include "../../cpp/Device/USB/USB-Header.hpp"
#include "../../cpp/Device/USB/xHCI/xHCI.hpp"
#include "../../cpp/Device/Bus/PCI.hpp"

_ESYM_C uni::PCI::Device* Mouse_Init_USB(uni::PCI& pci, usb::xhci::Controller* xhc);

namespace usb {
	class HIDMouseDriver : public HIDBaseDriver {
	public:
		HIDMouseDriver(Device* dev, int interface_index);

		void* operator new(size_t size);
		void operator delete(void* ptr) noexcept;

		Error OnDataReceived() override;

		using ObserverType = void(MouseMessage mmsg);
		void SubscribeMouseMove(std::function<ObserverType> observer);
		static std::function<ObserverType> default_observer;

	private:
		std::array<std::function<ObserverType>, 4> observers_;
		int num_observers_ = 0;

		void NotifyMouseMove(MouseMessage mmsg);
	};
}

#endif

#endif
