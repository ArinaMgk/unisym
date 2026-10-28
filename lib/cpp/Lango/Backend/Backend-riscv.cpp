// ASCII C++11 TAB4 LF
// Docutitle: Clover RISC-V Backend
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

#include "../../../../inc/cpp/Lango/Backend/Backend-riscv.hpp"

namespace uni::lango {

	namespace {

		// Check target-wide requirements before writing partial assembly output.
		bool ValidateTarget(const RISCVTargetInfo& target, const clover::Module& module) {
			if (target.bits != 64) {
				plogerro("RISC-V backend: unsupported -mbits=%[u]; supported bits: 64", target.bits);
				return false;
			}
			if (!target.extension_i) {
				plogerro("RISC-V backend: the I instruction set is required");
				return false;
			}
			if (target.extension_m) return true;

			for (const clover::Function& function : module.functions) {
				for (const clover::BasicBlock& block : function.blocks) {
					for (const clover::Instruction& instruction : block.instructions) {
						if (instruction.opcode == clover::Opcode::Mul ||
							instruction.opcode == clover::Opcode::SDiv ||
							instruction.opcode == clover::Opcode::SRem) {
							plogerro("RISC-V backend: Clover mul/sdiv/srem requires -minst=im");
							return false;
						}
					}
				}
			}
			return true;
		}

		// Stack locations assigned to one SSA result and, for Local, its object.
		struct ValueSlot {
			bool defined = false;
			stdsint value_offset = 0;
			stdsint local_offset = 0;
		};

		// Function-specific stack layout used by the simple first backend.
		struct FunctionLayout {
			uni::Vector<ValueSlot> slots;
			stdsint frame_size = 0;
		};

		// Round a frame size up to the RISC-V ABI's 16-byte stack alignment.
		stdsint AlignFrame(stdsint size) {
			return (size + 15) & ~stdsint(15);
		}

		// Find the largest SSA identifier so it can index the slot vector.
		unsigned LargestValueID(const clover::Function& function, bool& found) {
			unsigned largest = 0;
			found = false;
			for (const clover::BasicBlock& block : function.blocks) {
				for (const clover::Instruction& instruction : block.instructions) {
					if (!instruction.has_result) continue;
					if (!found || instruction.result.id > largest) largest = instruction.result.id;
					found = true;
				}
			}
			return largest;
		}

		// Assign every SSA result a spill slot and every Local an object slot.
		bool BuildLayout(const clover::Function& function, FunctionLayout& layout) {
			bool found;
			unsigned largest = LargestValueID(function, found);
			layout.slots = uni::Vector<ValueSlot>::From(ValueSlot(), found ? largest + 1 : 0);
			stdsint offset = 16; // -8(s0) and -16(s0) save ra and the caller's s0.

			for (const clover::BasicBlock& block : function.blocks) {
				for (const clover::Instruction& instruction : block.instructions) {
					if (!instruction.has_result) continue;
					ValueSlot& slot = layout.slots[instruction.result.id];
					if (slot.defined) {
						plogerro("RISC-V backend: duplicate SSA value %%%[u]", instruction.result.id);
						return false;
					}
					slot.defined = true;
					slot.value_offset = offset += 8;
					if (instruction.opcode == clover::Opcode::Local)
						slot.local_offset = offset += 8;
				}
			}
			layout.frame_size = AlignFrame(offset);
			return true;
		}

		// Resolve an SSA value to the stack slot containing its runtime value.
		bool SlotOf(const FunctionLayout& layout, clover::Value value, const ValueSlot*& slot) {
			if (value.id >= layout.slots.Count() || !layout.slots.begin()[value.id].defined) {
				plogerro("RISC-V backend: undefined SSA value %%%[u]", value.id);
				return false;
			}
			slot = &layout.slots.begin()[value.id];
			return true;
		}

		// Require an exact operand count before indexing the operand vector.
		bool RequireOperands(const clover::Instruction& instruction, stduint count) {
			if (instruction.operands.Count() == count) return true;
			plogerro("RISC-V backend: bad operand count for Clover opcode %[u]",
				(stduint)instruction.opcode);
			return false;
		}

