// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Device) TIM
// Codifiers: @dosconio: 20240422 ~ <Last-check> 
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

#include "../../../inc/cpp/Device/TIM"
#include "../../../inc/cpp/MCU/_ADDRESS/ADDR-STM32.h"
#include "../../../inc/cpp/Device/SysTick"


namespace uni {
#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)
	void TIM_t::setInterrupt(Handler_t f) const {
		FUNC_TIMx[getID()] = f;
	}
	static void timer_it(byte TIM_ID, bool enable);
#endif

#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x)
	// Return the NVIC IRQ line for a timer event.
	// typ: 0/2:Update, 1:CaptureCompare, 3:Break, 4:Trigger, 5:Commutation
	static Request_t TIM_Request(byte TIM_ID, byte typ) {
#if defined(_MCU_STM32H7x)
		if (TIM_ID == 1) {
			switch (typ) {
			case 1: return IRQ_TIM1_CC;
			case 3: return IRQ_TIM1_BRK;
			case 4: case 5: return IRQ_TIM1_TRG_COM;
			default: return IRQ_TIM1_UP;
			}
		}
		if (TIM_ID == 8) {
			switch (typ) {
			case 1: return IRQ_TIM8_CC;
			case 3: return IRQ_TIM8_BRK_TIM12;
			case 4: case 5: return IRQ_TIM8_TRG_COM_TIM14;
			default: return IRQ_TIM8_UP_TIM13;
			}
		}
#else
		if (TIM_ID == 1) {
			switch (typ) {
			case 1: return IRQ_TIM1CC;
			case 3: return IRQ_TIM1BRK;
			case 4: case 5: return IRQ_TIM1TRG_COM;
			default: return IRQ_TIM1UP;
			}
		}
		if (TIM_ID == 8) {
			switch (typ) {
			case 1: return IRQ_TIM8_CC;
			case 3: return IRQ_TIM8_BRK;
			case 4: case 5: return IRQ_TIM8_TRG_COM;
			default: return IRQ_TIM8_UP;
			}
		}
#endif
		static const Request_t list[] = { Request_None,
			Request_None, IRQ_TIM2, IRQ_TIM3,
			IRQ_TIM4, IRQ_TIM5,
#if defined(_MCU_STM32H7x)
			IRQ_TIM6_DAC, IRQ_TIM7,
#else
			IRQ_TIM6, IRQ_TIM7,
#endif
		};
		return (TIM_ID < numsof(list)) ? list[TIM_ID] : Request_None;
	}
	void TIM_t::setInterruptPriority(byte preempt, byte sub_priority) const {
		NVIC.setPriority(TIM_Request(getID(), 0), preempt, sub_priority);
		if (getID() == 1 || getID() == 8) {
			NVIC.setPriority(TIM_Request(getID(), 1), preempt, sub_priority);// CC
			NVIC.setPriority(TIM_Request(getID(), 3), preempt, sub_priority);// BRK
			NVIC.setPriority(TIM_Request(getID(), 4), preempt, sub_priority);// TRG_COM
		}
	}
	void TIM_t::enInterrupt(bool enable) const {
		NVIC.setAble(TIM_Request(getID(), 0), enable);// enable/disable NVIC
		timer_it(getID(), enable);// configure DIER + counter
	}
#elif defined(_MPU_STM32MP13)
	static Request_t TIM_Request(byte TIM_ID, byte typ) {
		if (TIM_ID == 1) {
			switch (typ) {
			case 1: return IRQ_TIM1_CC;
			case 3: return IRQ_TIM1_BRK;
			case 4: case 5: return IRQ_TIM1_TRG_COM;
			default: return IRQ_TIM1_UP;
			}
		}
		if (TIM_ID == 8) {
			switch (typ) {
			case 1: return IRQ_TIM8_CC;
			case 3: return IRQ_TIM8_BRK;
			case 4: case 5: return IRQ_TIM8_TRG_COM;
			default: return IRQ_TIM8_UP;
			}
		}
		static const Request_t list[] = { Request_None,
			Request_None, IRQ_TIM2, IRQ_TIM3,
			IRQ_TIM4, IRQ_TIM5, IRQ_TIM6, IRQ_TIM7,
		};
		return (TIM_ID < numsof(list)) ? list[TIM_ID] : Request_None;
	}
	void TIM_t::setInterruptPriority(byte preempt, byte sub_priority) const {
		(void)sub_priority;
		GIC.setPriority(TIM_Request(getID(), 0), preempt);
		if (getID() == 1 || getID() == 8) {
			GIC.setPriority(TIM_Request(getID(), 1), preempt);// CC
			GIC.setPriority(TIM_Request(getID(), 3), preempt);// BRK
			GIC.setPriority(TIM_Request(getID(), 4), preempt);// TRG_COM
		}
	}
	void TIM_t::enInterrupt(bool enable) const {
		GIC.enInterrupt(TIM_Request(getID(), 0), enable);// enable/disable GIC
		timer_it(getID(), enable);// configure DIER + counter
	}
#endif

#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)
	// aka HAL_TIM_Base_DeInit / PWM_DeInit / IC_DeInit / OC_DeInit / OnePulse_DeInit / Encoder_DeInit
	bool TIM_t::canMode() {
		using namespace TimReg;
		enInterrupt(false);// disable interrupt + clear UIE
		// Reset registers to their default (reset) values.
		// Writing to reserved/unimplemented offsets (e.g. F1 TIM6/7) is harmless.
		self[CR1] = 0;// CEN=0, aka __HAL_TIM_DISABLE
		self[CR2] = 0;
		self[SMCR] = 0;
		self[DIER] = 0;
		self[SR] = 0;
		self[EGR] = 0;
		self[CCMR1] = 0;
		self[CCMR2] = 0;
		self[CCER] = 0;
		self[CNT] = 0;
		self[PSC] = 0;
		self[ARR] = 0;
		self[RCR] = 0;
		self[CCR1] = 0;
		self[CCR2] = 0;
		self[CCR3] = 0;
		self[CCR4] = 0;
		self[BDTR] = 0;
		self[DCR] = 0;
		self[DMAR] = 0;
		self[OR] = 0;
		enClock(false);// __HAL_RCC_TIMx_CLK_DISABLE
		return true;
	}

	// aka __HAL_TIM_ENABLE_IT(TIM_IT_TRIGGER) + NVIC/GIC enable
	void TIM_t::enTriggerInterrupt(bool ena) {
		using namespace TimReg;
		self[DIER].setof(6, ena);// DIER.TIE
		if (TIM_ID == 1 || TIM_ID == 8) {// advanced timers route trigger to the TRG_COM line
#if defined(_MPU_STM32MP13)
			GIC.enInterrupt(TIM_Request(TIM_ID, 4), ena);
#else
			NVIC.setAble(TIM_Request(TIM_ID, 4), ena);
#endif
		}
	}

