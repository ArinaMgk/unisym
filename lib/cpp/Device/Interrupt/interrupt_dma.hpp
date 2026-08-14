#include "../../../../inc/cpp/Device/DMA"

#if defined(_MCU_STM32F1x)
static void _HandlerIRQ_DMAChannelx(byte dma_id, byte chanx);

#endif
#if defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)
static void _HandlerIRQ_DMAStreamx(byte dma_id, byte stream);
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
#elif defined(_MCU_STM32F4x)
	void DMA1_Stream0_IRQHandler(void) { _HandlerIRQ_DMAStreamx(1, 0); }
	void DMA1_Stream1_IRQHandler(void) { _HandlerIRQ_DMAStreamx(1, 1); }
	void DMA1_Stream2_IRQHandler(void) { _HandlerIRQ_DMAStreamx(1, 2); }
	void DMA1_Stream3_IRQHandler(void) { _HandlerIRQ_DMAStreamx(1, 3); }
	void DMA1_Stream4_IRQHandler(void) { _HandlerIRQ_DMAStreamx(1, 4); }
	void DMA1_Stream5_IRQHandler(void) { _HandlerIRQ_DMAStreamx(1, 5); }
	void DMA1_Stream6_IRQHandler(void) { _HandlerIRQ_DMAStreamx(1, 6); }
	void DMA1_Stream7_IRQHandler(void) { _HandlerIRQ_DMAStreamx(1, 7); }
	void DMA2_Stream0_IRQHandler(void) { _HandlerIRQ_DMAStreamx(2, 0); }
	void DMA2_Stream1_IRQHandler(void) { _HandlerIRQ_DMAStreamx(2, 1); }
	void DMA2_Stream2_IRQHandler(void) { _HandlerIRQ_DMAStreamx(2, 2); }
	void DMA2_Stream3_IRQHandler(void) { _HandlerIRQ_DMAStreamx(2, 3); }
	void DMA2_Stream4_IRQHandler(void) { _HandlerIRQ_DMAStreamx(2, 4); }
	void DMA2_Stream5_IRQHandler(void) { _HandlerIRQ_DMAStreamx(2, 5); }
	void DMA2_Stream6_IRQHandler(void) { _HandlerIRQ_DMAStreamx(2, 6); }
	void DMA2_Stream7_IRQHandler(void) { _HandlerIRQ_DMAStreamx(2, 7); }
#elif defined(_MCU_STM32H7x)
	void DMA1_Stream0_IRQHandler(void) { _HandlerIRQ_DMAStreamx(1, 0); }
	void DMA1_Stream1_IRQHandler(void) { _HandlerIRQ_DMAStreamx(1, 1); }
	void DMA1_Stream2_IRQHandler(void) { _HandlerIRQ_DMAStreamx(1, 2); }
	void DMA1_Stream3_IRQHandler(void) { _HandlerIRQ_DMAStreamx(1, 3); }
	void DMA1_Stream4_IRQHandler(void) { _HandlerIRQ_DMAStreamx(1, 4); }
	void DMA1_Stream5_IRQHandler(void) { _HandlerIRQ_DMAStreamx(1, 5); }
	void DMA1_Stream6_IRQHandler(void) { _HandlerIRQ_DMAStreamx(1, 6); }
	void DMA1_Stream7_IRQHandler(void) { _HandlerIRQ_DMAStreamx(1, 7); }
	void DMA2_Stream0_IRQHandler(void) { _HandlerIRQ_DMAStreamx(2, 0); }
	void DMA2_Stream1_IRQHandler(void) { _HandlerIRQ_DMAStreamx(2, 1); }
	void DMA2_Stream2_IRQHandler(void) { _HandlerIRQ_DMAStreamx(2, 2); }
	void DMA2_Stream3_IRQHandler(void) { _HandlerIRQ_DMAStreamx(2, 3); }
	void DMA2_Stream4_IRQHandler(void) { _HandlerIRQ_DMAStreamx(2, 4); }
	void DMA2_Stream5_IRQHandler(void) { _HandlerIRQ_DMAStreamx(2, 5); }
	void DMA2_Stream6_IRQHandler(void) { _HandlerIRQ_DMAStreamx(2, 6); }
	void DMA2_Stream7_IRQHandler(void) { _HandlerIRQ_DMAStreamx(2, 7); }
	void DMAMUX1_OVR_IRQHandler(void) {
		// Iterate over all DMA streams (DMA1 + DMA2) and check DMAMUX overrun flags
		for (byte dma_id = 1; dma_id <= 2; dma_id++) {
			for (byte st = 0; st < 8; st++)
				DMA[dma_id][st].HandleMuxIRQ();
		}
	}
