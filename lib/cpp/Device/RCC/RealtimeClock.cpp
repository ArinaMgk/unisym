// ASCII CPP TAB4 CRLF
// Docutitle: (Device) Real Time Clock
// Codifiers: @dosconio: 20240220 ~ 20240429
// Attribute: Arn-Covenant Any-Architect Bit-32mode Non-Dependence
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

#include "../../../../inc/c/driver/RealtimeClock.h"
#include "../../../../inc/cpp/Device/SysTick"

#if defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x)

using namespace uni;

// Unify IRQ numbers (F4 wakeup IRQ is named IRQ_RTC, H7 is IRQ_RTC_WKUP; both are 3)
#if defined(_MCU_STM32H7x)
#define _IRQ_RTC_ALARM IRQ_RTC_Alarm
#define _IRQ_RTC_WKUP  IRQ_RTC_WKUP
#else
#define _IRQ_RTC_ALARM IRQ_RTC_Alarm
#define _IRQ_RTC_WKUP  IRQ_RTC
#endif

// H7 RTC APB clock enable register (RCC_APB4ENR, bit 15 = RTCAPBEN)
#if defined(_MCU_STM32H7x)
#define _RCC_APB4ENR_ADDR (_RCC_BDCR_ADDR - 0x70U + 0xF4U)
#endif

namespace uni {
	RTC_t RTC;
}

// aka HAL RTC_ByteToBcd2
static byte _byteToBcd2(byte number) {
	byte bcdhigh = 0;
	while (number >= 10U) {
		bcdhigh++;
		number -= 10U;
	}
	return (byte)((bcdhigh << 4) | number);
}

// aka HAL RTC_Bcd2ToByte
static byte _bcd2ToByte(byte number) {
	byte tens = (byte)(((number & 0xF0U) >> 4) * 10U);
	return (byte)(tens + (number & 0x0FU));
}

namespace uni {

	// aka clock part of HAL_RTC_MspInit (DBP + LSE + RTCSEL + RTCEN [+ H7 RTCAPBEN])
	bool RTC_t::enClock(RTCClockSource src) const {
		// Enable backup domain write access (PWR_CR/CR1.DBP bit 8)
		Reference pwr(_PWR_CR_ADDR);
		pwr.setof(8, true);

		Reference bdcr(_RCC_BDCR_ADDR);
		// Enable LSE (BDCR.LSEON bit 0), wait for LSERDY bit 1
		bdcr.setof(0, true);
		uint64 tick = SysTick::getTick();
		while (!bdcr.bitof(1)) {
			if (SysTick::getTick() - tick > _RTC_TIMEOUT_VALUE) return false;
		}

		// Configure kernel clock source (BDCR.RTCSEL bit 9:8)
		bdcr.maset(8, 2, (stduint)src);
		// Enable RTC kernel clock (BDCR.RTCEN bit 15)
		bdcr.setof(15, true);
#if defined(_MCU_STM32H7x)
		// H7 additionally enables the RTC APB interface clock (RCC_APB4ENR.RTCAPBEN bit 15)
		Reference(_RCC_APB4ENR_ADDR).setof(15, true);
#endif
		return true;
	}

	// aka RTC_EnterInitMode
	bool RTC_t::enterInitMode() const {
		if (self[RTCReg::ISR].bitof(_RTC_ISR_POS_INITF)) return true;
		self[RTCReg::ISR].setof(_RTC_ISR_POS_INIT, true);
		uint64 tick = SysTick::getTick();
		while (!self[RTCReg::ISR].bitof(_RTC_ISR_POS_INITF)) {
			if (SysTick::getTick() - tick > _RTC_TIMEOUT_VALUE) return false;
		}
		return true;
	}

	// aka RTC_ExitInitMode
	bool RTC_t::exitInitMode() const {
		self[RTCReg::ISR].setof(_RTC_ISR_POS_INIT, false);
		if (!self[RTCReg::CR].bitof(_RTC_CR_POS_BYPSHAD)) {
			return waitSynchro();
		}
		return true;
	}

	// aka HAL_RTC_WaitForSynchro
	bool RTC_t::waitSynchro() const {
		self[RTCReg::ISR].rstof(_RTC_ISR_POS_RSF);
		uint64 tick = SysTick::getTick();
		while (!self[RTCReg::ISR].bitof(_RTC_ISR_POS_RSF)) {
			if (SysTick::getTick() - tick > _RTC_TIMEOUT_VALUE) return false;
		}
		return true;
	}

