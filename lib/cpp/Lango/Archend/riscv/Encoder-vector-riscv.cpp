// ASCII C++11 TAB4 LF
// Docutitle: AASM RISC-V Vector Field Encoder
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../../inc/cpp/Lango/Archend/riscv/Vector-riscv.hpp"

namespace uni::lango {

uint32_t EncodeRISCVVectorType(const RISCVVectorType& type)
{
	unsigned sew = type.element_bits == 8 ? 0 : type.element_bits == 16 ? 1 :
		type.element_bits == 32 ? 2 : 3;
	return static_cast<uint32_t>(type.lmul & 7) | sew << 3 |
		static_cast<uint32_t>(type.tail_agnostic) << 6 |
		static_cast<uint32_t>(type.mask_agnostic) << 7;
}

}
