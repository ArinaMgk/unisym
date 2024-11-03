// ASCII CPP TAB4 CRLF
// Docutitle: Interrupt List Abstract
// Codifiers: @dosconio: 20240528
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
/*
	Copyright 2023 ArinaMgk

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0
	http://unisym.org/license.html

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/
#include "../../../../inc/cpp/interrupt"
#include "../../../../inc/cpp/Device/EXTI"
#include "../../../../inc/cpp/Device/TIM"
#include "../../../../inc/cpp/Device/ADC"
#include "../../../../inc/cpp/Device/UART"

using namespace uni;

// SysTick_Handler in SysTick.cpp

#define i_index(a,b) (*a[b])

extern "C" {

#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x)
	_ESYM_CPP {
		static void _HandlerIRQ_EXTIx(byte x);
		static void _HandlerIRQ_TIMx(byte typ, byte TIM_ID);
	}
	Handler_t FUNC_EXTI[16] = { 0 };
	_ESYM_CPP{
	static void _HandlerIRQ_EXTIx(byte x) {
		if (uni::EXTI::Pending & (1 << x)) {
			// GPIO_PIN_x
			if (FUNC_EXTI[x]) FUNC_EXTI[x]();
			uni::EXTI::Pending = (1 << x);
		}
	}
	}

	void EXTI0_IRQHandler(void) { _HandlerIRQ_EXTIx(0); }
	void EXTI1_IRQHandler(void) { _HandlerIRQ_EXTIx(1); }
	void EXTI2_IRQHandler(void) { _HandlerIRQ_EXTIx(2); }
	void EXTI3_IRQHandler(void) { _HandlerIRQ_EXTIx(3); }
	void EXTI4_IRQHandler(void) { _HandlerIRQ_EXTIx(4); }
	void EXTI9_5_IRQHandler(void) {
		for (byte i = 5; i < 9; i++) _HandlerIRQ_EXTIx(i);
	}
	void EXTI15_10_IRQHandler(void) {
		for (byte i = 10; i < 16; i++) _HandlerIRQ_EXTIx(i);
	}

	// ----
	Handler_t FUNC_XART[8] = { 0 };

	// ---- TIM ----
	Handler_t FUNC_TIMx[16] = { 0 }; // Period Elapsed Callback
	//: typ: 0:Basic, 1:CaptureCompare, 2:Update, 3:Break, 4:Trigger, 5:Commutation
	void TIM2_IRQHandler(void) { _HandlerIRQ_TIMx(0, 2); }
	void TIM3_IRQHandler(void) { _HandlerIRQ_TIMx(0, 3); }
	void TIM4_IRQHandler(void) { _HandlerIRQ_TIMx(0, 4); }

	Handler_t FUNC_ADCx[4] = { 0 };

#endif

	
#if 0
	//
#elif defined(_MCU_STM32F1x)
	_ESYM_CPP{
		static void _HandlerIRQ_XART(byte art_id);
		static void _HandlerIRQ_TIMx(byte typ, byte TIM_ID);
		static void _HandlerIRQ_DMAChannelx(byte dma_id, byte chanx);
	}
	//
	_ESYM_CPP{
	static void _HandlerIRQ_XART(byte art_id) {
		if (!XART.isSync(art_id)) return;//{TEMP}
		if ((*(USART_t*)XART[art_id])[XARTReg::SR].bitof(5)) { //aka __HAL_UART_GET_FLAG, 5 is USART_SR_RXNE
			asserv(FUNC_XART[art_id])();
		}
		// ... more
	}
	static void _HandlerIRQ_TIMx(byte typ, byte TIM_ID) {}
	//{TODEL}
	static void _HandlerIRQ_TIMx(TIM_t& this_TIM) {
		using namespace TimReg;
		//{TODO} Capture compare 1 event 
		//{TODO} Capture compare 2 event 
		//{TODO} Capture compare 3 event 
		//{TODO} Capture compare 4 event
		// TIM Update event
		{
			const stduint _CVAL_TIM_SR_UIE = 1;
			if (this_TIM[DIER] & _CVAL_TIM_SR_UIE) { //aka __HAL_TIM_GET_IT_SOURCE
				asserv(FUNC_TIMx[this_TIM.getID()])();
				this_TIM[SR] = ~_CVAL_TIM_SR_UIE; //aka __HAL_TIM_CLEAR_IT
			}
		}
		//{TODO} TIM Break input event
		//{TODO} TIM Trigger detection event
		//{TODO} TIM commutation event
	}
		
	}


	void TIM6_IRQHandler(void) {_HandlerIRQ_TIMx(uni::TIM6);}
	
	void ADC1_2_IRQHandler(void) {
		// 1 Check End of Conversion flag for regular group
		for1(i, 2) {
			ADC_t& crt = i == 1 ? uni::ADC1 : uni::ADC2;
			if (crt[ADCReg::CR1].bitof(_ADC_CR1_POS_EOCIE) && crt[ADCReg::SR].bitof(_ADC_SR_POS_EOC)) {
				// 确定是否有即将到来的外部触发、连续模式或扫描序列转换，适用于组内常规转换。对于STM32F1设备，如果启用了序列器（选择了多个通道），则在序列结束后会触发转换结束标志。
				if (crt.isTriggeredBySoftware() && crt.getConfigLastDiscCount()) {
					crt[ADCReg::CR1].rstof(_ADC_CR1_POS_EOCIE);// Disable ADC end of conversion interrupt on group regular
				}
				asserv(FUNC_ADCx[i])();
				unchecked{//aka __HAL_ADC_CLEAR_FLAG
					crt[ADCReg::SR].rstof(_ADC_SR_POS_STRT); 
					crt[ADCReg::SR].rstof(_ADC_SR_POS_STRT);
				}
				break;
			}
		}
		//{TODO} 2 Check End of Conversion flag for injected group
		//{TODO} 3 Check Analog watchdog flags
	}

	void ADC3_IRQHandler(void) { _TODO }

	_ESYM_CPP{
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
			crt[DMAReg::IFCR] = (1U << 0 /*not 3*/ ) << (chanx << 2);// Clear all flags
		}
	}
	    
	}

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
		//{} ...
	}