	// aka HAL_RTC_Init
	bool RTC_t::init(RTCHourFormat fmt, stduint asynchPrediv, stduint synchPrediv) const {
		// Return early if the calendar is already initialized
		if (self[RTCReg::ISR].bitof(_RTC_ISR_POS_INITS)) return true;

		wpDisable();
		bool ok = enterInitMode();
		if (ok) {
			// Clear FMT/OSEL/POL and set hour format (no output for now: OSEL/POL=0)
			self[RTCReg::CR].maset(_RTC_CR_POS_FMT, 1, (stduint)fmt);
			// Configure prescaler: PREDIV_S[14:0] + PREDIV_A[22:16]
			self[RTCReg::PRER].maset(_RTC_PRER_POS_S, 15, synchPrediv);
			self[RTCReg::PRER].maset(_RTC_PRER_POS_A, 7, asynchPrediv);
			ok = exitInitMode();
		}
		wpEnable();
		return ok;
	}

	// aka HAL_RTC_DeInit
	bool RTC_t::deinit() const {
		wpDisable();
		bool ok = enterInitMode();
		if (ok) {
			self[RTCReg::TR] = 0x00000000U;
			self[RTCReg::DR] = 0x00002101U;// WDU_0 | MU_0 | DU_0
			self[RTCReg::CR] = 0x00000000U;
			self[RTCReg::WUTR] = 0x0000FFFFU;
			self[RTCReg::PRER] = 0x007F00FFU;// PREDIV_A=0x7F | 0xFF
			self[RTCReg::ALRMAR] = 0x00000000U;
			self[RTCReg::ALRMBR] = 0x00000000U;
			ok = exitInitMode();
		}
		wpEnable();
		return ok;
	}

	// aka HAL_RTC_SetTime
	bool RTC_t::setTime(const datime_t& time, RTCFormat fmt) const {
		stduint tmpreg;
		if (fmt == RTCFormat::Bin) {
			tmpreg = ((stduint)_byteToBcd2(time.hour)   << _RTC_TR_POS_HU)
				| ((stduint)_byteToBcd2(time.minute) << _RTC_TR_POS_MNU)
				| (stduint)_byteToBcd2(time.second);
		}
		else {
			tmpreg = ((stduint)time.hour   << _RTC_TR_POS_HU)
				| ((stduint)time.minute << _RTC_TR_POS_MNU)
				| (stduint)time.second;
		}

		wpDisable();
		bool ok = enterInitMode();
		if (ok) {
			self[RTCReg::TR] = tmpreg;
			ok = exitInitMode();
		}
		wpEnable();
		return ok;
	}

	// aka HAL_RTC_GetTime
	bool RTC_t::getTime(datime_t& time, RTCFormat fmt) const {
		time.hour = (byte)self[RTCReg::TR].masof(_RTC_TR_POS_HU, 6);// HT+HU
		time.minute = (byte)self[RTCReg::TR].masof(_RTC_TR_POS_MNU, 7);// MNT+MNU
		time.second = (byte)self[RTCReg::TR].masof(_RTC_TR_POS_SU, 7);// ST+SU
		if (fmt == RTCFormat::Bin) {
			time.hour = _bcd2ToByte(time.hour);
			time.minute = _bcd2ToByte(time.minute);
			time.second = _bcd2ToByte(time.second);
		}
		return true;
	}

	// aka HAL_RTC_SetDate
	bool RTC_t::setDate(const datime_t& date, RTCFormat fmt) const {
		// datime: year(since 1900), month[0-11], mday; RTC: 2-digit year[0-99], month[1-12]
		byte year = (byte)(date.year - 100);
		byte month = (byte)(date.month + 1);
		byte day = date.mday;
		// unisym weekday(): 0=Sunday..6=Saturday; RTC: 1=Monday..7=Sunday
		byte wday = (byte)weekday((word)(date.year + 1900), (word)month, (word)day);
		if (!wday) wday = 7;

		stduint tmpreg;
		if (fmt == RTCFormat::Bin) {
			tmpreg = ((stduint)_byteToBcd2(year)  << _RTC_DR_POS_YU)
				| ((stduint)_byteToBcd2(month) << _RTC_DR_POS_MU)
				| (stduint)_byteToBcd2(day)
				| ((stduint)wday << _RTC_DR_POS_WDU);
		}
		else {
			tmpreg = ((stduint)year  << _RTC_DR_POS_YU)
				| ((stduint)month << _RTC_DR_POS_MU)
				| (stduint)day
				| ((stduint)wday << _RTC_DR_POS_WDU);
		}

		wpDisable();
		bool ok = enterInitMode();
		if (ok) {
			self[RTCReg::DR] = tmpreg;
			ok = exitInitMode();
		}
		wpEnable();
		return ok;
	}

