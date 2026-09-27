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

_PACKED(struct) keyboard_modifier_t {
	byte l_ctrl : 1;
	byte l_shift : 1;
	byte l_alt : 1;
	byte l_logo : 1;// e.g. Windows key
	byte r_ctrl : 1;
	byte r_shift : 1;
	byte r_alt : 1;
	byte r_logo : 1;
};

#ifdef _INC_CPP
#ifdef _DEV_GCC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#endif
_PACKED(struct) keyboard_state_t
	// : public keyboard_modifier_t
{
	union {
		byte mod_val;
		keyboard_modifier_t mod;
	};
	byte lock_number : 1;
	byte lock_caps : 1;
	byte lock_scroll : 1;
};

_PACKED(struct) keyboard_event_t
{
	union {
		byte mod_val;
		keyboard_modifier_t mod;
	};
	byte resv;
	enum class method_t : byte {
		keydown,
		keyup,
		keyrepeat,
	} method;
	byte keycode;
};

enum _USB_IF_KEYENUM {
	_UKEY_NONE = 0x00,// reserved, no event
	_UKEY_ERR_ROLLOVER = 0x01,
	_UKEY_POSTFAIL = 0x02,
	_UKEY_ERR_UNDEFINED = 0x03,
	// letters
	_UKEY_A = 0x04,
	_UKEY_B = 0x05,
	_UKEY_C = 0x06,
	_UKEY_D = 0x07,
	_UKEY_E = 0x08,
	_UKEY_F = 0x09,
	_UKEY_G = 0x0A,
	_UKEY_H = 0x0B,
	_UKEY_I = 0x0C,
	_UKEY_J = 0x0D,
	_UKEY_K = 0x0E,
	_UKEY_L = 0x0F,
	_UKEY_M = 0x10,
	_UKEY_N = 0x11,
	_UKEY_O = 0x12,
	_UKEY_P = 0x13,
	_UKEY_Q = 0x14,
	_UKEY_R = 0x15,
	_UKEY_S = 0x16,
	_UKEY_T = 0x17,
	_UKEY_U = 0x18,
	_UKEY_V = 0x19,
	_UKEY_W = 0x1A,
	_UKEY_X = 0x1B,
	_UKEY_Y = 0x1C,
	_UKEY_Z = 0x1D,
	// digits 1-0
	_UKEY_1 = 0x1E,
	_UKEY_2 = 0x1F,
	_UKEY_3 = 0x20,
	_UKEY_4 = 0x21,
	_UKEY_5 = 0x22,
	_UKEY_6 = 0x23,
	_UKEY_7 = 0x24,
	_UKEY_8 = 0x25,
	_UKEY_9 = 0x26,
	_UKEY_0 = 0x27,
	// editing and punctuation
	_UKEY_ENTER = 0x28,
	_UKEY_ESC = 0x29,
	_UKEY_BACKSPACE = 0x2A,
	_UKEY_TAB = 0x2B,
	_UKEY_SPACE = 0x2C,
	_UKEY_MINUS = 0x2D,// -_
	_UKEY_EQUAL = 0x2E,// =+
	_UKEY_LBRACKET = 0x2F,// [{
	_UKEY_RBRACKET = 0x30,// ]}
	_UKEY_BACKSLASH = 0x31,// \|
	_UKEY_NONUS_HASH = 0x32,// non-US #~
	_UKEY_SEMICOLON = 0x33,// ;:
	_UKEY_QUOTE = 0x34,// '"
	_UKEY_GRAVE = 0x35,// `~
	_UKEY_COMMA = 0x36,// ,<
	_UKEY_DOT = 0x37,// .>
	_UKEY_SLASH = 0x38,// /?
	// locks and function keys
	_UKEY_CAPSLOCK = 0x39,
	_UKEY_F1 = 0x3A,
	_UKEY_F2 = 0x3B,
	_UKEY_F3 = 0x3C,
	_UKEY_F4 = 0x3D,
	_UKEY_F5 = 0x3E,
	_UKEY_F6 = 0x3F,
	_UKEY_F7 = 0x40,
	_UKEY_F8 = 0x41,
	_UKEY_F9 = 0x42,
	_UKEY_F10 = 0x43,
	_UKEY_F11 = 0x44,
	_UKEY_F12 = 0x45,
	// navigation
	_UKEY_PRINTSCREEN = 0x46,
	_UKEY_SCROLLLOCK = 0x47,
	_UKEY_PAUSE = 0x48,
	_UKEY_INSERT = 0x49,
	_UKEY_HOME = 0x4A,
	_UKEY_PAGEUP = 0x4B,
	_UKEY_DELETE = 0x4C,
	_UKEY_END = 0x4D,
	_UKEY_PAGEDOWN = 0x4E,
	_UKEY_RIGHT = 0x4F,
	_UKEY_LEFT = 0x50,
	_UKEY_DOWN = 0x51,
	_UKEY_UP = 0x52,
	// keypad
	_UKEY_NUMLOCK = 0x53,
	_UKEY_KP_SLASH = 0x54,
	_UKEY_KP_ASTERISK = 0x55,
	_UKEY_KP_MINUS = 0x56,
	_UKEY_KP_PLUS = 0x57,
	_UKEY_KP_ENTER = 0x58,
	_UKEY_KP_1 = 0x59,
	_UKEY_KP_2 = 0x5A,
	_UKEY_KP_3 = 0x5B,
	_UKEY_KP_4 = 0x5C,
	_UKEY_KP_5 = 0x5D,
	_UKEY_KP_6 = 0x5E,
	_UKEY_KP_7 = 0x5F,
	_UKEY_KP_8 = 0x60,
	_UKEY_KP_9 = 0x61,
	_UKEY_KP_0 = 0x62,
	_UKEY_KP_DOT = 0x63,
	_UKEY_NONUS_BACKSLASH = 0x64,// non-US \|
	_UKEY_APPLICATION = 0x65,
	_UKEY_POWER = 0x66,
	_UKEY_KP_EQUAL = 0x67,
	// F13-F24 and editing commands
	_UKEY_F13 = 0x68,
	_UKEY_F14 = 0x69,
	_UKEY_F15 = 0x6A,
	_UKEY_F16 = 0x6B,
	_UKEY_F17 = 0x6C,
	_UKEY_F18 = 0x6D,
	_UKEY_F19 = 0x6E,
	_UKEY_F20 = 0x6F,
	_UKEY_F21 = 0x70,
	_UKEY_F22 = 0x71,
	_UKEY_F23 = 0x72,
	_UKEY_F24 = 0x73,
	_UKEY_EXECUTE = 0x74,
	_UKEY_HELP = 0x75,
	_UKEY_MENU = 0x76,
	_UKEY_SELECT = 0x77,
	_UKEY_STOP = 0x78,
	_UKEY_AGAIN = 0x79,
	_UKEY_UNDO = 0x7A,
	_UKEY_CUT = 0x7B,
	_UKEY_COPY = 0x7C,
	_UKEY_PASTE = 0x7D,
	_UKEY_FIND = 0x7E,
	// multimedia
	_UKEY_MUTE = 0x7F,
	_UKEY_VOLUME_UP = 0x80,
	_UKEY_VOLUME_DOWN = 0x81,
	// locking variants and international
	_UKEY_LOCKING_CAPS = 0x82,
	_UKEY_LOCKING_NUM = 0x83,
	_UKEY_LOCKING_SCROLL = 0x84,
	_UKEY_KP_COMMA = 0x85,
	_UKEY_KP_EQUAL_AS400 = 0x86,
	_UKEY_INTL1 = 0x87,
	_UKEY_INTL2 = 0x88,
	_UKEY_INTL3 = 0x89,
	_UKEY_INTL4 = 0x8A,
	_UKEY_INTL5 = 0x8B,
	_UKEY_INTL6 = 0x8C,
	_UKEY_INTL7 = 0x8D,
	_UKEY_INTL8 = 0x8E,
	_UKEY_INTL9 = 0x8F,
	_UKEY_LANG1 = 0x90,
	_UKEY_LANG2 = 0x91,
	_UKEY_LANG3 = 0x92,
	_UKEY_LANG4 = 0x93,
	_UKEY_LANG5 = 0x94,
	_UKEY_LANG6 = 0x95,
	_UKEY_LANG7 = 0x96,
	_UKEY_LANG8 = 0x97,
	_UKEY_LANG9 = 0x98,
	// misc
	_UKEY_ALT_ERASE = 0x99,
	_UKEY_SYSREQ = 0x9A,
	_UKEY_CANCEL = 0x9B,
	_UKEY_CLEAR = 0x9C,
	_UKEY_PRIOR = 0x9D,
	_UKEY_RETURN = 0x9E,
	_UKEY_SEPARATOR = 0x9F,
	_UKEY_OUT = 0xA0,
	_UKEY_OPER = 0xA1,
	_UKEY_CLEAR_AGAIN = 0xA2,
	_UKEY_CRSEL = 0xA3,
	_UKEY_EXSEL = 0xA4,
	// keypad extras
	_UKEY_KP_00 = 0xB0,
	_UKEY_KP_000 = 0xB1,
	_UKEY_THOUSANDS_SEP = 0xB2,
	_UKEY_DECIMAL_SEP = 0xB3,
	_UKEY_CURRENCY_UNIT = 0xB4,
	_UKEY_CURRENCY_SUB = 0xB5,
	_UKEY_KP_LPAREN = 0xB6,
	_UKEY_KP_RPAREN = 0xB7,
	_UKEY_KP_LBRACE = 0xB8,
	_UKEY_KP_RBRACE = 0xB9,
	_UKEY_KP_TAB = 0xBA,
	_UKEY_KP_BACKSPACE = 0xBB,
	_UKEY_KP_A = 0xBC,
	_UKEY_KP_B = 0xBD,
	_UKEY_KP_C = 0xBE,
	_UKEY_KP_D = 0xBF,
	_UKEY_KP_E = 0xC0,
	_UKEY_KP_F = 0xC1,
	_UKEY_KP_XOR = 0xC2,
	_UKEY_KP_CARET = 0xC3,
	_UKEY_KP_PERCENT = 0xC4,
	_UKEY_KP_LESS = 0xC5,
	_UKEY_KP_GREATER = 0xC6,
	_UKEY_KP_AMPERSAND = 0xC7,
	_UKEY_KP_AND = 0xC8,
	_UKEY_KP_BAR = 0xC9,
	_UKEY_KP_OR = 0xCA,
	_UKEY_KP_COLON = 0xCB,
	_UKEY_KP_HASH = 0xCC,
	_UKEY_KP_SPACE = 0xCD,
	_UKEY_KP_AT = 0xCE,
	_UKEY_KP_EXCLAM = 0xCF,
	_UKEY_KP_MEM_STORE = 0xD0,
	_UKEY_KP_MEM_RECALL = 0xD1,
	_UKEY_KP_MEM_CLEAR = 0xD2,
	_UKEY_KP_MEM_ADD = 0xD3,
	_UKEY_KP_MEM_SUB = 0xD4,
	_UKEY_KP_MEM_MUL = 0xD5,
	_UKEY_KP_MEM_DIV = 0xD6,
	_UKEY_KP_PLUSMINUS = 0xD7,
	_UKEY_KP_CLEAR = 0xD8,
	_UKEY_KP_CLEAR_ENTRY = 0xD9,
	_UKEY_KP_BINARY = 0xDA,
	_UKEY_KP_OCTAL = 0xDB,
	_UKEY_KP_DECIMAL = 0xDC,
	_UKEY_KP_HEXADECIMAL = 0xDD,
	// modifiers, bit order matches keyboard_modifier_t
	_UKEY_LCTRL = 0xE0,
	_UKEY_LSHIFT = 0xE1,
	_UKEY_LALT = 0xE2,
	_UKEY_LGUI = 0xE3,
	_UKEY_RCTRL = 0xE4,
	_UKEY_RSHIFT = 0xE5,
	_UKEY_RALT = 0xE6,
	_UKEY_RGUI = 0xE7,
	//{} /// MORE
};

