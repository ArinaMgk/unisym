
#include "../../../../inc/cpp/Device/HASH"

#if defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x)
_ESYM_C{
	// AKA HAL: HASH_IRQHandler - HASH shares the RNG IRQ line (IRQ_RNG = 80).
	Handler_t FUNC_HASH[1] = { 0 };

	void HASH_IRQHandler(void) {
		// AKA HAL_HASH_IRQHandler -> HASH_IT; friend of uni::HASH
		uni::HASH1.ProcessIT();
		if (FUNC_HASH[0]) FUNC_HASH[0]();// user callback registered by setInterrupt()
	}
}
#endif
