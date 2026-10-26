// UTF-8 CPP-ISO11 TAB4 LF
// Docutitle: ACPI Resource Template
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
#ifndef _INCPP_Device_ACPI_ACPI_Resource
#define _INCPP_Device_ACPI_ACPI_Resource

#include "../../../c/stdinc.h"

namespace uni {
	class ACPIResource {
	public:
		enum class GPIOConnectionType : uint8 {
			Interrupt,
			IO,
		};

		enum class GPIOConnectionStatus : uint8 {
			Success,
			InvalidArgument,
			Truncated,
			InvalidTag,
			UnsupportedRevision,
			UnsupportedConnectionType,
			ReservedValue,
			InvalidPinTable,
			InvalidResourceSource,
			InvalidVendorData,
			DuplicatePin,
			Count,
		};

		struct GPIOConnection {
			const uint8* pin_table;
			const char* resource_source;
			const uint8* vendor_data;
			stduint pin_count;
			stduint resource_source_length;
			stduint vendor_data_length;
			uint16 general_flags;
			uint16 connection_flags;
			uint16 drive_strength;
			uint16 debounce_timeout;
			uint8 revision;
			uint8 resource_source_index;
			uint8 pin_configuration;
			GPIOConnectionType type;
		};

		static GPIOConnectionStatus DecodeGPIOConnection(const uint8* descriptor, stduint length, GPIOConnection& connection);
		static bool ReadGPIOPin(const GPIOConnection& connection, stduint index, uint16& pin);
	};
}

#endif
