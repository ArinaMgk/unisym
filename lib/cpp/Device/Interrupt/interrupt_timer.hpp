#include "../../../../inc/cpp/Device/TIM"



#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x) || defined(_MPU_STM32MP13) || defined(_MCU_STM32H7x)
_ESYM_C{
	//: typ: 0:Basic, 1:CaptureCompare, 2:Update, 3:Break, 4:Trigger, 5:Commutation
	static void _HandlerIRQ_TIMx(byte typ, byte TIM_ID);

	Handler_t FUNC_TIMx[16] = { 0 }; // Period Elapsed Callback

	void TIM2_IRQHandler(void) { _HandlerIRQ_TIMx(0, 2); }
	void TIM3_IRQHandler(void) { _HandlerIRQ_TIMx(0, 3); }
	void TIM4_IRQHandler(void) { _HandlerIRQ_TIMx(0, 4); }
#if defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)
	void TIM5_IRQHandler(void) { _HandlerIRQ_TIMx(0, 5); }
#endif

#ifdef _MCU_STM32F1x
	void TIM6_IRQHandler(void) { _HandlerIRQ_TIMx(0, 6); }
#elif defined(_MCU_STM32F4x)
	void TIM1_BRK_TIM9_IRQHandler(void) { _HandlerIRQ_TIMx(3, 1); }
	void TIM1_UP_TIM10_IRQHandler(void) { _HandlerIRQ_TIMx(2, 1); }
	void TIM1_TRG_COM_TIM11_IRQHandler(void) { _HandlerIRQ_TIMx(4, 1); _HandlerIRQ_TIMx(5, 1); }
	void TIM1_CC_IRQHandler(void) { _HandlerIRQ_TIMx(1, 1); }
	void TIM8_BRK_TIM12_IRQHandler(void) { _HandlerIRQ_TIMx(3, 8); }
	void TIM8_UP_TIM13_IRQHandler(void) { _HandlerIRQ_TIMx(2, 8); }
	void TIM8_TRG_COM_TIM14_IRQHandler(void) { _HandlerIRQ_TIMx(4, 8); _HandlerIRQ_TIMx(5, 8); }
	void TIM8_CC_IRQHandler(void) { _HandlerIRQ_TIMx(1, 8); }
	void TIM6_DAC_IRQHandler(void) { _HandlerIRQ_TIMx(0, 6); }
	void TIM7_IRQHandler(void) { _HandlerIRQ_TIMx(0, 7); }
#elif defined(_MCU_STM32H7x)
	void TIM1_BRK_IRQHandler(void) { _HandlerIRQ_TIMx(3, 1); }
	void TIM1_UP_IRQHandler(void) { _HandlerIRQ_TIMx(2, 1); }
	void TIM1_TRG_COM_IRQHandler(void) { _HandlerIRQ_TIMx(4, 1); _HandlerIRQ_TIMx(5, 1); }
	void TIM1_CC_IRQHandler(void) { _HandlerIRQ_TIMx(1, 1); }
	void TIM8_BRK_TIM12_IRQHandler(void) { _HandlerIRQ_TIMx(3, 8); }
	void TIM8_UP_TIM13_IRQHandler(void) { _HandlerIRQ_TIMx(2, 8); }
	void TIM8_TRG_COM_TIM14_IRQHandler(void) { _HandlerIRQ_TIMx(4, 8); _HandlerIRQ_TIMx(5, 8); }
	void TIM8_CC_IRQHandler(void) { _HandlerIRQ_TIMx(1, 8); }
	void TIM6_DAC_IRQHandler(void) { _HandlerIRQ_TIMx(0, 6); }
	void TIM7_IRQHandler(void) { _HandlerIRQ_TIMx(0, 7); }
#elif defined(_MPU_STM32MP13)
	void TIM1_BRK_IRQHandler(void) { _HandlerIRQ_TIMx(3, 1); }
	void TIM1_UP_IRQHandler(void) { _HandlerIRQ_TIMx(2, 1); }
	void TIM1_TRG_COM_IRQHandler(void) { _HandlerIRQ_TIMx(4, 1); _HandlerIRQ_TIMx(5, 1); }
	void TIM1_CC_IRQHandler(void) { _HandlerIRQ_TIMx(1, 1); }
	void TIM8_BRK_IRQHandler(void) { _HandlerIRQ_TIMx(3, 8); }
	void TIM8_UP_IRQHandler(void) { _HandlerIRQ_TIMx(2, 8); }
	void TIM8_TRG_COM_IRQHandler(void) { _HandlerIRQ_TIMx(4, 8); _HandlerIRQ_TIMx(5, 8); }
	void TIM8_CC_IRQHandler(void) { _HandlerIRQ_TIMx(1, 8); }
	void TIM6_IRQHandler(void) { _HandlerIRQ_TIMx(0, 6); }
	void TIM7_IRQHandler(void) { _HandlerIRQ_TIMx(0, 7); }
	void TIM12_IRQHandler(void) {}
	void TIM13_IRQHandler(void) {}
	void TIM14_IRQHandler(void) {}
	void TIM15_IRQHandler(void) {}
	void TIM16_IRQHandler(void) {}
	void TIM17_IRQHandler(void) {}
	// Low Power Timer
	void LPTIM1_IRQHandler(void) {}
	void LPTIM2_IRQHandler(void) {}
	void LPTIM3_IRQHandler(void) {}
	void LPTIM4_IRQHandler(void) {}
	void LPTIM5_IRQHandler(void) {}
#endif
}
#endif


