#include "../../../../inc/cpp/Device/UART"

#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x) || defined(_MPU_STM32MP13)
static void _HandlerIRQ_XART(byte art_id);
#endif

_ESYM_C{
#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x) || defined(_MPU_STM32MP13)
	Handler_t FUNC_XART[16] = { 0 };


	void USART1_IRQHandler(void) { _HandlerIRQ_XART(1); }
	void USART2_IRQHandler(void) { _HandlerIRQ_XART(2); }
	void USART3_IRQHandler(void) { _HandlerIRQ_XART(3); }
	void UART4_IRQHandler(void) { _HandlerIRQ_XART(4); }
	void UART5_IRQHandler(void) { _HandlerIRQ_XART(5); }
#if defined(_MCU_STM32F4x) || defined(_MPU_STM32MP13)
	void USART6_IRQHandler(void) { _HandlerIRQ_XART(6); }
#endif
#if defined(_MPU_STM32MP13)
	void UART7_IRQHandler(void) { _HandlerIRQ_XART(7); }
	void UART8_IRQHandler(void) { _HandlerIRQ_XART(8); }
#endif
	
#endif
}

#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x) || defined(_MPU_STM32MP13)
static void _HandlerIRQ_XART(byte art_id) {
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
#endif

















