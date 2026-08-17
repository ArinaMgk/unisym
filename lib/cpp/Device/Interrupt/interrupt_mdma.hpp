
#include "../../../../inc/cpp/Device/MDMA"

using namespace uni;

#if defined(_MCU_STM32H7x)
_ESYM_C{
	// AKA HAL: MDMA_IRQHandler — single global MDMA IRQ shared by 16 channels.
	// Each channel's HandleIRQ() checks its own GISR0 bit and returns early if not pending.
	void MDMA_IRQHandler(void) {
		for (byte ch = 0; ch < 16; ch++)
			MDMA[ch].HandleIRQ();
	}
}
#endif
