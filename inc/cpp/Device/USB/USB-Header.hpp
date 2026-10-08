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

#include "../../ISO_IEC_STD/array"
#include "../../ISO_IEC_STD/optional"
#include "../../ISO_IEC_STD/pair"
#include "../../vector"

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

	enum class StandardRequest : uint8 {
		GetStatus = 0,
		ClearFeature = 1,
		SetFeature = 3,
		SetAddress = 5,
		GetDescriptor = 6,
		SetDescriptor = 7,
		GetConfiguration = 8,
		SetConfiguration = 9,
		GetInterface = 10,
		SetInterface = 11,
		SynchFrame = 12,
		SetEncryption = 13,
		GetEncryption = 14,
		SetHandshake = 15,
		GetHandshake = 16,
		SetConnection = 17,
		SetSecurityData = 18,
		GetSecurityData = 19,
		SetWUSBData = 20,
		LoopbackDataWrite = 21,
		LoopbackDataRead = 22,
		SetInterfaceDS = 23,
		SetSel = 48,
		SetIsochDelay = 49,
	};

	enum class HIDRequest : uint8 {
		GetReport = 1,
		SetReport = 9,
		SetProtocol = 11,
	};

	enum class HubRequest : uint8 {
		ClearTTBuffer = 8,
		SetHubDepth = 12,
		GetPortErrorCount = 13,
	};

	enum class DeviceCapabilityType : uint8 {
		USB20Extension = 2,
		SuperSpeedUSB = 3,
		ContainerID = 4,
		SuperSpeedPlusUSB = 10,
	};

	enum class HubProtocol : uint8 {
		FullSpeed = 0,
		HighSpeedSingleTT = 1,
		HighSpeedMultipleTT = 2,
		SuperSpeed = 3,
	};

	enum class HubPortStatusType : uint16 {
		Standard = 0,
		PowerDelivery = 1,
		Extended = 2,
	};

	namespace descriptor_type {
		const int kDevice = 1;
		const int kConfiguration = 2;
		const int kString = 3;
		const int kInterface = 4;
		const int kEndpoint = 5;
		const int kHub = 0x29;
		const int kSuperSpeedHub = 0x2a;
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

		/** Construct an ID from endpoint number and direction.
			 *
			 * ep_num is an integer in 0..15.
			 * dir_in must always be true for Control endpoints.
			 */
		constexpr EndpointID(int ep_num, bool dir_in) : addr_{ ep_num << 1 | dir_in } {}

		EndpointID& operator =(const EndpointID& rhs) {
			addr_ = rhs.addr_;
			return *this;
		}

		/** Endpoint address (0..31) */
		int Address() const { return addr_; }

		/** Endpoint number (0..15) */
		int Number() const { return addr_ >> 1; }

		/** I/O direction. Control endpoints are true */
		bool IsIn() const { return addr_ & 1; }

	private:
		int addr_;
	};

	constexpr EndpointID kDefaultControlPipeID{ 0, true };
	// AKA USBH_DEVICE_ADDRESS: the address the host assigns first
	constexpr uint8 kDefaultDeviceAddress = 1;

	struct EndpointConfig {
	  /** Endpoint ID */
		EndpointID ep_id;

		/** Type of this endpoint */
		EndpointType ep_type;

		/** Maximum packet size of this endpoint (bytes) */
		int max_packet_size;

		/** Polling interval of this endpoint (125*2^(interval-1) microseconds) */
		int interval;

		/** SuperSpeed Endpoint Companion bMaxBurst. */
		uint8 max_burst;

		/** SuperSpeed isochronous Mult (zero based). */
		uint8 mult;

		/** Bulk MaxStreams exponent advertised by the companion descriptor. */
		uint8 max_streams;

		/** Maximum bytes transferred during one service interval. */
		uint32 bytes_per_interval;
	};

	struct IsochronousTransferOptions {
		// SIA asks the controller to schedule the TD at the next service opportunity.
		bool schedule_immediately = true;
		// When SIA is clear, the host can derive the next valid Frame ID from MFINDEX.
		bool automatic_frame_id = true;
		uint16 frame_id = 0;
	};
}

// ---- ---- ---- ---- arraymap.hpp ---- ---- ---- ---- //


