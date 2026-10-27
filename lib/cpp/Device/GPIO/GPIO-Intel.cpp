// ASCII CPP TAB4 LF
// AllAuthor: @ArinaMgk
// ModuTitle: Intel GPIO Community Register Core
// Copyright: UNISYM, under Apache License 2.0
#include "GPIO-Intel.hpp"

#if defined(_MCCA) && ((_MCCA & 0xFF00) == 0x8600)

namespace uni {
	namespace {
		// Register layout follows Linux pinctrl-intel.c and pinctrl-broxton.c for Broxton/Apollo Lake communities.
		constexpr uint32 RegisterWidth = sizeof(uint32);
		constexpr uint32 RevisionShift = 16;
		constexpr uint32 PadConfiguration0ReceiveEventMask = 3u << 25;
		constexpr uint32 PadConfiguration0ReceiveEventLevel = 0u << 25;
		constexpr uint32 PadConfiguration0ReceiveEventEdge = 1u << 25;
		constexpr uint32 PadConfiguration0ReceiveEventBoth = 3u << 25;
		constexpr uint32 PadConfiguration0ReceiveInvert = 1u << 23;
		constexpr uint32 PadConfiguration0RouteIoApic = 1u << 20;
		constexpr uint32 PadConfiguration0RouteSci = 1u << 19;
		constexpr uint32 PadConfiguration0RouteSmi = 1u << 18;
		constexpr uint32 PadConfiguration0RouteNmi = 1u << 17;
		constexpr uint32 PadConfiguration0ModeMask = 0xFu << 10;
		constexpr uint32 PadConfiguration0ReceiveDisable = 1u << 9;
		constexpr uint32 PadConfiguration0TransmitDisable = 1u << 8;
		constexpr uint32 PadConfiguration0ReceiveState = 1u << 1;
		constexpr uint32 PadConfiguration0TransmitState = 1u << 0;
		constexpr uint32 PadConfiguration1TerminationUp = 1u << 13;
		constexpr uint32 PadConfiguration1TerminationMask = 7u << 10;
		constexpr uint32 PadConfiguration1Termination5K = 2u << 10;

		bool OffsetAvailable(uint32 offset, uint32 length) {
			return offset <= length && RegisterWidth <= length - offset;
		}
	}

	IntelGPIOCommunity::IntelGPIOCommunity() : description(nullptr), context(nullptr), read_register(nullptr), write_register(nullptr), access_length(0), revision(0), pad_base(0), initialized(false) {}

	bool IntelGPIOCommunity::ReadRegister(uint32 offset, uint32& value) const {
		value = 0;
		return initialized && read_register && OffsetAvailable(offset, access_length) && read_register(context, offset, value);
	}

	bool IntelGPIOCommunity::WriteRegister(uint32 offset, uint32 value) const {
		return initialized && write_register && OffsetAvailable(offset, access_length) && write_register(context, offset, value);
	}

	uint32 IntelGPIOCommunity::PadConfigurationOffset(stduint pin) const {
		return pad_base + uint32(pin) * description->pad_configuration_stride;
	}

	uint32 IntelGPIOCommunity::GroupRegisterOffset(uint32 base, stduint pin, uint32 stride) const {
		return base + uint32(pin / description->group_size) * stride;
	}

