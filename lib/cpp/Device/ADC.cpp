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

#if defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)

	#if defined(_MCU_STM32H7x)
	ADC_t ADCr(0), ADC1(1), ADC2(2), ADC3(3);
	static const uint32 _REFADDR_ADC[] = { nil,
		0x40022000, 0x40022100, 0x58026000
	};
	static Request_t ADCx_Request_list[4] = {
		Request_None, IRQ_ADC, IRQ_ADC, IRQ_ADC3
	};
	#elif defined(_MPU_STM32MP13)
	ADC_t ADCr(0), ADC1(1), ADC2(2);
	static const uint32 _REFADDR_ADC[] = { nil,
		AHB2_PERIPH_BASE + 0x3000,// ADC1
		AHB2_PERIPH_BASE + 0x4000 // ADC2
	};
	static Request_t ADCx_Request_list[3] = {
		Request_None, IRQ_ADC1, IRQ_ADC2
	};
	#endif

	stduint ADC_t::getBaseAddr() const {
		return _REFADDR_ADC[self.ADC_ID];
	}

	void ADC_t::setInterrupt(Handler_t f) const {
		FUNC_ADCx[getID()] = f;
	}

	void ADC_t::setInterruptPriority(byte preempt, byte sub_priority) const {
	#if defined(_MPU_STM32MP13)
		(void)sub_priority;
		GIC.setPriority(ADCx_Request_list[ADC_ID], preempt);
	#else
		NVIC.setPriority(ADCx_Request_list[ADC_ID], preempt, sub_priority);
	#endif
	}

	bool ADC_t::enClock(bool ena, byte presc, ADCClkSrc src) {
		#if defined(_MCU_STM32H7x)
		if (ADC_ID == 3) {
			// RCC_AHB4ENR (D3): ADC3EN
			Reference(_RCC_AHB4ENR_ADDR).setof(_RCC_AHB4ENR_POSI_ENCLK_ADC3, ena);
			if (ena != Reference(_RCC_AHB4ENR_ADDR).bitof(_RCC_AHB4ENR_POSI_ENCLK_ADC3)) return false;
		} else if (ADC_ID == 1 || ADC_ID == 2) {
			// RCC_AHB1ENR (D2): ADC12EN
			Reference(_RCC_AHB1ENR_ADDR).setof(_RCC_AHB1ENR_POSI_ENCLK_ADC12, ena);
			if (ena != Reference(_RCC_AHB1ENR_ADDR).bitof(_RCC_AHB1ENR_POSI_ENCLK_ADC12)) return false;
		} else return false;
		if (ena) {
			// Select the ADC kernel clock source (ADCSEL, RCC_D3CCIPR[17:16]):
			// 0=pll2_p, 1=pll3_r, 2=per_ck.
			switch (src) {
			case ADCClkSrc::CKPER:// per_ck defaults to HSI; make sure it is up
				RCC.HSI.setMode(true, 0, 0);// ena, divr(HSIDIV=0 => /1), calibration
				RCC.setPeriphClock(PeriphClock::ADC, ClockSource::CKPER);
				break;
			case ADCClkSrc::PLL2:// pll2_p_ck (PLL2 must be configured by caller)
				RCC.setPeriphClock(PeriphClock::ADC, ClockSource::PLL2);
				break;
			case ADCClkSrc::PLL3:// pll3_r_ck (PLL3 must be configured by caller)
				RCC.setPeriphClock(PeriphClock::ADC, ClockSource::PLL3);
				break;
			}
			// CCR clock mode: asynchronous (CKMODE=0) — the ADC conversion clock is
			// adc_ker_ck selected above (ADCSEL), divided by PRESC. This is the
			// migrated configuration; keep ADCCLK in spec (<=36MHz).
			Common(ADCCom::CCR).maset(_ADC_CCR_POS_CKMODE, 2, 0);
			Common(ADCCom::CCR).maset(_ADC_CCR_POS_PRESC, 4, presc);
		}
		return true;
		#elif defined(_MPU_STM32MP13)
		using namespace RCCReg;
		(void)src;//{TODO} MP13 ADC kernel clock source selection
		byte bit = ADC_ID == 1 ? 5 : ADC_ID == 2 ? 6 : 0;// ADC1EN/ADC2EN in RCC_MP_AHB2ENSETR
		if (!bit) return false;
		RCC[ena ? MP_AHB2ENSETR : MP_AHB2ENCLRR] = _IMM1S(bit);
		if (ena != RCC[MP_AHB2ENSETR].bitof(bit)) return false;
		if (ena) {
			// CCR: CKMODE=asynchronous(0), PRESC=presc
			Common(ADCCom::CCR).maset(_ADC_CCR_POS_CKMODE, 2, 0);
			Common(ADCCom::CCR).maset(_ADC_CCR_POS_PRESC, 4, presc);
		}
		return true;
		#endif
	}

	bool ADC_t::enAble(bool ena) const {
		if (ena) {
			self[ADCReg::CR].setof(_ADC_CR_POS_ADEN, true);
			stduint timeout = 0xFFFF;
			while (!self[ADCReg::ISR].bitof(_ADC_ISR_POS_ADRD) && timeout--) {}
			return self[ADCReg::ISR].bitof(_ADC_ISR_POS_ADRD);
		} else {
			self[ADCReg::CR].setof(_ADC_CR_POS_ADDIS, true);
			return true;
		}
	}

	bool ADC_t::setMode(ADCRes res, stduint numsof_conv, stduint trigger_ext, bool cont) {
		enClock(true);
		self.enAble(false);
	#if defined(_MCU_STM32H7x)
		Reference cfgr = self[ADCReg::CFGR];
		self[ADCReg::CR].setof(_ADC_CR_POS_BOOST, true);// Boost mode (HAL BoostMode=ENABLE)
	#elif defined(_MPU_STM32MP13)
		Reference cfgr = self[ADCReg::CFGR1];
	#endif
		cfgr.maset(_ADC_CFGR_POS_RES, 3, (stduint)res);
		if (trigger_ext) {
			cfgr.maset(_ADC_CFGR_POS_EXTSEL, 5, trigger_ext);
			cfgr.maset(_ADC_CFGR_POS_EXTEN, 2, 1);// rising edge
		} else {
			cfgr.maset(_ADC_CFGR_POS_EXTSEL, 5, 0);
			cfgr.maset(_ADC_CFGR_POS_EXTEN, 2, 0);
		}
		cfgr.setof(_ADC_CFGR_POS_OVRMOD, true);// Overrun overwrite (HAL Overrun=OVR_DATA_OVERWRITTEN)
		cfgr.setof(_ADC_CFGR_POS_CONT, cont);
		self[ADCReg::SQR1].maset(0, 4, numsof_conv ? (numsof_conv - 1) : 0);
		return true;
	}

	/*
	 * [LESSON] H7/MP13 ADC channel number != GPIO pin number.
	 *
	 * On F1/F4 the ADC channel in SQR happens to equal the pin number (PA0=0,
	 * PA5=5, ...), so getChannelNumber() could just return pin.getID().  This
	 * is NOT true on H7: the channel number written to SQR/PCSEL/SMPR comes
	 * from the datasheet's ADC channel table, not from the pin number.
	 *
	 * The original port blindly reused the F4-style linear map and therefore
	 * sampled the WRONG channel (PA5 was mapped to channel 5, but on H743 PA5
	 * is ADC1 channel 19).  Symptom: constant low reading that never followed
	 * the input voltage.  Always look up the device's ADC channel table; do
	 * not assume pin number == channel number.
	 */
	byte ADC_t::getChannelNumber(GPIO_Pin& pin) {
		#if defined(_MCU_STM32H7x)
		// H743/H750 ADC1/2 single-ended (INP) channel table, per ST datasheet.
		// NOTE: ADC channel number != GPIO pin number (unlike F1/F4).
		// ADC1/ADC2 share PA/PB/PC mapping; they differ only on PF:
		//   INP2 = PF11 (ADC1) / PF13 (ADC2);  INP6 = PF12 (ADC1) / PF14 (ADC2).
		// Internal (pin-less) channels: ADC2 INP16 = DAC1_OUT1, INP17 = DAC1_OUT2;
		// use setChannelNum() for those.
		// Fast channels INP0..INP5 (shorter sample time OK); INP6..INP19 are slow.
		// PC2_C/PC3_C are ANA direct-connect pins, not GPIO: they route to
		// INP14/INP16/INP17 via the SYSCFG internal analog switch (SYSCFG_PMCR
		// PC2SO/PC3SO); not supported here (occupied by FMC SDRAM on Apollo).
		static const byte CH_A[8] = { 16, 17, 14, 15, 18, 19, 3, 7 };// PA0..PA7
		static const byte CH_B[2] = { 9, 5 };// PB0,PB1
		static const byte CH_C[6] = { 10, 11, 0xFF, 0xFF, 4, 8 };// PC0..PC5 (PC2/PC3 not on LQFP176)
		if (&pin.getParent() == &GPIO['A']) {
			if (pin.getID() < numsof(CH_A)) return CH_A[pin.getID()];
		}
		else if (&pin.getParent() == &GPIO['B']) {
			if (pin.getID() < numsof(CH_B)) return CH_B[pin.getID()];
		}
		else if (&pin.getParent() == &GPIO['C']) {
			if (pin.getID() < numsof(CH_C)) return CH_C[pin.getID()];
		}
		else if (&pin.getParent() == &GPIO['F']) {
			if (ADC_ID == 2) {
				if (pin.getID() == 13) return 2;// PF13 = INP2
				if (pin.getID() == 14) return 6;// PF14 = INP6
			} else {
				if (pin.getID() == 11) return 2;// PF11 = INP2
				if (pin.getID() == 12) return 6;// PF12 = INP6
			}
		}
		return 0xFF;
		#endif
		// F1/F4 linear map (pin number == channel number); NOT valid for H7/MP13.
		if (&pin.getParent() == &GPIO['A'])
			return pin.getID() < 8 ? pin.getID() : 0xFF;
		else if (&pin.getParent() == &GPIO['C'])
			return pin.getID() < 6 ? (10 + pin.getID()) : 0xFF;
		else return 0xFF;
	}

	bool ADC_t::_configChannel(byte chan, byte rank, ADCSample sample, bool diff) {
		if (rank >= 16 || chan >= 20) return false;
		#if defined(_MCU_STM32H7x)
		self[ADCReg::PCSEL].setof(chan, true);
		self[ADCReg::DIFSEL].setof(chan, diff);// single-ended=0, differential=1
		#endif
		if (chan < 10) self[ADCReg::SMPR1].maset(3 * chan, 3, (stduint)sample);
		else self[ADCReg::SMPR2].maset(3 * (chan - 10), 3, (stduint)sample);
		byte r = rank + 1;
		if (r < 5) self[ADCReg::SQR1].maset(6 * r, 5, chan);
		else if (r < 10) self[ADCReg::SQR2].maset(6 * (r - 5), 5, chan);
		else if (r < 15) self[ADCReg::SQR3].maset(6 * (r - 10), 5, chan);
		else self[ADCReg::SQR4].maset(6 * (r - 15), 5, chan);
		return true;
	}

	bool ADC_t::setChannel(GPIO_Pin& pin, byte rank, ADCSample sample) {
		byte chan = getChannelNumber(pin);
		if (chan == 0xFF) return false;
		pin.setMode(GPIOMode::IN_Analog);
		return _configChannel(chan, rank, sample, false);
	}

	bool ADC_t::setChannelNum(byte chan, byte rank, ADCSample sample) {
		return _configChannel(chan, rank, sample, false);
	}

	bool ADC_t::setDiffChannel(GPIO_Pin& pin, byte rank, ADCSample sample) {
		byte chan = getChannelNumber(pin);
		if (chan == 0xFF) return false;
		pin.setMode(GPIOMode::IN_Analog);
		return _configChannel(chan, rank, sample, true);
	}

	bool ADC_t::Start(IOMethod method) {
		if (method == IOMethod::DMA) return false;// use enDMA()
		if (!self[ADCReg::CR].bitof(_ADC_CR_POS_ADEN) && !enAble(true)) return false;
		self[ADCReg::ISR].setof(_ADC_ISR_POS_EOC, true);
		self[ADCReg::ISR].setof(_ADC_ISR_POS_EOS, true);
		self[ADCReg::ISR].setof(_ADC_ISR_POS_OVR, true);
		if (method == IOMethod::Rupt) enInterrupt(true);
		self[ADCReg::CR].setof(_ADC_CR_POS_ADSTART, true);
		return true;
	}

	bool ADC_t::Stop(IOMethod method) {
		if (method == IOMethod::DMA) return StopDMA();
		self[ADCReg::CR].setof(_ADC_CR_POS_ADSTP, true);
		stduint timeout = 0xFFFF;
		while (self[ADCReg::CR].bitof(_ADC_CR_POS_ADSTART) && timeout--) {}
		if (method == IOMethod::Rupt) {
			self[ADCReg::IER].setof(_ADC_ISR_POS_EOC, false);
			self[ADCReg::IER].setof(_ADC_ISR_POS_OVR, false);
		}
		self[ADCReg::CR].setof(_ADC_CR_POS_ADDIS, true);
		return true;
	}

	bool ADC_t::canMode() {
		// aka HAL_ADC_DeInit
		self[ADCReg::CR].setof(_ADC_CR_POS_ADSTP, true);
		self[ADCReg::CR].setof(_ADC_CR_POS_JADSTP, true);
		self[ADCReg::IER] = 0;
		self[ADCReg::ISR] = 0x7FE;// clear all clearable flags (EOSMP..JQOVF)
		#if defined(_MCU_STM32H7x)
		self[ADCReg::CFGR].setof(_ADC_CFGR_POS_JQM, true);// flush injected queue
		#elif defined(_MPU_STM32MP13)
		self[ADCReg::CFGR1].setof(_ADC_CFGR_POS_JQM, true);
		#endif
		self[ADCReg::CR].setof(_ADC_CR_POS_ADDIS, true);
		self[ADCReg::CR].setof(_ADC_CR_POS_ADVREGEN, false);
		self[ADCReg::CR].setof(_ADC_CR_POS_DEEPPWD, true);
		self[ADCReg::CFGR2] = 0;
		self[ADCReg::SMPR1] = 0;
		self[ADCReg::SMPR2] = 0;
		self[ADCReg::SQR1] = 0;
		self[ADCReg::SQR2] = 0;
		self[ADCReg::SQR3] = 0;
		self[ADCReg::SQR4] = 0;
		self[ADCReg::JSQR] = 0;
		self[ADCReg::OFR1] = 0;
		self[ADCReg::OFR2] = 0;
		self[ADCReg::OFR3] = 0;
		self[ADCReg::OFR4] = 0;
		self[ADCReg::JDR1] = 0;
		self[ADCReg::JDR2] = 0;
		self[ADCReg::JDR3] = 0;
		self[ADCReg::JDR4] = 0;
		self[ADCReg::AWD2CR] = 0;
		self[ADCReg::AWD3CR] = 0;
		self[ADCReg::DIFSEL] = 0;
		self[ADCReg::CALFACT] = 0;
		#if defined(_MCU_STM32H7x)
		self[ADCReg::CFGR] = 0;
		self[ADCReg::CFGR].setof(_ADC_CFGR_POS_JQDIS, true);
		self[ADCReg::PCSEL] = 0;
		self[ADCReg::LTR1] = 0;
		self[ADCReg::HTR1] = 0;
		self[ADCReg::LTR2] = 0;
		self[ADCReg::HTR2] = 0;
		self[ADCReg::LTR3] = 0;
		self[ADCReg::HTR3] = 0;
		self[ADCReg::CALFACT2] = 0;
		#elif defined(_MPU_STM32MP13)
		self[ADCReg::CFGR1] = 0;
		self[ADCReg::CFGR1].setof(_ADC_CFGR_POS_JQDIS, true);
		self[ADCReg::TR1] = 0;
		self[ADCReg::TR2] = 0;
		self[ADCReg::TR3] = 0;
		#endif
		bind = 0;
		enClock(false);
		return true;
	}

	bool ADC_t::setDMA(const DMAStream& stream) {
		bind = (pureptr_t)&stream;
		return true;
	}

	bool ADC_t::enDMA(pureptr_t addr, stduint leng) {
		if (!bind || !addr || !leng) return false;
		const DMAStream& stream = *(const DMAStream*)bind;
		stream.getParent().enClock();
		stduint req = 0;
		#if defined(_MCU_STM32H7x)
		req = ADC_ID == 1 ? 9 : ADC_ID == 2 ? 10 : ADC_ID == 3 ? 115 : 0;
		#elif defined(_MPU_STM32MP13)
		req = ADC_ID == 1 ? 9 : ADC_ID == 2 ? 10 : 0;
		#endif
		if (!req) return false;
		stream.setRequest(req);
		if (!enAble(true)) return false;
		self[ADCReg::ISR].setof(_ADC_ISR_POS_EOC, true);
		self[ADCReg::ISR].setof(_ADC_ISR_POS_EOS, true);
		self[ADCReg::ISR].setof(_ADC_ISR_POS_OVR, true);
		self[ADCReg::IER].setof(_ADC_ISR_POS_OVR, true);
		#if defined(_MCU_STM32H7x)
		self[ADCReg::CFGR].maset(_ADC_CFGR_POS_DMNGT, 2, 3);// circular
		#elif defined(_MPU_STM32MP13)
		self[ADCReg::CFGR1].setof(_ADC_CFGR_POS_DMAEN, true);
		self[ADCReg::CFGR1].setof(_ADC_CFGR_POS_DMACFG, true);// circular
		#endif
		if (!stream.Transfer(addr, (pureptr_t)&self[ADCReg::DR], leng, IOMethod::Rupt))
			return false;
		self[ADCReg::CR].setof(_ADC_CR_POS_ADSTART, true);
		return true;
	}

	bool ADC_t::StopDMA() {
		self[ADCReg::CR].setof(_ADC_CR_POS_ADSTP, true);
		stduint timeout = 0xFFFF;
		while (self[ADCReg::CR].bitof(_ADC_CR_POS_ADSTART) && timeout--) {}
		#if defined(_MCU_STM32H7x)
		self[ADCReg::CFGR].maset(_ADC_CFGR_POS_DMNGT, 2, 0);
		#elif defined(_MPU_STM32MP13)
		self[ADCReg::CFGR1].setof(_ADC_CFGR_POS_DMAEN, false);
		#endif
		if (bind) {
			const DMAStream& stream = *(const DMAStream*)bind;
			stream.Abort();
		}
		self[ADCReg::IER].setof(_ADC_ISR_POS_OVR, false);
		self[ADCReg::CR].setof(_ADC_CR_POS_ADDIS, true);
		return true;
	}

	bool ADC_t::setWatchdog(stduint low, stduint high, byte channel) {
		if (channel > 18) return false;
		self[ADCReg::ISR].setof(_ADC_ISR_POS_AWD1, true);// clear flag
		#if defined(_MCU_STM32H7x)
		self[ADCReg::LTR1] = low << 4;
		self[ADCReg::HTR1] = high << 4;
		self[ADCReg::CFGR].setof(_ADC_CFGR_POS_AWD1SGL, true);
		self[ADCReg::CFGR].setof(_ADC_CFGR_POS_AWD1EN, true);
		self[ADCReg::CFGR].maset(_ADC_CFGR_POS_AWD1CH, 5, channel);
		#elif defined(_MPU_STM32MP13)
		self[ADCReg::TR1] = (low & 0xFFF) | ((high & 0xFFF) << 16);
		self[ADCReg::CFGR1].setof(_ADC_CFGR_POS_AWD1SGL, true);
		self[ADCReg::CFGR1].setof(_ADC_CFGR_POS_AWD1EN, true);
		self[ADCReg::CFGR1].maset(_ADC_CFGR_POS_AWD1CH, 5, channel);
		#endif
		return true;
	}

	bool ADC_t::isBusy() const {
		return self[ADCReg::CR].bitof(_ADC_CR_POS_ADSTART)
			|| self[ADCReg::CR].bitof(_ADC_CR_POS_JADSTART);
	}

	uint32 ADC_t::getError() const {
		uint32 err = _ADC_ERROR_NONE;
		if (self[ADCReg::ISR].bitof(_ADC_ISR_POS_OVR)) err |= _ADC_ERROR_OVR;
		if (self[ADCReg::ISR].bitof(_ADC_ISR_POS_JQOVF)) err |= _ADC_ERROR_JQOVF;
		return err;
	}

	uint32 ADC_t::getCalibration() const {
		return self[ADCReg::CALFACT] & 0x7F;// single-ended factor
	}

	bool ADC_t::setCalibration(uint32 factor) {
		if (factor > 0x7F) return false;
		if (!self[ADCReg::CR].bitof(_ADC_CR_POS_ADEN)) return false;
		self[ADCReg::CALFACT].maset(0, 7, factor);
		return true;
	}

	bool ADC_t::Poll(stduint timeout) {
		while (!self[ADCReg::ISR].bitof(_ADC_ISR_POS_EOC) && timeout--) {}
		return self[ADCReg::ISR].bitof(_ADC_ISR_POS_EOC);
	}

	uint32 ADC_t::Calibrate() {
		// H7/MP13: calibration must run with the ADC disabled (ADEN=0).
		if (self[ADCReg::CR].bitof(_ADC_CR_POS_ADEN)) {
			self[ADCReg::CR].setof(_ADC_CR_POS_ADDIS, true);
			stduint t = 0xFFFF;
			while (self[ADCReg::CR].bitof(_ADC_CR_POS_ADEN) && t--) {}
		}
		self[ADCReg::CR].setof(_ADC_CR_POS_ADCAL, true);
		stduint timeout = 0xFFFF;
		while (self[ADCReg::CR].bitof(_ADC_CR_POS_ADCAL) && timeout--) {}
		return self[ADCReg::CALFACT];
	}

	void ADC_t::enInterrupt(bool enable) const {
		self[ADCReg::IER].setof(_ADC_ISR_POS_EOC, enable);
	#if defined(_MPU_STM32MP13)
		GIC.enInterrupt(ADCx_Request_list[self.ADC_ID], enable);
	#else
		NVIC.setAble(ADCx_Request_list[self.ADC_ID], enable);
	#endif
	}

	#if defined(_MCU_STM32H7x)
	bool ADC_t::getLinearCalibration(uint32* buffer) const {
		if (!buffer) return false;
		if (!self[ADCReg::CR].bitof(_ADC_CR_POS_ADEN) && !enAble(true)) return false;
		for (byte cnt = 0; cnt < 6; cnt++) {
			byte pos = _ADC_CR_POS_LINCALRDYW6 - cnt;
			self[ADCReg::CR].setof(pos, false);
			stduint timeout = 0xFFFF;
			while (self[ADCReg::CR].bitof(pos) && timeout--) {}
			buffer[cnt] = self[ADCReg::CALFACT2];
		}
		return true;
	}

	bool ADC_t::setLinearCalibration(const uint32* buffer) {
		if (!buffer) return false;
		if (self[ADCReg::CR].bitof(_ADC_CR_POS_DEEPPWD))
			self[ADCReg::CR].setof(_ADC_CR_POS_DEEPPWD, false);
		if (!self[ADCReg::CR].bitof(_ADC_CR_POS_ADVREGEN)) {
			self[ADCReg::CR].setof(_ADC_CR_POS_ADVREGEN, true);
			for0(i, 10 * SystemCoreClock / 1000000) i = i;
		}
		if (!self[ADCReg::CR].bitof(_ADC_CR_POS_ADVREGEN)) return false;
		for (byte cnt = 0; cnt < 6; cnt++) {
			byte pos = _ADC_CR_POS_LINCALRDYW6 - cnt;
			self[ADCReg::CALFACT2] = buffer[cnt];
			self[ADCReg::CR].setof(pos, true);
			stduint timeout = 0xFFFF;
			while (!self[ADCReg::CR].bitof(pos) && timeout--) {}
		}
		return true;
	}
	#endif

	bool ADC_t::StartInject() {
		if (!self[ADCReg::CR].bitof(_ADC_CR_POS_ADEN) && !enAble(true)) return false;
		self[ADCReg::ISR].setof(_ADC_ISR_POS_JEOC, true);
		self[ADCReg::ISR].setof(_ADC_ISR_POS_JEOS, true);
		self[ADCReg::CR].setof(_ADC_CR_POS_JADSTART, true);
		return true;
	}

	bool ADC_t::StopInject() {
		self[ADCReg::CR].setof(_ADC_CR_POS_JADSTP, true);
		stduint timeout = 0xFFFF;
		while (self[ADCReg::CR].bitof(_ADC_CR_POS_JADSTART) && timeout--) {}
		if (!self[ADCReg::CR].bitof(_ADC_CR_POS_ADSTART))
			self[ADCReg::CR].setof(_ADC_CR_POS_ADDIS, true);
		return true;
	}

	bool ADC_t::PollInject(stduint timeout) {
		while (!self[ADCReg::ISR].bitof(_ADC_ISR_POS_JEOC) && timeout--) {}
		return self[ADCReg::ISR].bitof(_ADC_ISR_POS_JEOC);
	}

	uint32 ADC_t::getInjectValue(byte rank) {
		switch (rank) {
		case 3: return self[ADCReg::JDR4];
		case 2: return self[ADCReg::JDR3];
		case 1: return self[ADCReg::JDR2];
		default: return self[ADCReg::JDR1];
		}
	}

	bool ADC_t::setInjectChannel(GPIO_Pin& pin, byte rank, ADCSample sample) {
		if (rank >= 4) return false;
		byte chan = getChannelNumber(pin);
		if (chan == 0xFF) return false;
		pin.setMode(GPIOMode::IN_Analog);
		#if defined(_MCU_STM32H7x)
		self[ADCReg::PCSEL].setof(chan, true);
		#endif
		if (chan < 10) self[ADCReg::SMPR1].maset(3 * chan, 3, (stduint)sample);
		else self[ADCReg::SMPR2].maset(3 * (chan - 10), 3, (stduint)sample);
		self[ADCReg::JSQR].maset(8 + 5 * rank, 5, chan);
		return true;
	}

	bool ADC_t::enInjectQueue(bool ena) {
		#if defined(_MCU_STM32H7x)
		self[ADCReg::CFGR].setof(_ADC_CFGR_POS_JQDIS, !ena);
		#elif defined(_MPU_STM32MP13)
		self[ADCReg::CFGR1].setof(_ADC_CFGR_POS_JQDIS, !ena);
		#endif
		return true;
	}

	bool ADC_t::setMultiMode(stduint mode, stduint dual_data, stduint delay) {
		Reference ccr = Common(ADCCom::CCR);
		ccr.maset(_ADC_CCR_POS_DAMDF, 4, dual_data);
		ccr.maset(_ADC_CCR_POS_DUAL, 5, mode);
		ccr.maset(_ADC_CCR_POS_DELAY, 4, delay);
		return true;
	}

	bool ADC_t::enMultiDMA(pureptr_t addr, stduint leng) {
		if (!bind || !addr || !leng) return false;
		const DMAStream& stream = *(const DMAStream*)bind;
		stream.getParent().enClock();
		stream.setRequest(9);// ADC1 (multimode master)
		if (!enAble(true)) return false;
		self[ADCReg::ISR].setof(_ADC_ISR_POS_EOC, true);
		self[ADCReg::ISR].setof(_ADC_ISR_POS_EOS, true);
		self[ADCReg::ISR].setof(_ADC_ISR_POS_OVR, true);
		self[ADCReg::IER].setof(_ADC_ISR_POS_OVR, true);
		if (!stream.Transfer(addr, (pureptr_t)&Common(ADCCom::CDR), leng, IOMethod::Rupt))
			return false;
		self[ADCReg::CR].setof(_ADC_CR_POS_ADSTART, true);
		return true;
	}

	bool ADC_t::StopMultiDMA() {
		self[ADCReg::CR].setof(_ADC_CR_POS_ADSTP, true);
		stduint timeout = 0xFFFF;
		while (self[ADCReg::CR].bitof(_ADC_CR_POS_ADSTART) && timeout--) {}
		if (bind) {
			const DMAStream& stream = *(const DMAStream*)bind;
			stream.Abort();
		}
		self[ADCReg::IER].setof(_ADC_ISR_POS_OVR, false);
		self[ADCReg::CR].setof(_ADC_CR_POS_ADDIS, true);
		return true;
	}

	uint32 ADC_t::getMultiValue() {
		return Common(ADCCom::CDR);
	}

	bool ADC_t::enVoltageRegulator(bool ena) {
		if (!ena && self[ADCReg::CR].bitof(_ADC_CR_POS_ADEN)) return false;
		self[ADCReg::CR].setof(_ADC_CR_POS_ADVREGEN, ena);
		if (ena) for0(i, 10 * SystemCoreClock / 1000000) i = i;
		return true;
	}

	bool ADC_t::enDeepPowerDown(bool ena) {
		if (ena && self[ADCReg::CR].bitof(_ADC_CR_POS_ADEN)) return false;
		self[ADCReg::CR].setof(_ADC_CR_POS_DEEPPWD, ena);
		return true;
	}

	void ADC_t::setCallback(ADCCallbackID id, Handler_t f) const {
		if (id == ADCCallbackID::ConvCplt)
			FUNC_ADCx[getID()] = f;
	}

	#if defined(_MPU_STM32MP13)
	bool ADC_t::PollEvent(ADCEvent event, stduint timeout) {
		byte pos = (byte)event;
		while (!self[ADCReg::ISR].bitof(pos) && timeout--) {}
		if (!self[ADCReg::ISR].bitof(pos)) return false;
		self[ADCReg::ISR].setof(pos, true);// clear flag
		return true;
	}

	bool ADC_t::enSampling(bool ena) {
		self[ADCReg::CFGR2].setof(_ADC_CFGR2_POS_SWTRIG, ena);
		return true;
	}
	#endif

	ADC_t& ADC_Global::operator[](byte id) {
		extern ADC_t ADCr;
		#if defined(_MCU_STM32H7x)
		const static ADC_t* ADC_LST[] = { &ADC1, &ADC2, &ADC3 };
		#elif defined(_MPU_STM32MP13)
		const static ADC_t* ADC_LST[] = { &ADC1, &ADC2 };
		#endif
		if (!Ranglin(id, 1, numsof(ADC_LST)))
			return ADCr;
		return *(ADC_t*)ADC_LST[id - 1];
	}

#endif
}
