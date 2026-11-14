// ASCII C++11 TAB4 LF
// Docutitle: ARM ELF Target Definitions
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

#ifndef UNISYM_FORMAT_CODE_ELF_ARM_HPP
#define UNISYM_FORMAT_CODE_ELF_ARM_HPP

#include "../../../../c/stdinc.h"

enum {
	EF_ARM_EABI_VER5 = 0x05000000,
	SHT_ARM_ATTRIBUTES = 0x70000003
};

enum reloc32_arm_type {
	R_ARM_NONE = 0,
	R_ARM_ABS32 = 2,
	R_ARM_ABS16 = 5,
	R_ARM_ABS8 = 8,
	R_ARM_RELATIVE = 23,
	R_ARM_CALL = 28,
	R_ARM_JUMP24 = 29,
	R_ARM_MOVW_ABS_NC = 43,
	R_ARM_MOVT_ABS = 44
};

enum reloc64_aarch64_type {
	R_AARCH64_NONE = 0,
	R_AARCH64_ABS64 = 257,
	R_AARCH64_ABS32 = 258,
	R_AARCH64_ABS16 = 259,
	R_AARCH64_ADR_PREL_PG_HI21 = 275,
	R_AARCH64_ADD_ABS_LO12_NC = 277,
	R_AARCH64_CONDBR19 = 280,
	R_AARCH64_JUMP26 = 282,
	R_AARCH64_CALL26 = 283
};

#endif
