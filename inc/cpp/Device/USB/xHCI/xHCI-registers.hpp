#ifndef _XHCI_REGISTERS_HPP_
#define _XHCI_REGISTERS_HPP_

#include <iterator>
#include "../../../unisym"
#include "./xHCI-template.hpp"
namespace uni::device::SpaceUSB3 {
	using namespace ext;

	_PACKED(union) HCSPARAMS1_t {
		uint32 data[1];
		_PACKED(struct) {
			uint32 max_device_slots : 8;
			uint32 max_interrupters : 11;
			uint32 : 5;
			uint32 max_ports : 8;
		} bits;
	};

	_PACKED(union) HCSPARAMS2_t {
		uint32 data[1];
		_PACKED(struct) {
			uint32 isochronous_scheduling_threshold : 4;
			uint32 event_ring_segment_table_max : 4;
			uint32 : 13;
			uint32 max_scratchpad_buffers_high : 5;
			uint32 scratchpad_restore : 1;
			uint32 max_scratchpad_buffers_low : 5;
		} bits;
	};

	_PACKED(union) HCSPARAMS3_t {
		uint32 data[1];
		_PACKED(struct) {
			uint32 u1_device_eixt_latency : 8;
			uint32 : 8;
			uint32 u2_device_eixt_latency : 16;
		} bits;
	};

	_PACKED(union) HCCPARAMS1_t {
		uint32 data[1];
		_PACKED(struct) {
			uint32 addressing_capability_64 : 1;
			uint32 bw_negotiation_capability : 1;
			uint32 context_size : 1;
			uint32 port_power_control : 1;
			uint32 port_indicators : 1;
			uint32 light_hc_reset_capability : 1;
			uint32 latency_tolerance_messaging_capability : 1;
			uint32 no_secondary_sid_support : 1;
			uint32 parse_all_event_data : 1;
			uint32 stopped_short_packet_capability : 1;
			uint32 stopped_edtla_capability : 1;
			uint32 contiguous_frame_id_capability : 1;
			uint32 maximum_primary_stream_array_size : 4;
			uint32 xhci_extended_capabilities_pointer : 16;
		} bits;
	};

	_PACKED(union) DBOFF_t {
		uint32 data[1];
		_PACKED(struct) {
			uint32 : 2;
			uint32 doorbell_array_offset : 30;
		} bits;
		//
		uint32 Offset() const { return bits.doorbell_array_offset << 2; }
	};

	_PACKED(union) RTSOFF_t {
		uint32 data[1];
		_PACKED(struct) {
			uint32 : 5;
			uint32 runtime_register_space_offset : 27;
		} bits;
		//
		uint32 Offset() const { return bits.runtime_register_space_offset << 5; }
	};

	_PACKED(union) HCCPARAMS2_t {
		uint32 data[1];
		_PACKED(struct) {
			uint32 u3_entry_capability : 1;
			uint32 configure_endpoint_command_max_exit_latency_too_large_capability : 1;
			uint32 force_save_context_capability : 1;
			uint32 compliance_transition_capability : 1;
			uint32 large_esit_payload_capability : 1;
			uint32 configuration_information_capability : 1;
			uint32 : 26;
		} bits;
	};

	struct CapabilityRegisters {
		MemMapRegister<DefaultBitmap<uint8_t>> CAPLENGTH;
		MemMapRegister<DefaultBitmap<uint8_t>> reserved1;
		MemMapRegister<DefaultBitmap<uint16_t>> HCIVERSION;
		MemMapRegister<HCSPARAMS1_t> HCSPARAMS1;
		MemMapRegister<HCSPARAMS2_t> HCSPARAMS2;
		MemMapRegister<HCSPARAMS3_t> HCSPARAMS3;
		MemMapRegister<HCCPARAMS1_t> HCCPARAMS1;
		MemMapRegister<DBOFF_t> DBOFF;
		MemMapRegister<RTSOFF_t> RTSOFF;
		MemMapRegister<HCCPARAMS2_t> HCCPARAMS2;
	} /*__attribute__((packed))*/;// ignoring packed attribute because of unpacked non-POD field

	static_assert(offsetof(CapabilityRegisters, HCIVERSION) == 0x02,
		"HCIVERSION offset must be 0x02");
	static_assert(offsetof(CapabilityRegisters, HCSPARAMS1) == 0x04,
		"HCSPARAMS1 offset must be 0x04");

