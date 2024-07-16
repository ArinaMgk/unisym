// UTF-8 CPP-ISO11 TAB4 CRLF
// Docutitle: (Device) Analog-Digital Converter
// Codifiers: @dosconio: 20240709~;
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

#include "../../inc/cpp/Device/ADC"
#include "../../inc/cpp/Device/RCC/RCCAddress"
#include "../../inc/cpp/Device/RCC/RCCClock"

namespace uni {
#if 0

#elif defined(_MCU_STM32F10x)
	#define _ADC_Counts 3

	static stduint RCC_ADCx_addrs[_ADC_Counts] = // 0.._ADC_Counts
	{
		_RCC_APB2ENR_ADDR,0,0
	};
	static stduint RCC_ADCx_bitpos[_ADC_Counts] = // 0.._ADC_Counts
	{
		_RCC_APB2ENR_POSI_ENCLK_ADC1,0,0
	};

	bool ADC_t::enClock(bool ena, byte divby) {
		Reference(RCC_ADCx_addrs[ADC_ID - 1]).setof(RCC_ADCx_bitpos[ADC_ID - 1], ena);
		if (ena != Reference(RCC_ADCx_addrs[ADC_ID - 1]).bitof(RCC_ADCx_bitpos[ADC_ID - 1])) return false;
		// Periph Clock Config
		RCCPeriphClock::ConfigADC(divby);
		return true;
	}

	//
	bool ADC_t::setMode(stduint numsof_conv, bool align_left, stduint numsof_disc, bool trigger_ext_posedge, byte divby) {
		_numsof_conv = numsof_conv; _align_left = align_left; _numsof_disc = numsof_disc; _trigger_ext_posedge = trigger_ext_posedge;
		enClock(true, divby);
		/* dosconio design yo 20240711 (TEMP)
		 * DataAlign <=> align_left
		 * ScanConvMode = !!numsof_conv // regular conversion
		 * NbrOfConversion = numsof_conv ? numsof_conv : none
		 * DiscontinuousConvMode = !!numsof_disc;
		 * ContinuousConvMode = !numsof_disc;
		 * NbrOfDiscConversion = numsof_disc ? numsof_disc : none
		 * ExternalTrigConv <=> trigger_ext_posedge
		*/
		if (numsof_conv > 16 || numsof_disc > 8) return false;
		self.enAble(false);// Stop potential conversion on going, on regular and injected groups
		for0(i, (/*SystemCoreClock=>us*/ 72)) i = i;
		self[ADCReg::CR2].setof(_ADC_CR2_POS_ALIGN, align_left);
		//aka ADC_CFGR_EXTSEL
		{
			uint32 tmp;
			if (!trigger_ext_posedge && getID() < 3) {
				tmp = 0x000E0000;
				self[ADCReg::CR2] |= tmp;
			}
			else {
				_TODO
			}
		}
		self[ADCReg::CR2].setof(_ADC_CR2_POS_CONT, !numsof_disc);
		//
		self[ADCReg::CR1].setof(_ADC_CR1_POS_SCAN, !!numsof_conv);
		self[ADCReg::CR1].setof(_ADC_CR1_POS_DISCEN, !!numsof_disc);
		if (numsof_disc--) {
			self[ADCReg::CR1] = (self[ADCReg::CR1] & 0xFFFF8FFF) | (numsof_disc << _ADC_CR1_POS_DISCNUM);
		}
		if (numsof_conv--) {
			self[ADCReg::SQR1] = (self[ADCReg::SQR1] & 0xFFF0FFFF) | (numsof_conv << _ADC_SQR1_POS_L);
		}
		return true;
	}

	bool ADC_t::setChannel(GPIO_Pin& pin, byte rank, ADCSample::ADCSample sample) {
		if (rank >= 16) return false;
		byte grp = rank / 6;// 0, 1, 2
		byte idx = rank % 6;// 0 .. 6
		byte chan = getChannelNumber(pin);
		if (chan == 0xFF) return false;
		const static ADCReg::ADCRegType sqr_reg[3] = {
			ADCReg::SQR3, ADCReg::SQR2, ADCReg::SQR1
		};
		const static ADCReg::ADCRegType smpr_reg[2] = {
			ADCReg::SMPR2, ADCReg::SMPR1
		};
		Reference reg = self[sqr_reg[grp]];
		Reference sam = self[smpr_reg[chan / 10]];
		reg = (reg & ~((stduint)0x1F << (idx * 5))) |
			(chan << (idx * 5));// Regular sequence configuration
		chan %= 10;
		sam = (sam & ~((stduint)0x7 << (chan * 3))) |
			((stduint)sample << (chan * 3));// Channel sampling time configuration
		//{TODO} Support ADC_CHANNEL_TEMPSENSOR and ADC_CHANNEL_VREFINT here for respective devices, not for this GPIO pins version.
		return true;
	}

