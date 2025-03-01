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
#define _DEBUG// for erro()
#include "../../../../inc/cpp/interrupt"
#include "../../../../inc/cpp/Device/EXTI"

using namespace uni;

//{TODO} Software State Fields in the class

// SysTick_Handler in SysTick.cpp

#define i_index(a,b) (*a[b])

#if defined(_MCCA) && _MCCA == 0x8632
#include "interset_mecocoa"
#endif

// EXTI
#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)
static void _HandlerIRQ_EXTIx(byte x);
#endif


extern "C" {
#ifdef _MCU_STM32
	_WEAK void HardFault_Handler() { erro(); }
#endif
#ifdef _MCU_STM32
	Handler_t FUNC_EXTI[16] = { 0 };
#endif
#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)
	void EXTI0_IRQHandler(void) { _HandlerIRQ_EXTIx(0); }
	void EXTI1_IRQHandler(void) { _HandlerIRQ_EXTIx(1); }
	void EXTI2_IRQHandler(void) { _HandlerIRQ_EXTIx(2); }
	void EXTI3_IRQHandler(void) { _HandlerIRQ_EXTIx(3); }
	void EXTI4_IRQHandler(void) { _HandlerIRQ_EXTIx(4); }
#endif
#if defined(_MCU_STM32F1x) || defined(_MCU_STM32H7x) || defined(_MCU_STM32F4x)
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
#elif defined(_MCU_STM32H7x)
namespace uni { EXTI_t EXTI; }
static void _HandlerIRQ_EXTIx(byte x) {
	if (EXTI.Pending(0, 1).bitof(x)) {
		asserv(FUNC_EXTI[x])();
		EXTI.Pending(0, 1) = _IMM1S(x);
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

// What associated to GICv2 IP from ARM
#define GIC_HIGHEST_INTERRUPT_VALUE 1020U
#define GIC_HIGHEST_SGI_PPI_VALUE     31U
#define GIC_HIGHEST_SGI_VALUE         15U

// ID 1023. This value is returned to a processor, in response to an interrupt acknowledge, 
// if there is no pending interrupt with sufficient priority for it to be signaled to the processor.
#define GIC_ACKNOWLEDGE_RESPONSE 1023U


// ---- Generic IRQ Handler (Software IRQs, PPIs & IRQs) ----
#if defined ( __GNUC__ )
#pragma GCC push_options
#pragma GCC target("general-regs-only")
_ESYM_C void __attribute__((interrupt("IRQ")))IRQ_Handler(void)
#elif defined ( __ICCARM__ )
__irq __arm void IRQ_Handler(void)
#endif
{
	Request_t ItId;
	Handler_t handler;
	while (true) {
		// Get highest pending Interrupt Id from GIC driver
		ItId = IRQ_GetActiveIRQ();
		if (ItId <= GIC_HIGHEST_INTERRUPT_VALUE) /* Highest value of GIC Valid Interrupt */
		{
			// Check validity of IRQ
			if (ItId >= MAX_IRQ_n)
				return;// erro rather SystemInit_IRQ_ErrorHandler();
			else
			{
				handler = uni::GIC.getHandler(ItId);
				asserv(handler)(); else return; // erro;
			}
			/* End Acknowledge interrupt */
			IRQ_EndOfInterrupt((Request_t)ItId);
		}
		else {
		  /* Normal case: whenever there is no more pending IRQ , IAR returns ACKNOWLEDGE special IRQ value */
			if (ItId == GIC_ACKNOWLEDGE_RESPONSE)
				break;
			else { // Spurious IRQ Value (1022)  ...
				return; //erro;
			}
		}
	}
}
#ifdef __GNUC__
#pragma GCC pop_options
#endif


#endif
#include "interrupt_timer.hpp"
#include "interrupt_adc.hpp"
#include "interrupt_dma.hpp"
#include "interrupt_xart.hpp"
#include "interrupt_iic.hpp"
#include "interrupt_spi.hpp"
#include "interrupt_sgi.hpp"
#include "interrupt_dram.hpp"
#include "interrupt_video.hpp"

#if defined(_MPU_STM32MP13)

bool EXTILine::isDirect() const {
	static byte Reserved[] = { 20,34,35,41,49,51,54,61,62,63,64,65,66,67,73,74 };
	if (isConfigurable()) return false;
	for0a(i, Reserved) if (_IMM(Reserved[i]) == _IMM(this))
		return false;
	return true;
}

byte& EXTILine::refRegisterNumber() const { return EXTI_REGx[getChannel()]; }

extichan& EXTI_t::operator[](const GeneralPurposeInputOutputPin& pin) const { return treat<extichan>_IMM(pin.getID()); }

void EXTI_t::setConfig(byte line) const {
	if (self[line].isConfigurable());
	// set {EXTI_MODE_NONE EXTI_TRIGGER_NONE}
	_TODO
}

void EXTI_t::setConfig(byte line, EXTIEdge edge, bool rupt_or_event, byte gport_id) const {
	byte chan = self[line].getChannel();
	byte regi = self[line].getRegroup();
	EXTI_REGx[chan] = regi;
	if (self[line].isConfigurable()) {
		TriggerRising(regi).setof(chan, (edge == EXTIEdge::Posedge || edge == EXTIEdge::Anyedge));
		TriggerFalling(regi).setof(chan, (edge == EXTIEdge::Negedge || edge == EXTIEdge::Anyedge));
		if (self[line].isGPIO()) {
			uint32* exti_ctrl = (uint32*)&self[EXTIReg::EXTICR];
			exti_ctrl += (chan & 0xF) >> 2;// 4 pins for each register
			Reference(exti_ctrl).maset((chan & 0b11) * 4, 4, gport_id);
		}
	}
	MaskRegister(regi, true).setof(chan, rupt_or_event);// Interrupt
	MaskRegister(regi, false).setof(chan, !rupt_or_event);// Event
}

#endif
