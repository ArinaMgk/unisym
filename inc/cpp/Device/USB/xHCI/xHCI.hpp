// UTF-8 CPP-ISO11 TAB4 CRLF
// Docutitle: (Device) USB xHCI
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

#ifndef _INCPP_Device_USB_xHCI
#define _INCPP_Device_USB_xHCI

// x86_64 part of USB Driver is borrowed from uchan-nos/mikanos
// - https://github.com/uchan-nos/mikanos
// cited sincerely.
#if (defined(_MCCA) && ((_MCCA & 0xFF00)==0x8600))
#include <setjmp.h>
#include <iterator>
#include <vector>
#include "../USB-Header.hpp"

// ---- ---- ---- ---- context.hpp ---- ---- ---- ---- //


namespace usb::xhci {
	class Ring;
	union TRB;

	union SlotContext {
		uint32_t dwords[8];
		struct {
			uint32_t route_string : 20;
			uint32_t speed : 4;
			uint32_t : 1; // reserved
			uint32_t mtt : 1;
			uint32_t hub : 1;
			uint32_t context_entries : 5;

			uint32_t max_exit_latency : 16;
			uint32_t root_hub_port_num : 8;
			uint32_t num_ports : 8;

			// TT : Transaction Translator
			uint32_t tt_hub_slot_id : 8;
			uint32_t tt_port_num : 8;
			uint32_t ttt : 2;
			uint32_t : 4; // reserved
			uint32_t interrupter_target : 10;

			uint32_t usb_device_address : 8;
			uint32_t : 19;
			uint32_t slot_state : 5;
		} __attribute__((packed)) bits;
	} __attribute__((packed));

	union EndpointContext {
		uint32_t dwords[8];
		struct {
			uint32_t ep_state : 3;
			uint32_t : 5;
			uint32_t mult : 2;
			uint32_t max_primary_streams : 5;
			uint32_t linear_stream_array : 1;
			uint32_t interval : 8;
			uint32_t max_esit_payload_hi : 8;

			uint32_t : 1;
			uint32_t error_count : 2;
			uint32_t ep_type : 3;
			uint32_t : 1;
			uint32_t host_initiate_disable : 1;
			uint32_t max_burst_size : 8;
			uint32_t max_packet_size : 16;

			uint32_t dequeue_cycle_state : 1;
			uint32_t : 3;
			uint64_t tr_dequeue_pointer : 60;

			uint32_t average_trb_length : 16;
			uint32_t max_esit_payload_lo : 16;
		} __attribute__((packed)) bits;

		TRB* TransferRingBuffer() const {
			return reinterpret_cast<TRB*>(bits.tr_dequeue_pointer << 4);
		}

		void SetTransferRingBuffer(TRB* buffer) {
			bits.tr_dequeue_pointer = reinterpret_cast<uint64_t>(buffer) >> 4;
		}
	} __attribute__((packed));

	struct DeviceContextIndex {
		int value;

		explicit DeviceContextIndex(int dci) : value{ dci } {}
		DeviceContextIndex(EndpointID ep_id) : value{ ep_id.Address() } {}

		DeviceContextIndex(int ep_num, bool dir_in)
			: value{ 2 * ep_num + (ep_num == 0 ? 1 : dir_in) } {
		}

		DeviceContextIndex(const DeviceContextIndex& rhs) = default;
		DeviceContextIndex& operator =(const DeviceContextIndex& rhs) = default;
	};

	struct DeviceContext {
		SlotContext slot_context;
		EndpointContext ep_contexts[31];
	} __attribute__((packed));

	struct InputControlContext {
		uint32_t drop_context_flags;
		uint32_t add_context_flags;
		uint32_t reserved1[5];
		uint8_t configuration_value;
		uint8_t interface_number;
		uint8_t alternate_setting;
		uint8_t reserved2;
	} __attribute__((packed));

	struct InputContext {
		InputControlContext input_control_context;
		SlotContext slot_context;
		EndpointContext ep_contexts[31];

		/** @brief Enable the slot context.
		 *
		 * @return Pointer to the slot context enabled.
		 */
		SlotContext* EnableSlotContext() {
			input_control_context.add_context_flags |= 1;
			return &slot_context;
		}

		/** @brief Enable an endpoint.
		 *
		 * @param dci Device Context Index (1 .. 31)
		 * @return Pointer to the endpoint context enabled.
		 */
		EndpointContext* EnableEndpoint(DeviceContextIndex dci) {
			input_control_context.add_context_flags |= 1u << dci.value;
			return &ep_contexts[dci.value - 1];
		}
	} __attribute__((packed));
}

// ---- ---- ---- ---- trb.hpp ---- ---- ---- ---- //

namespace usb::xhci {
	extern const std::array<const char*, 37> kTRBCompletionCodeToName;
	extern const std::array<const char*, 64> kTRBTypeToName;

	union TRB {
		std::array<uint32_t, 4> data{};
		struct {
			uint64_t parameter;
			uint32_t status;
			uint32_t cycle_bit : 1;
			uint32_t evaluate_next_trb : 1;
			uint32_t : 8;
			uint32_t trb_type : 6;
			uint32_t control : 16;
		} __attribute__((packed)) bits;
	};

	union NormalTRB {
		static const unsigned int Type = 1;
		std::array<uint32_t, 4> data{};
		struct {
			uint64_t data_buffer_pointer;

			uint32_t trb_transfer_length : 17;
			uint32_t td_size : 5;
			uint32_t interrupter_target : 10;

