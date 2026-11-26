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
#include "../../../../inc/cpp/Lango/Backend/RISCV/RISCV-ABI.hpp"
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
		stdsint TypeBits(const clover::Module& module,
			const clover::TypeReference& type, const RISCVTargetInfo& target);
		stdsint TypeStorageBytes(const clover::Module& module,
			const clover::TypeReference& type, const RISCVTargetInfo& target);
		stdsint TypeStorageAlignmentBytes(const clover::Module& module,
			const clover::TypeReference& type, const RISCVTargetInfo& target);
		stdsint TypeArrayElements(const clover::TypeReference& type);

		bool IsFloatType(clover::Type type) {
			return type == clover::Type::F32 || type == clover::Type::F64 ||
				type == clover::Type::F128;
		}

		bool IsRV32IntegerType(clover::Type type) {
			return type == clover::Type::I1 || type == clover::Type::I8 ||
				type == clover::Type::I16 || type == clover::Type::I32 ||
				type == clover::Type::Ptr;
		}

		bool ValidateRV32Subset(const clover::Module& module) {
			for (const clover::Function& function : module.functions) {
				if (!IsRV32IntegerType(function.return_type) &&
					function.return_type != clover::Type::Void) {
					plogerro("RISC-V RV32 backend: unsupported function return type in '%s'",
						function.name.reference());
					return false;
				}
				if (function.returns_by_value) {
					plogerro("RISC-V RV32 backend: by-value aggregate returns are not supported yet");
					return false;
				}
				for (const clover::Parameter& parameter : function.parameters) {
					if (parameter.by_value || !IsRV32IntegerType(parameter.value.type)) {
						plogerro("RISC-V RV32 backend: unsupported parameter type in '%s'",
							function.name.reference());
						return false;
					}
				}
				for (const clover::BasicBlock& block : function.blocks) {
					for (const clover::Instruction& instruction : block.instructions) {
						if (instruction.has_result &&
							!IsRV32IntegerType(instruction.result.type)) {
							plogerro("RISC-V RV32 backend: unsupported result type in '%s'",
								function.name.reference());
							return false;
						}
						if ((instruction.opcode == clover::Opcode::Load ||
							instruction.opcode == clover::Opcode::Store ||
							instruction.opcode == clover::Opcode::Const ||
							instruction.opcode == clover::Opcode::Call ||
							instruction.opcode == clover::Opcode::Ret) &&
							instruction.type != clover::Type::Void &&
							!IsRV32IntegerType(instruction.type)) {
							plogerro("RISC-V RV32 backend: unsupported instruction type in '%s'",
								function.name.reference());
							return false;
						}
						if (instruction.by_value_return ||
							(instruction.has_object_type &&
							(!instruction.object_type.IsScalar() ||
							 !IsRV32IntegerType(instruction.object_type.scalar)))) {
							plogerro("RISC-V RV32 backend: aggregate and non-integer objects are not supported yet");
							return false;
						}
						for (const clover::Value& operand : instruction.operands) {
							if (!IsRV32IntegerType(operand.type)) {
								plogerro("RISC-V RV32 backend: unsupported operand type in '%s'",
									function.name.reference());
								return false;
							}
						}
						for (const clover::CallArgument& argument : instruction.call_arguments) {
							if (argument.by_value || !IsRV32IntegerType(argument.value.type)) {
								plogerro("RISC-V RV32 backend: unsupported call argument type in '%s'",
									function.name.reference());
								return false;
							}
						}
					}
				}
			}
			return true;
		}

		const char* RISCVABIName(RISCVABI abi) {
			switch (abi) {
			case RISCVABI::ILP32: return "ilp32";
			case RISCVABI::ILP32F: return "ilp32f";
			case RISCVABI::ILP32D: return "ilp32d";
			case RISCVABI::LP64: return "lp64";
			case RISCVABI::LP64F: return "lp64f";
			case RISCVABI::LP64D: return "lp64d";
			default: return nullptr;
			}
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

		bool ValidateRegisterArguments(const clover::Function& function,
			const clover::Module& module, const RISCVTargetInfo& target) {
			RISCVABIArgumentState state;
			if (function.returns_by_value) {
				stdsint size = TypeStorageBytes(module, function.return_object_type, target);
				if (size <= 0) return false;
				if (size > (stdsint)(target.bits / 4)) {
					RISCVABIArgumentLocation location;
					if (!RISCVAllocateArgument(clover::Type::Ptr, target, state,
						false, location)) return false;
				}
			}
			for (const clover::Parameter& parameter : function.parameters) {
				RISCVABIArgumentLocation location;
				if (parameter.by_value) {
					if (!RISCVAllocateAggregateArgument(module, parameter.object_type,
						target, state, false, location)) return false;
					continue;
				}
				if (parameter.aggregate_group && IsFloatType(parameter.value.type) &&
					TypeBits(parameter.value.type, target) > ABIFloatBits(target)) {
					plogerro("RISC-V backend: flattened aggregate floating member exceeds ABI FLEN");
					return false;
				}
				if (!RISCVAllocateArgument(parameter.value.type, target, state,
					false, location)) {
					plogerro("RISC-V backend: invalid function argument type");
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

		bool IsSymbolLocallyBound(const clover::Module& module,
			const clover::Global& symbol) {
			if (symbol.linkage == clover::Linkage::Internal ||
				module.relocation_model == clover::RelocationModel::Static) return true;
			if (symbol.is_declaration || symbol.binding == clover::SymbolBinding::Weak)
				return false;
			if (module.relocation_model == clover::RelocationModel::PIE) return true;
			return symbol.visibility != clover::SymbolVisibility::Default;
		}

		bool IsSymbolLocallyBound(const clover::Module& module,
			const clover::Function& symbol) {
			if (symbol.linkage == clover::Linkage::Internal ||
				module.relocation_model == clover::RelocationModel::Static) return true;
			if (symbol.is_declaration || symbol.binding == clover::SymbolBinding::Weak)
				return false;
			if (module.relocation_model == clover::RelocationModel::PIE) return true;
			return symbol.visibility != clover::SymbolVisibility::Default;
		}

		bool EmitRuntimeCall(const char* symbol, const clover::Module& module,
			RISCVAssemblyOutputInterface& assembly_output,
			uni::OstreamTrait& output) {
			return assembly_output.EmitSymbol(symbol, true, false,
				clover::SymbolVisibility::Default, true, true, output) &&
				assembly_output.EmitDirectCall(symbol,
					module.relocation_model != clover::RelocationModel::Static, output);
		}

		void EmitSymbolInitializer(uni::OstreamTrait& output,
			const uni::String& symbol, stdsint addend) {
			output.OutFormat("%s", symbol.reference());
			if (addend > 0) output.OutFormat(" + %[i]", addend);
			else if (addend < 0) {
				stduint magnitude = (stduint)(-(addend + 1));
				magnitude++;
				output.OutFormat(" - %[u]", magnitude);
			}
		}

		const clover::SourceFile* FindSourceFile(const clover::Module& module,
			uint32 id) {
			for (const clover::SourceFile& source : module.source_files)
				if (source.id == id) return &source;
			return nullptr;
		}

		// Check target-wide requirements before writing partial assembly output.
		bool ValidateTarget(const RISCVTargetInfo& target, const clover::Module& module) {
			if (target.bits != 32 && target.bits != 64) {
				plogerro("RISC-V backend: unsupported -mbits=%[u]; supported bits: 32, 64", target.bits);
				return false;
			}
			if (!ValidateRISCVInstructionSet(target.extensions)) {
				plogerro("RISC-V backend: invalid or unsupported instruction-set extensions");
				return false;
			}
			if (!ValidateRISCVABI(target.bits, target.extensions, target.abi)) {
				plogerro("RISC-V backend: ABI is incompatible with the selected XLEN or extensions");
				return false;
			}
			if (module.pointer_bits && module.pointer_bits != target.bits) {
				plogerro("RISC-V backend: IR requires %[u]-bit pointers, but target uses %[u]-bit pointers",
					module.pointer_bits, target.bits);
				return false;
			}
			if (!HasRISCVExtension(target.extensions, RISCVExtension::I)) {
				plogerro("RISC-V backend: the I instruction set is required");
				return false;
			}
			if (HasRISCVExtension(target.extensions, RISCVExtension::D) && !HasRISCVExtension(target.extensions, RISCVExtension::F)) {
				plogerro("RISC-V backend: the D extension requires the F extension");
				return false;
			}
			if (target.bits == 32) {
				if (!ValidateRV32Subset(module)) return false;
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
				if (global.type == clover::Type::F32 && !HasRISCVExtension(target.extensions, RISCVExtension::F)) {
					plogerro("RISC-V backend: f32 globals require -minst=imf or imfd");
					return false;
				}
				if (global.type == clover::Type::F64 && !HasRISCVExtension(target.extensions, RISCVExtension::D)) {
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
				if ((global.initializer_values.Count() &&
					global.initializer_fragments.Count()) ||
					(global.initializer_symbol.getByteCount() &&
						(global.initializer_values.Count() ||
						global.initializer_fragments.Count() || global.initializer ||
						global.initializer_high))) {
					plogerro("RISC-V backend: global object has conflicting initializer forms");
					return false;
				}
				if (!global.initializer_symbol.getByteCount() &&
					global.initializer_addend) {
					plogerro("RISC-V backend: global symbol addend requires a symbol");
					return false;
				}
				if (global.initializer_symbol.getByteCount() &&
					(global.type != clover::Type::Ptr || global.element_count != 1 ||
						(!FindFunction(module, global.initializer_symbol) &&
							!FindGlobal(module, global.initializer_symbol)))) {
					plogerro("RISC-V backend: invalid global symbol initializer '%s'",
						global.initializer_symbol.reference());
					return false;
				}
				if (global.initializer_symbol.getByteCount() &&
					global.initializer_addend &&
					FindFunction(module, global.initializer_symbol)) {
					plogerro("RISC-V backend: function relocation cannot have an addend");
					return false;
				}
				if (global.is_read_only && global.initializer_symbol.getByteCount() &&
					module.relocation_model != clover::RelocationModel::Static) {
					plogerro("RISC-V backend: PIC/PIE read-only symbol pointer initializer is not supported");
					return false;
				}
				stdsint initializer_elements = TypeArrayElements(global.type);
				const stduint max_signed = stduint(-1) >> 1;
				if (initializer_elements > 0 &&
					(stduint)initializer_elements <= max_signed /
						(stduint)global.element_count)
					initializer_elements *= global.element_count;
				else initializer_elements = 0;
				if (global.initializer_values.Count() &&
					(!initializer_elements || global.initializer_values.Count() !=
						(stduint)initializer_elements)) {
					plogerro("RISC-V backend: global initializer element count mismatch");
					return false;
				}
				stdsint type_size = TypeStorageBytes(module, global.type, target);
				if (!type_size || (stduint)type_size > (stduint(-1) >> 1) /
					(stduint)global.element_count) {
					plogerro("RISC-V backend: global object size is invalid");
					return false;
				}
				stdsint object_size = type_size * global.element_count;
				if (!global.alignment && !TypeStorageAlignmentBytes(
					module, global.type, target)) {
					plogerro("RISC-V backend: global object requires explicit alignment");
					return false;
				}
				stdsint fragment_end = 0;
				for (const clover::GlobalInitializerFragment& fragment :
					global.initializer_fragments) {
					stdsint fragment_size = TypeStorageBytes(fragment.type, target);
					if (!fragment_size || fragment.offset < (stduint)fragment_end ||
						fragment.offset + fragment_size > (stduint)object_size) {
						plogerro("RISC-V backend: invalid global initializer fragment");
						return false;
					}
					if (!fragment.initializer_symbol.getByteCount() &&
						fragment.initializer_addend) {
						plogerro("RISC-V backend: fragment symbol addend requires a symbol");
						return false;
					}
					if (fragment.initializer_symbol.getByteCount()) {
						if (fragment.type != clover::Type::Ptr ||
							fragment.initializer || fragment.initializer_high ||
							(!FindFunction(module, fragment.initializer_symbol) &&
								!FindGlobal(module, fragment.initializer_symbol))) {
							plogerro("RISC-V backend: invalid fragment symbol initializer '%s'",
								fragment.initializer_symbol.reference());
							return false;
						}
						if (fragment.initializer_addend &&
							FindFunction(module, fragment.initializer_symbol)) {
							plogerro("RISC-V backend: function fragment relocation cannot have an addend");
							return false;
						}
						if (global.is_read_only &&
							module.relocation_model != clover::RelocationModel::Static) {
							plogerro("RISC-V backend: PIC/PIE read-only symbol pointer fragment is not supported");
							return false;
						}
					}
					if (fragment.type == clover::Type::F32 && !HasRISCVExtension(target.extensions, RISCVExtension::F)) {
						plogerro("RISC-V backend: f32 global data requires -minst=imf or imfd");
						return false;
					}
					if (fragment.type == clover::Type::F64 && !HasRISCVExtension(target.extensions, RISCVExtension::D)) {
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
				if (!ValidateRegisterArguments(function, module, target)) return false;
				for (const clover::BasicBlock& block : function.blocks) {
					for (const clover::Instruction& instruction : block.instructions) {
						if (instruction.opcode != clover::Opcode::Ret) continue;
						bool valid = instruction.type == function.return_type &&
							instruction.by_value_return == function.returns_by_value &&
							instruction.operands.Count() ==
								(function.return_type == clover::Type::Void ? 0 : 1);
						if (valid && function.returns_by_value)
							valid = !StrCompare(instruction.object_type.defined_name.reference(),
								function.return_object_type.defined_name.reference()) &&
								instruction.operands.begin()[0].type == clover::Type::Ptr;
						if (!valid) {
							plogerro("RISC-V backend: return instruction does not match function signature '%s'",
								function.name.reference());
							return false;
						}
					}
				}
				for (const clover::BasicBlock& block : function.blocks) {
					for (const clover::Instruction& instruction : block.instructions) {
						bool uses_f32 = instruction.type == clover::Type::F32;
						bool uses_f64 = instruction.type == clover::Type::F64;
						for (const clover::Value& operand : instruction.operands) {
							uses_f32 = uses_f32 || operand.type == clover::Type::F32;
							uses_f64 = uses_f64 || operand.type == clover::Type::F64;
						}
						if (uses_f32 && !HasRISCVExtension(target.extensions, RISCVExtension::F)) {
							plogerro("RISC-V backend: f32 Clover operations require -minst=imf or imfd");
							return false;
						}
						if (uses_f64 && !HasRISCVExtension(target.extensions, RISCVExtension::D)) {
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
						if (instruction.opcode == clover::Opcode::FunctionAddr &&
							!FindFunction(module, instruction.name)) {
							plogerro("RISC-V backend: undefined function '%s'",
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
						if (instruction.opcode == clover::Opcode::Local) {
							clover::Type count_type = target.bits == 32 ?
								clover::Type::I32 : clover::Type::I64;
							bool dynamic = instruction.operands.Count() == 1;
							if (!instruction.has_result || instruction.result.type != clover::Type::Ptr ||
								instruction.operands.Count() > 1 ||
								(dynamic && (instruction.immediate != 0 ||
									instruction.operands.begin()[0].type != count_type)) ||
								(!dynamic && instruction.immediate <= 0)) {
								plogerro("RISC-V backend: invalid local instruction");
								return false;
							}
						}
						if (instruction.opcode == clover::Opcode::StackSave &&
							(!instruction.has_result || instruction.result.type != clover::Type::Ptr ||
							instruction.operands.Count())) {
							plogerro("RISC-V backend: invalid stacksave instruction");
							return false;
						}
						if (instruction.opcode == clover::Opcode::StackRestore &&
							(instruction.has_result || instruction.operands.Count() != 1 ||
							instruction.operands.begin()[0].type != clover::Type::Ptr)) {
							plogerro("RISC-V backend: invalid stackrestore instruction");
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
						if (!HasRISCVExtension(target.extensions, RISCVExtension::M) &&
							(instruction.opcode == clover::Opcode::Mul ||
							instruction.opcode == clover::Opcode::SDiv ||
							instruction.opcode == clover::Opcode::UDiv ||
							instruction.opcode == clover::Opcode::SRem ||
							instruction.opcode == clover::Opcode::URem)) {
							plogerro("RISC-V backend: Clover multiply/divide/remainder requires -minst=im");
							return false;
						}
						if (instruction.opcode == clover::Opcode::Call) {
							if (instruction.has_callee_value) {
								if (instruction.callee_value.type != clover::Type::Ptr) {
									plogerro("RISC-V backend: indirect call target must be ptr");
									return false;
								}
								continue;
							}
							const clover::Function* callee = FindFunction(module, instruction.callee);
							if (!callee) {
								plogerro("RISC-V backend: call to undefined function '%s'",
									instruction.callee.reference());
								return false;
							}
							if (instruction.type != callee->return_type ||
								instruction.by_value_return != callee->returns_by_value ||
								(instruction.by_value_return &&
									StrCompare(instruction.object_type.defined_name.reference(),
										callee->return_object_type.defined_name.reference())) ||
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

		// Stack location assigned to an SSA spill or a rematerialized fixed Local object.
		struct ValueSlot {
			bool defined = false;
			stdsint value_offset = 0; // Runtime SSA spill, including dynamic Local addresses.
			stdsint local_offset = 0; // Fixed Local object address rematerialized from s0.
			stdsint argument_object_offset = 0; // Direct by-value argument reconstruction.
		};

		// Function-specific stack layout used by the simple first backend.
		struct FunctionLayout {
			uni::Vector<ValueSlot> slots;
			stdsint frame_size = 0;
			stdsint hidden_return_offset = 0;
			unsigned word_bytes = 8;
		};

		// Round a frame size up to the RISC-V ABI's 16-byte stack alignment.
		stdsint AlignFrame(stdsint size, unsigned alignment) {
			return (size + alignment - 1) & ~stdsint(alignment - 1);
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

		stdsint TypeArrayElements(const clover::TypeReference& type) {
			stdsint count = 1;
			const stduint max_signed = stduint(-1) >> 1;
			for (unsigned index = 0; index < type.array_depth; index++) {
				if (!type.array_counts[index] ||
					(stduint)count > (max_signed /
						type.array_counts[index])) return 0;
				count *= (stdsint)type.array_counts[index];
			}
			return count;
		}

		stdsint TypeBits(const clover::Module& module,
			const clover::TypeReference& type, const RISCVTargetInfo& target) {
			stduint bits = 0;
			if (type.IsBits()) bits = type.bit_width;
			else if (type.IsDefined()) {
				const clover::TypeDefinition* definition = clover::FindTypeDefinition(
					module, type.defined_name.reference());
				if (!definition) return 0;
				bits = definition->size_bits;
			}
			else {
				stdsint scalar_bits = TypeBits(type.scalar, target);
				if (scalar_bits <= 0) return 0;
				bits = (stduint)scalar_bits;
			}
			stdsint elements = TypeArrayElements(type);
			if (!bits || !elements || bits > ((stduint(-1) >> 1) /
				(stduint)elements)) return 0;
			return (stdsint)(bits * (stduint)elements);
		}

		stdsint TypeStorageBytes(const clover::Module& module,
			const clover::TypeReference& type, const RISCVTargetInfo& target) {
			stdsint bits = TypeBits(module, type, target);
			return bits > 0 ? (bits + 7) / 8 : 0;
		}

		stdsint TypeStorageAlignmentBytes(const clover::Module& module,
			const clover::TypeReference& type, const RISCVTargetInfo& target) {
			stduint bits = 0;
			if (type.IsBits()) bits = 1;
			else if (type.IsDefined()) {
				const clover::TypeDefinition* definition = clover::FindTypeDefinition(
					module, type.defined_name.reference());
				if (!definition) return 0;
				bits = definition->alignment_bits;
			}
			else {
				stdsint scalar_bits = TypeBits(type.scalar, target);
				if (scalar_bits <= 0) return 0;
				bits = (stduint)scalar_bits;
			}
			return bits ? (stdsint)((bits + 7) / 8) : 0;
		}

		struct AggregateReturnPiece {
			bool floating = false;
			unsigned register_index = 0;
			stduint object_offset = 0;
			stduint size_bytes = 0;
		};

		bool ClassifyAggregateReturn(const clover::Module& module,
			const clover::TypeReference& type, const RISCVTargetInfo& target,
			AggregateReturnPiece pieces[2], unsigned& piece_count,
			bool& indirect) {
			piece_count = 0;
			indirect = false;
			stdsint size = TypeStorageBytes(module, type, target);
			if (size <= 0) return false;
			const stduint word_bytes = target.bits / 8;
			if (size > (stdsint)(word_bytes * 2)) {
				indirect = true;
				return true;
			}

			const unsigned flen = ABIFloatBits(target);
			const clover::TypeDefinition* definition = type.IsDefined() ?
				clover::FindTypeDefinition(module, type.defined_name.reference()) : nullptr;
			if (flen && definition &&
				definition->kind == clover::TypeDefinitionKind::Structure) {
				unsigned float_register = 0;
				unsigned integer_register = 0;
				bool eligible = true;
				for (const clover::TypeMember& member : definition->members) {
					if (!member.name.getByteCount()) continue;
					if (piece_count == 2 || member.type.IsArray() ||
						member.type.IsBits() || member.type.IsDefined() ||
						member.offset_bits % 8) {
						eligible = false;
						break;
					}
					stdsint member_size = TypeStorageBytes(module, member.type, target);
					if (member_size <= 0 || member_size > (stdsint)word_bytes ||
						member.offset_bits / 8 + (stduint)member_size > (stduint)size) {
						eligible = false;
						break;
					}
					AggregateReturnPiece& piece = pieces[piece_count++];
					piece.object_offset = member.offset_bits / 8;
					piece.size_bytes = (stduint)member_size;
					if (member.type.scalar == clover::Type::F32 ||
						member.type.scalar == clover::Type::F64) {
						if (TypeBits(member.type.scalar, target) > flen ||
							float_register >= 2) {
							eligible = false;
							break;
						}
						piece.floating = true;
						piece.register_index = float_register++;
					}
					else {
						if (member.type.scalar == clover::Type::Void ||
							integer_register >= 2) {
							eligible = false;
							break;
						}
						piece.register_index = integer_register++;
					}
				}
				if (eligible && piece_count && float_register) return true;
			}

			piece_count = (unsigned)((size + word_bytes - 1) / word_bytes);
			for (unsigned index = 0; index < piece_count; index++) {
				pieces[index].floating = false;
				pieces[index].register_index = index;
				pieces[index].object_offset = index * word_bytes;
				pieces[index].size_bytes = (stduint)size - pieces[index].object_offset;
				if (pieces[index].size_bytes > word_bytes)
					pieces[index].size_bytes = word_bytes;
			}
			return true;
		}

		stdsint AlignOffset(stdsint offset, stdsint alignment) {
			if (alignment <= 1) return offset;
			return ((offset + alignment - 1) / alignment) * alignment;
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

		// Spill ordinary SSA results; fixed Local results retain only their object offset.
		bool BuildLayout(const clover::Function& function, const clover::Module& module,
			const RISCVTargetInfo& target, FunctionLayout& layout) {
			layout.word_bytes = target.bits / 8;
			bool found;
			unsigned largest = LargestValueID(function, found);
			layout.slots = uni::Vector<ValueSlot>::From(ValueSlot(), found ? largest + 1 : 0);
			stdsint offset = layout.word_bytes * 2;
			if (function.returns_by_value) {
				stdsint return_size = TypeStorageBytes(module,
					function.return_object_type, target);
				if (return_size <= 0) return false;
				if (return_size > (stdsint)(target.bits / 4)) {
					layout.hidden_return_offset = offset = AlignOffset(
						offset + layout.word_bytes, layout.word_bytes);
				}
			}
			for (const clover::Parameter& parameter : function.parameters) {
				ValueSlot& slot = layout.slots[parameter.value.id];
				if (slot.defined) {
					plogerro("RISC-V backend: duplicate SSA value %%%[u]", parameter.value.id);
					return false;
				}
				slot.defined = true;
				stdsint size = TypeStorageBytes(parameter.value.type, target);
				if (size < (stdsint)layout.word_bytes) size = layout.word_bytes;
				stdsint alignment = size > (stdsint)layout.word_bytes ? 16 :
					layout.word_bytes;
				slot.value_offset = offset = AlignOffset(offset + size, alignment);
				if (parameter.by_value) {
					stdsint object_size = TypeStorageBytes(module,
						parameter.object_type, target);
					if (object_size <= 0) return false;
					if (object_size <= (stdsint)(target.bits / 4)) {
						stdsint object_alignment = TypeStorageAlignmentBytes(module,
							parameter.object_type, target);
						if (object_alignment <= 0) return false;
						slot.argument_object_offset = offset = AlignOffset(
							offset + object_size, object_alignment);
					}
				}
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
					bool dynamic_local = instruction.opcode == clover::Opcode::Local &&
						instruction.operands.Count();
					if ((instruction.opcode == clover::Opcode::Local && !dynamic_local) ||
						instruction.by_value_return) {
						stdsint element_size = instruction.has_object_type ?
							TypeStorageBytes(module, instruction.object_type, target) :
							TypeStorageBytes(instruction.type, target);
						stdsint element_count = instruction.opcode == clover::Opcode::Local &&
							instruction.immediate > 0 ?
							instruction.immediate : 1;
						if (!element_size || element_count <= 0) return false;
						stdsint alignment = instruction.alignment ?
							(stdsint)instruction.alignment : instruction.has_object_type ?
							TypeStorageAlignmentBytes(module, instruction.object_type, target) :
							element_size;
						if (!alignment) return false;
						slot.local_offset = AlignOffset(
							offset + element_size * element_count, alignment);
						offset = slot.local_offset;
					}
					else {
						stdsint size = TypeStorageBytes(instruction.result.type, target);
						if (size < (stdsint)layout.word_bytes) size = layout.word_bytes;
						stdsint alignment = size > (stdsint)layout.word_bytes ? 16 :
							layout.word_bytes;
						slot.value_offset = offset = AlignOffset(offset + size, alignment);
					}
				}
			}
			layout.frame_size = AlignFrame(offset,
				RISCVABIStackAlignment(target.abi));
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

		void EmitStackAdjustment(stdsint bytes, uni::OstreamTrait& output) {
			if (!bytes) return;
			if (FitsSignedImmediate12(bytes))
				output.OutFormat("\taddi sp, sp, %[i]\n", bytes);
			else {
				output.OutFormat("\tli t0, %[i]\n\tadd sp, sp, t0\n", bytes);
			}
		}

		void EmitMultiplyByConstant(const char* reg, stdsint multiplier,
			const RISCVTargetInfo& target, uni::OstreamTrait& output) {
			if (multiplier <= 1) return;
			stduint unsigned_multiplier = (stduint)multiplier;
			if (!(unsigned_multiplier & (unsigned_multiplier - 1))) {
				unsigned shift = 0;
				while ((stduint(1) << shift) != unsigned_multiplier) shift++;
				output.OutFormat("\tslli %s, %s, %[u]\n", reg, reg, shift);
				return;
			}
			if (HasRISCVExtension(target.extensions, RISCVExtension::M)) {
				output.OutFormat("\tli t1, %[i]\n\tmul %s, %s, t1\n",
					multiplier, reg, reg);
				return;
			}
			output.OutFormat("\tli t1, 0\n\tmv t2, %s\n", reg);
			while (unsigned_multiplier) {
				if (unsigned_multiplier & 1)
					output.OutFormat("\tadd t1, t1, t2\n");
				unsigned_multiplier >>= 1;
				if (unsigned_multiplier) output.OutFormat("\tslli t2, t2, 1\n");
			}
			output.OutFormat("\tmv %s, t1\n", reg);
		}

		void EmitOutgoingStackStore(const char* reg, stduint offset,
			unsigned word_bytes, uni::OstreamTrait& output) {
			const char* mnemonic = word_bytes == 4 ? "sw" : "sd";
			if (offset <= 2047)
				output.OutFormat("\t%s %s, %[u](sp)\n", mnemonic, reg, offset);
			else {
				output.OutFormat("\tli t2, %[u]\n\tadd t2, sp, t2\n\t%s %s, 0(t2)\n",
					offset, mnemonic, reg);
			}
		}

		void EmitIncomingStackLoad(const char* reg, stduint offset,
			unsigned word_bytes, uni::OstreamTrait& output) {
			const char* mnemonic = word_bytes == 4 ? "lw" : "ld";
			if (offset <= 2047)
				output.OutFormat("\t%s %s, %[u](s0)\n", mnemonic, reg, offset);
			else {
				output.OutFormat("\tli t2, %[u]\n\tadd t2, s0, t2\n\t%s %s, 0(t2)\n",
					offset, mnemonic, reg);
			}
		}

		void EmitAddressOffset(const char* destination, const char* base,
			stduint offset, uni::OstreamTrait& output) {
			if (!offset) output.OutFormat("\tmv %s, %s\n", destination, base);
			else if (offset <= 2047)
				output.OutFormat("\taddi %s, %s, %[u]\n", destination, base, offset);
			else output.OutFormat("\tli %s, %[u]\n\tadd %s, %s, %s\n",
				destination, offset, destination, base, destination);
		}

		bool LoadValue(const FunctionLayout& layout, clover::Value value,
			const char* reg, uni::OstreamTrait& output);

		bool EmitLoadObjectPiece(const FunctionLayout& layout, clover::Value address,
			stduint object_offset, stduint size_bytes, uni::OstreamTrait& output) {
			if (!size_bytes || size_bytes > 8 ||
				!LoadValue(layout, address, "t0", output)) return false;
			EmitAddressOffset("t3", "t0", object_offset, output);
			output.OutFormat("\tlbu t1, 0(t3)\n");
			for (stduint index = 1; index < size_bytes; index++) {
				output.OutFormat("\tlbu t2, %[u](t3)\n\tslli t2, t2, %[u]\n\tor t1, t1, t2\n",
					index, index * 8);
			}
			return true;
		}

		void EmitFrameByteStore(const char* source, stdsint offset,
			uni::OstreamTrait& output) {
			if (FitsSignedImmediate12(-offset))
				output.OutFormat("\tsb %s, -%[i](s0)\n", source, offset);
			else {
				EmitFrameAddress("t3", offset, output);
				output.OutFormat("\tsb %s, 0(t3)\n", source);
			}
		}

		void EmitStoreObjectPieceToFrame(const char* source, stdsint object_offset,
			stduint size_bytes, uni::OstreamTrait& output) {
			output.OutFormat("\tmv t2, %s\n", source);
			for (stduint index = 0; index < size_bytes; index++) {
				EmitFrameByteStore("t2", object_offset - (stdsint)index, output);
				if (index + 1 < size_bytes) output.OutFormat("\tsrli t2, t2, 8\n");
			}
		}

		void EmitFrameStore(const char* source, stdsint offset,
			unsigned word_bytes, uni::OstreamTrait& output) {
			const char* mnemonic = word_bytes == 4 ? "sw" : "sd";
			if (FitsSignedImmediate12(-offset))
				output.OutFormat("\t%s %s, -%[i](s0)\n", mnemonic, source, offset);
			else {
				const char* address = StrCompare(source, "t3") ? "t3" : "t4";
				EmitFrameAddress(address, offset, output);
				output.OutFormat("\t%s %s, 0(%s)\n", mnemonic, source, address);
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
			const char* mnemonic = layout.word_bytes == 4 ? "lw" : "ld";
			if (FitsSignedImmediate12(-slot->value_offset))
				output.OutFormat("\t%s %s, -%[i](s0)\n", mnemonic, reg, slot->value_offset);
			else {
				EmitFrameAddress(reg, slot->value_offset, output);
				output.OutFormat("\t%s %s, 0(%s)\n", mnemonic, reg, reg);
			}
			return true;
		}

		// Store an instruction result from a temporary register to its spill slot.
		bool StoreResult(const FunctionLayout& layout, const clover::Instruction& instruction,
			const char* reg, uni::OstreamTrait& output) {
			const ValueSlot* slot;
			if (!instruction.has_result || !SlotOf(layout, instruction.result, slot)) return false;
			if (!slot->value_offset) return false;
			EmitFrameStore(reg, slot->value_offset, layout.word_bytes, output);
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
				EmitFrameStore(low_register, slot->value_offset, 8, output);
				EmitFrameStore(high_register, slot->value_offset - 8, 8, output);
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
		void EmitEpilogue(unsigned word_bytes, uni::OstreamTrait& output) {
			const char* mnemonic = word_bytes == 4 ? "lw" : "ld";
			output.OutFormat("\t%s ra, -%u(s0)\n", mnemonic, word_bytes);
			output.OutFormat("\t%s t0, -%u(s0)\n", mnemonic, word_bytes * 2);
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
			const clover::Module& module,
			RISCVAssemblyOutputInterface* assembly_output,
			uni::OstreamTrait& output) {
			if (!RequireOperands(instruction, 2) ||
				!LoadWideValue(layout, instruction.operands.begin()[0], "a0", "a1", output) ||
				!LoadWideValue(layout, instruction.operands.begin()[1], "a2", "a3", output))
				return false;
			if (!assembly_output || !EmitRuntimeCall(helper, module, *assembly_output, output))
				return false;
			return StoreWideResult(layout, instruction, "a0", "a1", output);
		}

		void EmitZeroExtend(const char* reg, unsigned source_bits,
			unsigned xlen, uni::OstreamTrait& output) {
			if (source_bits >= xlen) return;
			if (source_bits == 1 || source_bits == 8) {
				output.OutFormat("\tandi %s, %s, %u\n", reg, reg,
					(source_bits == 1) ? 1 : 255);
				return;
			}
			unsigned shift = xlen - source_bits;
			output.OutFormat("\tslli %s, %s, %u\n\tsrli %s, %s, %u\n",
				reg, reg, shift, reg, reg, shift);
		}

		void EmitSignExtend(const char* reg, unsigned source_bits,
			unsigned xlen, uni::OstreamTrait& output) {
			if (source_bits >= xlen) return;
			unsigned shift = xlen - source_bits;
			output.OutFormat("\tslli %s, %s, %u\n\tsrai %s, %s, %u\n",
				reg, reg, shift, reg, reg, shift);
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
				EmitSignExtend("t1", 16, target.bits, output);
			else if (instruction.type == clover::Type::I32 && target.bits == 64)
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
			const FunctionLayout& layout, const clover::Module& module,
			const RISCVTargetInfo& target,
			uni::OstreamTrait& output) {
			if (!RequireOperands(instruction, 2) ||
				instruction.operands.begin()[0].type != clover::Type::Ptr ||
				instruction.operands.begin()[1].type == clover::Type::Ptr ||
				!LoadValue(layout, instruction.operands.begin()[0], "t0", output) ||
				!LoadValue(layout, instruction.operands.begin()[1], "t1", output)) return false;
			stdsint element_size = instruction.has_object_type ?
				TypeStorageBytes(module, instruction.object_type, target) :
				TypeStorageBytes(instruction.type, target);
			switch (element_size) {
			case 1: break;
			case 2: output.OutFormat("\tslli t1, t1, 1\n"); break;
			case 4: output.OutFormat("\tslli t1, t1, 2\n"); break;
			case 8: output.OutFormat("\tslli t1, t1, 3\n"); break;
			case 16: output.OutFormat("\tslli t1, t1, 4\n"); break;
			default:
				if (element_size <= 0) {
					plogerro("RISC-V backend: invalid ptradd element size %[i]",
						element_size);
					return false;
				}
				output.OutFormat("\tli t2, 0\n");
				for (stduint multiplier = (stduint)element_size; multiplier;
					multiplier >>= 1) {
					if (multiplier & 1) output.OutFormat("\tadd t2, t2, t1\n");
					if (multiplier >> 1) output.OutFormat("\tslli t1, t1, 1\n");
				}
				output.OutFormat("\tmv t1, t2\n");
				break;
			}
			output.OutFormat("\tadd t2, t0, t1\n");
			return StoreResult(layout, instruction, "t2", output);
		}

		const char* BinaryMnemonic(const clover::Instruction& instruction,
			const RISCVTargetInfo& target, const char* wide, const char* word) {
			if (target.bits == 32) return wide;
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
				if (!instruction.has_result || !SlotOf(layout, instruction.result, slot))
					return false;
				if (!instruction.operands.Count()) return slot->local_offset != 0;
				if (instruction.operands.Count() != 1 || !slot->value_offset ||
					!LoadValue(layout, instruction.operands.begin()[0], "t0", output))
					return false;
				stdsint element_size = instruction.has_object_type ?
					TypeStorageBytes(module, instruction.object_type, target) :
					TypeStorageBytes(instruction.type, target);
				if (element_size <= 0) return false;
				EmitMultiplyByConstant("t0", element_size, target, output);
				output.OutFormat("\taddi t0, t0, 15\n");
				output.OutFormat("\tandi t0, t0, -16\n");
				output.OutFormat("\tsub sp, sp, t0\n");
				return StoreResult(layout, instruction, "sp", output);
			}

			case clover::Opcode::StackSave:
				if (!instruction.has_result || instruction.operands.Count()) return false;
				output.OutFormat("\tmv t0, sp\n");
				return StoreResult(layout, instruction, "t0", output);

			case clover::Opcode::StackRestore:
				if (!RequireOperands(instruction, 1) ||
					!LoadValue(layout, instruction.operands.begin()[0], "t0", output))
					return false;
				output.OutFormat("\tmv sp, t0\n");
				return true;

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
				case clover::Type::I64: output.OutFormat("\tld t1, 0(t0)\n"); break;
				case clover::Type::Ptr:
					output.OutFormat(target.bits == 32 ? "\tlw t1, 0(t0)\n" :
						"\tld t1, 0(t0)\n");
					break;
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
				case clover::Type::I64: output.OutFormat("\tsd t0, 0(t1)\n"); break;
				case clover::Type::Ptr:
					output.OutFormat(target.bits == 32 ? "\tsw t0, 0(t1)\n" :
						"\tsd t0, 0(t1)\n");
					break;
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
				output.OutFormat(".L%s_memcopy_loop_%[u]:\n",
					function.name.reference(), private_label);
				output.OutFormat("\tbeqz t3, .L%s_memcopy_end_%[u]\n",
					function.name.reference(), private_label);
				output.OutFormat("\tlbu t2, 0(t1)\n\tsb t2, 0(t0)\n");
				output.OutFormat("\taddi t0, t0, 1\n\taddi t1, t1, 1\n");
				output.OutFormat("\taddi t3, t3, -1\n\tj .L%s_memcopy_loop_%[u]\n",
					function.name.reference(), private_label);
				output.OutFormat(".L%s_memcopy_end_%[u]:\n",
					function.name.reference(), private_label);
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
				BinaryMnemonic(instruction, target, "add", "addw"), output);
			case clover::Opcode::Sub: return EmitBinary(instruction, layout,
				BinaryMnemonic(instruction, target, "sub", "subw"), output);
			case clover::Opcode::Mul: return EmitBinary(instruction, layout,
				BinaryMnemonic(instruction, target, "mul", "mulw"), output);
			case clover::Opcode::SDiv: return EmitBinary(instruction, layout,
				BinaryMnemonic(instruction, target, "div", "divw"), output);
			case clover::Opcode::UDiv: return EmitBinary(instruction, layout,
				BinaryMnemonic(instruction, target, "divu", "divuw"), output);
			case clover::Opcode::SRem: return EmitBinary(instruction, layout,
				BinaryMnemonic(instruction, target, "rem", "remw"), output);
			case clover::Opcode::URem: return EmitBinary(instruction, layout,
				BinaryMnemonic(instruction, target, "remu", "remuw"), output);
			case clover::Opcode::And: return EmitBinary(instruction, layout, "and", output);
			case clover::Opcode::Or: return EmitBinary(instruction, layout, "or", output);
			case clover::Opcode::Xor: return EmitBinary(instruction, layout, "xor", output);
			case clover::Opcode::Shl: return EmitBinary(instruction, layout,
				BinaryMnemonic(instruction, target, "sll", "sllw"), output);
			case clover::Opcode::AShr: return EmitBinary(instruction, layout,
				BinaryMnemonic(instruction, target, "sra", "sraw"), output);
			case clover::Opcode::LShr: return EmitBinary(instruction, layout,
				BinaryMnemonic(instruction, target, "srl", "srlw"), output);
			case clover::Opcode::FAdd:
				if (instruction.type == clover::Type::F128)
					return EmitWideBinary(instruction, layout, "__addtf3", module,
						assembly_output, output);
				return EmitFloatBinary(instruction, layout, "fadd", output);
			case clover::Opcode::FSub:
				if (instruction.type == clover::Type::F128)
					return EmitWideBinary(instruction, layout, "__subtf3", module,
						assembly_output, output);
				return EmitFloatBinary(instruction, layout, "fsub", output);
			case clover::Opcode::FMul:
				if (instruction.type == clover::Type::F128)
					return EmitWideBinary(instruction, layout, "__multf3", module,
						assembly_output, output);
				return EmitFloatBinary(instruction, layout, "fmul", output);
			case clover::Opcode::FDiv:
				if (instruction.type == clover::Type::F128)
					return EmitWideBinary(instruction, layout, "__divtf3", module,
						assembly_output, output);
				return EmitFloatBinary(instruction, layout, "fdiv", output);
			case clover::Opcode::PtrAdd:
				return EmitPtrAdd(instruction, layout, module, target, output);

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
					if (!assembly_output || !EmitRuntimeCall(helper, module,
						*assembly_output, output)) return false;
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
				EmitZeroExtend("t0",
					(unsigned)TypeBits(instruction.operands.begin()[0].type, target),
					target.bits, output);
				return StoreResult(layout, instruction, "t0", output);

			case clover::Opcode::SExt:
				if (!RequireOperands(instruction, 1) ||
					!LoadValue(layout, instruction.operands.begin()[0], "t0", output)) return false;
				EmitSignExtend("t0",
					(unsigned)TypeBits(instruction.operands.begin()[0].type, target),
					target.bits, output);
				return StoreResult(layout, instruction, "t0", output);

			case clover::Opcode::Trunc:
				if (!RequireOperands(instruction, 1) ||
					!LoadValue(layout, instruction.operands.begin()[0], "t0", output)) return false;
				EmitZeroExtend("t0", (unsigned)TypeBits(instruction.type, target),
					target.bits, output);
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
				EmitZeroExtend("t0", (unsigned)TypeBits(instruction.type, target),
					target.bits, output);
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
					output.OutFormat("\tmv a0, t0\n");
					if (!assembly_output || !EmitRuntimeCall(helper, module,
						*assembly_output, output)) return false;
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
					if (!assembly_output || !EmitRuntimeCall(helper, module,
						*assembly_output, output)) return false;
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
					if (!assembly_output || !EmitRuntimeCall(
						source_type == clover::Type::F32 ? "__extendsftf2" : "__extenddftf2",
						module, *assembly_output, output)) return false;
					return StoreWideResult(layout, instruction, "a0", "a1", output);
				}
				if (source_type == clover::Type::F128) {
					if (!LoadWideValue(layout, instruction.operands.begin()[0],
						"a0", "a1", output)) return false;
					if (!assembly_output || !EmitRuntimeCall(
						instruction.type == clover::Type::F32 ? "__trunctfsf2" : "__trunctfdf2",
						module, *assembly_output, output)) return false;
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
				if (!instruction.name.getByteCount() || !assembly_output) return false;
				{
					const clover::Global* symbol = FindGlobal(module, instruction.name);
					if (!symbol || !assembly_output->EmitAddress(
						symbol->name.reference(), !IsSymbolLocallyBound(module, *symbol),
						symbol->is_declaration,
						symbol->binding == clover::SymbolBinding::Weak,
						symbol->visibility, target.bits, output)) return false;
				}
				return StoreResult(layout, instruction, "t0", output);

			case clover::Opcode::FunctionAddr:
				if (!instruction.name.getByteCount() || !assembly_output) return false;
				{
					const clover::Function* symbol = FindFunction(module, instruction.name);
					if (!symbol || !assembly_output->EmitAddress(
						symbol->name.reference(), !IsSymbolLocallyBound(module, *symbol),
						symbol->is_declaration,
						symbol->binding == clover::SymbolBinding::Weak,
						symbol->visibility, target.bits, output)) return false;
				}
				return StoreResult(layout, instruction, "t0", output);

			case clover::Opcode::Call: {
				RISCVABIArgumentState state;
				const clover::Function* callee = instruction.has_callee_value ? nullptr :
					FindFunction(module, instruction.callee);
				if (!instruction.has_callee_value && !callee) return false;
				const bool aggregate_result = instruction.by_value_return;
				const stdsint result_size = aggregate_result ?
					TypeStorageBytes(module, instruction.object_type, target) : 0;
				if (aggregate_result && result_size <= 0) return false;
				AggregateReturnPiece return_pieces[2];
				unsigned return_piece_count = 0;
				bool indirect_result = false;
				if (aggregate_result && !ClassifyAggregateReturn(module,
					instruction.object_type, target, return_pieces,
					return_piece_count, indirect_result)) return false;
				if (indirect_result) {
					RISCVABIArgumentLocation hidden_location;
					if (!RISCVAllocateArgument(clover::Type::Ptr, target, state,
						false, hidden_location) || hidden_location.piece_count != 1 ||
						hidden_location.pieces[0].kind !=
							RISCVABIArgumentPieceKind::IntegerRegister ||
						hidden_location.pieces[0].register_index != 0 ||
						!LoadValue(layout, instruction.result, "a0", output)) return false;
				}
				stduint count = instruction.call_arguments.Count() ?
					instruction.call_arguments.Count() : instruction.operands.Count();
				uni::Vector<RISCVABIArgumentLocation> locations;
				for (stduint index = 0; index < count; index++) {
					const bool by_value = instruction.call_arguments.Count() &&
						instruction.call_arguments.begin()[index].by_value;
					clover::Value argument = instruction.call_arguments.Count() ?
						instruction.call_arguments.begin()[index].value :
						instruction.operands.begin()[index];
					bool variadic = callee && callee->is_variadic &&
						index >= callee->parameters.Count();
					RISCVABIArgumentLocation location;
					bool allocated = by_value ? RISCVAllocateAggregateArgument(module,
						instruction.call_arguments.begin()[index].object_type, target,
						state, variadic, location) : RISCVAllocateArgument(
						argument.type, target, state, variadic, location);
					if (!allocated || !locations.Append(location)) return false;
				}
				stduint stack_alignment = RISCVABIStackAlignment(target.abi);
				stdsint outgoing_size = (stdsint)((state.stack_bytes +
					stack_alignment - 1) & ~(stack_alignment - 1));
				EmitStackAdjustment(-outgoing_size, output);
				for (stduint index = 0; index < count; index++) {
					const clover::CallArgument* call_argument =
						instruction.call_arguments.Count() ?
						&instruction.call_arguments.begin()[index] : nullptr;
					const bool by_value = call_argument && call_argument->by_value;
					clover::Value argument = instruction.call_arguments.Count() ?
						call_argument->value :
						instruction.operands.begin()[index];
					const RISCVABIArgumentLocation& location = locations.begin()[index];
					for (unsigned piece_index = 0;
						piece_index < location.piece_count; piece_index++) {
						const RISCVABIArgumentPiece& piece = location.pieces[piece_index];
						const char* destination = piece.kind ==
							RISCVABIArgumentPieceKind::FloatRegister ?
							FloatArgumentRegisters[piece.register_index] :
							ArgumentRegisters[piece.register_index];
						if (by_value && !location.indirect) {
							if (!EmitLoadObjectPiece(layout, argument, piece.object_offset,
								piece.size_bytes, output)) return false;
						}
						else if (argument.type == clover::Type::F128) {
							if (!LoadWideValue(layout, argument, "t1", "t2", output)) return false;
							if (piece.object_offset) output.OutFormat("\tmv t1, t2\n");
						}
						else if (piece.kind == RISCVABIArgumentPieceKind::FloatRegister) {
							if (!LoadFloatValue(layout, argument, destination, output)) return false;
							continue;
						}
						else if (IsFloatType(argument.type)) {
							if (!LoadFloatBits(layout, argument, "t1", output)) return false;
						}
						else if (!LoadValue(layout, argument, "t1", output)) return false;
						if (piece.kind == RISCVABIArgumentPieceKind::Stack)
							EmitOutgoingStackStore("t1", piece.stack_offset,
								target.bits / 8, output);
						else {
							output.OutFormat("\tmv %s, t1\n", destination);
							if (!by_value && argument.type == clover::Type::I32 &&
								target.bits == 64)
								output.OutFormat("\taddiw %s, %s, 0\n", destination, destination);
						}
					}
				}
				if (instruction.has_callee_value) {
					if (!LoadValue(layout, instruction.callee_value, "t0", output)) return false;
					output.OutFormat("\tjalr ra, 0(t0)\n");
				}
				else if (!assembly_output || !callee ||
					!assembly_output->EmitDirectCall(callee->name.reference(),
						!IsSymbolLocallyBound(module, *callee), output)) return false;
				EmitStackAdjustment(outgoing_size, output);
				if (indirect_result) {
					// The callee wrote the object through the hidden result address.
					return true;
				}
				if (aggregate_result) {
					const ValueSlot* result_slot;
					if (!SlotOf(layout, instruction.result, result_slot) ||
						!result_slot->local_offset) return false;
					for (unsigned index = 0; index < return_piece_count; index++) {
						const AggregateReturnPiece& piece = return_pieces[index];
						if (piece.floating) {
							output.OutFormat(piece.size_bytes == 4 ?
								"\tfmv.x.w t1, fa%u\n" : "\tfmv.x.d t1, fa%u\n",
								piece.register_index);
						}
						else output.OutFormat("\tmv t1, a%u\n", piece.register_index);
						EmitStoreObjectPieceToFrame("t1",
							result_slot->local_offset - (stdsint)piece.object_offset,
							piece.size_bytes, output);
					}
					return true;
				}
				if (instruction.type == clover::Type::Void) return true;
				if (IsFloatType(instruction.type)) {
					if (instruction.type == clover::Type::F128)
						return StoreWideResult(layout, instruction, "a0", "a1", output);
					if (TypeBits(instruction.type, target) <= ABIFloatBits(target))
						return StoreFloatResult(layout, instruction, "fa0", output);
					return StoreResult(layout, instruction, "a0", output);
				}
				if (instruction.type == clover::Type::I32 && target.bits == 64)
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
				if (instruction.by_value_return) {
					if (!RequireOperands(instruction, 1) ||
						instruction.operands.begin()[0].type != clover::Type::Ptr)
						return false;
					stdsint size = TypeStorageBytes(module, instruction.object_type, target);
					AggregateReturnPiece return_pieces[2];
					unsigned return_piece_count = 0;
					bool indirect_result = false;
					if (size <= 0 || !ClassifyAggregateReturn(module,
						instruction.object_type, target, return_pieces,
						return_piece_count, indirect_result) ||
						!LoadValue(layout, instruction.operands.begin()[0], "t1", output))
						return false;
					if (indirect_result) {
						if (!layout.hidden_return_offset) return false;
						EmitFrameAddress("t0", layout.hidden_return_offset, output);
						output.OutFormat("\tld t0, 0(t0)\n");
						private_label++;
						output.OutFormat("\tli t3, %[i]\n.L%s_retcopy_loop_%[u]:\n",
							size, function.name.reference(), private_label);
						output.OutFormat("\tbeqz t3, .L%s_retcopy_end_%[u]\n",
							function.name.reference(), private_label);
						output.OutFormat("\tlbu t2, 0(t1)\n\tsb t2, 0(t0)\n");
						output.OutFormat("\taddi t0, t0, 1\n\taddi t1, t1, 1\n");
						output.OutFormat("\taddi t3, t3, -1\n\tj .L%s_retcopy_loop_%[u]\n",
							function.name.reference(), private_label);
						output.OutFormat(".L%s_retcopy_end_%[u]:\n",
							function.name.reference(), private_label);
					}
					else {
						for (unsigned index = 0; index < return_piece_count; index++) {
							const AggregateReturnPiece& piece = return_pieces[index];
							if (!EmitLoadObjectPiece(layout,
								instruction.operands.begin()[0], piece.object_offset,
								piece.size_bytes, output))
								return false;
							if (piece.floating)
								output.OutFormat(piece.size_bytes == 4 ?
									"\tfmv.w.x fa%u, t1\n" : "\tfmv.d.x fa%u, t1\n",
									piece.register_index);
							else output.OutFormat("\tmv a%u, t1\n",
								piece.register_index);
						}
					}
					EmitEpilogue(target.bits / 8, output);
					return true;
				}
				if (instruction.type == clover::Type::Void) {
					if (instruction.operands.Count()) return false;
					EmitEpilogue(target.bits / 8, output);
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
				EmitEpilogue(target.bits / 8, output);
				return true;
			}
			return false;
		}

		// Emit one function with a conservative stack slot for every SSA result.
		bool EmitFunction(const clover::Function& function, const RISCVTargetInfo& target,
			const clover::Module& module, uni::OstreamTrait& output,
			RISCVAssemblyOutputInterface* assembly_output) {
			FunctionLayout layout;
			if (!BuildLayout(function, module, target, layout)) return false;

			output.OutFormat("\n");
			if (!assembly_output->EmitSymbol(function.name.reference(), true,
				function.binding == clover::SymbolBinding::Weak,
				function.visibility, function.linkage == clover::Linkage::External,
				false, output)) return false;
			output.OutFormat("%s:\n", function.name.reference());
			const char* word_store = layout.word_bytes == 4 ? "sw" : "sd";
			if (layout.frame_size <= 2047) {
				output.OutFormat("\taddi sp, sp, -%[i]\n", layout.frame_size);
				output.OutFormat("\t%s ra, %[i](sp)\n", word_store,
					layout.frame_size - layout.word_bytes);
				output.OutFormat("\t%s s0, %[i](sp)\n", word_store,
					layout.frame_size - layout.word_bytes * 2);
				output.OutFormat("\taddi s0, sp, %[i]\n", layout.frame_size);
			}
			else {
				// Preserve the caller frame pointer before constructing a large frame.
				output.OutFormat("\tmv t1, s0\n");
				output.OutFormat("\tli t0, %[i]\n", layout.frame_size);
				output.OutFormat("\tsub sp, sp, t0\n");
				output.OutFormat("\tadd s0, sp, t0\n");
				output.OutFormat("\t%s ra, -%u(s0)\n", word_store,
					layout.word_bytes);
				output.OutFormat("\t%s t1, -%u(s0)\n", word_store,
					layout.word_bytes * 2);
			}
			RISCVABIArgumentState argument_state;
			if (function.returns_by_value && layout.hidden_return_offset) {
				EmitFrameStore("a0", layout.hidden_return_offset,
					layout.word_bytes, output);
				RISCVABIArgumentLocation hidden_location;
				if (!RISCVAllocateArgument(clover::Type::Ptr, target,
					argument_state, false, hidden_location)) return false;
			}
			for (stduint index = 0; index < function.parameters.Count(); index++) {
				const clover::Parameter& parameter = function.parameters.begin()[index];
				const ValueSlot* slot;
				if (!SlotOf(layout, parameter.value, slot) || !slot->value_offset) return false;
				RISCVABIArgumentLocation location;
				bool allocated = parameter.by_value ? RISCVAllocateAggregateArgument(
					module, parameter.object_type, target, argument_state, false, location) :
					RISCVAllocateArgument(parameter.value.type, target,
						argument_state, false, location);
				if (!allocated) return false;
				for (unsigned piece_index = 0; piece_index < location.piece_count;
					piece_index++) {
					const RISCVABIArgumentPiece& piece = location.pieces[piece_index];
					const char* source = piece.kind == RISCVABIArgumentPieceKind::FloatRegister ?
						FloatArgumentRegisters[piece.register_index] :
						ArgumentRegisters[piece.register_index];
					if (piece.kind == RISCVABIArgumentPieceKind::Stack) {
						EmitIncomingStackLoad("t0", piece.stack_offset,
							layout.word_bytes, output);
						source = "t0";
					}
					if (parameter.by_value && !location.indirect) {
						if (!slot->argument_object_offset) return false;
						EmitStoreObjectPieceToFrame(source,
							slot->argument_object_offset - (stdsint)piece.object_offset,
							piece.size_bytes, output);
						continue;
					}
					if (piece.kind == RISCVABIArgumentPieceKind::FloatRegister) {
						EmitFrameFloatStore(parameter.value.type, source,
							slot->value_offset - (stdsint)piece.object_offset, output);
						continue;
					}
					if (parameter.value.type == clover::Type::I32 && target.bits == 64 &&
						!parameter.by_value) {
						output.OutFormat("\taddiw t0, %s, 0\n", source);
						source = "t0";
					}
					EmitFrameStore(source,
						slot->value_offset - (stdsint)piece.object_offset,
						layout.word_bytes, output);
				}
				if (parameter.by_value && !location.indirect) {
					EmitFrameAddress("t0", slot->argument_object_offset, output);
					EmitFrameStore("t0", slot->value_offset,
						layout.word_bytes, output);
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
				EmitEpilogue(layout.word_bytes, output);
			}
			output.OutFormat(".size %s, .-%s\n", function.name.reference(), function.name.reference());
			return true;
		}

	}

	bool BackendRISCV::Generate(const clover::Module& module, uni::OstreamTrait& output) {
		if (module.ir_class != clover::IRClass::Soft) {
			plogerro("Clover RISC-V backend: expected Soft IR module");
			return false;
		}
		switch (module.relocation_model) {
		case clover::RelocationModel::Static:
		case clover::RelocationModel::PIC:
		case clover::RelocationModel::PIE:
			break;
		default:
			plogerro("Clover RISC-V backend: invalid relocation model");
			return false;
		}
		if (!ValidateTarget(target, module)) return false;
		RISCVAssemblyOutputGCC gcc_output;
		char instruction_set[128];
		if (!FormatRISCVInstructionSet(target.extensions, instruction_set,
			sizeof(instruction_set))) return false;
		const char* abi = RISCVABIName(target.abi);
		if (!abi) return false;
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
		for (const clover::Global& global : module.globals)
			if (global.is_declaration &&
				global.linkage == clover::Linkage::External &&
				!assembly_output->EmitSymbol(
				global.name.reference(), false,
				global.binding == clover::SymbolBinding::Weak, global.visibility,
				global.linkage == clover::Linkage::External, true,
				*assembly_stream))
				return false;
		for (const clover::Function& function : module.functions)
			if (function.is_declaration &&
				function.linkage == clover::Linkage::External &&
				!assembly_output->EmitSymbol(
				function.name.reference(), true,
				function.binding == clover::SymbolBinding::Weak, function.visibility,
				function.linkage == clover::Linkage::External, true,
				*assembly_stream))
				return false;
		for (const clover::Global& global : module.globals) {
			if (global.is_declaration) continue;
			stdsint element_size = TypeStorageBytes(module, global.type, target);
			stdsint object_size = element_size * global.element_count;
			if (!element_size || object_size <= 0) return false;
			bool has_initializer = global.initializer != 0 ||
				global.initializer_high != 0 ||
				global.initializer_symbol.getByteCount() != 0 ||
				global.initializer_values.Count() != 0 ||
				global.initializer_fragments.Count() != 0;
			assembly_stream->OutFormat(global.is_read_only ? ".section .rodata\n" :
				has_initializer ? ".data\n" : ".bss\n");
			if (!assembly_output->EmitSymbol(global.name.reference(), false,
				global.binding == clover::SymbolBinding::Weak, global.visibility,
				global.linkage == clover::Linkage::External, false,
				*assembly_stream))
				return false;
			if (options.assembly_syntax == RISCVAssemblySyntax::GCC)
				assembly_stream->OutFormat(".size %s, %[i]\n",
					global.name.reference(), object_size);
			stdsint natural_alignment = TypeStorageAlignmentBytes(
				module, global.type, target);
			assembly_stream->OutFormat(".balign %[u]\n%s:\n",
				global.alignment ? global.alignment : (stduint)natural_alignment,
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
						assembly_stream->OutFormat("\t.quad %[i]\n", fragment.initializer);
						break;
					case clover::Type::Ptr:
						assembly_stream->OutFormat(fragment_size == 4 ?
							"\t.word " : "\t.quad ");
						if (fragment.initializer_symbol.getByteCount())
							EmitSymbolInitializer(*assembly_stream,
								fragment.initializer_symbol,
								fragment.initializer_addend);
						else assembly_stream->OutFormat("%[i]", fragment.initializer);
						assembly_stream->OutChar('\n');
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
				clover::TypeReference base_type = global.type;
				base_type.array_depth = 0;
				stdsint value_size = TypeStorageBytes(module, base_type, target);
				for (stduint index = 0; index < global.initializer_values.Count(); index++) {
					stdsint value = global.initializer_values.begin()[index];
					switch (value_size) {
					case 1: assembly_stream->OutFormat("\t.byte %[i]\n", value); break;
					case 2: assembly_stream->OutFormat("\t.half %[i]\n", value); break;
					case 4: assembly_stream->OutFormat("\t.word %[i]\n", value); break;
					case 8: assembly_stream->OutFormat("\t.quad %[i]\n", value); break;
					default: return false;
					}
				}
			}
			else if (global.initializer_symbol.getByteCount()) {
				if (global.element_count != 1 || global.type != clover::Type::Ptr)
					return false;
				assembly_stream->OutFormat(element_size == 4 ? "\t.word " : "\t.quad ");
				EmitSymbolInitializer(*assembly_stream, global.initializer_symbol,
					global.initializer_addend);
				assembly_stream->OutChar('\n');
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
			if (!function.is_declaration &&
				!EmitFunction(function, target, module, *assembly_stream,
				assembly_output)) return false;
		return options.assembly_syntax != RISCVAssemblySyntax::AASM ||
			aasm_output.Flush();
	}

}
