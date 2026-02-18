// UTF-8 CPP-ISO11 TAB4 CRLF
// Docutitle: (Device) USB
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
// Copyright: uchan-nos/mikanos, under Apache License 2.0
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

/*
┌─────────────────────────────┐
│  Applications / OS Services │
├─────────────────────────────┤
│  USB Class Drivers          │  ← Device-specific functionality
├─────────────────────────────┤
│  USB Core Framework         │  ← Protocol abstraction layer
├─────────────────────────────┤
│  xHCI Driver                │  ← USB 3.0+ Host Controller Driver
├─────────────────────────────┤
│  PCI/PCIe Driver            │  ← Bus enumeration and communication (inc\cpp\Device\Bus\PCI.hpp)
├─────────────────────────────┤
│  xHCI Hardware Controller   │  ← Physical hardware
└─────────────────────────────┘
*/

#ifndef _INCPP_Device_USB_Header
#define _INCPP_Device_USB_Header
#include "../../unisym"
#include "../../../c/consio.h"
#include "../../../cpp/trait/MallocTrait.hpp"

#if (defined(_MCCA) && ((_MCCA & 0xFF00)==0x8600))

#include <array>
#include <optional>

// ---- ---- ---- ---- logger.hpp ---- ---- ---- ---- //

enum LogLevel {
	kError = 3,
	kWarn = 4,
	kInfo = 6,
	kDebug = 7,
};

inline static void SetLogLevel(LogLevel level) {}

inline static int Log(LogLevel level, const char* fmt, ...) {
	if (level > 4) return 0;
	Letpara(paras, fmt);
	outsfmt("Log%d:", _IMM(level));
	printlogx(loglevel_t::_LOG_INFO, fmt, paras);
	return 0;
}

#include "./xHCI/xHCI-Error.hpp"



// ---- ---- ---- ---- setupdata.hpp ---- ---- ---- ---- //


namespace uni::device::SpaceUSB {
	namespace request_type {
	  // bmRequestType recipient
		const int kDevice = 0;
		const int kInterface = 1;
		const int kEndpoint = 2;
		const int kOther = 3;

		// bmRequestType type
		const int kStandard = 0;
		const int kClass = 1;
		const int kVendor = 2;

		// bmRequestType direction
		const int kOut = 0;
		const int kIn = 1;
	}

	namespace request {
		const int kGetStatus = 0;
		const int kClearFeature = 1;
		const int kSetFeature = 3;
		const int kSetAddress = 5;
		const int kGetDescriptor = 6;
		const int kSetDescriptor = 7;
		const int kGetConfiguration = 8;
		const int kSetConfiguration = 9;
		const int kGetInterface = 10;
		const int kSetInterface = 11;
		const int kSynchFrame = 12;
		const int kSetEncryption = 13;
		const int kGetEncryption = 14;
		const int kSetHandshake = 15;
		const int kGetHandshake = 16;
		const int kSetConnection = 17;
		const int kSetSecurityData = 18;
		const int kGetSecurityData = 19;
		const int kSetWUSBData = 20;
		const int kLoopbackDataWrite = 21;
		const int kLoopbackDataRead = 22;
		const int kSetInterfaceDS = 23;
		const int kSetSel = 48;
		const int kSetIsochDelay = 49;

		// HID class specific report values
		const int kGetReport = 1;
		const int kSetProtocol = 11;
	}

	namespace descriptor_type {
		const int kDevice = 1;
		const int kConfiguration = 2;
		const int kString = 3;
		const int kInterface = 4;
		const int kEndpoint = 5;
		const int kInterfacePower = 8;
		const int kOTG = 9;
		const int kDebug = 10;
		const int kInterfaceAssociation = 11;
		const int kBOS = 15;
		const int kDeviceCapability = 16;
		const int kHID = 33;
		const int kSuperspeedUSBEndpointCompanion = 48;
		const int kSuperspeedPlusIsochronousEndpointCompanion = 49;
	}