			uint32_t cycle_bit : 1;
			uint32_t evaluate_next_trb : 1;
			uint32_t interrupt_on_short_packet : 1;
			uint32_t no_snoop : 1;
			uint32_t chain_bit : 1;
			uint32_t interrupt_on_completion : 1;
			uint32_t immediate_data : 1;
			uint32_t : 2;
			uint32_t block_event_interrupt : 1;
			uint32_t trb_type : 6;
			uint32_t : 16;
		} __attribute__((packed)) bits;

		NormalTRB() {
			bits.trb_type = Type;
		}

		void* Pointer() const {
			return reinterpret_cast<TRB*>(bits.data_buffer_pointer);
		}

		void SetPointer(const void* p) {
			bits.data_buffer_pointer = reinterpret_cast<uint64_t>(p);
		}
	};

	union SetupStageTRB {
		static const unsigned int Type = 2;
		static const unsigned int kNoDataStage = 0;
		static const unsigned int kOutDataStage = 2;
		static const unsigned int kInDataStage = 3;

		std::array<uint32_t, 4> data{};
		struct {
			uint32_t request_type : 8;
			uint32_t request : 8;
			uint32_t value : 16;

			uint32_t index : 16;
			uint32_t length : 16;

			uint32_t trb_transfer_length : 17;
			uint32_t : 5;
			uint32_t interrupter_target : 10;

			uint32_t cycle_bit : 1;
			uint32_t : 4;
			uint32_t interrupt_on_completion : 1;
			uint32_t immediate_data : 1;
			uint32_t : 3;
			uint32_t trb_type : 6;
			uint32_t transfer_type : 2;
			uint32_t : 14;
		} __attribute__((packed)) bits;

		SetupStageTRB() {
			bits.trb_type = Type;
			bits.immediate_data = true;
			bits.trb_transfer_length = 8;
		}
	};

	union DataStageTRB {
		static const unsigned int Type = 3;
		std::array<uint32_t, 4> data{};
		struct {
			uint64_t data_buffer_pointer;

			uint32_t trb_transfer_length : 17;
			uint32_t td_size : 5;
			uint32_t interrupter_target : 10;

			uint32_t cycle_bit : 1;
			uint32_t evaluate_next_trb : 1;
			uint32_t interrupt_on_short_packet : 1;
			uint32_t no_snoop : 1;
			uint32_t chain_bit : 1;
			uint32_t interrupt_on_completion : 1;
			uint32_t immediate_data : 1;
			uint32_t : 3;
			uint32_t trb_type : 6;
			uint32_t direction : 1;
			uint32_t : 15;
		} __attribute__((packed)) bits;

		DataStageTRB() {
			bits.trb_type = Type;
		}

		void* Pointer() const {
			return reinterpret_cast<void*>(bits.data_buffer_pointer);
		}

		void SetPointer(const void* p) {
			bits.data_buffer_pointer = reinterpret_cast<uint64_t>(p);
		}
	};

	union StatusStageTRB {
		static const unsigned int Type = 4;
		std::array<uint32_t, 4> data{};
		struct {
			uint64_t : 64;

			uint32_t : 22;
			uint32_t interrupter_target : 10;

			uint32_t cycle_bit : 1;
			uint32_t evaluate_next_trb : 1;
			uint32_t : 2;
			uint32_t chain_bit : 1;
			uint32_t interrupt_on_completion : 1;
			uint32_t : 4;
			uint32_t trb_type : 6;
			uint32_t direction : 1;
			uint32_t : 15;
		} __attribute__((packed)) bits;

		StatusStageTRB() {
			bits.trb_type = Type;
		}
	};

	union LinkTRB {
		static const unsigned int Type = 6;
		std::array<uint32_t, 4> data{};
		struct {
			uint64_t : 4;
			uint64_t ring_segment_pointer : 60;

			uint32_t : 22;
			uint32_t interrupter_target : 10;

			uint32_t cycle_bit : 1;
			uint32_t toggle_cycle : 1;
			uint32_t : 2;
			uint32_t chain_bit : 1;
			uint32_t interrupt_on_completion : 1;
			uint32_t : 4;
			uint32_t trb_type : 6;
			uint32_t : 16;
		} __attribute__((packed)) bits;

		LinkTRB(const TRB* ring_segment_pointer) {
			bits.trb_type = Type;
			SetPointer(ring_segment_pointer);
		}

		TRB* Pointer() const {
			return reinterpret_cast<TRB*>(bits.ring_segment_pointer << 4);
		}

		void SetPointer(const TRB* p) {
			bits.ring_segment_pointer = reinterpret_cast<uint64_t>(p) >> 4;
		}
	};

	union NoOpTRB {
		static const unsigned int Type = 8;
		std::array<uint32_t, 4> data{};
		struct {
			uint64_t : 64;

			uint32_t : 22;
			uint32_t interrupter_target : 10;

			uint32_t cycle_bit : 1;
			uint32_t evaluate_next_trb : 1;
			uint32_t : 2;
			uint32_t chain_bit : 1;
			uint32_t interrupt_on_completion : 1;
			uint32_t : 4;
			uint32_t trb_type : 6;
			uint32_t : 16;
		} __attribute__((packed)) bits;

		NoOpTRB() {
			bits.trb_type = Type;
		}
	};

	union EnableSlotCommandTRB {
		static const unsigned int Type = 9;
		std::array<uint32_t, 4> data{};
		struct {
			uint32_t : 32;

			uint32_t : 32;

			uint32_t : 32;

			uint32_t cycle_bit : 1;
			uint32_t : 9;
			uint32_t trb_type : 6;
			uint32_t slot_type : 5;
			uint32_t : 11;
		} __attribute__((packed)) bits;

