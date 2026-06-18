#define _HER_TIME_H
#include "../datime.h"

typedef sint64 time_t;

time_t time(time_t* seconds);

struct tm* localtime(const time_t* timer);
