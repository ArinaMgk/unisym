// ASCII CPL-ISO99 TAB4 CRLF
// Attribute: <ArnCovenant> <Env> <bin^%> <CPU()> [Allocation]
// LastCheck: <date>
// AllAuthor: @dosconio
// ModuTitle: Processor and Controller
// Copyright: UNISYM led by ArinaMgk, Apache License, Version 2.0, unisym.org

//{ISSUE} Whether should allow `_MCU_NO_USING` to pass some optional controls?

#ifndef _INC_PROCTRL
#define _INC_PROCTRL

#include "uoption.h"
#include "floating.h"

//{TODO} in uoption: _PCU_CortexM7
enum Procontroller_t // work with Architecture_t
{
	PCU_Intel8086,
	PCU_AMD_Ryzen7_5800H_Radeon, //{TODO} for `_CPU_AMDRyzen7_5800H_Radeon`
	//
	PCU_Unknown
};

#if !defined(_MCU_Intel8051) // Keil-C51 does not accept enum-element as macro-parameter
	#if (__ARCH__ == Architecture_x86) && defined(_MCCA)//{TEMP}
		#include "proctrl/x86/x86.h"
	#endif
#else
	// #include "MCU/Intel/i8051.h"
	#if defined(_IMPLEMENT_KEIL8051)
		#include "MCU/Intel/keil8051.h"
	#elif defined(_IMPLEMENT_SDCC8051)
		#include "MCU/Intel/sdcc8051.h"
	#endif
#endif

#if defined(_MCU_STM32F10x)
	// use 32b-align(times of 0x4) address
	typedef uint32  typedest;
	typedef uint32  typeaddr;
	#define _typedest_len 4
	#define _typeaddr_len 4
#elif defined(_MCU_STM32F4x)
	// I guessed ???
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