		EnableSlotCommandTRB() {
			bits.trb_type = Type;
		}
	};

	union AddressDeviceCommandTRB {
		static const unsigned int Type = 11;
		std::array<uint32_t, 4> data{};
		struct {
			uint64_t : 4;
			uint64_t input_context_pointer : 60;

			uint32_t : 32;

			uint32_t cycle_bit : 1;
			uint32_t : 8;
			uint32_t block_set_address_request : 1;
			uint32_t trb_type : 6;
			uint32_t : 8;
			uint32_t slot_id : 8;
		} __attribute__((packed)) bits;

		AddressDeviceCommandTRB(const InputContext* input_context, uint8_t slot_id) {
			bits.trb_type = Type;
			bits.slot_id = slot_id;
			SetPointer(input_context);
		}

		InputContext* Pointer() const {
			return reinterpret_cast<InputContext*>(bits.input_context_pointer << 4);
		}

		void SetPointer(const InputContext* p) {
			bits.input_context_pointer = reinterpret_cast<uint64_t>(p) >> 4;
		}
	};

	union ConfigureEndpointCommandTRB {
		static const unsigned int Type = 12;
		std::array<uint32_t, 4> data{};
		struct {
			uint64_t : 4;
			uint64_t input_context_pointer : 60;

			uint32_t : 32;

			uint32_t cycle_bit : 1;
			uint32_t : 8;
			uint32_t deconfigure : 1;
			uint32_t trb_type : 6;
			uint32_t : 8;
			uint32_t slot_id : 8;
		} __attribute__((packed)) bits;

		ConfigureEndpointCommandTRB(const InputContext* input_context, uint8_t slot_id) {
			bits.trb_type = Type;
			bits.slot_id = slot_id;
			SetPointer(input_context);
		}

		InputContext* Pointer() const {
			return reinterpret_cast<InputContext*>(bits.input_context_pointer << 4);
		}

		void SetPointer(const InputContext* p) {
			bits.input_context_pointer = reinterpret_cast<uint64_t>(p) >> 4;
		}
	};

	union StopEndpointCommandTRB {
		static const unsigned int Type = 15;
		std::array<uint32_t, 4> data{};
		struct {
			uint32_t : 32;

			uint32_t : 32;

			uint32_t : 32;

			uint32_t cycle_bit : 1;
			uint32_t : 9;
			uint32_t trb_type : 6;
			uint32_t endpoint_id : 5;
			uint32_t : 2;
			uint32_t suspend : 1;
			uint32_t slot_id : 8;
		} __attribute__((packed)) bits;

		StopEndpointCommandTRB(EndpointID endpoint_id, uint8_t slot_id) {
			bits.trb_type = Type;
			bits.endpoint_id = endpoint_id.Address();
			bits.slot_id = slot_id;
		}

		EndpointID GetEndpointID() const {
			return usb::EndpointID{ (int)bits.endpoint_id };
		}
	};

	union NoOpCommandTRB {
		static const unsigned int Type = 23;
		std::array<uint32_t, 4> data{};
		struct {
			uint32_t : 32;

			uint32_t : 32;

			uint32_t : 32;

			uint32_t cycle_bit : 1;
			uint32_t : 9;
			uint32_t trb_type : 6;
			uint32_t : 16;
		} __attribute__((packed)) bits;

		NoOpCommandTRB() {
			bits.trb_type = Type;
		}
	};

	union TransferEventTRB {
		static const unsigned int Type = 32;
		std::array<uint32_t, 4> data{};
		struct {
			uint64_t trb_pointer : 64;

			uint32_t trb_transfer_length : 24;
			uint32_t completion_code : 8;

			uint32_t cycle_bit : 1;
			uint32_t : 1;
			uint32_t event_data : 1;
			uint32_t : 7;
			uint32_t trb_type : 6;
			uint32_t endpoint_id : 5;
			uint32_t : 3;
			uint32_t slot_id : 8;
		} __attribute__((packed)) bits;

		TransferEventTRB() {
			bits.trb_type = Type;
		}

		TRB* Pointer() const {
			return reinterpret_cast<TRB*>(bits.trb_pointer);
		}

		void SetPointer(const TRB* p) {
			bits.trb_pointer = reinterpret_cast<uint64_t>(p);
		}

		EndpointID GetEndpointID() const {
			return usb::EndpointID{ (int)bits.endpoint_id };
		}
	};

	union CommandCompletionEventTRB {
		static const unsigned int Type = 33;
		std::array<uint32_t, 4> data{};
		struct {
			uint64_t : 4;
			uint64_t command_trb_pointer : 60;

			uint32_t command_completion_parameter : 24;
			uint32_t completion_code : 8;

			uint32_t cycle_bit : 1;
			uint32_t : 9;
			uint32_t trb_type : 6;
			uint32_t vf_id : 8;
			uint32_t slot_id : 8;
		} __attribute__((packed)) bits;

		CommandCompletionEventTRB() {
			bits.trb_type = Type;
		}

		TRB* Pointer() const {
			return reinterpret_cast<TRB*>(bits.command_trb_pointer << 4);
		}

		void SetPointer(TRB* p) {
			bits.command_trb_pointer = reinterpret_cast<uint64_t>(p) >> 4;
		}
	};

	union PortStatusChangeEventTRB {
		static const unsigned int Type = 34;
		std::array<uint32_t, 4> data{};
		struct {
			uint32_t : 24;
			uint32_t port_id : 8;

			uint32_t : 32;

