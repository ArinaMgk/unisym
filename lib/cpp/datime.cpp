// ASCII C++11 TAB4 LF
// Docutitle: Date and Time
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding
// Copyright: UNISYM, under Apache License 2.0

#include "../../inc/c/datime.h"

namespace {
	// HKO Gregorian-Lunar Conversion Tables, retrieved 2026-11-24:
	// https://www.hko.gov.hk/en/gts/time/conversion1_text.htm
	// Modern Chinese calendar civil dates. HKO near-midnight predictions may change.
	// Entries 2000..2050: bits 0..11 mark 30-day normal months, bit 12 marks a
	// 30-day leap month, bits 13..16 give its number (0 = none), bits 17..22
	// give the zero-based Gregorian day of New Year within that year.
	constexpr uint32 lunar_years[] = {
		0x460693U, 0x2e8a9bU, 0x54052bU, 0x3e0a5bU, 0x2a4aaeU, 0x4e056aU, 0x38edd5U, 0x600ba4U,
		0x4a0b49U, 0x32ad53U, 0x580a95U, 0x42052dU, 0x2c855dU, 0x500ab5U, 0x3d2baaU, 0x6205d2U,
		0x4c0da5U, 0x36de8aU, 0x5c0d4aU, 0x460c95U, 0x308a9eU, 0x540556U, 0x3e0ab5U, 0x2a4adaU,
		0x5006d2U, 0x38c765U, 0x5e0725U, 0x48064bU, 0x32a657U, 0x560cabU, 0x42055aU, 0x2c656eU,
		0x520b69U, 0x3d6f52U, 0x620b52U, 0x4c0b25U, 0x36dd0bU, 0x5a0a4bU, 0x4404abU, 0x2ea2bbU,
		0x5405adU, 0x3e0b6aU, 0x2a4daaU, 0x500d92U, 0x3aeea5U, 0x5e0d25U, 0x480a55U, 0x32ba4dU,
		0x5804b6U, 0x4005b5U, 0x2c76d2U,
	};

	constexpr unsigned LeapMonth(unsigned year) {
		return (lunar_years[year - 2000] >> 13) & 15U;
	}

	constexpr stdint LunarYearStart(unsigned year) {
		return getHerDaySpanYear(int(year)) + int(lunar_years[year - 2000] >> 17);
	}

	constexpr unsigned LunarMonthDays(unsigned year, unsigned month, bool leap) {
		return 29U + ((lunar_years[year - 2000] >> (leap ? 12 : month - 1)) & 1U);
	}

	constexpr bool ValidYearInfo(unsigned index) {
		return index == 51 ? true :
			((lunar_years[index] >> 17) >= 20 && (lunar_years[index] >> 17) <= 50 &&
			((lunar_years[index] >> 13) & 15U) <= 12 && ValidYearInfo(index + 1));
	}

	constexpr unsigned LunarRemainingDays(unsigned year, unsigned month) {
		return month == 13 ? 0 : LunarMonthDays(year, month, false) +
			(month == LeapMonth(year) ? LunarMonthDays(year, month, true) : 0) +
			LunarRemainingDays(year, month + 1);
	}

	constexpr bool ValidYearSpans(unsigned year) {
		return year == 2050 ? true :
			(LunarYearStart(year + 1) - LunarYearStart(year) == stdint(LunarRemainingDays(year, 1)) &&
			ValidYearSpans(year + 1));
	}

	static_assert(sizeof(lunar_years) == 204, "Lunar table size");
	static_assert(ValidYearInfo(0), "Lunar table fields");
	static_assert(ValidYearSpans(2000), "Lunar years must join without gaps");
	static_assert(LunarYearStart(2000) == herspan(2000, 2, 5), "2000 New Year");
	static_assert(LunarYearStart(2026) == herspan(2026, 2, 17), "2026 New Year");
	static_assert(LeapMonth(2025) == 6 && LeapMonth(2033) == 11, "Leap month boundaries");
}

int GregorianToLunar(const datime_t* date, LunarDate_t* result) {
	if (!date || !result || date->year < 2000 || date->year > 2050 ||
		date->month > 11 || date->mday < 1 ||
		date->mday > moondays(date->year, (stdint)date->month + 1)) return 0;

	const stdint days = date->getHerspan();
	LunarDate_t converted = {};
	if (days < LunarYearStart(2000)) {
		// The first 35 supported days belong to lunar 1999, months 11 and 12.
		const stdint offset = days - herspan(2000, 1, 1);
		converted.year = 1999;
		converted.month = offset < 6 ? 11 : 12;
		converted.day = byte(offset < 6 ? offset + 25 : offset - 5);
	}
	else {
		unsigned year = unsigned(date->year);
		if (days < LunarYearStart(year)) --year;
		stdint remaining = days - LunarYearStart(year);
		const unsigned leap_month = LeapMonth(year);
		unsigned month = 1;
		bool leap = false;
		while (remaining >= stdint(LunarMonthDays(year, month, leap))) {
			remaining -= LunarMonthDays(year, month, leap);
			if (!leap && month == leap_month) leap = true;
			else { leap = false; ++month; }
		}
		converted.year = year;
		converted.month = byte(month);
		converted.day = byte(remaining + 1);
		converted.leap_month = byte(leap);
	}
	*result = converted;
	return 1;
}

int LunarToGregorian(const LunarDate_t* date, datime_t* result) {
	if (!date || !result || date->year < 1999 || date->year > 2050 ||
		date->month < 1 || date->month > 12 || date->day < 1 ||
		date->day > 30 || date->leap_month > 1) return 0;

	stdint days;
	if (date->year == 1999) {
		if (date->leap_month || (date->month != 11 && date->month != 12) ||
			(date->month == 11 && date->day < 25) || (date->month == 12 && date->day > 29)) return 0;
		days = herspan(2000, 1, 1) + (date->month == 11 ? int(date->day) - 25 : int(date->day) + 5);
	}
	else {
		const unsigned year = unsigned(date->year);
		const unsigned leap_month = LeapMonth(year);
		if ((date->leap_month && date->month != leap_month) ||
			date->day > LunarMonthDays(year, date->month, date->leap_month != 0)) return 0;
		days = LunarYearStart(year);
		for (unsigned month = 1; month < date->month; ++month) {
			days += LunarMonthDays(year, month, false);
			if (month == leap_month) days += LunarMonthDays(year, month, true);
		}
		if (date->leap_month) days += LunarMonthDays(year, date->month, false);
		days += date->day - 1;
	}
	if (days < herspan(2000, 1, 1) || days > herspan(2050, 12, 31)) return 0;
	datime_t converted = {};
	word year, month, day;
	fromherp(days, &year, &month, &day);
	converted.year = year;
	converted.month = byte(month - 1);
	converted.mday = byte(day);
	*result = converted;
	return 1;
}