	// aka __HAL_TIM_ENABLE_IT(TIM_IT_BREAK) + NVIC/GIC enable
	void TIM_t::enBreakInterrupt(bool ena) {
		using namespace TimReg;
		if (TIM_ID != 1 && TIM_ID != 8) return;// break exists only on advanced timers
		self[DIER].setof(7, ena);// DIER.BIE
#if defined(_MPU_STM32MP13)
		GIC.enInterrupt(TIM_Request(TIM_ID, 3), ena);
#else
		NVIC.setAble(TIM_Request(TIM_ID, 3), ena);
#endif
	}
#endif

	#if defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)
	TIM_CHAN_t TIM_t::operator[](stduint chan_id) {
		return TIM_CHAN_t(TIM_ID, chan_id);
	}

	struct lock_timc {
		byte TIM_ID;
		lock_timc(byte _TIM_ID) : TIM_ID(_TIM_ID) { TIM[_TIM_ID]->enAble(false); }
		~lock_timc() { TIM[TIM_ID]->enAble(); }
	};
	#endif

	#if defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x)
	bool TIM_CHAN_t::setMode(stduint compare, GPIO_Pin* pin) {
		Letvar(addr, TIM_C*, TIM[TIM_ID]);
		return addr->setChannel(CHAN_ID, compare, pin);
	}
	#endif

#if 0
#elif defined(_MCU_STM32F1x)
	// contain: HAL_TIM_Base_Start_IT
	static void timer_it(byte TIM_ID, bool enable) {
		using namespace TimReg;
		TIM_t& sel = *TIM[TIM_ID];
		sel[DIER].setof(0, enable);// TIM_IT_UPDATE
		if (sel.getID() <= 5 || sel.getID() == 8) //aka IS_TIM_SLAVE_INSTANCE, true for TIM 1/2/3/4/5/8
		{
			/* TODO (why so?)
			tmpsmcr = htim->Instance->SMCR & TIM_SMCR_SMS;
			if (!IS_TIM_SLAVEMODE_TRIGGER_ENABLED(tmpsmcr))
			{
				__HAL_TIM_ENABLE(htim);
			}
			*/
		}
		else sel.enAble();
	}

	void TIM_t::ConfigMaster(_TEMP stduint master_output_triggerm, bool master_slave_enable) {
		using namespace TimReg;
		// Compatible with `HAL_StatusTypeDef HAL_TIMEx_MasterConfigSynchronization(TIM_HandleTypeDef *htim, TIM_MasterConfigTypeDef * sMasterConfig)`
		self[CR2] &= ~(7U << 4); // Reset MMS Bits
		self[CR2] |= master_output_triggerm; // Select the TRGO source
		self[SMCR].setof(_TIM_SMCR_POS_MSM, master_slave_enable);
	}
	
	TIM_B TIM6(0x40001000, 6);
	TIM_B TIM7(0x40001400, 7);
	TIM_t* TIM[] = { nullptr,
		nullptr, nullptr, nullptr, nullptr, nullptr,
		&TIM6, &TIM7,
	};

#elif defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)

	static const uint32 _REFADDR_TIM[] = { nil,
#if defined(_MCU_STM32H7x)
		D2_APB2PERIPH_BASE + 0x0000, D2_APB1PERIPH_BASE + 0x0000, D2_APB1PERIPH_BASE + 0x0400, D2_APB1PERIPH_BASE + 0x0800, // T 1 -> 4
		D2_APB1PERIPH_BASE + 0x0C00, D2_APB1PERIPH_BASE + 0x1000, D2_APB1PERIPH_BASE + 0x1400, D2_APB2PERIPH_BASE + 0x0400, // T 5 -> 8
		D2_APB2PERIPH_BASE + 0x4000, D2_APB2PERIPH_BASE + 0x4400, D2_APB2PERIPH_BASE + 0x4800, D2_APB1PERIPH_BASE + 0x1800, // T 9 -> 12
		D2_APB1PERIPH_BASE + 0x1C00, D2_APB1PERIPH_BASE + 0x2000 // T 13 -> 14
#elif defined(_MPU_STM32MP13)
		APB2_PERIPH_BASE + 0x0000, APB1_PERIPH_BASE + 0x0000, APB1_PERIPH_BASE + 0x1000, APB1_PERIPH_BASE + 0x2000, // T 1 -> 4
		APB1_PERIPH_BASE + 0x3000, APB1_PERIPH_BASE + 0x4000, APB1_PERIPH_BASE + 0x5000, APB2_PERIPH_BASE + 0x1000, // T 5 -> 8
		nil,                       nil,                       nil,                       APB6_PERIPH_BASE + 0x7000, // T 9 -> 12
		APB6_PERIPH_BASE + 0x8000, APB6_PERIPH_BASE + 0x9000, APB6_PERIPH_BASE + 0xA000, APB6_PERIPH_BASE + 0xB000, // T 13 -> 16
		APB6_PERIPH_BASE + 0xC000 // T 17
#else
		0x40010000, 0x40000000, 0x40000400, 0x40000800, // T 1 -> 4
		0x40000C00, 0x40001000, 0x40001400, 0x40010400, // T 5 -> 8
		0x40014000, 0x40014400, 0x40014800, 0x40001800, // T 9 -> 12
		0x40001C00, 0x40002000 // T 13 -> 14
#endif
	};
	
	TIM_A TIM1(_REFADDR_TIM[1], 1);
	TIM_C TIM2(_REFADDR_TIM[2], 2);
	TIM_C TIM3(_REFADDR_TIM[3], 3);
	TIM_C TIM4(_REFADDR_TIM[4], 4);
	TIM_C TIM5(_REFADDR_TIM[5], 5);
	TIM_B TIM6(_REFADDR_TIM[6], 6);
	TIM_B TIM7(_REFADDR_TIM[7], 7);
	TIM_A TIM8(_REFADDR_TIM[8], 8);
	TIM_t* TIM[] = { nullptr,
		(TIM_t*)(pureptr_t)&TIM1, (TIM_t*)(pureptr_t)&TIM2, (TIM_t*)(pureptr_t)&TIM3,
		(TIM_t*)(pureptr_t)&TIM4, (TIM_t*)(pureptr_t)&TIM5, (TIM_t*)(pureptr_t)&TIM6,
		(TIM_t*)(pureptr_t)&TIM7, (TIM_t*)(pureptr_t)&TIM8,
	};

