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
#ifdef _INC_CPP
#include "../../cpp/reference"
#include "../../cpp/interrupt"
#endif

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

// ---- Additional bit positions (TimeStamp / calibration / DST / tamper) ----
#define _RTC_CR_POS_TSEDGE 3
#define _RTC_CR_POS_REFCKON 4
#define _RTC_CR_POS_DCE    7// F4 only (coarse calibration enable)
#define _RTC_CR_POS_TSE    11
#define _RTC_CR_POS_TSIE   15
#define _RTC_CR_POS_ADD1H  16
#define _RTC_CR_POS_SUB1H  17
#define _RTC_CR_POS_BKP    18
#define _RTC_CR_POS_COSEL  19
#define _RTC_CR_POS_COE    23
#define _RTC_CR_POS_ITSE   24// H7 only (internal timestamp)

#define _RTC_ISR_POS_SHPF    3
#define _RTC_ISR_POS_TSF     11
#define _RTC_ISR_POS_TSOVF   12
#define _RTC_ISR_POS_TAMP1F  13
#define _RTC_ISR_POS_TAMP2F  14
#define _RTC_ISR_POS_TAMP3F  15// H7 only
#define _RTC_ISR_POS_RECALPF 16
#define _RTC_ISR_POS_ITSF    17// H7 only

// CALR (smooth calibration)
#define _RTC_CALR_POS_CALM   0// 9 bits
#define _RTC_CALR_POS_CALW16 13
#define _RTC_CALR_POS_CALW8  14
#define _RTC_CALR_POS_CALP   15

// CALIBR (coarse calibration, F4 only)
#define _RTC_CALIBR_POS_DC  0// 5 bits
#define _RTC_CALIBR_POS_DCS 7

// SHIFTR
#define _RTC_SHIFTR_POS_SUBFS 0// 15 bits
#define _RTC_SHIFTR_POS_ADD1S 31

// Tamper register bits (TAFCR on F4, TAMPCR on H7; same offsets)
#define _RTC_TAMP_POS_TAMP1E   0
#define _RTC_TAMP_POS_TAMP1TRG 1
#define _RTC_TAMP_POS_TAMPIE   2
#define _RTC_TAMP_POS_TAMP2E   3
#define _RTC_TAMP_POS_TAMP2TRG 4
#define _RTC_TAMP_POS_TAMP3E   5// H7 only
#define _RTC_TAMP_POS_TAMP3TRG 6// H7 only
#define _RTC_TAMP_POS_TAMPTS   7
#define _RTC_TAMP_POS_TAMPFREQ 8// 3 bits
#define _RTC_TAMP_POS_TAMPFLT  11// 2 bits
#define _RTC_TAMP_POS_TAMPPRCH 13// 2 bits
#define _RTC_TAMP_POS_TAMPPUDIS 15
#define _RTC_TAMP_POS_TAMP1INSEL 16// F4 only
#if defined(_MCU_STM32H7x)
#define _RTC_TAMP_POS_TAMP1IE      16
#define _RTC_TAMP_POS_TAMP1NOERASE 17
#define _RTC_TAMP_POS_TAMP1MF      18
#define _RTC_TAMP_POS_TAMP2IE      19
#define _RTC_TAMP_POS_TAMP2NOERASE 20
#define _RTC_TAMP_POS_TAMP2MF      21
#define _RTC_TAMP_POS_TAMP3IE      22
#define _RTC_TAMP_POS_TAMP3NOERASE 23
#define _RTC_TAMP_POS_TAMP3MF      24
#endif

// ALRMASSR / ALRMBSSR (sub-second alarm)
#define _RTC_ALRMSSR_POS_SS     0// 15 bits
#define _RTC_ALRMSSR_POS_MASKSS 24// 4 bits

