#define _MCU_XART_TEMP
#include "../../../../inc/cpp/Device/UART"

#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)
void _HandlerIRQ_XART(byte art_id);
#endif

_ESYM_C{
#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)
	Handler_t FUNC_XART[16] = { 0 };


	void USART1_IRQHandler(void) { _HandlerIRQ_XART(1); }
	void USART2_IRQHandler(void) { _HandlerIRQ_XART(2); }
	void USART3_IRQHandler(void) { _HandlerIRQ_XART(3); }
	void UART4_IRQHandler(void) { _HandlerIRQ_XART(4); }
	void UART5_IRQHandler(void) { _HandlerIRQ_XART(5); }
	#if defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)
	void USART6_IRQHandler(void) { _HandlerIRQ_XART(6); }
	#endif
	#if defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)
	void UART7_IRQHandler(void) { _HandlerIRQ_XART(7); }
	void UART8_IRQHandler(void) { _HandlerIRQ_XART(8); }
	#endif
	
#endif
}

#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x) || defined(_MPU_STM32MP13)
void _HandlerIRQ_XART(byte art_id) {
	//{?} Instrument here to support OS ?
	#if defined(_HIS_INT)
	asserv(FUNC_XART[art_id])();
	#elif !defined(_MPU_STM32MP13)
	if (!XART.isSync(art_id)) return;//{TEMP}
	if ((*(USART_t*)XART[art_id])[XARTReg::SR].bitof(5)) { //aka __HAL_UART_GET_FLAG, 5 is USART_SR_RXNE
		asserv(FUNC_XART[art_id])();
	}
	// ... more
	//{?} Instrument here to support OS ?
	#endif
}
#elif defined(_MCU_STM32H7x)
void _HandlerIRQ_XART(byte art_id) _Comment("AKA HAL_UART_IRQHandler") {
	USART_t& xart = *(USART_t*)XART[art_id];
	Reflocal(isrflags) = xart[XARTReg::ISR];
	Reflocal(cr1its) = xart[XARTReg::CR1];
	Reflocal(cr3its) = xart[XARTReg::CR3];
	uint32 errorflags = (isrflags & _IMM(USART_ISR_PE | USART_ISR_FE | USART_ISR_ORE | USART_ISR_NE));
	if (!errorflags) {
		if ((isrflags & USART_ISR_RXNE) && (USART_CR1_RXNEIE_REF(cr1its) || (USART_CR3_RXFTIE_REF(cr3its))))
		{
			xart.innHandlerByInterrupt();
			asserv(FUNC_XART[art_id])(); return;
		}
	}
	//
	bool err_ore = false;
	if ((USART_CR3_RXFTIE_REF(cr3its) | USART_CR3_EIE_REF(cr3its))
		|| ( USART_CR1_RXNEIE_REF(cr1its) | USART_CR1_PEIE_REF(cr1its) ) )
	{
		if ((isrflags & USART_ISR_PE) && USART_CR1_PEIE_REF(cr1its))
		{
			xart[XARTReg::ICR] = USART_ICR_PECF;
			xart.error = "PE";
		}
		if ((isrflags & USART_ISR_FE) && USART_CR3_EIE_REF(cr3its))
		{
			xart[XARTReg::ICR] = USART_ICR_FECF;
			xart.error = "FE";
		}
		if ((isrflags & USART_ISR_NE) && USART_CR3_EIE_REF(cr3its))
		{
			xart[XARTReg::ICR] = USART_ICR_NCF;
			xart.error = "NE";
		}
		if ((isrflags & USART_ISR_ORE)
			&& (USART_CR1_RXNEIE_REF(cr1its) ||
				USART_CR3_RXFTIE_REF(cr3its) ||
				USART_CR3_EIE_REF(cr3its)))
		{
			xart[XARTReg::ICR] = USART_ICR_ORECF;
			err_ore = true;
			xart.error = "ORE";
		}
		// call back
		if (xart.error)
		{
			/* UART in mode Receiver */
			if (isrflags & USART_ISR_RXNE
				&& (USART_CR1_RXNEIE_REF(cr1its)
					|| USART_CR3_RXFTIE_REF(cr3its)))
			{
				xart.innHandlerByInterrupt();
			}
			/* If Overrun error occurs, or if any error occurs in DMA mode reception, consider error as blocking */
			if (err_ore || (xart[XARTReg::CR3] & USART_CR3_DMAR))
			{
				/* Blocking error : transfer is aborted
				 Set the UART state ready to be able to start again the process,
				 Disable Rx Interrupts, and disable Rx DMA request, if ongoing */
				// UART_EndRxTransfer
				{
					USART_CR1_RXNEIE_REF(cr1its) = 0;
					USART_CR1_PEIE_REF(cr1its) = 0;
					USART_CR3_EIE_REF(cr3its) = 0;
					
				}
#if 0 //{TODO}
				/* Disable the UART DMA Rx request if enabled */
				if (xart[XARTReg::CR3] & USART_CR3_DMAR)
				{
					xart[XARTReg::CR3] &= ~_IMM(USART_CR3_DMAR);
					/* Abort the UART DMA Rx channel */
					if (huart->hdmarx != NULL)
					{
						/* Set the UART DMA Abort callback :
						 will lead to call HAL_UART_ErrorCallback() at end of DMA abort procedure */
						huart->hdmarx->XferAbortCallback = UART_DMAAbortOnError;

						/* Abort DMA RX */
						if (HAL_DMA_Abort_IT(huart->hdmarx) != HAL_OK)
						{
							/* Call Directly huart->hdmarx->XferAbortCallback function in case of error */
							huart->hdmarx->XferAbortCallback(huart->hdmarx);
						}
					}
					else
					{
					  /* Call user error callback */
						HAL_UART_ErrorCallback(huart);
					}
				}
				else {
					HAL_UART_ErrorCallback(huart);
				}
#endif
			}
			else
			{
				/* Non Blocking error : transfer could go on.
				 Error is notified to user through user error callback */
				//{} HAL_UART_ErrorCallback(huart);
				xart.error = NULL;
			}
		}
		asserv(FUNC_XART[art_id])(); return;

	} /* End if some error occurs */
	/* UART wakeup from Stop mode interrupt occurred ---------------------------*/
	if ((isrflags & USART_ISR_WUF) && (cr3its & USART_CR3_WUFIE))
	{
		xart[XARTReg::ICR] = USART_ICR_WUCF;
		/* Set the UART state ready to be able to start again the process */
		// huart->gState = HAL_UART_STATE_READY;
		// huart->RxState = HAL_UART_STATE_READY;
		//{} HAL_UARTEx_WakeupCallback(huart);
		asserv(FUNC_XART[art_id])(); return;
	}
#if 0 //{TODO}
	
	/* UART in mode Transmitter ------------------------------------------------*/
	if ((isrflags & USART_ISR_TXE) && ((cr1its & USART_CR1_TXEIE) || cr3its & USART_CR3_TXFTIE))
	{
		UART_Transmit_IT(huart);
		asserv(FUNC_XART[art_id])(); return;
	}
	
	/* UART in mode Transmitter (transmission end) -----------------------------*/
	if (USART_ISR_TC_REF(isrflags) && (cr1its & USART_CR1_TCIE))
	{
		UART_EndTransmit_IT(huart);
		asserv(FUNC_XART[art_id])(); return;
	}
#endif
	/* UART TX FIFO Empty  -----------------------------------------------------*/
	if ((isrflags & USART_ISR_TXFE) && (cr1its & USART_CR1_TXFEIE))
	{
		xart[XARTReg::CR1] &= ~USART_CR1_TXFEIE;
	}
	asserv(FUNC_XART[art_id])();
}
#endif

















