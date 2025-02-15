// ASCII C++-11 TAB4 CRLF
// LastCheck: 20241007
// AllAuthor: @dosconio
// ModuTitle: (MCU) Texas Instruments MSP432P4x
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
#ifdef _MCU_MSP432P4

/*--------------------- Configuration Instructions ----------------------------
   1. use_wdog
   2. Insert your desired CPU frequency in Hz at:
   #define __SYSTEM_CLOCK   12000000
   3. If we prefer the DC-DC power regulator (more efficient at higher
	   frequencies), set the __REGULATOR to 1:
   #define __REGULATOR      1
 *---------------------------------------------------------------------------*/


_WEAK bool use_wdog = false;// not using is to halt the watchdog timer
_WEAK bool use_fpu = true;

/*--------------------- CPU Frequency Configuration -------------------------*/
// 1.5 MHz, [3 MHz], 12 MHz, 24 MHz, 48 MHz
_WEAK stduint __SYSTEM_CLOCK = 3000000;

/*--------------------- Power Regulator Configuration -----------------------*/
//  Power Regulator Mode
//     <0> LDO
//     <1> DC-DC
_WEAK bool __REGULATOR = 0;

// ---- ---- ---- ---- ---- ---- ---- ---- //

#define _OPT_PCU_CortexM4F
#include "../../../inc/c/prochip/CortexM4.h"
#include "../../../inc/c/MCU/MSP432/MSP432P4.h"
#include "../../../inc/cpp/MCU/TI/MSP432P4"

extern "C" void SystemInit(void);
extern "C" void SystemInitA(void);

extern "C" char _IDN_BOARD[16];
_WEAK char _IDN_BOARD[16] = "xx";// e.g. "01R" for MSP-EXP432P401R

_Comment("[TEMP] for system_msp432p401r.c")

