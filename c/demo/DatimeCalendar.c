// RFT15 ASCII TAB4 C99 ArnAssume
// Doshou Haruno dosconyo@gmail.com
//$ gcc DatimeCalendar.c ../source/datime.c ../source/consio.c -I../ -D_WinNT
#include <stdio.h>
#include <time.h>
#include <conio.h>
#include "../datime.h"
#include "../consio.h"

void DrawCalendar(word year, word moon, byte crtday)
{
	unsigned char weekday, mondays;
	if (moon == 0 || moon > 12) return;
	unsigned pasts = DatimeCalendar(year, moon, &weekday, &mondays);
	ConClear();
	ConCursor(0, 0);
	printf("    %s %d\n", ((char* []){
		"   January ", "  February ", "     March ",
		"     April ", "       May ", "      June ",
		"      July ", "    August ", " September ",
		"   October ", "  November ", "  December "})[moon-1], year);
	puts("Sun.Mon.Tue.Wed.Thr.Fri.Sat.");
	ConStyleAbnormal();
	ConCursorMoveRight(weekday << 2);
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
		weekday++; pasts++;
		if (weekday >= 7)
		{
			ConStyleNormal();
			puts("");
			ConStyleAbnormal();
			weekday = 0;
		}
	}
	ConStyleNormal();
	puts("");
}

int main()
{
	// Q WASD F
	int chr;
	time_t timep;
	struct tm* tmp;
	time(&timep);
	tmp = localtime(&timep);// gmtime() for UTC
	DrawCalendar(1900 + tmp->tm_year, 1 + tmp->tm_mon, tmp->tm_mday);
	word CrtM = tmp->tm_mon, CrtY = tmp->tm_year;
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
			} else tmp->tm_mon--;
			DrawCalendar(1900 + tmp->tm_year, 1 + tmp->tm_mon, (CrtM == tmp->tm_mon && CrtY == tmp->tm_year) ? tmp->tm_mday : 0);
			break;
		case 'd':// next month
			if (tmp->tm_mon >= 11)
			{
				tmp->tm_mon = 0;
				tmp->tm_year++;
			} else tmp->tm_mon++;
			DrawCalendar(1900 + tmp->tm_year, 1 + tmp->tm_mon, (CrtM == tmp->tm_mon && CrtY == tmp->tm_year) ? tmp->tm_mday : 0);
			break;
		case 'w':// last year
			tmp->tm_year--;
			DrawCalendar(1900 + tmp->tm_year, 1 + tmp->tm_mon, (CrtM == tmp->tm_mon && CrtY == tmp->tm_year) ? tmp->tm_mday : 0);
			break;
		case 's':// next year
			tmp->tm_year++;
			DrawCalendar(1900 + tmp->tm_year, 1 + tmp->tm_mon, (CrtM == tmp->tm_mon && CrtY == tmp->tm_year) ? tmp->tm_mday : 0);
			break;
		case 'f':
			time(&timep);
			tmp = localtime(&timep);// gmtime() for UTC
			DrawCalendar(1900 + tmp->tm_year, 1 + tmp->tm_mon, tmp->tm_mday);
			break;
		default:
			break;
		}
}