#elif defined(_MPU_STM32MP13)
	void DMA1_Stream0_IRQHandler(void) { _HandlerIRQ_DMAStreamx(1, 0); }
	void DMA1_Stream1_IRQHandler(void) { _HandlerIRQ_DMAStreamx(1, 1); }
	void DMA1_Stream2_IRQHandler(void) { _HandlerIRQ_DMAStreamx(1, 2); }
	void DMA1_Stream3_IRQHandler(void) { _HandlerIRQ_DMAStreamx(1, 3); }
	void DMA1_Stream4_IRQHandler(void) { _HandlerIRQ_DMAStreamx(1, 4); }
	void DMA1_Stream5_IRQHandler(void) { _HandlerIRQ_DMAStreamx(1, 5); }
	void DMA1_Stream6_IRQHandler(void) { _HandlerIRQ_DMAStreamx(1, 6); }
	void DMA1_Stream7_IRQHandler(void) { _HandlerIRQ_DMAStreamx(1, 7); }
	void DMA2_Stream0_IRQHandler(void) { _HandlerIRQ_DMAStreamx(2, 0); }
	void DMA2_Stream1_IRQHandler(void) { _HandlerIRQ_DMAStreamx(2, 1); }
	void DMA2_Stream2_IRQHandler(void) { _HandlerIRQ_DMAStreamx(2, 2); }
	void DMA2_Stream3_IRQHandler(void) { _HandlerIRQ_DMAStreamx(2, 3); }
	void DMA2_Stream4_IRQHandler(void) { _HandlerIRQ_DMAStreamx(2, 4); }
	void DMA2_Stream5_IRQHandler(void) { _HandlerIRQ_DMAStreamx(2, 5); }
	void DMA2_Stream6_IRQHandler(void) { _HandlerIRQ_DMAStreamx(2, 6); }
	void DMA2_Stream7_IRQHandler(void) { _HandlerIRQ_DMAStreamx(2, 7); }
	void DMA3_Stream0_IRQHandler(void) { _HandlerIRQ_DMAStreamx(3, 0); }
	void DMA3_Stream1_IRQHandler(void) { _HandlerIRQ_DMAStreamx(3, 1); }
	void DMA3_Stream2_IRQHandler(void) { _HandlerIRQ_DMAStreamx(3, 2); }
	void DMA3_Stream3_IRQHandler(void) { _HandlerIRQ_DMAStreamx(3, 3); }
	void DMA3_Stream4_IRQHandler(void) { _HandlerIRQ_DMAStreamx(3, 4); }
	void DMA3_Stream5_IRQHandler(void) { _HandlerIRQ_DMAStreamx(3, 5); }
	void DMA3_Stream6_IRQHandler(void) { _HandlerIRQ_DMAStreamx(3, 6); }
	void DMA3_Stream7_IRQHandler(void) { _HandlerIRQ_DMAStreamx(3, 7); }
	void DMAMUX1_OVR_IRQHandler(void) {
		for (byte dma_id = 1; dma_id <= 2; dma_id++)
			for (byte st = 0; st < 8; st++)
				DMA[dma_id][st].HandleMuxIRQ();
	}
	void DMAMUX2_OVR_IRQHandler(void) {
		for (byte st = 0; st < 8; st++)
			DMA[3][st].HandleMuxIRQ();
	}
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

#if defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)