	_PACKED(union) USBCMD_t {
		uint32 data[1];
		_PACKED(struct) {
			uint32 run_stop : 1;
			uint32 host_controller_reset : 1;
			uint32 interrupter_enable : 1;
			uint32 host_system_error_enable : 1;
			uint32 : 3;
			uint32 lignt_host_controller_reset : 1;
			uint32 controller_save_state : 1;
			uint32 controller_restore_state : 1;
			uint32 enable_wrap_event : 1;
			uint32 enable_u3_mfindex_stop : 1;
			uint32 stopped_short_packet_enable : 1;
			uint32 cem_enable : 1;
			uint32 : 18;
		} bits;
	};

	_PACKED(union) USBSTS_t {
		uint32 data[1];
		_PACKED(struct) {
			uint32 host_controller_halted : 1;
			uint32 : 1;
			uint32 host_system_error : 1;
			uint32 event_interrupt : 1;
			uint32 port_change_detect : 1;
			uint32 : 3;
			uint32 save_state_status : 1;
			uint32 restore_state_status : 1;
			uint32 save_restore_error : 1;
			uint32 controller_not_ready : 1;
			uint32 host_controller_error : 1;
			uint32 : 19;
		} bits;
	};

	_PACKED(union) CRCR_t {
		uint64_t data[1];
		_PACKED(struct) {
			uint64_t ring_cycle_state : 1;
			uint64_t command_stop : 1;
			uint64_t command_abort : 1;
			uint64_t command_ring_running : 1;
			uint64_t : 2;
			uint64_t command_ring_pointer : 58;
		} bits;
		//
		uint64_t Pointer() const
		{
			return bits.command_ring_pointer << 6;
		}
		void SetPointer(uint64_t value)
		{
			bits.command_ring_pointer = value >> 6;
		}
	};

	_PACKED(union) DCBAAP_t {
		uint64_t data[1];
		_PACKED(struct) {
			uint64_t : 6;
			uint64_t device_context_base_address_array_pointer : 26;
		} bits;

		uint64_t Pointer() const {
			return bits.device_context_base_address_array_pointer << 6;
		}

		void SetPointer(uint64_t value) {
			bits.device_context_base_address_array_pointer = value >> 6;
		}
	};

	_PACKED(union) CONFIG_t {
		uint32 data[1];
		_PACKED(struct) {
			uint32 max_device_slots_enabled : 8;
			uint32 u3_entry_enable : 1;
			uint32 configuration_information_enable : 1;
			uint32 : 22;
		} bits;
	};

	struct OperationalRegisters {
		MemMapRegister<USBCMD_t> USBCMD;
		MemMapRegister<USBSTS_t> USBSTS;
		MemMapRegister<DefaultBitmap<uint32>> PAGESIZE;
		uint32 reserved1[2];
		MemMapRegister<DefaultBitmap<uint32>> DNCTRL;
		MemMapRegister<CRCR_t> CRCR;
		uint32 reserved2[4];
		MemMapRegister<DCBAAP_t> DCBAAP;
		MemMapRegister<CONFIG_t> CONFIG;
	} /*__attribute__((packed))*/;

	_PACKED(union) PORTSC_t {
		uint32 data[1];
		_PACKED(struct) {
			uint32 current_connect_status : 1;
			uint32 port_enabled_disabled : 1;
			uint32 : 1;
			uint32 over_current_active : 1;
			uint32 port_reset : 1;
			uint32 port_link_state : 4;
			uint32 port_power : 1;
			uint32 port_speed : 4;
			uint32 port_indicator_control : 2;
			uint32 port_link_state_write_strobe : 1;
			uint32 connect_status_change : 1;
			uint32 port_enabled_disabled_change : 1;
			uint32 warm_port_reset_change : 1;
			uint32 over_current_change : 1;
			uint32 port_reset_change : 1;
			uint32 port_link_state_change : 1;
			uint32 port_config_error_change : 1;
			uint32 cold_attach_status : 1;
			uint32 wake_on_connect_enable : 1;
			uint32 wake_on_disconnect_enable : 1;
			uint32 wake_on_over_current_enable : 1;
			uint32 : 2;
			uint32 device_removable : 1;
			uint32 warm_port_reset : 1;
		} bits;
	};

	_PACKED(union) PORTPMSC_t {
		uint32 data[1];
		_PACKED(struct) {  // definition for USB3 protocol
			uint32 u1_timeout : 8;
			uint32 u2_timeout : 8;
			uint32 force_link_pm_accept : 1;
			uint32 : 15;
		} bits_usb3;
	};

	_PACKED(union) PORTLI_t {
		uint32 data[1];
		_PACKED(struct) {  // definition for USB3 protocol
			uint32 link_error_count : 16;
			uint32 rx_lane_count : 4;
			uint32 tx_lane_count : 4;
			uint32 : 8;
		} bits_usb3;
	};