			uint32_t : 24;
			uint32_t completion_code : 8;

			uint32_t cycle_bit : 1;
			uint32_t : 9;
			uint32_t trb_type : 6;
		} __attribute__((packed)) bits;

		PortStatusChangeEventTRB() {
			bits.trb_type = Type;
		}
	};

	/** @brief TRBDynamicCast casts a trb pointer to other type of TRB.
	 *
	 * @param trb  source pointer
	 * @return  casted pointer if the source TRB's type is equal to the resulting
	 *  type. nullptr otherwise.
	 */
	template <class ToType, class FromType>
	ToType* TRBDynamicCast(FromType* trb) {
		if (ToType::Type == trb->bits.trb_type) {
			return reinterpret_cast<ToType*>(trb);
		}
		return nullptr;
	}
}

// ---- ---- ---- ---- register.hpp ---- ---- ---- ---- //


template <typename T>
struct ArrayLength {};

template <typename T, size_t N>
struct ArrayLength<T[N]> {
  static const size_t value = N;
};

/**
 * MemMapRegister is a wrapper for a memory mapped register.
 *
 * MemMapRegister forces user program to read/write the underlying register
 * with specific bit width. The bit width will be deduced from the type of
 * T::data. T is the template parameter. T::data should be an array.
 */
template <typename T>
class MemMapRegister {
 public:
  T Read() const {
    T tmp;
    for (size_t i = 0; i < len_; ++i) {
      tmp.data[i] = value_.data[i];
    }
    return tmp;
  }

  void Write(const T& value) {
    for (size_t i = 0; i < len_; ++i) {
      value_.data[i] = value.data[i];
    }
  }

 private:
  volatile T value_;
  static const size_t len_ = ArrayLength<decltype(T::data)>::value;
};

template <typename T>
struct DefaultBitmap {
  T data[1];

  DefaultBitmap& operator =(const T& value) {
    data[0] = value;
  }
  operator T() const { return data[0]; }
};

/*
 * Design: container-like classes.
 *
 * Container-like classes, such as PortArray and DeviceContextArray,
 * should have Size() method and Iterator type.
 * Size() should return the number of elements, and iterators
 * of that type should iterate all elements.
 *
 * Each element may have a flag indicating availableness of the element.
 * For example each port has "Port Enabled/Disabled" bit.
 * Size() and iterators should not skip disabled elements.
 */

template <typename T>
class ArrayWrapper {
public:
	using ValueType = T;
	using Iterator = ValueType*;
	using ConstIterator = const ValueType*;

	ArrayWrapper(uintptr_t array_base_addr, size_t size)
		: array_(reinterpret_cast<ValueType*>(array_base_addr)),
		size_(size) {
	}

	size_t Size() const { return size_; }

	// begin, end, cbegin, cend must be lower case names
	// to be used in rage-based for statements.
	Iterator begin() { return array_; }
	Iterator end() { return array_ + size_; }
	ConstIterator cbegin() const { return array_; }
	ConstIterator cend() const { return array_ + size_; }

	ValueType& operator [](size_t index) { return array_[index]; }

private:
	ValueType* const array_;
	const size_t size_;
};

// ---- ---- ---- ---- registers.hpp ---- ---- ---- ---- //

namespace usb::xhci {
	union HCSPARAMS1_Bitmap {
		uint32_t data[1];
		struct {
			uint32_t max_device_slots : 8;
			uint32_t max_interrupters : 11;
			uint32_t : 5;
			uint32_t max_ports : 8;
		} __attribute__((packed)) bits;
	} __attribute__((packed));

	union HCSPARAMS2_Bitmap {
		uint32_t data[1];
		struct {
			uint32_t isochronous_scheduling_threshold : 4;
			uint32_t event_ring_segment_table_max : 4;
			uint32_t : 13;
			uint32_t max_scratchpad_buffers_high : 5;
			uint32_t scratchpad_restore : 1;
			uint32_t max_scratchpad_buffers_low : 5;
		} __attribute__((packed)) bits;
	} __attribute__((packed));

	union HCSPARAMS3_Bitmap {
		uint32_t data[1];
		struct {
			uint32_t u1_device_eixt_latency : 8;
			uint32_t : 8;
			uint32_t u2_device_eixt_latency : 16;
		} __attribute__((packed)) bits;
	} __attribute__((packed));

	union HCCPARAMS1_Bitmap {
		uint32_t data[1];
		struct {
			uint32_t addressing_capability_64 : 1;
			uint32_t bw_negotiation_capability : 1;
			uint32_t context_size : 1;
			uint32_t port_power_control : 1;
			uint32_t port_indicators : 1;
			uint32_t light_hc_reset_capability : 1;
			uint32_t latency_tolerance_messaging_capability : 1;
			uint32_t no_secondary_sid_support : 1;
			uint32_t parse_all_event_data : 1;
			uint32_t stopped_short_packet_capability : 1;
			uint32_t stopped_edtla_capability : 1;
			uint32_t contiguous_frame_id_capability : 1;
			uint32_t maximum_primary_stream_array_size : 4;
			uint32_t xhci_extended_capabilities_pointer : 16;
		} __attribute__((packed)) bits;
	} __attribute__((packed));

	union DBOFF_Bitmap {
		uint32_t data[1];
		struct {
			uint32_t : 2;
			uint32_t doorbell_array_offset : 30;
		} __attribute__((packed)) bits;

		uint32_t Offset() const { return bits.doorbell_array_offset << 2; }
	} __attribute__((packed));

