	enum class GPIOInterruptTrigger : uint8 {
		Edge,
		Level,
	};

	enum class GPIOInterruptPolarity : uint8 {
		ActiveHigh,
		ActiveLow,
		ActiveBoth,
	};

	struct GPIOInterruptConfiguration {
		GPIOInterruptTrigger trigger;
		GPIOInterruptPolarity polarity;
		uint16 debounce_timeout;
		bool shared;
		bool wake_capable;
	};

	enum GeneralPurposeInputOutputCapability : uint32 {
		GeneralPurposeInputOutputCapability_None = 0,
		GeneralPurposeInputOutputCapability_Input = 1u << 0,
		GeneralPurposeInputOutputCapability_Output = 1u << 1,
		GeneralPurposeInputOutputCapability_PullUp = 1u << 2,
		GeneralPurposeInputOutputCapability_PullDown = 1u << 3,
		GeneralPurposeInputOutputCapability_InterruptEdge = 1u << 4,
		GeneralPurposeInputOutputCapability_InterruptLevel = 1u << 5,
		GeneralPurposeInputOutputCapability_ActiveHigh = 1u << 6,
		GeneralPurposeInputOutputCapability_ActiveLow = 1u << 7,
		GeneralPurposeInputOutputCapability_ActiveBoth = 1u << 8,
		GeneralPurposeInputOutputCapability_Shared = 1u << 9,
		GeneralPurposeInputOutputCapability_Wake = 1u << 10,
		GeneralPurposeInputOutputCapability_Pending = 1u << 11,
		GeneralPurposeInputOutputCapability_Mask = 1u << 12,
		GeneralPurposeInputOutputCapability_Acknowledge = 1u << 13,
	};

	struct GeneralPurposeInputOutputPortBackend {
		void* context;
		stduint pin_count;
		uint32 capabilities;
		bool (*read_pin)(void* context, stduint pin, bool& value);
		bool (*write_pin)(void* context, stduint pin, bool value);
		bool (*configure_pin)(void* context, stduint pin, GPIOMode mode, GPIOSpeed speed);
		bool (*is_input)(void* context, stduint pin, bool& input);
		bool (*set_pull)(void* context, stduint pin, bool pull_up);
		bool (*read_port)(void* context, stduint& value);
		bool (*write_port)(void* context, stduint value);
		bool (*modify_port)(void* context, stduint set_mask, stduint reset_mask);
		bool (*configure_interrupt)(void* context, stduint pin, const GPIOInterruptConfiguration& configuration);
		bool (*set_interrupt)(void* context, stduint pin, Handler_t handler);
		bool (*set_interrupt_priority)(void* context, stduint pin, byte preempt, byte sub_priority);
		bool (*enable_interrupt)(void* context, stduint pin, bool enable);
		bool (*interrupt_pending)(void* context, stduint pin, bool& pending);
		bool (*acknowledge_interrupt)(void* context, stduint pin);
	};

	const GeneralPurposeInputOutputPortBackend* ResolveGPIOBackend(SelfGpin pin);
	