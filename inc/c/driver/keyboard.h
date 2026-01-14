// ASCII CPL TAB4 CRLF
// Docutitle: (Device) Keyboard
// Codifiers: @dosconio: 20240502 ~ 20240502
// Attribute: Arn-Covenant Any-Architect Bit-32mode Non-Dependence
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

#ifndef _INC_DEVICE_Keyboard
#define _INC_DEVICE_Keyboard

#include "../stdinc.h"

typedef struct {
	byte ascii_usual;
	byte ascii_shift;
	const char* label_usual;// if not null, the char is not printable
	const char* label_shift;// if not null, the char is not printable
	rostr label_prefE0;
} keymap_element_t;

struct keyboard_state_t {
	byte l_ctrl : 1;
	byte r_ctrl : 1;
	byte l_shift : 1;
	byte r_shift : 1;
	byte l_alt : 1;
	byte r_alt : 1;
	byte l_logo : 1;// e.g. Windows key
	byte r_logo : 1;
	//
	byte lock_number : 1;
	byte lock_caps : 1;
	byte lock_scroll : 1;
	//
};

// ---- ATX PS/2 Keyboard ---- //

#define KEYBOARD_LED 0xED
#define KEYBOARD_ACK 0xFA

extern keymap_element_t _tab_keycode2ascii[0x80];

_ESYM_C void Keyboard_Init();
_ESYM_C void Keyboard_Wait();

// stat: B2 Caps, B1 Num, B0 Scroll
_ESYM_C void KbdSetLED(byte stat);

// ---- ATX USB Keyboard ---- //

#if defined(_INC_CPP) && (defined(_MCCA) && ((_MCCA)==0x8664))
#include "../../../inc/c/msgface.h"
#include <functional>
#include "../../cpp/Device/USB/USB.hpp"
#include "../../cpp/Device/USB/USB-Header.hpp"


namespace usb {
	class HIDKeyboardDriver : public HIDBaseDriver {
	public:
		HIDKeyboardDriver(Device* dev, int interface_index);

		void* operator new(size_t size);
		void operator delete(void* ptr) noexcept;

		Error OnDataReceived() override;

		using ObserverType = void(uint8_t keycode);
		void SubscribeKeyPush(std::function<ObserverType> observer);
		static std::function<ObserverType> default_observer;

	private:
		std::array<std::function<ObserverType>, 4> observers_;
		int num_observers_ = 0;

		void NotifyKeyPush(uint8_t keycode);
	};
}

#endif

#endif