// Backup register base offset (BKP0R)
#define _RTC_BKUP_BASE 0x50U

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

	// TimeStamp edge (RTC_CR.TSEDGE)
	enum class RTCTimeStampEdge { Rising = 0, Falling = 1 };

	// TimeStamp pin (TAFCR.TSINSEL / TAMPCR)
	enum class RTCTimeStampPin { Default = 0, Pos1 = 1 };

	// Coarse calibration sign (CALIBR.DCS, F4 only)
	enum class RTCCalibSign { Positive = 0, Negative = 1 };

	// Smooth calibration period (CALR.CALW8/CALW16)
	enum class RTCSmoothCalibPeriod { _32sec = 0, _16sec = 1, _8sec = 2 };

	// Calibration output (CR.COSEL)
	enum class RTCCalibOutput { _512Hz = 0, _1Hz = 1 };

	// Tamper selection
	enum class RTCTamper { Tamper1 = 0, Tamper2 = 1, Tamper3 = 2 };

	// Tamper trigger (TAMPxTRG)
	enum class RTCTamperTrigger { RisingEdge = 0, FallingEdge = 1 };

	// Tamper filter (TAMPFLT)
	enum class RTCTamperFilter { Disable = 0, _2Sample = 1, _4Sample = 2, _8Sample = 3 };

	// Tamper sampling frequency (TAMPFREQ)
	enum class RTCTamperSamplingFreq {
		Div32768 = 0, Div16384, Div8192, Div4096,
		Div2048, Div1024, Div512, Div256
	};

	// Tamper precharge duration (TAMPPRCH)
	enum class RTCTamperPrecharge { _1RTCCLK = 0, _2RTCCLK = 1, _4RTCCLK = 2, _8RTCCLK = 3 };

	// Tamper 1 pin selection (TAMP1INSEL, F4 only)
	enum class RTCTamperPin { Default = 0, Pos1 = 1 };

	// Alarm sub-second mask (ALRMASSR.MASKSS; 0 = no sub-second compare, 15 = full compare)
	enum class RTCAlarmSubSecondMask : stduint {
		All = 0x00000000,
		SS14_1 = 0x01000000, SS14_2 = 0x02000000, SS14_3 = 0x03000000,
		SS14_4 = 0x04000000, SS14_5 = 0x05000000, SS14_6 = 0x06000000,
		SS14_7 = 0x07000000, SS14_8 = 0x08000000, SS14_9 = 0x09000000,
		SS14_10 = 0x0A000000, SS14_11 = 0x0B000000, SS14_12 = 0x0C000000,
		SS14_13 = 0x0D000000, SS14 = 0x0E000000,
		None = 0x0F000000
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
		mutable Handler_t FUNC_TimeStamp;
		mutable Handler_t FUNC_Tamper1;
		mutable Handler_t FUNC_Tamper2;
#if defined(_MCU_STM32H7x)
		mutable Handler_t FUNC_Tamper3;
#endif

		RTC_t(stduint _baseaddr = _RTC_ADDR) : baseaddr(_baseaddr) {
			FUNC_AlarmA = 0;
			FUNC_AlarmB = 0;
			FUNC_WakeUp = 0;
			FUNC_TimeStamp = 0;
			FUNC_Tamper1 = 0;
			FUNC_Tamper2 = 0;
#if defined(_MCU_STM32H7x)
			FUNC_Tamper3 = 0;
#endif
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

		// Alarm (datime_t: hour/minute/second; dateOrWeekDay is day[1-31] or weekday[1-7];
		//       subSeconds/subSecondMask configure the sub-second alarm ALRMASSR/ALRMBSSR)
		bool setAlarm(RTCAlarm alarm, const datime_t& time, RTCAlarmMask mask = RTCAlarmMask::None,
			byte dateOrWeekDay = 1, RTCAlarmSel sel = RTCAlarmSel::Date, RTCFormat fmt = RTCFormat::Bin,
			stduint subSeconds = 0, RTCAlarmSubSecondMask subSecondMask = RTCAlarmSubSecondMask::All) const;
		bool getAlarm(RTCAlarm alarm, datime_t& time, RTCFormat fmt = RTCFormat::Bin) const;
		bool deactivateAlarm(RTCAlarm alarm) const;

		// Wakeup timer
		bool setWakeUp(stduint counter, RTCWakeUpClock clk = RTCWakeUpClock::Div16) const;
		stduint getWakeUp() const;
		bool deactivateWakeUp() const;

		// Backup registers (BKP0R..BKPxR)
		void bkupWrite(byte index, stduint data) const;
		stduint bkupRead(byte index) const;

		// Calibration
	#if defined(_MCU_STM32F4x)
		// Coarse calibration (F4 only, CALIBR)
		bool setCoarseCalib(RTCCalibSign sign, byte value) const;
		bool deactivateCoarseCalib() const;
	#endif
		// Smooth calibration (CALR)
		bool setSmoothCalib(RTCSmoothCalibPeriod period, bool plusPulses, stduint minusValue) const;
		// Synchronization shift (SHIFTR)
		bool setSynchroShift(bool add1S, stduint subFS) const;
		// Calibration output (CR.COE/COSEL)
		bool setCalibrationOutPut(RTCCalibOutput output) const;
		bool deactivateCalibrationOutPut() const;
		// Reference clock detection (CR.REFCKON)
		bool setRefClock() const;
		bool deactivateRefClock() const;
		// Bypass shadow (CR.BYPSHAD)
		bool enableBypassShadow() const;
		bool disableBypassShadow() const;

		// TimeStamp (TSTR/TSDR/TSSSR)
		bool setTimeStamp(RTCTimeStampEdge edge, RTCTimeStampPin pin, bool it = false) const;
		bool deactivateTimeStamp() const;
		bool getTimeStamp(datime_t& time, datime_t& date, RTCFormat fmt = RTCFormat::Bin) const;
	#if defined(_MCU_STM32H7x)
		// Internal time stamp (H7 only, CR.ITSE)
		bool setInternalTimeStamp() const;
		bool deactivateInternalTimeStamp() const;
	#endif

		// Tamper (TAFCR on F4 / TAMPCR on H7)
		bool setTamper(RTCTamper tamper, RTCTamperTrigger trigger = RTCTamperTrigger::RisingEdge, bool it = false,
			RTCTamperFilter filter = RTCTamperFilter::Disable,
			RTCTamperSamplingFreq freq = RTCTamperSamplingFreq::Div32768,
			RTCTamperPrecharge precharge = RTCTamperPrecharge::_1RTCCLK,
			bool pullUp = true, bool timeStampOnTamper = false
	#if defined(_MCU_STM32F4x)
			, RTCTamperPin pin = RTCTamperPin::Default
	#endif
	#if defined(_MCU_STM32H7x)
			, bool noErase = false, bool maskFlag = false
	#endif
		) const;
		bool deactivateTamper(RTCTamper tamper) const;

		// Daylight saving time (CR.ADD1H/SUB1H)
		void add1Hour() const;
		void sub1Hour() const;

		_COM_DEF_Interrupt_Interface();
	};

	extern RTC_t RTC;
}

#elif defined(_MCU_STM32F1x) // C++



#endif

#endif
