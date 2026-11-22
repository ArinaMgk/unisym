// ASCII C++11 TAB4 LF
// Docutitle: RISC-V Procedure Call ABI
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0
/*
	Copyright 2026 ArinaMgk

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0
	http://unisym.org/license.html

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/

#include "../../../../../inc/cpp/Lango/Backend/RISCV/RISCV-ABI.hpp"

namespace uni::lango {

namespace {
	unsigned FloatRegisterBits(const RISCVTargetInfo& target) {
	switch (target.abi) {
	case RISCVABI::LP64F:
	case RISCVABI::ILP32F:
		return 32;
	case RISCVABI::LP64D:
	case RISCVABI::ILP32D:
		return 64;
	default:
		return 0;
	}
	}

	unsigned TypeWidth(clover::Type type, const RISCVTargetInfo& target) {
	switch (type) {
	case clover::Type::I1: return 1;
	case clover::Type::I8: return 8;
	case clover::Type::I16: return 16;
	case clover::Type::I32:
	case clover::Type::F32: return 32;
	case clover::Type::I64:
	case clover::Type::F64: return 64;
	case clover::Type::F128: return 128;
	case clover::Type::Ptr: return target.bits;
	default: return 0;
	}
	}

	void AllocateIntegerPieces(unsigned count, stduint total_bytes,
		const RISCVTargetInfo& target, RISCVABIArgumentState& state,
		bool align_pair, stduint stack_alignment,
		RISCVABIArgumentLocation& location) {
		const unsigned words = target.bits / 8;
		if (align_pair && count == 2 && stack_alignment >= words * 2 &&
			(state.integer_registers & 1))
			state.integer_registers++;
		unsigned in_registers = 0;
		if (state.integer_registers < 8) {
			in_registers = 8 - state.integer_registers;
			if (in_registers > count) in_registers = count;
		}
		for (unsigned index = 0; index < count; index++) {
			RISCVABIArgumentPiece& piece = location.pieces[index];
			piece.object_offset = index * words;
			piece.size_bytes = total_bytes - piece.object_offset;
			if (piece.size_bytes > words) piece.size_bytes = words;
			if (index < in_registers) {
				piece.kind = RISCVABIArgumentPieceKind::IntegerRegister;
				piece.register_index = state.integer_registers++;
			}
			else {
				piece.kind = RISCVABIArgumentPieceKind::Stack;
				if (index == in_registers) {
					if (stack_alignment < words) stack_alignment = words;
					if (stack_alignment > 16) stack_alignment = 16;
					state.stack_bytes = ((state.stack_bytes + stack_alignment - 1) /
						stack_alignment) * stack_alignment;
				}
				piece.stack_offset = state.stack_bytes;
				state.stack_bytes += words;
			}
		}
		location.piece_count = count;
	}
}

bool RISCVAllocateArgument(clover::Type type, const RISCVTargetInfo& target,
	RISCVABIArgumentState& state, bool force_integer_registers,
	RISCVABIArgumentLocation& location) {
	location = RISCVABIArgumentLocation();
	if (type == clover::Type::Void) return false;
	unsigned width = TypeWidth(type, target);
	if (!width) return false;
	unsigned flen = FloatRegisterBits(target);
	if (!force_integer_registers &&
		(type == clover::Type::F32 || type == clover::Type::F64) &&
		width <= flen && state.float_registers < 8) {
		RISCVABIArgumentPiece& piece = location.pieces[0];
		piece.kind = RISCVABIArgumentPieceKind::FloatRegister;
		piece.register_index = state.float_registers++;
		piece.size_bytes = width / 8;
		location.piece_count = 1;
		return true;
	}
	unsigned words = width > target.bits ? (width + target.bits - 1) / target.bits : 1;
	if (words > 2) return false;
	stduint stack_alignment = words == 2 ? target.bits / 4 : target.bits / 8;
	AllocateIntegerPieces(words, (width + 7) / 8, target, state,
		force_integer_registers, stack_alignment, location);
	return true;
}

bool RISCVAllocateAggregateArgument(const clover::Module& module,
	const clover::TypeReference& type, const RISCVTargetInfo& target,
	RISCVABIArgumentState& state, bool force_integer_registers,
	RISCVABIArgumentLocation& location) {
	location = RISCVABIArgumentLocation();
	stduint bits = 0;
	if (!clover::TypeSizeBits(module, type, bits) || !bits) return false;
	stduint size_bytes = (bits + 7) / 8;
	if (!size_bytes) return false;
	const stduint word_bytes = target.bits / 8;
	if (size_bytes > word_bytes * 2) {
		if (!RISCVAllocateArgument(clover::Type::Ptr, target, state,
			force_integer_registers, location)) return false;
		location.indirect = true;
		return true;
	}
	unsigned words = (unsigned)((size_bytes + word_bytes - 1) / word_bytes);
	stduint alignment_bits = 0;
	if (!clover::TypeAlignmentBits(module, type, alignment_bits)) return false;
	stduint stack_alignment = (alignment_bits + 7) / 8;
	location.indirect = false;
	AllocateIntegerPieces(words, size_bytes, target, state,
		force_integer_registers, stack_alignment, location);
	return true;
}

}
