#include "../../../../inc/c/driver/RealtimeClock.h"
#include "../../../../inc/cpp/Device/EXTI"

#if defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x)
_ESYM_C{

	// aka HAL_RTC_AlarmIRQHandler (RTC Alarm A/B share EXTI line 17, IRQ 41)
	void RTC_Alarm_IRQHandler(void) {
#if defined(_MCU_STM32F4x)
		EXTI::Pending = _IMM1S(17);// write-1-to-clear
#elif defined(_MCU_STM32H7x)
		EXTI.Pending(0, 1) = _IMM1S(17);// D3 CPU1 PR1, write-1-to-clear
#endif
		if (uni::RTC[uni::RTCReg::ISR].bitof(_RTC_ISR_POS_ALRAF)
			&& uni::RTC[uni::RTCReg::CR].bitof(_RTC_CR_POS_ALRAIE)) {
			uni::RTC[uni::RTCReg::ISR].rstof(_RTC_ISR_POS_ALRAF);
			if (uni::RTC.FUNC_AlarmA) uni::RTC.FUNC_AlarmA();
		}
		if (uni::RTC[uni::RTCReg::ISR].bitof(_RTC_ISR_POS_ALRBF)
			&& uni::RTC[uni::RTCReg::CR].bitof(_RTC_CR_POS_ALRBIE)) {
			uni::RTC[uni::RTCReg::ISR].rstof(_RTC_ISR_POS_ALRBF);
			if (uni::RTC.FUNC_AlarmB) uni::RTC.FUNC_AlarmB();
		}
	}

	// aka HAL_RTCEx_WakeUpTimerIRQHandler (RTC wakeup has exclusive EXTI line 22, IRQ 3)
	void RTC_WKUP_IRQHandler(void) {
#if defined(_MCU_STM32F4x)
		EXTI::Pending = _IMM1S(22);// write-1-to-clear
#elif defined(_MCU_STM32H7x)
		EXTI.Pending(0, 1) = _IMM1S(22);// D3 CPU1 PR1, write-1-to-clear
#endif
		if (uni::RTC[uni::RTCReg::ISR].bitof(_RTC_ISR_POS_WUTF)) {
			uni::RTC[uni::RTCReg::ISR].rstof(_RTC_ISR_POS_WUTF);
			if (uni::RTC.FUNC_WakeUp) uni::RTC.FUNC_WakeUp();
		}
	}

	// aka HAL_RTCEx_TamperTimeStampIRQHandler (TimeStamp + Tamper share EXTI line 21, IRQ 2)
	void TAMP_STAMP_IRQHandler(void) {
#if defined(_MCU_STM32F4x)
		EXTI::Pending = _IMM1S(21);// write-1-to-clear
#elif defined(_MCU_STM32H7x)
		EXTI.Pending(0, 1) = _IMM1S(21);// D3 CPU1 PR1, write-1-to-clear
#endif
		if (uni::RTC[uni::RTCReg::ISR].bitof(_RTC_ISR_POS_TSF)
			&& uni::RTC[uni::RTCReg::CR].bitof(_RTC_CR_POS_TSIE)) {
			uni::RTC[uni::RTCReg::ISR].rstof(_RTC_ISR_POS_TSF);
			if (uni::RTC.FUNC_TimeStamp) uni::RTC.FUNC_TimeStamp();
		}
		if (uni::RTC[uni::RTCReg::ISR].bitof(_RTC_ISR_POS_TAMP1F)) {
			uni::RTC[uni::RTCReg::ISR].rstof(_RTC_ISR_POS_TAMP1F);
			if (uni::RTC.FUNC_Tamper1) uni::RTC.FUNC_Tamper1();
		}
		if (uni::RTC[uni::RTCReg::ISR].bitof(_RTC_ISR_POS_TAMP2F)) {
			uni::RTC[uni::RTCReg::ISR].rstof(_RTC_ISR_POS_TAMP2F);
			if (uni::RTC.FUNC_Tamper2) uni::RTC.FUNC_Tamper2();
		}
#if defined(_MCU_STM32H7x)
		if (uni::RTC[uni::RTCReg::ISR].bitof(_RTC_ISR_POS_TAMP3F)) {
			uni::RTC[uni::RTCReg::ISR].rstof(_RTC_ISR_POS_TAMP3F);
			if (uni::RTC.FUNC_Tamper3) uni::RTC.FUNC_Tamper3();
		}
#endif
	}

}
#endif
