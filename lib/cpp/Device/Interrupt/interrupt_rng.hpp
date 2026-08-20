
#include "../../../../inc/c/random.h"

#if defined(_MCU_STM32H7x)
_ESYM_C{
	// AKA HAL: RNG_IRQHandler - single global RNG IRQ (IRQ_RNG = 80).
	Handler_t FUNC_RNG[1] = { 0 };

	void RNG_IRQHandler(void) {
		// AKA HAL_RNG_IRQHandler; friend of uni::RNG
		uni::RNG& rng = uni::RNG1;
		// clock error or seed error
		if (rng[uni::RNGReg::SR].bitof(_RNG_SR_POS_CEIS) || rng[uni::RNGReg::SR].bitof(_RNG_SR_POS_SECS)) {
			rng.State = uni::RNGState::Error;
			if (rng.ErrorHandler) rng.ErrorHandler();
			rng[uni::RNGReg::SR].rstof(_RNG_SR_POS_CEIS);
			rng[uni::RNGReg::SR].rstof(_RNG_SR_POS_SECS);
		}
		// data ready
		if (rng[uni::RNGReg::SR].bitof(_RNG_SR_POS_DRDY)) {
			rng[uni::RNGReg::CR].rstof(_RNG_CR_POS_IE);// disable IT
			rng.RandomNumber = rng[uni::RNGReg::DR];
			if (rng.State != uni::RNGState::Error) {
				rng.State = uni::RNGState::Ready;
				if (rng.ReadyDataHandler) rng.ReadyDataHandler();
			}
		}
		if (FUNC_RNG[0]) FUNC_RNG[0]();// user callback registered by setInterrupt()
	}
}
#endif
