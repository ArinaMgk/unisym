// UTF-8 CPP-ISO11 TAB4 CRLF
// Docutitle: (Device) Digital-Analog Converter
// Codifiers: @dosconio: 20240715~;
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
// Dependens: GPIO DMA
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


#include "../../../inc/cpp/Device/DAC"
#include "../../../inc/cpp/Device/TIM"
#include "../../../inc/cpp/Device/RCC/RCCAddress"
#include "../../../inc/cpp/Device/RCC/RCCClock"

#define _DAC_ALIGN_12B_R 0x00000000U
#define _DAC_ALIGN_12B_L 0x00000004U
#define _DAC_ALIGN_8B_R  0x00000008U

namespace uni {

	#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x)
	
	bool DAC_t::enClock(bool ena) {
		Reference(_RCC_APB1ENR_ADDR).setof(_RCC_APB1ENR_POSI_ENCLK_DAC);
		if (ena != Reference(_RCC_APB1ENR_ADDR).bitof(_RCC_APB1ENR_POSI_ENCLK_DAC))
			return false;
		return true;
	}

	static DACReg::DACRegType DHR12Rx[] = { DACReg::DHR12R1, DACReg::DHR12R2 };
	void DAC_t::setOutput(byte channel, uint16 val) {
		using namespace ::uni::DACReg;
		// assert (channel)
		val &= 0x0FFF;// max output 2.5V
		self[DHR12Rx[channel]] = val;
	}

	#endif

	#if 0
	#elif defined(_MCU_STM32F1x)
	#define _DAC_Counts 1


	bool DAC_t::enDMA(GPIO_Pin& pin, DACTrigger::DACTrigger trigger, bool buffer_enable, pureptr_t data, uint32_t leng, uint32_t align) {
		enClock();
		//{TEMP} DMA2
		if (&pin == &GPIOA[5] || &pin == &GPIOA[4]) {
			//aka HAL_DAC_Init = HAL_DAC_MspInit
			pin.setMode(GPIOMode::IN_Analog);
			DMA2.enClock();
			DMA2.setMode(4, false, true, false, true);
			self.bind = (pureptr_t)&DMA2;
			DMA2.bind = (pureptr_t)this;
			//aka HAL_DAC_ConfigChannel
			byte channel = 2;//{???} for GPIOA5
			stduint tmp = 0;
			BitSev(tmp, _DAC_CR_POS_BOFFx, buffer_enable);
			tmp |= _IMM(trigger);
			BitClr(tmp, _DAC_CR_POS_WAVEx);
			if (channel == 1)
			{
				self[DACReg::CR] &= 0xFFFF0000;
				self[DACReg::CR] |= tmp;
				_TODO;//{}
			}
			else // 2
			{
				self[DACReg::CR] &= 0x0000FFFF;
				self[DACReg::CR] |= tmp << 0x10;
				//aka HAL_TIM_Base_Init
				TIM6.setMode();
				TIM6.ConfigMaster(0x00000020); // TIM_TRGO_UPDATE
				TIM6.enAble();
			}
			//aka HAL_DAC_Start_DMA
			//{PASS} hdac->DMA_Handle2->XferCpltCallback = DAC_DMAConvCpltCh2;
			//{PASS} hdac->DMA_Handle2->XferHalfCpltCallback = DAC_DMAHalfConvCpltCh2;
			//{PASS} hdac->DMA_Handle2->XferErrorCallback = DAC_DMAErrorCh2;
			self[DACReg::CR].setof(_DAC_CR_POS_DMAENx + 16*(channel-1));//{} // DAC_CR_DMAEN1
			tmp = 0;
			switch (align)
			{
			case _DAC_ALIGN_12B_R:
				tmp = (uint32_t)&self[DACReg::DHR12R2];//{} DHR12R1 for chan1
				break;
			case _DAC_ALIGN_12B_L:
				tmp = (uint32_t)&self[DACReg::DHR12L2];//{} DHR12L1 for chan1
				break;
			case _DAC_ALIGN_8B_R:
				tmp = (uint32_t)&self[DACReg::DHR8R2];//{} DHR8R1 for chan1
				break;
			default:
				break;
			}
			DMA2.StartInterrupt((pureptr_t)tmp, data, leng, _TEMP 4);
			enAble(channel);//aka __HAL_DAC_ENABLE
		}
		else return false;
		return true;
	}


	//
	/*
	void DAC_t::setInterrupt(Handler_t fn) {
		FUNC_DACx[getID()] = fn;
	}

	static Request_t DACx_Request_list[4] = {
		(Request_t)0, IRQ_DAC1_2, IRQ_DAC1_2,
		IRQ_DAC3
	};
	void DAC_t::setInterruptPriority(byte preempt, byte sub_priority) {
		NVIC.setPriority(DACx_Request_list[DAC_ID], preempt, sub_priority);
	}

	void DAC_t::enInterrupt(bool enable, bool trigger_ext_posedge) {
		// C-with HAL_DAC_Start_IT
		if (enable) {

			//
			NVIC.setAble(DACx_Request_list[self.DAC_ID]);
		}
		else _TODO;
	}
*/

	#elif defined(_MCU_STM32F4x)

	bool DAC_t::enAble(GPIO_Pin& pin) {
		using namespace DACReg;
		enClock();
		pin.setMode(GPIOMode::IN_Analog);//{ISSUE} need pull-dn?
		// : DAC_Init
		/*TEMP
		.DAC_Trigger = DAC_Trigger_None;
		.DAC_WaveGeneration = DAC_WaveGeneration_None;
		.DAC_LFSRUnmask_TriangleAmplitude = DAC_LFSRUnmask_Bit0;
		.DAC_OutputBuffer = DAC_OutputBuffer_Disable;
		*/
		uint32 tmp = self[CR];
		byte channel = 0;
		if (&pin == &GPIOA[4]) channel = 1;
		else if (&pin == &GPIOA[5]) channel = 2;
		if (!channel) return false;
		const byte shift = (channel - 1) << 4;
		tmp &= ~(0x0000FFFF << shift);
		// tmp2 = (DAC_InitStruct->DAC_Trigger | DAC_InitStruct->DAC_WaveGeneration | 		DAC_InitStruct->DAC_LFSRUnmask_TriangleAmplitude | DAC_InitStruct->DAC_OutputBuffer);
		uint32 tmp2 = 0x00000002 << shift;// DAC_OutputBuffer_Disable
		tmp |= tmp2;
		self[CR] = tmp;
		enAble(channel, true);//aka DAC_Cmd(DAC_Channel_1, ENABLE);
		//aka DAC_SetChannel1Data(DAC_Align_12b_R, 0);
		_TEMP{
			self[DHR12R1] = 0;
		}
		return true;
	}

	#endif

#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x)
	DAC_t DAC(0x40007400);//~0x400077FF
#endif
	
}
