// ASCII C++-11 TAB4 CRLF
// LastCheck: 20241029
// AllAuthor: @dosconio
// ModuTitle: (MCU) STM32H7 Series
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

#include "../../../inc/c/stdinc.h"
#ifdef _MCU_STM32H7x

#include "../../../inc/c/prochip/CortexM7.h"// main core, the other is m4
#include "../../../inc/cpp/MCU/_ADDRESS/ADDR-STM32.h"
#include "../../../inc/cpp/Device/GPIO"
#include "../../../inc/cpp/Device/RCC/RCC"

extern "C" void SystemInit(void);

extern "C" {
	_WEAK char _IDN_BOARD[16] = "STM32H7";
	_WEAK bool useExtSRAM(void) { return false; }
	_WEAK bool useExtSDRAM(void) { return false; }
	_WEAK bool useInnSRAM_IVT(void) { return false; }
	_WEAK bool useFPU(void) { return true; }
}

#define VECT_TAB_OFFSET  0x00


static void SystemInit_ExtMemCtl(void);
void SystemInit(void) {
	using namespace uni;
	if (useFPU()) SCB->CPACR |= ((3UL << 10 * 2) | (3UL << 11 * 2));  /* set CP10 and CP11 Full Access */
	/* Reset the RCC clock configuration to the default reset state ------------*/
	/* Set HSION bit */
	RCC[RCCReg::CR] |= 0x00000001; // RCC_CR_HSION;
	/* Reset CFGR register */
	RCC[RCCReg::CFGR] = 0x00000000;
	RCC[RCCReg::CR] &= (uint32_t)0xEAF6ED7F;
	RCC[RCCReg::D1CFGR] = 0x00000000;
	RCC[RCCReg::D2CFGR] = 0x00000000;
	RCC[RCCReg::D3CFGR] = 0x00000000;
	RCC[RCCReg::PLLCKSELR] = 0x00000000;
	RCC[RCCReg::PLLCFGR] = 0x00000000;
	RCC[RCCReg::PLL1DIVR] = 0x00000000;
	RCC[RCCReg::PLL1FRACR] = 0x00000000;
	RCC[RCCReg::PLL2DIVR] = 0x00000000;
	RCC[RCCReg::PLL2FRACR] = 0x00000000;
	RCC[RCCReg::PLL3DIVR] = 0x00000000;
	RCC[RCCReg::PLL3FRACR] = 0x00000000;
	/* Reset HSEBYP bit */
	RCC[RCCReg::CR] &= (uint32_t)0xFFFBFFFF;
	/* Disable all interrupts */
	RCC[RCCReg::CIER] = 0x00000000;
	/* Change  the switch matrix read issuing capability to 1 for the AXI SRAM target (Target 7) */
	*((uint32_t*)0x51008108) = 0x00000001;
	SystemInit_ExtMemCtl();
	SCB->VTOR = VECT_TAB_OFFSET | (useInnSRAM_IVT() ? D1_AXISRAM_BASE : FLASH_BANK1_BASE);
}


namespace uni {



	
}

