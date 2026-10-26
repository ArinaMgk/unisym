// UTF-8 CPP-ISO11 TAB4 LF
// Docutitle: Human Interface Device over I2C
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../../inc/cpp/Device/Bus/Serial/IIC-HID.hpp"
#include "../../../../../inc/cpp/trait/MallocTrait.hpp"

namespace uni {

	namespace {
		constexpr uint8 HIDI2CCommandReset = 1;
		constexpr uint8 HIDI2CCommandGetReport = 2;
		constexpr uint8 HIDI2CCommandSetReport = 3;
		constexpr uint8 HIDI2CCommandSetPower = 8;
		constexpr stduint HIDI2CTransferBufferLength = HIDI2CMaximumTransferLength + 16;

		void ResetResult(HIDI2CResult& result) {
			result = {};
			result.status = HIDI2CStatus::InvalidArgument;
			result.transfer_status = IICTransferStatus::InvalidArgument;
			result.descriptor_status = HIDI2CDescriptorStatus::InvalidArgument;
		}

		HIDI2CStatus SetStatus(HIDI2CResult& result, HIDI2CStatus status, stduint length = 0) {
			result.status = status;
			result.length = length;
			return status;
		}

		void CopyBytes(byte* destination, const byte* source, stduint length) {
			for0(index, length) destination[index] = source[index];
		}

		stduint EncodeCommand(byte* destination, uint8 opcode, HIDI2CReportType report_type, uint8 report_id) {
			destination[0] = byte(uint8(report_type) << 4) | byte(report_id < 15 ? report_id : 15);
			destination[1] = opcode;
			if (report_id < 15) return 2;
			destination[2] = report_id;
			return 3;
		}
	}

	uint16 DecodeHIDI2CWord(const byte* source) {
		return uint16(source[0]) | uint16(uint16(source[1]) << 8);
	}

	HIDI2CDescriptorStatus DecodeHIDI2CDescriptor(const byte* source, stduint length, HIDI2CDescriptor& descriptor) {
		descriptor = {};
		if (!source) return HIDI2CDescriptorStatus::InvalidArgument;
		if (length < HIDI2CDescriptorLength) return HIDI2CDescriptorStatus::InvalidLength;
		descriptor.descriptor_length = DecodeHIDI2CWord(source + 0);
		descriptor.version = DecodeHIDI2CWord(source + 2);
		descriptor.report_descriptor_length = DecodeHIDI2CWord(source + 4);
		descriptor.report_descriptor_register = DecodeHIDI2CWord(source + 6);
		descriptor.input_register = DecodeHIDI2CWord(source + 8);
		descriptor.maximum_input_length = DecodeHIDI2CWord(source + 10);
		descriptor.output_register = DecodeHIDI2CWord(source + 12);
		descriptor.maximum_output_length = DecodeHIDI2CWord(source + 14);
		descriptor.command_register = DecodeHIDI2CWord(source + 16);
		descriptor.data_register = DecodeHIDI2CWord(source + 18);
		descriptor.vendor_id = DecodeHIDI2CWord(source + 20);
		descriptor.product_id = DecodeHIDI2CWord(source + 22);
		descriptor.device_version = DecodeHIDI2CWord(source + 24);
		descriptor.reserved = uint32(source[26]) | uint32(source[27]) << 8 | uint32(source[28]) << 16 | uint32(source[29]) << 24;
		if (descriptor.descriptor_length != HIDI2CDescriptorLength) return HIDI2CDescriptorStatus::InvalidLength;
		if (descriptor.version != HIDI2CProtocolVersion) return HIDI2CDescriptorStatus::UnsupportedVersion;
		if (!descriptor.report_descriptor_length || descriptor.report_descriptor_length > HIDI2CMaximumReportDescriptorLength) return HIDI2CDescriptorStatus::InvalidReportDescriptor;
		if (descriptor.report_descriptor_register == 0xFFFFu || descriptor.input_register == 0xFFFFu || descriptor.command_register == 0xFFFFu || descriptor.data_register == 0xFFFFu || (descriptor.maximum_output_length && descriptor.output_register == 0xFFFFu)) return HIDI2CDescriptorStatus::InvalidRegister;
		if (descriptor.maximum_input_length < 2 || descriptor.maximum_input_length > HIDI2CMaximumTransferLength) return HIDI2CDescriptorStatus::InvalidInputLength;
		if (descriptor.maximum_output_length && (descriptor.maximum_output_length < 2 || descriptor.maximum_output_length > HIDI2CMaximumTransferLength)) return HIDI2CDescriptorStatus::InvalidOutputLength;
		return HIDI2CDescriptorStatus::Success;
	}

