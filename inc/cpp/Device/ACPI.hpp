// UTF-8 CPP-ISO11 TAB4 CRLF
// Docutitle: (Device) Advanced Configuration and Power Interface - Power Management (ACPI)
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
/*
	Copyright 2023 ArinaMgk

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0
	http://unisym.org/license.html

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/

#ifndef _INCPP_Device_ACPI
#define _INCPP_Device_ACPI
#include "../../c/stdinc.h"
namespace uni {

	
#if defined(_MCCA) && (_MCCA & 0xFF00) == 0x8600

	class ACPI {
	public:
		static constexpr stduint PMTimerFrequency = 3579545;
	public:
		_PACKED(struct) RSDP {
			char signature[8];
			uint8_t checksum;
			char oem_id[6];
			uint8_t revision;
			uint32_t rsdt_address;
			uint32_t length;
			uint64_t xsdt_address;
			uint8_t extended_checksum;
			char reserved[3];

			bool isValid() const;
		};
	public:
		_PACKED(struct) DescriptionHeader {
			char signature[4];
			uint32_t length;
			uint8_t revision;
			uint8_t checksum;
			char oem_id[6];
			char oem_table_id[8];
			uint32_t oem_revision;
			uint32_t creator_id;
			uint32_t creator_revision;
			//
			bool isValid(const char* expected_signature) const;
		};
		_PACKED(struct) XSDT {
			DescriptionHeader header;
			//
			const DescriptionHeader& operator[](size_t i) const;
			size_t Count() const;
		};
		_PACKED(struct) FADT {
			DescriptionHeader header;
			char reserved1[76 - sizeof(header)];
			uint32_t pm_tmr_blk;
			char reserved2[112 - 80];
			uint32_t flags;
			char reserved3[276 - 116];
		};
	public:
		static bool Assert(const RSDP& rsdp);
		static void Delay_ms(unsigned long msec);
		static const FADT* fadt;
	};

#endif
}


#endif
