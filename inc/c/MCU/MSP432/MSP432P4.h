// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: MCU TI MSP432P4
// Codifiers: @dosconio: 20241106
// Attribute: Inc(only-inner)
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

#include "../../stdinc.h"
#if !defined(_INC_MCU_MSP432P4_INNER)
#define _INC_MCU_MSP432P4_INNER

// ROM API Table
#define ROM_APITABLE       ((uint32 *)0x02000800)
#define ROM_VERSION                   (ROM_APITABLE[ 0])
#define ROM_ADC14TABLE     ((uint32 *)(ROM_APITABLE[ 1]))
#define ROM_AES256TABLE    ((uint32 *)(ROM_APITABLE[ 2]))
#define ROM_COMPTABLE      ((uint32 *)(ROM_APITABLE[ 3]))
#define ROM_CRC32TABLE     ((uint32 *)(ROM_APITABLE[ 4]))
#define ROM_CSTABLE        ((uint32 *)(ROM_APITABLE[ 5]))
#define ROM_DMATABLE       ((uint32 *)(ROM_APITABLE[ 6]))
#define ROM_FLASHCTLTABLE  ((uint32 *)(ROM_APITABLE[ 7]))
#define ROM_FPUTABLE       ((uint32 *)(ROM_APITABLE[ 8]))
#define ROM_GPIOTABLE      ((uint32 *)(ROM_APITABLE[ 9]))
#define ROM_I2CTABLE       ((uint32 *)(ROM_APITABLE[10]))
#define ROM_INTTABLE       ((uint32 *)(ROM_APITABLE[11]))
#define ROM_MPUTABLE       ((uint32 *)(ROM_APITABLE[12]))
#define ROM_PCMTABLE       ((uint32 *)(ROM_APITABLE[13]))
#define ROM_PMAPTABLE      ((uint32 *)(ROM_APITABLE[14]))
#define ROM_PSSTABLE       ((uint32 *)(ROM_APITABLE[15]))
#define ROM_REFTABLE       ((uint32 *)(ROM_APITABLE[16]))
#define ROM_RESETCTLTABLE  ((uint32 *)(ROM_APITABLE[17]))
#define ROM_RTCTABLE       ((uint32 *)(ROM_APITABLE[18]))
#define ROM_SPITABLE       ((uint32 *)(ROM_APITABLE[19]))
#define ROM_SYSCTLTABLE    ((uint32 *)(ROM_APITABLE[20]))
#define ROM_SYSTICKTABLE   ((uint32 *)(ROM_APITABLE[21]))
#define ROM_TIMER_ATABLE   ((uint32 *)(ROM_APITABLE[22]))
#define ROM_TIMER32TABLE   ((uint32 *)(ROM_APITABLE[23]))
#define ROM_UARTTABLE      ((uint32 *)(ROM_APITABLE[24]))
#define ROM_WDTTABLE       ((uint32 *)(ROM_APITABLE[25]))
#define ROM_SYSCTLATABLE   ((uint32 *)(ROM_APITABLE[26]))
#define ROM_FLASHCTLATABLE ((uint32 *)(ROM_APITABLE[27]))
#define ROM_LCDFTABLE      ((uint32 *)(ROM_APITABLE[28]))


//{TEMP} ---- MSP432P401R
#define FLASH_BASE         _IMM(0x00000000) // Main Flash memory start address
#define SRAM_BASE          _IMM(0x20000000) // SRAM memory start address
#define PERIPH_BASE        _IMM(0x40000000) // Peripherals start address
#define PERIPH_BASE2       _IMM(0xE0000000) // Peripherals start address
#define BITBAND_SRAM_BASE  _IMM(0x22000000)
#define BITBAND_PERI_BASE  _IMM(0x42000000)
// SRAM allows 32 bit bit band access
#define BITBAND_SRAM(x, b)  (*((uint32 *) (BITBAND_SRAM_BASE +  (((uint32)(volatile const uint32 *)&(x)) - SRAM_BASE  )*32 + (b)*4)))
// peripherals with 8 bit or 16 bit register access allow only 8 bit or 16 bit bit band access, so cast to 8 bit always
#define BITBAND_PERI(x, b)  (*((byte *) (BITBAND_PERI_BASE +  (((uint32)(volatile const uint32 *)&(x)) - PERIPH_BASE)*32 + (b)*4)))


#endif
