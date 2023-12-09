// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant yo Host
// LastCheck: 2023 Nov 16 (Doshou Haruno dosconyo@gmail.com)
// AllAuthor: @dosconio since RFT15
// ProjTitle: Console Calendar Shell

#define _AUTO_INCLUDE
#include <stdio.h>
#include <time.h>
#include <conio.h>
#include <datime.h>
#include <consio.h>

int show_weekid = 0;

void DrawCalendar(word year, word moon, byte crtday)
{
	unsigned char week_day, mondays;
	if (moon == 0 || moon > 12) return;
	llong pasts = herspan(year, moon, 1);
	week_day = weekday(year, moon, 1);
	mondays = moondays(year, moon);
	puts("");
	printf("    %s %d\n", ((char* []){
		"   January ", "  February ", "     March ",
		"     April ", "       May ", "      June ",
		"      July ", "    August ", " September ",
		"   October ", "  November ", "  December "})[moon-1], year);
	puts("Sun.Mon.Tue.Wed.Thr.Fri.Sat.");
	ConStyleAbnormal();
	ConCursorMoveRight(week_day << 2);
	for (unsigned char i = 1; i <= mondays; i++)
	{
		if (i == crtday)
		{
			ConStyleNormal();
			printf("[%02d]", i);
			ConStyleAbnormal();
		}
		else if (year >= 2014 && !((pasts + 1) % 30))
			printf("|%02d]", i);
		else
			printf("[%02d]", i);
		week_day++; pasts++;
		if (week_day >= 7)
		{
			if(show_weekid)
			{
				ConStyleNormal();
				printf(" Week %4d", getHerWeekNumber(year, moon, i));
				ConStyleAbnormal();
			}
			printf("\n");
			week_day = 0;
		}
	}
	ConStyleNormal();
	puts("");
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

	if (argc > 1)
	{
		show_weekid = 1;
		ConClear();
	}

	DrawCalendar(1900 + CrtY, 1 + CrtM, CrtD);

	if(argc <= 1) exit(0);
	
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
