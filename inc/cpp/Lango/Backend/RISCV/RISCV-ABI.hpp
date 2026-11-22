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

#ifndef UNISYM_LANGO_BACKEND_RISCV_ABI_HPP
#define UNISYM_LANGO_BACKEND_RISCV_ABI_HPP

#include "../Backend-riscv.hpp"

namespace uni::lango {

struct RISCVABIArgumentState {
	unsigned integer_registers = 0;
	unsigned float_registers = 0;
	stduint stack_bytes = 0;
};

enum class RISCVABIArgumentPieceKind {
	IntegerRegister, // Piece is carried in an a0-a7 register.
	FloatRegister, // Piece is carried in an fa0-fa7 register.
	Stack // Piece is carried in the caller's outgoing argument area.
};

struct RISCVABIArgumentPiece {
	RISCVABIArgumentPieceKind kind = RISCVABIArgumentPieceKind::IntegerRegister;
	unsigned register_index = 0; // Index within the selected argument register class.
	stduint stack_offset = 0; // Byte offset when kind is Stack.
	stduint object_offset = 0; // Byte offset within the scalar or aggregate object.
	stduint size_bytes = 0; // Number of object bytes represented by this piece.
};

struct RISCVABIArgumentLocation {
	bool indirect = false; // Aggregate is represented by its address.
	unsigned piece_count = 0; // Current ABI values need at most two XLEN pieces.
	RISCVABIArgumentPiece pieces[2]; // Location for each ordered object piece.
};

// Assign one scalar argument according to the selected RISC-V ABI.
bool RISCVAllocateArgument(clover::Type type, const RISCVTargetInfo& target,
	RISCVABIArgumentState& state, bool force_integer_registers,
	RISCVABIArgumentLocation& location);

// Classify a by-value aggregate as one/two XLEN pieces or an indirect pointer.
bool RISCVAllocateAggregateArgument(const clover::Module& module,
	const clover::TypeReference& type, const RISCVTargetInfo& target,
	RISCVABIArgumentState& state, bool force_integer_registers,
	RISCVABIArgumentLocation& location);

}

#endif
