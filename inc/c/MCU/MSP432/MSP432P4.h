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
#include "../../prochip/CortexM4.h"
#if !defined(_INC_MCU_MSP432P4_INNER)
#define _INC_MCU_MSP432P4_INNER

typedef void (*ROM_FUNC_t)(uint_fast8_t, uint_fast16_t);

// ROM API Table
#define ROM_VERSION                   (ROM_APITABLE[ 0])
#define ROM_ADC14TABLE     ((ROM_FUNC_t *)(ROM_APITABLE[ 1]))
#define ROM_AES256TABLE    ((ROM_FUNC_t *)(ROM_APITABLE[ 2]))
#define ROM_COMPTABLE      ((ROM_FUNC_t *)(ROM_APITABLE[ 3]))
#define ROM_CRC32TABLE     ((ROM_FUNC_t *)(ROM_APITABLE[ 4]))
#define ROM_CSTABLE        ((ROM_FUNC_t *)(ROM_APITABLE[ 5]))
#define ROM_DMATABLE       ((ROM_FUNC_t *)(ROM_APITABLE[ 6]))
#define ROM_FLASHCTLTABLE  ((ROM_FUNC_t *)(ROM_APITABLE[ 7]))
#define ROM_FPUTABLE       ((ROM_FUNC_t *)(ROM_APITABLE[ 8]))
#define ROM_GPIOTABLE      ((ROM_FUNC_t *)(ROM_APITABLE[ 9]))
/* ROM_GPIOTABLE
** 0 setAsOutputPin
** 1 setOutputHighOnPin
** 2 setOutputLowOnPin
** 3 toggleOutputOnPin  (unused)
** 4
** 5
** 6
** 7
** 8
** 9
**10
**11
**12
**13
**14 setAsInputPin
**15
**/
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
#define ROM_WDTTABLE       ((Handler_t*)ROM_APITABLE[25])
#define ROM_SYSCTLATABLE   ((uint32 *)(ROM_APITABLE[26]))
#define ROM_FLASHCTLATABLE ((uint32 *)(ROM_APITABLE[27]))
#define ROM_LCDFTABLE      ((uint32 *)(ROM_APITABLE[28]))


//{TEMP} ---- MSP432P401R
#define BITBAND_SRAM_BASE  _IMM(0x22000000)
#define BITBAND_PERI_BASE  _IMM(0x42000000)
// SRAM allows 32 bit bit band access
#define BITBAND_SRAM(x, b)  (*((uint32 *) (BITBAND_SRAM_BASE +  (((uint32)&(x)) - SRAM_BASE  )*32 + (b)*4)))
// peripherals with 8 bit or 16 bit register access allow only 8 bit or 16 bit bit band access, so cast to 8 bit always
#define BITBAND_PERI(x, b)  (*((byte *) (BITBAND_PERI_BASE +  (((uint32)&(x)) - PERIPH_BASE)*32 + (b)*4)))