	union RTSOFF_Bitmap {
		uint32_t data[1];
		struct {
			uint32_t : 5;
			uint32_t runtime_register_space_offset : 27;
		} __attribute__((packed)) bits;

		uint32_t Offset() const { return bits.runtime_register_space_offset << 5; }
	} __attribute__((packed));

	union HCCPARAMS2_Bitmap {
		uint32_t data[1];
		struct {
			uint32_t u3_entry_capability : 1;
			uint32_t configure_endpoint_command_max_exit_latency_too_large_capability : 1;
			uint32_t force_save_context_capability : 1;
			uint32_t compliance_transition_capability : 1;
			uint32_t large_esit_payload_capability : 1;
			uint32_t configuration_information_capability : 1;
			uint32_t : 26;
		} __attribute__((packed)) bits;
	} __attribute__((packed));

	struct CapabilityRegisters {
		MemMapRegister<DefaultBitmap<uint8_t>> CAPLENGTH;
		MemMapRegister<DefaultBitmap<uint8_t>> reserved1;
		MemMapRegister<DefaultBitmap<uint16_t>> HCIVERSION;
		MemMapRegister<HCSPARAMS1_Bitmap> HCSPARAMS1;
		MemMapRegister<HCSPARAMS2_Bitmap> HCSPARAMS2;
		MemMapRegister<HCSPARAMS3_Bitmap> HCSPARAMS3;
		MemMapRegister<HCCPARAMS1_Bitmap> HCCPARAMS1;
		MemMapRegister<DBOFF_Bitmap> DBOFF;
		MemMapRegister<RTSOFF_Bitmap> RTSOFF;
		MemMapRegister<HCCPARAMS2_Bitmap> HCCPARAMS2;
	} /*__attribute__((packed))*/;// ignoring packed attribute because of unpacked non-POD field

	union USBCMD_Bitmap {
		uint32_t data[1];
		struct {
			uint32_t run_stop : 1;
			uint32_t host_controller_reset : 1;
			uint32_t interrupter_enable : 1;
			uint32_t host_system_error_enable : 1;
			uint32_t : 3;
			uint32_t lignt_host_controller_reset : 1;
			uint32_t controller_save_state : 1;
			uint32_t controller_restore_state : 1;
			uint32_t enable_wrap_event : 1;
			uint32_t enable_u3_mfindex_stop : 1;
			uint32_t stopped_short_packet_enable : 1;
			uint32_t cem_enable : 1;
			uint32_t : 18;
		} __attribute__((packed)) bits;
	} __attribute__((packed));

	union USBSTS_Bitmap {
		uint32_t data[1];
		struct {
			uint32_t host_controller_halted : 1;
			uint32_t : 1;
			uint32_t host_system_error : 1;
			uint32_t event_interrupt : 1;
			uint32_t port_change_detect : 1;
			uint32_t : 3;
			uint32_t save_state_status : 1;
			uint32_t restore_state_status : 1;
			uint32_t save_restore_error : 1;
			uint32_t controller_not_ready : 1;
			uint32_t host_controller_error : 1;
			uint32_t : 19;
		} __attribute__((packed)) bits;
	} __attribute__((packed));

	union CRCR_Bitmap {
		uint64_t data[1];
		struct {
			uint64_t ring_cycle_state : 1;
			uint64_t command_stop : 1;
			uint64_t command_abort : 1;
			uint64_t command_ring_running : 1;
			uint64_t : 2;
			uint64_t command_ring_pointer : 58;
		} __attribute__((packed)) bits;

		uint64_t Pointer() const
		{
			return bits.command_ring_pointer << 6;
		}

		void SetPointer(uint64_t value)
		{
			bits.command_ring_pointer = value >> 6;
		}
	} __attribute__((packed));

	union DCBAAP_Bitmap {
		uint64_t data[1];
		struct {
			uint64_t : 6;
			uint64_t device_context_base_address_array_pointer : 26;
		} __attribute__((packed)) bits;

		uint64_t Pointer() const {
			return bits.device_context_base_address_array_pointer << 6;
		}

		void SetPointer(uint64_t value) {
			bits.device_context_base_address_array_pointer = value >> 6;
		}
	} __attribute__((packed));

	union CONFIG_Bitmap {
		uint32_t data[1];
		struct {
			uint32_t max_device_slots_enabled : 8;
			uint32_t u3_entry_enable : 1;
			uint32_t configuration_information_enable : 1;
			uint32_t : 22;
		} __attribute__((packed)) bits;
	} __attribute__((packed));

	struct OperationalRegisters {
		MemMapRegister<USBCMD_Bitmap> USBCMD;
		MemMapRegister<USBSTS_Bitmap> USBSTS;
		MemMapRegister<DefaultBitmap<uint32_t>> PAGESIZE;
		uint32_t reserved1[2];
		MemMapRegister<DefaultBitmap<uint32_t>> DNCTRL;
		MemMapRegister<CRCR_Bitmap> CRCR;
		uint32_t reserved2[4];
		MemMapRegister<DCBAAP_Bitmap> DCBAAP;
		MemMapRegister<CONFIG_Bitmap> CONFIG;
	} /*__attribute__((packed))*/;

