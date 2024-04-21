// ASCII CPL-ISO99 TAB4 CRLF
// Attribute: <ArnCovenant> <Env> <bin^%> <CPU()> [Allocation]
// LastCheck: <date>
// AllAuthor: @dosconio
// ModuTitle: Processor and Controller
// Copyright: UNISYM led by ArinaMgk, Apache License, Version 2.0, unisym.org

#ifndef _INC_PROCTRL
#define _INC_PROCTRL
#include "floating.h"
#if defined(_MCU_STM32F103VE)
// use 32b-align(times of 0x4) address
typedef uint32  typedest;
typedef uint32  typeaddr;
#define _typedest_len 4
#define _typeaddr_len 4
#else // x86
typedef byte    typedest;
typedef stduint typeaddr;
typedef byte    typeport;
typedef word    typepoid;
#define _typedest_len 1
#define _typeaddr_len 4
#define _typeport_len 1
#define _typepoid_len 2
#endif
#endif
