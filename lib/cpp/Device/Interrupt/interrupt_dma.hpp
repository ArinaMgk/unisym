#include "../../../../inc/cpp/Device/DMA"

#if defined(_MCU_STM32F1x)
static void _HandlerIRQ_DMAChannelx(byte dma_id, byte chanx);

#endif
_ESYM_C{
#if defined(_MCU_STM32F1x)
	void DMA1_Channel1_IRQHandler(void) { _HandlerIRQ_DMAChannelx(1, 1); }
	void DMA1_Channel2_IRQHandler(void) { _HandlerIRQ_DMAChannelx(1, 2); }
	void DMA1_Channel3_IRQHandler(void) { _HandlerIRQ_DMAChannelx(1, 3); }
	void DMA1_Channel4_IRQHandler(void) { _HandlerIRQ_DMAChannelx(1, 4); }
	void DMA1_Channel5_IRQHandler(void) { _HandlerIRQ_DMAChannelx(1, 5); }
	void DMA1_Channel6_IRQHandler(void) { _HandlerIRQ_DMAChannelx(1, 6); }
	void DMA1_Channel7_IRQHandler(void) { _HandlerIRQ_DMAChannelx(1, 7); }
	void DMA2_Channel1_IRQHandler(void) { _HandlerIRQ_DMAChannelx(2, 1); }
	void DMA2_Channel2_IRQHandler(void) { _HandlerIRQ_DMAChannelx(2, 2); }
	void DMA2_Channel3_IRQHandler(void) { _HandlerIRQ_DMAChannelx(2, 3); }
	void DMA2_Channel4_5_IRQHandler(void) {
		//{} ... judge if 4 or 5 input in.
	}
#elif defined(_MPU_STM32MP13)
	//{TODO}
	void DMA1_Stream0_IRQHandler(void) {}
	void DMA1_Stream1_IRQHandler(void) {}
	void DMA1_Stream2_IRQHandler(void) {}
	void DMA1_Stream3_IRQHandler(void) {}
	void DMA1_Stream4_IRQHandler(void) {}
	void DMA1_Stream5_IRQHandler(void) {}
	void DMA1_Stream6_IRQHandler(void) {}
	void DMA1_Stream7_IRQHandler(void) {}
	void DMA2_Stream0_IRQHandler(void) {}
	void DMA2_Stream1_IRQHandler(void) {}
	void DMA2_Stream2_IRQHandler(void) {}
	void DMA2_Stream3_IRQHandler(void) {}
	void DMA2_Stream4_IRQHandler(void) {}
	void DMA2_Stream5_IRQHandler(void) {}
	void DMA2_Stream6_IRQHandler(void) {}
	void DMA2_Stream7_IRQHandler(void) {}
	void DMA3_Stream0_IRQHandler(void) {}
	void DMA3_Stream1_IRQHandler(void) {}
	void DMA3_Stream2_IRQHandler(void) {}
	void DMA3_Stream3_IRQHandler(void) {}
	void DMA3_Stream4_IRQHandler(void) {}
	void DMA3_Stream5_IRQHandler(void) {}
	void DMA3_Stream6_IRQHandler(void) {}
	void DMA3_Stream7_IRQHandler(void) {}
	//
	void DMAMUX1_OVR_IRQHandler(void) {}
	void DMAMUX2_OVR_IRQHandler(void) {}
	void MDMA_IRQHandler(void) {}
	void MDMA_SEC_IT_IRQHandler(void) {}
#endif	
}




#if defined(_MCU_STM32F1x)

static void _HandlerIRQ_DMAChannelx(byte dma_id, byte chanx) {
	DMA_t& crt = DMA[dma_id];
	uint32 flag = crt[DMAReg::ISR];// each 4b: M-L[TEIFx HTIFx TCIFx GIFx], same with IFCR
	uint32 sors = crt[DMAReg::CCRx[chanx]];
	if ((flag & (0x2 << (chanx << 2))) &&
		BitGet(sors, _DMA_CCRx_POS_TCIE)) // Transfer Complete Interrupt
	{
		if (!BitGet(sors, _DMA_CCRx_POS_CIRC))
			crt.enInterrupt(false, 1, chanx);
		asserv(crt.XferCpltCallback)();
		crt[DMAReg::IFCR] = (1U << 1) << (chanx << 2);
	}
	else if ((flag & (0x4 << (chanx << 2))) &&
		BitGet(sors, _DMA_CCRx_POS_HTIE)) // Half Transfer Complete Interrupt
	{
		if (!BitGet(sors, _DMA_CCRx_POS_CIRC))
			crt.enInterrupt(false, 2, chanx);
		asserv(crt.XferHalfCallback)();
		crt[DMAReg::IFCR] = (1U << 2) << (chanx << 2);
	}
	else if ((flag & (0x8 << (chanx << 2))) &&
		BitGet(sors, _DMA_CCRx_POS_TEIE)) // Transfer Error Interrupt
	{
		crt.enInterrupt(false, 0, chanx);
		asserv(crt.XferErrorCallback)();
		crt[DMAReg::IFCR] = (1U << 0 /*not 3*/) << (chanx << 2);// Clear all flags
	}
}

#endif