// shared stream-style dispatch: LISR/HISR + LIFCR/HIFCR layout is identical on F4, H7 and MP13.
// per-stream 6-bit block [FEIF, rsv, DMEIF, TEIF, HTIF, TCIF] at base offset 0/6/16/22.
// Double buffer: CT bit (SxCR[19]) selects M0 vs M1 callback on TC/HT completion.
static void _HandlerIRQ_DMAStreamx(byte dma_id, byte stream) {
	DMA_t& crt = DMA[dma_id];
	using namespace DMAReg;
	byte posi = 0;
	if (stream & 0b010) posi += 16;
	if (stream & 0b001) posi += 6;
	bool ishigh = stream & 0b100;
	uint32 flag = crt[ishigh ? HISR : LISR];
	uint32 cr = crt[CR[stream]];
	uint32 fcr = crt[FCR[stream]];
	DMARegType ifcr_reg = ishigh ? HIFCR : LIFCR;
	bool is_circ = cr & (1U << _DMA_SxCR_POS_CIRC);
	bool ct = cr & (1U << _DMA_SxCR_POS_CT); // current target: 0=M0, 1=M1 (double buffer)

	// Transfer Error (aka HAL_DMA_IRQHandler TE block): clear + accumulate, callback handled at the end
	if ((flag & (1U << (posi + _DMA_SxFLAG_POS_TEIF))) && (cr & (1U << _DMA_SxCR_POS_TEIE))) {
		crt[CR[stream]].setof(_DMA_SxCR_POS_TEIE, false);
		crt[ifcr_reg] = (1U << (posi + _DMA_SxFLAG_POS_TEIF));
		crt.streamErrors[stream] |= _DMA_ERROR_TE;
	}
	// FIFO Error
	if ((flag & (1U << (posi + _DMA_SxFLAG_POS_FEIF))) && (fcr & (1U << _DMA_SxFCR_POS_FEIE))) {
		crt[ifcr_reg] = (1U << (posi + _DMA_SxFLAG_POS_FEIF));
		crt.streamErrors[stream] |= _DMA_ERROR_FE;
	}
	// Direct Mode Error
	if ((flag & (1U << (posi + _DMA_SxFLAG_POS_DMEIF))) && (cr & (1U << _DMA_SxCR_POS_DMEIE))) {
		crt[ifcr_reg] = (1U << (posi + _DMA_SxFLAG_POS_DMEIF));
		crt.streamErrors[stream] |= _DMA_ERROR_DME;
	}
	// Half Transfer Complete
	if ((flag & (1U << (posi + _DMA_SxFLAG_POS_HTIF))) && (cr & (1U << _DMA_SxCR_POS_HTIE))) {
		crt[ifcr_reg] = (1U << (posi + _DMA_SxFLAG_POS_HTIF));
		if ((cr & (1U << _DMA_SxCR_POS_DBM))) {
			// Double buffer: CT selects the current buffer
			if (ct) asserv(crt.XferM1HalfCpltCallback)();
			else    asserv(crt.XferHalfCallback)();
		} else {
			if (!is_circ)
				crt[CR[stream]].setof(_DMA_SxCR_POS_HTIE, false);
			asserv(crt.XferHalfCallback)();
		}
	}
	// Transfer Complete
	if ((flag & (1U << (posi + _DMA_SxFLAG_POS_TCIF))) && (cr & (1U << _DMA_SxCR_POS_TCIE))) {
		crt[ifcr_reg] = (1U << (posi + _DMA_SxFLAG_POS_TCIF)); // clear flag first
		if (crt.streamStates[stream] == _DMA_STATE_ABORT) {
			// AbortRupt completion: disable all IT, clear all flags, set READY
			Reference cr_ref = crt[CR[stream]];
			cr_ref.setof(_DMA_SxCR_POS_TCIE, false);
			cr_ref.setof(_DMA_SxCR_POS_TEIE, false);
			cr_ref.setof(_DMA_SxCR_POS_DMEIE, false);
			cr_ref.setof(_DMA_SxCR_POS_HTIE, false);
			crt[FCR[stream]].setof(_DMA_SxFCR_POS_FEIE, false);
			crt[ifcr_reg].maset(posi, 6, 0x3F); // clear all stream flags
			crt.streamStates[stream] = _DMA_STATE_READY;
			asserv(crt.XferAbortCallback)();
			return;
		}
		if ((cr & (1U << _DMA_SxCR_POS_DBM))) {
			// Double buffer: on TC the CT bit is the *next* target, so the finished buffer is the opposite
			if (ct) asserv(crt.XferCpltCallback)();
			else    asserv(crt.XferM1CpltCallback)();
		} else {
			if (!is_circ) {
				crt[CR[stream]].setof(_DMA_SxCR_POS_TCIE, false);
				crt.streamStates[stream] = _DMA_STATE_READY;
			}
			asserv(crt.XferCpltCallback)();
		}
	}
	// manage error case (aka HAL_DMA_IRQHandler trailing error handling)
	if (crt.streamErrors[stream] != _DMA_ERROR_NONE) {
		if (crt.streamErrors[stream] & _DMA_ERROR_TE) {
			crt.streamStates[stream] = _DMA_STATE_ABORT;
			crt[CR[stream]].setof(_DMA_SxCR_POS_EN, false);// disable stream
			// wait EN=0 with software timeout (aka HAL do-while)
			uint32 count = 0;
			do {
				if (++count > DMA_TIMEOUT_VALUE) break;
			} while (crt[CR[stream]].bitof(_DMA_SxCR_POS_EN));
			crt.streamStates[stream] = crt[CR[stream]].bitof(_DMA_SxCR_POS_EN)
				? _DMA_STATE_ERROR : _DMA_STATE_READY;
		}
		asserv(crt.XferErrorCallback)();
	}
}

#endif




