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
#include "../../../../inc/cpp/Lango/Backend/RISCV/Output-aasm.hpp"
#include "../../../../inc/cpp/Lango/Backend/RISCV/Output-gcc.hpp"

namespace uni::lango {

	namespace {
		const char* ArgumentRegisters[] = {
			"a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7"
		};
		const char* FloatArgumentRegisters[] = {
			"fa0", "fa1", "fa2", "fa3", "fa4", "fa5", "fa6", "fa7"
		};

		stdsint TypeBits(clover::Type type, const RISCVTargetInfo& target);
		stdsint TypeStorageBytes(clover::Type type, const RISCVTargetInfo& target);

		bool IsFloatType(clover::Type type) {
			return type == clover::Type::F32 || type == clover::Type::F64 ||
				type == clover::Type::F128;
		}

		unsigned ABIFloatBits(const RISCVTargetInfo& target) {
			switch (target.abi) {
			case RISCVABI::Default:
			case RISCVABI::ILP32:
			case RISCVABI::LP64:
				return 0;
			case RISCVABI::ILP32F:
			case RISCVABI::LP64F:
				return 32;
			case RISCVABI::ILP32D:
			case RISCVABI::LP64D:
				return 64;
			}
			return 0;
		}

		bool AllocateArgumentRegister(clover::Type type,
			const RISCVTargetInfo& target, unsigned& integer_count,
			unsigned& float_count, bool& use_float_register,
			unsigned& register_index) {
			if (IsFloatType(type) &&
				TypeBits(type, target) <= ABIFloatBits(target) &&
				float_count < numsof(FloatArgumentRegisters)) {
				use_float_register = true;
				register_index = float_count++;
				return true;
			}
			if (type == clover::Type::F128) {
				if (integer_count + 1 >= numsof(ArgumentRegisters)) return false;
				use_float_register = false;
				register_index = integer_count;
				integer_count += 2;
				return true;
			}
			if (integer_count >= numsof(ArgumentRegisters)) return false;
			use_float_register = false;
			register_index = integer_count++;
			return true;
		}

		bool ValidateRegisterArguments(const clover::Function& function,
			const RISCVTargetInfo& target) {
			unsigned integer_count = 0;
			unsigned float_count = 0;
			for (const clover::Parameter& parameter : function.parameters) {
				if (parameter.aggregate_group && IsFloatType(parameter.value.type) &&
					TypeBits(parameter.value.type, target) > ABIFloatBits(target)) {
					plogerro("RISC-V backend: flattened aggregate floating member exceeds ABI FLEN");
					return false;
				}
				bool use_float_register;
				unsigned register_index;
				if (!AllocateArgumentRegister(parameter.value.type, target,
					integer_count, float_count, use_float_register,
					register_index)) {
					plogerro("RISC-V backend: stack function arguments are not supported yet");
					return false;
				}
			}
			return true;
		}

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

		const clover::SourceFile* FindSourceFile(const clover::Module& module,
			uint32 id) {
			for (const clover::SourceFile& source : module.source_files)
				if (source.id == id) return &source;
			return nullptr;
		}

