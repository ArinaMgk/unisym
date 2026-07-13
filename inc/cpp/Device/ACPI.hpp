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
		_PACKED(struct) RSDT {
			DescriptionHeader header;
			//
			const DescriptionHeader& operator[](size_t i) const;
			size_t Count() const;
		};
		_PACKED(struct) XSDT {
			DescriptionHeader header;
			//
			const DescriptionHeader& operator[](size_t i) const;
			size_t Count() const;
		};
		_PACKED(struct) MADT {
			DescriptionHeader header;
			uint32_t lapic_address;
			uint32_t flags;
		};
		_PACKED(struct) MADTEntry {
			uint8_t type;
			uint8_t length;
		};
		_PACKED(struct) MADTLocalAPIC {
			MADTEntry entry;
			uint8_t acpi_processor_uid;
			uint8_t apic_id;
			uint32_t flags;
		};
		_PACKED(struct) MADTLocalX2APIC {
			MADTEntry entry;
			uint16_t reserved;
			uint32_t x2apic_id;
			uint32_t flags;
			uint32_t acpi_processor_uid;
		};
		_PACKED(struct) MCFGAllocation {
			uint64_t base_address;
			uint16_t segment_group_number;
			uint8_t start_bus_number;
			uint8_t end_bus_number;
			uint32_t reserved;
		};
		_PACKED(struct) MCFG {
			DescriptionHeader header;
			uint64_t reserved;
			//
			const MCFGAllocation& operator[](size_t i) const;
			size_t Count() const;
		};
		_PACKED(struct) GenericAddress {
			uint8_t address_space;
			uint8_t bit_width;
			uint8_t bit_offset;
			uint8_t access_size;
			uint64_t address;
		};
		_PACKED(struct) FADT {
			DescriptionHeader header;
			uint32_t firmware_ctrl;
			uint32_t dsdt;
			uint8_t reserved0;
			uint8_t preferred_pm_profile;
			uint16_t sci_int;
			uint32_t smi_cmd;
			uint8_t acpi_enable;
			uint8_t acpi_disable;
			uint8_t s4bios_req;
			uint8_t pstate_cnt;
			uint32_t pm1a_evt_blk;
			uint32_t pm1b_evt_blk;
			uint32_t pm1a_cnt_blk;
			uint32_t pm1b_cnt_blk;
			uint32_t pm2_cnt_blk;
			uint32_t pm_tmr_blk;
			uint32_t gpe0_blk;
			uint32_t gpe1_blk;
			uint8_t pm1_evt_len;
			uint8_t pm1_cnt_len;
			uint8_t pm2_cnt_len;
			uint8_t pm_tmr_len;
			uint8_t gpe0_blk_len;
			uint8_t gpe1_blk_len;
			uint8_t gpe1_base;
			uint8_t cst_cnt;
			uint16_t p_lvl2_lat;
			uint16_t p_lvl3_lat;
			uint16_t flush_size;
			uint16_t flush_stride;
			uint8_t duty_offset;
			uint8_t duty_width;
			uint8_t day_alrm;
			uint8_t mon_alrm;
			uint8_t century;
			uint16_t iapc_boot_arch;
			uint8_t reserved1;
			uint32_t flags;
			GenericAddress reset_reg;
			uint8_t reset_value;
			uint16_t arm_boot_arch;
			uint8_t fadt_minor_version;
			uint64_t x_firmware_ctrl;
			uint64_t x_dsdt;
		};
	public:
		static bool Assert(const RSDP& rsdp);
		static void Delay_ms(unsigned long msec);
		static const FADT* fadt;
	};

#endif
}


#endif