// This function configures the external memories (SRAM/SDRAM)
static void SystemInit_ExtMemCtl(void) {
	using namespace uni;
	if (!useExtSRAM() && !useExtSDRAM()) return;
	uint32_t tmp = 0;
#if 0 //{TODO}
	if (useExtSRAM() && useExtSDRAM()) {
		uint32_t tmpreg = 0, timeout = 0xFFFF;
		uint32_t index;
		/* Enable GPIOD, GPIOE, GPIOF, GPIOG, GPIOH and GPIOI interface clock */
		for0(i, 6) GPIO['D' + i].enClock();
		/* Connect PDx pins to FMC Alternate function */
		GPIOD[GPIOReg::AFRL] = 0x00CC00CC;
		GPIOD[GPIOReg::AFRH] = 0xCCCCCCCC;
		/* Configure PDx pins in Alternate function mode */
		GPIOD[GPIOReg::MODER] = 0xAAAAFAFA;
		/* Configure PDx pins speed to 100 MHz */
		GPIOD[GPIOReg::SPEED] = 0xFFFF0F0F;
		/* Configure PDx pins Output type to push-pull */
		GPIOD[GPIOReg::OTYPER] = 0x00000000;
		/* Configure PDx pins in Pull-up */
		GPIOD[GPIOReg::PULLS] = 0x55550505;
		/* Connect PEx pins to FMC Alternate function */
		GPIOE[GPIOReg::AFRL] = 0xC00CC0CC;
		GPIOE[GPIOReg::AFRH] = 0xCCCCCCCC;
		/* Configure PEx pins in Alternate function mode */
		GPIOE[GPIOReg::MODER] = 0xAAAABEBA;
		/* Configure PEx pins speed to 100 MHz */
		GPIOE[GPIOReg::SPEED]= 0xFFFFC3CF;
		/* Configure PEx pins Output type to push-pull */
		GPIOE[GPIOReg::OTYPER]= 0x00000000;
		/* Configure PEx pins in Pull-up */
		GPIOE[GPIOReg::PULLS] = 0x55554145;
		/* Connect PFx pins to FMC Alternate function */
		GPIOF[GPIOReg::AFRL] = 0x00CCCCCC;
		GPIOF[GPIOReg::AFRH] = 0xCCCCC000;
		/* Configure PFx pins in Alternate function mode */
		GPIOF[GPIOReg::MODER] = 0xAABFFAAA;
		/* Configure PFx pins speed to 100 MHz */
		GPIOF[GPIOReg::SPEED]= 0xFFC00FFF;
		/* Configure PFx pins Output type to push-pull */
		GPIOF[GPIOReg::OTYPER]= 0x00000000;
		/* Configure PFx pins in Pull-up */
		GPIOF[GPIOReg::PULLS] = 0x55400555;
		/* Connect PGx pins to FMC Alternate function */
		GPIOG[GPIOReg::AFRL] = 0x00CCCCCC;
		GPIOG[GPIOReg::AFRH] = 0xC0000C0C;
		/* Configure PGx pins in Alternate function mode */
		GPIOG[GPIOReg::MODER] = 0xBFEEFAAA;
		/* Configure PGx pins speed to 100 MHz */
		GPIOG[GPIOReg::SPEED]= 0xC0330FFF;
		/* Configure PGx pins Output type to push-pull */
		GPIOG[GPIOReg::OTYPER]= 0x00000000;
		/* Configure PGx pins in Pull-up */
		GPIOG[GPIOReg::PULLS] = 0x40110555;
		/* Connect PHx pins to FMC Alternate function */
		GPIOH[GPIOReg::AFRL] = 0xCCC00000;
		GPIOH[GPIOReg::AFRH] = 0xCCCCCCCC;
		/* Configure PHx pins in Alternate function mode */
		GPIOH[GPIOReg::MODER] = 0xAAAAABFF;
		/* Configure PHx pins speed to 100 MHz */
		GPIOH[GPIOReg::SPEED]= 0xFFFFFC00;
		/* Configure PHx pins Output type to push-pull */
		GPIOH[GPIOReg::OTYPER]= 0x00000000;
		/* Configure PHx pins in Pull-up */
		GPIOH[GPIOReg::PULLS] = 0x55555400;
		/* Connect PIx pins to FMC Alternate function */
		GPIOI[GPIOReg::AFRL] = 0xCCCCCCCC;
		GPIOI[GPIOReg::AFRH] = 0x00000CC0;
		/* Configure PIx pins in Alternate function mode */
		GPIOI[GPIOReg::MODER] = 0xFFEBAAAA;
		/* Configure PIx pins speed to 100 MHz */
		GPIOI[GPIOReg::SPEED]= 0x003CFFFF;
		/* Configure PIx pins Output type to push-pull */
		GPIOI[GPIOReg::OTYPER]= 0x00000000;
		/* Configure PIx pins in Pull-up */
		GPIOI[GPIOReg::PULLS] = 0x00145555;
		/* Enable the FMC/FSMC interface clock */
		(RCC[RCCReg::AHB3ENR |= (RCC_AHB3ENR_FMCEN));
		/* Configure and enable Bank1_SRAM2 */
		FMC_Bank1->BTCR[4] = 0x00001091;
		FMC_Bank1->BTCR[5] = 0x00110212;
		FMC_Bank1E->BWTR[4] = 0x0FFFFFFF;
		/*SDRAM Timing and access interface configuration*/
		/*LoadToActiveDelay  = 2
		  ExitSelfRefreshDelay = 6
		  SelfRefreshTime      = 4
		  RowCycleDelay        = 6
		  WriteRecoveryTime    = 2
		  RPDelay              = 2
		  RCDDelay             = 2
		  SDBank             = FMC_SDRAM_BANK2
		  ColumnBitsNumber   = FMC_SDRAM_COLUMN_BITS_NUM_9
		  RowBitsNumber      = FMC_SDRAM_ROW_BITS_NUM_12
		  MemoryDataWidth    = FMC_SDRAM_MEM_BUS_WIDTH_32
		  InternalBankNumber = FMC_SDRAM_INTERN_BANKS_NUM_4
		  CASLatency         = FMC_SDRAM_CAS_LATENCY_2
		  WriteProtection    = FMC_SDRAM_WRITE_PROTECTION_DISABLE
		  SDClockPeriod      = FMC_SDRAM_CLOCK_PERIOD_2
		  ReadBurst          = FMC_SDRAM_RBURST_ENABLE
		  ReadPipeDelay      = FMC_SDRAM_RPIPE_DELAY_0*/
		FMC_Bank5_6->SDCR[0] = 0x00001800;
		FMC_Bank5_6->SDCR[1] = 0x00000165;
		FMC_Bank5_6->SDTR[0] = 0x00105000;
		FMC_Bank5_6->SDTR[1] = 0x01010351;
		/* SDRAM initialization sequence */
		/* Clock enable command */
		FMC_Bank5_6->SDCMR = 0x00000009;
		tmpreg = FMC_Bank5_6->SDSR & 0x00000020;
		while ((tmpreg != 0) && (timeout-- > 0))
		{
			tmpreg = FMC_Bank5_6->SDSR & 0x00000020;
		}
		/* Delay */
		for (index = 0; index < 1000; index++);
		/* PALL command */
		FMC_Bank5_6->SDCMR = 0x0000000A;
		timeout = 0xFFFF;
		while ((tmpreg != 0) && (timeout-- > 0))
		{
			tmpreg = FMC_Bank5_6->SDSR & 0x00000020;
		}

		FMC_Bank5_6->SDCMR = 0x000000EB;
		timeout = 0xFFFF;
		while ((tmpreg != 0) && (timeout-- > 0))
		{
			tmpreg = FMC_Bank5_6->SDSR & 0x00000020;
		}
		FMC_Bank5_6->SDCMR = 0x0004400C;
		timeout = 0xFFFF;
		while ((tmpreg != 0) && (timeout-- > 0))
		{
			tmpreg = FMC_Bank5_6->SDSR & 0x00000020;
		}
		/* Set refresh count */
		tmpreg = FMC_Bank5_6->SDRTR;
		FMC_Bank5_6->SDRTR = (tmpreg | (0x00000603 << 1));
		/* Disable write protection */
		tmpreg = FMC_Bank5_6->SDCR[1];
		FMC_Bank5_6->SDCR[1] = (tmpreg & 0xFFFFFDFF);
		 /*FMC controller Enable*/
		FMC_Bank1->BTCR[0] |= 0x80000000;
	}
	else if (useExtSDRAM()) {
		uint32_t tmpreg = 0, timeout = 0xFFFF;
		uint32_t index;
		/* Enable GPIOD, GPIOE, GPIOF, GPIOG, GPIOH and GPIOI interface clock */
		for0(i, 6) GPIO['D' + i].enClock();
		/* Connect PDx pins to FMC Alternate function */
		GPIOD[GPIOReg::AFRL] = 0x000000CC;
		GPIOD[GPIOReg::AFRH] = 0xCC000CCC;
		/* Configure PDx pins in Alternate function mode */
		GPIOD[GPIOReg::MODER] = 0xAFEAFFFA;
		/* Configure PDx pins speed to 100 MHz */
		GPIOD[GPIOReg::SPEED]= 0xF03F000F;
		/* Configure PDx pins Output type to push-pull */
		GPIOD[GPIOReg::OTYPER]= 0x00000000;
		/* Configure PDx pins in Pull-up */
		GPIOD[GPIOReg::PULLS] = 0x50150005;
		/* Connect PEx pins to FMC Alternate function */
		GPIOE[GPIOReg::AFRL] = 0xC00000CC;
		GPIOE[GPIOReg::AFRH] = 0xCCCCCCCC;
		/* Configure PEx pins in Alternate function mode */
		GPIOE[GPIOReg::MODER] = 0xAAAABFFA;
		/* Configure PEx pins speed to 100 MHz */
		GPIOE[GPIOReg::SPEED]= 0xFFFFC00F;
		/* Configure PEx pins Output type to push-pull */
		GPIOE[GPIOReg::OTYPER]= 0x00000000;
		/* Configure PEx pins in Pull-up */
		GPIOE[GPIOReg::PULLS] = 0x55554005;
		/* Connect PFx pins to FMC Alternate function */
		GPIOF[GPIOReg::AFRL] = 0x00CCCCCC;
		GPIOF[GPIOReg::AFRH] = 0xCCCCC000;
		/* Configure PFx pins in Alternate function mode */
		GPIOF[GPIOReg::MODER] = 0xAABFFAAA;
		/* Configure PFx pins speed to 100 MHz */
		GPIOF[GPIOReg::SPEED]= 0xFFC00FFF;
		/* Configure PFx pins Output type to push-pull */
		GPIOF[GPIOReg::OTYPER]= 0x00000000;
		/* Configure PFx pins in Pull-up */
		GPIOF[GPIOReg::PULLS] = 0x55400555;
		/* Connect PGx pins to FMC Alternate function */
		GPIOG[GPIOReg::AFRL] = 0x00CCCCCC;
		GPIOG[GPIOReg::AFRH] = 0xC000000C;
		/* Configure PGx pins in Alternate function mode */
		GPIOG[GPIOReg::MODER] = 0xBFFEFAAA;
	   /* Configure PGx pins speed to 100 MHz */
		GPIOG[GPIOReg::SPEED]= 0xC0030FFF;
		/* Configure PGx pins Output type to push-pull */
		GPIOG[GPIOReg::OTYPER]= 0x00000000;
		/* Configure PGx pins in Pull-up */
		GPIOG[GPIOReg::PULLS] = 0x40010555;
		/* Connect PHx pins to FMC Alternate function */
		GPIOH[GPIOReg::AFRL] = 0xCCC00000;
		GPIOH[GPIOReg::AFRH] = 0xCCCCCCCC;
		/* Configure PHx pins in Alternate function mode */
		GPIOH[GPIOReg::MODER] = 0xAAAAABFF;
		/* Configure PHx pins speed to 100 MHz */
		GPIOH[GPIOReg::SPEED]= 0xFFFFFC00;
		/* Configure PHx pins Output type to push-pull */
		GPIOH[GPIOReg::OTYPER]= 0x00000000;
		/* Configure PHx pins in Pull-up */
		GPIOH[GPIOReg::PULLS] = 0x55555400;
		/* Connect PIx pins to FMC Alternate function */
		GPIOI[GPIOReg::AFRL] = 0xCCCCCCCC;
		GPIOI[GPIOReg::AFRH] = 0x00000CC0;
		/* Configure PIx pins in Alternate function mode */
		GPIOI[GPIOReg::MODER] = 0xFFEBAAAA;
		/* Configure PIx pins speed to 100 MHz */
		GPIOI[GPIOReg::SPEED]= 0x003CFFFF;
		/* Configure PIx pins Output type to push-pull */
		GPIOI[GPIOReg::OTYPER]= 0x00000000;
		/* Configure PIx pins in Pull-up */
		GPIOI[GPIOReg::PULLS] = 0x00145555;
		/*-- FMC Configuration ------------------------------------------------------*/
		/* Enable the FMC interface clock */
		(RCC[RCCReg::AHB3ENR |= (RCC_AHB3ENR_FMCEN));
		/*SDRAM Timing and access interface configuration*/
		/*LoadToActiveDelay  = 2
		  ExitSelfRefreshDelay = 6
		  SelfRefreshTime      = 4
		  RowCycleDelay        = 6
		  WriteRecoveryTime    = 2
		  RPDelay              = 2
		  RCDDelay             = 2
		  SDBank             = FMC_SDRAM_BANK2
		  ColumnBitsNumber   = FMC_SDRAM_COLUMN_BITS_NUM_9
		  RowBitsNumber      = FMC_SDRAM_ROW_BITS_NUM_12
		  MemoryDataWidth    = FMC_SDRAM_MEM_BUS_WIDTH_32
		  InternalBankNumber = FMC_SDRAM_INTERN_BANKS_NUM_4
		  CASLatency         = FMC_SDRAM_CAS_LATENCY_2
		  WriteProtection    = FMC_SDRAM_WRITE_PROTECTION_DISABLE
		  SDClockPeriod      = FMC_SDRAM_CLOCK_PERIOD_2
		  ReadBurst          = FMC_SDRAM_RBURST_ENABLE
		  ReadPipeDelay      = FMC_SDRAM_RPIPE_DELAY_0*/
		FMC_Bank5_6->SDCR[0] = 0x00001800;
		FMC_Bank5_6->SDCR[1] = 0x00000165;
		FMC_Bank5_6->SDTR[0] = 0x00105000;
		FMC_Bank5_6->SDTR[1] = 0x01010351;
		/* SDRAM initialization sequence */
		/* Clock enable command */
		FMC_Bank5_6->SDCMR = 0x00000009;
		tmpreg = FMC_Bank5_6->SDSR & 0x00000020;
		while ((tmpreg != 0) && (timeout-- > 0))
		{
			tmpreg = FMC_Bank5_6->SDSR & 0x00000020;
		}
		for (index = 0; index < 1000; index++);
		/* PALL command */
		FMC_Bank5_6->SDCMR = 0x0000000A;
		timeout = 0xFFFF;
		while ((tmpreg != 0) && (timeout-- > 0))
		{
			tmpreg = FMC_Bank5_6->SDSR & 0x00000020;
		}
		FMC_Bank5_6->SDCMR = 0x000000EB;
		timeout = 0xFFFF;
		while ((tmpreg != 0) && (timeout-- > 0))
		{
			tmpreg = FMC_Bank5_6->SDSR & 0x00000020;
		}
		FMC_Bank5_6->SDCMR = 0x0004400C;
		timeout = 0xFFFF;
		while ((tmpreg != 0) && (timeout-- > 0))
		{
			tmpreg = FMC_Bank5_6->SDSR & 0x00000020;
		}
		/* Set refresh count */
		tmpreg = FMC_Bank5_6->SDRTR;
		FMC_Bank5_6->SDRTR = (tmpreg | (0x00000603 << 1));
		/* Disable write protection */
		tmpreg = FMC_Bank5_6->SDCR[1];
		FMC_Bank5_6->SDCR[1] = (tmpreg & 0xFFFFFDFF);
		/*FMC controller Enable*/
		FMC_Bank1->BTCR[0] |= 0x80000000;
	}
	else if (useExtSRAM()) {
		/*-- GPIOs Configuration -----------------------------------------------------*/
		/* Enable GPIOD, GPIOE, GPIOF and GPIOG interface clock */
		for0(i, 4) GPIO['D' + i].enClock();
		/* Connect PDx pins to FMC Alternate function */
		GPIOD[GPIOReg::AFRL] = 0x00CC00CC;
		GPIOD[GPIOReg::AFRH] = 0xCCCCCCCC;
		/* Configure PDx pins in Alternate function mode */
		GPIOD[GPIOReg::MODER] = 0xAAAAFABA;
		/* Configure PDx pins speed to 100 MHz */
		GPIOD[GPIOReg::SPEED]= 0xFFFF0F0F;
		/* Configure PDx pins Output type to push-pull */
		GPIOD[GPIOReg::OTYPER]= 0x00000000;
		/* Configure PDx pins in Pull-up */
		GPIOD[GPIOReg::PULLS] = 0x55550505;
		/* Connect PEx pins to FMC Alternate function */
		GPIOE[GPIOReg::AFRL] = 0xC00CC0CC;
		GPIOE[GPIOReg::AFRH] = 0xCCCCCCCC;
		/* Configure PEx pins in Alternate function mode */
		GPIOE[GPIOReg::MODER] = 0xAAAABEBA;
		/* Configure PEx pins speed to 100 MHz */
		GPIOE[GPIOReg::SPEED]= 0xFFFFC3CF;
		/* Configure PEx pins Output type to push-pull */
		GPIOE[GPIOReg::OTYPER]= 0x00000000;
		/* Configure PEx pins in Pull-up */
		GPIOE[GPIOReg::PULLS] = 0x55554145;
		/* Connect PFx pins to FMC Alternate function */
		GPIOF[GPIOReg::AFRL] = 0x00CCCCCC;
		GPIOF[GPIOReg::AFRH] = 0xCCCC0000;
		/* Configure PFx pins in Alternate function mode */
		GPIOF[GPIOReg::MODER] = 0xAAFFFAAA;
		/* Configure PFx pins speed to 100 MHz */
		GPIOF[GPIOReg::SPEED]= 0xFF000FFF;
		/* Configure PFx pins Output type to push-pull */
		GPIOF[GPIOReg::OTYPER]= 0x00000000;
		/* Configure PFx pins in Pull-up */
		GPIOF[GPIOReg::PULLS] = 0x55000555;
		/* Connect PGx pins to FMC Alternate function */
		GPIOG[GPIOReg::AFRL] = 0x00CCCCCC;
		GPIOG[GPIOReg::AFRH] = 0x00000C00;
		/* Configure PGx pins in Alternate function mode */
		GPIOG[GPIOReg::MODER] = 0xFFEFFAAA;
		/* Configure PGx pins speed to 100 MHz */
		GPIOG[GPIOReg::SPEED]= 0x00300FFF;
		/* Configure PGx pins Output type to push-pull */
		GPIOG[GPIOReg::OTYPER]= 0x00000000;
		/* Configure PGx pins in Pull-up */
		GPIOG[GPIOReg::PULLS] = 0x00100555;
		/*-- FMC/FSMC Configuration --------------------------------------------------*/
		/* Enable the FMC/FSMC interface clock */
		(RCC[RCCReg::AHB3ENR |= (RCC_AHB3ENR_FMCEN));
		/* Configure and enable Bank1_SRAM2 */
		FMC_Bank1->BTCR[4] = 0x00001091;
		FMC_Bank1->BTCR[5] = 0x00110212;
		FMC_Bank1E->BWTR[4] = 0x0FFFFFFF;
		/*FMC controller Enable*/
		FMC_Bank1->BTCR[0] |= 0x80000000;
	}
#endif
}

#endif