#elif defined(_MCU_STM32F4x)

	

	static void _HandlerIRQ_XART(byte art_id) {
		if (!XART.isSync(art_id)) return;//{TEMP}
		if ((*(USART_t*)XART[art_id])[XARTReg::SR].bitof(5)) { //aka __HAL_UART_GET_FLAG, 5 is USART_SR_RXNE
			asserv(FUNC_XART[art_id])();
		}
		// ... more
	}
	

	void USART6_IRQHandler(void) { _HandlerIRQ_XART(6); }	


	// pres: pos of SR and of DIER should be the same
	static bool _HandlerIRQ_TIMx_Exist(byte TIM_ID, stduint pos) {
		using namespace TimReg;
		TIM_t& t = i_index(TIM, TIM_ID);
		if (t[SR].bitof(pos)) {
			if (t[DIER].bitof(pos)) { 
				t[SR].setof(pos, false);
				return true;
			}
			else return _TEMP false;
		}
		return false;
	}
	static void _HandlerIRQ_TIMx_Channel(byte TIM_ID, byte chan) {
		using namespace TimReg;
		TIM_t& t = i_index(TIM, TIM_ID);
		if (_HandlerIRQ_TIMx_Exist(TIM_ID, TIM_ID)) {
			// SR.CCxIF and DIER.CCx
			t.last_src = chan;
			bool tim1or2 = chan <= 2;
			if (t[tim1or2 ? CCMR1 : CCMR2].mask(iseven(chan) ? 8 : 0, 2)) {
				// Input capture event
				callif(t.FUNC_IC_Capture);
			}
			else {
				// Output compare event
				callif(t.FUNC_OC_DelayElapsed);
				callif(t.FUNC_PWMPulseFinished);
			}
			t.last_src = nil;
		}
	}
	static void _HandlerIRQ_TIMx(byte typ, byte TIM_ID) {
		//{TEMP} consider TIM_C just
		using namespace TimReg;
		if (typ) _TEMP return;
		if (Ranglin(TIM_ID, 2, 4)) {
			TIM_t& t = i_index(TIM, TIM_ID);
			_HandlerIRQ_TIMx_Channel(TIM_ID, 1);
			_HandlerIRQ_TIMx_Channel(TIM_ID, 2);
			_HandlerIRQ_TIMx_Channel(TIM_ID, 3);
			_HandlerIRQ_TIMx_Channel(TIM_ID, 4);
			if (_HandlerIRQ_TIMx_Exist(TIM_ID, 0)) { // UIF
				callif(FUNC_TIMx[TIM_ID]);
				//(*TIM[TIM_ID])[SR].setof(0, true);
				TIM[TIM_ID]->enAble();
			}
			if (_HandlerIRQ_TIMx_Exist(TIM_ID, 7)) { // BIF BIE
				callif(t.FUNC_Break);// Break input event
				//{TODO}
			}
			if (_HandlerIRQ_TIMx_Exist(TIM_ID, 6)) { // TIF TIE
				callif(t.FUNC_Trigger);// Trigger detection event
				//{TODO}
			}
			if (_HandlerIRQ_TIMx_Exist(TIM_ID, 5)) { // COMIF COMIE
				callif(t.FUNC_Commute);// Commutation event
				//{TODO}
			}
		}
		else _TEMP return;
	}

	void ADC_IRQHandler(void) {
		
	}

	
#endif

#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x)
	// ---- XART ----
	void USART1_IRQHandler(void) { _HandlerIRQ_XART(1); }
	void USART2_IRQHandler(void) { _HandlerIRQ_XART(2); }
	void USART3_IRQHandler(void) { _HandlerIRQ_XART(3); }
	void UART4_IRQHandler(void) { _HandlerIRQ_XART(4); }
	void UART5_IRQHandler(void) { _HandlerIRQ_XART(5); }
#endif	

}
