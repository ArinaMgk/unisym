// ASCII C99 TAB4 LF
// Docutitle: Trigonometric lookup table interface
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#ifndef _INC_AUXILIARY_TRIGTAB
#define _INC_AUXILIARY_TRIGTAB

#include "../stdinc.h"

#if defined(_MCU_STM32)
typedef float Trigtab;
#define TRIGTAB_QUARTER_N 128
#else
typedef double Trigtab;
#define TRIGTAB_QUARTER_N 256
#endif
#ifndef _INC_CPP
extern const Trigtab _tab_sin_quarter[TRIGTAB_QUARTER_N + 1];
#else
#include "../../cpp/ISO_IEC_STD/array"
_ESYM_C const uni::Array<Trigtab, TRIGTAB_QUARTER_N + 1> _tab_sin_quarter;
#endif


#endif