	union PORTSC_Bitmap {
		uint32_t data[1];
		struct {
			uint32_t current_connect_status : 1;
			uint32_t port_enabled_disabled : 1;
			uint32_t : 1;
			uint32_t over_current_active : 1;
			uint32_t port_reset : 1;
			uint32_t port_link_state : 4;
			uint32_t port_power : 1;
			uint32_t port_speed : 4;
			uint32_t port_indicator_control : 2;
			uint32_t port_link_state_write_strobe : 1;
			uint32_t connect_status_change : 1;
			uint32_t port_enabled_disabled_change : 1;
			uint32_t warm_port_reset_change : 1;
			uint32_t over_current_change : 1;
			uint32_t port_reset_change : 1;
			uint32_t port_link_state_change : 1;
			uint32_t port_config_error_change : 1;
			uint32_t cold_attach_status : 1;
			uint32_t wake_on_connect_enable : 1;
			uint32_t wake_on_disconnect_enable : 1;
			uint32_t wake_on_over_current_enable : 1;
			uint32_t : 2;
			uint32_t device_removable : 1;
			uint32_t warm_port_reset : 1;
		} __attribute__((packed)) bits;
	} __attribute__((packed));

	union PORTPMSC_Bitmap {
		uint32_t data[1];
		struct {  // definition for USB3 protocol
			uint32_t u1_timeout : 8;
			uint32_t u2_timeout : 8;
			uint32_t force_link_pm_accept : 1;
			uint32_t : 15;
		} __attribute__((packed)) bits_usb3;
	} __attribute__((packed));

	union PORTLI_Bitmap {
		uint32_t data[1];
		struct {  // definition for USB3 protocol
			uint32_t link_error_count : 16;
			uint32_t rx_lane_count : 4;
			uint32_t tx_lane_count : 4;
			uint32_t : 8;
		} __attribute__((packed)) bits_usb3;
	} __attribute__((packed));

	union PORTHLPMC_Bitmap {
		uint32_t data[1];
		struct {  // definition for USB2 protocol
			uint32_t host_initiated_resume_duration_mode : 2;
			uint32_t l1_timeout : 8;
			uint32_t best_effort_service_latency_deep : 4;
			uint32_t : 18;
		} __attribute__((packed)) bits_usb2;
	} __attribute__((packed));

	struct PortRegisterSet {
		MemMapRegister<PORTSC_Bitmap> PORTSC;
		MemMapRegister<PORTPMSC_Bitmap> PORTPMSC;
		MemMapRegister<PORTLI_Bitmap> PORTLI;
		MemMapRegister<PORTHLPMC_Bitmap> PORTHLPMC;
	} /*__attribute__((packed))*/;

	using PortRegisterSetArray = ArrayWrapper<PortRegisterSet>;

	union IMAN_Bitmap {
		uint32_t data[1];
		struct {
			uint32_t interrupt_pending : 1;
			uint32_t interrupt_enable : 1;
			uint32_t : 30;
		} __attribute__((packed)) bits;
	} __attribute__((packed));

	union IMOD_Bitmap {
		uint32_t data[1];
		struct {
			uint32_t interrupt_moderation_interval : 16;
			uint32_t interrupt_moderation_counter : 16;
		} __attribute__((packed)) bits;
	} __attribute__((packed));

	union ERSTSZ_Bitmap {
		uint32_t data[1];
		struct {
			uint32_t event_ring_segment_table_size : 16;
			uint32_t : 16;
		} __attribute__((packed)) bits;

		uint16_t Size() const {
			return bits.event_ring_segment_table_size;
		}

		void SetSize(uint16_t value) {
			bits.event_ring_segment_table_size = value;
		}
	} __attribute__((packed));

	union ERSTBA_Bitmap {
		uint64_t data[1];
		struct {
			uint64_t : 6;
			uint64_t event_ring_segment_table_base_address : 58;
		} __attribute__((packed)) bits;

		uint64_t Pointer() const {
			return bits.event_ring_segment_table_base_address << 6;
		}

		void SetPointer(uint64_t value) {
			bits.event_ring_segment_table_base_address = value >> 6;
		}
	} __attribute__((packed));

	union ERDP_Bitmap {
		uint64_t data[1];
		struct {
			uint64_t dequeue_erst_segment_index : 3;
			uint64_t event_handler_busy : 1;
			uint64_t event_ring_dequeue_pointer : 60;
		} __attribute__((packed)) bits;

		uint64_t Pointer() const {
			return bits.event_ring_dequeue_pointer << 4;
		}

		void SetPointer(uint64_t value) {
			bits.event_ring_dequeue_pointer = value >> 4;
		}
	} __attribute__((packed));

	struct InterrupterRegisterSet {
		MemMapRegister<IMAN_Bitmap> IMAN;
		MemMapRegister<IMOD_Bitmap> IMOD;
		MemMapRegister<ERSTSZ_Bitmap> ERSTSZ;
		uint32_t reserved;
		MemMapRegister<ERSTBA_Bitmap> ERSTBA;
		MemMapRegister<ERDP_Bitmap> ERDP;
	} /*__attribute__((packed))*/;

	using InterrupterRegisterSetArray = ArrayWrapper<InterrupterRegisterSet>;

	union Doorbell_Bitmap {
		uint32_t data[1];
		struct {
			uint32_t db_target : 8;
			uint32_t : 8;
			uint32_t db_stream_id : 16;
		} __attribute__((packed)) bits;
	} __attribute__((packed));

	class DoorbellRegister {
		MemMapRegister<Doorbell_Bitmap> reg_;

	public:
		void Ring(uint8_t target, uint16_t stream_id = 0) {
			Doorbell_Bitmap value{};
			value.bits.db_target = target;
			value.bits.db_stream_id = stream_id;
			reg_.Write(value);
		}
	};

	using DoorbellRegisterArray = ArrayWrapper<DoorbellRegister>;