	//{TODO} a channel may connect multiple pins
#if defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x)
	static GPIO_Pin* GPINs_chan1_TIMx[] = { nullptr,
		nullptr, // TIM1
		& GPIOA[15], // or A[0]
		& GPIOA[6], // or C[6] B[4]
		& GPIOB[6], // or D[12]
		& GPIOA[0]
	};
	static GPIO_Pin* GPINs_chan2_TIMx[] = { nullptr,
		nullptr, // TIM1
		& GPIOB[3], // or A[1]
		& GPIOA[7], // TIM3 or C[7] B[5]
		& GPIOD[13], // or B[7]
		& GPIOA[1]
	};
	static GPIO_Pin* GPINs_chan3_TIMx[] = { nullptr,
		nullptr, // TIM1
		& GPIOB[10], // or A[2]
		& GPIOB[0], // TIM3 or C[8] 
		& GPIOB[8], // or D[14]
		& GPIOA[2]
	};
	static GPIO_Pin* GPINs_chan4_TIMx[] = { nullptr,
		nullptr, // TIM1
		& GPIOB[11], // or A[3]
		& GPIOB[1], // TIM3 or C[9]
		& GPIOB[9], // or D[15]
		& GPIOA[3]
	};
	static GPIO_Pin** GPINs_chanx[] = {
		GPINs_chan1_TIMx, GPINs_chan2_TIMx, GPINs_chan3_TIMx, GPINs_chan4_TIMx
	};
	static byte GPINs_AFs_TIMx[1 + 11] = { nil,
		1,1, 2,2,2, // TIM1~5: AF1 for TIM1&2, AF2 for TIM3/4/5
		0xFF, 0xFF, // TIM6,7
		3,3,3,3
	};// F407 & F417 & H743
#endif

	static TimReg::TimRegType _tab_timregs_ccr[] = {
		TimReg::CCR1, TimReg::CCR2, TimReg::CCR3, TimReg::CCR4
	};
	static void timer_it(byte TIM_ID, bool enable) {
		using namespace TimReg;
		TIM_t& sel = *TIM[TIM_ID];
		if (enable) {
			sel[DIER] |= 1;// TIM_IT_UPDATE
			#if defined(_MCU_STM32H7x)
			sel.enAble(); // H7: unconditionally start counter
			#else
			if ((sel[SMCR] & 0x7) != 0x6) // !IS_TIM_SLAVEMODE_TRIGGER_ENABLED
				sel.enAble();
			#endif
		}
		else {
			sel[DIER].setof(0, false);// clear UIE
			#if defined(_MCU_STM32H7x)
			sel.enAble(false); // H7: stop counter on disable
			#else
						// F4: stop only if not in slave trigger mode
			if ((sel[SMCR] & 0x7) != 0x6)
				sel.enAble(false);
			#endif
		}
	}

	// aka HAL_TIM_Base_Init for basic timers (TIM6/7): 16-bit time base without channels.
	void TIM_B::setMode(stduint prescaler, stduint period, bool auto_reload_preload) {
		using namespace TimReg;
		asserv(prescaler)--;
		asserv(period)--;
		enClock();
		self[CR1].setof(_TIM_CR1_POS_ARPE, auto_reload_preload);
		self[ARR] = period;
		self[PSC] = prescaler;
		self[EGR] = 1;// reload prescaler immediately
		self[SR].setof(0, false);// clear UIF to avoid a spurious interrupt
	}

	void TIM_C::setMode(stduint prescaler, stduint period, bool auto_reload_preload) {
		using namespace TimReg;
		// 16-bit timers (TIM3/4): clamp the default period to avoid ARR truncation
		if ((TIM_ID == 3 || TIM_ID == 4) && period == _TIMC_DEFA_PERIOD)
			period = _TIMC_DEFA_PERIOD16;
		asserv(prescaler)--;
		asserv(period)--;
		bool _TEMP count_down = false;// direction
		//{TEMP} ASSUME ClockDivision=TIM_CLOCKDIVISION_DIV1 aka non-prescaled;
		enClock();
		//:aka HAL_TIM_Base_Init -> TIM_Base_SetConfig
		stduint tmpcr1 = self[CR1];
		if (TIM_ID == 8 || Ranglin(TIM_ID, 1, 5)) // IS_TIM_COUNTER_MODE_SELECT_INSTANCE
		{
			Reference(&tmpcr1).setof(_TIM_CR1_POS_DIR, count_down);
			Reference(&tmpcr1) &= ~_IMM(0x00000060);// TIM_CR1_CMS
		}
		if (TIM_ID != 6 && TIM_ID != 7) { // IS_TIM_CLOCK_DIVISION_INSTANCE
			Reference(&tmpcr1) &= ~_IMM(0x00000300);// TIM_CR1_CKD
			// 0x00000000U TIM_CLOCKDIVISION_DIV1
		}
		Reference(&tmpcr1).setof(_TIM_CR1_POS_ARPE, auto_reload_preload);
		self[CR1] = tmpcr1;
		self[ARR] = period;
		self[PSC] = prescaler;
		if (TIM_ID == 1 || TIM_ID == 8) // IS_TIM_REPETITION_COUNTER_INSTANCE
			self[RCR] = 0; // RepetitionCounter;
		self[EGR] = _TEMP 1;// TIM_PSCReloadMode_Immediate
		// Clear UIF after UG to avoid spurious interrupt
		self[SR].setof(0, false);
	}

	// preset: Tim.setMode, e.g.(pres, period);
#if defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x)
	bool TIM_C::setChannel(byte channel, stduint pulse_compar, GPIO_Pin* pin) {
		using namespace TimReg;
		if (!Ranglin(channel, 1, numsof(GPINs_chanx)) ||
			!GPINs_chanx[channel - 1][TIM_ID])
			return false;
		enAble(false);
		if (pin || true) {
			GPIO_Pin& friendo = *GPINs_chanx[channel - 1][TIM_ID];
			friendo.setMode(GPIOMode::OUT_AF_PushPull, GPIOSpeed::Veryhigh);
			if (0xFF != GPINs_AFs_TIMx[TIM_ID])
				friendo._set_alternate(GPINs_AFs_TIMx[TIM_ID]);
		}
		self.ConfigChannel(channel, pulse_compar);
		self.enChannel(channel);
		enAble();
		return true;
	}
#endif

