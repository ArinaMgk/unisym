// UTF-8 C99 TAB4 CRLF
// Attribute: ArnCovenant yo Host
// LastCheck: 2023 Nov 16 (Doshou Haruno dosconyo@gmail.com)
// AllAuthor: @dosconio since RFT15
// ProjTitle: Console Calendar Shell

#include <time.h>
#include <datime.h>
#include <stdio.h>
#ifndef _Linux
	#include <conio.h>
#else	
	#define getch getchar//{TEMP}
#endif
#include <consio.h>
#include "../../../inc/c/ISO_IEC_STD/stdlib.h"

int show_weekid = 0;

static char getPrefix(stdsint pasts) {
	if (pasts < 0) return '[';
	byte mod = (pasts + 1) % 30;
	if (!mod) return '|';// if the beginning of the period
	else if (mod < 5) return ' ';// if in the period
	else return '[';
}

static char getSuffix(stdsint pasts) {
	if (pasts < 0) return ']';
	byte mod = (pasts + 1) % 30;
	if (mod >= 5) return ']';
	if (mod == 4) return '|';// if the ending of the period
	else return ' ';// if in the period
}

static void ShowLunarDate(word year, byte month, byte day) {
	static const char* const stems[] = { "甲", "乙", "丙", "丁", "戊", "己", "庚", "辛", "壬", "癸" };
	static const char* const branches[] = { "子", "丑", "寅", "卯", "辰", "巳", "午", "未", "申", "酉", "戌", "亥" };
	static const char* const months[] = { "正", "二", "三", "四", "五", "六", "七", "八", "九", "十", "十一", "十二" };
	static const char* const digits[] = { "一", "二", "三", "四", "五", "六", "七", "八", "九", "十" };
	struct datime_t date = {0};
	LunarDate_t lunar;
	date.year = year;
	date.month = month - 1;
	date.mday = day;
	if (!GregorianToLunar(&date, &lunar)) {
		printf("农历日期超出支持范围（2000—2050）\n");
		return;
	}
	printf("%s%s年%s%s月", stems[(lunar.year - 4) % 10], branches[(lunar.year - 4) % 12],
		lunar.leap_month ? "(闰)" : "", months[lunar.month - 1]);
	if (lunar.day <= 10) printf("初%s", digits[lunar.day - 1]);
	else if (lunar.day < 20) printf("十%s", digits[lunar.day - 11]);
	else if (lunar.day == 20) printf("二十");
	else if (lunar.day < 30) printf("廿%s", digits[lunar.day - 21]);
	else printf("三十");
	printf("\n");
}

void DrawCalendar(word year, word moon, byte crtday)
{
	unsigned char week_day, mondays;
	if (moon == 0 || moon > 12) return;
	stdsint pasts = herspan(year, moon, 1);
	week_day = weekday(year, moon, 1);
	mondays = moondays(year, moon);
	printf("\n");
	printf("    %s %d \n", ((char* []){
		"   January ", "  February ", "     March ",
		"     April ", "       May ", "      June ",
		"      July ", "    August ", " September ",
		"   October ", "  November ", "  December "})[moon-1], year);
	printf("Sun.Mon.Tue.Wed.Thr.Fri.Sat.");
	       "(日)(月)(火)(水)(木)(金)(土)";
	printf("\n");
	ConStyleAbnormal();
	ConCursorMoveRight(week_day << 2);
	for (unsigned char i = 1; i <= mondays; i++)
	{
		if (i == crtday)
		{
			ConStyleNormal();
			printf("%c%02d%c", getPrefix(pasts) == '[' ? '<' : '!',
				i, getSuffix(pasts) == ']' ? '>' : '!');
			ConStyleAbnormal();
		}
		else printf("%c%02d%c", getPrefix(pasts), i, getSuffix(pasts));
		week_day++; pasts++;
		if (week_day >= 7)
		{
			if(show_weekid)
			{
				ConStyleNormal();
				printf(" Week %4d", getHerWeekNumber(year, moon, i));
				ConStyleAbnormal();
			}
			ConStyleNormal();
			printf("\n");// Although add a space, it do not fit `cmd` or `PowerShell` well(other printable characters can), but for `wt` and others.
			ConStyleAbnormal();
			week_day = 0;
		}
	}
	ConStyleNormal();
	printf("\n");
	ShowLunarDate(year, (byte)moon, crtday ? crtday : 1);
}

int main(int argc, char* argv[])
{
	// Q WASD F
	int chr;
	time_t timep;
	struct tm* tmp;
	time(&timep);
	tmp = localtime(&timep);// gmtime() for UTC
	word CrtM = tmp->tm_mon, CrtY = tmp->tm_year, CrtD = tmp->tm_mday;

	if (argc <= 1) {
		DrawCalendar(1900 + CrtY, 1 + CrtM, CrtD);
		return 0;
	}
	if (!StrCompare("-wn", argv[1])) { // week number
		stdsint week_number = getHerWeekNumber(1900 + CrtY, 1 + CrtM, CrtD);
		outsfmt("%[i]\n", week_number);
		return 0;
	}
	else if (!StrCompare("-dn", argv[1])) { // date number
		stdsint date_number = herspan(1900 + CrtY, 1 + CrtM, CrtD);
		outsfmt("%[i]\n", date_number);
		return 0;
	}
	show_weekid = 1;
	ConClear(); ConCursor(0, 0);
	DrawCalendar(1900 + tmp->tm_year, 1 + tmp->tm_mon, (CrtM == tmp->tm_mon && CrtY == tmp->tm_year) ? tmp->tm_mday : 0);
	while (chr = getch())
		switch (chr)
		{
		case 'q':
			return 0;
		case 'a':// last month
			if (tmp->tm_mon == 0)
			{
				tmp->tm_mon = 11;
				tmp->tm_year--;
			}
			else tmp->tm_mon--;
			ConClear(); ConCursor(0, 0);
			DrawCalendar(1900 + tmp->tm_year, 1 + tmp->tm_mon, (CrtM == tmp->tm_mon && CrtY == tmp->tm_year) ? tmp->tm_mday : 0);
			break;
		case 'd':// next month
			if (tmp->tm_mon >= 11)
			{
				tmp->tm_mon = 0;
				tmp->tm_year++;
			}
			else tmp->tm_mon++;
			ConClear(); ConCursor(0, 0);
			DrawCalendar(1900 + tmp->tm_year, 1 + tmp->tm_mon, (CrtM == tmp->tm_mon && CrtY == tmp->tm_year) ? tmp->tm_mday : 0);
			break;
		case 'w':// last year
			tmp->tm_year--;
			ConClear(); ConCursor(0, 0);
			DrawCalendar(1900 + tmp->tm_year, 1 + tmp->tm_mon, (CrtM == tmp->tm_mon && CrtY == tmp->tm_year) ? tmp->tm_mday : 0);
			break;
		case 's':// next year
			tmp->tm_year++;
			ConClear(); ConCursor(0, 0);
			DrawCalendar(1900 + tmp->tm_year, 1 + tmp->tm_mon, (CrtM == tmp->tm_mon && CrtY == tmp->tm_year) ? tmp->tm_mday : 0);
			break;
		case 'f':// today
			time(&timep);
			tmp = localtime(&timep);// gmtime() for UTC
			ConClear(); ConCursor(0, 0);
			DrawCalendar(1900 + tmp->tm_year, 1 + tmp->tm_mon, tmp->tm_mday);
			break;
		default:
			break;
		}
}