	/** @brief 拡張レジスタの共通ヘッダ構造 */
	union ExtendedRegister_Bitmap {
		uint32_t data[1];
		struct {
			uint32_t capability_id : 8;
			uint32_t next_pointer : 8;
			uint32_t value : 16;
		} __attribute__((packed)) bits;
	} __attribute__((packed));

	class ExtendedRegisterList {
	public:
		using ValueType = MemMapRegister<ExtendedRegister_Bitmap>;

		class Iterator {
		public:
			// 添加迭代器特性类型别名
			using iterator_category = std::forward_iterator_tag;
			using value_type = ValueType;
			using difference_type = std::ptrdiff_t;
			using pointer = value_type*;
			using reference = value_type&;
			//
			Iterator(ValueType* reg) : reg_{ reg } {}
			auto operator->() const { return reg_; }
			auto& operator*() const { return *reg_; }
			bool operator==(Iterator lhs) const { return reg_ == lhs.reg_; }
			bool operator!=(Iterator lhs) const { return reg_ != lhs.reg_; }
			Iterator& operator++();

		private:
			ValueType* reg_;
		};

		ExtendedRegisterList(uint64_t mmio_base, HCCPARAMS1_Bitmap hccp);

		Iterator begin() const { return first_; }
		Iterator end() const { return Iterator{ nullptr }; }

	private:
		const Iterator first_;
	};

	/**** 個別の拡張レジスタ定義 ****/
	union USBLEGSUP_Bitmap {
		uint32_t data[1];
		struct {
			uint32_t capability_id : 8;
			uint32_t next_pointer : 8;
			uint32_t hc_bios_owned_semaphore : 1;
			uint32_t : 7;
			uint32_t hc_os_owned_semaphore : 1;
			uint32_t : 7;
		} __attribute__((packed)) bits;
	} __attribute__((packed));
}

// ---- ---- ---- ---- speed.hpp ---- ---- ---- ---- //

namespace usb::xhci {
	const int kFullSpeed = 1;
	const int kLowSpeed = 2;
	const int kHighSpeed = 3;
	const int kSuperSpeed = 4;
	const int kSuperSpeedPlus = 5;
}

// ---- ---- ---- ---- ring.hpp ---- ---- ---- ---- //

namespace usb::xhci {
  /** @brief Command/Transfer Ring を表すクラス． */
	class Ring {
	public:
		Ring() = default;
		Ring(const Ring&) = delete;
		~Ring();
		Ring& operator=(const Ring&) = delete;

		/** @brief リングのメモリ領域を割り当て，メンバを初期化する． */
		Error Initialize(size_t buf_size);

		/** @brief TRB に cycle bit を設定した上でリング末尾に追加する．
		 *
		 * @return 追加された（リング上の）TRB を指すポインタ．
		 */
		template <typename TRBType>
		TRB* Push(const TRBType& trb) {
			return Push(trb.data);
		}

		TRB* Buffer() const { return buf_; }

	private:
		TRB* buf_ = nullptr;
		size_t buf_size_ = 0;

		/** @brief プロデューサ・サイクル・ステートを表すビット */
		bool cycle_bit_;
		/** @brief リング上で次に書き込む位置 */
		size_t write_index_;

		/** @brief TRB に cycle bit を設定した上でリング末尾に書き込む．
		 *
		 * write_index_ は変化させない．
		 */
		void CopyToLast(const std::array<uint32_t, 4>& data);

		/** @brief TRB に cycle bit を設定した上でリング末尾に追加する．
		 *
		 * write_index_ をインクリメントする．その結果 write_index_ がリング末尾
		 * に達したら LinkTRB を適切に配置して write_index_ を 0 に戻し，
		 * cycle bit を反転させる．
		 *
		 * @return 追加された（リング上の）TRB を指すポインタ．
		 */
		TRB* Push(const std::array<uint32_t, 4>& data);
	};

	union EventRingSegmentTableEntry {
		std::array<uint32_t, 4> data;
		struct {
			uint64_t ring_segment_base_address;  // 64 バイトアライメント

			uint32_t ring_segment_size : 16;
			uint32_t : 16;

			uint32_t : 32;
		} __attribute__((packed)) bits;
	};

	class EventRing {
	public:
		Error Initialize(size_t buf_size, InterrupterRegisterSet* interrupter);

		TRB* ReadDequeuePointer() const {
			return reinterpret_cast<TRB*>(interrupter_->ERDP.Read().Pointer());
		}

		void WriteDequeuePointer(TRB* p);

		bool HasFront() const {
			return Front()->bits.cycle_bit == cycle_bit_;
		}

		TRB* Front() const {
			return ReadDequeuePointer();
		}

		void Pop();

	private:
		TRB* buf_;
		size_t buf_size_;

		bool cycle_bit_;
		EventRingSegmentTableEntry* erst_;
		InterrupterRegisterSet* interrupter_;
	};
}

// ---- ---- ---- ---- port.hpp ---- ---- ---- ---- //


#define CLEAR_STATUS_BIT(bitname) \
  [this](){ \
    PORTSC_Bitmap portsc = port_reg_set_.PORTSC.Read(); \
    portsc.data[0] &= 0x0e01c3e0u; \
    portsc.bits.bitname = 1; \
    port_reg_set_.PORTSC.Write(portsc); \
  }()

namespace usb::xhci {
	class Controller;
	struct PortRegisterSet;
	class Device;

	class Port {
	public:
		Port(uint8_t port_num, PortRegisterSet& port_reg_set)
			: port_num_{ port_num }, port_reg_set_{ port_reg_set }
		{
		}