		// Load one SSA value from its spill slot into a temporary register.
		bool LoadValue(const FunctionLayout& layout, clover::Value value,
			const char* reg, uni::OstreamTrait& output) {
			const ValueSlot* slot;
			if (!SlotOf(layout, value, slot)) return false;
			output.OutFormat("\tld %s, -%[i](s0)\n", reg, slot->value_offset);
			return true;
		}

		// Store an instruction result from a temporary register to its spill slot.
		bool StoreResult(const FunctionLayout& layout, const clover::Instruction& instruction,
			const char* reg, uni::OstreamTrait& output) {
			const ValueSlot* slot;
			if (!instruction.has_result || !SlotOf(layout, instruction.result, slot)) return false;
			output.OutFormat("\tsd %s, -%[i](s0)\n", reg, slot->value_offset);
			return true;
		}

		// Emit the common function epilogue. t0 temporarily holds the caller's s0.
		void EmitEpilogue(uni::OstreamTrait& output) {
			output.OutFormat("\tld ra, -8(s0)\n");
			output.OutFormat("\tld t0, -16(s0)\n");
			output.OutFormat("\tmv sp, s0\n");
			output.OutFormat("\tmv s0, t0\n");
			output.OutFormat("\tret\n");
		}

		// Emit an arithmetic or bitwise operation with two loaded operands.
		bool EmitBinary(const clover::Instruction& instruction, const FunctionLayout& layout,
			const char* mnemonic, uni::OstreamTrait& output) {
			if (!RequireOperands(instruction, 2) ||
				!LoadValue(layout, instruction.operands.begin()[0], "t0", output) ||
				!LoadValue(layout, instruction.operands.begin()[1], "t1", output)) return false;
			output.OutFormat("\t%s t2, t0, t1\n", mnemonic);
			return StoreResult(layout, instruction, "t2", output);
		}