#define FLASH_BASE         _IMM(0x00000000) // Main Flash memory start address
#define SRAM_BASE          _IMM(0x20000000) // SRAM memory start address
#define PERIPH_BASE        _IMM(0x40000000) // Peripherals start address
#define PERIPH_BASE2       _IMM(0xE0000000) // Peripherals start address
#define ADC14_BASE                            (PERIPH_BASE +0x00012000) // Base address of module ADC14 
#define AES256_BASE                           (PERIPH_BASE +0x00003C00) // Base address of module AES256 
#define CAPTIO0_BASE                          (PERIPH_BASE +0x00005400) // Base address of module CAPTIO0 
#define CAPTIO1_BASE                          (PERIPH_BASE +0x00005800) // Base address of module CAPTIO1 
#define COMP_E0_BASE                          (PERIPH_BASE +0x00003400) // Base address of module COMP_E0 
#define COMP_E1_BASE                          (PERIPH_BASE +0x00003800) // Base address of module COMP_E1 
#define CRC32_BASE                            (PERIPH_BASE +0x00004000) // Base address of module CRC32 
#define CS_BASE                               (PERIPH_BASE +0x00010400) // Base address of module CS 
#define DIO_BASE                              (PERIPH_BASE +0x00004C00) // Base address of module DIO 
#define DMA_BASE                              (PERIPH_BASE +0x0000E000) // Base address of module DMA 
#define EUSCI_A0_BASE                         (PERIPH_BASE +0x00001000) // Base address of module EUSCI_A0 
#define EUSCI_A0_SPI_BASE                     (PERIPH_BASE +0x00001000) // Base address of module EUSCI_A0 
#define EUSCI_A1_BASE                         (PERIPH_BASE +0x00001400) // Base address of module EUSCI_A1 
#define EUSCI_A1_SPI_BASE                     (PERIPH_BASE +0x00001400) // Base address of module EUSCI_A1 
#define EUSCI_A2_BASE                         (PERIPH_BASE +0x00001800) // Base address of module EUSCI_A2 
#define EUSCI_A2_SPI_BASE                     (PERIPH_BASE +0x00001800) // Base address of module EUSCI_A2 
#define EUSCI_A3_BASE                         (PERIPH_BASE +0x00001C00) // Base address of module EUSCI_A3 
#define EUSCI_A3_SPI_BASE                     (PERIPH_BASE +0x00001C00) // Base address of module EUSCI_A3 
#define EUSCI_B0_BASE                         (PERIPH_BASE +0x00002000) // Base address of module EUSCI_B0 
#define EUSCI_B0_SPI_BASE                     (PERIPH_BASE +0x00002000) // Base address of module EUSCI_B0 
#define EUSCI_B1_BASE                         (PERIPH_BASE +0x00002400) // Base address of module EUSCI_B1 
#define EUSCI_B1_SPI_BASE                     (PERIPH_BASE +0x00002400) // Base address of module EUSCI_B1 
#define EUSCI_B2_BASE                         (PERIPH_BASE +0x00002800) // Base address of module EUSCI_B2 
#define EUSCI_B2_SPI_BASE                     (PERIPH_BASE +0x00002800) // Base address of module EUSCI_B2 
#define EUSCI_B3_BASE                         (PERIPH_BASE +0x00002C00) // Base address of module EUSCI_B3 
#define EUSCI_B3_SPI_BASE                     (PERIPH_BASE +0x00002C00) // Base address of module EUSCI_B3 
#define FLCTL_BASE                            (PERIPH_BASE +0x00011000) // Base address of module FLCTL 
#define FL_BOOTOVER_MAILBOX_BASE                 ((uint32_t)0x00200000) // Base address of module FL_BOOTOVER_MAILBOX 
#define PCM_BASE                              (PERIPH_BASE +0x00010000) // Base address of module PCM 
#define PMAP_BASE                             (PERIPH_BASE +0x00005000) // Base address of module PMAP 
#define PSS_BASE                              (PERIPH_BASE +0x00010800) // Base address of module PSS 
#define REF_A_BASE                            (PERIPH_BASE +0x00003000) // Base address of module REF_A 
#define RSTCTL_BASE                           (PERIPH_BASE2+0x00042000) // Base address of module RSTCTL 
#define RTC_C_BASE                            (PERIPH_BASE +0x00004400) // Base address of module RTC_C 
#define RTC_C_BCD_BASE                        (PERIPH_BASE +0x00004400) // Base address of module RTC_C 
#define SYSCTL_BASE                           (PERIPH_BASE2+0x00043000) // Base address of module SYSCTL 
#define TIMER32_BASE                          (PERIPH_BASE +0x0000C000) // Base address of module TIMER32 
#define TIMER_A0_BASE                         (PERIPH_BASE +0x00000000) // Base address of module TIMER_A0 
#define TIMER_A1_BASE                         (PERIPH_BASE +0x00000400) // Base address of module TIMER_A1 
#define TIMER_A2_BASE                         (PERIPH_BASE +0x00000800) // Base address of module TIMER_A2 
#define TIMER_A3_BASE                         (PERIPH_BASE +0x00000C00) // Base address of module TIMER_A3 
#define TLV_BASE                                 ((uint32_t)0x00201000) // Base address of module TLV 
#define WDT_A_BASE                            (PERIPH_BASE +0x00004800) // Base address of module WDT_A 
#define TLV_START_ADDR                    (TLV_BASE + 0x0004)           // Start Address of the TLV structure */
#define FLCTL_A_BASE                          (PERIPH_BASE +0x00011000) // Base address of module FLCTL_A 
#define LCD_F_BASE                            (PERIPH_BASE +0x00012400) // Base address of module LCD_F 
#define SYSCTL_A_BASE                         (PERIPH_BASE2+0x00043000) // Base address of module SYSCTL_A 

