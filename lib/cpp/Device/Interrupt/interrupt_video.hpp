

#include "../../../../inc/c/stdinc.h"

_ESYM_C{
	#if defined(_MPU_STM32MP13)
	void LTDC_IRQHandler(void) {}
	void LTDC_ER_IRQHandler(void) {}
	void LTDC_SEC_IRQHandler(void) {}
	void LTDC_SEC_ER_IRQHandler(void) {}
	#endif
}

