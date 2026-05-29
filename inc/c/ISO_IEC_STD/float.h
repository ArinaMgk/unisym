#ifndef _INC_FLOAT
#define _INC_FLOAT

#include "../stdinc.h"//::floating

#ifndef FLT_RADIX
#define FLT_RADIX 2
#endif

#ifndef FLT_MANT_DIG
#define FLT_MANT_DIG 24
#endif
#ifndef DBL_MANT_DIG
#define DBL_MANT_DIG 53
#endif
#ifndef LDBL_MANT_DIG
#define LDBL_MANT_DIG DBL_MANT_DIG
#endif

#ifndef FLT_MIN_EXP
#define FLT_MIN_EXP (-125)
#endif
#ifndef DBL_MIN_EXP
#define DBL_MIN_EXP (-1021)
#endif
#ifndef LDBL_MIN_EXP
#define LDBL_MIN_EXP DBL_MIN_EXP
#endif

#ifndef FLT_MIN_10_EXP
#define FLT_MIN_10_EXP (-37)
#endif
#ifndef DBL_MIN_10_EXP
#define DBL_MIN_10_EXP (-307)
#endif
#ifndef LDBL_MIN_10_EXP
#define LDBL_MIN_10_EXP DBL_MIN_10_EXP
#endif

#ifndef FLT_MAX_10_EXP
#define FLT_MAX_10_EXP 38
#endif
#ifndef DBL_MAX_10_EXP
#define DBL_MAX_10_EXP 308
#endif
#ifndef LDBL_MAX_10_EXP
#define LDBL_MAX_10_EXP DBL_MAX_10_EXP
#endif
#ifndef FLT_MAX_EXP
#define FLT_MAX_EXP 128
#endif
#ifndef DBL_MAX_EXP
#define DBL_MAX_EXP 1024
#endif
#ifndef LDBL_MAX_EXP
#define LDBL_MAX_EXP DBL_MAX_EXP
#endif

#endif