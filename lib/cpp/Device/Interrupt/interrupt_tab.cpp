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

using namespace uni;

extern "C" {

	#if 0
	//
	#elif defined(_MCU_STM32F10x)
	Handler_t FUNC_EXTI[16] = { 0 };
	Handler_t FUNC_TIMx[16] = { 0 };// keep 0
	Handler_t FUNC_ADCx[4] = { 0 };// keep 0
	//

	static void _HandlerIRQ_EXTIx(byte x) {
		if (uni::EXTI::Pending & (1 << x)) {
			// GPIO_PIN_x
			if (FUNC_EXTI[x]) FUNC_EXTI[x]();
			uni::EXTI::Pending = (1 << x);
		}
	}
	void EXTI0_IRQHandler(void) {_HandlerIRQ_EXTIx(0);}
	void EXTI1_IRQHandler(void) {_HandlerIRQ_EXTIx(1);}
	void EXTI2_IRQHandler(void) {_HandlerIRQ_EXTIx(2);}
	void EXTI3_IRQHandler(void) {_HandlerIRQ_EXTIx(3);}
	void EXTI4_IRQHandler(void) {_HandlerIRQ_EXTIx(4);}
	void EXTI9_5_IRQHandler(void) {
		for (byte i = 5; i < 9; i++) _HandlerIRQ_EXTIx(i);
	}
	void EXTI15_10_IRQHandler(void) {
		for (byte i = 10; i < 16; i++) _HandlerIRQ_EXTIx(i);
	}

	static void _HandlerIRQ_TIMx(TIM_t& this_TIM) {
		//{TODO} Capture compare 1 event 
		//{TODO} Capture compare 2 event 
		//{TODO} Capture compare 3 event 
		//{TODO} Capture compare 4 event
		// TIM Update event
		{
			const stduint _CVAL_TIM_SR_UIE = 1;
			if (this_TIM[TimReg::DIER] & _CVAL_TIM_SR_UIE) { //aka __HAL_TIM_GET_IT_SOURCE
				asserv(FUNC_TIMx[this_TIM.getID()])();
				this_TIM[TimReg::SR] = ~_CVAL_TIM_SR_UIE; //aka __HAL_TIM_CLEAR_IT
			}
		}
		//{TODO} TIM Break input event
		//{TODO} TIM Trigger detection event
		//{TODO} TIM commutation event
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


	#else

	#endif
}
