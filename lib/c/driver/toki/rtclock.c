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
#define _HER_TIME_H
#include "../../../../inc/c/datime.h"
#include "../../../../inc/c/driver/i8259A.h"
#include "../../../../inc/c/driver/RealtimeClock.h"

#if defined(_MCCA) && ((_MCCA & 0xFF00)==0x8600)

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

// MC146818 register A UIP, register B data/hour format, and register D VRT.
int CMOS_ReadDateTime(struct datime_t* result)
{
	if (!result) return 0;
	const byte saved_index = innpb(PORT_CMOS_ADDR);
	const byte registers[] = { CMOS_SECOND, CMOS_MINUTE, CMOS_HOUR, CMOS_MDAY, CMOS_MONTH, CMOS_YEAR };
	byte first[6], second[6];
	struct datime_t date = {0};
	int valid = 0;
	// read the time, and the delta is less than 1s
	for (unsigned attempt = 0; attempt < 1024; ++attempt) {
		if (CMOS_Get(CMOS_STATUS_A) & 0x80) continue;
		const byte mode = CMOS_Get(CMOS_STATUS_B);
		if ((mode & 0x80) || !(CMOS_Get(CMOS_STATUS_D) & 0x80)) continue;
		for (unsigned i = 0; i < 6; ++i) first[i] = CMOS_Get(registers[i]);
		if (CMOS_Get(CMOS_STATUS_A) & 0x80) continue;
		for (unsigned i = 0; i < 6; ++i) second[i] = CMOS_Get(registers[i]);
		if ((CMOS_Get(CMOS_STATUS_A) & 0x80) || mode != CMOS_Get(CMOS_STATUS_B)) continue;
		int same = 1;
		for (unsigned i = 0; i < 6; ++i) if (first[i] != second[i]) same = 0;
		if (!same) continue;
		const byte pm = first[2] & 0x80;
		first[2] &= 0x7F;
		if (!(mode & 4)) {
			for (unsigned i = 0; i < 6; ++i) {
				if ((first[i] & 15) > 9 || (first[i] >> 4) > 9) same = 0;
				first[i] = bcd_to_bin(first[i]);
			}
		}
		if (!same) break;
		if (!(mode & 2)) {
			if (first[2] < 1 || first[2] > 12) break;
			first[2] = first[2] % 12 + (pm ? 12 : 0);
		}
		else if (pm) break;
		if (first[0] > 59 || first[1] > 59 || first[2] > 23 ||
			first[4] < 1 || first[4] > 12 || first[5] > 99 ||
			first[3] < 1 || first[3] > moondays(2000 + first[5], first[4])) break;
		date.second = first[0]; date.minute = first[1]; date.hour = first[2];
		date.mday = first[3]; date.month = first[4] - 1;
		date.year = 2000 + first[5];// Preserve the platform's existing 2000..2099 year window.
		valid = 1;
		break;
	}
	outpb(PORT_CMOS_ADDR, saved_index);
	if (valid) *result = date;
	return valid;
}

void CMOS_Readtime(struct tm* time)
{
	if (!time) return;
	struct datime_t date;
	if (!CMOS_ReadDateTime(&date)) return;
	// century = CMOS_Get(CMOS_CENTURY);
	time->tm_sec = date.second;
	time->tm_min = date.minute;
	time->tm_hour = date.hour;
	time->tm_mday = date.mday;
	time->tm_mon = date.month + 1;
	time->tm_year = date.year;// Legacy CMOS interface returns the full year and a 1-based month.
	time->tm_wday = weekday((word)date.year, time->tm_mon, date.mday);
	time->tm_yday = herspan(date.year, time->tm_mon, date.mday) - herspan(date.year, 1, 1);
	time->tm_isdst = -1;
	// century = bcd_to_bin(century);
}

//{unchk}
// Book a interrupt after n seconds
void RTC_SetAlarm(uint32 secs)
{
	struct tm time = {0};
	CMOS_Readtime(&time);
	if (!time.tm_year) return;
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
