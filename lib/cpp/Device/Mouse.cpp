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

#ifdef _SUPPORT_Port8
#include "../../../inc/c/driver/i8259A.h"
#include "../../../inc/c/board/IBM.h"

#define KEYCMD_SENDTO_MOUSE 0xD4
#define MOUSECMD_ENABLE     0xF4

void Mouse_Init()
{
	i8259Master_Enable(2);
	i8259Slaver_Enable(4);// KBD
	//
	Keyboard_Wait();
	outpb(PORT_KEYBOARD_CMD, 0xA8); // enable aux device
	//
	Keyboard_Wait();
	outpb(PORT_KEYBOARD_CMD, KEYCMD_SENDTO_MOUSE);
	Keyboard_Wait();
	outpb(PORT_KEYBOARD_DAT, MOUSECMD_ENABLE);
	return; /* うまくいくとACK(0xfa)が送信されてくる */
}

#endif

#if defined(_INC_CPP) && ((defined(_UEFI) && (defined(_MCCA) && ((_MCCA & 0xFF00)==0x8600))) || defined(_MCU_STM32H7x))

#if defined(_MCCA) && ((_MCCA & 0xFF00)==0x8600)
void* uni::device::SpaceUSB::HIDMouseDriver::operator new(size_t size) {
	auto ret = uni_hostenv_allocator->allocate(sizeof(HIDMouseDriver));
	return ret;
}

void uni::device::SpaceUSB::HIDMouseDriver::operator delete(void* ptr) noexcept {
	uni_hostenv_allocator->deallocate(ptr);
}
#endif

namespace uni::device::SpaceUSB {
	HIDMouseDriver::HIDMouseDriver(USBHostDevice* dev, int interface_index)
		: HIDBaseDriver{ dev, interface_index, 3 } {
	}

	Error HIDMouseDriver::OnDataReceived() {
		MouseMessage mmsg;
		MemCopyN(&mmsg, (const void*)&Buffer()[0], sizeof(MouseMessage));
		NotifyMouseMove(mmsg);
		return MAKE_ERROR(Error::kSuccess);
	}

	void HIDMouseDriver::SubscribeMouseMove(
		ObserverType observer) {
		observers_[num_observers_++] = observer;
	}

	HIDMouseDriver::ObserverType HIDMouseDriver::default_observer;

	void HIDMouseDriver::NotifyMouseMove(MouseMessage mmsg) {
		for (int i = 0; i < num_observers_; ++i) {
			observers_[i](mmsg);
		}
	}
}


#endif
