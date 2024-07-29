
#include "../../../inc/c/driver/UART.h"

#if 0


#elif defined(_MCU_STM32F4x)

// bi: 8 or 16           

inline static stduint UART_DIV_SAMPLING(stduint _PCLK_, stduint _BAUD_, byte bi) {
	bi >>= 2;// div-by 4
	return (_PCLK_ * 25U) / (bi * _BAUD_);
}
inline static stduint UART_DIVMANT_SAMPLING(stduint _PCLK_, stduint _BAUD_, byte bi) {
	return (UART_DIV_SAMPLING(_PCLK_, _BAUD_, bi) / 100U);
}
inline static stduint UART_DIVFRAQ_SAMPLING(stduint _PCLK_, stduint _BAUD_, byte bi) {
	return (((UART_DIV_SAMPLING(_PCLK_, _BAUD_, bi) - (UART_DIVMANT_SAMPLING(_PCLK_, _BAUD_, bi) * 100U)) * bi + 50U) / 100U);
}

// 16: UART BRR = mantissa + overflow + fraction = (UART DIVMANT << 4) + (UART DIVFRAQ & 0xF0) + (UART DIVFRAQ & 0x0FU)
//  8: UART BRR = mantissa + overflow + fraction = (UART DIVMANT << 4) + ((UART DIVFRAQ & 0xF8) << 1) + (UART DIVFRAQ & 0x07U)
inline static stduint UART_BRR_SAMPLING(stduint _PCLK_, stduint _BAUD_, byte bi) {
	if (bi == 16)
		return (((UART_DIVMANT_SAMPLING(_PCLK_, _BAUD_, 16) << 4U) + (UART_DIVFRAQ_SAMPLING(_PCLK_, _BAUD_, 16) & 0xF0U)) + (UART_DIVFRAQ_SAMPLING(_PCLK_, _BAUD_, 16) & 0x0FU));
	else if (bi == 8)
		return (((UART_DIVMANT_SAMPLING(_PCLK_, _BAUD_, 8) << 4U) + ((UART_DIVFRAQ_SAMPLING(_PCLK_, _BAUD_, 8) & 0xF8U) << 1U)) + (UART_DIVFRAQ_SAMPLING(_PCLK_, _BAUD_, 8) & 0x07U));
	return 0;
}


namespace uni {
	//aka HAL_UART_Init
	void USART_t::setMode(_TEMP void) {
		using namespace XARTReg;
		_TEMP if (XART_ID != 1) return;
		{
			//{TODO} busy flag
			// HAL_UART_MspInit
			{
				enClock();
				GPIOA[9].setMode(GPIOMode::OUT_AF_PushPull, GPIOSpeed::Atmost_Veryhigh);// Tx
				GPIOA[9].setPull(true);
				GPIOA[10].setMode(GPIOMode::OUT_AF_PushPull, GPIOSpeed::Atmost_Veryhigh);// Rx
				GPIOA[9].setPull(true);
				(void)"GPIO Config for AUF regs"; {
					byte GPIO_AF7_USART1 = 0x07;
					GPIOA[9]._set_alternate(GPIO_AF7_USART1);
					GPIOA[10]._set_alternate(GPIO_AF7_USART1);
				}
				// : then for USART1 Interrupt
				//_TEMP NVIC.setPriority(IRQ_USART1, 0, 1);
				//_TEMP NVIC.setAble(IRQ_USART1);
			}
			enAble(false);
			//aka UART_SetConfig(UART_HandleTypeDef *huart)
			{
				self[CR2] &= ~(stduint)(0x3 << 12);//set zero, aka UartHandle.Init.StopBits = UART_STOPBITS_1;
				self[CR1] &= ~(stduint)(0x3 << 9);//aka UartHandle.Init.Parity = UART_PARITY_NONE; (USART_CR1_PCE and USART_CR1_PS)
				self[CR1].rstof(12); //aka UartHandle.Init.WordLength = UART_WORDLENGTH_8B; (USART_CR1_M)
				self[CR1] |= 0xC;//aka UartHandle.Init.Mode = UART_MODE_TX_RX; (USART_CR1_TE | USART_CR1_RE)
				self[CR1].rstof(15);// (USART_CR1_OVER8)
				self[CR3] &= ~(stduint)(0x3 << 8);//aka UartHandle.Init.HwFlowCtl = UART_HWCONTROL_NONE; (USART_CR3_RTSE | USART_CR3_CTSE)
				// : byte over_sampling = _TEMP 16;// or 8
				stduint freq = XART_ID == 1 || XART_ID == 6 ? RCC.getFrequencyPCLK2() : RCC.getFrequencyPCLK1();
				self[BRR] = UART_BRR_SAMPLING(freq, /*UartHandle.Init.BaudRate = DEBUG_USART_BAUDRATE;*/ 115200, 16);
			}
			// In asynchronous mode, the following bits must be kept cleared:
			// - LINEN and CLKEN bits in the USART_CR2 register,
			// - SCEN, HDSEL and IREN  bits in the USART_CR3 register.
			self[CR2].rstof(_USART_CR2_POSI_CLKEN);
			self[CR2].rstof(_USART_CR2_POSI_LINEN);
			self[CR3].rstof(_USART_CR3_POSI_IREN);
			self[CR3].rstof(_USART_CR3_POSI_HDSEL);
			self[CR3].rstof(_USART_CR3_POSI_SCEN);
			enAble(true);
		}
	}

	void USART_t::setInterrupt(Handler_t fn) {
		FUNC_XART[XART_ID] = fn;
	}
	static Request_t XART_Request_list[8] = {
		(Request_t)0, IRQ_USART1, IRQ_USART2, IRQ_USART3,
		IRQ_UART4, IRQ_UART5, IRQ_USART6, (Request_t)0
	};
	void USART_t::setInterruptPriority(byte preempt, byte sub_priority) {
		NVIC.setPriority(XART_Request_list[XART_ID], preempt, sub_priority);
	}
	void USART_t::enInterrupt(bool enable) {
		using namespace XARTReg;
		if (enable)
		{
			NVIC.setAble(XART_Request_list[XART_ID]);
			stduint val = ((stduint)1 << 28U | 0x00000020);// _TEMP only UART_IT_RXNE aka ((uint32_t)(UART_CR1_REG_INDEX << 28U | USART_CR1_RXNEIE)) aka this
			stduint UART_IT_MASK = 0x0000FFFFU;
			(((val >> 28U) == 1/*UART_CR1_REG_INDEX*/) ? (self[CR1] |= (val & UART_IT_MASK)) :
				((val >> 28U) == 2/*UART_CR2_REG_INDEX*/) ? (self[CR2] |= (val & UART_IT_MASK)) :
				(self[CR3] |= (val & UART_IT_MASK)));//aka __HAL_UART_ENABLE_IT
		}
		else _TODO;
	}



}


#endif
