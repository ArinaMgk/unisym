#include <stdlib.h>
#include "../../../inc/c/driver/keyboard.h"
#include "../../../inc/c/msgface.h"
#include <algorithm>


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
		for (int i = 2; i < 8; ++i) {
			const uint8_t key = Buffer()[i];
			if (key == 0) {
				continue;
			}
			const auto& prev_buf = PreviousBuffer();
			if (std::find(prev_buf.begin(), prev_buf.end(), key) != prev_buf.end()) {
				continue;
			}
			NotifyKeyPush(key);
		}
		return MAKE_ERROR(Error::kSuccess);
	}

	void HIDKeyboardDriver::SubscribeKeyPush(
		std::function<void(uint8_t keycode)> observer) {
		observers_[num_observers_++] = observer;
	}

	std::function<HIDKeyboardDriver::ObserverType> HIDKeyboardDriver::default_observer;

	void HIDKeyboardDriver::NotifyKeyPush(uint8_t keycode) {
		for (int i = 0; i < num_observers_; ++i) {
			observers_[i](keycode);
		}
	}
}
#endif