// HID LED page (0x08) usages for the SET_REPORT output report bitmap
enum _USB_IF_LEDENUM {
	_ULED_NUMLOCK = 0x01,
	_ULED_CAPSLOCK = 0x02,
	_ULED_SCROLLLOCK = 0x04,
	_ULED_COMPOSE = 0x08,
	_ULED_KANA = 0x10,
};

#ifdef _DEV_GCC
static_assert(sizeof(keyboard_event_t) == 4);
static_assert(__builtin_offsetof(keyboard_event_t, keycode) == 3);
#pragma GCC diagnostic pop
#endif
#endif

// ---- ATX PS/2 Keyboard ---- //

#define KEYBOARD_LED 0xED
#define KEYBOARD_ACK 0xFA

extern const byte key_ps2set1_usb[128];
extern const uint8_t key_ps2set1_usb_E0[128];

_ESYM_C void Keyboard_Init();
_ESYM_C void Keyboard_Wait();

// stat: B2 Caps, B1 Num, B0 Scroll
_ESYM_C void KbdSetLED(byte stat);

// ---- ATX USB Keyboard ---- //

#if defined(_INC_CPP)
#include "../../cpp/Device/USB/USBHost-HID.hpp"
#include "../../../inc/c/msgface.h"
#include "../../cpp/Device/USB/USB-Header.hpp"


namespace uni::device::SpaceUSB {
	class HIDKeyboardDriver : public HIDBaseDriver {
	public:
		HIDKeyboardDriver(USBHostDevice* dev, int interface_index);

#if defined(_MCCA) && ((_MCCA)==0x8664)
		void* operator new(size_t size);
		void operator delete(void* ptr) noexcept;
#endif

		Error OnDataReceived() override;

		// a boot keyboard owns the LED output report
		bool HasLedReport() const override { return true; }

		using ObserverType = void(*)(keyboard_event_t keyevent);
		void SubscribeKeyPush(ObserverType observer);
		static ObserverType default_observer;

	private:
		std::array<ObserverType, 4> observers_;
		int num_observers_ = 0;

		void NotifyKeyPush(keyboard_event_t keyevent);
		// AKA the LED output report: CapsLock/NumLock/ScrollLock drive the device LEDs
		void ToggleLockLed(byte keycode);
	};
}

#endif

#endif