namespace uni::device::SpaceUSB {
	template <class K, class V, size_t N = 16>
	class ArrayMap {
	public:
		uni::Optional<V> Get(const K& key) const {
			for (size_t i = 0; i < table_.size(); ++i) {
				if (auto opt_k = table_[i].first; opt_k && opt_k.value() == key) {
					return table_[i].second;
				}
			}
			return uni::nullopt;
		}

		bool Insert(const K& key, const V& value) {
			for (size_t i = 0; i < table_.size(); ++i) {
				if (auto opt_k = table_[i].first; opt_k && opt_k.value() == key) {
					return false;
				}
			}
			for (size_t i = 0; i < table_.size(); ++i) {
				if (!table_[i].first) {
					table_[i].first = key;
					table_[i].second = value;
					return true;
				}
			}
			return false;
		}

		bool Put(const K& key, const V& value) {
			for (size_t i = 0; i < table_.size(); ++i) {
				if (auto opt_k = table_[i].first; opt_k && opt_k.value() == key) {
					table_[i].second = value;
					return true;
				}
			}
			return Insert(key, value);
		}

		void Delete(const K& key) {
			for (size_t i = 0; i < table_.size(); ++i) {
				if (auto opt_k = table_[i].first; opt_k && opt_k.value() == key) {
					table_[i].first = uni::nullopt;
					break;
				}
			}
		}

		bool IsFull() const {
			for (size_t i = 0; i < table_.size(); ++i) {
				if (!table_[i].first) return false;
			}
			return true;
		}

		void Clear() {
			for (size_t i = 0; i < table_.size(); ++i) {
				table_[i].first = uni::nullopt;
			}
		}

	private:
		uni::Array<uni::Pair<uni::Optional<K>, V>, N> table_{};
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

	struct BOSDescriptor {
		static const uint8_t kType = descriptor_type::kBOS;

		uint8_t length;
		uint8_t descriptor_type;
		uint16_t total_length;
		uint8_t num_device_capabilities;
	} __attribute__((packed));

	struct DeviceCapabilityDescriptor {
		static const uint8_t kType = descriptor_type::kDeviceCapability;

		uint8_t length;
		uint8_t descriptor_type;
		uint8_t capability_type;
	} __attribute__((packed));

	struct USB20ExtensionCapabilityDescriptor {
		static const uint8_t kType = descriptor_type::kDeviceCapability;

		uint8_t length;
		uint8_t descriptor_type;
		uint8_t capability_type;
		uint32_t attributes;
	} __attribute__((packed));

	struct SuperSpeedUSBCapabilityDescriptor {
		static const uint8_t kType = descriptor_type::kDeviceCapability;

		uint8_t length;
		uint8_t descriptor_type;
		uint8_t capability_type;
		uint8_t attributes;
		uint16_t speeds_supported;
		uint8_t functionality_support;
		uint8_t u1_device_exit_latency;
		uint16_t u2_device_exit_latency;
	} __attribute__((packed));

	struct SuperSpeedPlusUSBCapabilityDescriptor {
		static const uint8_t kType = descriptor_type::kDeviceCapability;

		uint8_t length;
		uint8_t descriptor_type;
		uint8_t capability_type;
		uint8_t reserved;
		uint32_t attributes;
		uint16_t functionality_support;
		uint16_t reserved2;
	} __attribute__((packed));

	union SuperSpeedPlusSublinkSpeedAttribute {
		uint32_t data;
		struct {
			uint32_t id : 4;
			uint32_t lane_speed_exponent : 2;
			uint32_t asymmetric : 1;
			uint32_t transmit : 1;
			uint32_t reserved : 6;
			uint32_t link_protocol : 2;
			uint32_t lane_speed_mantissa : 16;
		} __attribute__((packed)) bits;
	} __attribute__((packed));

	struct ContainerIDCapabilityDescriptor {
		static const uint8_t kType = descriptor_type::kDeviceCapability;

		uint8_t length;
		uint8_t descriptor_type;
		uint8_t capability_type;
		uint8_t reserved;
		uint8_t container_id[16];
	} __attribute__((packed));

