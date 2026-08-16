// ASCII CPL TAB4 CRLF
// Docutitle: (Module) Real Time Clock
// Codifiers: @dosconio: 20240220 ~ 20240429
// Attribute: Arn-Covenant Any-Architect Bit-32mode Non-Dependence
// Copyright: UNISYM, under Apache License 2.0; Dosconio Mecocoa, BSD 3-Clause License
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

#ifndef _INC_DEVICE_RealtimeClock
#define _INC_DEVICE_RealtimeClock

#include "../stdinc.h"
#include "../datime.h"
#include "../../cpp/reference"
#include "../../cpp/interrupt"

#if defined(_MCCA) && (_MCCA==0x8616||_MCCA==0x8632)

#ifdef _INC_CPP
extern "C" {
#endif

void CMOS_Readtime(struct tm* time);

void RTC_Init();

void RTC_SetAlarm(uint32 secs);

#ifdef _INC_CPP
}
#endif


#elif defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x)

// ---- Base addresses ----
#if defined(_MCU_STM32F4x)
#define _RTC_ADDR       0x40002800U
#define _RCC_BDCR_ADDR  0x40023870U
#define _PWR_CR_ADDR    0x40007000U
#elif defined(_MCU_STM32H7x)
#define _RTC_ADDR       0x58004000U
#define _RCC_BDCR_ADDR  0x58024470U
#define _PWR_CR_ADDR    0x58024800U
#endif

// ---- Bit positions of RTC_CR / RTC_ISR / RTC_TR / RTC_DR / RTC_ALRM / RTC_PRER (common to F4/H7) ----
#define _RTC_CR_POS_WUCKSEL 0
#define _RTC_CR_POS_BYPSHAD 5
#define _RTC_CR_POS_FMT     6
#define _RTC_CR_POS_ALRAE   8
#define _RTC_CR_POS_ALRBE   9
#define _RTC_CR_POS_WUTE    10
#define _RTC_CR_POS_ALRAIE  12
#define _RTC_CR_POS_ALRBIE  13
#define _RTC_CR_POS_WUTIE   14

#define _RTC_ISR_POS_ALRAWF 0
#define _RTC_ISR_POS_ALRBWF 1
#define _RTC_ISR_POS_WUTWF  2
#define _RTC_ISR_POS_INITS  4
#define _RTC_ISR_POS_RSF    5
#define _RTC_ISR_POS_INITF  6
#define _RTC_ISR_POS_INIT   7
#define _RTC_ISR_POS_ALRAF  8
#define _RTC_ISR_POS_ALRBF  9
#define _RTC_ISR_POS_WUTF   10

#define _RTC_TR_POS_SU   0
#define _RTC_TR_POS_ST   4
#define _RTC_TR_POS_MNU  8
#define _RTC_TR_POS_MNT  12
#define _RTC_TR_POS_HU   16
#define _RTC_TR_POS_HT   20
#define _RTC_TR_POS_PM   22

#define _RTC_DR_POS_DU   0
#define _RTC_DR_POS_DT   4
#define _RTC_DR_POS_MU   8
#define _RTC_DR_POS_MT   12
#define _RTC_DR_POS_WDU  13
#define _RTC_DR_POS_YU   16
#define _RTC_DR_POS_YT   20

#define _RTC_ALRM_POS_SU    0
#define _RTC_ALRM_POS_ST    4
#define _RTC_ALRM_POS_MSK1  7
#define _RTC_ALRM_POS_MNU   8
#define _RTC_ALRM_POS_MNT   12
#define _RTC_ALRM_POS_MSK2  15
#define _RTC_ALRM_POS_HU    16
#define _RTC_ALRM_POS_HT    20
#define _RTC_ALRM_POS_PM    22
#define _RTC_ALRM_POS_MSK3  23
#define _RTC_ALRM_POS_DU    24
#define _RTC_ALRM_POS_DT    28
#define _RTC_ALRM_POS_WDSEL 30
#define _RTC_ALRM_POS_MSK4  31

#define _RTC_PRER_POS_S 0
#define _RTC_PRER_POS_A 16

#define _RTC_TIMEOUT_VALUE 1000U

namespace uni {

	// Register map (calendar register offsets identical on F4/H7)
	namespace RTCReg {
		typedef enum {
			TR = 0, DR, CR, ISR, PRER, WUTR,
			CALIBR,// 0x18 F4 only (reserved on H7)
			ALRMAR, ALRMBR, WPR, SSR, SHIFTR,
			TSTR, TSDR, TSSSR, CALR,
			TAFCR,// 0x40 F4=TAFCR, H7=TAMPCR
			ALRMASSR, ALRMBSSR
		} RTCRegType;
	}

