// UTF-8 C/C++11 TAB4 CRLF
// Docutitle: (Device.Bus.Serial) Synopsys DesignWare I2C
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../../inc/cpp/Device/IIC"

#if defined(_MCCA) && ((_MCCA & 0xFF00) == 0x8600)
namespace {

	uint32 ReadDesignWareMmio(void* context, stduint offset) {
		return *reinterpret_cast<volatile uint32*>(reinterpret_cast<stduint>(context) + offset);
	}

	void WriteDesignWareMmio(void* context, stduint offset, uint32 value) {
		*reinterpret_cast<volatile uint32*>(reinterpret_cast<stduint>(context) + offset) = value;
	}

}

namespace uni {
	namespace {
		constexpr uint32 ControlMaster = 1u << 0;
		constexpr uint32 ControlSpeedStandard = 1u << 1;
		constexpr uint32 ControlSpeedFast = 2u << 1;
		constexpr uint32 Control10BitAddressMaster = 1u << 4;
		constexpr uint32 ControlRestart = 1u << 5;
		constexpr uint32 ControlSlaveDisable = 1u << 6;
		constexpr uint32 TargetAddress10Bit = 1u << 12;
		constexpr uint32 DataCommandRead = 1u << 8;
		constexpr uint32 DataCommandStop = 1u << 9;
		constexpr uint32 DataCommandRestart = 1u << 10;
		constexpr uint32 EnableController = 1u << 0;
		constexpr uint32 EnableAbort = 1u << 1;
		constexpr uint32 RawInterruptTransmitAbort = 1u << 6;
		constexpr uint32 RawInterruptStopDetected = 1u << 9;
		constexpr uint32 StatusActivity = 1u << 0;
		constexpr uint32 StatusTransmitFifoNotFull = 1u << 1;
		constexpr uint32 StatusTransmitFifoEmpty = 1u << 2;
		constexpr uint32 StatusReceiveFifoNotEmpty = 1u << 3;
		constexpr uint32 AbortAddressNack = (1u << 0) | (1u << 1) | (1u << 2) | (1u << 4);
		constexpr uint32 AbortDataNack = 1u << 3;
		constexpr uint32 AbortArbitrationLost = (1u << 12) | (1u << 14);
		constexpr uint16 SupportedMessageFlags = IICMessageFlag_Read | IICMessageFlag_TenBitAddress | IICMessageFlag_Stop;

		bool CalculateSclCount(uint32 input_clock_hz, uint32 period_ns, uint32 adjustment, uint32& count) {
			const uint64 clocks = (uint64(input_clock_hz) * period_ns + 500000000u) / 1000000000u;
			if (clocks <= adjustment || clocks - adjustment > 0xFFFFu) return false;
			count = uint32(clocks - adjustment);
			return true;
		}

		IICTransferStatus DecodeAbortSource(uint32 source) {
			if (source & AbortAddressNack) return IICTransferStatus::AddressNack;
			if (source & AbortDataNack) return IICTransferStatus::DataNack;
			if (source & AbortArbitrationLost) return IICTransferStatus::ArbitrationLost;
			return IICTransferStatus::ControllerError;
		}

		bool ConsumeBudget(stduint& remaining) {
			if (!remaining) return false;
			--remaining;
			return true;
		}

		bool WaitRegister(IIC_DESIGNWARE& controller, IICDesignWareReg reg, uint32 mask, uint32 expected, stduint& remaining) {
			while ((controller.ReadRegister(reg) & mask) != expected) {
				if (!ConsumeBudget(remaining)) return false;
			}
			return true;
		}

		bool ClearControllerState(IIC_DESIGNWARE& controller, stduint& remaining) {
			while (controller.ReadRegister(IICDesignWareReg::Status) & StatusReceiveFifoNotEmpty) {
				controller.ReadRegister(IICDesignWareReg::DataCommand);
				if (!ConsumeBudget(remaining)) return false;
			}
			controller.ReadRegister(IICDesignWareReg::ClearInterrupt);
			return true;
		}

