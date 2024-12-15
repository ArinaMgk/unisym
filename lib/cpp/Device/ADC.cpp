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

#include "../../../inc/cpp/Device/ADC"
#include "../../../inc/cpp/Device/RCC/RCC"
#include "../../../inc/c/driver/ADConverter/Register-ADC.h"
#include "../../../inc/cpp/MCU/_ADDRESS/ADDR-STM32.h"

namespace uni {

#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x)

	ADC_t ADCr(0), // error
		ADC1(1), ADC2(2), ADC3(3);

#endif	
#if 0

#elif defined(_MCU_STM32F1x)

	static const uint32 _REFADDR_ADC[] = { nil,
		0x40012400, 0x40012800, 0x40013C00
	};

	static stduint RCC_ADCx_addrs[3] = // 0.._ADC_Counts
	{
		_RCC_APB2ENR_ADDR, _RCC_APB2ENR_ADDR, _RCC_APB2ENR_ADDR
	};
	static byte RCC_ADCx_bitpos[3] = // 0.._ADC_Counts
	{
		_RCC_APB2ENR_POSI_ENCLK_ADC1,
		_RCC_APB2ENR_POSI_ENCLK_ADC2,
		_RCC_APB2ENR_POSI_ENCLK_ADC3
	};

	static Request_t ADCx_Request_list[4] = {
		Request_None, IRQ_ADC, IRQ_ADC, IRQ_ADC3
	};

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
		reg = (reg & ~(_IMM(0x1F) << (idx * 5))) |
			(chan << (idx * 5));// Regular sequence configuration
		chan %= 10;
		sam = (sam & ~(_IMM(0x7) << (chan * 3))) |
			(_IMM(sample) << (chan * 3));// Channel sampling time configuration
		//{TODO} Support ADC_CHANNEL_TEMPSENSOR and ADC_CHANNEL_VREFINT here for respective devices, not for this GPIO pins version.
		return true;
	}

	void ADC_t::enInterrupt(bool enable) const {
		bool trigger_ext = false;
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
		return true;
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

#elif defined(_MCU_STM32F4x)
	static const uint32 _REFADDR_ADC[] = { nil,
		0x40012000, 0x40012100, 0x40012200
	};// len 0x100 for each
	Reference ADC_t::Reflect(ADCReg::ADCComType idx) {
		return Reference(0x40012300 + _IMMx4(idx));
	}

	static stduint RCC_ADCx_addrs[3] = { // 0.._ADC_Counts
		_IMM(& RCC[RCCReg::APB2ENR]),
		_IMM(& RCC[RCCReg::APB2ENR]),
		_IMM(& RCC[RCCReg::APB2ENR])
	};
	static byte RCC_ADCx_bitpos[3] = { // 0.._ADC_Counts
		_RCC_APB2ENR_POSI_ENCLK_ADC1,
		_RCC_APB2ENR_POSI_ENCLK_ADC2,
		_RCC_APB2ENR_POSI_ENCLK_ADC3
	};

	static Request_t ADCx_Request_list[4] = {
		Request_None, IRQ_ADC, IRQ_ADC, IRQ_ADC
	};

	static const byte CHANn_GPIOFx[] = {
		0xFF, 0xFF, 0xFF,
		9 , // pF3
		14, // pF4
		15, // pF5
		4 , // pF6
		5 , // pF7
		6 , // pF8
		7 , // pF9
		8 , // pF10
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF
	};
	
	bool ADC_t::setChannel(GPIO_Pin& pin, byte rank, ADCSample::ADCSample sample) {
		using namespace ::uni::ADCReg;
		if (rank >= 16) return false;
		byte chan = getChannelNumber(pin);
		if (chan == 0xFF) return false;
		pin.setMode(GPIOMode::IN_Analog);
		const ADCRegType smpr = chan > 9 ? SMPR1 : SMPR2;
		self[smpr].maset(3 * (chan % 10), 3, _IMM(sample));// 9 and 10
		const ADCRegType sqr = ADCRegType(_IMM(SQR3) - rank / 6);
		self[sqr].maset(5 * (rank % 6), 5, chan);
		//: CASE ADC1 Channel_18 is selected for VBAT Channel ennable VBATE
		if (getID() == 1 && chan == 18/*VBAT*/) {
			// Because here is GPIO input, no this case.
			// Disable the TEMPSENSOR channel in case of using board with multiplixed ADC_CHANNEL_VBAT & ADC_CHANNEL_TEMPSENSOR
			//{TODO} if ((uint16_t)ADC_CHANNEL_TEMPSENSOR == (uint16_t)ADC_CHANNEL_VBAT)
			if (0) ADC[CCR] = ADC[CCR];
		}
		//: If ADC1 Channel_16 or Channel_18 is selected for Temperature sensor or  Channel_17 is selected for VREFINT enable TSVREFE
		if (getID() == 1 && false /*((sConfig->Channel == ADC_CHANNEL_TEMPSENSOR) || (sConfig->Channel == ADC_CHANNEL_VREFINT))*/) {
			//{TODO}
		}
		return true;
	}

	static void func_sub_1(const ADC_t& sel) {
		using namespace ADCReg;
		//{TEMP} assume ADC2 ADC3 both exist
		//: If no ADC2&3, do not judge the if :
		if (1 == sel.getID() ||
			(2 == sel.getID() && ((ADC[CCR] & 0x0000001F) < 0x01)) ||
			(3 == sel.getID() && ((ADC[CCR] & 0x0000001F) < 0x10)))
		{
			// if no external trigger present enable software conversion of regular channels
			if (!(sel[CR2] & 0x30000000))// ADC_CR2_EXTEN
				sel[CR2] |= 0x40000000;// ADC_CR2_SWSTART
		}
	}
	void ADC_t::enInterrupt(bool enable) const {
		using namespace ADCReg;
		if (enable) {
			if (!self[CR2].bitof(0)) { // ADON
				self[CR2].setof(0);
				for0(i, 3 * SystemCoreClock / 1000000U) {}
				while (!self[CR2].bitof(0));
			}
			
			// Clear regular group conversion flag and overrun flag To ensure of no unknown state from potential previous ADC operations
			self[SR] &= ~_IMM(0x22);// Clear EOC and OVR flags
			self[CR1].setof(5);// EOCIE
			self[CR1].setof(26);// OVRIE
			if (ADC[CCR] & 0x0000001F) { // ADC_CCR_MULTI
				// ADC1 and  no external trigger present enable software conversion of regular channels
				if (getID() == 1 && !(self[CR2] & 0x30000000))// ADC_CR2_EXTEN
					self[CR2] |= 0x40000000;// ADC_CR2_SWSTART
			}
			else func_sub_1(self);
			NVIC.setAble(ADCx_Request_list[self.ADC_ID]);
		}
	}
#elif defined(_MPU_STM32MP13)
	static const uint32 _REFADDR_ADC[] = { nil,
		AHB2_PERIPH_BASE + 0x3000,
		AHB2_PERIPH_BASE + 0x4000
	};
	// ADC_Common_TypeDef of each is base plus 0x0300

	
#endif

#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x)

	stduint ADC_t::getBaseAddr() const {
		return _REFADDR_ADC[self.ADC_ID];
	} //{TEMP} no-assert-opt

	void ADC_t::setInterrupt(Handler_t f) const {
		FUNC_ADCx[getID()] = f;
	}

	void ADC_t::setInterruptPriority(byte preempt, byte sub_priority) const {
		NVIC.setPriority(ADCx_Request_list[ADC_ID], preempt, sub_priority);
	}

	bool ADC_t::enClock(bool ena, byte divby) {
		using namespace ADCReg;
		Reference(RCC_ADCx_addrs[ADC_ID - 1]).setof(RCC_ADCx_bitpos[ADC_ID - 1], ena);
	#if defined(_MCU_STM32F1x)
		if (ena) RCCPeriphClock::ConfigADC(divby);// Periph Clock Config
	#elif defined(_MCU_STM32F4x)
		// assert () in 2, 4, 6, 8
		ADC_t::Reflect(CCR).maset(16, 2, divby >> 1);// set ADCPRE half literal
	#endif
		return ena == Reference(RCC_ADCx_addrs[ADC_ID - 1]).bitof(RCC_ADCx_bitpos[ADC_ID - 1]);
	}

	bool ADC_t::enAble(bool ena) const {
		using namespace ADCReg;
		self[CR2].setof(_ADC_CR2_POS_ADON, ena);
		return self[CR2].bitof(_ADC_CR2_POS_ADON) == ena;
	}

	byte ADC_t::getChannelNumber(GPIO_Pin& pin) {
		if (getID() == 3) {
			if (pin.getID() < 4) {
				if (&pin.getParent() == &GPIO['A'])
					return pin.getID();
				else if (&pin.getParent() == &GPIO['C'])
					return pin.getID() + 10;
			#if defined(_MCU_STM32F4x)
				else if (&pin.getParent() == &GPIO['F'])
					return CHANn_GPIOFx[pin.getID()];
			#endif
				else return 0xFF;
			}
			else return 0xFF;
		}
		else if (getID() < 3) {
			if (&pin.getParent() == &GPIO['A'])
				return pin.getID() < 8 ? pin.getID() : 0xFF;
			else if (&pin.getParent() == &GPIO['B'])
				return pin.getID() < 2 ? (8 + pin.getID()) : 0xFF;
			else if (&pin.getParent() == &GPIO['C'])
				return pin.getID() < 6 ? (10 + pin.getID()) : 0xFF;
			else return 0xFF;
		}
		else return 0xFF;
	}

	bool ADC_t::setMode(stduint numsof_conv, bool align_left, stduint numsof_disc, stduint trigger_ext, byte divby) {
		using namespace ADCReg;
	#if defined(_MCU_STM32F4x)
		_TEMP byte bitsband = 0;// ADC_RESOLUTION_12B
		_TEMP bool EOCSelection = false;
		_TEMP bool DMAContinuousRequests = true;//{TODO: Split out} for single ADC
	#endif
		enClock(true, divby);
		if (numsof_conv > 16 || numsof_disc > 8) return false;//? for F1 or F1&4
		self.enAble(false);// Stop potential conversion on going, on regular and injected groups
		for0(i, SystemCoreClock / 1000000) i = i;
		self[CR2].setof(_ADC_CR2_POS_ALIGN, align_left);
	#if defined(_MCU_STM32F1x)
		/* aka ADC_CFGR_EXTSEL */ {
			if (!trigger_ext && getID() < 3) {
				self[CR2] |= 0x000E0000;//{} magic
			}
			else _TODO;
		}
	#elif defined(_MCU_STM32F4x)
		self[CR2].setof(9, DMAContinuousRequests);// DDS
		self[CR2].setof(10, EOCSelection);// EOCS
		self[CR2].maset(24, 6, nil);// EXTSEL and EXTEN
		if (trigger_ext != /*ADC_SOFTWARE_START*/ 0x0F000001) {
			(void)_TEMP _TODO "Assume ExternalTrigConvEdge = 0 ADC_EXTERNALTRIGCONVEDGE_NONE;";
			self[CR2] |= trigger_ext;
		}
	#endif
		self[CR2].setof(_ADC_CR2_POS_CONT, !numsof_disc);
		Letvar(blk_cr1, volatile BLK_CR1*, &self[CR1]); {
			blk_cr1->SCAN = !!numsof_conv;
			blk_cr1->DISCEN = !!numsof_disc;
		#if defined(_MCU_STM32F4x)
			blk_cr1->RES = bitsband;
		#endif
			if (numsof_disc--)
				blk_cr1->DISCNUM = numsof_disc;
		}
		if (numsof_conv--) {
			self[SQR1].maset(_ADC_SQR1_POS_L, 4, numsof_conv);
		}
		return true;
	}

	stduint ADC_t::getConfigLastDiscCount() {
		using namespace ADCReg;
		Letvar(blk_cr1, volatile BLK_CR1*, &self[CR1]); {
			return blk_cr1->DISCNUM + 1;//{?} whether should plus 1
		}
	}

	//

	//{TODO} unioned with other virtuals into single byte
	ADC_t& ADC_Global::operator[](byte id) {
		extern ADC_t ADCr;
		const static ADC_t* ADC_LST[] = {
			&ADC1,&ADC2,&ADC3
		};
		if (!Ranglin(id, 1, numsof(ADC_LST)))
			return ADCr;
		return *(ADC_t*)ADC_LST[id - 1];
	}

#endif	
}