		uint8_t Number() const;
		bool IsConnected() const;
		bool IsEnabled() const;
		bool IsConnectStatusChanged() const;
		bool IsPortResetChanged() const;
		int Speed() const;
		Error Reset();
		Device* Initialize();

		void ClearConnectStatusChanged() const {
			CLEAR_STATUS_BIT(connect_status_change);
		}
		void ClearPortResetChange() const {
			CLEAR_STATUS_BIT(port_reset_change);
		}

	private:
		const uint8_t port_num_;
		PortRegisterSet& port_reg_set_;
	};
}

// ---- ---- ---- ---- device.hpp ---- ---- ---- ---- //

namespace usb::xhci {
	class Device : public usb::Device {
	public:
		enum class State {
			kInvalid,
			kBlank,
			kSlotAssigning,
			kSlotAssigned
		};

		using OnTransferredCallbackType = void (
			Device* dev,
			DeviceContextIndex dci,
			int completion_code,
			int trb_transfer_length,
			TRB* issue_trb);

		Device(uint8_t slot_id, DoorbellRegister* dbreg);

		Error Initialize();

		DeviceContext* GetDeviceContext() { return &ctx_; }
		InputContext* GetInputContext() { return &input_ctx_; }
		//usb::Device* USBDevice() { return usb_device_; }
		//void SetUSBDevice(usb::Device* value) { usb_device_ = value; }

		State State() const { return state_; }
		uint8_t SlotID() const { return slot_id_; }

		void SelectForSlotAssignment();
		Ring* AllocTransferRing(DeviceContextIndex index, size_t buf_size);

		Error ControlIn(EndpointID ep_id, SetupData setup_data,
			void* buf, int len, ClassDriver* issuer) override;
		Error ControlOut(EndpointID ep_id, SetupData setup_data,
			const void* buf, int len, ClassDriver* issuer) override;
		Error InterruptIn(EndpointID ep_id, void* buf, int len) override;
		Error InterruptOut(EndpointID ep_id, void* buf, int len) override;

		Error OnTransferEventReceived(const TransferEventTRB& trb);

	private:
		alignas(64) struct DeviceContext ctx_;
		alignas(64) struct InputContext input_ctx_;

		const uint8_t slot_id_;
		DoorbellRegister* const dbreg_;

		enum State state_;
		std::array<Ring*, 31> transfer_rings_; // index = dci - 1

		/** コントロール転送が完了した際に DataStageTRB や StatusStageTRB
		 * から対応する SetupStageTRB を検索するためのマップ．
		 */
		ArrayMap<const void*, const SetupStageTRB*, 16> setup_stage_map_{};

		//usb::Device* usb_device_;
	};
}

// ---- ---- ---- ---- devmgr.hpp ---- ---- ---- ---- //

namespace usb::xhci {
	class DeviceManager {

	public:
		Error Initialize(size_t max_slots);
		DeviceContext** DeviceContexts() const;
		Device* FindByPort(uint8_t port_num, uint32_t route_string) const;
		Device* FindByState(enum Device::State state) const;
		Device* FindBySlot(uint8_t slot_id) const;
		//WithError<Device*> Get(uint8_t device_id) const;
		Error AllocDevice(uint8_t slot_id, DoorbellRegister* dbreg);
		Error LoadDCBAA(uint8_t slot_id);
		Error Remove(uint8_t slot_id);

	private:
	 // device_context_pointers_ can be used as DCBAAP's value.
	 // The number of elements is max_slots_ + 1.
		DeviceContext** device_context_pointers_;
		size_t max_slots_;

		// The number of elements is max_slots_ + 1.
		Device** devices_;
	};
}

// ---- ---- ---- ---- xHCI.hpp ---- ---- ---- ---- //

namespace usb::xhci {
	class Controller {
	public:
		Controller(uintptr_t mmio_base);
		Error Initialize();
		Error Run();
		Ring* CommandRing() { return &cr_; }
		EventRing* PrimaryEventRing() { return &er_; }
		DoorbellRegister* DoorbellRegisterAt(uint8_t index);
		Port PortAt(uint8_t port_num) {
			return Port{ port_num, PortRegisterSets()[port_num - 1] };
		}
		uint8_t MaxPorts() const { return max_ports_; }
		DeviceManager* GetDeviceManager() { return &devmgr_; }
	public:
		Error ProcessEvents();
	private:
		static const size_t kDeviceSize = 8;

		const uintptr_t mmio_base_;
		CapabilityRegisters* const cap_;
		OperationalRegisters* const op_;
		const uint8_t max_ports_;

		class DeviceManager devmgr_;
		Ring cr_;
		EventRing er_;

		InterrupterRegisterSetArray InterrupterRegisterSets() const {
			return { mmio_base_ + cap_->RTSOFF.Read().Offset() + 0x20u, 1024 };
		}

		PortRegisterSetArray PortRegisterSets() const {
			return { reinterpret_cast<uintptr_t>(op_) + 0x400u, max_ports_ };
		}

		DoorbellRegisterArray DoorbellRegisters() const {
			return { mmio_base_ + cap_->DBOFF.Read().Offset(), 256 };
		}
	};

	Error ConfigurePort(Controller& xhc, Port& port);
	Error ConfigureEndpoints(Controller& xhc, Device& dev);

	/** @brief イベントリングに登録されたイベントを高々1つ処理する．
	 *
	 * xhc のプライマリイベントリングの先頭のイベントを処理する．
	 * イベントが無ければ即座に Error::kSuccess を返す．
	 *
	 * @return イベントを正常に処理できたら Error::kSuccess
	 */
	Error ProcessEvent(Controller& xhc);
}


#endif

#endif