	IntelGPIOStatus IntelGPIOCommunity::Initialize(const IntelGPIOCommunityDescription& layout, void* register_context, IntelGPIOReadRegister reader, IntelGPIOWriteRegister writer, uint32 length) {
		description = nullptr;
		context = nullptr;
		read_register = nullptr;
		write_register = nullptr;
		access_length = 0;
		revision = 0;
		pad_base = 0;
		initialized = false;
		if (!register_context || !reader || !writer || !layout.pin_count || !layout.group_size || layout.group_size > 32 || !layout.ownership_registers_per_group || layout.pad_configuration_stride < 8 || layout.minimum_access_length > length) return IntelGPIOStatus::InvalidArgument;
		if (!OffsetAvailable(layout.revision_offset, length) || !OffsetAvailable(layout.pad_base_offset, length) || !OffsetAvailable(layout.pad_ownership_offset, length) || !OffsetAvailable(layout.pad_configuration_lock_offset, length) || !OffsetAvailable(layout.host_software_ownership_offset, length) || !OffsetAvailable(layout.interrupt_status_offset, length) || !OffsetAvailable(layout.interrupt_enable_offset, length)) return IntelGPIOStatus::InvalidLayout;
		description = &layout;
		context = register_context;
		read_register = reader;
		write_register = writer;
		access_length = length;
		initialized = true;
		uint32 revision_register;
		uint32 pad_base_register;
		if (!ReadRegister(layout.revision_offset, revision_register) || !ReadRegister(layout.pad_base_offset, pad_base_register)) {
			initialized = false;
			return IntelGPIOStatus::AccessFailure;
		}
		revision = revision_register >> RevisionShift;
		pad_base = pad_base_register;
		// Initialization requires PADCFG0 for every declared pin; operations that need PADCFG1 validate that register separately.
		const uint64 last_pad_end = uint64(pad_base) + uint64(layout.pin_count - 1) * layout.pad_configuration_stride + RegisterWidth;
		if ((pad_base & 3u) || last_pad_end > length) {
			initialized = false;
			return IntelGPIOStatus::InvalidLayout;
		}
		return IntelGPIOStatus::Success;
	}

	stduint IntelGPIOCommunity::PinCount() const {
		return initialized ? description->pin_count : 0;
	}

	uint32 IntelGPIOCommunity::Revision() const {
		return initialized ? revision : 0;
	}

	uint32 IntelGPIOCommunity::PadBase() const {
		return initialized ? pad_base : 0;
	}

	IntelGPIOStatus IntelGPIOCommunity::InspectOwnership(stduint pin, IntelGPIOPinState& state) const {
		if (!initialized) return IntelGPIOStatus::InvalidArgument;
		if (pin >= description->pin_count) return IntelGPIOStatus::OutOfRange;
		const stduint group_pin = pin % description->group_size;
		const uint32 ownership_offset = description->pad_ownership_offset + uint32(pin / description->group_size) * description->ownership_registers_per_group * RegisterWidth + uint32(group_pin / 8) * RegisterWidth;
		uint32 ownership;
		uint32 configuration_lock;
		uint32 output_lock;
		uint32 software_ownership;
		if (!ReadRegister(ownership_offset, ownership) || !ReadRegister(GroupRegisterOffset(description->pad_configuration_lock_offset, pin, 8), configuration_lock) || !ReadRegister(GroupRegisterOffset(description->pad_configuration_lock_offset + RegisterWidth, pin, 8), output_lock) || !ReadRegister(GroupRegisterOffset(description->host_software_ownership_offset, pin, RegisterWidth), software_ownership)) return IntelGPIOStatus::AccessFailure;
		state.host_owned = ((ownership >> ((group_pin % 8) * 4)) & 0xFu) == 0;
		state.configuration_locked = (configuration_lock & (1u << group_pin)) != 0;
		state.output_locked = (output_lock & (1u << group_pin)) != 0;
		state.host_software_owned = (software_ownership & (1u << group_pin)) != 0;
		return IntelGPIOStatus::Success;
	}

	IntelGPIOStatus IntelGPIOCommunity::InspectPin(stduint pin, IntelGPIOPinState& state) const {
		state = {};
		auto status = InspectOwnership(pin, state);
		if (status != IntelGPIOStatus::Success) return status;
		const uint32 pad_offset = PadConfigurationOffset(pin);
		uint32 interrupt_status;
		uint32 interrupt_enable;
		if (!ReadRegister(pad_offset, state.pad_configuration0) || !ReadRegister(pad_offset + RegisterWidth, state.pad_configuration1) || !ReadRegister(GroupRegisterOffset(description->interrupt_status_offset, pin, RegisterWidth), interrupt_status) || !ReadRegister(GroupRegisterOffset(description->interrupt_enable_offset, pin, RegisterWidth), interrupt_enable)) return IntelGPIOStatus::AccessFailure;
		const uint32 pin_mask = 1u << (pin % description->group_size);
		state.input_value = (state.pad_configuration0 & PadConfiguration0ReceiveState) != 0;
		state.output_value = (state.pad_configuration0 & PadConfiguration0TransmitState) != 0;
		state.interrupt_pending = (interrupt_status & pin_mask) != 0;
		state.interrupt_enabled = (interrupt_enable & pin_mask) != 0;
		return IntelGPIOStatus::Success;
	}

