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

//{TODO} Boot protocol mouse report: 3 bytes (buttons, X, Y) -- there is no wheel byte in it
_PACKED(struct) MouseMessage {
	byte BtnLeft : 1;
	byte BtnRight : 1;
	byte BtnMiddle : 1;
	byte HIGH : 1;// always 1
	byte DirX : 1;
	byte DirY : 1;
	byte OvfX : 1;
	byte OvfY : 1;
	int8 X;// sign of PS2 depends on DirX; sign of USB is always signed.
	int8 Y;// sign of PS2 depends on DirY; sign of USB is always signed.
};

#ifdef _INC_CPP
#include "../graphic.h"

// 1. avoid usual overflow of 8-bit
struct MouseGeneralMessage {
	bool btn_left;
	bool btn_middle;
	bool btn_right;
	uni::Size2dif movement;
	MouseGeneralMessage() : btn_left(false), btn_middle(false), btn_right(false), movement(0, 0) {}
	// IBM PS/2 Hardware Interface Technical Reference, Mouse: signed 9-bit packet displacement.
	MouseGeneralMessage(const MouseMessage& mmsg, bool xy_depends_on_dir = true)
		: btn_left(mmsg.BtnLeft), btn_middle(mmsg.BtnMiddle), btn_right(mmsg.BtnRight) {
		const uint8 raw_x = uint8(mmsg.X);
		const uint8 raw_y = uint8(mmsg.Y);
		if (xy_depends_on_dir) {
			movement.x = int(raw_x) - (mmsg.DirX ? 256 : 0);
			movement.y = -(int(raw_y) - (mmsg.DirY ? 256 : 0));
		}
		else {
			// USB HID 1.11 Appendix B.2: signed 8-bit displacement, without PS/2 Y inversion.
			movement.x = int(raw_x) - ((raw_x & 0x80) ? 256 : 0);
			movement.y = int(raw_y) - ((raw_y & 0x80) ? 256 : 0);
		}
	}
};
#endif

// pres: init keyboard
_ESYM_C void Mouse_Init();


#include "../../cpp/Device/USB/USBHost-HID.hpp"
#include "../../../inc/c/msgface.h"
#include "../../cpp/Device/USB/USB-Header.hpp"

namespace uni::device::SpaceUSB {
	class HIDMouseDriver : public HIDBaseDriver {
	public:
		HIDMouseDriver(USBHostDevice* dev, int interface_index);
		void* operator new(size_t size);
		void operator delete(void* ptr) noexcept;

		Error OnDataReceived() override;

		using ObserverType = void(*)(MouseGeneralMessage mmsg);
		void SubscribeMouseMove(ObserverType observer);
		static ObserverType default_observer;

	private:
		std::array<ObserverType, 4> observers_;
		int num_observers_ = 0;

		void NotifyMouseMove(MouseGeneralMessage mmsg);
	};
}

#endif