// ---- ---- TEMP AREA ---- ---- //

	// devices\msp432p4xx\inc\msp432p4xx.h
typedef struct {
	_Comment(IO) uint32_t KEY;           // Key
	_Comment(IO) uint32_t CTL0;          // Control 0
	_Comment(IO) uint32_t CTL1;          // Control 1
	_Comment(IO) uint32_t CTL2;          // Control 2
	_Comment(IO) uint32_t CTL3;          // Control 3
	_Comment() uint32_t RESERVED0[7];
	_Comment(IO) uint32_t CLKEN;         // Clock Enable Register
	_Comment(I) uint32_t STAT;          // Status Register
	_Comment() uint32_t RESERVED1[2];
	_Comment(IO) uint32_t IE;            // Interrupt Enable Register
	_Comment() uint32_t RESERVED2;
	_Comment(I) uint32_t IFG;           // Interrupt Flag Register
	_Comment() uint32_t RESERVED3;
	_Comment(O) uint32_t CLRIFG;        // Clear Interrupt Flag Register
	_Comment() uint32_t RESERVED4;
	_Comment(O) uint32_t SETIFG;        // Set Interrupt Flag Register
	_Comment() uint32_t RESERVED5;
	_Comment(IO) uint32_t DCOERCAL0;     // DCO External Resistor Cailbration 0 Register
	_Comment(IO) uint32_t DCOERCAL1;     // DCO External Resistor Calibration 1 Register
} CS_Type;
enum class CS_CTL1_SELM_E {
	LFXTCLK = 0,// when LFXT available, otherwise REFOCLK
	VLOCLK,
	REFOCLK,
	DCOCLK,
	MODOSC,
	HFXTCLK,// when HFXT available, otherwise DCOCLK
	HFXT2CLK,// when HFXT2 available, otherwise DCOCLK
};// (pos 0, len 3)
#define CS ((CS_Type *) CS_BASE)
#define CS_KEY_VAL ((uint32)0x0000695A) // Pre-defined bitfield values
//
#define CS_IFG_LFXTIFG_OFS 0
#define CS_IFG_HFXTIFG_OFS 1
#define CS_CLKEN_REFOFSEL_OFS 15
#define CS_CTL0_DCORES_OFS 22

#define CS_CLRIFG_CLR_LFXTIFG 1
#define CS_CLRIFG_CLR_HFXTIFG 2


// [WDOG]
#define WDT_A_CTL_HOLD_OFS                       ( 7)                            /*!< WDTHOLD Bit Offset */
#define WDT_A_CTL_HOLD                           ((uint16_t)0x0080)              /*!< Watchdog timer hold */
#define WDT_A_CTL_PW_OFS                         ( 8)                            /*!< WDTPW Bit Offset */
#define WDT_A_CTL_PW_MASK                        ((uint16_t)0xFF00)              /*!< WDTPW Bit Mask */
#define WDT_A_CTL_PW                              ((uint16_t)0x5A00)              /*!< WDT Key Value for WDT write access */