	// aka HAL_TIM_PWM_ConfigChannel / HAL_TIM_OC_ConfigChannel
	// ocmode defaults to PWM1 (0x6) for backward compatibility
	void TIM_C::ConfigChannel(byte channel, stduint pulse, stduint ocmode) {
		using namespace TimReg;
		_TEMP bool ddp_before_compar = false;
		_TEMP bool TIM_OutputState_Enable = true;
		if (!Ranglin(channel, 1, 4)) return;
		//
		byte chan0x = 4 * (channel - 1);
		self[CCER].setof(chan0x, false);// CCxE
		TimReg::TimRegType trt = channel <= 2 ? CCMR1 : CCMR2;
		stduint shift = isodd(channel) ? 0 : 8;
		//: aka TIM_OCx_SetConfig
		{
			enCaptureCompareChannel(channel, false);
			self[trt].maset(4 + shift, 3, ocmode);// OCxM
			self[trt].maset(0 + shift, 2, nil);// CCxS = output
			//: Select the Output Compare Mode
			self[CCER].setof(chan0x + 1, ddp_before_compar);
			self[CCER].setof(chan0x + 0, TIM_OutputState_Enable);
			if ((channel < 4) && (TIM_ID == 1 || TIM_ID == 8) &&
				Ranglin(channel, 1, 3)) { // IS_TIM_CCXN_INSTANCE chan1~3
				_TODO /*
				tmpccer &= ~TIM_CCER_CC1NP;
				tmpccer |= OC_Config->OCNPolarity << ...;
				tmpccer &= ~TIM_CCER_CC1NE;
				*/
			}
			if (TIM_ID == 1 || TIM_ID == 8) { // IS_TIM_BREAK_INSTANCE
				_TODO /*
				tmpcr2 &= ~TIM_CR2_OIS1;// ISx
				tmpcr2 &= ~TIM_CR2_OIS1N;// ISxN
				tmpcr2 |= OC_Config->OCIdleState;// <<2 <<4
				tmpcr2 |= OC_Config->OCNIdleState;// <<2 << 4
				*/
			}
			self[_tab_timregs_ccr[channel - 1]] = pulse;//: aka __HAL_TIM_SET_COMPARE
		}
		// ARPE only meaningful for PWM modes (OC modes don't need preload)
		if (ocmode == 0x6 || ocmode == 0x7) {
			self[trt] |= _IMM(0x00000008 << shift);// OCxPE
		}
		self[trt] &= ~_IMM(0x00000004 << shift);// OCxFE; // TIM_OCFAST_DISABLE
	}


	_TEMP void TIM_C::Select(stduint SlaveMode, stduint TriggerInn, stduint TriggerOut, bool MasterSlaveMode)
	{
		using namespace TimReg;
		self[SMCR].maset(0, 3, SlaveMode);// TIM_SelectSlaveMode
		self[SMCR].maset(4, 3, TriggerInn);// TIM_SelectInputTrigger
		self[CR2].maset(_TIM_CR2_POSI_MMS, 3, TriggerOut);// TIM_SelectOutputTrigger
		self[SMCR].setof(7, MasterSlaveMode);// TIM_SelectMasterSlaveMode
	}

	// aka HAL_TIM_OnePulse_Init (OPM bit) / HAL_TIM_Encoder_Init (SMS bits) / HAL_TIMEx_HallSensor_Init (XOR + TI1F_ED + Reset)
	void TIM_C::setMode(TIMMode::TIMMode mode) {
		using namespace TimReg;
		switch (mode) {
		case TIMMode::OnePulse:
			self[CR1].setof(_TIM_CR1_POS_OPM, true);// OPM = 1 (single pulse)
			break;
		case TIMMode::Encoder:
			self[SMCR].maset(0, 3, 0x3);// SMS = 3 (encoder, dual-edge TI1+TI2)
			break;
		case TIMMode::HallSensor:
			self[CR2].setof(7, true);// TI1S = XOR (three hall inputs)
			self[SMCR].maset(4, 3, 0x4);// TS = TI1F_ED
			self[SMCR].maset(0, 3, 0x4);// SMS = Reset Mode
			break;
		}
	}

	// Configure input channel (aka HAL_TIM_IC_ConfigChannel / TIM_TIx_SetConfig)
	void TIM_C::ConfigChannelInn(byte channel, TimChinSel::TimChinSel sel, TimIcPol::TimIcPol pol, byte filter, byte prescaler) {
		using namespace TimReg;
		if (!Ranglin(channel, 1, 4)) return;
		if (!Ranglin(prescaler, 0, 3)) return;// DIV 1/2/4/8
		byte chan0x = 4 * (channel - 1);
		TimReg::TimRegType trt = channel <= 2 ? CCMR1 : CCMR2;
		byte shift = isodd(channel) ? 0 : 8;
		// aka TIM_TIx_SetConfig
		self[CCER].setof(chan0x, false);// CCxE = 0 (disable before config)
		self[trt].maset(shift, 2, stduint(sel));// CCxS input selection
		self[trt].maset(shift + 4, 4, filter);// ICxF filter
		self[trt].maset(shift + 2, 2, prescaler);// ICxPSC prescaler
		self[CCER].setof(chan0x + 1, pol == TimIcPol::Falling);// CCxP polarity
		self[CCER].setof(chan0x + 3, false);// CCxNP = 0
		self[CCER].setof(chan0x, true);// CCxE = 1
	}

	// Configure the clock source (aka HAL_TIM_ConfigClockSource)
	void TIM_C::ConfigClock(TimClockSrc::TimClockSrc src, TimIcPol::TimIcPol pol, byte prescaler, byte filter) {
		using namespace TimReg;
		if (!Ranglin(prescaler, 0, 3)) return;
		if (!Ranglin(filter, 0, 15)) return;
		// reset SMS, TS and ETR conditioning bits
		self[SMCR].maset(0, 3, 0);// SMS
		self[SMCR].maset(4, 3, 0);// TS
		self[SMCR].maset(8, 4, 0);// ETF
		self[SMCR].maset(12, 2, 0);// ETPS
		self[SMCR].setof(14, false);// ECE
		self[SMCR].setof(15, false);// ETP
		switch (src) {
		case TimClockSrc::Internal:
			break;
		case TimClockSrc::ETRMode1:
			self[SMCR].maset(8, 4, filter);// ETF
			self[SMCR].maset(12, 2, prescaler);// ETPS
			self[SMCR].setof(15, pol == TimIcPol::Falling);// ETP
			self[SMCR].maset(0, 3, 0x7);// SMS = external clock mode 1
			self[SMCR].maset(4, 3, 0x7);// TS = ETRF
			break;
		case TimClockSrc::ETRMode2:
			self[SMCR].maset(8, 4, filter);// ETF
			self[SMCR].maset(12, 2, prescaler);// ETPS
			self[SMCR].setof(15, pol == TimIcPol::Falling);// ETP
			self[SMCR].setof(14, true);// ECE
			break;
		case TimClockSrc::TI1:
			ConfigChannelInn(1, TimChinSel::Direct, pol, filter, 0);
			self[SMCR].maset(0, 3, 0x7);// SMS
			self[SMCR].maset(4, 3, 0x5);// TS = TI1FP1
			break;
		case TimClockSrc::TI2:
			ConfigChannelInn(2, TimChinSel::Direct, pol, filter, 0);
			self[SMCR].maset(0, 3, 0x7);// SMS
			self[SMCR].maset(4, 3, 0x6);// TS = TI2FP2
			break;
		case TimClockSrc::TI1ED:
			ConfigChannelInn(1, TimChinSel::Direct, pol, filter, 0);
			self[SMCR].maset(0, 3, 0x7);// SMS
			self[SMCR].maset(4, 3, 0x4);// TS = TI1F_ED
			break;
		case TimClockSrc::ITR0:
		case TimClockSrc::ITR1:
		case TimClockSrc::ITR2:
		case TimClockSrc::ITR3:
			self[SMCR].maset(4, 3, stduint(src) - stduint(TimClockSrc::ITR0));// TS = ITRx
			break;
		}
	}

