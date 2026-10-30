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
		const char* ArgumentRegisters[] = {
			"a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7"
		};

		const clover::Function* FindFunction(const clover::Module& module,
			const uni::String& name) {
			for (const clover::Function& function : module.functions)
				if (!StrCompare(function.name.reference(), name.reference())) return &function;
			return nullptr;
		}

		const clover::Global* FindGlobal(const clover::Module& module,
			const uni::String& name) {
			for (const clover::Global& global : module.globals)
				if (!StrCompare(global.name.reference(), name.reference())) return &global;
			return nullptr;
		}

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
			for (stduint index = 0; index < module.globals.Count(); index++) {
				const clover::Global& global = module.globals.begin()[index];
				if (!global.name.getByteCount() || global.type == clover::Type::Void ||
					global.element_count <= 0) {
					plogerro("RISC-V backend: invalid global object");
					return false;
				}
				for (stduint previous = 0; previous < index; previous++)
					if (!StrCompare(global.name.reference(),
						module.globals.begin()[previous].name.reference())) {
						plogerro("RISC-V backend: duplicate global '%s'", global.name.reference());
						return false;
					}
			}
			for (stduint function_index = 0;
				function_index < module.functions.Count(); function_index++) {
				const clover::Function& function = module.functions.begin()[function_index];
				if (FindGlobal(module, function.name)) {
					plogerro("RISC-V backend: function conflicts with global '%s'",
						function.name.reference());
					return false;
				}
				for (stduint previous = 0; previous < function_index; previous++)
					if (!StrCompare(function.name.reference(),
						module.functions.begin()[previous].name.reference())) {
						plogerro("RISC-V backend: duplicate function '%s'",
							function.name.reference());
						return false;
					}
				if (function.parameters.Count() > numsof(ArgumentRegisters)) {
					plogerro("RISC-V backend: more than 8 register parameters are not supported");
					return false;
				}
				for (const clover::BasicBlock& block : function.blocks) {
					for (const clover::Instruction& instruction : block.instructions) {
						if (instruction.opcode == clover::Opcode::GlobalAddr &&
							!FindGlobal(module, instruction.name)) {
							plogerro("RISC-V backend: undefined global '%s'",
								instruction.name.reference());
							return false;
						}
						if (!target.extension_m &&
							(instruction.opcode == clover::Opcode::Mul ||
							instruction.opcode == clover::Opcode::SDiv ||
							instruction.opcode == clover::Opcode::UDiv ||
							instruction.opcode == clover::Opcode::SRem ||
							instruction.opcode == clover::Opcode::URem)) {
							plogerro("RISC-V backend: Clover multiply/divide/remainder requires -minst=im");
							return false;
						}
						if (instruction.opcode == clover::Opcode::Call) {
							const clover::Function* callee = FindFunction(module, instruction.callee);
							if (!callee) {
								plogerro("RISC-V backend: call to undefined function '%s'",
									instruction.callee.reference());
								return false;
							}
							if (instruction.type != callee->return_type ||
								instruction.operands.Count() != callee->parameters.Count()) {
								plogerro("RISC-V backend: call signature mismatch for '%s'",
									instruction.callee.reference());
								return false;
							}
							for (stduint index = 0; index < instruction.operands.Count(); index++)
								if (instruction.operands.begin()[index].type !=
									callee->parameters.begin()[index].value.type) {
									plogerro("RISC-V backend: call argument type mismatch for '%s'",
										instruction.callee.reference());
									return false;
								}
						}
					}
				}
			}
			return true;
		}

		// Stack location assigned to an SSA spill or a rematerialized Local object.
		struct ValueSlot {
			bool defined = false;
			stdsint value_offset = 0; // Runtime SSA value spill; zero for Local.
			stdsint local_offset = 0; // Local object address rematerialized from s0.
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

		stdsint TypeBits(clover::Type type, const RISCVTargetInfo& target) {
			switch (type) {
			case clover::Type::Void: return 0;
			case clover::Type::I1: return 1;
			case clover::Type::I8: return 8;
			case clover::Type::I16: return 16;
			case clover::Type::I32: return 32;
			case clover::Type::I64: return 64;
			case clover::Type::Ptr: return target.bits;
			}
			return 0;
		}

		stdsint TypeStorageBytes(clover::Type type, const RISCVTargetInfo& target) {
			stdsint bits = TypeBits(type, target);
			return bits ? (bits + 7) / 8 : 0;
		}

		stdsint AlignOffset(stdsint offset, stdsint alignment) {
			return (offset + alignment - 1) & ~(alignment - 1);
		}

		// Find the largest SSA identifier so it can index the slot vector.
		unsigned LargestValueID(const clover::Function& function, bool& found) {
			unsigned largest = 0;
			found = false;
			for (const clover::Parameter& parameter : function.parameters) {
				if (!found || parameter.value.id > largest) largest = parameter.value.id;
				found = true;
			}
			for (const clover::BasicBlock& block : function.blocks) {
				for (const clover::Instruction& instruction : block.instructions) {
					if (!instruction.has_result) continue;
					if (!found || instruction.result.id > largest) largest = instruction.result.id;
					found = true;
				}
			}
			return largest;
		}

		// Spill ordinary SSA results; Local results retain only their object offset.
		bool BuildLayout(const clover::Function& function, const RISCVTargetInfo& target,
			FunctionLayout& layout) {
			bool found;
			unsigned largest = LargestValueID(function, found);
			layout.slots = uni::Vector<ValueSlot>::From(ValueSlot(), found ? largest + 1 : 0);
			stdsint offset = 16; // -8(s0) and -16(s0) save ra and the caller's s0.
			for (const clover::Parameter& parameter : function.parameters) {
				ValueSlot& slot = layout.slots[parameter.value.id];
				if (slot.defined) {
					plogerro("RISC-V backend: duplicate SSA value %%%[u]", parameter.value.id);
					return false;
				}
				slot.defined = true;
				slot.value_offset = offset += 8;
			}

			for (const clover::BasicBlock& block : function.blocks) {
				for (const clover::Instruction& instruction : block.instructions) {
					if (!instruction.has_result) continue;
					ValueSlot& slot = layout.slots[instruction.result.id];
					if (slot.defined) {
						plogerro("RISC-V backend: duplicate SSA value %%%[u]", instruction.result.id);
						return false;
					}
					slot.defined = true;
					if (instruction.opcode == clover::Opcode::Local) {
						stdsint element_size = TypeStorageBytes(instruction.type, target);
						stdsint element_count = instruction.immediate > 0 ?
							instruction.immediate : 1;
						if (!element_size || element_count <= 0) return false;
						offset = AlignOffset(offset, element_size);
						slot.local_offset = offset += element_size * element_count;
					}
					else slot.value_offset = offset += 8;
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

		bool HasBlock(const clover::Function& function, const uni::String& name) {
			for (const clover::BasicBlock& block : function.blocks)
				if (!StrCompare(block.name.reference(), name.reference())) return true;
			return false;
		}

		// Load a spilled SSA value, or rematerialize a Local frame address.
		bool LoadValue(const FunctionLayout& layout, clover::Value value,
			const char* reg, uni::OstreamTrait& output) {
			const ValueSlot* slot;
			if (!SlotOf(layout, value, slot)) return false;
			if (slot->local_offset) {
				output.OutFormat("\taddi %s, s0, -%[i]\n", reg, slot->local_offset);
				return true;
			}
			if (!slot->value_offset) return false;
			output.OutFormat("\tld %s, -%[i](s0)\n", reg, slot->value_offset);
			return true;
		}

		// Store an instruction result from a temporary register to its spill slot.
		bool StoreResult(const FunctionLayout& layout, const clover::Instruction& instruction,
			const char* reg, uni::OstreamTrait& output) {
			const ValueSlot* slot;
			if (!instruction.has_result || !SlotOf(layout, instruction.result, slot)) return false;
			if (!slot->value_offset) return false;
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

		// Scale a signed element index and add it to a target-width pointer.
		bool EmitPtrAdd(const clover::Instruction& instruction,
			const FunctionLayout& layout, const RISCVTargetInfo& target,
			uni::OstreamTrait& output) {
			if (!RequireOperands(instruction, 2) ||
				instruction.operands.begin()[0].type != clover::Type::Ptr ||
				instruction.operands.begin()[1].type == clover::Type::Ptr ||
				!LoadValue(layout, instruction.operands.begin()[0], "t0", output) ||
				!LoadValue(layout, instruction.operands.begin()[1], "t1", output)) return false;
			stdsint element_size = TypeStorageBytes(instruction.type, target);
			switch (element_size) {
			case 1: break;
			case 2: output.OutFormat("\tslli t1, t1, 1\n"); break;
			case 4: output.OutFormat("\tslli t1, t1, 2\n"); break;
			case 8: output.OutFormat("\tslli t1, t1, 3\n"); break;
			default:
				plogerro("RISC-V backend: unsupported ptradd element size %[i]", element_size);
				return false;
			}
			output.OutFormat("\tadd t2, t0, t1\n");
			return StoreResult(layout, instruction, "t2", output);
		}

		const char* BinaryMnemonic(const clover::Instruction& instruction,
			const char* wide, const char* word) {
			return instruction.type == clover::Type::I32 ? word : wide;
		}

		// Lower one Clover instruction to RV64 assembler.
		bool EmitInstruction(const clover::Instruction& instruction, const FunctionLayout& layout,
			const clover::Function& function, const RISCVTargetInfo& target,
			uni::OstreamTrait& output) {
			switch (instruction.opcode) {
			case clover::Opcode::Const:
				output.OutFormat("\tli t0, %[i]\n", instruction.immediate);
				return StoreResult(layout, instruction, "t0", output);

			case clover::Opcode::Local: {
				const ValueSlot* slot;
				if (!instruction.has_result || !SlotOf(layout, instruction.result, slot) || !slot->local_offset)
					return false;
				return true;
			}

			case clover::Opcode::Load:
				if (!RequireOperands(instruction, 1) ||
					!LoadValue(layout, instruction.operands.begin()[0], "t0", output)) return false;
				switch (instruction.type) {
				case clover::Type::I1:
				case clover::Type::I8: output.OutFormat("\tlb t1, 0(t0)\n"); break;
				case clover::Type::I16: output.OutFormat("\tlh t1, 0(t0)\n"); break;
				case clover::Type::I32: output.OutFormat("\tlw t1, 0(t0)\n"); break;
				case clover::Type::I64:
				case clover::Type::Ptr: output.OutFormat("\tld t1, 0(t0)\n"); break;
				}
				return StoreResult(layout, instruction, "t1", output);

			case clover::Opcode::Store:
				if (!RequireOperands(instruction, 2) ||
					!LoadValue(layout, instruction.operands.begin()[0], "t0", output) ||
					!LoadValue(layout, instruction.operands.begin()[1], "t1", output)) return false;
				switch (instruction.type) {
				case clover::Type::I1:
				case clover::Type::I8: output.OutFormat("\tsb t0, 0(t1)\n"); break;
				case clover::Type::I16: output.OutFormat("\tsh t0, 0(t1)\n"); break;
				case clover::Type::I32: output.OutFormat("\tsw t0, 0(t1)\n"); break;
				case clover::Type::I64:
				case clover::Type::Ptr: output.OutFormat("\tsd t0, 0(t1)\n"); break;
				}
				return true;

			case clover::Opcode::Mov:
				if (!RequireOperands(instruction, 1) ||
					!LoadValue(layout, instruction.operands.begin()[0], "t0", output)) return false;
				return StoreResult(layout, instruction, "t0", output);

			case clover::Opcode::Add: return EmitBinary(instruction, layout,
				BinaryMnemonic(instruction, "add", "addw"), output);
			case clover::Opcode::Sub: return EmitBinary(instruction, layout,
				BinaryMnemonic(instruction, "sub", "subw"), output);
			case clover::Opcode::Mul: return EmitBinary(instruction, layout,
				BinaryMnemonic(instruction, "mul", "mulw"), output);
			case clover::Opcode::SDiv: return EmitBinary(instruction, layout,
				BinaryMnemonic(instruction, "div", "divw"), output);
			case clover::Opcode::UDiv: return EmitBinary(instruction, layout,
				BinaryMnemonic(instruction, "divu", "divuw"), output);
			case clover::Opcode::SRem: return EmitBinary(instruction, layout,
				BinaryMnemonic(instruction, "rem", "remw"), output);
			case clover::Opcode::URem: return EmitBinary(instruction, layout,
				BinaryMnemonic(instruction, "remu", "remuw"), output);
			case clover::Opcode::And: return EmitBinary(instruction, layout, "and", output);
			case clover::Opcode::Or: return EmitBinary(instruction, layout, "or", output);
			case clover::Opcode::Xor: return EmitBinary(instruction, layout, "xor", output);
			case clover::Opcode::Shl: return EmitBinary(instruction, layout,
				BinaryMnemonic(instruction, "sll", "sllw"), output);
			case clover::Opcode::AShr: return EmitBinary(instruction, layout,
				BinaryMnemonic(instruction, "sra", "sraw"), output);
			case clover::Opcode::LShr: return EmitBinary(instruction, layout,
				BinaryMnemonic(instruction, "srl", "srlw"), output);
			case clover::Opcode::PtrAdd:
				return EmitPtrAdd(instruction, layout, target, output);

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
				case clover::ICmpPredicate::ULT:
					output.OutFormat("\tsltu t2, t0, t1\n");
					break;
				case clover::ICmpPredicate::ULE:
					output.OutFormat("\tsltu t2, t1, t0\n\txori t2, t2, 1\n");
					break;
				case clover::ICmpPredicate::UGT:
					output.OutFormat("\tsltu t2, t1, t0\n");
					break;
				case clover::ICmpPredicate::UGE:
					output.OutFormat("\tsltu t2, t0, t1\n\txori t2, t2, 1\n");
					break;
				}
				return StoreResult(layout, instruction, "t2", output);

			case clover::Opcode::ZExt:
				if (!RequireOperands(instruction, 1) ||
					!LoadValue(layout, instruction.operands.begin()[0], "t0", output)) return false;
				switch (TypeBits(instruction.operands.begin()[0].type, target)) {
				case 1: output.OutFormat("\tandi t0, t0, 1\n"); break;
				case 8: output.OutFormat("\tandi t0, t0, 255\n"); break;
				case 16: output.OutFormat("\tslli t0, t0, 48\n\tsrli t0, t0, 48\n"); break;
				case 32: output.OutFormat("\tslli t0, t0, 32\n\tsrli t0, t0, 32\n"); break;
				}
				return StoreResult(layout, instruction, "t0", output);

			case clover::Opcode::SExt:
				if (!RequireOperands(instruction, 1) ||
					!LoadValue(layout, instruction.operands.begin()[0], "t0", output)) return false;
				switch (TypeBits(instruction.operands.begin()[0].type, target)) {
				case 1: output.OutFormat("\tslli t0, t0, 63\n\tsrai t0, t0, 63\n"); break;
				case 8: output.OutFormat("\tslli t0, t0, 56\n\tsrai t0, t0, 56\n"); break;
				case 16: output.OutFormat("\tslli t0, t0, 48\n\tsrai t0, t0, 48\n"); break;
				case 32: output.OutFormat("\taddiw t0, t0, 0\n"); break;
				}
				return StoreResult(layout, instruction, "t0", output);

			case clover::Opcode::Trunc:
				if (!RequireOperands(instruction, 1) ||
					!LoadValue(layout, instruction.operands.begin()[0], "t0", output)) return false;
				switch (TypeBits(instruction.type, target)) {
				case 1: output.OutFormat("\tandi t0, t0, 1\n"); break;
				case 8: output.OutFormat("\tandi t0, t0, 255\n"); break;
				case 16: output.OutFormat("\tslli t0, t0, 48\n\tsrli t0, t0, 48\n"); break;
				case 32: output.OutFormat("\tslli t0, t0, 32\n\tsrli t0, t0, 32\n"); break;
				}
				return StoreResult(layout, instruction, "t0", output);

			case clover::Opcode::GlobalAddr:
				if (!instruction.name.getByteCount()) return false;
				output.OutFormat("\tla t0, %s\n", instruction.name.reference());
				return StoreResult(layout, instruction, "t0", output);

			case clover::Opcode::Call:
				if (instruction.operands.Count() > numsof(ArgumentRegisters)) return false;
				for (stduint index = 0; index < instruction.operands.Count(); index++) {
					const char* reg = ArgumentRegisters[index];
					if (!LoadValue(layout, instruction.operands.begin()[index], reg, output))
						return false;
					if (instruction.operands.begin()[index].type == clover::Type::I32)
						output.OutFormat("\taddiw %s, %s, 0\n", reg, reg);
				}
				output.OutFormat("\tcall %s\n", instruction.callee.reference());
				if (instruction.type == clover::Type::Void) return true;
				if (instruction.type == clover::Type::I32)
					output.OutFormat("\taddiw a0, a0, 0\n");
				return StoreResult(layout, instruction, "a0", output);

			case clover::Opcode::Jmp:
				if (!HasBlock(function, instruction.target) ||
					(instruction.target_false.getByteCount() &&
						!HasBlock(function, instruction.target_false))) {
					plogerro("RISC-V backend: jump to undefined block");
					return false;
				}
				if (!instruction.operands.Count()) {
					output.OutFormat("\tj .L%s_%s\n", function.name.reference(),
						instruction.target.reference());
					return true;
				}
				if (!RequireOperands(instruction, 1) ||
					!instruction.target_false.getByteCount() ||
					instruction.operands.begin()[0].type != clover::Type::I1 ||
					!LoadValue(layout, instruction.operands.begin()[0], "t0", output)) return false;
				output.OutFormat("\tbnez t0, .L%s_%s\n", function.name.reference(),
					instruction.target.reference());
				output.OutFormat("\tj .L%s_%s\n", function.name.reference(),
					instruction.target_false.reference());
				return true;

			case clover::Opcode::Ret:
				if (instruction.type == clover::Type::Void) {
					if (instruction.operands.Count()) return false;
					EmitEpilogue(output);
					return true;
				}
				if (!RequireOperands(instruction, 1) ||
					!LoadValue(layout, instruction.operands.begin()[0], "a0", output)) return false;
				EmitEpilogue(output);
				return true;
			}
			return false;
		}

		// Emit one function with a conservative stack slot for every SSA result.
		bool EmitFunction(const clover::Function& function, const RISCVTargetInfo& target,
			uni::OstreamTrait& output) {
			FunctionLayout layout;
			if (!BuildLayout(function, target, layout)) return false;

			output.OutFormat("\n.globl %s\n", function.name.reference());
			output.OutFormat(".type %s, @function\n", function.name.reference());
			output.OutFormat("%s:\n", function.name.reference());
			output.OutFormat("\taddi sp, sp, -%[i]\n", layout.frame_size);
			output.OutFormat("\tsd ra, %[i](sp)\n", layout.frame_size - 8);
			output.OutFormat("\tsd s0, %[i](sp)\n", layout.frame_size - 16);
			output.OutFormat("\taddi s0, sp, %[i]\n", layout.frame_size);
			for (stduint index = 0; index < function.parameters.Count(); index++) {
				const clover::Parameter& parameter = function.parameters.begin()[index];
				const ValueSlot* slot;
				if (!SlotOf(layout, parameter.value, slot) || !slot->value_offset) return false;
				const char* reg = ArgumentRegisters[index];
				if (parameter.value.type == clover::Type::I32)
					output.OutFormat("\taddiw %s, %s, 0\n", reg, reg);
				output.OutFormat("\tsd %s, -%[i](s0)\n", reg, slot->value_offset);
			}

			for (const clover::BasicBlock& block : function.blocks) {
				output.OutFormat(".L%s_%s:\n", function.name.reference(), block.name.reference());
				for (const clover::Instruction& instruction : block.instructions)
					if (!EmitInstruction(instruction, layout, function, target, output)) return false;
			}
			bool terminated = false;
			if (function.blocks.Count()) {
				const clover::BasicBlock& last_block =
					function.blocks.begin()[function.blocks.Count() - 1];
				if (last_block.instructions.Count()) {
					clover::Opcode opcode = last_block.instructions.begin()[
						last_block.instructions.Count() - 1].opcode;
					terminated = opcode == clover::Opcode::Ret || opcode == clover::Opcode::Jmp;
				}
			}
			if (!terminated) {
				output.OutFormat("\tli a0, 0\n");
				EmitEpilogue(output);
			}
			output.OutFormat(".size %s, .-%s\n", function.name.reference(), function.name.reference());
			return true;
		}

	}

	bool BackendRISCV::Generate(const clover::Module& module, uni::OstreamTrait& output) {
		if (!ValidateTarget(target, module)) return false;
		for (const clover::Global& global : module.globals) {
			stdsint element_size = TypeStorageBytes(global.type, target);
			stdsint object_size = element_size * global.element_count;
			if (!element_size || object_size <= 0) return false;
			output.OutFormat(global.initializer ? ".data\n" : ".bss\n");
			output.OutFormat(".globl %s\n.type %s, @object\n.size %s, %[i]\n",
				global.name.reference(), global.name.reference(), global.name.reference(), object_size);
			output.OutFormat(".balign %[i]\n%s:\n", element_size,
				global.name.reference());
			if (!global.initializer) output.OutFormat("\t.zero %[i]\n", object_size);
			else if (global.element_count != 1) return false;
			else {
				switch (element_size) {
				case 1: output.OutFormat("\t.byte %[i]\n", global.initializer); break;
				case 2: output.OutFormat("\t.half %[i]\n", global.initializer); break;
				case 4: output.OutFormat("\t.word %[i]\n", global.initializer); break;
				case 8: output.OutFormat("\t.quad %[i]\n", global.initializer); break;
				default: return false;
				}
			}
		}
		output.OutFormat(".text\n");
		for (const clover::Function& function : module.functions)
			if (!EmitFunction(function, target, output)) return false;
		return true;
	}

}
