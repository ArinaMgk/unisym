
#include "../../../../inc/cpp/Device/BDMA"

using namespace uni;

#if defined(_MCU_STM32H7x)
_ESYM_C{
	void BDMA_Channel0_IRQHandler(void) { BDMA.HandleIRQ(0); }
	void BDMA_Channel1_IRQHandler(void) { BDMA.HandleIRQ(1); }
	void BDMA_Channel2_IRQHandler(void) { BDMA.HandleIRQ(2); }
	void BDMA_Channel3_IRQHandler(void) { BDMA.HandleIRQ(3); }
	void BDMA_Channel4_IRQHandler(void) { BDMA.HandleIRQ(4); }
	void BDMA_Channel5_IRQHandler(void) { BDMA.HandleIRQ(5); }
	void BDMA_Channel6_IRQHandler(void) { BDMA.HandleIRQ(6); }
	void BDMA_Channel7_IRQHandler(void) { BDMA.HandleIRQ(7); }
}
#endif