	// Select the signal connected to TI1 (aka HAL_TIM_ConfigTI1Input)
	void TIM_C::ConfigTI1(TimTi1Sel::TimTi1Sel sel) {
		using namespace TimReg;
		self[CR2].setof(7, sel == TimTi1Sel::XOR);// TI1S
	}

	// Configure OCxREF clear source (aka HAL_TIM_ConfigOCrefClear)
	void TIM_C::ConfigOcrefClear(byte channel, TimOcrefSrc::TimOcrefSrc src, TimIcPol::TimIcPol pol, byte filter) {
		using namespace TimReg;
		if (!Ranglin(channel, 1, 4)) return;
		if (!Ranglin(filter, 0, 15)) return;
		if (src == TimOcrefSrc::None) {
			// clear ETR conditioning bits
			self[SMCR].maset(8, 4, 0);// ETF
			self[SMCR].maset(12, 2, 0);// ETPS
			self[SMCR].setof(14, false);// ECE
			self[SMCR].setof(15, false);// ETP
		}
		else {
			// ETR source: prescaler forced DIV1, no external clock
			self[SMCR].maset(8, 4, filter);// ETF
			self[SMCR].maset(12, 2, 0);// ETPS = DIV1
			self[SMCR].setof(14, false);// ECE = 0
			self[SMCR].setof(15, pol == TimIcPol::Falling);// ETP
		}
		TimReg::TimRegType trt = channel <= 2 ? CCMR1 : CCMR2;
		byte shift = isodd(channel) ? 0 : 8;
		self[trt].setof(shift + 7, src == TimOcrefSrc::ETR);// OCxCE
	}

	// Generate a software event (aka HAL_TIM_GenerateEvent)
	void TIM_C::GenerateEvent(TimEvSrc::TimEvSrc ev) {
		using namespace TimReg;
		self[EGR].setof(stduint(ev), true);
	}

	// Read the captured value of a channel (aka HAL_TIM_ReadCapturedValue)
	stduint TIM_C::ReadCapture(byte channel) {
		using namespace TimReg;
		if (!Ranglin(channel, 1, 4)) return 0;
		return self[_tab_timregs_ccr[channel - 1]];
	}

	static byte _TIM_ICxPSC[] = {

	};
	void TIM_CHAN_t::setMode(TimChinSel::TimChinSel sel, byte presexpo) {
		using namespace TimReg;
		TIM_C& t = *(TIM_C*)TIM[TIM_ID];
		_TEMP int polar = 0;// 0(Posedge), 2(Negedge), A(Anyedge)
		_TEMP byte filter = 0;
		TimReg::TimRegType trt = CHAN_ID <= 2 ? CCMR1 : CCMR2;
		byte shift = isodd(CHAN_ID) ? 0 : 8;
		if (!Ranglin(presexpo, 0, 4)) return;// DIV 1 2 4 8
		//aka TIx_Config
		{
			byte chan0x = 4 * (CHAN_ID - 1);
			t[CCER].setof(chan0x, false);// CCxE
			t[trt].maset(shift, 2, _IMM(sel)); // CCxS
			t[trt].maset(shift + 4, 4, filter);// ICxF
			t[CCER].setof(chan0x + 1, polar & 0x2);// CCxP
			t[CCER].setof(chan0x + 3, polar & 0x8);// CCxNP
			t[CCER].setof(chan0x, true);// CCxE
		}
		t[trt].maset(shift + 2, 2, presexpo);//aka TIM_SetICxPrescaler

	}

	bool TIM_CHAN_t::GenerateHalfwave(stduint pres, stduint period) {
		using namespace TimReg;
		TIM_C& t = *(TIM_C *)TIM[TIM_ID];
		t.setMode(pres, period);
		return self.setMode(period / 2);
	}
	void TIM_CHAN_t::RenerateHalfwave(stduint period) {
		using namespace TimReg;
		Letvar(addr, TIM_C*, self.getParent());
		lock_timc lock(addr->getID());
		(*addr)[ARR] = period - 1;
		(*addr)[_tab_timregs_ccr[CHAN_ID - 1]] = period / 2;
	}

	double TIM_CHAN_t::getDuty() const {
		using namespace TimReg;
		TIM_C& t = *(TIM_C*)TIM[TIM_ID];
		return (double)(t[_tab_timregs_ccr[CHAN_ID - 1]]) / (1. + t[ARR]);
	}
	// Configure output compare mode with a specific mode and pulse
	void TIM_CHAN_t::setMode(TimOutMode::TimOutMode mode, stduint pulse) {
		TIM_C& t = *(TIM_C*)getParent();
		if (t.getID() == 6 || t.getID() == 7) return; // basic timers have no channels
		t.ConfigChannel(CHAN_ID, pulse, stduint(mode));
		t.enChannel(CHAN_ID);
		t.enAble();
	}

	// TIM DMA callbacks — located via DMA_t.bind (same scheme as UART DMA)
#if defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)
	static void _TIM_DMA_UpdateCplt() {
		TIM_t* t = (TIM_t*)DMA1.bind;
		if (DMA1.XferCpltCallback != _TIM_DMA_UpdateCplt) t = (TIM_t*)DMA2.bind;
		if (!t) return;
		callif(FUNC_TIMx[t->getID()]);// period-elapsed callback
	}
	static void _TIM_DMA_DelayPulseCplt() {
		TIM_t* t = (TIM_t*)DMA1.bind;
		if (DMA1.XferCpltCallback != _TIM_DMA_DelayPulseCplt) t = (TIM_t*)DMA2.bind;
		if (!t) return;
		callif(t->FUNC_OC_DelayElapsed);
		callif(t->FUNC_PWMPulseFinished);
	}
	static void _TIM_DMA_CaptureCplt() {
		TIM_t* t = (TIM_t*)DMA1.bind;
		if (DMA1.XferCpltCallback != _TIM_DMA_CaptureCplt) t = (TIM_t*)DMA2.bind;
		if (!t) return;
		callif(t->FUNC_IC_Capture);
	}
	static void _TIM_DMA_Error() {
		TIM_t* t = (TIM_t*)DMA1.bind;
		if (DMA1.XferErrorCallback != _TIM_DMA_Error) t = (TIM_t*)DMA2.bind;
		if (!t) return;
		using namespace TimReg;
		(*t)[DIER] &= ~_IMM(0x1F00);// clear UDE + CC1~4DE (bits 8..12)
		t->enAble(false);// stop counter
	}