	_PACKED(union) PORTHLPMC_t {
		uint32 data[1];
		_PACKED(struct) {  // definition for USB2 protocol
			uint32 host_initiated_resume_duration_mode : 2;
			uint32 l1_timeout : 8;
			uint32 best_effort_service_latency_deep : 4;
			uint32 : 18;
		} bits_usb2;
	};

	struct PortRegisterSet {
		MemMapRegister<PORTSC_t> PORTSC;
		MemMapRegister<PORTPMSC_t> PORTPMSC;
		MemMapRegister<PORTLI_t> PORTLI;
		MemMapRegister<PORTHLPMC_t> PORTHLPMC;
	} /*__attribute__((packed))*/;

	using PortRegisterSetArray = ArrayWrapper<PortRegisterSet>;

	_PACKED(union) IMAN_t {
		uint32 data[1];
		_PACKED(struct) {
			uint32 interrupt_pending : 1;
			uint32 interrupt_enable : 1;
			uint32 : 30;
		} bits;
	};

	_PACKED(union) IMOD_t {
		uint32 data[1];
		_PACKED(struct) {
			uint32 interrupt_moderation_interval : 16;
			uint32 interrupt_moderation_counter : 16;
		} bits;
	};

	_PACKED(union) ERSTSZ_t {
		uint32 data[1];
		_PACKED(struct) {
			uint32 event_ring_segment_table_size : 16;
			uint32 : 16;
		} bits;

		uint16_t Size() const {
			return bits.event_ring_segment_table_size;
		}

		void SetSize(uint16_t value) {
			bits.event_ring_segment_table_size = value;
		}
	};

	_PACKED(union) ERSTBA_t {
		uint64_t data[1];
		_PACKED(struct) {
			uint64_t : 6;
			uint64_t event_ring_segment_table_base_address : 58;
		} bits;
		//
		uint64_t Pointer() const {
			return bits.event_ring_segment_table_base_address << 6;
		}
		void SetPointer(uint64_t value) {
			bits.event_ring_segment_table_base_address = value >> 6;
		}
	};

	_PACKED(union) ERDP_t {
		uint64_t data[1];
		_PACKED(struct) {
			uint64_t dequeue_erst_segment_index : 3;
			uint64_t event_handler_busy : 1;
			uint64_t event_ring_dequeue_pointer : 60;
		} bits;

		uint64_t Pointer() const {
			return bits.event_ring_dequeue_pointer << 4;
		}

		void SetPointer(uint64_t value) {
			bits.event_ring_dequeue_pointer = value >> 4;
		}
	};

	struct InterrupterRegisterSet {
		MemMapRegister<IMAN_t> IMAN;
		MemMapRegister<IMOD_t> IMOD;
		MemMapRegister<ERSTSZ_t> ERSTSZ;
		uint32 reserved;
		MemMapRegister<ERSTBA_t> ERSTBA;
		MemMapRegister<ERDP_t> ERDP;
	} /*__attribute__((packed))*/;

	using InterrupterRegisterSetArray = ArrayWrapper<InterrupterRegisterSet>;

	_PACKED(union) Doorbell_t {
		uint32 data[1];
		_PACKED(struct) {
			uint32 db_target : 8;
			uint32 : 8;
			uint32 db_stream_id : 16;
		} bits;
	};

	class DoorbellRegister {
		MemMapRegister<Doorbell_t> reg_;

	public:
		void Ring(uint8_t target, uint16_t stream_id = 0) {
			Doorbell_t value{};
			value.bits.db_target = target;
			value.bits.db_stream_id = stream_id;
			reg_.Write(value);
		}
	};

	using DoorbellRegisterArray = ArrayWrapper<DoorbellRegister>;

	/** @brief 拡張レジスタの共通ヘッダ構造 */
	_PACKED(union) ExtendedRegister_t {
		uint32 data[1];
		_PACKED(struct) {
			uint32 capability_id : 8;
			uint32 next_pointer : 8;
			uint32 value : 16;
		} bits;
	};

	class ExtendedRegisterList {
	public:
		using ValueType = MemMapRegister<ExtendedRegister_t>;

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

		ExtendedRegisterList(uint64_t mmio_base, HCCPARAMS1_t hccp);

		Iterator begin() const { return first_; }
		Iterator end() const { return Iterator{ nullptr }; }

	private:
		const Iterator first_;
	};

	/**** 個別の拡張レジスタ定義 ****/
	_PACKED(union) USBLEGSUP_t {
		uint32 data[1];
		_PACKED(struct) {
			uint32 capability_id : 8;
			uint32 next_pointer : 8;
			uint32 hc_bios_owned_semaphore : 1;
			uint32 : 7;
			uint32 hc_os_owned_semaphore : 1;
			uint32 : 7;
		} bits;
	};


}

#endif
