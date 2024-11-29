// ASCII CPP TAB4 CRLF
// Docutitle: Interrupt List Abstract
// Codifiers: @dosconio: 20240528
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
#include "../../../../inc/cpp/interrupt"
#include "../../../../inc/cpp/Device/EXTI"

using namespace uni;

//{TODO} Software State Fields in the class

// SysTick_Handler in SysTick.cpp

#define i_index(a,b) (*a[b])

// EXTI
#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x) || defined(_MPU_STM32MP13)
static void _HandlerIRQ_EXTIx(byte x);
#endif


extern "C" {
#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x) || defined(_MPU_STM32MP13)
	Handler_t FUNC_EXTI[16] = { 0 };

	void EXTI0_IRQHandler(void) { _HandlerIRQ_EXTIx(0); }
	void EXTI1_IRQHandler(void) { _HandlerIRQ_EXTIx(1); }
	void EXTI2_IRQHandler(void) { _HandlerIRQ_EXTIx(2); }
	void EXTI3_IRQHandler(void) { _HandlerIRQ_EXTIx(3); }
	void EXTI4_IRQHandler(void) { _HandlerIRQ_EXTIx(4); }
#endif
#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x)
	void EXTI9_5_IRQHandler(void) {
		for (byte i = 5; i < 9; i++) _HandlerIRQ_EXTIx(i);
	}
	void EXTI15_10_IRQHandler(void) {
		for (byte i = 10; i < 16; i++) _HandlerIRQ_EXTIx(i);
	}
#elif defined(_MPU_STM32MP13)
	// relationship: lib\cpp\Device\Interrupt\interset_stm32mp13
	// relationship: inc\cpp\Device\Interrupt\interrupt_stm32mp13.h
	// [from]
	// - mp135d mp135f
	
	//: access by Bitmap
	byte EXTI_Lastlev[16 / bitsof(byte)]{ 0 };// last trigger level
	// byte EXTI_Posedge[16 / bitsof(byte)]{ 0 };
	// byte EXTI_Negedge[16 / bitsof(byte)]{ 0 };
	byte EXTI_REGx[16];// 1, 2, ...

	void EXTI5_IRQHandler(void) { _HandlerIRQ_EXTIx(5); }
	void EXTI6_IRQHandler(void) { _HandlerIRQ_EXTIx(6); }
	void EXTI7_IRQHandler(void) { _HandlerIRQ_EXTIx(7); }
	void EXTI8_IRQHandler(void) { _HandlerIRQ_EXTIx(8); }
	void EXTI9_IRQHandler(void) { _HandlerIRQ_EXTIx(9); }
	void EXTI10_IRQHandler(void) { _HandlerIRQ_EXTIx(10); }
	void EXTI11_IRQHandler(void) { _HandlerIRQ_EXTIx(11); }
	void EXTI12_IRQHandler(void) { _HandlerIRQ_EXTIx(12); }
	void EXTI13_IRQHandler(void) { _HandlerIRQ_EXTIx(13); }
	void EXTI14_IRQHandler(void) { _HandlerIRQ_EXTIx(14); }
	void EXTI15_IRQHandler(void) { _HandlerIRQ_EXTIx(15); }
	
	_TEMP symbol_t SecurePhysicalTimer_hand;
	
	void RESERVED_IRQHandler(void) {}
	//
	void HypervisorTimer_IRQHandler(void) {}
	void VirtualTimer_IRQHandler(void) {}
	void SecurePhysicalTimer_IRQHandler(void) { SecurePhysicalTimer_hand(); }// TICK ?
	void NonSecurePhysicalTimer_IRQHandler(void) {}
	//
	void RTC_TIMESTAMP_IRQHandler(void) {}
	void RTC_WKUP_ALARM_IRQHandler(void) {}
	void RTC_WKUP_ALARM_S_IRQHandler(void) {}
	void RTC_TS_S_IRQHandler(void) {}
	//
	void ETH1_LPI_IRQHandler(void) {}
	void ETH1_IRQHandler(void) {}
	void ETH1_WKUP_IRQHandler(void) {}
	void ETH2_LPI_IRQHandler(void) {}
	void ETH2_IRQHandler(void) {}
	void ETH2_WKUP_IRQHandler(void) {}
	//
	void FDCAN1_IT0_IRQHandler(void) {}
	void FDCAN2_IT0_IRQHandler(void) {}
	void FDCAN1_IT1_IRQHandler(void) {}
	void FDCAN2_IT1_IRQHandler(void) {}
	void FDCAN_CAL_IRQHandler(void) {}
	//
	void VirtualMaintenanceInterrupt_IRQHandler(void) {}
	void Legacy_nFIQ_IRQHandler(void) {}
	void Legacy_nIRQ_IRQHandler(void) {}
	void PVD_AVD_IRQHandler(void) {}
	void TAMP_IRQHandler(void) {}
	void TZC_IT_IRQHandler(void) {}
	void RCC_IRQHandler(void) {}
	void FMC_IRQHandler(void) {}
	void SDMMC1_IRQHandler(void) {}
	void USBH_PORT1_IRQHandler(void) {}
	void USBH_PORT2_IRQHandler(void) {}
	void DCMIPP_IRQHandler(void) {}
	void CRYP1_IRQHandler(void) {} // Ex{135F} !{135D}
	void HASH1_IRQHandler(void) {}
	void SAES_IRQHandler(void) {}
	void SAI1_IRQHandler(void) {}
	void SAI2_IRQHandler(void) {}
	void QUADSPI_IRQHandler(void) {}
	void SPDIF_RX_IRQHandler(void) {}
	void OTG_IRQHandler(void) {}
	void SDMMC2_IRQHandler(void) {}
	void RNG1_IRQHandler(void) {}
	void RCC_WAKEUP__IRQHandler(void) {}
	void DTS_IRQHandler(void) {}
	void MPU_WAKEUP_PIN_IRQHandler(void) {}
	void IWDG1_IRQHandler(void) {}
	void IWDG2_IRQHandler(void) {}
	void TAMP_SIRQHandler(void) {}
	void PMUIRQ0_IRQHandler(void) {}
	void COMMRX0_IRQHandler(void) {}
	void COMMTX0_IRQHandler(void) {}
	void AXIERRIRQ_IRQHandler(void) {}
	void nCTIIRQ0_IRQHandler(void) {}
	void DFSDM1_IRQHandler(void) {}
	void DFSDM2_IRQHandler(void) {}
	void PKA_IRQHandler(void) {}
	void MCE_IRQHandler(void) {}


	#include "interset_stm32mp13"
	
#endif

}

