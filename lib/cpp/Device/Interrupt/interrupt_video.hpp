

#include "../../../../inc/c/stdinc.h"
#include "../../../../inc/cpp/Device/LTDC"

static void _HandlerIRQ_LTDC(void);

_ESYM_C{
#if defined(_MCU_STM32H7x)
	void LTDC_IRQHandler(void) { _HandlerIRQ_LTDC(); }
	void LTDC_ER_IRQHandler(void) { _HandlerIRQ_LTDC(); }
#elif defined(_MPU_STM32MP13)
	void LTDC_IRQHandler(void) {}
	void LTDC_ER_IRQHandler(void) {}
	void LTDC_SEC_IRQHandler(void) {}
	void LTDC_SEC_ER_IRQHandler(void) {}
#endif
}

#if defined(_MCU_STM32H7x)
// AKA HAL_LTDC_IRQHandler
static void _HandlerIRQ_LTDC(void) {
	// Transfer Error (TERRIF)
	if (LTDC[LTDCReg::ISR].bitof(2) && LTDC[LTDCReg::IER].bitof(2)) {
		LTDC[LTDCReg::IER].rstof(2);
		LTDC[LTDCReg::ICR] = _IMM1S(2);
		callif(LTDC.ErrorCallback);
	}
	// FIFO Underrun (FUIF)
	if (LTDC[LTDCReg::ISR].bitof(1) && LTDC[LTDCReg::IER].bitof(1)) {
		LTDC[LTDCReg::IER].rstof(1);
		LTDC[LTDCReg::ICR] = _IMM1S(1);
		callif(LTDC.ErrorCallback);
	}
	// Line Interrupt (LIF)
	if (LTDC[LTDCReg::ISR].bitof(0) && LTDC[LTDCReg::IER].bitof(0)) {
		LTDC[LTDCReg::IER].rstof(0);
		LTDC[LTDCReg::ICR] = _IMM1S(0);
		callif(LTDC.LineEventCallback);
	}
	// Register Reload (RRIF)
	if (LTDC[LTDCReg::ISR].bitof(3) && LTDC[LTDCReg::IER].bitof(3)) {
		LTDC[LTDCReg::IER].rstof(3);
		LTDC[LTDCReg::ICR] = _IMM1S(3);
		callif(LTDC.ReloadEventCallback);
	}
}
#endif