	// aka HAL_RTC_GetDate
	bool RTC_t::getDate(datime_t& date, RTCFormat fmt) const {
		byte year = (byte)self[RTCReg::DR].masof(_RTC_DR_POS_YU, 8);// YT+YU
		byte month = (byte)self[RTCReg::DR].masof(_RTC_DR_POS_MU, 5);// MT+MU
		byte day = (byte)self[RTCReg::DR].masof(_RTC_DR_POS_DU, 6);// DT+DU
		if (fmt == RTCFormat::Bin) {
			year = _bcd2ToByte(year);
			month = _bcd2ToByte(month);
			day = _bcd2ToByte(day);
		}
		// RTC 2-digit year -> since 1900; RTC month[1-12] -> datime[0-11]
		date.year = (stdint)(year + 100);
		date.month = (byte)(month - 1);
		date.mday = day;
		return true;
	}

	// aka HAL_RTC_SetAlarm / HAL_RTC_SetAlarm_IT (enables alarm + interrupt bit; NVIC via enInterrupt)
	bool RTC_t::setAlarm(RTCAlarm alarm, const datime_t& time, RTCAlarmMask mask,
		byte dateOrWeekDay, RTCAlarmSel sel, RTCFormat fmt) const {
		bool isA = (alarm == RTCAlarm::AlarmA);
		RTCReg::RTCRegType alrmReg = isA ? RTCReg::ALRMAR : RTCReg::ALRMBR;
		byte aeBit = isA ? _RTC_CR_POS_ALRAE : _RTC_CR_POS_ALRBE;
		byte ieBit = isA ? _RTC_CR_POS_ALRAIE : _RTC_CR_POS_ALRBIE;
		byte wfBit = isA ? _RTC_ISR_POS_ALRAWF : _RTC_ISR_POS_ALRBWF;
		byte fBit = isA ? _RTC_ISR_POS_ALRAF : _RTC_ISR_POS_ALRBF;

		stduint tmpreg;
		if (fmt == RTCFormat::Bin) {
			tmpreg = ((stduint)_byteToBcd2(time.hour)   << _RTC_ALRM_POS_HU)
				| ((stduint)_byteToBcd2(time.minute) << _RTC_ALRM_POS_MNU)
				| (stduint)_byteToBcd2(time.second)
				| ((stduint)_byteToBcd2(dateOrWeekDay) << _RTC_ALRM_POS_DU);
		}
		else {
			tmpreg = ((stduint)time.hour   << _RTC_ALRM_POS_HU)
				| ((stduint)time.minute << _RTC_ALRM_POS_MNU)
				| (stduint)time.second
				| ((stduint)dateOrWeekDay << _RTC_ALRM_POS_DU);
		}
		tmpreg |= (stduint)sel | (stduint)mask;

		wpDisable();
		// Disable alarm and its interrupt, clear flag
		self[RTCReg::CR].setof(aeBit, false);
		self[RTCReg::CR].setof(ieBit, false);
		self[RTCReg::ISR].setof(fBit, false);
		// Wait for ALRxWF
		uint64 tick = SysTick::getTick();
		while (!self[RTCReg::ISR].bitof(wfBit)) {
			if (SysTick::getTick() - tick > _RTC_TIMEOUT_VALUE) { wpEnable(); return false; }
		}
		// Write ALRMxR and enable alarm + interrupt bit
		self[alrmReg] = tmpreg;
		self[RTCReg::CR].setof(aeBit, true);
		self[RTCReg::CR].setof(ieBit, true);
		wpEnable();
		return true;
	}