#if defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)
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
	if (_HandlerIRQ_TIMx_Exist(TIM_ID, chan)) {
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
#endif

static void _HandlerIRQ_TIMx(byte typ, byte TIM_ID) {
#if 0
#elif defined(_MCU_STM32F1x)
	using namespace TimReg;
	if (TIM_ID != 6) _TEMP return;

	TIM_t& this_TIM = treat<TIM_t>(TIM[TIM_ID]);
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
#elif defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)
	using namespace TimReg;
	if (!TIM[TIM_ID]) _TEMP return;
	// typ: 0:Global, 1:CaptureCompare, 2:Update, 3:Break, 4:Trigger, 5:Commutation
	if (typ == 0) {
		// Single global IRQ line: general timers (TIM2~5) and basic timers (TIM6/7).
		if (TIM_ID == 6 || TIM_ID == 7) {
			if (_HandlerIRQ_TIMx_Exist(TIM_ID, 0)) { // UIF
				callif(FUNC_TIMx[TIM_ID]);
			}
			_TEMP return;
		}
		TIM_t& t = i_index(TIM, TIM_ID);
		_HandlerIRQ_TIMx_Channel(TIM_ID, 1);
		_HandlerIRQ_TIMx_Channel(TIM_ID, 2);
		_HandlerIRQ_TIMx_Channel(TIM_ID, 3);
		_HandlerIRQ_TIMx_Channel(TIM_ID, 4);
		if (_HandlerIRQ_TIMx_Exist(TIM_ID, 0)) { // UIF
			callif(FUNC_TIMx[TIM_ID]);
			TIM[TIM_ID]->enAble();
		}
		if (_HandlerIRQ_TIMx_Exist(TIM_ID, 7)) callif(t.FUNC_Break);
		if (_HandlerIRQ_TIMx_Exist(TIM_ID, 6)) callif(t.FUNC_Trigger);
		if (_HandlerIRQ_TIMx_Exist(TIM_ID, 5)) callif(t.FUNC_Commute);
		_TEMP return;
	}
	// Advanced timers (TIM1/8): dedicated BRK/UP/TRG_COM/CC IRQ lines.
	if (TIM_ID != 1 && TIM_ID != 8) _TEMP return;
	{
		TIM_t& t = i_index(TIM, TIM_ID);
		switch (typ) {
		case 1:
			_HandlerIRQ_TIMx_Channel(TIM_ID, 1);
			_HandlerIRQ_TIMx_Channel(TIM_ID, 2);
			_HandlerIRQ_TIMx_Channel(TIM_ID, 3);
			_HandlerIRQ_TIMx_Channel(TIM_ID, 4);
			break;
		case 2:
			if (_HandlerIRQ_TIMx_Exist(TIM_ID, 0)) { // UIF
				callif(FUNC_TIMx[TIM_ID]);
				TIM[TIM_ID]->enAble();
			}
			break;
		case 3:
			if (_HandlerIRQ_TIMx_Exist(TIM_ID, 7)) callif(t.FUNC_Break);
			break;
		case 4:
			if (_HandlerIRQ_TIMx_Exist(TIM_ID, 6)) callif(t.FUNC_Trigger);
			break;
		case 5:
			if (_HandlerIRQ_TIMx_Exist(TIM_ID, 5)) callif(t.FUNC_Commute);
			break;
		}
	}
#endif
}

#if defined(_MPU_STM32MP13)
void HypervisorTimer_IRQHandler(void) {}
void VirtualTimer_IRQHandler(void) {}
#endif
