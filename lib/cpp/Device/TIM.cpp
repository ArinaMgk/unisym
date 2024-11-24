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

namespace uni {
#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x)
	
	void TIM_t::setInterrupt(Handler_t fn) const {
		FUNC_TIMx[getID()] = fn;
	}
	static Request_t TIM_Request_list[16] = {
		Request_None, Request_None, IRQ_TIM2, IRQ_TIM3,
		IRQ_TIM4, IRQ_TIM5, IRQ_TIM6, IRQ_TIM7,
	};
	void TIM_t::setInterruptPriority(byte preempt, byte sub_priority) const {
		NVIC.setPriority(TIM_Request_list[getID()], preempt, sub_priority);
	}
	static void timer_it(byte TIM_ID, bool enable);
	void TIM_t::enInterrupt(bool enable) const {
		if (enable) {
			NVIC.setAble(TIM_Request_list[getID()]);// dest
		}
		timer_it(getID(), enable);// sors
	}

	
	#if defined(_MCU_STM32F4x)
	bool TIM_CHAN_t::setMode(stduint compare, GPIO_Pin* pin) {
		Letvar(addr, TIM_C*, TIM[TIM_ID]);
		return addr->setChannel(CHAN_ID, compare, pin);
	}
	
	TIM_CHAN_t TIM_t::operator[](stduint chan_id) {
		return TIM_CHAN_t(TIM_ID, chan_id);
	}

	struct lock_timc {
		byte TIM_ID;
		lock_timc(byte _TIM_ID) : TIM_ID(_TIM_ID) { TIM[_TIM_ID]->enAble(false); }
		~lock_timc() { TIM[TIM_ID]->enAble(); }
	};

	#endif

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

#elif defined(_MCU_STM32F4x)

	static const uint32 _REFADDR_TIM[] = { nil,
		0x40010000, 0x40000000, 0x40000400, 0x40000800, // T 1 -> 4
		0x40000C00, 0x40001000, 0x40001400, 0x40010400, // T 5 -> 8
		0x40014000, 0x40014400, 0x40014800, 0x40001800, // T 9 -> 12
		0x40001C00, 0x40002000 // T 13 -> 14
	};
	
	TIM_C TIM2(_REFADDR_TIM[2], 2);
	TIM_C TIM3(_REFADDR_TIM[3], 3);
	TIM_C TIM4(_REFADDR_TIM[4], 4);
	TIM_C TIM5(_REFADDR_TIM[5], 5);
	// STATIC : 0x40000000 + 0x400 * (TIM_ID-2) : IF TIM_ID IN 2..6
	TIM_t* TIM[] = { nullptr,
		nullptr, (TIM_t*)(pureptr_t)&TIM2,(TIM_t*)(pureptr_t)&TIM3,
		(TIM_t*)(pureptr_t)&TIM4,(TIM_t*)(pureptr_t)&TIM5,
	};

	//{TODO} a channel may connect multiple pins
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
		1,1, 2,2,2, // TIM1~5: AF1 for TIM1&2
		0xFF, 0xFF, // TIM6,7
		3,3,3,3
	};//{ONLY}  F407 & F417

	static TimReg::TimRegType _tab_timregs_ccr[] = {
		TimReg::CCR1, TimReg::CCR2, TimReg::CCR3, TimReg::CCR4
	};
	static void timer_it(byte TIM_ID, bool enable) {
		using namespace TimReg;
		TIM_t& sel = *TIM[TIM_ID];
		sel[DIER] |= 1;// TIM_IT_UPDATE
		if ((sel[SMCR] & 0x7) == 0x6) // TIM_SLAVEMODE_TRIGGER is TIM_SMCR_SMS
			sel.enAble();
	}

	void TIM_C::setMode(stduint prescaler, stduint period , bool auto_reload_preload) {
		using namespace TimReg;
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
	}

	// preset: Tim.setMode, e.g.(pres, period);
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

	// aka HAL_TIM_PWM_ConfigChannel
	void TIM_C::ConfigChannel(byte channel, stduint pulse) {
		using namespace TimReg;
		_TEMP stduint TIM_OCMODE_PWM1 = 0x6;// 0b110
		//{TEMP} OCFastMode = TIM_OCFAST_DISABLE; (0x00000000U)
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
			self[trt].maset(4 + shift, 3, TIM_OCMODE_PWM1);// OCxM
			self[trt].maset(0 + shift, 2, nil);// CCxS
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
		self[trt] |= _IMM(0x00000008 << shift);// OCxPE
		self[trt] &= ~_IMM(0x00000004 << shift);// OCxFE; // TIM_OCFAST_DISABLE; (0x00000000U)
	}


	_TEMP void TIM_C::Select(stduint SlaveMode, stduint TriggerInn, stduint TriggerOut, bool MasterSlaveMode)
	{
		using namespace TimReg;
		self[SMCR].maset(0, 3, SlaveMode);// TIM_SelectSlaveMode
		self[SMCR].maset(4, 3, TriggerInn);// TIM_SelectInputTrigger
		self[CR2].maset(_TIM_CR2_POSI_MMS, 3, TriggerOut);// TIM_SelectOutputTrigger
		self[SMCR].setof(7, MasterSlaveMode);// TIM_SelectMasterSlaveMode
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

#endif
}