		IICDesignWareStatus ConfigureController(IIC_DESIGNWARE& controller, const IICDesignWareInitialization& initialization, IICDesignWareCapabilities& capabilities) {
			const uint32 input_clock_hz = initialization.input_clock_hz;
			const uint32 bus_speed_hz = initialization.bus_speed_hz;
			if (!input_clock_hz || (bus_speed_hz != 100000u && bus_speed_hz != 400000u) || !initialization.timeout) return IICDesignWareStatus::InvalidArgument;
			if (!controller.Probe(capabilities) || !capabilities.transmit_fifo_depth || !capabilities.receive_fifo_depth) return IICDesignWareStatus::ComponentMismatch;

			uint32 standard_high = 0;
			uint32 standard_low = 0;
			uint32 fast_high = 0;
			uint32 fast_low = 0;
			if (!CalculateSclCount(input_clock_hz, 4300u, 3u, standard_high) || !CalculateSclCount(input_clock_hz, 5000u, 1u, standard_low) || !CalculateSclCount(input_clock_hz, 900u, 3u, fast_high) || !CalculateSclCount(input_clock_hz, 1600u, 1u, fast_low)) return IICDesignWareStatus::InvalidArgument;

			controller.WriteRegister(IICDesignWareReg::InterruptMask, 0);
			controller.WriteRegister(IICDesignWareReg::Enable, 0);
			stduint remaining = initialization.timeout;
			if (!WaitRegister(controller, IICDesignWareReg::EnableStatus, EnableController, 0, remaining)) return IICDesignWareStatus::DisableTimeout;
			if (!ClearControllerState(controller, remaining)) return IICDesignWareStatus::DisableTimeout;

			const uint32 speed = bus_speed_hz == 100000u ? ControlSpeedStandard : ControlSpeedFast;
			controller.WriteRegister(IICDesignWareReg::Control, ControlMaster | speed | ControlRestart | ControlSlaveDisable);
			controller.WriteRegister(IICDesignWareReg::StandardSclHighCount, standard_high);
			controller.WriteRegister(IICDesignWareReg::StandardSclLowCount, standard_low);
			controller.WriteRegister(IICDesignWareReg::FastSclHighCount, fast_high);
			controller.WriteRegister(IICDesignWareReg::FastSclLowCount, fast_low);
			controller.WriteRegister(IICDesignWareReg::ReceiveFifoThreshold, 0);
			controller.WriteRegister(IICDesignWareReg::TransmitFifoThreshold, 0);
			controller.WriteRegister(IICDesignWareReg::InterruptMask, 0);
			controller.WriteRegister(IICDesignWareReg::Enable, EnableController);
			remaining = initialization.timeout;
			return WaitRegister(controller, IICDesignWareReg::EnableStatus, EnableController, EnableController, remaining) ? IICDesignWareStatus::Success : IICDesignWareStatus::EnableTimeout;
		}

		bool RecoverController(IIC_DESIGNWARE& controller, const IICDesignWareInitialization& initialization, IICDesignWareCapabilities& capabilities) {
			controller.WriteRegister(IICDesignWareReg::InterruptMask, 0);
			if (controller.ReadRegister(IICDesignWareReg::Status) & StatusActivity) {
				controller.WriteRegister(IICDesignWareReg::Enable, EnableController | EnableAbort);
				stduint abort_remaining = initialization.timeout;
				while ((controller.ReadRegister(IICDesignWareReg::Status) & StatusActivity) && ConsumeBudget(abort_remaining)) {}
			}

			controller.WriteRegister(IICDesignWareReg::Enable, 0);
			stduint remaining = initialization.timeout;
			if (!WaitRegister(controller, IICDesignWareReg::EnableStatus, EnableController, 0, remaining)) return false;
			if ((controller.ReadRegister(IICDesignWareReg::Status) & StatusActivity) && initialization.recover_bus) {
				if (!initialization.recover_bus(initialization.recovery_context, initialization.timeout)) return false;
				remaining = initialization.timeout;
				if (!WaitRegister(controller, IICDesignWareReg::Status, StatusActivity, 0, remaining)) return false;
			}
			remaining = initialization.timeout;
			if (!ClearControllerState(controller, remaining)) return false;
			return ConfigureController(controller, initialization, capabilities) == IICDesignWareStatus::Success;
		}
	}

