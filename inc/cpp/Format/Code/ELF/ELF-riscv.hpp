// ASCII C++11 TAB4 LF
// Docutitle: RISC-V ELF Target Metadata
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

#ifndef UNISYM_FORMAT_CODE_ELF_RISCV_HPP
#define UNISYM_FORMAT_CODE_ELF_RISCV_HPP

#include "../../../../c/stdinc.h"

// ---- C/C++ common part

enum {
	EF_RISCV_RVC = 0x0001,
	EF_RISCV_FLOAT_ABI_SINGLE = 0x0002,
	EF_RISCV_FLOAT_ABI_DOUBLE = 0x0004,
	EF_RISCV_FLOAT_ABI_QUAD = 0x0006,
	EF_RISCV_FLOAT_ABI = 0x0006
};

enum reloc32_riscv_type {
	R_RISCV_NONE = 0,
	R_RISCV_32 = 1,
	R_RISCV_64 = 2,
	R_RISCV_RELATIVE = 3,
	R_RISCV_JUMP_SLOT = 5,
	R_RISCV_BRANCH = 16,
	R_RISCV_JAL = 17,
	R_RISCV_CALL = 18,
	R_RISCV_CALL_PLT = 19,
	R_RISCV_GOT_HI20 = 20,
	R_RISCV_PCREL_HI20 = 23,
	R_RISCV_PCREL_LO12_I = 24,
	R_RISCV_PCREL_LO12_S = 25,
	R_RISCV_HI20 = 26,
	R_RISCV_LO12_I = 27,
	R_RISCV_LO12_S = 28,
	R_RISCV_ADD8 = 33,
	R_RISCV_ADD16 = 34,
	R_RISCV_ADD32 = 35,
	R_RISCV_ADD64 = 36,
	R_RISCV_SUB8 = 37,
	R_RISCV_SUB16 = 38,
	R_RISCV_SUB32 = 39,
	R_RISCV_SUB64 = 40,
	R_RISCV_GOT32_PCREL = 41,
	R_RISCV_ALIGN = 43,
	R_RISCV_RVC_BRANCH = 44,
	R_RISCV_RVC_JUMP = 45,
	R_RISCV_RELAX = 51,
	R_RISCV_SUB6 = 52,
	R_RISCV_SET6 = 53,
	R_RISCV_SET8 = 54,
	R_RISCV_SET16 = 55,
	R_RISCV_SET32 = 56,
	R_RISCV_32_PCREL = 57,
	R_RISCV_IRELATIVE = 58,
	R_RISCV_PLT32 = 59,
	R_RISCV_SET_ULEB128 = 60,
	R_RISCV_SUB_ULEB128 = 61
};

// ---- C++ only part
#ifdef _INC_CPP // C++ only part

namespace uni {

struct CodeFixup;

int RISCVRelocationType(const CodeFixup& fixup);
bool RISCVFixupSupportsRelaxation(const CodeFixup& fixup);

class RISCVELFTarget {
public:
	void Reset(int bits);
	bool Update(int bits, unsigned int cpu, unsigned int abi,
		bool abi_explicit);
	unsigned int Flags() const;
	size_t BuildAttributes(unsigned char* output, size_t capacity) const;

private:
	int Bits = 0;
	unsigned int Extensions = 0;
	unsigned int Abi = 0;
	bool HasExplicitAbi = false;
	bool ReportedAbiConflict = false;
};

}

#endif

#endif
