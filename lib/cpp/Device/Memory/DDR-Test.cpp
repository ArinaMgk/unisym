// UTF-8 CPP-ISO20 TAB4 CRLF
// Docutitle: (Device) Double Data Rate (DDR) Memory
// Codifiers: @dosconio: 20241213 ~ <Last-check> 
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

#include "../../../../inc/cpp/Device/DDR"
#include "../../../../inc/cpp/Device/SysTick"
#include "../../../../inc/cpp/Device/RCC/RCC"
#include "../../../../inc/c/driver/SRAM.h"
#include "../../../../inc/cpp/Device/_Power.hpp"
#include "../../../../inc/cpp/Device/RCC/RCC"
#include "../../../../inc/cpp/MCU/_ADDRESS/ADDR-STM32.h"
#include "../../../../inc/c/ustring.h"

#define re(x) Reference(&localcfg.x)

#ifdef _MPU_STM32MP13
#include "../../../../inc/c/proctrl/ARM/cortex_a7.h"

_TEMP stduint _dbg_reg;

namespace uni {
#define impl(x) x DDR_t
#define DDR_PATTERN     0xAAAAAAAAU
#define DDR_ANTIPATTERN 0x55555555U
#define DDR_MAX_SIZE    0x40000000U

	extern DDRConfig localcfg;

	bool ddr_test_rw_access(DDR_t& ddr) {
		uint32_t saved_value = *ddr.getRoleress();
		*ddr.getRoleress() = DDR_PATTERN;
		asrtret(*ddr.getRoleress() == DDR_PATTERN);
		*ddr.getRoleress() = saved_value;
		return true;
	}

	bool ddr_test_data_bus(DDR_t& ddr) {
		stduint pattern;
		for0(i, 32) {
			pattern = _IMM1S(i);
			*ddr.getRoleress() = DDR_PATTERN;
			asrtret(*ddr.getRoleress() == DDR_PATTERN);
		}
		return true;
	}

	bool ddr_test_addr_bus(DDR_t& ddr) {
		stduint test_offset{ nil };
		uint32 addressmask = (localcfg.info.size - 1U);
		// Write the default pattern at each of the power-of-two offsets.
		for (stduint offset = byteof(uint32);
			offset & addressmask; offset <<= 1) {
			*ddr.getRoleress(offset) = DDR_PATTERN;
		}
		// Check for address bits stuck high
		// compensation for Cache (?)
		*ddr.getRoleress(test_offset) = DDR_ANTIPATTERN;
		for (stduint offset = byteof(uint32);
			offset & addressmask; offset <<= 1) {
			asrtret(*ddr.getRoleress(offset) == DDR_PATTERN);
		}
		*ddr.getRoleress(test_offset) = DDR_PATTERN;
		// Check for address bits stuck low or shorted
		for (test_offset = byteof(uint32);
			test_offset & addressmask; test_offset <<= 1) {
			*ddr.getRoleress(test_offset) = DDR_ANTIPATTERN;
			asrtret(*ddr.getRoleress() == DDR_PATTERN);
			for (stduint offset = byteof(uint32);
				offset & addressmask; offset <<= 1) {
				asrtret(!((offset != test_offset) && (*ddr.getRoleress(offset) != DDR_PATTERN)));
			}
			*ddr.getRoleress(test_offset) = DDR_PATTERN;
		}
		return true;
	}

	uint32 ddr_check_size(DDR_t& ddr) {
		uint32 offset = sizeof(uint32);
		ddr.getRoleress()[0] = DDR_PATTERN;
		while (offset < DDR_MAX_SIZE) {
			*ddr.getRoleress(offset) = DDR_ANTIPATTERN;
			__DSB();
			if (!*ddr.getRoleress(offset) == DDR_ANTIPATTERN)
				break;
			offset <<= 1;
		}
		return offset;
	}


}
#endif