	IIC_DESIGNWARE::IIC_DESIGNWARE(stduint _baseaddr) : baseaddr(_baseaddr), register_context(reinterpret_cast<void*>(_baseaddr)), read_register(_baseaddr ? ReadDesignWareMmio : nullptr), write_register(_baseaddr ? WriteDesignWareMmio : nullptr) {}

	IIC_DESIGNWARE::IIC_DESIGNWARE(void* context, IICDesignWareReadRegister read, IICDesignWareWriteRegister write) : baseaddr(0), register_context(context), read_register(read), write_register(write) {}

	stduint IIC_DESIGNWARE::getAddress() const {
		return baseaddr;
	}

	uint32 IIC_DESIGNWARE::ReadRegister(IICDesignWareReg reg) const {
		return read_register ? read_register(register_context, _IMM(reg)) : 0xFFFFFFFFu;
	}

	void IIC_DESIGNWARE::WriteRegister(IICDesignWareReg reg, uint32 value) {
		if (write_register) write_register(register_context, _IMM(reg), value);
	}

	bool IIC_DESIGNWARE::Probe(IICDesignWareCapabilities& capabilities) const {
		capabilities = {};
		if (!read_register) return false;
		capabilities.component_type = ReadRegister(IICDesignWareReg::ComponentType);
		if (capabilities.component_type != ComponentTypeValue) return false;
		capabilities.component_version = ReadRegister(IICDesignWareReg::ComponentVersion);
		capabilities.component_parameter = ReadRegister(IICDesignWareReg::ComponentParameter);
		capabilities.transmit_fifo_depth = uint16(((capabilities.component_parameter >> 16) & 0xFFu) + 1u);
		capabilities.receive_fifo_depth = uint16(((capabilities.component_parameter >> 8) & 0xFFu) + 1u);
		return true;
	}

	IICDesignWareStatus IIC_DESIGNWARE::Initialize(uint32 input_clock_hz, uint32 bus_speed_hz, stduint timeout) {
		const IICDesignWareInitialization initialization{ input_clock_hz, bus_speed_hz, timeout, nullptr, nullptr };
		return Initialize(initialization);
	}

	IICDesignWareStatus IIC_DESIGNWARE::Initialize(const IICDesignWareInitialization& initialization) {
		initialized = false;
		if (!read_register || !write_register) return IICDesignWareStatus::InvalidArgument;
		IICDesignWareCapabilities capabilities{};
		const auto status = ConfigureController(*this, initialization, capabilities);
		if (status != IICDesignWareStatus::Success) return status;
		runtime_initialization = initialization;
		runtime_capabilities = capabilities;
		initialized = true;
		return IICDesignWareStatus::Success;
	}

