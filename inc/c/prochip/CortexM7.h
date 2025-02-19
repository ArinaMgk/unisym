// ASCII C/C++ TAB4 CRLF
// Docutitle: (Processor, not MCU and not ARCH) Cortex-M4
// Codifiers: @dosconio: 20240718
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

#ifndef _INC_Processor_CortexM7
#define _INC_Processor_CortexM7

//{TODO} core_cm7.h
//{TODO} up-abstract from STM32F7

#include "../stdinc.h"

#define _SCS_BASE       (0xE000E000UL)
#define _ITM_BASE       (0xE0000000UL)
#define _DWT_BASE       (0xE0001000UL)
#define _TPI_BASE       (0xE0040000UL)
#define _CoreDebug_BASE (0xE000EDF0UL)
#define _SysTick_BASE   (_SCS_BASE +  0x0010UL)
#define _NVIC_BASE      (_SCS_BASE +  0x0100UL)
#define _SCB_BASE       (_SCS_BASE +  0x0D00UL)

struct SysCtrlBlock_Map {
	uint32 CPUID;                  // 0x000 (R/ )  CPUID Base
	uint32 ICSR;                   // 0x004 (R/W)  Interrupt Control and State
	uint32 VTOR;                   // 0x008 (R/W)  Vector Table Offset
	uint32 AIRCR;                  // 0x00C (R/W)  Application Interrupt and Reset Control
	uint32 SCR;                    // 0x010 (R/W)  System Control
	uint32 CCR;                    // 0x014 (R/W)  Configuration Control
	uint8  SHPR[12U];              // 0x018 (R/W)  System Handlers Priority Registers (4-7, 8-11, 12-15)
	uint32 SHCSR;                  // 0x024 (R/W)  System Handler Control and State
	uint32 CFSR;                   // 0x028 (R/W)  Configurable Fault Status
	uint32 HFSR;                   // 0x02C (R/W)  HardFault Status
	uint32 DFSR;                   // 0x030 (R/W)  Debug Fault Status
	uint32 MMFAR;                  // 0x034 (R/W)  MemManage Fault Address
	uint32 BFAR;                   // 0x038 (R/W)  BusFault Address
	uint32 AFSR;                   // 0x03C (R/W)  Auxiliary Fault Status
	uint32 ID_PFR[2U];             // 0x040 (R/ )  Processor Feature
	uint32 ID_DFR;                 // 0x048 (R/ )  Debug Feature
	uint32 ID_AFR;                 // 0x04C (R/ )  Auxiliary Feature
	uint32 ID_MFR[4U];             // 0x050 (R/ )  Memory Model Feature
	uint32 ID_ISAR[5U];            // 0x060 (R/ )  Instruction Set Attributes
	uint32 RESERVED0[1U]; //////////////
	uint32 CLIDR;                  // 0x078 (R/ )  Cache Level ID
	uint32 CTR;                    // 0x07C (R/ )  Cache Type
	uint32 CCSIDR;                 // 0x080 (R/ )  Cache Size ID
	uint32 CSSELR;                 // 0x084 (R/W)  Cache Size Selection
	uint32 CPACR;                  // 0x088 (R/W)  Coprocessor Access Control
	uint32 RESERVED3[93U]; //////////////
	uint32 STIR;                   // 0x200 ( /W)  Software Triggered Interrupt Register
	uint32 RESERVED4[15U]; //////////////
	uint32 MVFR0;                  // 0x240 (R/ )  Media and VFP Feature 0
	uint32 MVFR1;                  // 0x244 (R/ )  Media and VFP Feature 1
	uint32 MVFR2;                  // 0x248 (R/ )  Media and VFP Feature 2
	uint32 RESERVED5[1U]; //////////////
	uint32 ICIALLU;                // 0x250 ( /W)  I-Cache Invalidate All to PoU
	uint32 RESERVED6[1U]; //////////////
	uint32 ICIMVAU;                // 0x258 ( /W)  I-Cache Invalidate by MVA to PoU
	uint32 DCIMVAC;                // 0x25C ( /W)  D-Cache Invalidate by MVA to PoC
	uint32 DCISW;                  // 0x260 ( /W)  D-Cache Invalidate by Set-way
	uint32 DCCMVAU;                // 0x264 ( /W)  D-Cache Clean by MVA to PoU
	uint32 DCCMVAC;                // 0x268 ( /W)  D-Cache Clean by MVA to PoC
	uint32 DCCSW;                  // 0x26C ( /W)  D-Cache Clean by Set-way
	uint32 DCCIMVAC;               // 0x270 ( /W)  D-Cache Clean and Invalidate by MVA to PoC
	uint32 DCCISW;                 // 0x274 ( /W)  D-Cache Clean and Invalidate by Set-way
	uint32 RESERVED7[6U]; //////////////
	uint32 ITCMCR;                 // 0x290 (R/W)  Instruction Tightly-Coupled Memory Control Register
	uint32 DTCMCR;                 // 0x294 (R/W)  Data Tightly-Coupled Memory Control Registers
	uint32 AHBPCR;                 // 0x298 (R/W)  AHBP Control Register
	uint32 CACR;                   // 0x29C (R/W)  L1 Cache Control Register
	uint32 AHBSCR;                 // 0x2A0 (R/W)  AHB Slave Control Register
	uint32 RESERVED8[1U]; //////////////
	uint32 ABFSR;                  // 0x2A8 (R/W)  Auxiliary Bus Fault Status
};
#define SCB ((struct SysCtrlBlock_Map *)_SCB_BASE)
#ifdef _CortexM7_SCB_TEMP
#include "CortexM7/CortexM7-SCB.h"
#endif

#endif
