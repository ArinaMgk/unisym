// ASCII C++11 TAB4 LF
// Docutitle: RISC-V ELF Target Metadata
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

#ifndef UNISYM_FORMAT_CODE_ELF_RISCV_HPP
#define UNISYM_FORMAT_CODE_ELF_RISCV_HPP

#include "../../../../c/stdinc.h"

namespace uni {

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