	IntelGPIOStatus IntelGPIOCommunity::RequireWritablePin(stduint pin, bool output_only) const {
		IntelGPIOPinState state{};
		auto status = InspectOwnership(pin, state);
		if (status != IntelGPIOStatus::Success) return status;
		if (!state.host_owned) return IntelGPIOStatus::NotHostOwned;
		if (output_only ? state.output_locked : state.configuration_locked) return output_only ? IntelGPIOStatus::OutputLocked : IntelGPIOStatus::ConfigurationLocked;
		return IntelGPIOStatus::Success;
	}

	IntelGPIOStatus IntelGPIOCommunity::ReadPin(stduint pin, bool& value) const {
		value = false;
		IntelGPIOPinState state{};
		auto status = InspectPin(pin, state);
		if (status != IntelGPIOStatus::Success) return status;
		if (!state.host_owned) return IntelGPIOStatus::NotHostOwned;
		value = state.input_value;
		return IntelGPIOStatus::Success;
	}

	IntelGPIOStatus IntelGPIOCommunity::WritePin(stduint pin, bool value) {
		auto status = RequireWritablePin(pin, true);
		if (status != IntelGPIOStatus::Success) return status;
		const uint32 offset = PadConfigurationOffset(pin);
		uint32 configuration;
		if (!ReadRegister(offset, configuration)) return IntelGPIOStatus::AccessFailure;
		configuration = value ? configuration | PadConfiguration0TransmitState : configuration & ~PadConfiguration0TransmitState;
		return WriteRegister(offset, configuration) ? IntelGPIOStatus::Success : IntelGPIOStatus::AccessFailure;
	}

	IntelGPIOStatus IntelGPIOCommunity::ConfigurePin(stduint pin, GPIOMode mode) {
		if (mode == GPIOMode::OUT_OpenDrain) return IntelGPIOStatus::Unsupported;
		if (mode != GPIOMode::IN_Floating && mode != GPIOMode::IN_Pull && mode != GPIOMode::OUT_PushPull) return IntelGPIOStatus::Unsupported;
		auto status = RequireWritablePin(pin, false);
		if (status != IntelGPIOStatus::Success) return status;
		const uint32 offset = PadConfigurationOffset(pin);
		uint32 configuration;
		if (!ReadRegister(offset, configuration)) return IntelGPIOStatus::AccessFailure;
		configuration &= ~PadConfiguration0ModeMask;
		if (mode == GPIOMode::OUT_PushPull) configuration = (configuration | PadConfiguration0ReceiveDisable) & ~PadConfiguration0TransmitDisable;
		else configuration = (configuration & ~PadConfiguration0ReceiveDisable) | PadConfiguration0TransmitDisable;
		return WriteRegister(offset, configuration) ? IntelGPIOStatus::Success : IntelGPIOStatus::AccessFailure;
	}

	IntelGPIOStatus IntelGPIOCommunity::IsInput(stduint pin, bool& input) const {
		input = false;
		IntelGPIOPinState state{};
		auto status = InspectPin(pin, state);
		if (status != IntelGPIOStatus::Success) return status;
		input = (state.pad_configuration0 & PadConfiguration0ReceiveDisable) == 0;
		return IntelGPIOStatus::Success;
	}

	IntelGPIOStatus IntelGPIOCommunity::SetPull(stduint pin, bool pull_up) {
		auto status = RequireWritablePin(pin, false);
		if (status != IntelGPIOStatus::Success) return status;
		const uint32 offset = PadConfigurationOffset(pin) + RegisterWidth;
		uint32 configuration;
		if (!ReadRegister(offset, configuration)) return IntelGPIOStatus::AccessFailure;
		configuration = (configuration & ~(PadConfiguration1TerminationMask | PadConfiguration1TerminationUp)) | PadConfiguration1Termination5K;
		if (pull_up) configuration |= PadConfiguration1TerminationUp;
		return WriteRegister(offset, configuration) ? IntelGPIOStatus::Success : IntelGPIOStatus::AccessFailure;
	}

