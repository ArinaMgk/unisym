// ASCII CPP-ISO11 TAB4 CRLF
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

#include "../../../../inc/c/datime.h"
#include "../../../../inc/c/driver/i8259A.h"
#include "../../../../inc/c/driver/RealtimeClock.h"

#if defined(_MCCA) && (_MCCA==0x8616||_MCCA==0x8632)

void RTC_Init()// <=> Kasha TimerInit16 
{
	outpb(PORT_CMOS_ADDR, 0x8B);// mgk: RTC Register B and NMI
	outpb(PORT_CMOS_DATA, 0x12);// mgk: Set Reg-B {Ban Periodic, Open Update-Int, BCD, 24h}
	i8259Slaver_Enable(DEV_SLV_RTC);
	outpb(PORT_CMOS_ADDR, 0x0C);// mgk: ?
	innpb(PORT_CMOS_DATA);// Read Reg-C, reset pending interrupt
	// Check PIC Device?
}

inline static byte CMOS_Get(byte addr)
{
	outpb(PORT_CMOS_ADDR, CMOS_NMI | addr);
	return innpb(PORT_CMOS_DATA);
}
inline static void CMOS_Set(byte addr, byte data)
{
	outpb(PORT_CMOS_ADDR, CMOS_NMI | addr);
	outpb(PORT_CMOS_DATA, data);
}
inline static uint8 bcd_to_bin(uint8 value)
{
	return (value & 0xf) + (value >> 4) * 10;
}
inline static uint8 bin_to_bcd(uint8 value)
{
	return (value / 10) * 0x10 + (value % 10);
}

void CMOS_Readtime(struct tm* time)
{
	do // read the time, and the delta is less than 1s
	{
		time->tm_sec = CMOS_Get(CMOS_SECOND);
		time->tm_min = CMOS_Get(CMOS_MINUTE);
		time->tm_hour = CMOS_Get(CMOS_HOUR);
		time->tm_wday = CMOS_Get(CMOS_WDAY);
		time->tm_mday = CMOS_Get(CMOS_MDAY);
		time->tm_mon = CMOS_Get(CMOS_MONTH);
		time->tm_year = CMOS_Get(CMOS_YEAR);
		// century = CMOS_Get(CMOS_CENTURY);
	} while (time->tm_sec != CMOS_Get(CMOS_SECOND));
	//
	time->tm_sec = bcd_to_bin(time->tm_sec);
	time->tm_min = bcd_to_bin(time->tm_min);
	time->tm_hour = bcd_to_bin(time->tm_hour);
	time->tm_wday = bcd_to_bin(time->tm_wday);
	time->tm_mday = bcd_to_bin(time->tm_mday);
	time->tm_mon = bcd_to_bin(time->tm_mon);
	time->tm_year = bcd_to_bin(time->tm_year) + 2000;
	time->tm_yday = herspan(time->tm_year, time->tm_mon, time->tm_mday) - herspan(time->tm_year, 1, 1);
	time->tm_isdst = -1;
	// century = bcd_to_bin(century);
}

//{unchk}
// Book a interrupt after n seconds
void RTC_SetAlarm(uint32 secs)
{
	struct tm time;
	CMOS_Readtime(&time);
	byte sec = secs % 60;
	secs /= 60;
	byte min = secs % 60;
	secs /= 60;
	uint32 hour = secs;
	time.tm_sec += sec;
	if (time.tm_sec >= 60) {
		time.tm_sec %= 60;
		time.tm_min += 1;
	}
	time.tm_min += min;
	if (time.tm_min >= 60) {
		time.tm_min %= 60;
		time.tm_hour += 1;
	}
	time.tm_hour += hour;
	if (time.tm_hour >= 24) {
		time.tm_hour %= 24;
	}
	CMOS_Set(CMOS_HOUR, bin_to_bcd(time.tm_hour));
	CMOS_Set(CMOS_MINUTE, bin_to_bcd(time.tm_min));
	CMOS_Set(CMOS_SECOND, bin_to_bcd(time.tm_sec));
	CMOS_Set(CMOS_STATUS_B, 0b00100010); // Open Alarm Int
	CMOS_Get(CMOS_STATUS_C);// Read Reg-C, reset pending interrupt
}

#endif
