// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Stroage) SDCard
// Codifiers: @dosconio: 20250107
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

#include "../../../../../inc/cpp/Device/SD.hpp"
#include "../../../../../inc/cpp/Device/RCC/RCC"
#include "../../../../../inc/cpp/Device/GPIO"
#include "../../../../../inc/cpp/Device/SysTick"
#include "../../../../../inc/c/driver/interrupt/GIC.h"
#include "../../../../../inc/cpp/MCU/_ADDRESS/ADDR-STM32.h"

#define BLOCKSIZE 512

#define SD_INIT_FREQ           400000U // Initialization phase : 400 kHz max
#define SD_NORMAL_SPEED_FREQ 25000000U // Normal speed phase : 25 MHz max
#define SD_HIGH_SPEED_FREQ   50000000U // High speed phase : 50 MHz max

#define SDMMC_CMDTIMEOUT                   ((uint32_t)5000U)      // Command send and response timeout
#define SDMMC_MAXERASETIMEOUT              ((uint32_t)63000U)     // Max erase Timeout 63 s
#define SDMMC_STOPTRANSFERTIMEOUT          ((uint32_t)100000000U) // Timeout for STOP TRANSMISSION command

extern "C" int  outsfmt0(const char* fmt, ...);

namespace uni {
#if defined(_MPU_STM32MP13)

	// AKA HAL_SD_Init + HAL_SD_ConfigSpeedBusOperation
	// refer SDMMC_InitTypeDef
	//[pres]
	// // SDMMC_CLKSRC::PLL3: User should config RCCPLL::setMode and enable DIVR
	// // SDMMC_CLKSRC::PLL4: User should config RCCPLL::setMode and enable DIVP
	bool SecureDigitalCard_t::setMode(SDMMC_CLKSRC clk_src, bool clock_edge, bool powersave_enable, byte bus_width, bool hardware_flow_control_enable) {
		byte _id = getID() - 1;
		asrtret(_id < 2);
		//{TODO} USE_SD_TRANSCEIVER and then [TranceiverPresent] needed.
		//{TODO} SDMMC2
		//{TEMP} self.ClockDiv = SDMMC_NSPEED_CLK_DIV;
		/* GPIO Configuration
		GPIN    SDMMC
		PC8  --->  D0
		PC9  --->  D1
		PC11 --->  D3
		PC10 --->  D2
		PC12 --->  CK
		PD2  ---> CMD
		*/
		// assume state (reset)
		// assert_param
		asrtret(bus_width == 1 || bus_width == 4 || bus_width == 8);
		//{TODO} asrtret(.ClockDiv < 0x400U)
		//{TODO} if (USE_SD_TRANSCEIVER && hsd->Init.TranceiverPresent == SDMMC_TRANSCEIVER_UNKNOWN) hsd->Init.TranceiverPresent = SDMMC_TRANSCEIVER_PRESENT;
		//{}HAL_SD_ConfigSpeedBusOperation(&hsd1, SDMMC_SPEED_MODE_HIGH);// Try to switch to High Speed Mode , if supported by the card
		//() there may be callbacks set
		// AKA HAL_SD_MspInit
		//	// AKA Partial HAL_RCCEx_PeriphCLKConfig
		setClockSource(clk_src);
		//	//{TODO DMA}
		RCC[RCCReg::MP_AHB6ENSETR].setof(16 + _id);
		if (getID() == 1) {

			// __HAL_RCC_SDMMC1_CONFIG
			// GPIN Configure
			for (unsigned i = 8; i < 13; i++) {
				GPIOC[i].setMode(GPIOMode::OUT_AF_PushPull, GPIOSpeed::High);
				GPIOC[i]._set_alternate(12);// GPIO_AF12_SDIO1
			}
			GPIOD[2].setMode(GPIOMode::OUT_AF_PushPull, GPIOSpeed::High);
			GPIOD[2]._set_alternate(12);// GPIO_AF12_SDIO1
			// GVIC configuration for SDIO interrupts
			GIC.setPriority(IRQ_SDMMC1, 0x00);
			GIC.enInterrupt(IRQ_SDMMC1);

		}
		// state AKA HAL_SD_STATE_PROGRAMMING
		// Initialize the Card parameters
		asrtret(setModeSub());
		return true;
	}

	bool SecureDigitalCard_t::setModeSub() {
		stduint sdmmc_clk = getFrequency();
		// Default SDMMC peripheral configuration for SD card initialization
		bool clock_edge = true;// Init.ClockEdge
		bool powersave_enable = false;// Init.ClockPowerSave
		byte bus_width = 1;// Init.BusWide
		bool hardware_flow_control_enable = false;// Init.HardwareFlowControl
		//
		asrtret(sdmmc_clk);
		stduint Init_ClockDiv = sdmmc_clk / (2U * SD_INIT_FREQ);
		// Set Transceiver polarity
	#if (USE_SD_TRANSCEIVER != 0U)//{TEMP} no-use
		Init.TranceiverPresent = hsd->Init.TranceiverPresent;
		if (hsd->Init.TranceiverPresent == SDMMC_TRANSCEIVER_PRESENT)
			hsd->Instance->POWER |= SDMMC_POWER_DIRPOL;
	#elif defined (USE_SD_DIRPOL)//{TEMP} no-use
		hsd->Instance->POWER |= SDMMC_POWER_DIRPOL;
	#endif /* USE_SD_TRANSCEIVER  */
		// AKA SDMMC_Init, Initialize SDMMC peripheral interface with default configuration
		self[SDReg::CLKCR] = 0;
		setClockEdge(clock_edge);
		setClockPowerSave(powersave_enable);
		setBusWide(bus_width);
		setHardwareFlowControl(hardware_flow_control_enable);
		setClockDiv(Init_ClockDiv);
		// AKA SDMMC_PowerState_ON, 
		self[SDReg::POWER].maset(0, 2, 0b11);// PWRCTRL
		// wait 74 Cycles: required power up waiting time before starting	the SD initialization sequence
		if (Init_ClockDiv) sdmmc_clk /= 2 * Init_ClockDiv;
		if (sdmmc_clk) SysDelay(1U + (74U * SysTickHz / sdmmc_clk));

		asrtret(SD_PowerON(NULL));
		asrtret(SD_InitCard(NULL));{}{}
		if (!SDMMC_CmdBlockLength(BLOCKSIZE)) {
			self[SDReg::ICR] = (_IMM1S(0U)) | (_IMM1S(1U)) | (_IMM1S(2U)) |
				(_IMM1S(3U)) | (_IMM1S(4U)) | (_IMM1S(5U)) |
				(_IMM1S(6U)) | (_IMM1S(7U)) | (_IMM1S(8U)) |
				(_IMM1S(9U)) | (_IMM1S(10U)) | (_IMM1S(11U)) |
				(_IMM1S(21U)) | (_IMM1S(22U)) | (_IMM1S(23U)) |
				(_IMM1S(24U)) | (_IMM1S(25U)) | (_IMM1S(26U)) |
				(_IMM1S(27U)) | (_IMM1S(28U));// __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_FLAGS)
			return false;
		}
		return true;
	}



#endif
}