	HIDI2CDevice::HIDI2CDevice(const HIDI2CTransport& transport, trait::Malloc* allocator) : transport(transport), allocator(allocator) {}

	HIDI2CDevice::~HIDI2CDevice() {
		Release();
	}

	void HIDI2CDevice::Release() {
		report_layout.Clear();
		if (report_descriptor && allocator) allocator->deallocate(report_descriptor, descriptor.report_descriptor_length);
		if (transfer_buffer && allocator) allocator->deallocate(transfer_buffer, HIDI2CTransferBufferLength);
		report_descriptor = nullptr;
		transfer_buffer = nullptr;
		descriptor = {};
		initialized = false;
		report_descriptor_ready = false;
		reset_pending = false;
	}

	HIDI2CStatus HIDI2CDevice::Transfer(IICMessage* messages, stduint count, stduint timeout, HIDI2CResult& result) {
		result.transfer = {};
		result.transfer.failed_message = count;
		result.transfer_status = IICTransferStatus::InvalidArgument;
		if (!transport.transfer || !messages || !count || !timeout) return SetStatus(result, HIDI2CStatus::InvalidArgument);
		for0(index, count) messages[index].transferred = 0;
		result.transfer_status = transport.transfer(transport.context, messages, count, timeout, result.transfer);
		if (result.transfer_status != IICTransferStatus::Success) return SetStatus(result, HIDI2CStatus::TransferFailed);
		for0(index, count) {
			if (messages[index].transferred != messages[index].length) return SetStatus(result, HIDI2CStatus::ShortTransfer);
		}
		return SetStatus(result, HIDI2CStatus::Success);
	}

	HIDI2CStatus HIDI2CDevice::ReadRegister(uint16 register_address, byte* destination, stduint length, stduint timeout, HIDI2CResult& result) {
		if (!destination || !length) return SetStatus(result, HIDI2CStatus::InvalidArgument);
		byte address[2] = { byte(register_address), byte(register_address >> 8) };
		IICMessage messages[2] = {
			{ transport.address, transport.address_flags, address, nullptr, numsof(address), 0 },
			{ transport.address, uint16(transport.address_flags | IICMessageFlag_Read | IICMessageFlag_Stop), nullptr, destination, length, 0 },
		};
		const auto status = Transfer(messages, numsof(messages), timeout, result);
		if (status == HIDI2CStatus::Success) result.length = length;
		return status;
	}