	// aka HAL_RTC_GetAlarm
	bool RTC_t::getAlarm(RTCAlarm alarm, datime_t& time, RTCFormat fmt) const {
		RTCReg::RTCRegType alrmReg = (alarm == RTCAlarm::AlarmA) ? RTCReg::ALRMAR : RTCReg::ALRMBR;
		time.hour = (byte)self[alrmReg].masof(_RTC_ALRM_POS_HU, 6);
		time.minute = (byte)self[alrmReg].masof(_RTC_ALRM_POS_MNU, 7);
		time.second = (byte)self[alrmReg].masof(_RTC_ALRM_POS_SU, 7);
		if (fmt == RTCFormat::Bin) {
			time.hour = _bcd2ToByte(time.hour);
			time.minute = _bcd2ToByte(time.minute);
			time.second = _bcd2ToByte(time.second);
		}
		return true;
	}

	// aka HAL_RTC_DeactivateAlarm
	bool RTC_t::deactivateAlarm(RTCAlarm alarm) const {
		bool isA = (alarm == RTCAlarm::AlarmA);
		byte aeBit = isA ? _RTC_CR_POS_ALRAE : _RTC_CR_POS_ALRBE;
		byte ieBit = isA ? _RTC_CR_POS_ALRAIE : _RTC_CR_POS_ALRBIE;
		byte wfBit = isA ? _RTC_ISR_POS_ALRAWF : _RTC_ISR_POS_ALRBWF;

		wpDisable();
		self[RTCReg::CR].setof(aeBit, false);
		self[RTCReg::CR].setof(ieBit, false);
		uint64 tick = SysTick::getTick();
		while (!self[RTCReg::ISR].bitof(wfBit)) {
			if (SysTick::getTick() - tick > _RTC_TIMEOUT_VALUE) { wpEnable(); return false; }
		}
		wpEnable();
		return true;
	}

	// aka HAL_RTCEx_SetWakeUpTimer / _IT
	bool RTC_t::setWakeUp(stduint counter, RTCWakeUpClock clk) const {
		wpDisable();
		self[RTCReg::CR].setof(_RTC_CR_POS_WUTE, false);
		self[RTCReg::ISR].setof(_RTC_ISR_POS_WUTF, false);
		uint64 tick = SysTick::getTick();
		while (!self[RTCReg::ISR].bitof(_RTC_ISR_POS_WUTWF)) {
			if (SysTick::getTick() - tick > _RTC_TIMEOUT_VALUE) { wpEnable(); return false; }
		}
		self[RTCReg::CR].maset(_RTC_CR_POS_WUCKSEL, 3, (stduint)clk);
		self[RTCReg::WUTR] = counter;
		self[RTCReg::CR].setof(_RTC_CR_POS_WUTE, true);
		self[RTCReg::CR].setof(_RTC_CR_POS_WUTIE, true);
		wpEnable();
		return true;
	}

	// aka HAL_RTCEx_GetWakeUpTimer
	stduint RTC_t::getWakeUp() const {
		return self[RTCReg::WUTR].masof(0, 16);
	}

	// aka HAL_RTCEx_DeactivateWakeUpTimer
	bool RTC_t::deactivateWakeUp() const {
		wpDisable();
		self[RTCReg::CR].setof(_RTC_CR_POS_WUTE, false);
		self[RTCReg::CR].setof(_RTC_CR_POS_WUTIE, false);
		uint64 tick = SysTick::getTick();
		while (!self[RTCReg::ISR].bitof(_RTC_ISR_POS_WUTWF)) {
			if (SysTick::getTick() - tick > _RTC_TIMEOUT_VALUE) { wpEnable(); return false; }
		}
		wpEnable();
		return true;
	}

	// ---- Interrupt interface (NVIC wiring of HAL_RTC_AlarmIRQHandler / WakeUpTimerIRQHandler) ----

	void RTC_t::setInterrupt(Handler_t f) const {
		FUNC_AlarmA = f;
	}

	void RTC_t::setInterruptPriority(byte preempt, byte sub_priority) const {
		NVIC.setPriority((Request_t)_IRQ_RTC_ALARM, preempt, sub_priority);
		NVIC.setPriority((Request_t)_IRQ_RTC_WKUP, preempt, sub_priority);
	}

	void RTC_t::enInterrupt(bool enable) const {
		NVIC.setAble((Request_t)_IRQ_RTC_ALARM, enable);
		NVIC.setAble((Request_t)_IRQ_RTC_WKUP, enable);
	}

}

#endif // _MCU_STM32F4x || _MCU_STM32H7x