	IICTransferStatus IIC_DESIGNWARE::TransferPolling(IICMessage* messages, stduint message_count, stduint timeout, IICTransferResult& result, const IICDesignWareCapabilities& capabilities) {
		result = {};
		result.failed_message = message_count;
		if (!messages || !message_count || !timeout) return IICTransferStatus::InvalidArgument;
		stduint total_length = 0;
		for (stduint message_index = 0; message_index < message_count; ++message_index) {
			auto& message = messages[message_index];
			message.transferred = 0;
			result.failed_message = message_index;
			if ((message.flags & ~SupportedMessageFlags) || !message.length || message.length > ~stduint(0) - total_length) return IICTransferStatus::InvalidArgument;
			const bool read = message.flags & IICMessageFlag_Read;
			const bool ten_bit = message.flags & IICMessageFlag_TenBitAddress;
			if (message.address > (ten_bit ? 0x3FFu : 0x7Fu)) return IICTransferStatus::InvalidArgument;
			if ((read && (!message.read_buffer || message.write_buffer)) || (!read && (!message.write_buffer || message.read_buffer))) return IICTransferStatus::InvalidArgument;
			if (message_index && !(messages[message_index - 1].flags & IICMessageFlag_Stop)) {
				const auto& previous = messages[message_index - 1];
				if (previous.address != message.address || ((previous.flags ^ message.flags) & IICMessageFlag_TenBitAddress)) return IICTransferStatus::Unsupported;
			}
			total_length += message.length;
		}
		result.failed_message = message_count;

		stduint remaining = timeout;
		auto ConsumeTransferBudget = [&]() -> bool { return ConsumeBudget(remaining); };
		auto CheckAbort = [&](stduint message_index, IICTransferStatus& status) -> bool {
			if (!(ReadRegister(IICDesignWareReg::RawInterruptStatus) & RawInterruptTransmitAbort)) return false;
			result.hardware_status = ReadRegister(IICDesignWareReg::TransmitAbortSource);
			result.failed_message = message_index;
			ReadRegister(IICDesignWareReg::ClearTransmitAbort);
			status = DecodeAbortSource(result.hardware_status);
			return true;
		};

		stduint group_start = 0;
		while (group_start < message_count) {
			stduint group_end = group_start;
			while (group_end + 1 < message_count && !(messages[group_end].flags & IICMessageFlag_Stop)) ++group_end;
			result.failed_message = group_start;
			while (ReadRegister(IICDesignWareReg::Status) & StatusActivity) {
				if (!ConsumeTransferBudget()) return IICTransferStatus::BusBusy;
			}
			WriteRegister(IICDesignWareReg::Enable, 0);
			while (ReadRegister(IICDesignWareReg::EnableStatus) & 1u) {
				if (!ConsumeTransferBudget()) return IICTransferStatus::Timeout;
			}
			const bool ten_bit = messages[group_start].flags & IICMessageFlag_TenBitAddress;
			uint32 control = ReadRegister(IICDesignWareReg::Control) & ~Control10BitAddressMaster;
			if (ten_bit) control |= Control10BitAddressMaster;
			WriteRegister(IICDesignWareReg::Control, control);
			WriteRegister(IICDesignWareReg::TargetAddress, messages[group_start].address | (ten_bit ? TargetAddress10Bit : 0));
			WriteRegister(IICDesignWareReg::InterruptMask, 0);
			ReadRegister(IICDesignWareReg::ClearInterrupt);
			WriteRegister(IICDesignWareReg::Enable, 1);
			while (!(ReadRegister(IICDesignWareReg::EnableStatus) & 1u)) {
				if (!ConsumeTransferBudget()) return IICTransferStatus::Timeout;
			}

			for (stduint message_index = group_start; message_index <= group_end; ++message_index) {
				auto& message = messages[message_index];
				result.failed_message = message_index;
				const bool restart = message_index != group_start;
				const bool stop = message_index == group_end;
				if (!(message.flags & IICMessageFlag_Read)) {
					for (stduint byte_index = 0; byte_index < message.length; ++byte_index) {
						IICTransferStatus abort_status = IICTransferStatus::ControllerError;
						while (!(ReadRegister(IICDesignWareReg::Status) & StatusTransmitFifoNotFull)) {
							if (CheckAbort(message_index, abort_status)) return abort_status;
							if (!ConsumeTransferBudget()) return IICTransferStatus::Timeout;
						}
						uint32 command = message.write_buffer[byte_index];
						if (!byte_index && restart) command |= DataCommandRestart;
						if (byte_index + 1 == message.length && stop) command |= DataCommandStop;
						WriteRegister(IICDesignWareReg::DataCommand, command);
						++result.submitted_bytes;
						if (CheckAbort(message_index, abort_status)) return abort_status;
					}
					IICTransferStatus abort_status = IICTransferStatus::ControllerError;
					while (!(ReadRegister(IICDesignWareReg::Status) & StatusTransmitFifoEmpty)) {
						if (CheckAbort(message_index, abort_status)) return abort_status;
						if (!ConsumeTransferBudget()) return IICTransferStatus::Timeout;
					}
					if (CheckAbort(message_index, abort_status)) return abort_status;
					message.transferred = message.length;
					result.completed_bytes += message.length;
				}
				else {
					stduint read_commands = 0;
					stduint outstanding = 0;
					while (message.transferred < message.length) {
						IICTransferStatus abort_status = IICTransferStatus::ControllerError;
						if (CheckAbort(message_index, abort_status)) return abort_status;
						bool progressed = false;
						uint32 status = ReadRegister(IICDesignWareReg::Status);
						while (read_commands < message.length && outstanding < capabilities.receive_fifo_depth && (status & StatusTransmitFifoNotFull)) {
							uint32 command = DataCommandRead;
							if (!read_commands && restart) command |= DataCommandRestart;
							if (read_commands + 1 == message.length && stop) command |= DataCommandStop;
							WriteRegister(IICDesignWareReg::DataCommand, command);
							++read_commands;
							++outstanding;
							++result.submitted_bytes;
							progressed = true;
							if (CheckAbort(message_index, abort_status)) return abort_status;
							status = ReadRegister(IICDesignWareReg::Status);
						}
						while (message.transferred < message.length && (ReadRegister(IICDesignWareReg::Status) & StatusReceiveFifoNotEmpty)) {
							message.read_buffer[message.transferred++] = byte(ReadRegister(IICDesignWareReg::DataCommand));
							++result.completed_bytes;
							if (outstanding) --outstanding;
							progressed = true;
						}
						if (!progressed && !ConsumeTransferBudget()) return IICTransferStatus::Timeout;
					}
				}
				if (message_index != group_end) ++result.completed_messages;
			}

			while (!(ReadRegister(IICDesignWareReg::RawInterruptStatus) & RawInterruptStopDetected)) {
				IICTransferStatus abort_status = IICTransferStatus::ControllerError;
				if (CheckAbort(group_end, abort_status)) return abort_status;
				if (!ConsumeTransferBudget()) return IICTransferStatus::Timeout;
			}
			ReadRegister(IICDesignWareReg::ClearStopDetection);
			++result.completed_messages;
			group_start = group_end + 1;
		}
		result.failed_message = message_count;
		return IICTransferStatus::Success;
	}
	IICTransferStatus IIC_DESIGNWARE::Transfer(IICMessage* messages, stduint message_count, stduint timeout, IICTransferResult& result) {
		result = {};
		result.failed_message = message_count;
		if (!initialized) return IICTransferStatus::ControllerError;
		const auto status = TransferPolling(messages, message_count, timeout, result, runtime_capabilities);
		if (status == IICTransferStatus::Success || status == IICTransferStatus::InvalidArgument || status == IICTransferStatus::Unsupported) return status;
		IICDesignWareCapabilities recovered_capabilities{};
		if (!RecoverController(*this, runtime_initialization, recovered_capabilities)) initialized = false;
		else runtime_capabilities = recovered_capabilities;
		return status;
	}