	void ADC_t::setInterrupt(Handler_t fn) {
		FUNC_ADCx[getID()] = fn;
	}
	
	static Request_t ADCx_Request_list[4] = {
		(Request_t)0, IRQ_ADC1_2, IRQ_ADC1_2,
		IRQ_ADC3
	};
	void ADC_t::setInterruptPriority(byte preempt, byte sub_priority) {
		NVIC.setPriority(ADCx_Request_list[ADC_ID], preempt, sub_priority);
	}

	void ADC_t::enInterrupt(bool enable, bool trigger_ext_posedge) {
		// C-with HAL_ADC_Start_IT
		if (enable) {
			if (!self.enAble(true)) return;
			for0(i, (/*SystemCoreClock=>us*/ 72)) i = i;
			byte id = self.getID();
			bool ADC_NONMULTIMODE_OR_MULTIMODEMASTER = (id != 2 || !(ADC1[ADCReg::CR1] & (0xFU << _ADC_CR1_POS_DUALMOD)));
			//{useless} ... inner def ... 
			self[ADCReg::SR] = ~(1 << _ADC_SR_POS_EOC); // Clear regular group conversion flag and overrun flag. To ensure of no unknown state from potential previous ADC operations
			self[ADCReg::CR1].setof(_ADC_CR1_POS_EOCIE, true);
			self[ADCReg::CR2] |= (ADC_NONMULTIMODE_OR_MULTIMODEMASTER && (ADC1.isTriggeredBySoftware())) ?
				((1 << _ADC_CR2_POS_SWStart) | (1 << _ADC_CR2_POS_ExtTrig)) : ((1 << _ADC_CR2_POS_ExtTrig));
			//if (ADC_NONMULTIMODE_OR_MULTIMODEMASTER && isTriggeredBySoftware())
			//	self[ADCReg::CR2].setof(_ADC_CR2_POS_SWStart, true);
			//self[ADCReg::CR2].setof(_ADC_CR2_POS_ExtTrig, true);
			//
			NVIC.setAble(ADCx_Request_list[self.ADC_ID]);
		}
		else _TODO;
	}

	bool ADC_t::setDMA(bool periph_inc, bool memory_inc, stduint periph_align, stduint memory_align, bool circular_mode, byte priority) {
		if (ADC_ID == 1) {
			DMA1.enClock();
			DMA1.setMode(1/*channel for ADC1*/, true, false, periph_inc, memory_inc, periph_align, memory_align, circular_mode, priority);// from_periph to__memory
			self.bind = (pureptr_t)&DMA1;
			DMA1.bind = (pureptr_t)this;
		}
		else return false;
		return this;
	}

	//aka HAL_ADC_Start_DMA
	bool ADC_t::enDMA(pureptr_t addr, stduint leng) {
		bool ena = addr && !!leng;
		if (!bind) {
			return false;//{TODO} combinate this and enInterrupt into setMode or enAble
		}
		else if (ADC_ID == 1 && bind == &DMA1) {
			const byte channel = 1;
			if (!channel) return false;
			if (!ena) _TODO return false; // {undef}
			DMA_t & crt = DMA1;
			bool ADC_MULTIMODE_IS_ENABLE = (ADC_ID == 1 || ADC_ID == 2) ? ADC1[ADCReg::CR1] & (0xFU << _ADC_CR1_POS_DUALMOD) : false; // only check for F103x6 ||  F103xB ||  F105xC ||  F107xC ||  F103xE ||  F103xG
			if (ADC_MULTIMODE_IS_ENABLE) return false; // why?
			if (!enAble(true)) return false;
			//{PASS} set XferCpltCallback = ADC_DMAConvCplt;
			//{PASS} set XferHalfCpltCallback = ADC_DMAHalfConvCplt;
			//{PASS} set XferErrorCallback = ADC_DMAError;
			self[ADCReg::SR].rstof(_ADC_SR_POS_EOC);// Clear regular group conversion flag and overrun flag
			self[ADCReg::CR2].setof(_ADC_CR2_POS_DMA, true);
			crt.StartInterrupt(addr, (pureptr_t)&self[ADCReg::DR], leng, channel);
			bool ADC_IS_SOFTWARE_START_REGULAR = (self[ADCReg::CR2] & (7 << _ADC_CR2_POS_EXTSEL)) == 0x000E0000;
			self[ADCReg::CR2] |= (ADC_IS_SOFTWARE_START_REGULAR) ?
				(1 << _ADC_CR2_POS_SWStart | 1 << _ADC_CR2_POS_ExtTrig) :
				(1 << _ADC_CR2_POS_ExtTrig);
		}
		else return false;
		return true;
	}

	//

	ADC_t \
		ADCr(0x00000000, 0), // error
		ADC1(0x40012400, 1),
		ADC2(0x40012800, 2),
		ADC3(0x40013C00, 3);

#endif
}
