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

#define bitmatch(bits,mask) (((bits) & (mask)) == (mask))
#define statin inline static

#define SD_INIT_FREQ           400000U // Initialization phase : 400 kHz max
#define SD_NORMAL_SPEED_FREQ 25000000U // Normal speed phase : 25 MHz max
#define SD_HIGH_SPEED_FREQ   50000000U // High speed phase : 50 MHz max

#define SDMMC_CMDTIMEOUT                   ((uint32_t)5000U)      // Command send and response timeout
#define SDMMC_MAXERASETIMEOUT              ((uint32_t)63000U)     // Max erase Timeout 63 s
#define SDMMC_STOPTRANSFERTIMEOUT          ((uint32_t)100000000U) // Timeout for STOP TRANSMISSION command

namespace uni {
#if defined(_MPU_STM32MP13)
#include "../../../../../inc/cpp/Device/Storage/SD-PARA.h"

	// Set SDMMC Power state to OFF
	// AKA SD_PowerOFF
	statin void SDMMC_PowerState_OFF(const SecureDigitalCard_t& sd) {
		sd[SDReg::POWER].maset(0, 2, nil);// PWRCTRL
	}
	
	// AKA HAL_SD_Init + HAL_SD_ConfigSpeedBusOperation
	// refer SDMMC_InitTypeDef
	//[pres]
	// // SDMMC_CLKSRC::PLL3: User should config RCCPLL::setMode and enable DIVR
	// // SDMMC_CLKSRC::PLL4: User should config RCCPLL::setMode and enable DIVP
	bool SecureDigitalCard_t::setMode(SDMMC_CLKSRC clk_src, bool clock_edge, bool powersave_enable, SDMMC_BusWidth bus_width, bool hardware_flow_control_enable) {
		byte _id = getID() - 1;
		asrtret(_id < 2);
		//
		//{?} BSP_IOEXPANDER_Init(0, IOEXPANDER_IO_MODE);
		//{?} LL_ETZPC_SetSecureSysRamSize(ETZPC, 0); after __HAL_RCC_TZC_CLK_ENABLE
		//
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
		//{TODO} asrtret(.ClockDiv < 0x400U)
		//{TODO} if (USE_SD_TRANSCEIVER && hsd->Init.TranceiverPresent == SDMMC_TRANSCEIVER_UNKNOWN) hsd->Init.TranceiverPresent = SDMMC_TRANSCEIVER_PRESENT;
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
		//
		{
			uint32 tmp;
			HAL_SD_CardStatusTypeDef CardStatus;
			uint32 speedgrade, unitsize;
			asrtret(HAL_SD_GetCardStatus(&CardStatus, &tmp));
			// Get Initial Card Speed from Card Status
			speedgrade = CardStatus.UhsSpeedGrade;
			unitsize = CardStatus.UhsAllocationUnitSize;
			if ((CardType == CardType_E ::SDHC_SDXC) && ((speedgrade != 0U) || (unitsize != 0U)))
			{
				CardInfo.CardSpeed = CARD_ULTRA_HIGH_SPEED;
			}
			else {
				CardInfo.CardSpeed = (CardType == CardType_E::SDHC_SDXC) ? CARD_HIGH_SPEED : CARD_NORMAL_SPEED;
			}
		}
		// Configure the bus wide
		asrtret(HAL_SD_ConfigWideBusOperation(clock_edge, powersave_enable, bus_width, hardware_flow_control_enable, NULL));
		// Verify that SD card is ready to use after Initialization
		while ((HAL_SD_GetCardState() != HAL_SD_CardStateTypeDef::TRANSFER));
		Context = SDContext::NONE;
		asrtret (HAL_SD_ConfigSpeedBusOperation(SDMMC_SPEED_MODE::HIGH));// Try to switch to High Speed Mode , if supported by the card
		return true;
	}

	bool SecureDigitalCard_t::setModeSub() {
		stduint sdmmc_clk = getFrequency();
		// Default SDMMC peripheral configuration for SD card initialization
		bool clock_edge = true;// Init.ClockEdge
		bool powersave_enable = false;// Init.ClockPowerSave
		auto bus_width = SDMMC_BusWidth::Bits1;// Init.BusWide
		bool hardware_flow_control_enable = false;// Init.HardwareFlowControl
		//
		asrtret(sdmmc_clk);
		stduint Init_ClockDiv = sdmmc_clk / (2U * SD_INIT_FREQ);
		// Set Transceiver polarity
	#if (USE_SD_TRANSCEIVER != 0U)//{TEMP} no-use
		Init.TranceiverPresent = hsd->Init.TranceiverPresent;
		if (hsd->Init.TranceiverPresent == SDMMC_TRANSCEIVER_PRESENT)
			self[SDReg::POWER] |= SDMMC_POWER_DIRPOL;
	#elif defined (USE_SD_DIRPOL)//{TEMP} no-use
		self[SDReg::POWER] |= SDMMC_POWER_DIRPOL;
	#endif /* USE_SD_TRANSCEIVER  */
		// AKA SDMMC_Init, Initialize SDMMC peripheral interface with default configuration
		{
			Reflocal(clkcr) = 0;
			setClockEdge(clkcr, clock_edge);
			setClockPowerSave(clkcr, powersave_enable);
			setBusWide(clkcr, bus_width);
			setHardwareFlowControl(clkcr, hardware_flow_control_enable);
			setClockDiv(clkcr, last_ClockDiv = Init_ClockDiv);
			self[SDReg::CLKCR] = clkcr;
		}
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

	bool SecureDigitalCard_t::canMode() {
	#if (USE_SD_TRANSCEIVER != 0U)//{TODO}
		// Deactivate the 1.8V Mode */
		if (hsd->Init.TranceiverPresent == SDMMC_TRANSCEIVER_PRESENT)
		{
		#if defined (USE_HAL_SD_REGISTER_CALLBACKS) && (USE_HAL_SD_REGISTER_CALLBACKS == 1U)
			if (hsd->DriveTransceiver_1_8V_Callback == NULL)
			{
				hsd->DriveTransceiver_1_8V_Callback = HAL_SD_DriveTransceiver_1_8V_Callback;
			}
			hsd->DriveTransceiver_1_8V_Callback(RESET);
		#else
			HAL_SD_DriveTransceiver_1_8V_Callback(RESET);
		#endif // USE_HAL_SD_REGISTER_CALLBACKS
		}
	#endif // USE_SD_TRANSCEIVER
		SDMMC_PowerState_OFF(self);
		//{} DeInitCallback
		return true;
	}

#endif
}