typedef const struct {
	uint32 TLV_CHECKSUM;           // TLV Checksum
	uint32 DEVICE_INFO_TAG;        // Device Info Tag
	uint32 DEVICE_INFO_LEN;        // Device Info Length
	uint32 DEVICE_ID;              // Device ID
	uint32 HWREV;                  // HW Revision
	uint32 BCREV;                  // Boot Code Revision
	uint32 ROM_DRVLIB_REV;         // ROM Driver Library Revision
	uint32 DIE_REC_TAG;            // Die Record Tag
	uint32 DIE_REC_LEN;            // Die Record Length
	uint32 DIE_XPOS;               // Die X-Position
	uint32 DIE_YPOS;               // Die Y-Position
	uint32 WAFER_ID;               // Wafer ID
	uint32 LOT_ID;                 // Lot ID
	uint32 RESERVED0;
	uint32 RESERVED1;
	uint32 RESERVED2;
	uint32 TEST_RESULTS;           // Test Results
	uint32 CS_CAL_TAG;             // Clock System Calibration Tag
	uint32 CS_CAL_LEN;             // Clock System Calibration Length
	uint32 DCOIR_FCAL_RSEL04;      // DCO IR mode: Frequency calibration for DCORSEL 0 to 4
	uint32 DCOIR_FCAL_RSEL5;       // DCO IR mode: Frequency calibration for DCORSEL 5
	uint32 RESERVED3;
	uint32 RESERVED4;
	uint32 RESERVED5;
	uint32 RESERVED6;
	uint32 DCOIR_CONSTK_RSEL04;    // DCO IR mode: DCO Constant (K) for DCORSEL 0 to 4
	uint32 DCOIR_CONSTK_RSEL5;     // DCO IR mode: DCO Constant (K) for DCORSEL 5
	uint32 DCOER_FCAL_RSEL04;      // DCO ER mode: Frequency calibration for DCORSEL 0 to 4
	uint32 DCOER_FCAL_RSEL5;       // DCO ER mode: Frequency calibration for DCORSEL 5
	uint32 RESERVED7;
	uint32 RESERVED8;
	uint32 RESERVED9;
	uint32 RESERVED10;
	uint32 DCOER_CONSTK_RSEL04;    // DCO ER mode: DCO Constant (K) for DCORSEL 0 to 4
	uint32 DCOER_CONSTK_RSEL5;     // DCO ER mode: DCO Constant (K) for DCORSEL 5
	uint32 ADC14_CAL_TAG;          // ADC14 Calibration Tag
	uint32 ADC14_CAL_LEN;          // ADC14 Calibration Length
	uint32 ADC_GAIN_FACTOR;        // ADC Gain Factor
	uint32 ADC_OFFSET;             // ADC Offset
	uint32 RESERVED11;
	uint32 RESERVED12;
	uint32 RESERVED13;
	uint32 RESERVED14;
	uint32 RESERVED15;
	uint32 RESERVED16;
	uint32 RESERVED17;
	uint32 RESERVED18;
	uint32 RESERVED19;
	uint32 RESERVED20;
	uint32 RESERVED21;
	uint32 RESERVED22;
	uint32 RESERVED23;
	uint32 RESERVED24;
	uint32 RESERVED25;
	uint32 RESERVED26;
	uint32 ADC14_REF1P2V_TS30C;    // ADC14 1.2V Reference Temp. Sensor 30C
	uint32 ADC14_REF1P2V_TS85C;    // ADC14 1.2V Reference Temp. Sensor 85C
	uint32 ADC14_REF1P45V_TS30C;   // ADC14 1.45V Reference Temp. Sensor 30C
	uint32 ADC14_REF1P45V_TS85C;   // ADC14 1.45V Reference Temp. Sensor 85C
	uint32 ADC14_REF2P5V_TS30C;    // ADC14 2.5V Reference Temp. Sensor 30C
	uint32 ADC14_REF2P5V_TS85C;    // ADC14 2.5V Reference Temp. Sensor 85C
	uint32 REF_CAL_TAG;            // REF Calibration Tag
	uint32 REF_CAL_LEN;            // REF Calibration Length
	uint32 REF_1P2V;               // REF 1.2V Reference
	uint32 REF_1P45V;              // REF 1.45V Reference
	uint32 REF_2P5V;               // REF 2.5V Reference
	uint32 FLASH_INFO_TAG;         // Flash Info Tag
	uint32 FLASH_INFO_LEN;         // Flash Info Length
	uint32 FLASH_MAX_PROG_PULSES;  // Flash Maximum Programming Pulses
	uint32 FLASH_MAX_ERASE_PULSES; // Flash Maximum Erase Pulses
	uint32 RANDOM_NUM_TAG;         // 128-bit Random Number Tag
	uint32 RANDOM_NUM_LEN;         // 128-bit Random Number Length
	uint32 RANDOM_NUM_1;           // 32-bit Random Number 1
	uint32 RANDOM_NUM_2;           // 32-bit Random Number 2
	uint32 RANDOM_NUM_3;           // 32-bit Random Number 3
	uint32 RANDOM_NUM_4;           // 32-bit Random Number 4
	uint32 BSL_CFG_TAG;            // BSL Configuration Tag
	uint32 BSL_CFG_LEN;            // BSL Configuration Length
	uint32 BSL_PERIPHIF_SEL;       // BSL Peripheral Interface Selection
	uint32 BSL_PORTIF_CFG_UART;    // BSL Port Interface Configuration for UART
	uint32 BSL_PORTIF_CFG_SPI;     // BSL Port Interface Configuration for SPI
	uint32 BSL_PORTIF_CFG_I2C;     // BSL Port Interface Configuration for I2C
	uint32 TLV_END;                // TLV End Word
} TLV_Type;
#define TLV ((TLV_Type *) TLV_BASE) 


