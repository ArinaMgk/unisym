
#include "../../../inc/c/driver/UART.h"

#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x)
// bi: 8 or 16           
//:not check detailedly for F1
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
	static void setMode_initGPIO(byte XART_ID);
	//aka HAL_UART_Init
	void USART_t::setMode(stduint band_rate) {
		using namespace XARTReg;
		_TEMP if (XART_ID != 1) return;
		{
			//{TODO} busy flag
			enClock();
			setMode_initGPIO(XART_ID);// HAL_UART_MspInit
			enAble(false);
			//aka UART_SetConfig(UART_HandleTypeDef *huart)
			{
				self[CR2] &= ~_IMM(0x3 << 12);//set zero, aka UartHandle.Init.StopBits = UART_STOPBITS_1;
				self[CR1] &= ~_IMM(0x3 << 9);//aka UartHandle.Init.Parity = UART_PARITY_NONE; (USART_CR1_PCE and USART_CR1_PS)
				self[CR1].rstof(12); //aka UartHandle.Init.WordLength = UART_WORDLENGTH_8B; (USART_CR1_M)
				self[CR1] |= 0xC;//aka UartHandle.Init.Mode = UART_MODE_TX_RX; (USART_CR1_TE | USART_CR1_RE)
				self[CR1].rstof(15);// (USART_CR1_OVER8)
				self[CR3] &= ~_IMM(0x3 << 8);//aka UartHandle.Init.HwFlowCtl = UART_HWCONTROL_NONE; (USART_CR3_RTSE | USART_CR3_CTSE)
				// : byte over_sampling = _TEMP 16;// or 8
				stduint freq = XART_ID == 1 || XART_ID == 6 ? RCC.getFrequencyPCLK2() : RCC.getFrequencyPCLK1();
				last_bandrate = band_rate;
				self[BRR] = UART_BRR_SAMPLING(freq, band_rate, 16);
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

	//aka HAL_UART_Receive
	//{TODO} conflict
	int USART_t::operator>> (int& res) {
		using namespace XARTReg;
		bool len9b = false;// or 8b
		bool parity = false;
		stduint mask = len9b ?
			(parity ? 0x1FF : 0xFF) :
			(parity ? 0x0FF : 0x7F);
		int d = int(self[DR]);
		return res = d & mask;
	}

	extern stduint SystemCoreClock;
	void USART_t::Delay_unit() {
		for (volatile stduint i = 0; i < SystemCoreClock / last_bandrate; i++);
	}
	
	//aka HAL_UART_Transmit
	//{TODO} conflict
	USART_t& USART_t::operator << (stduint dat) {
		using namespace XARTReg;
		bool len9b = false;// or 8b
		self[DR] = dat & (len9b ? 0x1FF : 0x0FF);
		Delay_unit();
		return self;
	}
	USART_t& USART_t::operator << (const char* p) {
		while (*p) self << stduint(*p++);
		return self;
	}

	bool USART_t::enAble(bool ena) {
		using namespace XARTReg;
		self[CR1].setof(13, ena);// 13 is USART_CR1_UE_Pos
		return true;
	}

	bool USART_t::enClock(bool ena) {
		//{!} only for usart1
		using namespace RCCReg;
		RCC[APB2ENR].setof(_RCC_APB2ENR_POSI_ENCLK_USART1, ena);
		return RCC[APB2ENR].bitof(_RCC_APB2ENR_POSI_ENCLK_USART1) == ena;
	}

	// ---- ---- INTSYS ---- ----

	void USART_t::setInterrupt(Handler_t fn) {
		FUNC_XART[XART_ID] = fn;
	}
	static Request_t XART_Request_list[8] = {
		Request_None, IRQ_USART1, IRQ_USART2, IRQ_USART3,
		IRQ_UART4, IRQ_UART5
		#ifdef _MCU_STM32F4x
		, IRQ_USART6, Request_None
		#endif
	};
	void USART_t::setInterruptPriority(byte preempt, byte sub_priority) {
		NVIC.setPriority(XART_Request_list[XART_ID], preempt, sub_priority);
	}

	void USART_t::enInterrupt(bool enable) {
		using namespace XARTReg;
		if (enable)
		{
			NVIC.setAble(XART_Request_list[XART_ID]);
			stduint val = (_IMM1 << 28U | 0x00000020);// _TEMP only UART_IT_RXNE aka ((uint32_t)(UART_CR1_REG_INDEX << 28U | USART_CR1_RXNEIE)) aka this
			stduint UART_IT_MASK = 0x0000FFFFU;
			(((val >> 28U) == 1/*UART_CR1_REG_INDEX*/) ? (self[CR1] |= (val & UART_IT_MASK)) :
				((val >> 28U) == 2/*UART_CR2_REG_INDEX*/) ? (self[CR2] |= (val & UART_IT_MASK)) :
				(self[CR3] |= (val & UART_IT_MASK)));//aka __HAL_UART_ENABLE_IT
		}
		else _TODO;
	}
}
#endif

#if 0

#elif defined(_MCU_STM32F1x)

namespace uni {
	static void setMode_initGPIO(byte XART_ID) {
		GPIOA[9].setMode(GPIOMode::OUT_AF_PushPull, GPIOSpeed::Atmost_50MHz);// Tx
		GPIOA[9].setPull(true);
		GPIOA[10].setMode(GPIOMode::IN_Floating);// Rx
	}
}

#elif defined(_MCU_STM32F4x)




namespace uni {

	//{ONLY}  F407 & F417
	static byte GPINs_AFs_XARTx[1 + 6] = { nil,
		7,7,7, 8,8,8
	};

	static void setMode_initGPIO(byte XART_ID) {
		GPIOA[9].setMode(GPIOMode::OUT_AF_PushPull, GPIOSpeed::Atmost_Veryhigh);// Tx
		GPIOA[9].setPull(true);
		GPIOA[10].setMode(GPIOMode::OUT_AF_PushPull, GPIOSpeed::Atmost_Veryhigh);// Rx
		GPIOA[10].setPull(true);
		(void)"GPIO Config for AUF regs"; {
			byte GPIO_AF7_USART1 = 0x07;
			GPIOA[ 9]._set_alternate(GPINs_AFs_XARTx[XART_ID]);
			GPIOA[10]._set_alternate(GPINs_AFs_XARTx[XART_ID]);
		}
	}





}


#endif