//{TEMP} 401R
// 1. Enables the FPU
// 2. Halts the WDT if requested
// 3. Enables all SRAM banks
// 4. Sets up power regulator and VCORE
// 5. Enable Flash wait states if needed
// 6. Change MCLK to desired frequency
// 7. Enable Flash read buffering
void SystemInit(void) {
	using namespace uni;
	
	if (use_fpu || true) { // why add true: MCU may not load fpu initializing true
		SCB->CPACR |= ((3UL << 10 * 2) | // (0xE000ED88) Set CP10 Full Access
			(3UL << 11 * 2)); // Set CP11 Full Access
	}

	if (!use_wdog) {
		WdogA[WDogReg::CTL] = WDT_A_CTL_PW | WDT_A_CTL_HOLD; // Halt the WDT
	}
	
	Stdfield DCORSEL(&CS->CTL0, 16, 3);
	SYSCTL->SRAM_BANKEN = SYSCTL_SRAM_BANKEN_BNK7_EN;      // Enable all SRAM banks
	const uint32_t mask_selm_divm = 0x7 | 0x70000;// CS_CTL1_SELM_MASK | CS_CTL1_DIVM_MASK;

	switch (__SYSTEM_CLOCK)
	{
	case 1500000:
		// Default VCORE is LDO VCORE0 so no change necessary
		if (__REGULATOR) { // Switches LDO VCORE0 to DCDC VCORE0 if requested
			while ((PCM->CTL1 & PCM_CTL1_PMR_BUSY));
			PCM->CTL0 = PCM_CTL0_KEY_VAL | 4 _Comment(PCM_CTL0_AMR_4);
			while ((PCM->CTL1 & PCM_CTL1_PMR_BUSY));
		}
		// No flash wait states necessary
		// DCO = 1.5 MHz; MCLK = source
		CS->KEY = CS_KEY_VAL;                                  // Unlock CS module for register access
		DCORSEL = 0; // Set DCO to 1.5MHz
		CS->CTL1 = (CS->CTL1 & ~mask_selm_divm) | _IMM(CS_CTL1_SELM_E::DCOCLK);
															   // Select MCLK as DCO source
		CS->KEY = 0;
		// Set Flash Bank read buffering
		FLCTL->BANK0_RDCTL = FLCTL->BANK0_RDCTL & ~(FLCTL_BANK0_RDCTL_BUFD | FLCTL_BANK0_RDCTL_BUFI);
		FLCTL->BANK1_RDCTL = FLCTL->BANK1_RDCTL & ~(FLCTL_BANK1_RDCTL_BUFD | FLCTL_BANK1_RDCTL_BUFI);
		break;
	case 3000000:
		// Default VCORE is LDO VCORE0 so no change necessary
		if (__REGULATOR) {// Switches LDO VCORE0 to DCDC VCORE0 if requested
			while (PCM->CTL1 & PCM_CTL1_PMR_BUSY);
			PCM->CTL0 = PCM_CTL0_KEY_VAL | 4 _Comment(PCM_CTL0_AMR_4);
			while (PCM->CTL1 & PCM_CTL1_PMR_BUSY);
		}
		// No flash wait states necessary
		// DCO = 3 MHz; MCLK = source
		CS->KEY = CS_KEY_VAL;                                  // Unlock CS module for register access
		DCORSEL = 1; // Set DCO to 1.5MHz
		CS->CTL1 = (CS->CTL1 & ~mask_selm_divm) | _IMM(CS_CTL1_SELM_E::DCOCLK);
															   // Select MCLK as DCO source
		CS->KEY = 0;
		// Set Flash Bank read buffering
		FLCTL->BANK0_RDCTL = FLCTL->BANK0_RDCTL & ~(FLCTL_BANK0_RDCTL_BUFD | FLCTL_BANK0_RDCTL_BUFI);
		FLCTL->BANK1_RDCTL = FLCTL->BANK1_RDCTL & ~(FLCTL_BANK1_RDCTL_BUFD | FLCTL_BANK1_RDCTL_BUFI);
		break;
	case 12000000:
		// Default VCORE is LDO VCORE0 so no change necessary
		if (__REGULATOR) {// Switches LDO VCORE0 to DCDC VCORE0 if requested
			while ((PCM->CTL1 & PCM_CTL1_PMR_BUSY));
			PCM->CTL0 = PCM_CTL0_KEY_VAL | 4 _Comment(PCM_CTL0_AMR_4);
			while ((PCM->CTL1 & PCM_CTL1_PMR_BUSY));
		}
		// No flash wait states necessary
		// DCO = 12 MHz; MCLK = source
		CS->KEY = CS_KEY_VAL;                                  // Unlock CS module for register access
		DCORSEL = 3;                          // Set DCO to 12MHz
		CS->CTL1 = (CS->CTL1 & ~mask_selm_divm) | _IMM(CS_CTL1_SELM_E::DCOCLK);
															   // Select MCLK as DCO source
		CS->KEY = 0;
		// Set Flash Bank read buffering
		FLCTL->BANK0_RDCTL = FLCTL->BANK0_RDCTL & ~(FLCTL_BANK0_RDCTL_BUFD | FLCTL_BANK0_RDCTL_BUFI);
		FLCTL->BANK1_RDCTL = FLCTL->BANK1_RDCTL & ~(FLCTL_BANK1_RDCTL_BUFD | FLCTL_BANK1_RDCTL_BUFI);
		break;
	case 24000000:
		// Default VCORE is LDO VCORE0 so no change necessary
		if (__REGULATOR) {// Switches LDO VCORE0 to DCDC VCORE0 if requested
			while ((PCM->CTL1 & PCM_CTL1_PMR_BUSY));
			PCM->CTL0 = PCM_CTL0_KEY_VAL | 4 _Comment(PCM_CTL0_AMR_4);
			while ((PCM->CTL1 & PCM_CTL1_PMR_BUSY));
		}
		// 1 flash wait state (BANK0 VCORE0 max is 12 MHz)
		FLCTL->BANK0_RDCTL = (FLCTL->BANK0_RDCTL & ~FLCTL_BANK0_RDCTL_WAIT_MASK) | FLCTL_BANK0_RDCTL_WAIT_1;
		FLCTL->BANK1_RDCTL = (FLCTL->BANK1_RDCTL & ~FLCTL_BANK1_RDCTL_WAIT_MASK) | FLCTL_BANK1_RDCTL_WAIT_1;
		// DCO = 24 MHz; MCLK = source
		CS->KEY = CS_KEY_VAL;                                  // Unlock CS module for register access
		DCORSEL = 4; // Set DCO to 24MHz
		CS->CTL1 = (CS->CTL1 & ~mask_selm_divm) | _IMM(CS_CTL1_SELM_E::DCOCLK);
															   // Select MCLK as DCO source
		CS->KEY = 0;
		// Set Flash Bank read buffering
		FLCTL->BANK0_RDCTL = FLCTL->BANK0_RDCTL | (FLCTL_BANK0_RDCTL_BUFD | FLCTL_BANK0_RDCTL_BUFI);
		FLCTL->BANK1_RDCTL = FLCTL->BANK1_RDCTL & ~(FLCTL_BANK1_RDCTL_BUFD | FLCTL_BANK1_RDCTL_BUFI);
		break;
	case 48000000:
		// Switches LDO VCORE0 to LDO VCORE1; mandatory for 48 MHz setting
		while ((PCM->CTL1 & PCM_CTL1_PMR_BUSY));
		PCM->CTL0 = PCM_CTL0_KEY_VAL | 1 _Comment(PCM_CTL0_AMR_1);
		while ((PCM->CTL1 & PCM_CTL1_PMR_BUSY));
		if (__REGULATOR) {// Switches LDO VCORE1 to DCDC VCORE1 if requested
			while ((PCM->CTL1 & PCM_CTL1_PMR_BUSY));
			PCM->CTL0 = PCM_CTL0_KEY_VAL | 5 _Comment(PCM_CTL0_AMR_5);
			while ((PCM->CTL1 & PCM_CTL1_PMR_BUSY));
		}
		// 1 flash wait states (BANK0 VCORE1 max is 16 MHz, BANK1 VCORE1 max is 32 MHz)
		FLCTL->BANK0_RDCTL = (FLCTL->BANK0_RDCTL & ~FLCTL_BANK0_RDCTL_WAIT_MASK) | FLCTL_BANK0_RDCTL_WAIT_1;
		FLCTL->BANK1_RDCTL = (FLCTL->BANK1_RDCTL & ~FLCTL_BANK1_RDCTL_WAIT_MASK) | FLCTL_BANK1_RDCTL_WAIT_1;
		// DCO = 48 MHz; MCLK = source
		CS->KEY = CS_KEY_VAL;                                  // Unlock CS module for register access
		DCORSEL = 5; // Set DCO to 48MHz
		CS->CTL1 = (CS->CTL1 & ~mask_selm_divm) | _IMM(CS_CTL1_SELM_E::DCOCLK);
															   // Select MCLK as DCO source
		CS->KEY = 0;
		// Set Flash Bank read buffering
		FLCTL->BANK0_RDCTL = FLCTL->BANK0_RDCTL | (FLCTL_BANK0_RDCTL_BUFD | FLCTL_BANK0_RDCTL_BUFI);
		FLCTL->BANK1_RDCTL = FLCTL->BANK1_RDCTL | (FLCTL_BANK1_RDCTL_BUFD | FLCTL_BANK1_RDCTL_BUFI);
		break;
	default:
		break;
	}
}


namespace uni {


}

#endif