	// aka HAL_TIM_Base_Start_DMA: DMA writes ARR on each update event
	bool TIM_t::UpdateDMA(pureptr_t data, stduint leng, IOMethod method) {
		using namespace TimReg;
		if (!dma[0] || !data || !leng) return false;
		DMA1.bind = (pureptr_t)this;
		DMA1.XferCpltCallback = _TIM_DMA_UpdateCplt;
		DMA1.XferErrorCallback = _TIM_DMA_Error;
		// mem -> ARR (peripheral address is the ARR register)
		stduint arr_addr = getBaseaddr() + _IMMx4(ARR);
		if (!dma[0]->Transfer((pureptr_t)arr_addr, data, leng, method)) return false;
		// enable the update DMA request (DIER.UDE = bit 8)
		self[DIER].setof(8, true);
		// enable counter, except in slave-mode trigger
		if (0x6 != self[SMCR].mask(0, 3)) enAble(true);
		return true;
	}

	// aka HAL_TIM_PWM/OC/IC_Start_DMA / Stop_DMA
	bool TIM_C::enChannel(byte channel, bool ena, IOMethod method) {
		using namespace TimReg;
		if (!Ranglin(channel, 1, 4)) return false;
		if (method != IOMethod::DMA) return false;
		if (ena) {
			// enable CCxDE (CC1DE=bit9 .. CC4DE=bit12)
			self[DIER].setof(channel + 8, true);
			// bind callbacks by direction (output: DelayPulseCplt, input: CaptureCplt)
			if (dma[channel]) {
				DMA1.bind = (pureptr_t)this;
				TimReg::TimRegType trt = channel <= 2 ? CCMR1 : CCMR2;
				byte shift = isodd(channel) ? 0 : 8;
				if (self[trt].mask(shift, 2))// CCxS != 0 -> input capture
					DMA1.XferCpltCallback = _TIM_DMA_CaptureCplt;
				else
					DMA1.XferCpltCallback = _TIM_DMA_DelayPulseCplt;
				DMA1.XferErrorCallback = _TIM_DMA_Error;
			}
			enChannel(channel);// CCxE + MOE + CEN
		}
		else {
			// disable CCxDE
			self[DIER].setof(channel + 8, false);
			// abort the DMA stream
			if (dma[channel]) dma[channel]->Abort();
			enCaptureCompareChannel(channel, false);// CCxE = 0
			enAble(false);// CEN = 0
		}
		return true;
	}

	// aka HAL_TIM_Encoder_Start_DMA: DMA reads CCR1->data1, CCR2->data2 (position capture)
	bool TIM_C::CaptureDMA(pureptr_t data1, pureptr_t data2, stduint leng, IOMethod method) {
		using namespace TimReg;
		if (!dma[1] || !dma[2] || !data1 || !data2 || !leng) return false;
		DMA1.bind = (pureptr_t)this;
		DMA1.XferCpltCallback = _TIM_DMA_CaptureCplt;
		DMA1.XferErrorCallback = _TIM_DMA_Error;
		// peripheral -> memory read (DIR configured at DMA layer setMode)
		stduint ccr1_addr = getBaseaddr() + _IMMx4(CCR1);
		stduint ccr2_addr = getBaseaddr() + _IMMx4(CCR2);
		if (!dma[1]->Transfer(data1, (pureptr_t)ccr1_addr, leng, method)) return false;
		if (!dma[2]->Transfer(data2, (pureptr_t)ccr2_addr, leng, method)) return false;
		self[DIER].setof(1 + 8, true);// CC1DE = bit 9
		self[DIER].setof(2 + 8, true);// CC2DE = bit 10
		enCaptureCompareChannel(1, true);// CC1E
		enCaptureCompareChannel(2, true);// CC2E
		enAble(true);// CEN
		return true;
	}

	// DMA burst write: memory -> DMAR register burst (aka HAL_TIM_DMABurst_WriteStart)
	bool TIM_C::BurstWrite(pureptr_t data, byte base, byte burst_len, TimBurstSrc::TimBurstSrc src, stduint datalen, IOMethod method) {
		using namespace TimReg;
		if (!Ranglin(base, 0, 17)) return false;// DBA 0..17
		if (!Ranglin(burst_len, 1, 18)) return false;// DBL 1..18 transfers
		if (!Ranglin(stduint(src), 0, 4)) return false;
		if (!dma[stduint(src)] || !data || !datalen) return false;
		// configure DMA burst mode: DCR = DBA | (DBL << 8)
		self[DCR] = stduint(base) | (stduint(burst_len - 1) << 8);
		DMA1.bind = (pureptr_t)this;
		DMA1.XferCpltCallback = (src == TimBurstSrc::Update) ? _TIM_DMA_UpdateCplt : _TIM_DMA_DelayPulseCplt;
		DMA1.XferErrorCallback = _TIM_DMA_Error;
		// write direction: source = memory (data), dest = peripheral (DMAR)
		stduint dmar_addr = getBaseaddr() + _IMMx4(DMAR);
		if (!dma[stduint(src)]->Transfer((pureptr_t)dmar_addr, data, datalen, method)) return false;
		self[DIER].setof(8 + stduint(src), true);// UDE=8 .. CC4DE=12
		return true;
	}

	// DMA burst read: DMAR register burst -> memory (aka HAL_TIM_DMABurst_ReadStart)
	bool TIM_C::BurstRead(pureptr_t data, byte base, byte burst_len, TimBurstSrc::TimBurstSrc src, stduint datalen, IOMethod method) {
		using namespace TimReg;
		if (!Ranglin(base, 0, 17)) return false;
		if (!Ranglin(burst_len, 1, 18)) return false;
		if (!Ranglin(stduint(src), 0, 4)) return false;
		if (!dma[stduint(src)] || !data || !datalen) return false;
		self[DCR] = stduint(base) | (stduint(burst_len - 1) << 8);
		DMA1.bind = (pureptr_t)this;
		DMA1.XferCpltCallback = (src == TimBurstSrc::Update) ? _TIM_DMA_UpdateCplt : _TIM_DMA_CaptureCplt;
		DMA1.XferErrorCallback = _TIM_DMA_Error;
		// read direction: source = peripheral (DMAR), dest = memory (data)
		stduint dmar_addr = getBaseaddr() + _IMMx4(DMAR);
		if (!dma[stduint(src)]->Transfer(data, (pureptr_t)dmar_addr, datalen, method)) return false;
		self[DIER].setof(8 + stduint(src), true);
		return true;
	}

	// Stop DMA burst (aka HAL_TIM_DMABurst_WriteStop / ReadStop)
	void TIM_C::BurstStop(TimBurstSrc::TimBurstSrc src) {
		using namespace TimReg;
		if (!Ranglin(stduint(src), 0, 4)) return;
		if (dma[stduint(src)]) dma[stduint(src)]->Abort();
		self[DIER].setof(8 + stduint(src), false);// clear UDE/CCxDE
	}
#endif

#if defined(_MPU_STM32MP13)
	// Stop update-event DMA (aka HAL_TIM_Base_Stop_DMA)
	void TIM_t::StopUpdateDMA() {
		using namespace TimReg;
		self[DIER].setof(8, false);// UDE = 0
		if (dma[0]) dma[0]->Abort();
		enAble(false);// CEN = 0
	}

