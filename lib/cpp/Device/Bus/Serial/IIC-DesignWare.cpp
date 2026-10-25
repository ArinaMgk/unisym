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
		constexpr uint32 ControlRestart = 1u << 5;
		constexpr uint32 ControlSlaveDisable = 1u << 6;
		constexpr uint32 DataCommandRead = 1u << 8;
		constexpr uint32 DataCommandStop = 1u << 9;
		constexpr uint32 DataCommandRestart = 1u << 10;
		constexpr uint32 RawInterruptTransmitAbort = 1u << 6;
		constexpr uint32 RawInterruptStopDetected = 1u << 9;
		constexpr uint32 StatusTransmitFifoNotFull = 1u << 1;
		constexpr uint32 StatusReceiveFifoNotEmpty = 1u << 3;

		bool CalculateSclCount(uint32 input_clock_hz, uint32 period_ns, uint32 adjustment, uint32& count) {
			const uint64 clocks = (uint64(input_clock_hz) * period_ns + 500000000u) / 1000000000u;
			if (clocks <= adjustment || clocks - adjustment > 0xFFFFu) return false;
			count = uint32(clocks - adjustment);
			return true;
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
		if (!input_clock_hz || (bus_speed_hz != 100000u && bus_speed_hz != 400000u) || !timeout || !write_register) return IICDesignWareStatus::InvalidArgument;
		IICDesignWareCapabilities capabilities{};
		if (!Probe(capabilities)) return IICDesignWareStatus::ComponentMismatch;

		uint32 standard_high = 0;
		uint32 standard_low = 0;
		uint32 fast_high = 0;
		uint32 fast_low = 0;
		if (!CalculateSclCount(input_clock_hz, 4300u, 3u, standard_high) ||
			!CalculateSclCount(input_clock_hz, 5000u, 1u, standard_low) ||
			!CalculateSclCount(input_clock_hz, 900u, 3u, fast_high) ||
			!CalculateSclCount(input_clock_hz, 1600u, 1u, fast_low)) return IICDesignWareStatus::InvalidArgument;

		WriteRegister(IICDesignWareReg::Enable, 0);
		stduint remaining = timeout;
		while ((ReadRegister(IICDesignWareReg::EnableStatus) & 1u) && --remaining) {}
		if (ReadRegister(IICDesignWareReg::EnableStatus) & 1u) return IICDesignWareStatus::DisableTimeout;

		const uint32 speed = bus_speed_hz == 100000u ? ControlSpeedStandard : ControlSpeedFast;
		WriteRegister(IICDesignWareReg::Control, ControlMaster | speed | ControlRestart | ControlSlaveDisable);
		WriteRegister(IICDesignWareReg::StandardSclHighCount, standard_high);
		WriteRegister(IICDesignWareReg::StandardSclLowCount, standard_low);
		WriteRegister(IICDesignWareReg::FastSclHighCount, fast_high);
		WriteRegister(IICDesignWareReg::FastSclLowCount, fast_low);
		WriteRegister(IICDesignWareReg::ReceiveFifoThreshold, 0);
		WriteRegister(IICDesignWareReg::TransmitFifoThreshold, 0);
		WriteRegister(IICDesignWareReg::InterruptMask, 0);
		WriteRegister(IICDesignWareReg::Enable, 1);
		remaining = timeout;
		while (!(ReadRegister(IICDesignWareReg::EnableStatus) & 1u) && --remaining) {}
		return ReadRegister(IICDesignWareReg::EnableStatus) & 1u ? IICDesignWareStatus::Success : IICDesignWareStatus::EnableTimeout;
	}

	IICDesignWareStatus IIC_DESIGNWARE::Transfer(uint16 address, const byte* transmit, stduint transmit_length, byte* receive, stduint receive_length, stduint timeout, uint32* abort_source) {
		if (abort_source) *abort_source = 0;
		if (!address || address > 0x7Fu || (!transmit_length && !receive_length) || (transmit_length && !transmit) || (receive_length && !receive) || !timeout || !write_register) return IICDesignWareStatus::InvalidArgument;
		IICDesignWareCapabilities capabilities{};
		if (!Probe(capabilities)) return IICDesignWareStatus::ComponentMismatch;

		auto CheckAbort = [&]() -> bool {
			if (!(ReadRegister(IICDesignWareReg::RawInterruptStatus) & RawInterruptTransmitAbort)) return false;
			const uint32 source = ReadRegister(IICDesignWareReg::TransmitAbortSource);
			if (abort_source) *abort_source = source;
			ReadRegister(IICDesignWareReg::ClearTransmitAbort);
			return true;
		};
		auto ConsumeBudget = [&]() -> bool {
			if (!timeout) return false;
			--timeout;
			return timeout != 0;
		};

		WriteRegister(IICDesignWareReg::Enable, 0);
		while (ReadRegister(IICDesignWareReg::EnableStatus) & 1u) {
			if (!ConsumeBudget()) return IICDesignWareStatus::DisableTimeout;
		}
		WriteRegister(IICDesignWareReg::TargetAddress, address);
		WriteRegister(IICDesignWareReg::InterruptMask, 0);
		ReadRegister(IICDesignWareReg::ClearInterrupt);
		WriteRegister(IICDesignWareReg::Enable, 1);
		while (!(ReadRegister(IICDesignWareReg::EnableStatus) & 1u)) {
			if (!ConsumeBudget()) return IICDesignWareStatus::EnableTimeout;
		}

		for (stduint index = 0; index < transmit_length; ++index) {
			while (!(ReadRegister(IICDesignWareReg::Status) & StatusTransmitFifoNotFull)) {
				if (CheckAbort()) return IICDesignWareStatus::TransmitAbort;
				if (!ConsumeBudget()) return IICDesignWareStatus::TransferTimeout;
			}
			uint32 command = transmit[index];
			if (index + 1 == transmit_length && !receive_length) command |= DataCommandStop;
			WriteRegister(IICDesignWareReg::DataCommand, command);
			if (CheckAbort()) return IICDesignWareStatus::TransmitAbort;
		}

		stduint read_commands = 0;
		stduint received = 0;
		stduint outstanding = 0;
		while (received < receive_length) {
			if (CheckAbort()) return IICDesignWareStatus::TransmitAbort;
			bool progressed = false;
			uint32 status = ReadRegister(IICDesignWareReg::Status);
			while (read_commands < receive_length && outstanding < capabilities.receive_fifo_depth && (status & StatusTransmitFifoNotFull)) {
				uint32 command = DataCommandRead;
				if (!read_commands && transmit_length) command |= DataCommandRestart;
				if (read_commands + 1 == receive_length) command |= DataCommandStop;
				WriteRegister(IICDesignWareReg::DataCommand, command);
				++read_commands;
				++outstanding;
				progressed = true;
				if (CheckAbort()) return IICDesignWareStatus::TransmitAbort;
				status = ReadRegister(IICDesignWareReg::Status);
			}
			while (received < receive_length && (ReadRegister(IICDesignWareReg::Status) & StatusReceiveFifoNotEmpty)) {
				receive[received++] = byte(ReadRegister(IICDesignWareReg::DataCommand));
				if (outstanding) --outstanding;
				progressed = true;
			}
			if (!progressed && !ConsumeBudget()) return IICDesignWareStatus::TransferTimeout;
		}

		while (!(ReadRegister(IICDesignWareReg::RawInterruptStatus) & RawInterruptStopDetected)) {
			if (CheckAbort()) return IICDesignWareStatus::TransmitAbort;
			if (!ConsumeBudget()) return IICDesignWareStatus::TransferTimeout;
		}
		ReadRegister(IICDesignWareReg::ClearStopDetection);
		return IICDesignWareStatus::Success;
	}

}
#endif
