// RFX12
#define _AUTO_INCLUDE
#include <stdio.h>
#include <ustring.h>
#include <datime.h>
#include <string.h>

int main()
{
	char* Weekiden[] = {"Sun.", "Mon.", "Tue.", "Wed.", "Thu.", "Fri.", "Sat."};

	for (int i = 27; i <= 30; i++)
		printf("[2013 11 %2d: %4d %s]\n", i, herspan(2013, 11, i), Weekiden[weekday(2013, 11, i)]);

	for (int i = 1; i <= 31; i++)
		printf("[2013 12 %2d: %4d %s]\n", i, herspan(2013, 12, i), Weekiden[weekday(2013, 12, i)]);

	return 0;

	for (int i = 1; i <=  4 ; i++)
		printf("[2014 01 %2d: %4d %s]\n", i, herspan(2014, 1, i), Weekiden[weekday(2014, 1, i)]);
}
