
#include "../../../../inc/cpp/Device/Lock/HSEM.hpp"

#if defined(_MCU_STM32H7x)
_ESYM_C{
	// AKA HAL: HSEM1_IRQHandler - single global HSEM IRQ (IRQ_HSEM1 = 125).
	Handler_t FUNC_HSEM[1] = { 0 };

	void HSEM1_IRQHandler(void) {
		// AKA HAL_HSEM_IRQHandler; friend of uni::HSEM
		uni::HSEM& hsem = uni::HSEM1;
		stduint statusreg = hsem[uni::HSEMReg::MISR];
		// disable the notified semaphores' interrupts
		hsem[uni::HSEMReg::IER] &= ~statusreg;
		// clear flags
		hsem[uni::HSEMReg::ICR] |= statusreg;
		// fire free callback with the mask of released semaphores
		if (hsem.FreeHandler) hsem.FreeHandler((pureptr_t)statusreg);
		if (FUNC_HSEM[0]) FUNC_HSEM[0]();// user callback registered by setInterrupt()
	}
}
#endif
