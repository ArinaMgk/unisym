
#include "../../../../inc/cpp/Device/Graphic/GPE-JPEG.hpp"

#if defined(_MCU_STM32H7x)
_ESYM_C{
	// AKA HAL: JPEG_IRQHandler - single global JPEG IRQ (IRQ_JPEG = 121).
	Handler_t FUNC_JPEG[1] = { 0 };

	void JPEG_IRQHandler(void) {
		// AKA HAL_JPEG_IRQHandler: dispatch IT/DMA pump; friend of JPEG_HARD
		uni::JPEG_HARD& jpeg = uni::JPEG;
		switch (jpeg.State) {
		case uni::JPEGState::BusyEncoding:
		case uni::JPEGState::BusyDecoding:
			if ((jpeg.Context & _JPEG_CONTEXT_METHOD_MASK) == _JPEG_CONTEXT_IT)
				jpeg.ProcessPump();
			else if ((jpeg.Context & _JPEG_CONTEXT_METHOD_MASK) == _JPEG_CONTEXT_DMA)
				uni::JPEG_HARD::DMAContinueProcess(jpeg);
			break;
		default:
			break;
		}
		if (FUNC_JPEG[0]) FUNC_JPEG[0]();// user callback registered by setInterrupt()
	}
}
#endif