	HIDI2CStatus HIDI2CDevice::WriteCommand(uint8 opcode, HIDI2CReportType report_type, uint8 report_id, const byte* payload, stduint length, bool read, byte* destination, stduint capacity, stduint timeout, HIDI2CResult& result) {
		if (!initialized || !transfer_buffer) return SetStatus(result, HIDI2CStatus::NotInitialized);
		if (uint8(report_type) < uint8(HIDI2CReportType::Input) || uint8(report_type) > uint8(HIDI2CReportType::Feature)) return SetStatus(result, HIDI2CStatus::InvalidArgument);
		if ((length && !payload) || (read && !destination) || length > HIDI2CMaximumTransferLength || capacity > HIDI2CMaximumTransferLength - 2) return SetStatus(result, HIDI2CStatus::InvalidArgument);
		stduint write_length = 0;
		transfer_buffer[write_length++] = byte(descriptor.command_register);
		transfer_buffer[write_length++] = byte(descriptor.command_register >> 8);
		write_length += EncodeCommand(transfer_buffer + write_length, opcode, report_type, report_id);
		transfer_buffer[write_length++] = byte(descriptor.data_register);
		transfer_buffer[write_length++] = byte(descriptor.data_register >> 8);
		if (read) {
			IICMessage messages[2] = {
				{ transport.address, transport.address_flags, transfer_buffer, nullptr, write_length, 0 },
				{ transport.address, uint16(transport.address_flags | IICMessageFlag_Read | IICMessageFlag_Stop), nullptr, transfer_buffer + 8, capacity + 2, 0 },
			};
			const auto status = Transfer(messages, numsof(messages), timeout, result);
			if (status != HIDI2CStatus::Success) return status;
			const auto accept_status = ParseInput(transfer_buffer + 8, capacity + 2, destination, capacity, result);
			if (accept_status == HIDI2CStatus::Success && report_id && (!result.length || destination[0] != report_id)) return SetStatus(result, HIDI2CStatus::UnexpectedReportId);
			return accept_status;
		}
		const stduint report_length = length + 2 + (report_id ? 1 : 0);
		if (report_length > HIDI2CMaximumTransferLength || write_length + report_length > HIDI2CTransferBufferLength) return SetStatus(result, HIDI2CStatus::InvalidReportLength);
		transfer_buffer[write_length++] = byte(report_length);
		transfer_buffer[write_length++] = byte(report_length >> 8);
		if (report_id) transfer_buffer[write_length++] = report_id;
		CopyBytes(transfer_buffer + write_length, payload, length);
		write_length += length;
		IICMessage message{ transport.address, uint16(transport.address_flags | IICMessageFlag_Stop), transfer_buffer, nullptr, write_length, 0 };
		const auto status = Transfer(&message, 1, timeout, result);
		if (status == HIDI2CStatus::Success) result.length = length;
		return status;
	}

	HIDI2CStatus HIDI2CDevice::Initialize(uint16 descriptor_register, stduint timeout, HIDI2CResult& result) {
		ResetResult(result);
		Release();
		if (!allocator || !transport.transfer || descriptor_register == 0xFFFFu || transport.address_flags & ~uint16(IICMessageFlag_TenBitAddress)) return SetStatus(result, HIDI2CStatus::InvalidArgument);
		const uint16 maximum_address = transport.address_flags & IICMessageFlag_TenBitAddress ? 0x03FFu : 0x007Fu;
		if (transport.address > maximum_address) return SetStatus(result, HIDI2CStatus::InvalidArgument);
		transfer_buffer = static_cast<byte*>(allocator->allocate(HIDI2CTransferBufferLength));
		if (!transfer_buffer) return SetStatus(result, HIDI2CStatus::AllocationFailed);
		const auto read_status = ReadRegister(descriptor_register, transfer_buffer, HIDI2CDescriptorLength, timeout, result);
		if (read_status != HIDI2CStatus::Success) {
			Release();
			return read_status;
		}
		result.descriptor_status = DecodeHIDI2CDescriptor(transfer_buffer, HIDI2CDescriptorLength, descriptor);
		if (result.descriptor_status != HIDI2CDescriptorStatus::Success) {
			Release();
			return SetStatus(result, HIDI2CStatus::InvalidDescriptor);
		}
		report_descriptor = static_cast<byte*>(allocator->allocate(descriptor.report_descriptor_length));
		if (!report_descriptor) {
			Release();
			return SetStatus(result, HIDI2CStatus::AllocationFailed);
		}
		initialized = true;
		return SetStatus(result, HIDI2CStatus::Success, HIDI2CDescriptorLength);
	}