	IICDesignWareStatus IIC_DESIGNWARE::Transfer(uint16 address, const byte* transmit, stduint transmit_length, byte* receive, stduint receive_length, stduint timeout, uint32* abort_source) {
		if (abort_source) *abort_source = 0;
		IICMessage messages[2]{};
		stduint message_count = 0;
		if (transmit_length) messages[message_count++] = { address, receive_length ? IICMessageFlag_None : IICMessageFlag_Stop, transmit, nullptr, transmit_length, 0 };
		if (receive_length) messages[message_count++] = { address, uint16(IICMessageFlag_Read | IICMessageFlag_Stop), nullptr, receive, receive_length, 0 };
		IICTransferResult result{};
		const auto status = Transfer(messages, message_count, timeout, result);
		if (abort_source) *abort_source = result.hardware_status;
		switch (status) {
		case IICTransferStatus::Success: return IICDesignWareStatus::Success;
		case IICTransferStatus::InvalidArgument:
		case IICTransferStatus::Unsupported: return IICDesignWareStatus::InvalidArgument;
		case IICTransferStatus::AddressNack:
		case IICTransferStatus::DataNack:
		case IICTransferStatus::ArbitrationLost: return IICDesignWareStatus::TransmitAbort;
		case IICTransferStatus::BusBusy:
		case IICTransferStatus::Timeout: return IICDesignWareStatus::TransferTimeout;
		case IICTransferStatus::ControllerError: return result.hardware_status ? IICDesignWareStatus::TransmitAbort : IICDesignWareStatus::ComponentMismatch;
		default: return IICDesignWareStatus::TransferTimeout;
		}
	}

}
#endif