	struct SetupData {
		union {
			uint8_t data;
			struct {
				uint8_t recipient : 5;
				uint8_t type : 2;
				uint8_t direction : 1;
			} bits;
		} request_type;
		uint8_t request;
		uint16_t value;
		uint16_t index;
		uint16_t length;
	} __attribute__((packed));

	inline bool operator ==(SetupData lhs, SetupData rhs) {
		return
			lhs.request_type.data == rhs.request_type.data &&
			lhs.request == rhs.request &&
			lhs.value == rhs.value &&
			lhs.index == rhs.index &&
			lhs.length == rhs.length;
	}
}

// ---- ---- ---- ---- endpoint.hpp ---- ---- ---- ---- //

namespace uni::device::SpaceUSB {
	enum class EndpointType {
		kControl = 0,
		kIsochronous = 1,
		kBulk = 2,
		kInterrupt = 3,
	};

	class EndpointID {
	public:
		constexpr EndpointID() : addr_{ 0 } {}
		constexpr EndpointID(const EndpointID& ep_id) : addr_{ ep_id.addr_ } {}
		explicit constexpr EndpointID(int addr) : addr_{ addr } {}

		/** エンドポイント番号と入出力方向から ID を構成する．
		 *
		 * ep_num は 0..15 の整数．
		 * dir_in は Control エンドポイントでは常に true にしなければならない．
		 */
		constexpr EndpointID(int ep_num, bool dir_in) : addr_{ ep_num << 1 | dir_in } {}

		EndpointID& operator =(const EndpointID& rhs) {
			addr_ = rhs.addr_;
			return *this;
		}

		/** エンドポイントアドレス（0..31） */
		int Address() const { return addr_; }

		/** エンドポイント番号（0..15） */
		int Number() const { return addr_ >> 1; }

		/** 入出力方向．Control エンドポイントは true */
		bool IsIn() const { return addr_ & 1; }

	private:
		int addr_;
	};

	constexpr EndpointID kDefaultControlPipeID{ 0, true };

	struct EndpointConfig {
	  /** エンドポイント ID */
		EndpointID ep_id;

		/** このエンドポイントの種別 */
		EndpointType ep_type;

		/** このエンドポイントの最大パケットサイズ（バイト） */
		int max_packet_size;

		/** このエンドポイントの制御周期（125*2^(interval-1) マイクロ秒） */
		int interval;
	};
}

// ---- ---- ---- ---- arraymap.hpp ---- ---- ---- ---- //


namespace uni::device::SpaceUSB {
	template <class K, class V, size_t N = 16>
	class ArrayMap {
	public:
		std::optional<V> Get(const K& key) const {
			for (int i = 0; i < table_.size(); ++i) {
				if (auto opt_k = table_[i].first; opt_k && opt_k.value() == key) {
					return table_[i].second;
				}
			}
			return std::nullopt;
		}

		void Put(const K& key, const V& value) {
			for (int i = 0; i < table_.size(); ++i) {
				if (!table_[i].first) {
					table_[i].first = key;
					table_[i].second = value;
					break;
				}
			}
		}

		void Delete(const K& key) {
			for (int i = 0; i < table_.size(); ++i) {
				if (auto opt_k = table_[i].first; opt_k && opt_k.value() == key) {
					table_[i].first = std::nullopt;
					break;
				}
			}
		}

	private:
		std::array<std::pair<std::optional<K>, V>, N> table_{};
	};
}


// ---- ---- ---- ---- descriptor.hpp ---- ---- ---- ---- //


namespace uni::device::SpaceUSB {
	struct DeviceDescriptor {
		static const uint8_t kType = 1;

		uint8_t length;             // offset 0
		uint8_t descriptor_type;    // offset 1
		uint16_t usb_release;       // offset 2
		uint8_t device_class;       // offset 4
		uint8_t device_sub_class;   // offset 5
		uint8_t device_protocol;    // offset 6
		uint8_t max_packet_size;    // offset 7
		uint16_t vendor_id;         // offset 8
		uint16_t product_id;        // offset 10
		uint16_t device_release;    // offset 12
		uint8_t manufacturer;       // offset 14
		uint8_t product;            // offset 15
		uint8_t serial_number;      // offset 16
		uint8_t num_configurations; // offset 17
	} __attribute__((packed));

