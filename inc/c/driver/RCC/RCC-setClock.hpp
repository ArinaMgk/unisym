// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Partial) RCC setClock 
// Codifiers: @dosconio: 20241116
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

// inner included file ----

#if defined(_MCU_STM32F1x)
bool RCC_t::setClock(SysclkSource::RCCSysclockSource source) {
	using namespace RCCReg;
	//{TEMP} HSE only
	if (!(RCC.HSE.setMode() && RCC.PLL.setMode()))// PLL<<HSE*9/1
		while (true);
	if (RCC.setFlash() && RCC.AHB.setMode() && RCC.Sysclock.setMode() &&
		RCC.setFlash(true) && RCC.APB1.setMode(1) && RCC.APB2.setMode(0));
	else while (true);
	RCC.Sysclock.getCoreFrequency();
	SysTick::enClock(1000);// default 1000Hz
	return true;
}

#elif defined(_MCU_STM32F4x)

// naming from system_*chip*.c
bool RCC_t::setClock(SysclkSource::RCCSysclockSource source) {
	using namespace RCCReg;
	if (source == SysclkSource::HSE /*{TODO} || source == SysclkSource::HSI*/) {
		//{}bool use_HSE = true;
		//{TODO} RCC->CFGR = 0x00000000;// reset
		RCC[APB1ENR].setof(_RCC_APB1ENR_POSI_ENCLK_PWR);//aka __HAL_RCC_PWR_CLK_ENABLE
		//{} __HAL_PWR_VOLTAGESCALING_CONFIG // The voltage scaling allows optimizing the power consumption when the device is clocked below the maximum system frequency
		if (RCC.HSE.setMode() && RCC.PLL.setMode()); else
			return false;
		if (RCC.setFlash() && RCC.AHB.setMode() && RCC.Sysclock.setMode() && RCC.setFlash(true) && RCC.APB1.setMode(2) && RCC.APB2.setMode(1)); else
			return false;

		RCC.Sysclock.getCoreFrequency();
		SysTick::enClock(1000);// default 1000Hz

		Reference DBGMCU_IDCODE(0xE0042000UL /*DBGMCU_BASE+0x00*/);
		if ((DBGMCU_IDCODE >> 16) == 0x1001)// HAL_GetREVID() ぞ ((DBGMCU->IDCODE) >> 16U) よ if (HAL_GetREVID() == 0x1001) // STM32F405x/407x/415x/417x Revision Z devices: prefetch is supported
			Flash[FlashReg::ACR].setof(_Flash_ACR_POSI_PRFTEN);//aka __HAL_FLASH_PREFETCH_BUFFER_ENABLE(); ぞ (FLASH->ACR |= FLASH_ACR_PRFTEN) // Enable the Flash prefetch
	}
	else return false;
	return true;
}

#endif
