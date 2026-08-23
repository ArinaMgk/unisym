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
#include "../../../inc/cpp/Device/SysTick"
#include "../../../inc/cpp/Device/RCC/RCCAddress"
#include "../../../inc/cpp/Device/RCC/RCCClock"

namespace uni {

	#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x)

	bool DAC_t::enClock(bool ena) {
		#if defined(_MCU_STM32H7x)
		Reference(_RCC_APB1LENR_ADDR).setof(_RCC_APB1LENR_POSI_ENCLK_DAC, ena);
		if (ena != Reference(_RCC_APB1LENR_ADDR).bitof(_RCC_APB1LENR_POSI_ENCLK_DAC))
		#else
		Reference(_RCC_APB1ENR_ADDR).setof(_RCC_APB1ENR_POSI_ENCLK_DAC, ena);
		if (ena != Reference(_RCC_APB1ENR_ADDR).bitof(_RCC_APB1ENR_POSI_ENCLK_DAC))
		#endif
			return false;
		return true;
	}

	static DACReg::DACRegType DHR12Rx[] = { DACReg::DHR12R1, DACReg::DHR12R2 };
	static DACReg::DACRegType DORx[] = { DACReg::DOR1, DACReg::DOR2 };
	void DAC_t::setOutput(byte channel, uint16 val) {
		using namespace ::uni::DACReg;
		if (channel != 1 && channel != 2) return;
		val &= 0x0FFF;// max output 2.5V
		self[DHR12Rx[channel - 1]] = val;
	}
	uint16 DAC_t::getValue(byte channel) {
		if (channel != 1 && channel != 2) return 0;
		return self[DORx[channel - 1]];
	}
	bool DAC_t::StopDMA(byte channel) {
		if (channel != 1 && channel != 2) return false;
		self[DACReg::CR].setof(_DAC_CR_POS_DMAENx + 16 * (channel - 1), false);
		#ifdef _MCU_STM32F1x
		DMA2.enAble(false, channel + 2);// DAC ch1 -> DMA2 ch3, DAC ch2 -> DMA2 ch4
		#endif
		#ifdef _MCU_STM32H7x
		self[DACReg::CR].setof(_DAC_CR_POS_ENx + 16 * (channel - 1), false);
		self[DACReg::CR].setof(_DAC_CR_POS_DMAUDRIEx + 16 * (channel - 1), false);
		if (bind) {
			const DMAStream& stream = *(const DMAStream*)bind;
			stream.Abort();
		}
		#endif
		return true;
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
			byte channel = getChannel(pin);
			if (!channel) return false;
			byte dma_channel = channel + 2;// DAC ch1 -> DMA2 ch3, DAC ch2 -> DMA2 ch4
			DMA2.enClock();
			if (!DMA2.setMode(dma_channel, false, true, false, true))
				return false;
			self.bind = (pureptr_t)&DMA2;
			DMA2.bind = (pureptr_t)this;
			//aka HAL_DAC_ConfigChannel
			stduint tmp = 0;
			BitSev(tmp, _DAC_CR_POS_BOFFx, buffer_enable);
			tmp |= _IMM(trigger);
			BitClr(tmp, _DAC_CR_POS_WAVEx);
			byte shift = (channel - 1) * 16;
			stduint cr_mask = (stduint)0xFFFF << shift;
			self[DACReg::CR] &= ~cr_mask;
			self[DACReg::CR] |= tmp << shift;
			if (trigger == DACTrigger::T6) {
				// Keep the original convenience path for TIM6-triggered DAC DMA.
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
				tmp = (uint32_t)&self[channel == 1 ? DACReg::DHR12R1 : DACReg::DHR12R2];
				break;
			case _DAC_ALIGN_12B_L:
				tmp = (uint32_t)&self[channel == 1 ? DACReg::DHR12L1 : DACReg::DHR12L2];
				break;
			case _DAC_ALIGN_8B_R:
				tmp = (uint32_t)&self[channel == 1 ? DACReg::DHR8R1 : DACReg::DHR8R2];
				break;
			default:
				break;
			}
			DMA2.StartInterrupt((pureptr_t)tmp, data, leng, dma_channel);
			enAble(channel);//aka __HAL_DAC_ENABLE
		}
		else return false;
		return true;
	}


	//
	/*
	void DAC_t::setInterrupt(Handler_t f) {
		FUNC_DACx[getID()] = f;
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
		return true;
	}

	#elif defined(_MCU_STM32H7x)

	bool DAC_t::enAble(GPIO_Pin& pin) {
		using namespace DACReg;
		enClock();
		pin.setMode(GPIOMode::IN_Analog);//{ISSUE} need pull-dn?
		byte channel = getChannel(pin);
		if (!channel) return false;
		const byte shift = (channel - 1) << 4;
		// H7 output buffer disable lives in MCR.MODE1_1, not CR.BOFFx.
		self[MCR].setof(_DAC_MCR_POS_MODEx + shift + 1, true);
		enAble(channel, true);//aka __HAL_DAC_ENABLE
		return true;
	}

	// aka HAL_DAC_DeInit
	bool DAC_t::canMode() {
		using namespace DACReg;
		self[CR] = 0;
		self[SWTRIGR] = 0;
		self[DHR12R1] = 0;
		self[DHR12L1] = 0;
		self[DHR8R1] = 0;
		self[DHR12R2] = 0;
		self[DHR12L2] = 0;
		self[DHR8R2] = 0;
		self[DHR12RD] = 0;
		self[DHR12LD] = 0;
		self[DHR8RD] = 0;
		self[CCR] = 0;
		self[MCR] = 0;
		self[SHSR1] = 0;
		self[SHSR2] = 0;
		self[SHHR] = 0;
		self[SHRR] = 0;
		bind = 0;
		enClock(false);
		return true;
	}

	// aka HAL_DAC_ConfigChannel trigger part
	bool DAC_t::setTrigger(byte channel, DACTrigger::DACTrigger trigger) {
		using namespace DACReg;
		if (channel != 1 && channel != 2) return false;
		const byte shift = (channel - 1) << 4;
		self[CR].maset(_DAC_CR_POS_TENx + shift, 5, 0);// clear TENx + TSELx
		self[CR] |= (stduint)trigger << shift;
		return true;
	}

	// aka HAL_DAC_ConfigChannel wave part;
	// also HAL_DACEx_TriangleWaveGenerate (wave=Triangle) / HAL_DACEx_NoiseWaveGenerate (wave=Noise)
	bool DAC_t::setWave(byte channel, DACWave::DACWave wave, byte amplitude) {
		using namespace DACReg;
		if (channel != 1 && channel != 2) return false;
		const byte shift = (channel - 1) << 4;
		self[CR].maset(_DAC_CR_POS_WAVEx + shift, 2, 0);// clear WAVEx
		self[CR] |= (stduint)wave << shift;
		self[CR].maset(_DAC_CR_POS_MAMPx + shift, 4, amplitude);
		return true;
	}

	// aka HAL_DAC_ConfigChannel output buffer part (MCR.MODE1_1)
	bool DAC_t::setBuffer(byte channel, bool ena) {
		using namespace DACReg;
		if (channel != 1 && channel != 2) return false;
		const byte shift = (channel - 1) << 4;
		self[MCR].setof(_DAC_MCR_POS_MODEx + shift + 1, !ena);
		return true;
	}

	// aka HAL_DAC_Start_DMA
	bool DAC_t::enDMA(GPIO_Pin& pin, const DMAStream& stream, pureptr_t data, uint32_t leng, uint32_t align) {
		using namespace DACReg;
		enClock();
		pin.setMode(GPIOMode::IN_Analog);
		byte channel = getChannel(pin);
		if (!channel) return false;
		const byte shift = (channel - 1) << 4;
		stream.getParent().enClock();// DMA + DMAMUX1 clock
		// DMA request routing via DMAMUX1: DMA_REQUEST_DAC1_CH1=67, DMA_REQUEST_DAC1_CH2=68
		stream.setRequest(67 + (channel - 1));
		byte dhr_reg = 2 + (channel - 1) * 3;
		switch (align) {
		case _DAC_ALIGN_12B_R: break;
		case _DAC_ALIGN_12B_L: dhr_reg += 1; break;
		case _DAC_ALIGN_8B_R:  dhr_reg += 2; break;
		default: return false;
		}
		pureptr_t dhr_addr = (pureptr_t)(baseaddr + ((stduint)dhr_reg << 2));
		self[CR].setof(_DAC_CR_POS_DMAENx + shift, true);// DAC_CR_DMAENx
		if (!stream.Transfer(dhr_addr, data, leng, IOMethod::Rupt))
			return false;
		self.bind = (pureptr_t)&stream;
		enAble(channel, true);// aka __HAL_DAC_ENABLE
		return true;
	}

	// aka HAL_DACEx_DualSetValue; default 12-bit right-aligned dual write
	bool DAC_t::setOutputs(uint32 val1, uint32 val2, uint32 align) {
		using namespace DACReg;
		uint32 data;
		if (align == _DAC_ALIGN_8B_R)
			data = (val1 << 8) | val2;
		else
			data = (val1 << 16) | val2;
		DACRegType reg;
		switch (align) {
		case _DAC_ALIGN_12B_R: reg = DHR12RD; break;
		case _DAC_ALIGN_12B_L: reg = DHR12LD; break;
		case _DAC_ALIGN_8B_R:  reg = DHR8RD;  break;
		default: return false;
		}
		self[reg] = data;
		return true;
	}

	// aka HAL_DAC_Start software-trigger part
	bool DAC_t::swTrigger(byte channel) {
		using namespace DACReg;
		if (channel != 1 && channel != 2) return false;
		const byte shift = (channel - 1) << 4;
		// software trigger requires TENx=1 and TSELx=0
		if (self[CR].masof(_DAC_CR_POS_TENx + shift, 1) != 1) return false;
		if (self[CR].masof(_DAC_CR_POS_TSELx + shift, 4) != 0) return false;
		self[SWTRIGR].setof(shift, true);// SWTRIG1/SWTRIG2
		return true;
	}

	// aka HAL_DAC_SetValue
	bool DAC_t::setValue(byte channel, uint32 data, uint32 align) {
		using namespace DACReg;
		if (channel != 1 && channel != 2) return false;
		DACRegType reg;
		switch (align) {
		case _DAC_ALIGN_12B_R: reg = (channel == 1) ? DHR12R1 : DHR12R2; break;
		case _DAC_ALIGN_12B_L: reg = (channel == 1) ? DHR12L1 : DHR12L2; break;
		case _DAC_ALIGN_8B_R:  reg = (channel == 1) ? DHR8R1 : DHR8R2; break;
		default: return false;
		}
		self[reg] = data;
		return true;
	}

	// aka HAL_DAC_ConfigChannel sample-and-hold part
	bool DAC_t::setSampleHold(byte channel, uint32 sampleTime, uint32 holdTime, uint32 refreshTime) {
		using namespace DACReg;
		if (channel != 1 && channel != 2) return false;
		const byte shift = (channel - 1) << 4;
		uint64 tickstart = SysTick::getTick();
		// SHSR1/SHSR2 writable only when BWSTx == 0
		while (self[SR].bitof(channel == 1 ? _DAC_SR_POS_BWST1 : _DAC_SR_POS_BWST2)) {
			if ((SysTick::getTick() - tickstart) > 1) return false;// 1 ms timeout
		}
		SysDelay_ms(1);
		self[channel == 1 ? SHSR1 : SHSR2] = sampleTime;
		self[SHHR].maset(shift, 10, holdTime);
		self[SHRR].maset(shift, 8, refreshTime);
		self[MCR].setof(_DAC_MCR_POS_MODEx + shift + 2, true);// MODE1_2 sample-and-hold
		return true;
	}

	// aka HAL_DAC_ConfigChannel user-trimming part
	bool DAC_t::setUserTrimming(byte channel, uint32 trimValue) {
		using namespace DACReg;
		if (channel != 1 && channel != 2) return false;
		const byte shift = (channel - 1) << 4;
		self[CCR].maset(shift, 5, trimValue);// OTRIM1[4:0] / OTRIM2[20:16]
		return true;
	}

	// aka HAL_DAC_ConfigChannel chip-connect part (MCR.MODE1_0)
	bool DAC_t::setChipConnect(byte channel, bool ena) {
		using namespace DACReg;
		if (channel != 1 && channel != 2) return false;
		const byte shift = (channel - 1) << 4;
		self[MCR].setof(_DAC_MCR_POS_MODEx + shift + 0, ena);
		return true;
	}

	// aka HAL_DAC_ConfigChannel CENx clear (normal operating mode)
	bool DAC_t::setCalibrationEnable(byte channel, bool ena) {
		using namespace DACReg;
		if (channel != 1 && channel != 2) return false;
		const byte shift = (channel - 1) << 4;
		self[CR].setof(_DAC_CR_POS_CENx + shift, ena);
		return true;
	}

	// aka HAL_DACEx_DualGetValue
	void DAC_t::getOutputs(uint16& val1, uint16& val2) {
		using namespace DACReg;
		val1 = self[DOR1];
		val2 = self[DOR2];
	}

	// aka HAL_DACEx_SelfCalibrate
	uint32 DAC_t::selfCalibrate(byte channel) {
		using namespace DACReg;
		if (channel != 1 && channel != 2) return 0xFFFFFFFF;
		const byte shift = (channel - 1) << 4;
		const byte cal_flag = (channel == 1) ? _DAC_SR_POS_CAL_FLAG1 : _DAC_SR_POS_CAL_FLAG2;
		uint32 oldmode = self[MCR].mask(shift, 3);// save MODEx
		self[CR].setof(_DAC_CR_POS_ENx + shift, false);// disable channel
		self[MCR].maset(shift, 3, 0);// normal mode for calibration
		self[channel == 1 ? DHR12R1 : DHR12R2] = 0x0800;// middle value
		self[CR].setof(_DAC_CR_POS_CENx + shift, true);// enable calibration
		uint32 trimmingvalue = 16;
		uint32 delta = 8;
		while (delta != 0) {
			self[CCR].maset(shift, 5, trimmingvalue);
			SysDelay_ms(1);// tOFFTRIMmax
			if (self[SR].bitof(cal_flag))
				trimmingvalue -= delta;
			else
				trimmingvalue += delta;
			delta >>= 1;
		}
		self[CCR].maset(shift, 5, trimmingvalue);
		SysDelay_ms(1);
		if (!self[SR].bitof(cal_flag)) {
			trimmingvalue++;
			self[CCR].maset(shift, 5, trimmingvalue);
		}
		self[CR].setof(_DAC_CR_POS_CENx + shift, false);// disable calibration
		self[MCR].maset(shift, 3, oldmode >> shift);// restore MODEx
		return trimmingvalue;
	}

	// aka HAL_DACEx_GetTrimOffset
	uint32 DAC_t::getTrimOffset(byte channel) {
		using namespace DACReg;
		if (channel != 1 && channel != 2) return 0xFFFFFFFF;
		const byte shift = (channel - 1) << 4;
		return self[CCR].masof(shift, 5);
	}

	#endif

#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x)
	DAC_t DAC(0x40007400);
#endif
	
}
