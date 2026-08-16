#include "../../../../inc/cpp/Device/RCC/RCC"
#include "../../../../inc/cpp/Device/_Power.hpp"
#include "../../../../inc/c/driver/RCC/RCC-registers.hpp"

#if defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)
_ESYM_C{
	// AKA HAL_RCC_CCSCallback / HAL_RCC_Callback / HAL_RCC_WAKEUP_Callback
	Handler_t FUNC_RCC_CSS = { 0 };
	Handler_t FUNC_RCC = { 0 };
	Handler_t FUNC_RCC_WAKEUP = { 0 };
}
#endif

#if defined(_MCU_STM32H7x)
// AKA HAL_RCC_NMI_IRQHandler (HSE Clock Security System)
_ESYM_C void NMI_Handler(void) {
	using namespace uni;
	if (RCC[RCCReg::CIFR].bitof(10)) {// HSECSSF
		asserv(FUNC_RCC_CSS)();
		RCC[RCCReg::CICR].setof(10);// HSECSSC
	}
}

// AKA HAL_RCCEx_CRS callbacks
_ESYM_C{
	Handler_t FUNC_CRS_SYNCOK = { 0 };
	Handler_t FUNC_CRS_SYNCWARN = { 0 };
	Handler_t FUNC_CRS_ESYNC = { 0 };
	Handler_t FUNC_CRS_ERROR = { 0 };
}

// AKA HAL_RCCEx_CRS_IRQHandler
_ESYM_C void CRS_IRQHandler(void) {
	using namespace uni;
	Reference isr(0x40004800 + 0x08);// CRS_ISR
	Reference icr(0x40004800 + 0x0C);// CRS_ICR
	Reference cr(0x40004800 + 0x04);// CRS_CR
	if (isr.bitof(0) && cr.bitof(0)) {// SYNCOKF + SYNCOKIE
		icr.setof(0);// SYNCOKC
		asserv(FUNC_CRS_SYNCOK)();
	}
	else if (isr.bitof(1) && cr.bitof(1)) {// SYNCWARNF + SYNCWARNIE
		icr.setof(1);// SYNCWARNC
		asserv(FUNC_CRS_SYNCWARN)();
	}
	else if (isr.bitof(3) && cr.bitof(3)) {// ESYNCF + ESYNCIE
		icr.setof(3);// ESYNCC
		asserv(FUNC_CRS_ESYNC)();
	}
	else if (isr.bitof(2) && cr.bitof(2)) {// ERRF + ERRIE
		icr.setof(2);// ERRC
		asserv(FUNC_CRS_ERROR)();
	}
}

#elif defined(_MPU_STM32MP13)
// AKA HAL_RCC_IRQHandler
_ESYM_C void RCC_IRQHandler(void) {
	using namespace uni;
	using namespace RCCReg;
	uint32 flags = RCC[MP_CIFR] & _IMM(_MP_CIxR::_MASK);
	RCC[MP_CIFR] = flags;// clear flags (write 1)
	if (flags & _IMM1S(16)) {// LSECSSF
		PWR.setDBP(true);// enable backup domain write
		RCC.enLSECSS(false);// AKA HAL_RCCEx_DisableLSECSS
		RCC[BDCR].rstof(_IMM(_BDCR::LSEON));
	}
	asserv(FUNC_RCC)();
}

// AKA HAL_RCC_WAKEUP_IRQHandler
_ESYM_C void RCC_WAKEUP__IRQHandler(void) {
	using namespace uni;
	using namespace RCCReg;
	if (RCC[MP_CIFR].bitof(20)) {// WKUPF
		RCC[MP_CIFR].setof(20);// clear flag (write 1)
		asserv(FUNC_RCC_WAKEUP)();
	}
}
#endif
