// ASCII CPP TAB4 LF
// AllAuthor: @ArinaMgk
// ModuTitle: Intel GPIO Community Register Core
// Copyright: UNISYM, under Apache License 2.0
#ifndef _INC_DEVICE_GPIO_INTEL_HPP
#define _INC_DEVICE_GPIO_INTEL_HPP

#include "../../../../inc/cpp/Device/GPIO"

#if defined(_MCCA) && ((_MCCA & 0xFF00) == 0x8600)

namespace uni {

	enum class IntelGPIOStatus : uint8 {
		Success,
		InvalidArgument,
		InvalidLayout,
		AccessFailure,
		OutOfRange,
		NotHostOwned,
		ConfigurationLocked,
		OutputLocked,
		FirmwareOwned,
		Unsupported,
	};

	struct IntelGPIOCommunityDescription {
		uint16 pin_count;
		uint8 group_size;
		uint8 ownership_registers_per_group;
		uint16 pad_configuration_stride;
		uint32 minimum_access_length;
		uint32 revision_offset;
		uint32 pad_base_offset;
		uint32 pad_ownership_offset;
		uint32 pad_configuration_lock_offset;
		uint32 host_software_ownership_offset;
		uint32 interrupt_status_offset;
		uint32 interrupt_enable_offset;
	};

	struct IntelGPIOPinState {
		uint32 pad_configuration0;
		uint32 pad_configuration1;
		bool host_owned;
		bool configuration_locked;
		bool output_locked;
		bool host_software_owned;
		bool input_value;
		bool output_value;
		bool interrupt_pending;
		bool interrupt_enabled;
	};

	using IntelGPIOReadRegister = bool (*)(void* context, uint32 offset, uint32& value);
	using IntelGPIOWriteRegister = bool (*)(void* context, uint32 offset, uint32 value);

	class IntelGPIOCommunity {
		const IntelGPIOCommunityDescription* description;
		void* context;
		IntelGPIOReadRegister read_register;
		IntelGPIOWriteRegister write_register;
		uint32 access_length;
		uint32 revision;
		uint32 pad_base;
		bool initialized;

		bool ReadRegister(uint32 offset, uint32& value) const;
		bool WriteRegister(uint32 offset, uint32 value) const;
		IntelGPIOStatus InspectOwnership(stduint pin, IntelGPIOPinState& state) const;
		IntelGPIOStatus RequireWritablePin(stduint pin, bool output_only) const;
		uint32 PadConfigurationOffset(stduint pin) const;
		uint32 GroupRegisterOffset(uint32 base, stduint pin, uint32 stride) const;
	public:
		IntelGPIOCommunity();
		IntelGPIOStatus Initialize(const IntelGPIOCommunityDescription& layout, void* register_context, IntelGPIOReadRegister reader, IntelGPIOWriteRegister writer, uint32 length);
		stduint PinCount() const;
		uint32 Revision() const;
		uint32 PadBase() const;
		IntelGPIOStatus InspectPin(stduint pin, IntelGPIOPinState& state) const;
		IntelGPIOStatus ReadPin(stduint pin, bool& value) const;
		IntelGPIOStatus WritePin(stduint pin, bool value);
		IntelGPIOStatus ConfigurePin(stduint pin, GPIOMode mode);
		IntelGPIOStatus IsInput(stduint pin, bool& input) const;
		IntelGPIOStatus SetPull(stduint pin, bool pull_up);
		IntelGPIOStatus ConfigureInterrupt(stduint pin, const GPIOInterruptConfiguration& configuration);
		IntelGPIOStatus EnableInterrupt(stduint pin, bool enable);
		IntelGPIOStatus InterruptPending(stduint pin, bool& pending) const;
		IntelGPIOStatus AcknowledgeInterrupt(stduint pin);
	};
}

#endif

#endif
