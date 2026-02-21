#include <stdlib.h>
#include "../../../inc/c/driver/keyboard.h"
#include "../../../inc/c/msgface.h"
#include <algorithm>

_ESYM_CPP
const char key_map[256] = {
// #embed "../../c/data/keyboard-USBIF_ASCII-ENUS-normal.dat"
0,    0,    0,    0,    'a',  'b',  'c',  'd', // 0x00
'e',  'f',  'g',  'h',  'i',  'j',  'k',  'l', 
'm',  'n',  'o',  'p',  'q',  'r',  's',  't', // 0x10
'u',  'v',  'w',  'x',  'y',  'z',  '1',  '2', 
'3',  '4',  '5',  '6',  '7',  '8',  '9',  '0', // 0x20
'\n', 0,    '\b', '\t', ' ',  '-',  '=',  '[', 
']', '\\',  '#',  ';', '\'',  '`',  ',',  '.', // 0x30
'/',  0,    0,    0,    0,    0,    0,    0,   
0,    0,    0,    0,    0,    0,    0,    0,   // 0x40
0,    0,    0,    0,    0,    0,    0,    0,   
0,    0,    0,    0,    '/',  '*',  '-',  '+', // 0x50
'\n', '1',  '2',  '3',  '4',  '5',  '6',  '7', 
'8',  '9',  '0',  '.', '\\',  0,    0,    '=', // 0x60
};

_ESYM_CPP
const char key_map_shift[256] = {
0,    0,    0,    0,    'A',  'B',  'C',  'D', // 0x00
'E',  'F',  'G',  'H',  'I',  'J',  'K',  'L', 
'M',  'N',  'O',  'P',  'Q',  'R',  'S',  'T', // 0x10
'U',  'V',  'W',  'X',  'Y',  'Z',  '!',  '@', 
'#',  '$',  '%',  '^',  '&',  '*',  '(',  ')', // 0x20
'\n', '\b', 0x08, '\t', ' ',  '_',  '+',  '{', 
'}',  '|',  '~',  ':',  '"',  '~',  '<',  '>', // 0x30
'?',  0,    0,    0,    0,    0,    0,    0,   
0,    0,    0,    0,    0,    0,    0,    0,   // 0x40
0,    0,    0,    0,    0,    0,    0,    0,   
0,    0,    0,    0,    '/',  '*',  '-',  '+', // 0x50
'\n', '1',  '2',  '3',  '4',  '5',  '6',  '7', 
'8',  '9',  '0',  '.', '\\',  0,    0,    '=', // 0x60
};

#if defined(_INC_CPP) && (defined(_MCCA) && ((_MCCA)==0x8664))
namespace uni::device::SpaceUSB {
	void* HIDKeyboardDriver::operator new(size_t size) {
		return uni_hostenv_allocator->allocate(sizeof(HIDKeyboardDriver));
	}

	void HIDKeyboardDriver::operator delete(void* ptr) noexcept {
		uni_hostenv_allocator->deallocate(ptr);
	}
}
namespace uni::device::SpaceUSB {
	HIDKeyboardDriver::HIDKeyboardDriver(DeviceUSB* dev, int interface_index)
		: HIDBaseDriver{ dev, interface_index, 8 } {
	}

	Error HIDKeyboardDriver::OnDataReceived() {
		keyboard_event_t keyevent = {};
		keyevent.mod_val = Buffer()[0];
		if (Buffer()[0] != PreviousBuffer()[0]) {
			NotifyKeyPush(keyevent);
		}
		keyevent.method = keyboard_event_t::method_t::keyup;
		const auto& last_buf = Buffer();
		const auto& prev_buf = PreviousBuffer();
		for (stduint i = 2; i < 8; ++i) {
			if (prev_buf[i] && std::find(last_buf.begin(), last_buf.end(), prev_buf[i]) == last_buf.end()) {
				keyevent.keycode = prev_buf[i];
				NotifyKeyPush(keyevent);
			}
		}
		keyevent.method = keyboard_event_t::method_t::keydown;
		for (stduint i = 2; i < 8; ++i) {
			if (last_buf[i] && std::find(prev_buf.begin(), prev_buf.end(), last_buf[i]) == prev_buf.end()) {
				keyevent.keycode = last_buf[i];
				NotifyKeyPush(keyevent);
			}
		}
		return MAKE_ERROR(Error::kSuccess);
	}

	void HIDKeyboardDriver::SubscribeKeyPush(
		std::function<void(keyboard_event_t)> observer) {
		observers_[num_observers_++] = observer;
	}

	std::function<HIDKeyboardDriver::ObserverType> HIDKeyboardDriver::default_observer;

	void HIDKeyboardDriver::NotifyKeyPush(keyboard_event_t keyevent) {
		for (int i = 0; i < num_observers_; ++i) {
			observers_[i](keyevent);
		}
	}
}
#endif