	HIDI2CStatus HIDI2CDevice::ReadReportDescriptor(stduint timeout, HIDI2CResult& result) {
		ResetResult(result);
		if (!initialized || !report_descriptor) return SetStatus(result, HIDI2CStatus::NotInitialized);
		report_layout.Clear();
		report_descriptor_ready = false;
		const auto status = ReadRegister(descriptor.report_descriptor_register, report_descriptor, descriptor.report_descriptor_length, timeout, result);
		if (status == HIDI2CStatus::Success) report_descriptor_ready = true;
		return status;
	}

	HIDReportDescriptorStatus HIDI2CDevice::ParseReportDescriptor(HIDReportParseResult& result) {
		if (!initialized || !report_descriptor_ready || !report_descriptor || !allocator) {
			result = {};
			result.status = HIDReportDescriptorStatus::InvalidArgument;
			return result.status;
		}
		return report_layout.Parse(report_descriptor, descriptor.report_descriptor_length, allocator, result);
	}

	HIDI2CStatus HIDI2CDevice::ReadInput(byte* destination, stduint capacity, stduint timeout, HIDI2CResult& result) {
		ResetResult(result);
		if (!initialized || !transfer_buffer) return SetStatus(result, HIDI2CStatus::NotInitialized);
		if (!destination || !capacity) return SetStatus(result, HIDI2CStatus::InvalidArgument);
		IICMessage message{ transport.address, uint16(transport.address_flags | IICMessageFlag_Read | IICMessageFlag_Stop), nullptr, transfer_buffer, descriptor.maximum_input_length, 0 };
		const auto status = Transfer(&message, 1, timeout, result);
		if (status != HIDI2CStatus::Success) return status;
		return ParseInput(transfer_buffer, descriptor.maximum_input_length, destination, capacity, result);
	}

	HIDI2CStatus HIDI2CDevice::WriteOutput(uint8 report_id, const byte* source, stduint length, stduint timeout, HIDI2CResult& result) {
		ResetResult(result);
		if (!initialized || !transfer_buffer) return SetStatus(result, HIDI2CStatus::NotInitialized);
		if (!descriptor.maximum_output_length || descriptor.output_register == 0xFFFFu) return SetStatus(result, HIDI2CStatus::Unsupported);
		if ((length && !source) || length > HIDI2CMaximumTransferLength || length + 2 + (report_id ? 1 : 0) > descriptor.maximum_output_length) return SetStatus(result, HIDI2CStatus::InvalidReportLength);
		stduint write_length = 0;
		transfer_buffer[write_length++] = byte(descriptor.output_register);
		transfer_buffer[write_length++] = byte(descriptor.output_register >> 8);
		const stduint report_length = length + 2 + (report_id ? 1 : 0);
		transfer_buffer[write_length++] = byte(report_length);
		transfer_buffer[write_length++] = byte(report_length >> 8);
		if (report_id) transfer_buffer[write_length++] = report_id;
		CopyBytes(transfer_buffer + write_length, source, length);
		write_length += length;
		IICMessage message{ transport.address, uint16(transport.address_flags | IICMessageFlag_Stop), transfer_buffer, nullptr, write_length, 0 };
		const auto status = Transfer(&message, 1, timeout, result);
		if (status == HIDI2CStatus::Success) result.length = length;
		return status;
	}

	HIDI2CStatus HIDI2CDevice::getReport(HIDI2CReportType report_type, uint8 report_id, byte* destination, stduint capacity, stduint timeout, HIDI2CResult& result) {
		ResetResult(result);
		if (report_type == HIDI2CReportType::Output) return SetStatus(result, HIDI2CStatus::InvalidArgument);
		return WriteCommand(HIDI2CCommandGetReport, report_type, report_id, nullptr, 0, true, destination, capacity, timeout, result);
	}

