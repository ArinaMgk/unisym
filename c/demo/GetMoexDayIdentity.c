// ASCII RFV26
// $ gcc GetMoexDayIdentity.c ../source/datime.c
#include "../datime.h"
#include "../consio.h"
#include <stdio.h>

#define MOON 9
#define YEAR 2023

int main()
{
	unsigned char weekday;
	unsigned char mondays;
	unsigned int pasts;
	ConClear();
	ConCursor(0, 0);
	printf("Month : %02d Year : %d\n", MOON, YEAR);
	pasts = GetMoexDayIdentity(YEAR, MOON, &weekday, &mondays);
	printf("PastDays :%04X\n", pasts);
	printf("WeekCode :%04X\n", pasts / 7);
	puts("Sun.Mon.Tue.Wed.Thr.Fri.Sat.");
	ConStyleAbnormal();
	ConCursorMoveRight(weekday << 2);
	for (unsigned char i = 1; i <= mondays; i++)
	{
		if (((pasts + 1) % 30) <= 4)
			printf("<%02d>", i);
		else
			printf("[%02d]", i);
		weekday++; pasts++;
		if (weekday >= 7)
		{
			puts("");
			weekday = 0;
		}
	}
	puts("");
	ConStyleNormal();
	return 0;
}