	struct ConfigurationDescriptor {
		static const uint8_t kType = 2;

		uint8_t length;             // offset 0
		uint8_t descriptor_type;    // offset 1
		uint16_t total_length;      // offset 2
		uint8_t num_interfaces;     // offset 4
		uint8_t configuration_value;// offset 5
		uint8_t configuration_id;   // offset 6
		uint8_t attributes;         // offset 7
		uint8_t max_power;          // offset 8
	} __attribute__((packed));

	struct InterfaceDescriptor {
		static const uint8_t kType = 4;

		uint8_t length;             // offset 0
		uint8_t descriptor_type;    // offset 1
		uint8_t interface_number;   // offset 2
		uint8_t alternate_setting;  // offset 3
		uint8_t num_endpoints;      // offset 4
		uint8_t interface_class;    // offset 5
		uint8_t interface_sub_class;// offset 6
		uint8_t interface_protocol; // offset 7
		uint8_t interface_id;       // offset 8
	} __attribute__((packed));

	struct EndpointDescriptor {
		static const uint8_t kType = 5;

		uint8_t length;             // offset 0
		uint8_t descriptor_type;    // offset 1
		union {
			uint8_t data;
			struct {
				uint8_t number : 4;
				uint8_t : 3;
				uint8_t dir_in : 1;
			} __attribute__((packed)) bits;
		} endpoint_address;         // offset 2
		union {
			uint8_t data;
			struct {
				uint8_t transfer_type : 2;
				uint8_t sync_type : 2;
				uint8_t usage_type : 2;
				uint8_t : 2;
			} __attribute__((packed)) bits;
		} attributes;               // offset 3
		uint16_t max_packet_size;   // offset 4
		uint8_t interval;           // offset 6
	} __attribute__((packed));

	struct HIDDescriptor {
		static const uint8_t kType = 33;

		uint8_t length;             // offset 0
		uint8_t descriptor_type;    // offset 1
		uint16_t hid_release;       // offset 2
		uint8_t country_code;       // offset 4
		uint8_t num_descriptors;    // offset 5

		struct ClassDescriptor {
		  /** @brief クラス特有ディスクリプタのタイプ値． */
			uint8_t descriptor_type;
			/** @brief クラス特有ディスクリプタのバイト数． */
			uint16_t descriptor_length;
		} __attribute__((packed));

		/** @brief HID 特有のディスクリプタに関する情報を得る．
		 *
		 * HID はクラス特有（class-specific）のディスクリプタを 1 つ以上持つ．
		 * その数は num_descriptors に記載されている．
		 * Report ディスクリプタ（type = 34）は HID デバイスであれば必ず存在するため，
		 * num_descriptors は必ず 1 以上となる．
		 *
		 * @param index  取得するディスクリプタの番号．0 <= index < num_descriptors.
		 * @return index で指定されたディスクリプタの情報．index が範囲外なら nullptr.
		 */
		ClassDescriptor* GetClassDescriptor(size_t index) const {
			if (index >= num_descriptors) {
				return nullptr;
			}
			const auto end_of_struct =
				reinterpret_cast<uintptr_t>(this) + sizeof(HIDDescriptor);
			return reinterpret_cast<ClassDescriptor*>(end_of_struct) + index;
		}
	} __attribute__((packed));

	template <class T>
	T* DescriptorDynamicCast(uint8_t* desc_data) {
		if (desc_data[1] == T::kType) {
			return reinterpret_cast<T*>(desc_data);
		}
		return nullptr;
	}

	template <class T>
	const T* DescriptorDynamicCast(const uint8_t* desc_data) {
		if (desc_data[1] == T::kType) {
			return reinterpret_cast<const T*>(desc_data);
		}
		return nullptr;
	}
}

namespace uni::device::SpaceUSB3 {
	const int kFullSpeed = 1;
	const int kLowSpeed = 2;
	const int kHighSpeed = 3;
	const int kSuperSpeed = 4;
	const int kSuperSpeedPlus = 5;
}

extern
::uni::trait::Malloc* uni_hostenv_allocator;

#endif

#endif