typedef struct {
	uint32_t CTL0;   //[IO] Control 0
	uint32_t CTL1;   //[IO] Control 1
	uint32_t IE;     //[IO] Interrupt Enable
	uint32_t IFG;    //[I ] Interrupt Flag
	uint32_t CLRIFG; //[O ] Clear Interrupt Flag
} PCM_Type;
#define PCM ((PCM_Type *) PCM_BASE)
//
#define PCM_CTL0_KEY_VAL                         ((uint32_t)0x695A0000)          /*!< PCM key value */
#define PCM_CTL1_KEY_VAL                         ((uint32_t)0x695A0000)          /*!< PCM key value */
#define PCM_CTL1_PMR_BUSY_OFS                    ( 8)                            /*!< PMR_BUSY Bit Offset */
#define PCM_CTL1_PMR_BUSY                        ((uint32_t)0x00000100)          /*!< Power mode request busy flag */



typedef struct {
	uint32 POWER_STAT;         // [I ] Power Status Register */
	uint32 RESERVED0[3];       // [  ]
	uint32 BANK0_RDCTL;        // [IO] Bank0 Read Control Register */
	uint32 BANK1_RDCTL;        // [IO] Bank1 Read Control Register */
	uint32 RESERVED1[2];       // [  ]
	uint32 RDBRST_CTLSTAT;     // [IO] Read Burst/Compare Control and Status Register */
	uint32 RDBRST_STARTADDR;   // [IO] Read Burst/Compare Start Address Register */
	uint32 RDBRST_LEN;         // [IO] Read Burst/Compare Length Register */
	uint32 RESERVED2[4];       // [  ]
	uint32 RDBRST_FAILADDR;    // [IO] Read Burst/Compare Fail Address Register */
	uint32 RDBRST_FAILCNT;     // [IO] Read Burst/Compare Fail Count Register */
	uint32 RESERVED3[3];       // [  ]
	uint32 PRG_CTLSTAT;        // [IO] Program Control and Status Register */
	uint32 PRGBRST_CTLSTAT;    // [IO] Program Burst Control and Status Register */
	uint32 PRGBRST_STARTADDR;  // [IO] Program Burst Start Address Register */
	uint32 RESERVED4;          // [  ]
	uint32 PRGBRST_DATA0_0;    // [IO] Program Burst Data0 Register0 */
	uint32 PRGBRST_DATA0_1;    // [IO] Program Burst Data0 Register1 */
	uint32 PRGBRST_DATA0_2;    // [IO] Program Burst Data0 Register2 */
	uint32 PRGBRST_DATA0_3;    // [IO] Program Burst Data0 Register3 */
	uint32 PRGBRST_DATA1_0;    // [IO] Program Burst Data1 Register0 */
	uint32 PRGBRST_DATA1_1;    // [IO] Program Burst Data1 Register1 */
	uint32 PRGBRST_DATA1_2;    // [IO] Program Burst Data1 Register2 */
	uint32 PRGBRST_DATA1_3;    // [IO] Program Burst Data1 Register3 */
	uint32 PRGBRST_DATA2_0;    // [IO] Program Burst Data2 Register0 */
	uint32 PRGBRST_DATA2_1;    // [IO] Program Burst Data2 Register1 */
	uint32 PRGBRST_DATA2_2;    // [IO] Program Burst Data2 Register2 */
	uint32 PRGBRST_DATA2_3;    // [IO] Program Burst Data2 Register3 */
	uint32 PRGBRST_DATA3_0;    // [IO] Program Burst Data3 Register0 */
	uint32 PRGBRST_DATA3_1;    // [IO] Program Burst Data3 Register1 */
	uint32 PRGBRST_DATA3_2;    // [IO] Program Burst Data3 Register2 */
	uint32 PRGBRST_DATA3_3;    // [IO] Program Burst Data3 Register3 */
	uint32 ERASE_CTLSTAT;      // [IO] Erase Control and Status Register */
	uint32 ERASE_SECTADDR;     // [IO] Erase Sector Address Register */
	uint32 RESERVED5[2];       // [  ]
	uint32 BANK0_INFO_WEPROT;  // [IO] Information Memory Bank0 Write/Erase Protection Register */
	uint32 BANK0_MAIN_WEPROT;  // [IO] Main Memory Bank0 Write/Erase Protection Register */
	uint32 RESERVED6[2];       // [  ]
	uint32 BANK1_INFO_WEPROT;  // [IO] Information Memory Bank1 Write/Erase Protection Register */
	uint32 BANK1_MAIN_WEPROT;  // [IO] Main Memory Bank1 Write/Erase Protection Register */
	uint32 RESERVED7[2];       // [  ]
	uint32 BMRK_CTLSTAT;       // [IO] Benchmark Control and Status Register */
	uint32 BMRK_IFETCH;        // [IO] Benchmark Instruction Fetch Count Register */
	uint32 BMRK_DREAD;         // [IO] Benchmark Data Read Count Register */
	uint32 BMRK_CMP;           // [IO] Benchmark Count Compare Register */
	uint32 RESERVED8[4];       // [  ]
	uint32 IFG;                // [IO] Interrupt Flag Register */
	uint32 IE;                 // [IO] Interrupt Enable Register */
	uint32 CLRIFG;             // [IO] Clear Interrupt Flag Register */
	uint32 SETIFG;             // [IO] Set Interrupt Flag Register */
	uint32 READ_TIMCTL;        // [I ] Read Timing Control Register */
	uint32 READMARGIN_TIMCTL;  // [I ] Read Margin Timing Control Register */
	uint32 PRGVER_TIMCTL;      // [I ] Program Verify Timing Control Register */
	uint32 ERSVER_TIMCTL;      // [I ] Erase Verify Timing Control Register */
	uint32 LKGVER_TIMCTL;      // [I ] Leakage Verify Timing Control Register */
	uint32 PROGRAM_TIMCTL;     // [I ] Program Timing Control Register */
	uint32 ERASE_TIMCTL;       // [I ] Erase Timing Control Register */
	uint32 MASSERASE_TIMCTL;   // [I ] Mass Erase Timing Control Register */
	uint32 BURSTPRG_TIMCTL;    // [I ] Burst Program Timing Control Register */
} FLCTL_Type;
#define FLCTL ((FLCTL_Type *) FLCTL_BASE)
//
#define FLCTL_BANK0_RDCTL_BUFD ((uint32_t)0x00000020) // Enables read buffering feature for data reads to this Bank
#define FLCTL_BANK1_RDCTL_BUFD ((uint32_t)0x00000020) // Enables read buffering feature for data reads to this Bank
#define FLCTL_BANK0_RDCTL_WAIT_MASK (0x0000F000) // WAIT Bit Mask
#define FLCTL_BANK1_RDCTL_WAIT_MASK (0x0000F000) // WAIT Bit Mask
#define FLCTL_BANK0_RDCTL_BUFI (0x00000010) // Enables read buffering feature for instruction fetches to this Bank
#define FLCTL_BANK1_RDCTL_BUFI (0x00000010) // Enables read buffering feature for instruction fetches to this Bank
#define FLCTL_BANK0_RDCTL_WAIT_1 (0x00001000) // 1 wait states
#define FLCTL_BANK1_RDCTL_WAIT_1 (0x00001000) // 1 wait states



#endif