	HIDI2CStatus HIDI2CDevice::setReport(HIDI2CReportType report_type, uint8 report_id, const byte* source, stduint length, stduint timeout, HIDI2CResult& result) {
		ResetResult(result);
		if (report_type == HIDI2CReportType::Input) return SetStatus(result, HIDI2CStatus::InvalidArgument);
		return WriteCommand(HIDI2CCommandSetReport, report_type, report_id, source, length, false, nullptr, 0, timeout, result);
	}

	HIDI2CStatus HIDI2CDevice::setPower(HIDI2CPowerState state, stduint timeout, HIDI2CResult& result) {
		ResetResult(result);
		if (!initialized || !transfer_buffer) return SetStatus(result, HIDI2CStatus::NotInitialized);
		if (state != HIDI2CPowerState::On && state != HIDI2CPowerState::Sleep) return SetStatus(result, HIDI2CStatus::InvalidArgument);
		transfer_buffer[0] = byte(descriptor.command_register);
		transfer_buffer[1] = byte(descriptor.command_register >> 8);
		transfer_buffer[2] = byte(state);
		transfer_buffer[3] = HIDI2CCommandSetPower;
		IICMessage message{ transport.address, uint16(transport.address_flags | IICMessageFlag_Stop), transfer_buffer, nullptr, 4, 0 };
		return Transfer(&message, 1, timeout, result);
	}

	HIDI2CStatus HIDI2CDevice::BeginReset(stduint timeout, HIDI2CResult& result) {
		ResetResult(result);
		if (!initialized || !transfer_buffer) return SetStatus(result, HIDI2CStatus::NotInitialized);
		if (reset_pending) return SetStatus(result, HIDI2CStatus::ResetPending);
		transfer_buffer[0] = byte(descriptor.command_register);
		transfer_buffer[1] = byte(descriptor.command_register >> 8);
		transfer_buffer[2] = 0;
		transfer_buffer[3] = HIDI2CCommandReset;
		IICMessage message{ transport.address, uint16(transport.address_flags | IICMessageFlag_Stop), transfer_buffer, nullptr, 4, 0 };
		const auto status = Transfer(&message, 1, timeout, result);
		if (status == HIDI2CStatus::Success) reset_pending = true;
		return status;
	}

	HIDI2CStatus HIDI2CDevice::ParseInput(const byte* source, stduint length, byte* destination, stduint capacity, HIDI2CResult& result) {
		if (!initialized) return SetStatus(result, HIDI2CStatus::NotInitialized);
		if (!source || length < 2 || (!destination && capacity)) return SetStatus(result, HIDI2CStatus::InvalidArgument);
		const stduint report_length = DecodeHIDI2CWord(source);
		if (!report_length) {
			reset_pending = false;
			return SetStatus(result, HIDI2CStatus::ResetComplete);
		}
		if (report_length < 2 || report_length > length) return SetStatus(result, HIDI2CStatus::InvalidReportLength);
		const stduint payload_length = report_length - 2;
		if (payload_length > capacity) return SetStatus(result, HIDI2CStatus::BufferTooSmall);
		CopyBytes(destination, source + 2, payload_length);
		return SetStatus(result, HIDI2CStatus::Success, payload_length);
	}

	HIDI2CStatus HIDI2CDevice::AcceptInput(const byte* source, stduint length, byte* destination, stduint capacity, HIDI2CResult& result) {
		ResetResult(result);
		result.transfer_status = IICTransferStatus::Success;
		return ParseInput(source, length, destination, capacity, result);
	}

	HIDI2CStatus HIDI2CDevice::ExpireReset(HIDI2CResult& result) {
		ResetResult(result);
		if (!initialized) return SetStatus(result, HIDI2CStatus::NotInitialized);
		if (!reset_pending) return SetStatus(result, HIDI2CStatus::InvalidArgument);
		reset_pending = false;
		return SetStatus(result, HIDI2CStatus::Timeout);
	}

}