		// Lower one Clover instruction to RV64 assembler.
		bool EmitInstruction(const clover::Instruction& instruction, const FunctionLayout& layout,
			uni::OstreamTrait& output, bool& returned) {
			switch (instruction.opcode) {
			case clover::Opcode::Const:
				output.OutFormat("\tli t0, %[i]\n", instruction.immediate);
				return StoreResult(layout, instruction, "t0", output);

			case clover::Opcode::Local: {
				const ValueSlot* slot;
				if (!instruction.has_result || !SlotOf(layout, instruction.result, slot) || !slot->local_offset)
					return false;
				output.OutFormat("\taddi t0, s0, -%[i]\n", slot->local_offset);
				return StoreResult(layout, instruction, "t0", output);
			}

			case clover::Opcode::Load:
				if (!RequireOperands(instruction, 1) ||
					!LoadValue(layout, instruction.operands.begin()[0], "t0", output)) return false;
				output.OutFormat("\tld t1, 0(t0)\n");
				return StoreResult(layout, instruction, "t1", output);

			case clover::Opcode::Store:
				if (!RequireOperands(instruction, 2) ||
					!LoadValue(layout, instruction.operands.begin()[0], "t0", output) ||
					!LoadValue(layout, instruction.operands.begin()[1], "t1", output)) return false;
				output.OutFormat("\tsd t0, 0(t1)\n");
				return true;

			case clover::Opcode::Mov:
				if (!RequireOperands(instruction, 1) ||
					!LoadValue(layout, instruction.operands.begin()[0], "t0", output)) return false;
				return StoreResult(layout, instruction, "t0", output);

			case clover::Opcode::Add: return EmitBinary(instruction, layout, "add", output);
			case clover::Opcode::Sub: return EmitBinary(instruction, layout, "sub", output);
			case clover::Opcode::Mul: return EmitBinary(instruction, layout, "mul", output);
			case clover::Opcode::SDiv: return EmitBinary(instruction, layout, "div", output);
			case clover::Opcode::SRem: return EmitBinary(instruction, layout, "rem", output);
			case clover::Opcode::And: return EmitBinary(instruction, layout, "and", output);
			case clover::Opcode::Or: return EmitBinary(instruction, layout, "or", output);
			case clover::Opcode::Xor: return EmitBinary(instruction, layout, "xor", output);
			case clover::Opcode::Shl: return EmitBinary(instruction, layout, "sll", output);
			case clover::Opcode::AShr: return EmitBinary(instruction, layout, "sra", output);

			case clover::Opcode::ICmp:
				if (!RequireOperands(instruction, 2) ||
					!LoadValue(layout, instruction.operands.begin()[0], "t0", output) ||
					!LoadValue(layout, instruction.operands.begin()[1], "t1", output)) return false;
				switch (instruction.predicate) {
				case clover::ICmpPredicate::EQ:
					output.OutFormat("\txor t2, t0, t1\n\tseqz t2, t2\n");
					break;
				case clover::ICmpPredicate::NE:
					output.OutFormat("\txor t2, t0, t1\n\tsnez t2, t2\n");
					break;
				case clover::ICmpPredicate::SLT:
					output.OutFormat("\tslt t2, t0, t1\n");
					break;
				case clover::ICmpPredicate::SLE:
					output.OutFormat("\tslt t2, t1, t0\n\txori t2, t2, 1\n");
					break;
				case clover::ICmpPredicate::SGT:
					output.OutFormat("\tslt t2, t1, t0\n");
					break;
				case clover::ICmpPredicate::SGE:
					output.OutFormat("\tslt t2, t0, t1\n\txori t2, t2, 1\n");
					break;
				}
				return StoreResult(layout, instruction, "t2", output);

			case clover::Opcode::ZExt:
				if (!RequireOperands(instruction, 1) ||
					!LoadValue(layout, instruction.operands.begin()[0], "t0", output)) return false;
				if (instruction.operands.begin()[0].type == clover::Type::I1)
					output.OutFormat("\tandi t0, t0, 1\n");
				return StoreResult(layout, instruction, "t0", output);

			case clover::Opcode::Ret:
				if (!RequireOperands(instruction, 1) ||
					!LoadValue(layout, instruction.operands.begin()[0], "a0", output)) return false;
				EmitEpilogue(output);
				returned = true;
				return true;
			}
			return false;
		}

		// Emit one function with a conservative stack slot for every SSA result.
		bool EmitFunction(const clover::Function& function, uni::OstreamTrait& output) {
			FunctionLayout layout;
			if (!BuildLayout(function, layout)) return false;

			output.OutFormat("\n.globl %s\n", function.name.reference());
			output.OutFormat(".type %s, @function\n", function.name.reference());
			output.OutFormat("%s:\n", function.name.reference());
			output.OutFormat("\taddi sp, sp, -%[i]\n", layout.frame_size);
			output.OutFormat("\tsd ra, %[i](sp)\n", layout.frame_size - 8);
			output.OutFormat("\tsd s0, %[i](sp)\n", layout.frame_size - 16);
			output.OutFormat("\taddi s0, sp, %[i]\n", layout.frame_size);

			bool returned = false;
			for (const clover::BasicBlock& block : function.blocks) {
				output.OutFormat(".L%s_%s:\n", function.name.reference(), block.name.reference());
				for (const clover::Instruction& instruction : block.instructions)
					if (!EmitInstruction(instruction, layout, output, returned)) return false;
			}
			if (!returned) {
				output.OutFormat("\tli a0, 0\n");
				EmitEpilogue(output);
			}
			output.OutFormat(".size %s, .-%s\n", function.name.reference(), function.name.reference());
			return true;
		}

	}

	bool BackendRISCV::Generate(const clover::Module& module, uni::OstreamTrait& output) {
		if (!ValidateTarget(target, module)) return false;
		output.OutFormat(".text\n");
		for (const clover::Function& function : module.functions)
			if (!EmitFunction(function, output)) return false;
		return true;
	}

}
