// UTF-8 CPP-ISO11 TAB4 LF
// Docutitle: ACPI Resource Template
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../inc/cpp/Device/ACPI/ACPI-Resource.hpp"

namespace uni {
	ACPIResource::GPIOConnectionStatus ACPIResource::DecodeGPIOConnection(const uint8* descriptor, stduint length, GPIOConnection& connection) {
		connection = {};
		if (!descriptor) return GPIOConnectionStatus::InvalidArgument;
		if (length < 23) return GPIOConnectionStatus::Truncated;
		if (descriptor[0] != 0x8Cu) return GPIOConnectionStatus::InvalidTag;
		const stduint encoded_length = stduint(descriptor[1]) | stduint(descriptor[2]) << 8;
		if (encoded_length > ~stduint(0) - 3 || encoded_length + 3 != length) return GPIOConnectionStatus::Truncated;
		connection.revision = descriptor[3];
		if (connection.revision != 1) return GPIOConnectionStatus::UnsupportedRevision;
		if (descriptor[4] > 1) return GPIOConnectionStatus::UnsupportedConnectionType;
		connection.type = descriptor[4] ? GPIOConnectionType::IO : GPIOConnectionType::Interrupt;
		connection.general_flags = uint16(descriptor[5]) | uint16(descriptor[6]) << 8;
		connection.connection_flags = uint16(descriptor[7]) | uint16(descriptor[8]) << 8;
		connection.pin_configuration = descriptor[9];
		connection.drive_strength = uint16(descriptor[10]) | uint16(descriptor[11]) << 8;
		connection.debounce_timeout = uint16(descriptor[12]) | uint16(descriptor[13]) << 8;
		connection.resource_source_index = descriptor[16];
		const uint16 pin_offset = uint16(descriptor[14]) | uint16(descriptor[15]) << 8;
		const uint16 source_offset = uint16(descriptor[17]) | uint16(descriptor[18]) << 8;
		const uint16 vendor_offset = uint16(descriptor[19]) | uint16(descriptor[20]) << 8;
		const uint16 vendor_length = uint16(descriptor[21]) | uint16(descriptor[22]) << 8;
		const uint16 allowed_connection_flags = connection.type == GPIOConnectionType::Interrupt ? 0x001Fu : 0x000Bu;
		const uint8 interrupt_polarity = uint8((connection.connection_flags >> 1) & 3u);
		const bool reserved_pin_configuration = connection.pin_configuration > 3 && connection.pin_configuration < 128;
		if ((connection.general_flags & 0xFFFEu) || (connection.connection_flags & ~allowed_connection_flags) || connection.resource_source_index || reserved_pin_configuration || (connection.type == GPIOConnectionType::Interrupt && interrupt_polarity == 3)) return GPIOConnectionStatus::ReservedValue;
		if (pin_offset < 23 || source_offset <= pin_offset || source_offset > length || ((source_offset - pin_offset) & 1u)) return GPIOConnectionStatus::InvalidPinTable;
		connection.pin_count = (source_offset - pin_offset) / 2;
		if (!connection.pin_count) return GPIOConnectionStatus::InvalidPinTable;
		if (vendor_offset <= source_offset || vendor_offset > length) return GPIOConnectionStatus::InvalidResourceSource;
		const stduint source_length = vendor_offset - source_offset;
		if (source_length < 2 || descriptor[vendor_offset - 1] != 0) return GPIOConnectionStatus::InvalidResourceSource;
		for0(index, source_length - 1) if (!descriptor[source_offset + index]) return GPIOConnectionStatus::InvalidResourceSource;
		if (vendor_length > length - vendor_offset || vendor_offset + vendor_length != length) return GPIOConnectionStatus::InvalidVendorData;
		connection.pin_table = descriptor + pin_offset;
		connection.resource_source = reinterpret_cast<const char*>(descriptor + source_offset);
		connection.resource_source_length = source_length - 1;
		connection.vendor_data = vendor_length ? descriptor + vendor_offset : nullptr;
		connection.vendor_data_length = vendor_length;
		for0(index, connection.pin_count) {
			uint16 pin = 0;
			ReadGPIOPin(connection, index, pin);
			if (pin == 0xFFFFu) continue;
			for (stduint previous = 0; previous < index; ++previous) {
				uint16 previous_pin = 0;
				ReadGPIOPin(connection, previous, previous_pin);
				if (previous_pin == 0xFFFFu) continue;
				if (pin == previous_pin) return GPIOConnectionStatus::DuplicatePin;
			}
		}
		return GPIOConnectionStatus::Success;
	}

	bool ACPIResource::ReadGPIOPin(const GPIOConnection& connection, stduint index, uint16& pin) {
		if (!connection.pin_table || index >= connection.pin_count) return false;
		pin = uint16(connection.pin_table[index * 2]) | uint16(connection.pin_table[index * 2 + 1]) << 8;
		return true;
	}
}