		// Check target-wide requirements before writing partial assembly output.
		bool ValidateTarget(const RISCVTargetInfo& target, const clover::Module& module) {
			if (target.bits != 64) {
				plogerro("RISC-V backend: unsupported -mbits=%[u]; supported bits: 64", target.bits);
				return false;
			}
			if (target.abi == RISCVABI::ILP32 || target.abi == RISCVABI::ILP32F ||
				target.abi == RISCVABI::ILP32D) {
				plogerro("RISC-V backend: ILP32 ABIs require a 32-bit backend target");
				return false;
			}
			if (module.pointer_bits && module.pointer_bits != target.bits) {
				plogerro("RISC-V backend: IR requires %[u]-bit pointers, but target uses %[u]-bit pointers",
					module.pointer_bits, target.bits);
				return false;
			}
			if (!target.extension_i) {
				plogerro("RISC-V backend: the I instruction set is required");
				return false;
			}
			if (target.extension_d && !target.extension_f) {
				plogerro("RISC-V backend: the D extension requires the F extension");
				return false;
			}
			if (target.abi == RISCVABI::LP64F && !target.extension_f) {
				plogerro("RISC-V backend: -mabi=lp64f requires -minst=imf or imfd");
				return false;
			}
			if (target.abi == RISCVABI::LP64D && !target.extension_d) {
				plogerro("RISC-V backend: -mabi=lp64d requires -minst=imfd");
				return false;
			}
			for (stduint index = 0; index < module.source_files.Count(); index++) {
				const clover::SourceFile& source = module.source_files.begin()[index];
				if (!source.id || !source.path.getByteCount()) {
					plogerro("RISC-V backend: invalid source file entry");
					return false;
				}
				for (stduint previous = 0; previous < index; previous++)
					if (module.source_files.begin()[previous].id == source.id) {
						plogerro("RISC-V backend: duplicate source file identifier %[u]",
							source.id);
						return false;
					}
			}
			for (stduint index = 0; index < module.globals.Count(); index++) {
				const clover::Global& global = module.globals.begin()[index];
				if (global.type == clover::Type::F32 && !target.extension_f) {
					plogerro("RISC-V backend: f32 globals require -minst=imf or imfd");
					return false;
				}
				if (global.type == clover::Type::F64 && !target.extension_d) {
					plogerro("RISC-V backend: f64 globals require -minst=imfd");
					return false;
				}
				if (!global.name.getByteCount() || global.type == clover::Type::Void ||
					global.element_count <= 0) {
					plogerro("RISC-V backend: invalid global object");
					return false;
				}
				if (global.alignment &&
					((global.alignment & (global.alignment - 1)) ||
					global.alignment > 16)) {
					plogerro("RISC-V backend: global alignment must be a power of two up to 16 bytes");
					return false;
				}
				if (global.initializer_values.Count() &&
					global.initializer_fragments.Count()) {
					plogerro("RISC-V backend: global object has conflicting initializer forms");
					return false;
				}
				if (global.initializer_values.Count() &&
					global.initializer_values.Count() != (stduint)global.element_count) {
					plogerro("RISC-V backend: global initializer element count mismatch");
					return false;
				}
				stdsint object_size = TypeStorageBytes(global.type, target) *
					global.element_count;
				stdsint fragment_end = 0;
				for (const clover::GlobalInitializerFragment& fragment :
					global.initializer_fragments) {
					stdsint fragment_size = TypeStorageBytes(fragment.type, target);
					if (!fragment_size || fragment.offset < (stduint)fragment_end ||
						fragment.offset + fragment_size > (stduint)object_size) {
						plogerro("RISC-V backend: invalid global initializer fragment");
						return false;
					}
					if (fragment.type == clover::Type::F32 && !target.extension_f) {
						plogerro("RISC-V backend: f32 global data requires -minst=imf or imfd");
						return false;
					}
					if (fragment.type == clover::Type::F64 && !target.extension_d) {
						plogerro("RISC-V backend: f64 global data requires -minst=imfd");
						return false;
					}
					fragment_end = (stdsint)fragment.offset + fragment_size;
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
				if (!ValidateRegisterArguments(function, target)) return false;
				for (const clover::BasicBlock& block : function.blocks) {
					for (const clover::Instruction& instruction : block.instructions) {
						bool uses_f32 = instruction.type == clover::Type::F32;
						bool uses_f64 = instruction.type == clover::Type::F64;
						for (const clover::Value& operand : instruction.operands) {
							uses_f32 = uses_f32 || operand.type == clover::Type::F32;
							uses_f64 = uses_f64 || operand.type == clover::Type::F64;
						}
						if (uses_f32 && !target.extension_f) {
							plogerro("RISC-V backend: f32 Clover operations require -minst=imf or imfd");
							return false;
						}
						if (uses_f64 && !target.extension_d) {
							plogerro("RISC-V backend: f64 Clover operations require -minst=imfd");
							return false;
						}
						if ((instruction.debug_file == 0) !=
							(instruction.debug_line == 0)) {
							plogerro("RISC-V backend: incomplete instruction source location");
							return false;
						}
						if (instruction.debug_file &&
							!FindSourceFile(module, instruction.debug_file)) {
							plogerro("RISC-V backend: instruction references undefined source file %[u]",
								instruction.debug_file);
							return false;
						}
						if (instruction.opcode == clover::Opcode::GlobalAddr &&
							!FindGlobal(module, instruction.name)) {
							plogerro("RISC-V backend: undefined global '%s'",
								instruction.name.reference());
							return false;
						}
						if (instruction.opcode == clover::Opcode::Local &&
							instruction.alignment &&
							((instruction.alignment & (instruction.alignment - 1)) ||
							instruction.alignment > 16)) {
							plogerro("RISC-V backend: local alignment must be a power of two up to 16 bytes");
							return false;
						}
						if ((instruction.opcode == clover::Opcode::Load ||
							instruction.opcode == clover::Opcode::Store) &&
							instruction.alignment) {
							if (instruction.alignment & (instruction.alignment - 1)) {
								plogerro("RISC-V backend: memory alignment must be a power of two");
								return false;
							}
						}
						if (instruction.opcode == clover::Opcode::MemCopy) {
							if (instruction.immediate <= 0 ||
								instruction.operands.Count() != 2 ||
								instruction.operands.begin()[0].type != clover::Type::Ptr ||
								instruction.operands.begin()[1].type != clover::Type::Ptr ||
								(instruction.alignment &&
									(instruction.alignment & (instruction.alignment - 1))) ||
								(instruction.source_alignment &&
									(instruction.source_alignment &
										(instruction.source_alignment - 1)))) {
								plogerro("RISC-V backend: invalid memcopy instruction");
								return false;
							}
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
								instruction.operands.Count() < callee->parameters.Count() ||
								(!callee->is_variadic && instruction.operands.Count() !=
									callee->parameters.Count())) {
								plogerro("RISC-V backend: call signature mismatch for '%s'",
									instruction.callee.reference());
								return false;
							}
							for (stduint index = 0; index < callee->parameters.Count(); index++)
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
			case clover::Type::F32: return 32;
			case clover::Type::F64: return 64;
			case clover::Type::F128: return 128;
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
				stdsint size = TypeStorageBytes(parameter.value.type, target);
				if (size < 8) size = 8;
				stdsint alignment = size > 8 ? 16 : 8;
				slot.value_offset = offset = AlignOffset(offset + size, alignment);
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
						stdsint alignment = instruction.alignment ?
							(stdsint)instruction.alignment : element_size;
						slot.local_offset = AlignOffset(
							offset + element_size * element_count, alignment);
						offset = slot.local_offset;
					}
					else {
						stdsint size = TypeStorageBytes(instruction.result.type, target);
						if (size < 8) size = 8;
						stdsint alignment = size > 8 ? 16 : 8;
						slot.value_offset = offset = AlignOffset(offset + size, alignment);
					}
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

		bool FitsSignedImmediate12(stdsint value) {
			return value >= -2048 && value <= 2047;
		}

		// Materialize an address below s0 without assuming a 12-bit frame offset.
		void EmitFrameAddress(const char* reg, stdsint offset,
			uni::OstreamTrait& output) {
			if (FitsSignedImmediate12(-offset))
				output.OutFormat("\taddi %s, s0, -%[i]\n", reg, offset);
			else {
				output.OutFormat("\tli %s, -%[i]\n", reg, offset);
				output.OutFormat("\tadd %s, s0, %s\n", reg, reg);
			}
		}

		void EmitFrameStore(const char* source, stdsint offset,
			uni::OstreamTrait& output) {
			if (FitsSignedImmediate12(-offset))
				output.OutFormat("\tsd %s, -%[i](s0)\n", source, offset);
			else {
				const char* address = StrCompare(source, "t3") ? "t3" : "t4";
				EmitFrameAddress(address, offset, output);
				output.OutFormat("\tsd %s, 0(%s)\n", source, address);
			}
		}

		void EmitFrameFloatStore(clover::Type type, const char* source,
			stdsint offset, uni::OstreamTrait& output) {
			const char* mnemonic = type == clover::Type::F32 ? "fsw" : "fsd";
			if (FitsSignedImmediate12(-offset))
				output.OutFormat("\t%s %s, -%[i](s0)\n", mnemonic, source, offset);
			else {
				EmitFrameAddress("t3", offset, output);
				output.OutFormat("\t%s %s, 0(t3)\n", mnemonic, source);
			}
		}

		// Load a spilled SSA value, or rematerialize a Local frame address.
		bool LoadValue(const FunctionLayout& layout, clover::Value value,
			const char* reg, uni::OstreamTrait& output) {
			const ValueSlot* slot;
			if (!SlotOf(layout, value, slot)) return false;
			if (slot->local_offset) {
				EmitFrameAddress(reg, slot->local_offset, output);
				return true;
			}
			if (!slot->value_offset) return false;
			if (FitsSignedImmediate12(-slot->value_offset))
				output.OutFormat("\tld %s, -%[i](s0)\n", reg, slot->value_offset);
			else {
				EmitFrameAddress(reg, slot->value_offset, output);
				output.OutFormat("\tld %s, 0(%s)\n", reg, reg);
			}
			return true;
		}

		// Store an instruction result from a temporary register to its spill slot.
		bool StoreResult(const FunctionLayout& layout, const clover::Instruction& instruction,
			const char* reg, uni::OstreamTrait& output) {
			const ValueSlot* slot;
			if (!instruction.has_result || !SlotOf(layout, instruction.result, slot)) return false;
			if (!slot->value_offset) return false;
			EmitFrameStore(reg, slot->value_offset, output);
			return true;
		}

		bool LoadWideValue(const FunctionLayout& layout, clover::Value value,
			const char* low_register, const char* high_register,
			uni::OstreamTrait& output) {
			if (value.type != clover::Type::F128) return false;
			const ValueSlot* slot;
			if (!SlotOf(layout, value, slot) || !slot->value_offset) return false;
			if (FitsSignedImmediate12(-slot->value_offset) &&
				FitsSignedImmediate12(-slot->value_offset + 8)) {
				output.OutFormat("\tld %s, -%[i](s0)\n", low_register,
					slot->value_offset);
				output.OutFormat("\tld %s, -%[i](s0)\n", high_register,
					slot->value_offset - 8);
			}
			else {
				EmitFrameAddress("t3", slot->value_offset, output);
				output.OutFormat("\tld %s, 0(t3)\n\tld %s, 8(t3)\n",
					low_register, high_register);
			}
			return true;
		}

		bool StoreWideResult(const FunctionLayout& layout,
			const clover::Instruction& instruction, const char* low_register,
			const char* high_register, uni::OstreamTrait& output) {
			if (instruction.type != clover::Type::F128 || !instruction.has_result)
				return false;
			const ValueSlot* slot;
			if (!SlotOf(layout, instruction.result, slot) || !slot->value_offset)
				return false;
			if (FitsSignedImmediate12(-slot->value_offset) &&
				FitsSignedImmediate12(-slot->value_offset + 8)) {
				EmitFrameStore(low_register, slot->value_offset, output);
				EmitFrameStore(high_register, slot->value_offset - 8, output);
			}
			else {
				EmitFrameAddress("t3", slot->value_offset, output);
				output.OutFormat("\tsd %s, 0(t3)\n\tsd %s, 8(t3)\n",
					low_register, high_register);
			}
			return true;
		}

		bool LoadFloatValue(const FunctionLayout& layout, clover::Value value,
			const char* reg, uni::OstreamTrait& output) {
			if (!IsFloatType(value.type) || value.type == clover::Type::F128) return false;
			const ValueSlot* slot;
			if (!SlotOf(layout, value, slot) || !slot->value_offset) return false;
			const char* mnemonic = value.type == clover::Type::F32 ? "flw" : "fld";
			if (FitsSignedImmediate12(-slot->value_offset))
				output.OutFormat("\t%s %s, -%[i](s0)\n", mnemonic, reg, slot->value_offset);
			else {
				EmitFrameAddress("t3", slot->value_offset, output);
				output.OutFormat("\t%s %s, 0(t3)\n", mnemonic, reg);
			}
			return true;
		}

		bool LoadFloatBits(const FunctionLayout& layout, clover::Value value,
			const char* reg, uni::OstreamTrait& output) {
			if (!IsFloatType(value.type) || value.type == clover::Type::F128) return false;
			const ValueSlot* slot;
			if (!SlotOf(layout, value, slot) || !slot->value_offset) return false;
			const char* mnemonic = value.type == clover::Type::F32 ? "lw" : "ld";
			if (FitsSignedImmediate12(-slot->value_offset))
				output.OutFormat("\t%s %s, -%[i](s0)\n", mnemonic, reg,
					slot->value_offset);
			else {
				EmitFrameAddress("t3", slot->value_offset, output);
				output.OutFormat("\t%s %s, 0(t3)\n", mnemonic, reg);
			}
			return true;
		}

		bool StoreFloatResult(const FunctionLayout& layout,
			const clover::Instruction& instruction, const char* reg,
			uni::OstreamTrait& output) {
			if (!IsFloatType(instruction.type) || instruction.type == clover::Type::F128 ||
				!instruction.has_result) return false;
			const ValueSlot* slot;
			if (!SlotOf(layout, instruction.result, slot) || !slot->value_offset) return false;
			EmitFrameFloatStore(instruction.type, reg, slot->value_offset, output);
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

		bool EmitFloatBinary(const clover::Instruction& instruction,
			const FunctionLayout& layout, const char* operation,
			uni::OstreamTrait& output) {
			if (!RequireOperands(instruction, 2) ||
				!LoadFloatValue(layout, instruction.operands.begin()[0], "ft0", output) ||
				!LoadFloatValue(layout, instruction.operands.begin()[1], "ft1", output))
				return false;
			output.OutFormat("\t%s.%c ft2, ft0, ft1\n", operation,
				instruction.type == clover::Type::F32 ? 's' : 'd');
			return StoreFloatResult(layout, instruction, "ft2", output);
		}

		bool EmitWideBinary(const clover::Instruction& instruction,
			const FunctionLayout& layout, const char* helper,
			uni::OstreamTrait& output) {
			if (!RequireOperands(instruction, 2) ||
				!LoadWideValue(layout, instruction.operands.begin()[0], "a0", "a1", output) ||
				!LoadWideValue(layout, instruction.operands.begin()[1], "a2", "a3", output))
				return false;
			output.OutFormat("\tcall %s\n", helper);
			return StoreWideResult(layout, instruction, "a0", "a1", output);
		}

		// Assemble a little-endian unaligned integer from byte loads.
		bool EmitUnalignedLoad(const clover::Instruction& instruction,
			const FunctionLayout& layout, const RISCVTargetInfo& target,
			uni::OstreamTrait& output) {
			stdsint bytes = TypeStorageBytes(instruction.type, target);
			if (bytes != 2 && bytes != 4 && bytes != 8) return false;
			output.OutFormat("\tlbu t1, 0(t0)\n");
			for (stdsint index = 1; index < bytes; index++) {
				output.OutFormat("\tlbu t2, %[i](t0)\n", index);
				output.OutFormat("\tslli t2, t2, %[i]\n", index * 8);
				output.OutFormat("\tor t1, t1, t2\n");
			}
			if (instruction.type == clover::Type::I16)
				output.OutFormat("\tslli t1, t1, 48\n\tsrai t1, t1, 48\n");
			else if (instruction.type == clover::Type::I32)
				output.OutFormat("\taddiw t1, t1, 0\n");
			return StoreResult(layout, instruction, "t1", output);
		}

		// Split a little-endian unaligned integer store into byte stores.
		bool EmitUnalignedStore(const clover::Instruction& instruction,
			const RISCVTargetInfo& target, uni::OstreamTrait& output) {
			stdsint bytes = TypeStorageBytes(instruction.type, target);
			if (bytes != 2 && bytes != 4 && bytes != 8) return false;
			output.OutFormat("\tsb t0, 0(t1)\n");
			if (bytes > 1) output.OutFormat("\tsrli t2, t0, 8\n");
			for (stdsint index = 1; index < bytes; index++) {
				output.OutFormat("\tsb t2, %[i](t1)\n", index);
				if (index + 1 < bytes) output.OutFormat("\tsrli t2, t2, 8\n");
			}
			return true;
		}

		bool EmitUnalignedFloatLoad(const clover::Instruction& instruction,
			const FunctionLayout& layout, const RISCVTargetInfo& target,
			uni::OstreamTrait& output) {
			stdsint bytes = TypeStorageBytes(instruction.type, target);
			if (bytes != 4 && bytes != 8) return false;
			output.OutFormat("\tlbu t1, 0(t0)\n");
			for (stdsint index = 1; index < bytes; index++) {
				output.OutFormat("\tlbu t2, %[i](t0)\n", index);
				output.OutFormat("\tslli t2, t2, %[i]\n", index * 8);
				output.OutFormat("\tor t1, t1, t2\n");
			}
			output.OutFormat(instruction.type == clover::Type::F32 ?
				"\tfmv.w.x ft0, t1\n" : "\tfmv.d.x ft0, t1\n");
			return StoreFloatResult(layout, instruction, "ft0", output);
		}

		bool EmitUnalignedFloatStore(const clover::Instruction& instruction,
			const RISCVTargetInfo& target, uni::OstreamTrait& output) {
			stdsint bytes = TypeStorageBytes(instruction.type, target);
			if (bytes != 4 && bytes != 8) return false;
			output.OutFormat(instruction.type == clover::Type::F32 ?
				"\tfmv.x.w t1, ft0\n" : "\tfmv.x.d t1, ft0\n");
			output.OutFormat("\tsb t1, 0(t0)\n");
			if (bytes > 1) output.OutFormat("\tsrli t2, t1, 8\n");
			for (stdsint index = 1; index < bytes; index++) {
				output.OutFormat("\tsb t2, %[i](t0)\n", index);
				if (index + 1 < bytes) output.OutFormat("\tsrli t2, t2, 8\n");
			}
			return true;
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
			case 16: output.OutFormat("\tslli t1, t1, 4\n"); break;
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
			const clover::Function& function, const clover::Module& module,
			const RISCVTargetInfo& target,
			uni::OstreamTrait& output, RISCVAssemblyOutputInterface* assembly_output,
			uint32& current_file, uint32& current_line, unsigned& private_label) {
			if (assembly_output && instruction.debug_file && instruction.debug_line &&
				(instruction.debug_file != current_file || instruction.debug_line != current_line)) {
				if (!assembly_output->EmitLocation(instruction.debug_file,
					instruction.debug_line, output)) return false;
				current_file = instruction.debug_file;
				current_line = instruction.debug_line;
			}
			switch (instruction.opcode) {
			case clover::Opcode::Const:
				output.OutFormat("\tli t0, %[i]\n", instruction.immediate);
				return StoreResult(layout, instruction, "t0", output);
			case clover::Opcode::FConst:
				output.OutFormat("\tli t0, %[i]\n", instruction.immediate);
				if (instruction.type == clover::Type::F128) {
					output.OutFormat("\tli t1, %[i]\n", instruction.immediate_high);
					return StoreWideResult(layout, instruction, "t0", "t1", output);
				}
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
				if (instruction.type == clover::Type::F128) {
					if (!instruction.alignment || instruction.alignment >= 8)
						output.OutFormat("\tld t1, 0(t0)\n\tld t2, 8(t0)\n");
					else {
						for (unsigned half = 0; half < 2; half++) {
							const char* result = half ? "t2" : "t1";
							output.OutFormat("\tlbu %s, %[u](t0)\n", result, half * 8);
							for (unsigned byte = 1; byte < 8; byte++) {
								output.OutFormat("\tlbu t4, %[u](t0)\n", half * 8 + byte);
								output.OutFormat("\tslli t4, t4, %[u]\n", byte * 8);
								output.OutFormat("\tor %s, %s, t4\n", result, result);
							}
						}
					}
					return StoreWideResult(layout, instruction, "t1", "t2", output);
				}
				if (IsFloatType(instruction.type)) {
					if (instruction.alignment &&
						(stdsint)instruction.alignment < TypeStorageBytes(instruction.type, target))
						return EmitUnalignedFloatLoad(instruction, layout, target, output);
					output.OutFormat(instruction.type == clover::Type::F32 ?
						"\tflw ft0, 0(t0)\n" : "\tfld ft0, 0(t0)\n");
					return StoreFloatResult(layout, instruction, "ft0", output);
				}
				if (instruction.alignment &&
					(stdsint)instruction.alignment < TypeStorageBytes(instruction.type, target))
					return EmitUnalignedLoad(instruction, layout, target, output);
				switch (instruction.type) {
				case clover::Type::I1:
				case clover::Type::I8: output.OutFormat("\tlb t1, 0(t0)\n"); break;
				case clover::Type::I16: output.OutFormat("\tlh t1, 0(t0)\n"); break;
				case clover::Type::I32: output.OutFormat("\tlw t1, 0(t0)\n"); break;
				case clover::Type::I64:
				case clover::Type::Ptr: output.OutFormat("\tld t1, 0(t0)\n"); break;
				default: break;
				}
				return StoreResult(layout, instruction, "t1", output);

			case clover::Opcode::Store:
				if (!RequireOperands(instruction, 2)) return false;
				if (instruction.type == clover::Type::F128) {
					if (!LoadWideValue(layout, instruction.operands.begin()[0],
						"t1", "t2", output) ||
						!LoadValue(layout, instruction.operands.begin()[1], "t0", output))
						return false;
					if (!instruction.alignment || instruction.alignment >= 8)
						output.OutFormat("\tsd t1, 0(t0)\n\tsd t2, 8(t0)\n");
					else {
						for (unsigned half = 0; half < 2; half++) {
							const char* source = half ? "t2" : "t1";
							output.OutFormat("\tsb %s, %[u](t0)\n", source, half * 8);
							output.OutFormat("\tmv t4, %s\n", source);
							for (unsigned byte = 1; byte < 8; byte++) {
								output.OutFormat("\tsrli t4, t4, 8\n");
								output.OutFormat("\tsb t4, %[u](t0)\n", half * 8 + byte);
							}
						}
					}
					return true;
				}
				if (IsFloatType(instruction.type)) {
					if (!LoadFloatValue(layout, instruction.operands.begin()[0], "ft0", output) ||
						!LoadValue(layout, instruction.operands.begin()[1], "t0", output)) return false;
					if (instruction.alignment &&
						(stdsint)instruction.alignment < TypeStorageBytes(instruction.type, target))
						return EmitUnalignedFloatStore(instruction, target, output);
					output.OutFormat(instruction.type == clover::Type::F32 ?
						"\tfsw ft0, 0(t0)\n" : "\tfsd ft0, 0(t0)\n");
					return true;
				}
				if (!LoadValue(layout, instruction.operands.begin()[0], "t0", output) ||
					!LoadValue(layout, instruction.operands.begin()[1], "t1", output)) return false;
				if (instruction.alignment &&
					(stdsint)instruction.alignment < TypeStorageBytes(instruction.type, target))
					return EmitUnalignedStore(instruction, target, output);
				switch (instruction.type) {
				case clover::Type::I1:
				case clover::Type::I8: output.OutFormat("\tsb t0, 0(t1)\n"); break;
				case clover::Type::I16: output.OutFormat("\tsh t0, 0(t1)\n"); break;
				case clover::Type::I32: output.OutFormat("\tsw t0, 0(t1)\n"); break;
				case clover::Type::I64:
				case clover::Type::Ptr: output.OutFormat("\tsd t0, 0(t1)\n"); break;
				default: break;
				}
				return true;

			case clover::Opcode::MemCopy:
				if (!RequireOperands(instruction, 2) || instruction.immediate <= 0 ||
					!LoadValue(layout, instruction.operands.begin()[0], "t0", output) ||
					!LoadValue(layout, instruction.operands.begin()[1], "t1", output))
					return false;
				private_label++;
				output.OutFormat("\tli t3, %[i]\n", instruction.immediate);
				output.OutFormat(".Lmemcopy_loop_%[u]:\n", private_label);
				output.OutFormat("\tbeqz t3, .Lmemcopy_end_%[u]\n", private_label);
				output.OutFormat("\tlbu t2, 0(t1)\n\tsb t2, 0(t0)\n");
				output.OutFormat("\taddi t0, t0, 1\n\taddi t1, t1, 1\n");
				output.OutFormat("\taddi t3, t3, -1\n\tj .Lmemcopy_loop_%[u]\n",
					private_label);
				output.OutFormat(".Lmemcopy_end_%[u]:\n", private_label);
				return true;

			case clover::Opcode::Mov:
				if (instruction.type == clover::Type::F128) {
					if (!RequireOperands(instruction, 1) ||
						!LoadWideValue(layout, instruction.operands.begin()[0],
							"t0", "t1", output)) return false;
					return StoreWideResult(layout, instruction, "t0", "t1", output);
				}
				if (IsFloatType(instruction.type)) {
					if (!RequireOperands(instruction, 1) ||
						!LoadFloatValue(layout, instruction.operands.begin()[0], "ft0", output))
						return false;
					return StoreFloatResult(layout, instruction, "ft0", output);
				}
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
			case clover::Opcode::FAdd:
				if (instruction.type == clover::Type::F128)
					return EmitWideBinary(instruction, layout, "__addtf3", output);
				return EmitFloatBinary(instruction, layout, "fadd", output);
			case clover::Opcode::FSub:
				if (instruction.type == clover::Type::F128)
					return EmitWideBinary(instruction, layout, "__subtf3", output);
				return EmitFloatBinary(instruction, layout, "fsub", output);
			case clover::Opcode::FMul:
				if (instruction.type == clover::Type::F128)
					return EmitWideBinary(instruction, layout, "__multf3", output);
				return EmitFloatBinary(instruction, layout, "fmul", output);
			case clover::Opcode::FDiv:
				if (instruction.type == clover::Type::F128)
					return EmitWideBinary(instruction, layout, "__divtf3", output);
				return EmitFloatBinary(instruction, layout, "fdiv", output);
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

			case clover::Opcode::FCmp:
				if (instruction.type == clover::Type::F128) {
					if (!RequireOperands(instruction, 2) ||
						!LoadWideValue(layout, instruction.operands.begin()[0],
							"a0", "a1", output) ||
						!LoadWideValue(layout, instruction.operands.begin()[1],
							"a2", "a3", output)) return false;
					const char* helper = instruction.float_predicate == clover::FCmpPredicate::EQ ?
						"__eqtf2" : instruction.float_predicate == clover::FCmpPredicate::NE ?
						"__netf2" : instruction.float_predicate == clover::FCmpPredicate::LT ?
						"__lttf2" : instruction.float_predicate == clover::FCmpPredicate::LE ?
						"__letf2" : instruction.float_predicate == clover::FCmpPredicate::GT ?
						"__gttf2" : "__getf2";
					output.OutFormat("\tcall %s\n", helper);
					switch (instruction.float_predicate) {
					case clover::FCmpPredicate::EQ: output.OutFormat("\tseqz t2, a0\n"); break;
					case clover::FCmpPredicate::NE: output.OutFormat("\tsnez t2, a0\n"); break;
					case clover::FCmpPredicate::LT: output.OutFormat("\tsltz t2, a0\n"); break;
					case clover::FCmpPredicate::LE:
						output.OutFormat("\tsgtz t2, a0\n\txori t2, t2, 1\n"); break;
					case clover::FCmpPredicate::GT: output.OutFormat("\tsgtz t2, a0\n"); break;
					case clover::FCmpPredicate::GE:
						output.OutFormat("\tsltz t2, a0\n\txori t2, t2, 1\n"); break;
					}
					return StoreResult(layout, instruction, "t2", output);
				}
				if (!RequireOperands(instruction, 2) ||
					!LoadFloatValue(layout, instruction.operands.begin()[0], "ft0", output) ||
					!LoadFloatValue(layout, instruction.operands.begin()[1], "ft1", output))
					return false;
				switch (instruction.float_predicate) {
				case clover::FCmpPredicate::EQ:
					output.OutFormat("\tfeq.%c t2, ft0, ft1\n",
						instruction.type == clover::Type::F32 ? 's' : 'd');
					break;
				case clover::FCmpPredicate::NE:
					output.OutFormat("\tfeq.%c t2, ft0, ft1\n\txori t2, t2, 1\n",
						instruction.type == clover::Type::F32 ? 's' : 'd');
					break;
				case clover::FCmpPredicate::LT:
					output.OutFormat("\tflt.%c t2, ft0, ft1\n",
						instruction.type == clover::Type::F32 ? 's' : 'd');
					break;
				case clover::FCmpPredicate::LE:
					output.OutFormat("\tfle.%c t2, ft0, ft1\n",
						instruction.type == clover::Type::F32 ? 's' : 'd');
					break;
				case clover::FCmpPredicate::GT:
					output.OutFormat("\tflt.%c t2, ft1, ft0\n",
						instruction.type == clover::Type::F32 ? 's' : 'd');
					break;
				case clover::FCmpPredicate::GE:
					output.OutFormat("\tfle.%c t2, ft1, ft0\n",
						instruction.type == clover::Type::F32 ? 's' : 'd');
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

			case clover::Opcode::IntToPtr:
				if (!RequireOperands(instruction, 1) ||
					instruction.type != clover::Type::Ptr ||
					!LoadValue(layout, instruction.operands.begin()[0], "t0", output))
					return false;
				return StoreResult(layout, instruction, "t0", output);

			case clover::Opcode::PtrToInt:
				if (!RequireOperands(instruction, 1) ||
					instruction.operands.begin()[0].type != clover::Type::Ptr ||
					!LoadValue(layout, instruction.operands.begin()[0], "t0", output))
					return false;
				switch (TypeBits(instruction.type, target)) {
				case 1: output.OutFormat("\tandi t0, t0, 1\n"); break;
				case 8: output.OutFormat("\tandi t0, t0, 255\n"); break;
				case 16: output.OutFormat("\tslli t0, t0, 48\n\tsrli t0, t0, 48\n"); break;
				case 32: output.OutFormat("\tslli t0, t0, 32\n\tsrli t0, t0, 32\n"); break;
				}
				return StoreResult(layout, instruction, "t0", output);

			case clover::Opcode::SIToFP:
			case clover::Opcode::UIToFP: {
				if (!RequireOperands(instruction, 1) ||
					!LoadValue(layout, instruction.operands.begin()[0], "t0", output)) return false;
				bool wide = instruction.operands.begin()[0].type == clover::Type::I64;
				if (instruction.type == clover::Type::F128) {
					const char* helper = instruction.opcode == clover::Opcode::SIToFP ?
						(wide ? "__floatditf" : "__floatsitf") :
						(wide ? "__floatunditf" : "__floatunsitf");
					output.OutFormat("\tmv a0, t0\n\tcall %s\n", helper);
					return StoreWideResult(layout, instruction, "a0", "a1", output);
				}
				output.OutFormat("\tfcvt.%c.%s ft0, t0\n",
					instruction.type == clover::Type::F32 ? 's' : 'd',
					wide ? instruction.opcode == clover::Opcode::SIToFP ? "l" : "lu" :
						instruction.opcode == clover::Opcode::SIToFP ? "w" : "wu");
				return StoreFloatResult(layout, instruction, "ft0", output);
			}

			case clover::Opcode::FPToSI:
			case clover::Opcode::FPToUI: {
				bool wide = instruction.type == clover::Type::I64;
				if (!RequireOperands(instruction, 1)) return false;
				if (instruction.operands.begin()[0].type == clover::Type::F128) {
					if (!LoadWideValue(layout, instruction.operands.begin()[0],
						"a0", "a1", output)) return false;
					const char* helper = instruction.opcode == clover::Opcode::FPToSI ?
						(wide ? "__fixtfdi" : "__fixtfsi") :
						(wide ? "__fixunstfdi" : "__fixunstfsi");
					output.OutFormat("\tcall %s\n", helper);
					return StoreResult(layout, instruction, "a0", output);
				}
				if (!LoadFloatValue(layout, instruction.operands.begin()[0], "ft0", output))
					return false;
				output.OutFormat("\tfcvt.%s.%c t0, ft0, rtz\n",
					wide ? instruction.opcode == clover::Opcode::FPToSI ? "l" : "lu" :
						instruction.opcode == clover::Opcode::FPToSI ? "w" : "wu",
					instruction.operands.begin()[0].type == clover::Type::F32 ? 's' : 'd');
				return StoreResult(layout, instruction, "t0", output);
			}

			case clover::Opcode::FPExt:
			case clover::Opcode::FPTrunc: {
				if (!RequireOperands(instruction, 1)) return false;
				clover::Type source_type = instruction.operands.begin()[0].type;
				if (instruction.type == clover::Type::F128) {
					if (TypeBits(source_type, target) <= ABIFloatBits(target)) {
						if (!LoadFloatValue(layout, instruction.operands.begin()[0], "fa0", output))
							return false;
					}
					else if (!LoadFloatBits(layout, instruction.operands.begin()[0], "a0", output))
						return false;
					output.OutFormat("\tcall %s\n", source_type == clover::Type::F32 ?
						"__extendsftf2" : "__extenddftf2");
					return StoreWideResult(layout, instruction, "a0", "a1", output);
				}
				if (source_type == clover::Type::F128) {
					if (!LoadWideValue(layout, instruction.operands.begin()[0],
						"a0", "a1", output)) return false;
					output.OutFormat("\tcall %s\n", instruction.type == clover::Type::F32 ?
						"__trunctfsf2" : "__trunctfdf2");
					if (TypeBits(instruction.type, target) <= ABIFloatBits(target))
						return StoreFloatResult(layout, instruction, "fa0", output);
					output.OutFormat(instruction.type == clover::Type::F32 ?
						"\tfmv.w.x ft0, a0\n" : "\tfmv.d.x ft0, a0\n");
					return StoreFloatResult(layout, instruction, "ft0", output);
				}
				if (!LoadFloatValue(layout, instruction.operands.begin()[0], "ft0", output))
					return false;
				output.OutFormat(instruction.opcode == clover::Opcode::FPExt ?
					"\tfcvt.d.s ft1, ft0\n" : "\tfcvt.s.d ft1, ft0\n");
				return StoreFloatResult(layout, instruction, "ft1", output);
			}

			case clover::Opcode::GlobalAddr:
				if (!instruction.name.getByteCount()) return false;
				output.OutFormat("\tla t0, %s\n", instruction.name.reference());
				return StoreResult(layout, instruction, "t0", output);

			case clover::Opcode::Call: {
				unsigned integer_count = 0;
				unsigned float_count = 0;
				const clover::Function* callee = FindFunction(module, instruction.callee);
				if (!callee) return false;
				for (stduint index = 0; index < instruction.operands.Count(); index++) {
					clover::Value argument = instruction.operands.begin()[index];
					bool use_float_register;
					unsigned register_index;
					if (callee->is_variadic && index >= callee->parameters.Count()) {
						if (argument.type == clover::Type::F128) {
							integer_count = (integer_count + 1) & ~1U;
							if (integer_count + 1 >= numsof(ArgumentRegisters)) return false;
							use_float_register = false;
							register_index = integer_count;
							integer_count += 2;
						}
						else {
							if (integer_count >= numsof(ArgumentRegisters)) return false;
							use_float_register = false;
							register_index = integer_count++;
						}
					}
					else if (!AllocateArgumentRegister(argument.type, target,
						integer_count, float_count, use_float_register,
						register_index)) return false;
					if (use_float_register) {
						if (!LoadFloatValue(layout, argument,
							FloatArgumentRegisters[register_index], output)) return false;
					}
					else {
						const char* reg = ArgumentRegisters[register_index];
						if (argument.type == clover::Type::F128) {
							if (!LoadWideValue(layout, argument, reg,
								ArgumentRegisters[register_index + 1], output)) return false;
						}
						else if (IsFloatType(argument.type)) {
							if (!LoadFloatBits(layout, argument, reg, output)) return false;
						}
						else {
							if (!LoadValue(layout, argument, reg, output)) return false;
							if (argument.type == clover::Type::I32)
								output.OutFormat("\taddiw %s, %s, 0\n", reg, reg);
						}
					}
				}
				output.OutFormat("\tcall %s\n", instruction.callee.reference());
				if (instruction.type == clover::Type::Void) return true;
				if (IsFloatType(instruction.type)) {
					if (instruction.type == clover::Type::F128)
						return StoreWideResult(layout, instruction, "a0", "a1", output);
					if (TypeBits(instruction.type, target) <= ABIFloatBits(target))
						return StoreFloatResult(layout, instruction, "fa0", output);
					return StoreResult(layout, instruction, "a0", output);
				}
				if (instruction.type == clover::Type::I32)
					output.OutFormat("\taddiw a0, a0, 0\n");
				return StoreResult(layout, instruction, "a0", output);
			}

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
				if (!RequireOperands(instruction, 1)) return false;
				if (IsFloatType(instruction.type)) {
					if (instruction.type == clover::Type::F128) {
						if (!LoadWideValue(layout, instruction.operands.begin()[0],
							"a0", "a1", output)) return false;
					}
					else if (TypeBits(instruction.type, target) <= ABIFloatBits(target)) {
						if (!LoadFloatValue(layout, instruction.operands.begin()[0],
							"fa0", output)) return false;
					}
					else if (!LoadFloatBits(layout, instruction.operands.begin()[0],
						"a0", output)) return false;
				}
				else if (!LoadValue(layout, instruction.operands.begin()[0], "a0", output))
					return false;
				EmitEpilogue(output);
				return true;
			}
			return false;
		}

		// Emit one function with a conservative stack slot for every SSA result.
		bool EmitFunction(const clover::Function& function, const RISCVTargetInfo& target,
			const clover::Module& module, uni::OstreamTrait& output,
			RISCVAssemblyOutputInterface* assembly_output) {
			FunctionLayout layout;
			if (!BuildLayout(function, target, layout)) return false;

			output.OutFormat("\n");
			if (function.linkage == clover::Linkage::External)
				output.OutFormat(".globl %s\n", function.name.reference());
			output.OutFormat(".type %s, @function\n", function.name.reference());
			output.OutFormat("%s:\n", function.name.reference());
			if (layout.frame_size <= 2047) {
				output.OutFormat("\taddi sp, sp, -%[i]\n", layout.frame_size);
				output.OutFormat("\tsd ra, %[i](sp)\n", layout.frame_size - 8);
				output.OutFormat("\tsd s0, %[i](sp)\n", layout.frame_size - 16);
				output.OutFormat("\taddi s0, sp, %[i]\n", layout.frame_size);
			}
			else {
				// Preserve the caller frame pointer before constructing a large frame.
				output.OutFormat("\tmv t1, s0\n");
				output.OutFormat("\tli t0, %[i]\n", layout.frame_size);
				output.OutFormat("\tsub sp, sp, t0\n");
				output.OutFormat("\tadd s0, sp, t0\n");
				output.OutFormat("\tsd ra, -8(s0)\n");
				output.OutFormat("\tsd t1, -16(s0)\n");
			}
			unsigned integer_count = 0;
			unsigned float_count = 0;
			for (stduint index = 0; index < function.parameters.Count(); index++) {
				const clover::Parameter& parameter = function.parameters.begin()[index];
				const ValueSlot* slot;
				if (!SlotOf(layout, parameter.value, slot) || !slot->value_offset) return false;
				bool use_float_register;
				unsigned register_index;
				if (!AllocateArgumentRegister(parameter.value.type, target,
					integer_count, float_count, use_float_register,
					register_index)) return false;
				if (use_float_register)
					EmitFrameFloatStore(parameter.value.type,
						FloatArgumentRegisters[register_index], slot->value_offset, output);
				else {
					const char* reg = ArgumentRegisters[register_index];
					if (parameter.value.type == clover::Type::F128) {
						EmitFrameStore(reg, slot->value_offset, output);
						EmitFrameStore(ArgumentRegisters[register_index + 1],
							slot->value_offset - 8, output);
						continue;
					}
					if (!IsFloatType(parameter.value.type) &&
						parameter.value.type == clover::Type::I32)
						output.OutFormat("\taddiw %s, %s, 0\n", reg, reg);
					EmitFrameStore(reg, slot->value_offset, output);
				}
			}

			uint32 current_file = 0;
			uint32 current_line = 0;
			unsigned private_label = 0;
			for (const clover::BasicBlock& block : function.blocks) {
				output.OutFormat(".L%s_%s:\n", function.name.reference(), block.name.reference());
				for (const clover::Instruction& instruction : block.instructions)
					if (!EmitInstruction(instruction, layout, function, module, target, output,
						assembly_output, current_file, current_line, private_label)) return false;
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
				if (function.return_type == clover::Type::F128)
					output.OutFormat("\tli a0, 0\n\tli a1, 0\n");
				else if (IsFloatType(function.return_type) &&
					TypeBits(function.return_type, target) <= ABIFloatBits(target))
					output.OutFormat(function.return_type == clover::Type::F32 ?
						"\tfmv.w.x fa0, zero\n" : "\tfmv.d.x fa0, zero\n");
				else output.OutFormat("\tli a0, 0\n");
				EmitEpilogue(output);
			}
			output.OutFormat(".size %s, .-%s\n", function.name.reference(), function.name.reference());
			return true;
		}

	}

	bool BackendRISCV::Generate(const clover::Module& module, uni::OstreamTrait& output) {
		if (!ValidateTarget(target, module)) return false;
		RISCVAssemblyOutputGCC gcc_output;
		char instruction_set[8] = {'i', '\0'};
		unsigned instruction_length = 1;
		if (target.extension_m) instruction_set[instruction_length++] = 'm';
		if (target.extension_f) instruction_set[instruction_length++] = 'f';
		if (target.extension_d) instruction_set[instruction_length++] = 'd';
		instruction_set[instruction_length] = '\0';
		const char* abi = target.abi == RISCVABI::LP64D ? "lp64d" :
			target.abi == RISCVABI::LP64F ? "lp64f" : "lp64";
		RISCVAssemblyOutputAASM aasm_output(target.bits, instruction_set, abi, output);
		RISCVAssemblyOutputInterface* assembly_output = nullptr;
		uni::OstreamTrait* assembly_stream = &output;
		switch (options.assembly_syntax) {
		case RISCVAssemblySyntax::GCC: assembly_output = &gcc_output; break;
		case RISCVAssemblySyntax::AASM:
			assembly_output = &aasm_output;
			assembly_stream = &aasm_output;
			if (!aasm_output.Begin()) return false;
			break;
		}
		if (!assembly_output) return false;
		if (options.debug_symbols &&
			!assembly_output->EmitSourceFiles(module, *assembly_stream))
			return false;
		for (const clover::Global& global : module.globals) {
			if (global.is_declaration) continue;
			stdsint element_size = TypeStorageBytes(global.type, target);
			stdsint object_size = element_size * global.element_count;
			if (!element_size || object_size <= 0) return false;
			bool has_initializer = global.initializer != 0 ||
				global.initializer_high != 0 ||
				global.initializer_values.Count() != 0 ||
				global.initializer_fragments.Count() != 0;
			assembly_stream->OutFormat(global.is_read_only ? ".section .rodata\n" :
				has_initializer ? ".data\n" : ".bss\n");
			if (global.linkage == clover::Linkage::External)
				assembly_stream->OutFormat(".globl %s\n", global.name.reference());
			assembly_stream->OutFormat(".type %s, @object\n.size %s, %[i]\n",
				global.name.reference(), global.name.reference(), object_size);
			assembly_stream->OutFormat(".balign %[u]\n%s:\n",
				global.alignment ? global.alignment : (stduint)element_size,
				global.name.reference());
			if (global.initializer_fragments.Count()) {
				stdsint offset = 0;
				for (const clover::GlobalInitializerFragment& fragment :
					global.initializer_fragments) {
					if ((stdsint)fragment.offset > offset)
						assembly_stream->OutFormat("\t.zero %[i]\n",
							(stdsint)fragment.offset - offset);
					stdsint fragment_size = TypeStorageBytes(fragment.type, target);
					switch (fragment.type) {
					case clover::Type::I1:
					case clover::Type::I8:
						assembly_stream->OutFormat("\t.byte %[i]\n", fragment.initializer);
						break;
					case clover::Type::I16:
						assembly_stream->OutFormat("\t.half %[i]\n", fragment.initializer);
						break;
					case clover::Type::I32:
					case clover::Type::F32:
						assembly_stream->OutFormat("\t.word %[i]\n", fragment.initializer);
						break;
					case clover::Type::I64:
					case clover::Type::F64:
					case clover::Type::Ptr:
						assembly_stream->OutFormat("\t.quad %[i]\n", fragment.initializer);
						break;
					case clover::Type::F128:
						assembly_stream->OutFormat("\t.quad %[i]\n\t.quad %[i]\n",
							fragment.initializer, fragment.initializer_high);
						break;
					default: return false;
					}
					offset = (stdsint)fragment.offset + fragment_size;
				}
				if (offset < object_size)
					assembly_stream->OutFormat("\t.zero %[i]\n", object_size - offset);
			}
			else if (global.initializer_values.Count()) {
				for (stduint index = 0; index < global.initializer_values.Count(); index++) {
					stdsint value = global.initializer_values.begin()[index];
					switch (element_size) {
					case 1: assembly_stream->OutFormat("\t.byte %[i]\n", value); break;
					case 2: assembly_stream->OutFormat("\t.half %[i]\n", value); break;
					case 4: assembly_stream->OutFormat("\t.word %[i]\n", value); break;
					case 8: assembly_stream->OutFormat("\t.quad %[i]\n", value); break;
					default: return false;
					}
				}
			}
			else if (!global.initializer && !global.initializer_high)
				assembly_stream->OutFormat("\t.zero %[i]\n", object_size);
			else if (global.element_count != 1) return false;
			else {
				switch (element_size) {
				case 1: assembly_stream->OutFormat("\t.byte %[i]\n", global.initializer); break;
				case 2: assembly_stream->OutFormat("\t.half %[i]\n", global.initializer); break;
				case 4: assembly_stream->OutFormat("\t.word %[i]\n", global.initializer); break;
				case 8: assembly_stream->OutFormat("\t.quad %[i]\n", global.initializer); break;
				case 16:
					assembly_stream->OutFormat("\t.quad %[i]\n\t.quad %[i]\n",
						global.initializer, global.initializer_high);
					break;
				default: return false;
				}
			}
		}
		assembly_stream->OutFormat(".text\n");
		for (const clover::Function& function : module.functions)
			if (!EmitFunction(function, target, module, *assembly_stream,
				options.debug_symbols ? assembly_output : nullptr)) return false;
		return options.assembly_syntax != RISCVAssemblySyntax::AASM ||
			aasm_output.Flush();
	}

}