	// Stop encoder capture DMA (aka HAL_TIM_Encoder_Stop_DMA, both channels)
	void TIM_C::StopCaptureDMA() {
		using namespace TimReg;
		self[DIER].setof(1 + 8, false);// CC1DE = 0
		self[DIER].setof(2 + 8, false);// CC2DE = 0
		if (dma[1]) dma[1]->Abort();
		if (dma[2]) dma[2]->Abort();
		enCaptureCompareChannel(1, false);// CC1E = 0
		enCaptureCompareChannel(2, false);// CC2E = 0
		enAble(false);// CEN = 0
	}

	// Start Hall sensor interface DMA (aka HAL_TIMEx_HallSensor_Start_DMA): CCR1 -> memory
	bool TIM_C::HallSensorStartDMA(pureptr_t data, stduint leng, IOMethod method) {
		using namespace TimReg;
		if (!dma[1] || !data || !leng) return false;
		DMA1.bind = (pureptr_t)this;
		DMA1.XferCpltCallback = _TIM_DMA_CaptureCplt;
		DMA1.XferErrorCallback = _TIM_DMA_Error;
		stduint ccr1_addr = getBaseaddr() + _IMMx4(CCR1);
		if (!dma[1]->Transfer(data, (pureptr_t)ccr1_addr, leng, method)) return false;
		self[DIER].setof(1 + 8, true);// CC1DE = bit 9
		enCaptureCompareChannel(1, true);// CC1E = 1
		if (0x6 != self[SMCR].mask(0, 3)) enAble(true);// CEN (unless trigger mode)
		return true;
	}

	// Stop Hall sensor interface DMA (aka HAL_TIMEx_HallSensor_Stop_DMA)
	void TIM_C::HallSensorStopDMA() {
		using namespace TimReg;
		enCaptureCompareChannel(1, false);// CC1E = 0
		self[DIER].setof(1 + 8, false);// CC1DE = 0
		if (dma[1]) dma[1]->Abort();
		enAble(false);// CEN = 0
	}
#endif

	// Configure Hall sensor interface (aka HAL_TIMEx_HallSensor_Init params)
	void TIM_C::ConfigHallSensor(stduint delay, TimIcPol::TimIcPol pol, byte filter, byte prescaler) {
		using namespace TimReg;
		if (!Ranglin(filter, 0, 15)) return;
		if (!Ranglin(prescaler, 0, 3)) return;
		// CH1 as TRC input (XOR of the three hall inputs)
		ConfigChannelInn(1, TimChinSel::TRC, pol, filter, prescaler);
		// CH2 in PWM2 mode with commutation delay
		ConfigChannel(2, delay, 0x7);// PWM2
		// OC2REF as TRGO (MMS = 101)
		self[CR2].maset(_TIM_CR2_POSI_MMS, 3, 0x5);
	}

	// Enable/disable complementary channel N (aka HAL_TIMEx_OCN/PWMN/OnePulseN_Start/Stop/_IT/_DMA)
#if defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)
	bool TIM_C::enChannelN(byte channel, bool ena, IOMethod method) {
		using namespace TimReg;
		if (!Ranglin(channel, 1, 4)) return false;
		if (!(TIM_ID == 1 || TIM_ID == 8)) return false;// IS_TIM_CCXN_INSTANCE
		byte chan0x = 4 * (channel - 1);
		if (ena) {
			self[CCER].setof(chan0x + 2, true);// CCxNE = 1
			self[BDTR].setof(15, true);// MOE = 1
			if (method == IOMethod::Rupt) {
				self[DIER].setof(channel, true);// CCxIE
			}
			else if (method == IOMethod::DMA) {
				self[DIER].setof(channel + 8, true);// CCxDE
				if (dma[channel]) {
					DMA1.bind = (pureptr_t)this;
					DMA1.XferCpltCallback = _TIM_DMA_DelayPulseCplt;
					DMA1.XferErrorCallback = _TIM_DMA_Error;
				}
			}
			if (0x6 != self[SMCR].mask(0, 3)) enAble(true);// CEN (unless trigger mode)
		}
		else {
			self[CCER].setof(chan0x + 2, false);// CCxNE = 0
			self[BDTR].setof(15, false);// MOE = 0
			if (method == IOMethod::Rupt) {
				self[DIER].setof(channel, false);// CCxIE
			}
			else if (method == IOMethod::DMA) {
				self[DIER].setof(channel + 8, false);// CCxDE
				if (dma[channel]) dma[channel]->Abort();
			}
			enAble(false);// CEN = 0
		}
		return true;
	}
#endif

	// Configure commutation event (aka HAL_TIMEx_ConfigCommutEvent/_IT/_DMA)
	void TIM_C::ConfigCommutation(byte trigger, TimCommutSrc::TimCommutSrc src, IOMethod method) {
		using namespace TimReg;
		if (!(TIM_ID == 1 || TIM_ID == 8)) return;// IS_TIM_COMMUTATION_EVENT_INSTANCE
		if (!Ranglin(trigger, 0, 3)) return;
		self[SMCR].maset(4, 3, trigger);// TS = ITRx
		self[CR2].setof(3, true);// CCPC = 1 (capture/compare preload)
		self[CR2].setof(2, src == TimCommutSrc::Software);// CCUS
		if (method == IOMethod::Rupt) {
			self[DIER].setof(5, true);// COMIE
			self[DIER].setof(13, false);// COMDE
#if defined(_MPU_STM32MP13)
			GIC.enInterrupt(TIM_Request(TIM_ID, 5), true);
#else
			NVIC.setAble(TIM_Request(TIM_ID, 5), true);
#endif
		}
		else if (method == IOMethod::DMA) {
			self[DIER].setof(13, true);// COMDE
			self[DIER].setof(5, false);// COMIE
#if defined(_MPU_STM32MP13)
			GIC.enInterrupt(TIM_Request(TIM_ID, 5), false);
#else
			NVIC.setAble(TIM_Request(TIM_ID, 5), false);
#endif
		}
		else {
			self[DIER].setof(5, false);// COMIE
			self[DIER].setof(13, false);// COMDE
#if defined(_MPU_STM32MP13)
			GIC.enInterrupt(TIM_Request(TIM_ID, 5), false);
#else
			NVIC.setAble(TIM_Request(TIM_ID, 5), false);
#endif
		}
	}