// AKA HAL_EXTI_IRQHandler
#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x)
static void _HandlerIRQ_EXTIx(byte x) {
	if (EXTI::Pending.bitof(x)) {
		asserv (FUNC_EXTI[x])();
		EXTI::Pending = _IMM1S(x);
	}
}
#elif defined(_MPU_STM32MP13)
#include "../../../../inc/c/bitmap.h"
static void _HandlerIRQ_EXTIx(byte x) {
	if (!EXTI_REGx[x]) return;
	Bitmap llevel(EXTI_Lastlev, sizeof(EXTI_Lastlev));
	if (EXTI.PendingRising(EXTI_REGx[x]).bitof(x)) {
		llevel.setof(x, true);
		asserv(FUNC_EXTI[x])();
		EXTI.PendingRising(EXTI_REGx[x]) = _IMM1S(x);
	}
	if (EXTI.PendingFalling(EXTI_REGx[x]).bitof(x)) {
		llevel.setof(x, false);
		asserv(FUNC_EXTI[x])();
		EXTI.PendingFalling(EXTI_REGx[x]) = _IMM1S(x);
	}
}
namespace uni { EXTI_t EXTI; }
#endif
#include "Interrupt_timer.hpp"
#include "Interrupt_adc.hpp"
#include "Interrupt_dma.hpp"
#include "Interrupt_xart.hpp"
#include "Interrupt_iic.hpp"
#include "Interrupt_spi.hpp"
#include "Interrupt_sgi.hpp"
#include "Interrupt_dram.hpp"
#include "Interrupt_video.hpp"

#if defined(_MPU_STM32MP13)
byte& EXTILine::refRegisterNumber() const { return EXTI_REGx[getChannel()]; }
extichan& EXTI_t::operator[](const GeneralPurposeInputOutputPin& pin) const { return treat<extichan>_IMM(pin.getID()); }
#endif