	IntelGPIOStatus IntelGPIOCommunity::ConfigureInterrupt(stduint pin, const GPIOInterruptConfiguration& configuration) {
		if (configuration.debounce_timeout) return IntelGPIOStatus::Unsupported;
		if (configuration.polarity == GPIOInterruptPolarity::ActiveBoth && configuration.trigger != GPIOInterruptTrigger::Edge) return IntelGPIOStatus::Unsupported;
		auto status = RequireWritablePin(pin, false);
		if (status != IntelGPIOStatus::Success) return status;
		IntelGPIOPinState state{};
		status = InspectOwnership(pin, state);
		if (status != IntelGPIOStatus::Success) return status;
		if (!state.host_software_owned) return IntelGPIOStatus::FirmwareOwned;
		const uint32 offset = PadConfigurationOffset(pin);
		uint32 value;
		if (!ReadRegister(offset, value)) return IntelGPIOStatus::AccessFailure;
		value &= ~(PadConfiguration0ModeMask | PadConfiguration0ReceiveEventMask | PadConfiguration0ReceiveInvert | PadConfiguration0RouteSci | PadConfiguration0RouteSmi | PadConfiguration0RouteNmi | PadConfiguration0ReceiveDisable);
		value |= PadConfiguration0TransmitDisable | PadConfiguration0RouteIoApic;
		if (configuration.polarity == GPIOInterruptPolarity::ActiveBoth) value |= PadConfiguration0ReceiveEventBoth;
		else {
			value |= configuration.trigger == GPIOInterruptTrigger::Edge ? PadConfiguration0ReceiveEventEdge : PadConfiguration0ReceiveEventLevel;
			if (configuration.polarity == GPIOInterruptPolarity::ActiveLow) value |= PadConfiguration0ReceiveInvert;
		}
		if (!WriteRegister(offset, value)) return IntelGPIOStatus::AccessFailure;
		uint32 readback;
		const uint32 interrupt_configuration_mask = PadConfiguration0ModeMask | PadConfiguration0ReceiveEventMask | PadConfiguration0ReceiveInvert | PadConfiguration0RouteIoApic | PadConfiguration0RouteSci | PadConfiguration0RouteSmi | PadConfiguration0RouteNmi | PadConfiguration0ReceiveDisable | PadConfiguration0TransmitDisable;
		if (!ReadRegister(offset, readback) || ((readback ^ value) & interrupt_configuration_mask)) return IntelGPIOStatus::AccessFailure;
		return IntelGPIOStatus::Success;
	}

	IntelGPIOStatus IntelGPIOCommunity::EnableInterrupt(stduint pin, bool enable) {
		if (!initialized) return IntelGPIOStatus::InvalidArgument;
		if (pin >= description->pin_count) return IntelGPIOStatus::OutOfRange;
		const uint32 bit = 1u << (pin % description->group_size);
		const uint32 status_offset = GroupRegisterOffset(description->interrupt_status_offset, pin, RegisterWidth);
		const uint32 enable_offset = GroupRegisterOffset(description->interrupt_enable_offset, pin, RegisterWidth);
		uint32 value;
		if (!WriteRegister(status_offset, bit) || !ReadRegister(enable_offset, value)) return IntelGPIOStatus::AccessFailure;
		value = enable ? value | bit : value & ~bit;
		if (!WriteRegister(enable_offset, value) || !ReadRegister(enable_offset, value)) return IntelGPIOStatus::AccessFailure;
		return ((value & bit) != 0) == enable ? IntelGPIOStatus::Success : IntelGPIOStatus::AccessFailure;
	}

	IntelGPIOStatus IntelGPIOCommunity::InterruptPending(stduint pin, bool& pending) const {
		pending = false;
		if (!initialized) return IntelGPIOStatus::InvalidArgument;
		if (pin >= description->pin_count) return IntelGPIOStatus::OutOfRange;
		uint32 value;
		if (!ReadRegister(GroupRegisterOffset(description->interrupt_status_offset, pin, RegisterWidth), value)) return IntelGPIOStatus::AccessFailure;
		pending = (value & (1u << (pin % description->group_size))) != 0;
		return IntelGPIOStatus::Success;
	}

	IntelGPIOStatus IntelGPIOCommunity::AcknowledgeInterrupt(stduint pin) {
		if (!initialized) return IntelGPIOStatus::InvalidArgument;
		if (pin >= description->pin_count) return IntelGPIOStatus::OutOfRange;
		return WriteRegister(GroupRegisterOffset(description->interrupt_status_offset, pin, RegisterWidth), 1u << (pin % description->group_size)) ? IntelGPIOStatus::Success : IntelGPIOStatus::AccessFailure;
	}
}

#endif
