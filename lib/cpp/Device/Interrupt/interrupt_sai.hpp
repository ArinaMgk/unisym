
#include "../../../../inc/cpp/Device/SAI"

using namespace uni;

#if defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)

// AKA HAL_SAI_IRQHandler (SAIx IRQ line is shared by block A and block B)
static void _HandlerIRQ_SAI(SAI_t& sai) {
	sai[1].irqHandler();
	sai[2].irqHandler();
}

_ESYM_C{
	void SAI1_IRQHandler(void) { _HandlerIRQ_SAI(SAI1); }
	void SAI2_IRQHandler(void) { _HandlerIRQ_SAI(SAI2); }
}

#endif
