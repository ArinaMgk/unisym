// ASCII CPL-ISO99 TAB4 CRLF
// LastCheck: 20240523
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

#if defined(_MCU_Intel8051) // Keil-C51 does not accept enum-element as macro-parameter
	#include "MCU/Intel/i8051.h"
#elif defined(_MCCA) && (_MCCA==0x8616||_MCCA==0x8632)// defined(_ARC_x86)
	#include "proctrl/x86/x86.h"
	#include "board/IBM.h"
	#define _SUPPORT_Port8
#elif defined(_ARC_RISCV_64)
	#include "proctrl/RISCV/riscv64.h"
#else
	//...
#endif

// Operational Unit Size
#if defined(_MCU_STM32) || defined(_MCU_CW32F030)
	// use 32b-align(times of 0x4) address
	typedef uint32  typedest;
	typedef uint32  typeaddr;
	#define _typedest_len 4
	#define _typeaddr_len 4

#elif 0

	
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
