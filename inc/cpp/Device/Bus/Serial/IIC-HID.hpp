// UTF-8 CPP-ISO11 TAB4 LF
// Docutitle: Human Interface Device over I2C
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#ifndef _INCPP_Device_IIC_HID
#define _INCPP_Device_IIC_HID

#include "../../../unisym"
#include "../../IIC"
#include "../../HID/HID-Report.hpp"

namespace uni {
	namespace trait { class Malloc; }

	constexpr stduint HIDI2CDescriptorLength = 30;
	constexpr uint16 HIDI2CProtocolVersion = 0x0100;
	constexpr uint16 HIDI2CMaximumReportDescriptorLength = 4096;
	constexpr uint16 HIDI2CMaximumTransferLength = 4096;

	enum class HIDI2CDescriptorStatus : uint8 {
		Success,
		InvalidArgument,
		InvalidLength,
		UnsupportedVersion,
		InvalidReportDescriptor,
		InvalidRegister,
		InvalidInputLength,
		InvalidOutputLength,
	};

	struct HIDI2CDescriptor {
		uint16 descriptor_length;
		uint16 version;
		uint16 report_descriptor_length;
		uint16 report_descriptor_register;
		uint16 input_register;
		uint16 maximum_input_length;
		uint16 output_register;
		uint16 maximum_output_length;
		uint16 command_register;
		uint16 data_register;
		uint16 vendor_id;
		uint16 product_id;
		uint16 device_version;
		uint32 reserved;
	};

	enum class HIDI2CStatus : uint8 {
		Success,
		ResetComplete,
		InvalidArgument,
		NotInitialized,
		AllocationFailed,
		TransferFailed,
		ShortTransfer,
		InvalidDescriptor,
		InvalidReportLength,
		BufferTooSmall,
		UnexpectedReportId,
		ResetPending,
		Timeout,
		Unsupported,
	};

	enum class HIDI2CReportType : uint8 { Input = 1, Output = 2, Feature = 3 };
	enum class HIDI2CPowerState : uint8 { On = 0, Sleep = 1 };

	struct HIDI2CResult {
		HIDI2CStatus status;
		IICTransferStatus transfer_status;
		IICTransferResult transfer;
		HIDI2CDescriptorStatus descriptor_status;
		stduint length;
	};

	typedef IICTransferStatus (*HIDI2CTransfer)(void* context, IICMessage* messages, stduint count, stduint timeout, IICTransferResult& result);

	struct HIDI2CTransport {
		void* context;
		HIDI2CTransfer transfer;
		uint16 address;
		uint16 address_flags;
	};

	uint16 DecodeHIDI2CWord(const byte* source);
	HIDI2CDescriptorStatus DecodeHIDI2CDescriptor(const byte* source, stduint length, HIDI2CDescriptor& descriptor);

	class HIDI2CDevice {
		HIDI2CTransport transport;
		trait::Malloc* allocator;
		HIDI2CDescriptor descriptor{};
		HIDReportLayout report_layout;
		byte* report_descriptor = nullptr;
		byte* transfer_buffer = nullptr;
		bool initialized = false;
		bool report_descriptor_ready = false;
		bool reset_pending = false;

		void Release();
		HIDI2CStatus Transfer(IICMessage* messages, stduint count, stduint timeout, HIDI2CResult& result);
		HIDI2CStatus ReadRegister(uint16 register_address, byte* destination, stduint length, stduint timeout, HIDI2CResult& result);
		HIDI2CStatus WriteCommand(uint8 opcode, HIDI2CReportType report_type, uint8 report_id, const byte* payload, stduint length, bool read, byte* destination, stduint capacity, stduint timeout, HIDI2CResult& result);
		HIDI2CStatus ParseInput(const byte* source, stduint length, byte* destination, stduint capacity, HIDI2CResult& result);

	public:
		HIDI2CDevice(const HIDI2CTransport& transport, trait::Malloc* allocator);
		~HIDI2CDevice();
		HIDI2CDevice(const HIDI2CDevice&) = delete;
		HIDI2CDevice& operator=(const HIDI2CDevice&) = delete;

		HIDI2CStatus Initialize(uint16 descriptor_register, stduint timeout, HIDI2CResult& result);
		HIDI2CStatus ReadReportDescriptor(stduint timeout, HIDI2CResult& result);
		HIDReportDescriptorStatus ParseReportDescriptor(HIDReportParseResult& result);
		// Write payloads exclude an explicit report ID; reads preserve bytes after the length prefix.
		HIDI2CStatus ReadInput(byte* destination, stduint capacity, stduint timeout, HIDI2CResult& result);
		HIDI2CStatus WriteOutput(uint8 report_id, const byte* source, stduint length, stduint timeout, HIDI2CResult& result);
		HIDI2CStatus getReport(HIDI2CReportType report_type, uint8 report_id, byte* destination, stduint capacity, stduint timeout, HIDI2CResult& result);
		HIDI2CStatus setReport(HIDI2CReportType report_type, uint8 report_id, const byte* source, stduint length, stduint timeout, HIDI2CResult& result);
		HIDI2CStatus setPower(HIDI2CPowerState state, stduint timeout, HIDI2CResult& result);
		HIDI2CStatus BeginReset(stduint timeout, HIDI2CResult& result);
		HIDI2CStatus AcceptInput(const byte* source, stduint length, byte* destination, stduint capacity, HIDI2CResult& result);
		HIDI2CStatus ExpireReset(HIDI2CResult& result);

		bool IsInitialized() const { return initialized; }
		bool IsResetPending() const { return reset_pending; }
		const HIDI2CDescriptor& Descriptor() const { return descriptor; }
		const byte* ReportDescriptor() const { return report_descriptor; }
		const HIDReportLayout& ReportLayout() const { return report_layout; }
	};

}

#endif