	// Configure break & dead-time (aka HAL_TIMEx_ConfigBreakDeadTime)
	void TIM_C::ConfigBreakDeadTime(byte deadtime, byte lock, bool ossi, bool ossr, bool bke, bool bkp, bool aoe) {
		using namespace TimReg;
		if (!(TIM_ID == 1 || TIM_ID == 8)) return;// IS_TIM_BREAK_INSTANCE
		if (!Ranglin(lock, 0, 3)) return;
		stduint bdtr = stduint(deadtime) & 0xFF;// DTG
		bdtr |= (stduint(lock) & 0x3) << 8;// LOCK
		if (ossi) bdtr |= 0x00000400;// OSSI
		if (ossr) bdtr |= 0x00000800;// OSSR
		if (bke)  bdtr |= 0x00001000;// BKE
		if (bkp)  bdtr |= 0x00002000;// BKP
		if (aoe)  bdtr |= 0x00004000;// AOE
		self[BDTR] = bdtr;
	}

	// Configure remapping (aka HAL_TIMEx_RemapConfig)
	void TIM_C::ConfigRemap(stduint remap) {
		using namespace TimReg;
#if defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)
		self[AF1] = remap;// H7/MP13 remaps via AF1 (ETRSEL)
#else
		self[OR] = remap;// F1/F4 remaps via OR
#endif
	}

#if defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)
	// Configure break input (aka HAL_TIMEx_ConfigBreakInput)
	void TIM_C::ConfigBreakInput(TimBreakIn::TimBreakIn in, TimBreakSrc::TimBreakSrc src, bool ena, bool pol) {
		using namespace TimReg;
		if (!(TIM_ID == 1 || TIM_ID == 8)) return;// IS_TIM_BREAK_INSTANCE
		byte enable_pos = 0, polarity_pos = 0;
		switch (src) {
		case TimBreakSrc::Bkin:   enable_pos = 0; polarity_pos = 9;  break;
		case TimBreakSrc::Comp1:  enable_pos = 1; polarity_pos = 10; break;
		case TimBreakSrc::Comp2:  enable_pos = 2; polarity_pos = 11; break;
		case TimBreakSrc::Dfsdm1: enable_pos = 8; polarity_pos = 0;  break;// DFSDM1 has no polarity
		}
		TimReg::TimRegType trt = (in == TimBreakIn::Brk) ? AF1 : AF2;
		self[trt].setof(enable_pos, ena);
		if (src != TimBreakSrc::Dfsdm1) self[trt].setof(polarity_pos, pol);
	}

	// Configure channel 5 grouping (aka HAL_TIMEx_GroupChannel5)
	void TIM_C::ConfigGroupChannel5(stduint channels) {
		using namespace TimReg;
		if (!(TIM_ID == 1 || TIM_ID == 8)) return;// IS_TIM_COMBINED3PHASEPWM_INSTANCE
		if (!Ranglin(channels, 0, 3)) return;
		stduint mask = channels ? ((1U << channels) - 1) : 0;// GC5C1..GC5Cn
		self[CCR5].maset(29, 3, mask);// GC5C1~3
	}

	// Select TIx input source via TISEL register (aka HAL_TIMEx_TISelection)
	void TIM_C::ConfigTISelection(byte channel, byte sel) {
		using namespace TimReg;
		if (!Ranglin(channel, 1, 4)) return;
		if (!Ranglin(sel, 0, 15)) return;
		self[TISEL].maset((channel - 1) * 8, 4, sel);// TIxSEL[3:0]
	}
#endif

#if defined(_MPU_STM32MP13)
	// Disarm a break input in bidirectional mode (aka HAL_TIMEx_DisarmBreakInput)
	void TIM_C::DisarmBreak(TimBreakIn::TimBreakIn in) {
		using namespace TimReg;
		if (!(TIM_ID == 1 || TIM_ID == 8)) return;// IS_TIM_BREAK_INSTANCE
		if (in == TimBreakIn::Brk) {
			if (self[BDTR].bitof(28) && !self[BDTR].bitof(15)) self[BDTR].setof(26);// BKDSRM
		}
		else {
			if (self[BDTR].bitof(29) && !self[BDTR].bitof(15)) self[BDTR].setof(27);// BK2DSRM
		}
	}

	// Re-arm a break input in bidirectional mode (aka HAL_TIMEx_ReArmBreakInput)
	bool TIM_C::ReArmBreak(TimBreakIn::TimBreakIn in) {
		using namespace TimReg;
		if (!(TIM_ID == 1 || TIM_ID == 8)) return false;// IS_TIM_BREAK_INSTANCE
		byte dsrm = (in == TimBreakIn::Brk) ? 26 : 27;// BKDSRM / BK2DSRM
		byte bid  = (in == TimBreakIn::Brk) ? 28 : 29;// BKBID  / BK2BID
		if (!self[BDTR].bitof(bid)) return true;// not bidirectional: nothing to re-arm
		uint64 tickstart = SysTick::getTick();
		while (self[BDTR].bitof(dsrm)) {
			if ((SysTick::getTick() - tickstart) > 5) return false;// 5 ms timeout
		}
		return true;
	}
#endif

	// aka HAL_TIM_PWM_Start_IT / IC_Start_IT / OC_Start_IT (and Stop_IT variants)
	bool TIM_CHAN_t::enInterrupt(bool ena) {
		TIM_t* t = getParent();
		if (!t) return false;
		if (ena) {
			// Enable CCx interrupt in DIER
			t->enCCInterrupt(CHAN_ID, true);
			// Enable capture/compare channel (CCER)
			((TIM_C*)t)->enCaptureCompareChannel(CHAN_ID, true);
			// Enable main output for advanced timers (TIM1/8)
			if (TIM_ID == 1 || TIM_ID == 8) {
				(*t)[TimReg::BDTR] |= 0x00008000; // TIM_BDTR_MOE
				// Advanced timers route CC to a dedicated IRQ line.
#if defined(_MPU_STM32MP13)
				GIC.enInterrupt(TIM_Request(TIM_ID, 1), true);
#else
				NVIC.setAble(TIM_Request(TIM_ID, 1), true);
#endif
			}
			// Enable counter, except in slave-mode trigger
			if (0x6 != (*t)[TimReg::SMCR].mask(0, 3)) {
				t->enAble(true);
			}
		}
		else {
			// Disable CCx interrupt
			t->enCCInterrupt(CHAN_ID, false);
			// Disable capture/compare channel
			((TIM_C*)t)->enCaptureCompareChannel(CHAN_ID, false);
			// Disable main output for advanced timers
			if (TIM_ID == 1 || TIM_ID == 8) {
				(*t)[TimReg::BDTR] &= ~0x00008000U;
#if defined(_MPU_STM32MP13)
				GIC.enInterrupt(TIM_Request(TIM_ID, 1), false);
#else
				NVIC.setAble(TIM_Request(TIM_ID, 1), false);
#endif
			}
			// Stop counter
			t->enAble(false);
		}
		return true;
	}

#endif
}


