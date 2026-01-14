// UTF-8 CPL TAB4 CRLF
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

#include <stdlib.h>
#include "../../../inc/c/driver/mouse.h"
#include "../../../inc/c/driver/i8259A.h"

#ifdef _SUPPORT_Port8

#define KEYCMD_SENDTO_MOUSE 0xD4
#define MOUSECMD_ENABLE     0xF4

_ESYM_C void Mouse_Init()
{
	i8259Master_Enable(2);
	i8259Slaver_Enable(4);// KBD
	//
	// Keyboard_Wait();
	// outpb(KEYBOARD_CMD, 0xA8); // enable aux device
	//
	Keyboard_Wait();
	outpb(KEYBOARD_CMD, KEYCMD_SENDTO_MOUSE);
	Keyboard_Wait();
	outpb(KEYBOARD_DAT, MOUSECMD_ENABLE);
	return; /* うまくいくとACK(0xfa)が送信されてくる */
}

#endif

#if defined(_INC_CPP) && (defined(_MCCA) && ((_MCCA)==0x8664))
#include <algorithm>

namespace usb {
	HIDMouseDriver::HIDMouseDriver(Device* dev, int interface_index)
		: HIDBaseDriver{ dev, interface_index, 3 } {
	}

	Error HIDMouseDriver::OnDataReceived() {
		int8_t displacement_x = Buffer()[1];
		int8_t displacement_y = Buffer()[2];
		NotifyMouseMove(displacement_x, displacement_y);
		// Log(kDebug, "%02x,(%3d,%3d)\n", Buffer()[0], displacement_x, displacement_y);
		return MAKE_ERROR(Error::kSuccess);
	}

	void* HIDMouseDriver::operator new(size_t size) {
		return AllocMem(sizeof(HIDMouseDriver), 0, 0);
	}

	void HIDMouseDriver::operator delete(void* ptr) noexcept {
		// FreeMem(ptr);
	}

	void HIDMouseDriver::SubscribeMouseMove(
		std::function<void(int8_t displacement_x, int8_t displacement_y)> observer) {
		observers_[num_observers_++] = observer;
	}

	std::function<HIDMouseDriver::ObserverType> HIDMouseDriver::default_observer;

	void HIDMouseDriver::NotifyMouseMove(int8_t displacement_x, int8_t displacement_y) {
		for (int i = 0; i < num_observers_; ++i) {
			observers_[i](displacement_x, displacement_y);
		}
	}
}


#endif