	static_assert(sizeof(BOSDescriptor) == 5,
		"BOS descriptor must be 5 bytes");
	static_assert(sizeof(USB20ExtensionCapabilityDescriptor) == 7,
		"USB 2.0 extension capability descriptor must be 7 bytes");
	static_assert(sizeof(SuperSpeedUSBCapabilityDescriptor) == 10,
		"SuperSpeed USB capability descriptor must be 10 bytes");
	static_assert(sizeof(SuperSpeedPlusUSBCapabilityDescriptor) == 12,
		"SuperSpeedPlus USB capability descriptor header must be 12 bytes");
	static_assert(sizeof(SuperSpeedPlusSublinkSpeedAttribute) == 4,
		"SuperSpeedPlus sublink speed attribute must be 4 bytes");
	static_assert(sizeof(ContainerIDCapabilityDescriptor) == 20,
		"Container ID capability descriptor must be 20 bytes");

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

	struct SuperSpeedEndpointCompanionDescriptor {
		static const uint8_t kType = descriptor_type::kSuperspeedUSBEndpointCompanion;

		uint8_t length;
		uint8_t descriptor_type;
		uint8_t max_burst;
		uint8_t attributes;
		uint16_t bytes_per_interval;
	} __attribute__((packed));

	struct SuperSpeedPlusIsochronousEndpointCompanionDescriptor {
		static const uint8_t kType = descriptor_type::kSuperspeedPlusIsochronousEndpointCompanion;

		uint8_t length;
		uint8_t descriptor_type;
		uint16_t reserved;
		uint32_t bytes_per_interval;
	} __attribute__((packed));

	static_assert(sizeof(SuperSpeedEndpointCompanionDescriptor) == 6,
		"SuperSpeed endpoint companion descriptor must be 6 bytes");
	static_assert(sizeof(SuperSpeedPlusIsochronousEndpointCompanionDescriptor) == 8,
		"SuperSpeedPlus isochronous endpoint companion descriptor must be 8 bytes");

	struct HubDescriptor {
		static const uint8_t kType = descriptor_type::kHub;

		uint8_t length;
		uint8_t descriptor_type;
		uint8_t num_ports;
		uint16_t characteristics;
		uint8_t power_on_to_power_good;
		uint8_t hub_control_current;
	} __attribute__((packed));

	struct SuperSpeedHubDescriptor {
		static const uint8_t kType = descriptor_type::kSuperSpeedHub;

		uint8_t length;
		uint8_t descriptor_type;
		uint8_t num_ports;
		uint16_t characteristics;
		uint8_t power_on_to_power_good;
		uint8_t hub_control_current;
		uint8_t hub_header_decode_latency;
		uint16_t hub_delay;
		uint16_t device_removable;
	} __attribute__((packed));

	static_assert(sizeof(SuperSpeedHubDescriptor) == 12,
		"SuperSpeed hub descriptor must be 12 bytes");

	struct HubPortStatus {
		uint16_t status;
		uint16_t change;
	} __attribute__((packed));

	struct ExtendedHubPortStatus {
		uint16_t status;
		uint16_t change;
		uint32_t extended_status;
	} __attribute__((packed));

	static_assert(sizeof(ExtendedHubPortStatus) == 8,
		"Extended hub port status must be 8 bytes");

	struct HIDDescriptor {
		static const uint8_t kType = 33;

		uint8_t length;             // offset 0
		uint8_t descriptor_type;    // offset 1
		uint16_t hid_release;       // offset 2
		uint8_t country_code;       // offset 4
		uint8_t num_descriptors;    // offset 5

		struct ClassDescriptor {
		  /** @brief Type value of the class-specific descriptor. */
			uint8_t descriptor_type;
			/** @brief Byte count of the class-specific descriptor. */
			uint16_t descriptor_length;
		} __attribute__((packed));

		/** @brief Get information about HID-specific descriptors.
			 *
			 * HID has one or more class-specific descriptors.
			 * The count is stored in num_descriptors.
			 * A Report descriptor (type = 34) must exist for any HID device,
			 * so num_descriptors is always at least 1.
			 *
			 * @param index  Descriptor number to retrieve. 0 <= index < num_descriptors.
			 * @return Information of the descriptor specified by index. nullptr if index is out of range.
			 */
		ClassDescriptor* GetClassDescriptor(size_t index) const {
			if (index >= num_descriptors) {
				return nullptr;
			}
			const auto end_of_struct =
				reinterpret_cast<stduint>(this) + sizeof(HIDDescriptor);
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
