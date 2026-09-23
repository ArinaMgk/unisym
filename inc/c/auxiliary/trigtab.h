// ASCII C99 TAB4 CRLF
// Docutitle: Trigonometric lookup table interface
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#ifndef _INC_AUXILIARY_TRIGTAB
#define _INC_AUXILIARY_TRIGTAB

#if defined(_MCU_STM32)
typedef float Trigtab;
#define TRIGTAB_QUARTER_N 128
#else
typedef double Trigtab;
#define TRIGTAB_QUARTER_N 256
#endif

extern const Trigtab _tab_sin_quarter[TRIGTAB_QUARTER_N + 1];

#endif