	// Hour format (RTC_CR.FMT)
	enum class RTCHourFormat { Hour24 = 0, Hour12 = 1 };

	// Parameter format (aka HAL RTC_FORMAT_BIN / RTC_FORMAT_BCD)
	enum class RTCFormat { Bin, Bcd };

	// RTC kernel clock source (BDCR.RTCSEL)
	enum class RTCClockSource { LSE = 1, LSI = 2, HSE = 3 };

	// Alarm selection
	enum class RTCAlarm { AlarmA = 0, AlarmB = 1 };

	// Alarm mask (ALRMxR.MSK1~4)
	enum class RTCAlarmMask : stduint {
		None = 0x00000000,
		Seconds = 0x00000080,
		Minutes = 0x00008080,
		Hours = 0x00808080,
		DateWeekDay = 0x80808080,
		All = 0x80808080
	};

	// Alarm day match: by date or weekday (ALRMxR.WDSEL)
	enum class RTCAlarmSel : stduint { Date = 0, WeekDay = 0x40000000 };

	// Wakeup clock source (RTC_CR.WUCKSEL)
	enum class RTCWakeUpClock {
		Div16 = 0, Div8, Div4, Div2,
		Spare1Hz = 4, Spare1HzExt = 6
	};

	class RTC_t : public RuptTrait {
	protected:
		stduint baseaddr;

		void wpDisable() const { self[RTCReg::WPR] = 0xCA; self[RTCReg::WPR] = 0x53; }
		void wpEnable()  const { self[RTCReg::WPR] = 0xFF; }
		bool enterInitMode() const;
		bool exitInitMode() const;
	public:
		mutable Handler_t FUNC_AlarmA;
		mutable Handler_t FUNC_AlarmB;
		mutable Handler_t FUNC_WakeUp;

		RTC_t(stduint _baseaddr = _RTC_ADDR) : baseaddr(_baseaddr) {
			FUNC_AlarmA = 0;
			FUNC_AlarmB = 0;
			FUNC_WakeUp = 0;
		}
		Reference operator[](RTCReg::RTCRegType reg) const { return Reference(baseaddr + _IMMx4(reg)); }
		stduint getBaseaddr() const { return baseaddr; }

		// Enable RTC kernel clock (aka clock part of HAL_RTC_MspInit + HAL_RCCEx_PeriphCLKConfig)
		bool enClock(RTCClockSource src = RTCClockSource::LSE) const;

		// aka HAL_RTC_Init / HAL_RTC_DeInit
		bool init(RTCHourFormat fmt = RTCHourFormat::Hour24, stduint asynchPrediv = 0x7F, stduint synchPrediv = 0xFF) const;
		bool deinit() const;

		// aka HAL_RTC_WaitForSynchro
		bool waitSynchro() const;

		// Time (datime_t: hour/minute/second)
		bool setTime(const datime_t& time, RTCFormat fmt = RTCFormat::Bin) const;
		bool getTime(datime_t& time, RTCFormat fmt = RTCFormat::Bin) const;
		// Date (datime_t: year/month[0-11]/mday)
		bool setDate(const datime_t& date, RTCFormat fmt = RTCFormat::Bin) const;
		bool getDate(datime_t& date, RTCFormat fmt = RTCFormat::Bin) const;

		// Alarm (datime_t: hour/minute/second; dateOrWeekDay is day[1-31] or weekday[1-7])
		bool setAlarm(RTCAlarm alarm, const datime_t& time, RTCAlarmMask mask = RTCAlarmMask::None,
			byte dateOrWeekDay = 1, RTCAlarmSel sel = RTCAlarmSel::Date, RTCFormat fmt = RTCFormat::Bin) const;
		bool getAlarm(RTCAlarm alarm, datime_t& time, RTCFormat fmt = RTCFormat::Bin) const;
		bool deactivateAlarm(RTCAlarm alarm) const;

		// Wakeup timer
		bool setWakeUp(stduint counter, RTCWakeUpClock clk = RTCWakeUpClock::Div16) const;
		stduint getWakeUp() const;
		bool deactivateWakeUp() const;

		_COM_DEF_Interrupt_Interface();
	};

	extern RTC_t RTC;
}

#elif defined(_MCU_STM32F1x) // C++



#endif

#endif
