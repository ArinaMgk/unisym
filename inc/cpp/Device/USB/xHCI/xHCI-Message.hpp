
// ---- ---- ---- ---- context.hpp ---- ---- ---- ---- //


namespace uni::device::SpaceUSB3 {
	using namespace uni::device::SpaceUSB;

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

namespace uni::device::SpaceUSB3 {
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
			return uni::device::SpaceUSB::EndpointID{ (int)bits.endpoint_id };
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
			return uni::device::SpaceUSB::EndpointID{ (int)bits.endpoint_id };
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


