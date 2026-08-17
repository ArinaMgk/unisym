
#include "../../../../inc/cpp/Device/GPU"

using namespace uni;

#if defined(_MCU_STM32H7x)
_ESYM_C{
	void DMA2D_IRQHandler(void) { DMA2D.HandleIRQ(); }
}
#endif
