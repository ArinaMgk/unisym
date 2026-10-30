// ASCII C++11 TAB4 LF
// Docutitle: Lango C Frontend Clover Lowering
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

#include "../../../../../inc/cpp/Lango/Frontend/C/Frontend-c-internal.hpp"

namespace clover {
	using namespace uni::lango;

	namespace {

		// Result returned while recursively lowering one AST expression.
		struct LoweredValue {
			Value value;
			bool valid = false;
			FrontendCType type;
			bool is_lvalue = false;
		};

		// Associates a source identifier with the Ptr value produced by Local.
		struct LocalValue {
			uni::String name;
			Value address;
			FrontendCType type;
			const uni::Nnode* declaration = nullptr;
			const uni::Nnode* scope = nullptr;
			bool active = false;
		};

		// One C statement atom after transparent tok_statement wrappers are removed.
		struct StatementAtom {
			uni::Nnode* node = nullptr;
			const uni::Nnode* source = nullptr;
		};

		// Function declaration visible at the current C translation-unit position.
		struct FrontendCFunction {
			uni::String name;
			FrontendCType return_type;
			uni::Vector<FrontendCType> parameter_types;
		};

		// Module-scope object visible at the current translation-unit position.
		struct FrontendCGlobal {
			uni::String name;
			FrontendCType type;
		};

		bool ParseIntegerLiteral(const char* text, stdsint& value) {
			if (!text || !*text) return false;
			unsigned base = 10;
			if (text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
				base = 16;
				text += 2;
			}
			else if (text[0] == '0' && text[1]) base = 8;
			if (!*text) return false;
			stdsint result = 0;
			for (; *text; text++) {
				unsigned digit;
				if (*text >= '0' && *text <= '9') digit = (unsigned)(*text - '0');
				else if (*text >= 'a' && *text <= 'f') digit = (unsigned)(*text - 'a' + 10);
				else if (*text >= 'A' && *text <= 'F') digit = (unsigned)(*text - 'A' + 10);
				else return false;
				if (digit >= base) return false;
				result = result * (stdsint)base + (stdsint)digit;
			}
			value = result;
			return true;
		}

		Type CloverTypeFor(const FrontendCType& type,
			const FrontendCTargetInfo& target) {
			if (type.IsVoid()) return Type::Void;
			if (type.IsPointer()) return Type::Ptr;
			unsigned bits = target.int_bits;
			switch (type.integer_rank) {
			case FrontendCIntegerRank::Char: bits = target.char_bits; break;
			case FrontendCIntegerRank::Short: bits = target.short_bits; break;
			case FrontendCIntegerRank::Int: bits = target.int_bits; break;
			case FrontendCIntegerRank::Long: bits = target.long_bits; break;
			case FrontendCIntegerRank::LongLong: bits = target.long_long_bits; break;
			}
			return bits == 8 ? Type::I8 : bits == 16 ? Type::I16 :
				bits == 64 ? Type::I64 : Type::I32;
		}

		// Lowers one C function body into one Clover function.
		class FunctionBuilder {
		private:
			Function* function = nullptr;
			FrontendCType return_type;
			stduint current_block = 0;
			unsigned next_block = 0;
			unsigned next_value = 0;
			uni::Vector<LocalValue> locals;
			uni::Vector<const uni::Nnode*> emitted_sources;
			const uni::Vector<FrontendCFunction>* visible_functions = nullptr;
			const uni::Vector<FrontendCGlobal>* visible_globals = nullptr;
			const FrontendCTargetInfo* target = nullptr;
			const FrontendCTypeInfo* type_info = nullptr;
			const FrontendCDebugInfo* debug_info = nullptr;
			const FrontendCDiagnostics* diagnostics = nullptr;
			bool failed = false;

			BasicBlock& CurrentBlock() {
				return function->blocks[current_block];
			}

			BasicBlock& EntryBlock() {
				return function->blocks[0];
			}

			bool IsKeyword(const uni::Nnode* node, const char* keyword) const {
				return node && node->addr && !StrCompare(node->addr, keyword);
			}

			void AppendUnsigned(uni::String& text, unsigned value) {
				char digits[16];
				stduint count = 0;
				do {
					digits[count++] = (char)('0' + value % 10);
					value /= 10;
				} while (value);
				while (count) text << digits[--count];
			}

			uni::String NewBlockName(const char* prefix) {
				uni::String name;
				while (*prefix) name << *prefix++;
				name << '.';
				AppendUnsigned(name, next_block++);
				return name;
			}

			stduint AppendBlock(const uni::String& name) {
				BasicBlock block;
				block.name = name;
				function->blocks.Append(block);
				return function->blocks.Count() - 1;
			}

			stduint NewBlock(const char* prefix) {
				return AppendBlock(NewBlockName(prefix));
			}

			bool BlockTerminated() const {
				const BasicBlock& block = function->blocks.begin()[current_block];
				if (!block.instructions.Count()) return false;
				Opcode opcode = block.instructions.begin()[block.instructions.Count() - 1].opcode;
				return opcode == Opcode::Jmp || opcode == Opcode::Ret;
			}

			void EmitJump(stduint target) {
				EmitJump(function->blocks[target].name);
			}

			void EmitJump(const uni::String& target) {
				Instruction instruction;
				instruction.opcode = Opcode::Jmp;
				instruction.target = target;
				CurrentBlock().instructions.Append(instruction);
			}

			void EmitJump(Value condition, stduint target_true, stduint target_false) {
				EmitJump(condition, function->blocks[target_true].name,
					function->blocks[target_false].name);
			}

			void EmitJump(Value condition, const uni::String& target_true,
				const uni::String& target_false) {
				Instruction instruction;
				instruction.opcode = Opcode::Jmp;
				instruction.type = Type::I1;
				instruction.operands.Append(condition);
				instruction.target = target_true;
				instruction.target_false = target_false;
				CurrentBlock().instructions.Append(instruction);
			}

			bool SourceEmitted(const uni::Nnode* statement) const {
				for (const uni::Nnode* emitted : emitted_sources)
					if (emitted == statement) return true;
				return false;
			}

			void AttachSource(Instruction& instruction, const uni::Nnode* statement) {
				if (!debug_info || !statement || SourceEmitted(statement)) return;
				const uni::String* source = debug_info->Find(statement);
				if (!source) return;
				instruction.debug_source = *source;
				emitted_sources.Append(statement);
			}

			void AttachSource(stduint instruction_index, const uni::Nnode* statement) {
				if (instruction_index >= CurrentBlock().instructions.Count()) return;
				AttachSource(CurrentBlock().instructions[instruction_index], statement);
			}

			// Reserve the next function-local SSA identifier.
			Value NewValue(Type type) {
				Value value;
				value.type = type;
				value.id = next_value++;
				return value;
			}

			// Append a result-producing instruction with one operand.
			Value EmitResult(Opcode opcode, Type type, Value operand) {
				Instruction instruction;
				instruction.opcode = opcode;
				instruction.type = type;
				instruction.has_result = true;
				instruction.result = NewValue(type);
				instruction.operands.Append(operand);
				CurrentBlock().instructions.Append(instruction);
				return instruction.result;
			}

			// Append a result-producing instruction with two operands.
			Value EmitResult(Opcode opcode, Type type, Value left, Value right) {
				Instruction instruction;
				instruction.opcode = opcode;
				instruction.type = type;
				instruction.has_result = true;
				instruction.result = NewValue(type);
				instruction.operands.Append(left);
				instruction.operands.Append(right);
				CurrentBlock().instructions.Append(instruction);
				return instruction.result;
			}

			// Add a signed element index to a pointer while retaining pointee size.
			Value EmitPtrAdd(Value pointer, Value index, const FrontendCType& pointer_type) {
				FrontendCType pointee_type = pointer_type.DereferenceType();
				stduint scalar_count = pointee_type.StorageElementCount();
				if (scalar_count > 1) {
					Value scale = EmitConst((stdsint)scalar_count);
					if (index.type == Type::I64)
						scale = EmitConversion(Opcode::SExt, Type::I64, scale);
					index = EmitResult(Opcode::Mul, index.type, index, scale);
				}
				Instruction instruction;
				instruction.opcode = Opcode::PtrAdd;
				instruction.type = CloverType(pointee_type);
				instruction.has_result = true;
				instruction.result = NewValue(Type::Ptr);
				instruction.operands.Append(pointer);
				instruction.operands.Append(index);
				CurrentBlock().instructions.Append(instruction);
				return instruction.result;
			}

			// Store is side-effecting and therefore has no SSA result.
			void EmitStore(Value value, Value address) {
				Instruction instruction;
				instruction.opcode = Opcode::Store;
				instruction.type = value.type;
				instruction.operands.Append(value);
				instruction.operands.Append(address);
				CurrentBlock().instructions.Append(instruction);
			}

			unsigned IntegerBits(const FrontendCType& type) const {
				switch (type.integer_rank) {
				case FrontendCIntegerRank::Char: return target->char_bits;
				case FrontendCIntegerRank::Short: return target->short_bits;
				case FrontendCIntegerRank::Int: return target->int_bits;
				case FrontendCIntegerRank::Long: return target->long_bits;
				case FrontendCIntegerRank::LongLong: return target->long_long_bits;
				}
				return 0;
			}

			Type CloverType(const FrontendCType& type) const {
				return CloverTypeFor(type, *target);
			}

			FrontendCType IntType() const {
				return FrontendCType();
			}

			FrontendCType SizeType() const {
				FrontendCType type;
				type.is_signed = false;
				type.integer_rank = target->long_bits == target->pointer_bits ?
					FrontendCIntegerRank::Long : FrontendCIntegerRank::LongLong;
				return type;
			}

			Value EmitConversion(Opcode opcode, Type destination, Value operand) {
				Instruction instruction;
				instruction.opcode = opcode;
				instruction.type = destination;
				instruction.has_result = true;
				instruction.result = NewValue(destination);
				instruction.operands.Append(operand);
				CurrentBlock().instructions.Append(instruction);
				return instruction.result;
			}

			LoweredValue ConvertInteger(LoweredValue lowered,
				const FrontendCType& destination, const uni::Nnode* node) {
				lowered = ToRValue(lowered, node);
				if (!lowered.valid || !lowered.type.IsInteger() || !destination.IsInteger())
					return Error("integer conversion requires integer operands", node);
				if (lowered.value.type == Type::I1 && CloverType(lowered.type) != Type::I1)
					lowered.value = EmitConversion(Opcode::ZExt,
						CloverType(lowered.type), lowered.value);
				unsigned source_bits = IntegerBits(lowered.type);
				unsigned destination_bits = IntegerBits(destination);
				if (source_bits < destination_bits)
					lowered.value = EmitConversion(lowered.type.is_signed ? Opcode::SExt :
						Opcode::ZExt, CloverType(destination), lowered.value);
				else if (source_bits > destination_bits)
					lowered.value = EmitConversion(Opcode::Trunc,
						CloverType(destination), lowered.value);
				lowered.type = destination;
				lowered.is_lvalue = false;
				return lowered;
			}

			FrontendCType PromoteType(FrontendCType type) const {
				if (type.IsInteger() &&
					type.integer_rank < FrontendCIntegerRank::Int) return IntType();
				return type;
			}

			FrontendCType CommonIntegerType(FrontendCType left,
				FrontendCType right) const {
				left = PromoteType(left);
				right = PromoteType(right);
				if (left.integer_rank == right.integer_rank) {
					if (!left.is_signed || !right.is_signed) left.is_signed = false;
					return left;
				}
				FrontendCType higher = left.integer_rank > right.integer_rank ? left : right;
				FrontendCType lower = left.integer_rank > right.integer_rank ? right : left;
				if (higher.is_signed == lower.is_signed || !higher.is_signed) return higher;
				if (IntegerBits(higher) > IntegerBits(lower)) return higher;
				higher.is_signed = false;
				return higher;
			}

			LoweredValue ToRValue(LoweredValue lowered, const uni::Nnode* node) {
				if (lowered.valid && lowered.type.IsArray()) {
					lowered.type = lowered.type.DecayType();
					lowered.is_lvalue = false;
					return lowered;
				}
				if (!lowered.valid || !lowered.is_lvalue) return lowered;
				lowered.value = EmitResult(Opcode::Load, CloverType(lowered.type),
					lowered.value);
				lowered.is_lvalue = false;
				return lowered;
			}

			Value AsCondition(LoweredValue lowered, const uni::Nnode* node) {
				lowered = ToRValue(lowered, node);
				if (!lowered.valid || !lowered.type.IsInteger()) {
					Error("condition requires an integer expression", node);
					return lowered.value;
				}
				if (lowered.value.type == Type::I1) return lowered.value;
				Value integer = lowered.value;
				Value zero = EmitConst(0, lowered.type);
				Instruction instruction;
				instruction.opcode = Opcode::ICmp;
				instruction.type = CloverType(lowered.type);
				instruction.has_result = true;
				instruction.result = NewValue(Type::I1);
				instruction.operands.Append(integer);
				instruction.operands.Append(zero);
				instruction.predicate = ICmpPredicate::NE;
				CurrentBlock().instructions.Append(instruction);
				return instruction.result;
			}

			// Materialize a source integer literal as a Const instruction.
			Value EmitConst(stdsint immediate,
				const FrontendCType& type = FrontendCType()) {
				Instruction instruction;
				instruction.opcode = Opcode::Const;
				instruction.type = CloverType(type);
				instruction.has_result = true;
				instruction.result = NewValue(instruction.type);
				instruction.immediate = immediate;
				CurrentBlock().instructions.Append(instruction);
				return instruction.result;
			}

			LocalValue* FindLocal(const char* name) {
				for (stduint index = locals.Count(); index; index--) {
					LocalValue& local = locals[index - 1];
					if (local.active && !StrCompare(local.name.reference(), name)) return &local;
				}
				return nullptr;
			}

			const FrontendCFunction* FindFunction(const char* name) const {
				if (!visible_functions) return nullptr;
				for (const FrontendCFunction& candidate : *visible_functions)
					if (!StrCompare(candidate.name.reference(), name)) return &candidate;
				return nullptr;
			}

			const FrontendCGlobal* FindGlobal(const char* name) const {
				if (!visible_globals) return nullptr;
				for (const FrontendCGlobal& candidate : *visible_globals)
					if (!StrCompare(candidate.name.reference(), name)) return &candidate;
				return nullptr;
			}

			Value EmitGlobalAddress(const FrontendCGlobal& global) {
				Instruction instruction;
				instruction.opcode = Opcode::GlobalAddr;
				instruction.type = Type::Ptr;
				instruction.has_result = true;
				instruction.result = NewValue(Type::Ptr);
				instruction.name = global.name;
				CurrentBlock().instructions.Append(instruction);
				return instruction.result;
			}

			bool PredeclareLocal(const char* name, const FrontendCType& type,
				const uni::Nnode* declaration, const uni::Nnode* scope) {
				for (const LocalValue& local : locals)
					if (local.scope == scope && !StrCompare(local.name.reference(), name)) {
						diagnostics->Report(DiagnosticStage::Semantic,
							"redefinition of local variable", declaration);
						failed = true;
						return false;
					}
				Instruction instruction;
				instruction.opcode = Opcode::Local;
				instruction.type = CloverType(type);
				instruction.immediate = (stdsint)type.StorageElementCount();
				instruction.has_result = true;
				instruction.result = NewValue(Type::Ptr);
				instruction.name = name;
				AttachSource(instruction, declaration);
				EntryBlock().instructions.Append(instruction);

				LocalValue local;
				local.name = name;
				local.address = instruction.result;
				local.type = type;
				local.declaration = declaration;
				local.scope = scope;
				locals.Append(local);
				return true;
			}

			bool ParseDeclaratorNode(uni::Nnode* node, uni::Nnode*& identifier,
				FrontendCType& type) {
				if (!node) return false;
				if (node->type == tok_func && node->addr &&
					!StrCompare(node->addr, "OP@PREMEMB") && node->subf &&
					!node->subf->next) {
					if (!ParseDeclaratorNode(node->subf, identifier, type)) return false;
					type = type.AddressType();
					return true;
				}
				if (node->type == tok_func && node->addr &&
					!StrCompare(node->addr, "operator[]")) {
					uni::Nnode* declarator = node->subf;
					uni::Nnode* dimension = declarator ? declarator->next : nullptr;
					if (!declarator || !dimension || dimension->next ||
						dimension->type != tok_number || !dimension->addr ||
						!ParseDeclaratorNode(declarator, identifier, type)) return false;
					stdsint count = atoins(dimension->addr);
					if (count <= 0) return false;
					return type.AppendArrayDimension((stduint)count);
				}
				if (node->type != tok_identy || !node->addr) return false;
				identifier = node;
				return true;
			}

			bool ParseDeclarator(uni::Nnode* node, uni::Nnode*& identifier,
				FrontendCType& type, const FrontendCType& base_type) {
				type = base_type;
				return ParseDeclaratorNode(node, identifier, type);
			}

			bool PredeclareDeclarator(uni::Nnode* node, const uni::Nnode* declaration,
				const uni::Nnode* scope, const FrontendCType& base_type) {
				if (!node) return false;
				if (node->type == tok_func && node->addr &&
					!StrCompare(node->addr, "OP@COMMA")) {
					for (uni::Nnode* child = node->subf; child; child = child->next)
						if (!PredeclareDeclarator(child, declaration, scope, base_type)) return false;
					return true;
				}
				uni::Nnode* declarator = node;
				if (node->type == tok_func && node->addr &&
					!StrCompare(node->addr, "OP@ASSIGN")) declarator = node->subf;
				uni::Nnode* identifier = nullptr;
				FrontendCType type;
				if (!ParseDeclarator(declarator, identifier, type, base_type)) return false;
				return PredeclareLocal(identifier->addr, type, declaration, scope);
			}

			bool PredeclareBlock(uni::Nnode* block) {
				for (uni::Nnode* node = block ? block->subf : nullptr; node; node = node->next) {
					if (node->type == ToToken(FrontendCToken::Declaration)) {
						const FrontendCType* base_type = type_info->Find(node);
						if (!base_type || !PredeclareDeclarator(node->subf, node, block, *base_type)) {
							if (failed) return false;
							return Error("invalid int declaration", node).valid;
						}
					}
					if (node->type == tok_block && !PredeclareBlock(node)) return false;
					if (node->subf) {
						for (uni::Nnode* child = node->subf; child; child = child->next)
							if (child->type == tok_block && !PredeclareBlock(child)) return false;
					}
				}
				return true;
			}

			LocalValue* ActivateLocal(const char* name, const uni::Nnode* declaration) {
				for (LocalValue& local : locals) {
					if (local.declaration == declaration &&
						!StrCompare(local.name.reference(), name)) {
						local.active = true;
						return &local;
					}
				}
				return nullptr;
			}

			void AppendInitializerValues(uni::Nnode* node,
				uni::Vector<uni::Nnode*>& values) {
				if (node && (node->type == tok_block ||
					(node->type == tok_func && !node->addr)) && node->subf) {
					AppendInitializerValues(node->subf, values);
					return;
				}
				if (node && node->type == tok_func && node->addr &&
					!StrCompare(node->addr, "OP@COMMA")) {
					for (uni::Nnode* child = node->subf; child; child = child->next)
						AppendInitializerValues(child, values);
					return;
				}
				if (node) values.Append(node);
			}

			bool InitializeArray(LocalValue& local, uni::Nnode* initializer) {
				uni::Vector<uni::Nnode*> values;
				AppendInitializerValues(initializer, values);
				stduint element_count = local.type.StorageElementCount();
				if (values.Count() > element_count)
					return Error("too many array initializer values", initializer).valid;
				FrontendCType element_type = local.type;
				element_type.array_dimension_count = 0;
				FrontendCType element_pointer = element_type.AddressType();
				for (stduint index = 0; index < element_count; index++) {
					Value value;
					if (index < values.Count()) {
						uni::Nnode* value_node = values.begin()[index];
						LoweredValue lowered = ToRValue(LowerExpression(value_node), value_node);
						if (!lowered.valid) return false;
						if (!lowered.type.IsInteger())
							return Error("array initializer requires an integer", value_node).valid;
						lowered = ConvertInteger(lowered, element_type, value_node);
						if (!lowered.valid) return false;
						value = lowered.value;
					}
					else value = EmitConst(0, element_type);
					Value element_index = EmitConst((stdsint)index);
					Value address = EmitPtrAdd(local.address, element_index, element_pointer);
					EmitStore(value, address);
				}
				return true;
			}

			bool LowerDeclarator(uni::Nnode* node, const uni::Nnode* declaration) {
				if (!node) return Error("missing declarator", declaration).valid;
				if (node->type == tok_func && node->addr &&
					!StrCompare(node->addr, "OP@COMMA")) {
					for (uni::Nnode* child = node->subf; child; child = child->next)
						if (!LowerDeclarator(child, declaration)) return false;
					return true;
				}
				uni::Nnode* declarator = node;
				uni::Nnode* initializer_node = nullptr;
				if (node->type == tok_func && node->addr &&
					!StrCompare(node->addr, "OP@ASSIGN")) {
					declarator = node->subf;
					initializer_node = declarator ? declarator->next : nullptr;
					if (!initializer_node || initializer_node->next)
						return Error("invalid initializer", node).valid;
				}
				uni::Nnode* identifier = nullptr;
				FrontendCType declared_type;
				const FrontendCType* base_type = type_info->Find(declaration);
				if (!base_type || !ParseDeclarator(declarator, identifier,
					declared_type, *base_type))
					return Error("invalid declarator", declarator).valid;
				LocalValue* local = ActivateLocal(identifier->addr, declaration);
				if (!local) return Error("invalid local declaration", identifier).valid;
				if (!initializer_node) return true;
				if (local->type.IsArray())
					return InitializeArray(*local, initializer_node);
				LoweredValue initializer = ToRValue(
					LowerExpression(initializer_node), initializer_node);
				if (!initializer.valid) return false;
				if (local->type.IsInteger())
					initializer = ConvertInteger(initializer, local->type, initializer_node);
				else if (!local->type.IsSame(initializer.type))
					return Error("incompatible initializer type", initializer_node).valid;
				if (!initializer.valid) return false;
				EmitStore(initializer.value, local->address);
				return true;
			}

			void LeaveScope(const uni::Nnode* block) {
				for (LocalValue& local : locals)
					if (local.scope == block) local.active = false;
			}

			// Record a lowering error and return an invalid expression result.
			LoweredValue Error(const char* message, const uni::Nnode* node) {
				failed = true;
				diagnostics->Report(DiagnosticStage::Lowering, message, node);
				return {};
			}

			void AppendCallArguments(uni::Nnode* node,
				uni::Vector<uni::Nnode*>& arguments) {
				if (node && node->type == tok_func && node->addr &&
					!StrCompare(node->addr, "OP@COMMA")) {
					for (uni::Nnode* child = node->subf; child; child = child->next)
						AppendCallArguments(child, arguments);
					return;
				}
				if (node) arguments.Append(node);
			}

			LoweredValue LowerCall(uni::Nnode* node) {
				const FrontendCFunction* signature = FindFunction(node->addr);
				if (!signature) return Error("call to undeclared function", node);
				uni::Vector<uni::Nnode*> arguments;
				AppendCallArguments(node->subf, arguments);
				if (arguments.Count() != signature->parameter_types.Count())
					return Error("function argument count mismatch", node);

				Instruction instruction;
				instruction.opcode = Opcode::Call;
				instruction.type = CloverType(signature->return_type);
				instruction.callee = signature->name;
				for (stduint index = 0; index < arguments.Count(); index++) {
					uni::Nnode* argument_node = arguments.begin()[index];
					LoweredValue argument = ToRValue(
						LowerExpression(argument_node), argument_node);
					if (!argument.valid) return argument;
					const FrontendCType& parameter_type =
						signature->parameter_types.begin()[index];
					if (parameter_type.IsInteger())
						argument = ConvertInteger(argument, parameter_type, argument_node);
					else if (!argument.type.IsSame(parameter_type))
						return Error("incompatible function argument type", argument_node);
					if (!argument.valid) return argument;
					instruction.operands.Append(argument.value);
				}
				instruction.has_result = !signature->return_type.IsVoid();
				if (instruction.has_result) instruction.result = NewValue(instruction.type);
				CurrentBlock().instructions.Append(instruction);
				return { instruction.result, true, signature->return_type, false };
			}

			LoweredValue LowerSubscript(uni::Nnode* node) {
				uni::Nnode* left_node = node->subf;
				uni::Nnode* right_node = left_node ? left_node->next : nullptr;
				if (!left_node || !right_node || right_node->next)
					return Error("array subscript requires two operands", node);
				LoweredValue left = ToRValue(LowerExpression(left_node), left_node);
				LoweredValue right = ToRValue(LowerExpression(right_node), right_node);
				if (!left.valid || !right.valid) return {};
				if (left.type.IsInteger() && right.type.IsPointer()) {
					LoweredValue temporary = left;
					left = right;
					right = temporary;
				}
				if (!left.type.IsPointer() || !right.type.IsInteger())
					return Error("array subscript requires a pointer and an integer", node);
				FrontendCType index_type = PromoteType(right.type);
				right = ConvertInteger(right, index_type, right_node);
				if (!right.valid) return right;
				Value index = right.value;
				Value address = EmitPtrAdd(left.value, index, left.type);
				return { address, !failed, left.type.DereferenceType(), true };
			}

			LoweredValue LowerIncrement(uni::Nnode* operand, bool increment,
				bool return_updated) {
				LoweredValue lowered = LowerExpression(operand);
				if (!lowered.valid) return lowered;
				if (!lowered.is_lvalue ||
					(!lowered.type.IsInteger() && !lowered.type.IsPointer()))
					return Error("increment requires an arithmetic or pointer lvalue", operand);
				Value previous = EmitResult(Opcode::Load, CloverType(lowered.type), lowered.value);
				Value one = EmitConst(1, lowered.type.IsInteger() ?
					PromoteType(lowered.type) : IntType());
				Value updated;
				if (lowered.type.IsPointer()) {
					Value index = one;
					if (!increment) {
						Value zero = EmitConst(0);
						index = EmitResult(Opcode::Sub, Type::I32, zero, one);
					}
					updated = EmitPtrAdd(previous, index, lowered.type);
				}
				else {
					FrontendCType arithmetic_type = PromoteType(lowered.type);
					LoweredValue promoted = ConvertInteger(
						{ previous, true, lowered.type, false }, arithmetic_type, operand);
					updated = EmitResult(increment ? Opcode::Add : Opcode::Sub,
						CloverType(arithmetic_type), promoted.value, one);
					LoweredValue converted = ConvertInteger(
						{ updated, true, arithmetic_type, false }, lowered.type, operand);
					if (!converted.valid) return converted;
					updated = converted.value;
				}
				EmitStore(updated, lowered.value);
				return { return_updated ? updated : previous, true, lowered.type, false };
			}

			bool IsTypeWord(const uni::Nnode* node, const char* word) const {
				return node && node->type == tok_identy && node->addr &&
					!StrCompare(node->addr, word);
			}

			// Parse the integer and pointer subset of a C type-name used by sizeof.
			bool ParseTypeName(uni::Nnode* node, FrontendCType& type,
				const uni::Nnode* end = nullptr) const {
				if (node && node->next == end && node->type == tok_func && node->addr &&
					!StrCompare(node->addr, "operator[]")) {
					uni::Nnode* dimension = node->subf ? node->subf->Tail() : nullptr;
					if (!dimension || dimension == node->subf ||
						dimension->type != tok_number || !dimension->addr ||
						!ParseTypeName(node->subf, type, dimension)) return false;
					stdsint count = atoins(dimension->addr);
					return count > 0 && type.AppendArrayDimension((stduint)count);
				}
				unsigned signed_count = 0, unsigned_count = 0, char_count = 0;
				unsigned short_count = 0, int_count = 0, long_count = 0, void_count = 0;
				bool found = false;
				for (; node != end && node && node->type == tok_identy; node = node->next) {
					if (IsTypeWord(node, "signed")) signed_count++;
					else if (IsTypeWord(node, "unsigned")) unsigned_count++;
					else if (IsTypeWord(node, "char")) char_count++;
					else if (IsTypeWord(node, "short")) short_count++;
					else if (IsTypeWord(node, "int")) int_count++;
					else if (IsTypeWord(node, "long")) long_count++;
					else if (IsTypeWord(node, "void")) void_count++;
					else break;
					found = true;
				}
				if (!found || signed_count > 1 || unsigned_count > 1 || void_count > 1 ||
					(signed_count && unsigned_count) || char_count > 1 ||
					short_count > 1 || int_count > 1 || long_count > 2 ||
					(char_count && (short_count || int_count || long_count)) ||
					(short_count && long_count) ||
					(void_count && (signed_count || unsigned_count || char_count ||
						short_count || int_count || long_count))) return false;
				type = FrontendCType();
				type.is_void = void_count != 0;
				type.is_signed = !unsigned_count;
				if (char_count) type.integer_rank = FrontendCIntegerRank::Char;
				else if (short_count) type.integer_rank = FrontendCIntegerRank::Short;
				else if (long_count == 1) type.integer_rank = FrontendCIntegerRank::Long;
				else if (long_count == 2) type.integer_rank = FrontendCIntegerRank::LongLong;
				while (node != end && node && node->type == tok_symbol && node->addr &&
					!StrCompare(node->addr, "*")) {
					type = type.AddressType();
					node = node->next;
				}
				return node == end;
			}

			bool ResolveExpressionType(uni::Nnode* node, FrontendCType& type) {
				if (!node) return false;
				if (node->type == tok_number) {
					type = IntType();
					return true;
				}
				if (node->type == tok_identy) {
					LocalValue* local = FindLocal(node->addr);
					if (local) type = local->type;
					else {
						const FrontendCGlobal* global = FindGlobal(node->addr);
						if (!global) return false;
						type = global->type;
					}
					return true;
				}
				if (node->type != tok_func) return false;
				if (!node->addr) return ResolveExpressionType(node->subf, type);
				if (!StrCompare(node->addr, "sizeof") ||
					!StrCompare(node->addr, "OP@SIZEOF")) {
					type = SizeType();
					return true;
				}
				if (!StrCompare(node->addr, "operator[]")) {
					uni::Nnode* left = node->subf;
					uni::Nnode* right = left ? left->next : nullptr;
					FrontendCType left_type, right_type;
					if (!left || !right || right->next ||
						!ResolveExpressionType(left, left_type) ||
						!ResolveExpressionType(right, right_type)) return false;
					if (left_type.IsArray()) left_type = left_type.DecayType();
					if (right_type.IsArray()) right_type = right_type.DecayType();
					if (left_type.IsInteger() && right_type.IsPointer()) left_type = right_type;
					if (!left_type.IsPointer()) return false;
					type = left_type.DereferenceType();
					return true;
				}
				if (StrLength(node->addr) < 3 || node->addr[0] != 'O' ||
					node->addr[1] != 'P' || node->addr[2] != '@') {
					const FrontendCFunction* signature = FindFunction(node->addr);
					if (!signature) return false;
					type = signature->return_type;
					return true;
				}
				uni::Nnode* left = node->subf;
				uni::Nnode* right = left ? left->next : nullptr;
				FrontendCType left_type;
				if (!ResolveExpressionType(left, left_type)) return false;
				if (!StrCompare(node->addr, "OP@PREADDR")) {
					type = left_type.AddressType();
					return true;
				}
				if (!StrCompare(node->addr, "OP@PREMEMB")) {
					if (!left_type.IsPointer()) return false;
					type = left_type.DereferenceType();
					return true;
				}
				if (!StrCompare(node->addr, "OP@JEQUAL") ||
					!StrCompare(node->addr, "OP@JNOTEQ") ||
					!StrCompare(node->addr, "OP@JBELOW") ||
					!StrCompare(node->addr, "OP@JBEEQU") ||
					!StrCompare(node->addr, "OP@JGREAT") ||
					!StrCompare(node->addr, "OP@JGREQU")) {
					type = IntType();
					return true;
				}
				if (!right) {
					if (!StrCompare(node->addr, "OP@SUFADD") ||
						!StrCompare(node->addr, "OP@SUFSUB") ||
						!StrCompare(node->addr, "OP@PREADD") ||
						!StrCompare(node->addr, "OP@@PRESUB")) type = left_type;
					else type = PromoteType(left_type);
					return true;
				}
				FrontendCType right_type;
				if (!ResolveExpressionType(right, right_type)) return false;
				if (!StrCompare(node->addr, "OP@ASSIGN")) {
					type = left_type;
					return true;
				}
				if (!StrCompare(node->addr, "OP@COMMA")) {
					type = right_type;
					return true;
				}
				if ((!StrCompare(node->addr, "OP@ARIADD") ||
					!StrCompare(node->addr, "OP@ARISUB")) && left_type.IsPointer()) {
					type = left_type;
					return true;
				}
				if (!StrCompare(node->addr, "OP@ARIADD") && right_type.IsPointer()) {
					type = right_type;
					return true;
				}
				if (!left_type.IsInteger() || !right_type.IsInteger()) return false;
				type = CommonIntegerType(left_type, right_type);
				return true;
			}

			stdsint SizeOf(const FrontendCType& type) const {
				unsigned bits = type.IsPointer() ? target->pointer_bits : IntegerBits(type);
				stduint element_count = type.IsPointer() ? 1 : type.StorageElementCount();
				return (stdsint)((bits / target->char_bits) * element_count);
			}

			LoweredValue LowerSizeof(uni::Nnode* operand) {
				FrontendCType operand_type;
				if (!ParseTypeName(operand, operand_type) &&
					!ResolveExpressionType(operand, operand_type))
					return Error("invalid sizeof operand", operand);
				if (operand_type.IsVoid()) return Error("sizeof cannot be applied to void", operand);
				FrontendCType result_type = SizeType();
				return { EmitConst(SizeOf(operand_type), result_type), true,
					result_type, false };
			}

			// Translate one operator and recursively lower its operands.
			LoweredValue LowerOperator(uni::Nnode* node) {
				using namespace uni;
				if (!node->addr && node->subf) return LowerExpression(node->subf);
				if (!node->addr || !node->subf) return Error("bad operator", node);

				Nnode* left_node = node->subf;
				Nnode* right_node = left_node->next;
				const char* operation = node->addr;

				if (!StrCompare(operation, "OP@SUFADD"))
					return LowerIncrement(left_node, true, false);
				if (!StrCompare(operation, "OP@SUFSUB"))
					return LowerIncrement(left_node, false, false);
				if (!StrCompare(operation, "OP@PREADD"))
					return LowerIncrement(left_node, true, true);
				if (!StrCompare(operation, "OP@@PRESUB"))
					return LowerIncrement(left_node, false, true);
				if (!StrCompare(operation, "OP@SIZEOF"))
					return LowerSizeof(left_node);
				if (!StrCompare(operation, "OP@PREADDR")) {
					LoweredValue operand = LowerExpression(left_node);
					if (!operand.valid) return operand;
					if (!operand.is_lvalue)
						return Error("cannot take the address of an rvalue", left_node);
					return { operand.value, true, operand.type.AddressType(), false };
				}
				if (!StrCompare(operation, "OP@PREMEMB")) {
					LoweredValue operand = ToRValue(LowerExpression(left_node), left_node);
					if (!operand.valid) return operand;
					if (!operand.type.IsPointer())
						return Error("dereference requires a pointer", left_node);
					return { operand.value, true, operand.type.DereferenceType(), true };
				}
				if (!StrCompare(operation, "OP@PREPOSI")) {
					LoweredValue operand = ToRValue(LowerExpression(left_node), left_node);
					if (!operand.valid) return operand;
					if (!operand.type.IsInteger())
						return Error("unary plus requires an integer", left_node);
					return ConvertInteger(operand, PromoteType(operand.type), left_node);
				}
				if (!StrCompare(operation, "OP@PRENEGA")) {
					LoweredValue operand = ToRValue(LowerExpression(left_node), left_node);
					if (!operand.valid) return operand;
					FrontendCType arithmetic_type = PromoteType(operand.type);
					operand = ConvertInteger(operand, arithmetic_type, left_node);
					if (!operand.valid) return operand;
					Value zero = EmitConst(0, arithmetic_type);
					Value value = EmitResult(Opcode::Sub, CloverType(arithmetic_type),
						zero, operand.value);
					return { value, !failed, arithmetic_type, false };
				}

				if (!StrCompare(operation, "OP@ASSIGN")) {
					if (!right_node) return Error("assignment requires two operands", node);
					LoweredValue left = LowerExpression(left_node);
					if (!left.valid) return left;
					if (!left.is_lvalue) return Error("assignment requires an lvalue", left_node);
					LoweredValue right = ToRValue(LowerExpression(right_node), right_node);
					if (!right.valid) return right;
					if (left.type.IsInteger())
						right = ConvertInteger(right, left.type, right_node);
					else if (!left.type.IsSame(right.type))
						return Error("incompatible assignment types", node);
					if (!right.valid) return right;
					EmitStore(right.value, left.value);
					return right;
				}

				if (!right_node) return Error("binary operator requires two operands", node);
				LoweredValue left = LowerExpression(left_node);
				LoweredValue right = LowerExpression(right_node);
				if (!left.valid || !right.valid) return {};

				if (!StrCompare(operation, "OP@ARIADD") ||
					!StrCompare(operation, "OP@ARISUB")) {
					left = ToRValue(left, left_node);
					right = ToRValue(right, right_node);
					bool subtract = !StrCompare(operation, "OP@ARISUB");
					if (left.type.IsPointer() && right.type.IsInteger()) {
						FrontendCType index_type = PromoteType(right.type);
						right = ConvertInteger(right, index_type, right_node);
						Value index = right.value;
						if (subtract) {
							Value zero = EmitConst(0, index_type);
							index = EmitResult(Opcode::Sub, CloverType(index_type), zero, index);
						}
						Value value = EmitPtrAdd(left.value, index, left.type);
						return { value, !failed, left.type, false };
					}
					if (!subtract && left.type.IsInteger() && right.type.IsPointer()) {
						FrontendCType index_type = PromoteType(left.type);
						left = ConvertInteger(left, index_type, left_node);
						Value index = left.value;
						Value value = EmitPtrAdd(right.value, index, right.type);
						return { value, !failed, right.type, false };
					}
					if (left.type.IsPointer() || right.type.IsPointer())
						return Error("invalid operands to pointer arithmetic", node);
				}

				left = ToRValue(left, left_node);
				right = ToRValue(right, right_node);
				if (!left.valid || !right.valid || !left.type.IsInteger() ||
					!right.type.IsInteger()) return Error("expected integer operands", node);
				FrontendCType arithmetic_type = CommonIntegerType(left.type, right.type);
				left = ConvertInteger(left, arithmetic_type, left_node);
				right = ConvertInteger(right, arithmetic_type, right_node);
				if (!left.valid || !right.valid) return {};
				Value lhs = left.value;
				Value rhs = right.value;

				Opcode opcode;
				if (!StrCompare(operation, "OP@ARIADD")) opcode = Opcode::Add;
				else if (!StrCompare(operation, "OP@ARISUB")) opcode = Opcode::Sub;
				else if (!StrCompare(operation, "OP@ARIMUL")) opcode = Opcode::Mul;
				else if (!StrCompare(operation, "OP@ARIDIV"))
					opcode = arithmetic_type.is_signed ? Opcode::SDiv : Opcode::UDiv;
				else if (!StrCompare(operation, "OP@ARIREM"))
					opcode = arithmetic_type.is_signed ? Opcode::SRem : Opcode::URem;
				else if (!StrCompare(operation, "OP@BITAND")) opcode = Opcode::And;
				else if (!StrCompare(operation, "OP@BITWOR")) opcode = Opcode::Or;
				else if (!StrCompare(operation, "OP@BITXOR")) opcode = Opcode::Xor;
				else if (!StrCompare(operation, "OP@BITSHL")) opcode = Opcode::Shl;
				else if (!StrCompare(operation, "OP@BITSHR"))
					opcode = arithmetic_type.is_signed ? Opcode::AShr : Opcode::LShr;
				else {
					ICmpPredicate predicate;
					if (!StrCompare(operation, "OP@JEQUAL")) predicate = ICmpPredicate::EQ;
					else if (!StrCompare(operation, "OP@JNOTEQ")) predicate = ICmpPredicate::NE;
					else if (!StrCompare(operation, "OP@JBELOW")) predicate =
						arithmetic_type.is_signed ? ICmpPredicate::SLT : ICmpPredicate::ULT;
					else if (!StrCompare(operation, "OP@JBEEQU")) predicate =
						arithmetic_type.is_signed ? ICmpPredicate::SLE : ICmpPredicate::ULE;
					else if (!StrCompare(operation, "OP@JGREAT")) predicate =
						arithmetic_type.is_signed ? ICmpPredicate::SGT : ICmpPredicate::UGT;
					else if (!StrCompare(operation, "OP@JGREQU")) predicate =
						arithmetic_type.is_signed ? ICmpPredicate::SGE : ICmpPredicate::UGE;
					else return Error("unsupported operator", node);

					Instruction instruction;
					instruction.opcode = Opcode::ICmp;
					instruction.type = CloverType(arithmetic_type);
					instruction.has_result = true;
					instruction.result = NewValue(Type::I1);
					instruction.operands.Append(lhs);
					instruction.operands.Append(rhs);
					instruction.predicate = predicate;
					CurrentBlock().instructions.Append(instruction);
					return { instruction.result, true, IntType(), false };
				}

				Value value = EmitResult(opcode, CloverType(arithmetic_type), lhs, rhs);
				return { value, true, arithmetic_type, false };
			}

		public:
			// The expression frontend currently requires one entry block.
			FunctionBuilder(Function& function,
				const uni::Vector<FrontendCFunction>& functions,
				const uni::Vector<FrontendCGlobal>& globals,
				const FrontendCType& function_return_type,
				const FrontendCTargetInfo& target_info,
				const FrontendCTypeInfo& semantic_types,
				const FrontendCDiagnostics& diagnostic_output,
				const FrontendCDebugInfo* debug_information)
				: function(&function), return_type(function_return_type),
					next_value(function.parameters.Count()),
					visible_functions(&functions), visible_globals(&globals),
					debug_info(debug_information),
					target(&target_info), type_info(&semantic_types),
					diagnostics(&diagnostic_output)
			{
				function.blocks.Append(BasicBlock());
				function.blocks[0].name = "entry";
			}

			// Translate a number, identifier, grouping node, or operator node.
			LoweredValue LowerExpression(uni::Nnode* node) {
				using namespace uni;
				if (!node) return Error("missing expression", node);
				switch (node->type) {
				case tok_number: {
					stdsint immediate;
					if (!ParseIntegerLiteral(node->addr, immediate))
						return Error("invalid integer literal", node);
					return { EmitConst(immediate), true };
				}
				case tok_identy: {
					LocalValue* local = FindLocal(node->addr);
					if (local) return { local->address, true, local->type, true };
					const FrontendCGlobal* global = FindGlobal(node->addr);
					if (!global) return Error("use of undeclared identifier", node);
					return { EmitGlobalAddress(*global), true, global->type, true };
				}
				case tok_func:
					if (node->addr && !StrCompare(node->addr, "sizeof"))
						return LowerSizeof(node->subf);
					if (node->addr && !StrCompare(node->addr, "operator[]"))
						return LowerSubscript(node);
					if (node->addr && StrLength(node->addr) >= 3 &&
						!(node->addr[0] == 'O' && node->addr[1] == 'P' &&
						node->addr[2] == '@')) return LowerCall(node);
					if (node->addr && StrLength(node->addr) < 3) return LowerCall(node);
					return LowerOperator(node);
				default:
					return Error("unsupported syntax node", node);
				}
			}

			bool EmitReturnExpression(uni::Nnode* expression) {
				if (!expression)
					return Error("return requires a value", expression).valid;
				LoweredValue lowered = LowerExpression(expression);
				if (!lowered.valid) return false;
				if (!return_type.IsInteger())
					return Error("unsupported function return type", expression).valid;
				lowered = ConvertInteger(lowered, return_type, expression);
				if (!lowered.valid) return false;
				Instruction instruction;
				instruction.opcode = Opcode::Ret;
				instruction.type = CloverType(return_type);
				instruction.operands.Append(lowered.value);
				CurrentBlock().instructions.Append(instruction);
				return true;
			}

			bool BeginIf(uni::Nnode* condition_node, const uni::Nnode* source_statement,
				stduint& then_block, stduint& else_block, uni::String& end_block) {
				stduint first_instruction = CurrentBlock().instructions.Count();
				LoweredValue lowered = LowerExpression(condition_node);
				if (!lowered.valid) return false;
				Value condition = AsCondition(lowered, condition_node);
				if (failed) return false;
				AttachSource(first_instruction, source_statement);
				then_block = NewBlock("if.then");
				else_block = NewBlock("if.else");
				end_block = NewBlockName("if.end");
				EmitJump(condition, then_block, else_block);
				current_block = then_block;
				return true;
			}

			void EndIfBranch(const uni::String& end_block) {
				if (!BlockTerminated()) EmitJump(end_block);
			}

			void AppendStatementAtoms(uni::Nnode* node,
				uni::Vector<StatementAtom>& atoms) {
				for (; node; node = node->next) {
					if (node->type == tok_statement) {
						for (uni::Nnode* child = node->subf; child; child = child->next) {
							StatementAtom atom;
							atom.node = child;
							atom.source = node;
							atoms.Append(atom);
						}
						continue;
					}
					StatementAtom atom;
					atom.node = node;
					atom.source = node;
					atoms.Append(atom);
				}
			}

			uni::Nnode* ForClause(uni::Nnode* clause) {
				return clause && clause->type == tok_statement ? clause->subf : nullptr;
			}

			bool LowerWhile(uni::Nnode* node, const uni::Nnode* source_statement,
				uni::Vector<StatementAtom>& atoms, stduint& index) {
				if (!node->subf) return Error("while requires a condition", node).valid;
				stduint condition_block = NewBlock("while.cond");
				stduint body_block = NewBlock("while.body");
				uni::String end_name = NewBlockName("while.end");
				EmitJump(condition_block);

				current_block = condition_block;
				stduint first_instruction = CurrentBlock().instructions.Count();
				LoweredValue lowered = LowerExpression(node->subf);
				if (!lowered.valid) return false;
				Value condition = AsCondition(lowered, node->subf);
				if (failed) return false;
				AttachSource(first_instruction, source_statement);
				EmitJump(condition, function->blocks[body_block].name, end_name);

				current_block = body_block;
				if (!LowerStatement(atoms, index)) return false;
				if (!BlockTerminated()) EmitJump(condition_block);
				current_block = AppendBlock(end_name);
				return true;
			}

			bool LowerFor(uni::Nnode* node, const uni::Nnode* source_statement,
				uni::Vector<StatementAtom>& atoms, stduint& index) {
				uni::Nnode* init_clause = node->subf;
				uni::Nnode* condition_clause = init_clause ? init_clause->next : nullptr;
				uni::Nnode* step_clause = condition_clause ? condition_clause->next : nullptr;
				if (!init_clause || !condition_clause ||
					init_clause->type != tok_statement ||
					condition_clause->type != tok_statement ||
					(step_clause && (step_clause->type != tok_statement || step_clause->next)))
					return Error("invalid for clauses", node).valid;

				uni::Nnode* init = ForClause(init_clause);
				uni::Nnode* condition_node = ForClause(condition_clause);
				uni::Nnode* step = ForClause(step_clause);
				stduint source_instruction = CurrentBlock().instructions.Count();
				if (init && !LowerExpression(init).valid) return false;
				AttachSource(source_instruction, source_statement);

				stduint condition_block = NewBlock("for.cond");
				stduint body_block = NewBlock("for.body");
				uni::String step_name = NewBlockName("for.step");
				uni::String end_name = NewBlockName("for.end");
				EmitJump(condition_block);

				current_block = condition_block;
				if (condition_node) {
					stduint first_instruction = CurrentBlock().instructions.Count();
					LoweredValue lowered = LowerExpression(condition_node);
					if (!lowered.valid) return false;
					Value condition = AsCondition(lowered, condition_node);
					if (failed) return false;
					AttachSource(first_instruction, source_statement);
					EmitJump(condition, function->blocks[body_block].name, end_name);
				}
				else EmitJump(body_block);

				current_block = body_block;
				if (!LowerStatement(atoms, index)) return false;
				if (!BlockTerminated()) EmitJump(step_name);

				current_block = AppendBlock(step_name);
				if (step && !LowerExpression(step).valid) return false;
				if (!BlockTerminated()) EmitJump(condition_block);
				current_block = AppendBlock(end_name);
				return true;
			}

			bool LowerStatement(uni::Vector<StatementAtom>& atoms, stduint& index) {
				if (index >= atoms.Count())
					return Error("if requires a statement", nullptr).valid;
				StatementAtom atom = atoms.begin()[index++];
				uni::Nnode* node = atom.node;
				if (!node) return Error("missing statement", node).valid;

				if (node->type == ToToken(FrontendCToken::Empty)) return true;
				if (node->type == ToToken(FrontendCToken::Else))
					return Error("else has no matching if", node).valid;
				if (node->type == ToToken(FrontendCToken::If)) {
					if (!node->subf) return Error("if requires a condition", node).valid;
					stduint then_block, else_block;
					uni::String end_block;
					if (!BeginIf(node->subf, node,
						then_block, else_block, end_block)) return false;
					if (!LowerStatement(atoms, index)) return false;
					EndIfBranch(end_block);

					current_block = else_block;
					if (index < atoms.Count() &&
						atoms.begin()[index].node->type == ToToken(FrontendCToken::Else)) {
						index++;
						if (!LowerStatement(atoms, index)) return false;
					}
					EndIfBranch(end_block);
					current_block = AppendBlock(end_block);
					return true;
				}
				if (node->type == ToToken(FrontendCToken::While))
					return LowerWhile(node, atom.source, atoms, index);
				if (node->type == ToToken(FrontendCToken::For))
					return LowerFor(node, atom.source, atoms, index);

				if (node->type == tok_block) return LowerBlock(node);
				stduint first_instruction = CurrentBlock().instructions.Count();
				if (node->type == ToToken(FrontendCToken::Declaration)) {
					bool success = LowerDeclarator(node->subf, node);
					if (success) AttachSource(first_instruction, atom.source);
					return success;
				}
				if (node->type == ToToken(FrontendCToken::Return)) {
					bool success;
					if (node->subf) success = EmitReturnExpression(node->subf);
					else if (index < atoms.Count())
						success = EmitReturnExpression(atoms.begin()[index++].node);
					else success = Error("return requires a value", node).valid;
					if (success) AttachSource(first_instruction, atom.source);
					return success;
				}
				if (node->type == ToToken(FrontendCToken::ReturnVoid)) {
					if (!return_type.IsVoid())
						return Error("non-void function requires a return value", node).valid;
					Instruction instruction;
					instruction.opcode = Opcode::Ret;
					instruction.type = Type::Void;
					CurrentBlock().instructions.Append(instruction);
					AttachSource(first_instruction, atom.source);
					return true;
				}

				bool success = LowerExpression(node).valid;
				if (success) AttachSource(first_instruction, atom.source);
				return success;
			}

			// Lower every statement in source order, including statements after return.
			bool LowerBlock(uni::Nnode* block) {
				if (!block || block->type != tok_block) {
					Error("expected block", block);
					return false;
				}
				uni::Vector<StatementAtom> atoms;
				AppendStatementAtoms(block->subf, atoms);
				for (stduint index = 0; index < atoms.Count();) {
					if (!LowerStatement(atoms, index) || failed) return false;
				}
				LeaveScope(block);
				return true;
			}

			bool InitializeParameters(uni::Nnode* parameter_node,
				const uni::Nnode* body) {
				stduint index = 0;
				for (; parameter_node; parameter_node = parameter_node->next, index++) {
					if (index >= function->parameters.Count() || !parameter_node->addr)
						return Error("invalid function parameter", parameter_node).valid;
					const FrontendCType* parameter_type = type_info->Find(parameter_node);
					if (!parameter_type)
						return Error("missing function parameter type", parameter_node).valid;
					FrontendCType type = *parameter_type;
					if (!PredeclareLocal(parameter_node->addr, type,
						parameter_node, body)) return false;
					LocalValue* local = ActivateLocal(parameter_node->addr, parameter_node);
					if (!local) return Error("invalid function parameter", parameter_node).valid;
					EmitStore(function->parameters.begin()[index].value, local->address);
				}
				return index == function->parameters.Count();
			}

			// Lower one function body without requiring an explicit return statement.
			bool Lower(uni::Nnode* body, uni::Nnode* parameters) {
				if (!InitializeParameters(parameters, body)) return false;
				if (!PredeclareBlock(body)) return false;
				if (!LowerBlock(body)) return false;
				if (return_type.IsVoid() && !BlockTerminated()) {
					Instruction instruction;
					instruction.opcode = Opcode::Ret;
					instruction.type = Type::Void;
					CurrentBlock().instructions.Append(instruction);
				}
				return true;
			}
		};

		bool ParseGlobalDeclaratorNode(uni::Nnode* node, uni::Nnode*& identifier,
			FrontendCType& type) {
			if (!node) return false;
			if (node->type == tok_func && node->addr &&
				!StrCompare(node->addr, "OP@PREMEMB") && node->subf &&
				!node->subf->next) {
				if (!ParseGlobalDeclaratorNode(node->subf, identifier, type)) return false;
				type = type.AddressType();
				return true;
			}
			if (node->type == tok_func && node->addr &&
				!StrCompare(node->addr, "operator[]")) {
				uni::Nnode* declarator = node->subf;
				uni::Nnode* dimension = declarator ? declarator->next : nullptr;
				stdsint count;
				if (!declarator || !dimension || dimension->next ||
					dimension->type != tok_number || !dimension->addr ||
					!ParseIntegerLiteral(dimension->addr, count) || count <= 0 ||
					!ParseGlobalDeclaratorNode(declarator, identifier, type)) return false;
				return type.AppendArrayDimension((stduint)count);
			}
			if (node->type != tok_identy || !node->addr) return false;
			identifier = node;
			return true;
		}

		bool ParseGlobalConstant(uni::Nnode* node, stdsint& value) {
			if (!node) return false;
			if (node->type == tok_number) return ParseIntegerLiteral(node->addr, value);
			if (node->type == tok_func && node->addr && node->subf &&
				!node->subf->next && !StrCompare(node->addr, "OP@PRENEGA")) {
				if (!ParseGlobalConstant(node->subf, value)) return false;
				value = -value;
				return true;
			}
			return false;
		}

		bool LowerGlobalDeclarator(uni::Nnode* node, const FrontendCType& base_type,
			Module& module, uni::Vector<FrontendCGlobal>& globals,
			const FrontendCTargetInfo& target, const FrontendCDiagnostics& diagnostics,
			const uni::Nnode* declaration) {
			if (node && node->type == tok_func && node->addr &&
				!StrCompare(node->addr, "OP@COMMA")) {
				for (uni::Nnode* child = node->subf; child; child = child->next)
					if (!LowerGlobalDeclarator(child, base_type, module, globals,
						target, diagnostics, declaration)) return false;
				return true;
			}
			uni::Nnode* declarator = node;
			uni::Nnode* initializer_node = nullptr;
			if (node && node->type == tok_func && node->addr &&
				!StrCompare(node->addr, "OP@ASSIGN")) {
				declarator = node->subf;
				initializer_node = declarator ? declarator->next : nullptr;
				if (!initializer_node || initializer_node->next) {
					diagnostics.Report(DiagnosticStage::Lowering,
						"invalid global initializer", node);
					return false;
				}
			}
			FrontendCType type = base_type;
			uni::Nnode* identifier = nullptr;
			if (!ParseGlobalDeclaratorNode(declarator, identifier, type)) {
				diagnostics.Report(DiagnosticStage::Lowering,
					"invalid global declarator", declarator ? declarator : declaration);
				return false;
			}
			for (const FrontendCGlobal& previous : globals)
				if (!StrCompare(previous.name.reference(), identifier->addr)) {
					diagnostics.Report(DiagnosticStage::Semantic,
						"redefinition of global variable", identifier);
					return false;
				}
			Global global;
			global.name = identifier->addr;
			global.type = CloverTypeFor(type, target);
			global.element_count = (stdsint)type.StorageElementCount();
			if (initializer_node && !ParseGlobalConstant(initializer_node, global.initializer)) {
				diagnostics.Report(DiagnosticStage::Lowering,
					"global initializer must be an integer constant", initializer_node);
				return false;
			}
			module.globals.Append(global);
			FrontendCGlobal frontend_global;
			frontend_global.name = identifier->addr;
			frontend_global.type = type;
			globals.Append(frontend_global);
			return true;
		}

	}

	bool LowerFrontendC(uni::Nchain& syntax_tree, Module& module,
		const FrontendCDiagnostics& diagnostics, const FrontendCTargetInfo& target,
		const FrontendCTypeInfo& type_info,
		const FrontendCDebugInfo* debug_info) {
		module.globals.Clear();
		module.functions.Clear();
		uni::Vector<FrontendCFunction> visible_functions;
		uni::Vector<FrontendCGlobal> visible_globals;
		for (uni::Nnode* node = syntax_tree.Root(); node;) {
			if (node->type == ToToken(FrontendCToken::GlobalDeclaration)) {
				const FrontendCType* base_type = type_info.Find(node);
				if (!base_type || !LowerGlobalDeclarator(node->subf, *base_type,
					module, visible_globals, target, diagnostics, node)) return false;
				node = node->next;
				continue;
			}
			if (node->type != ToToken(FrontendCToken::Function) || !node->addr ||
				!node->next || node->next->type != tok_block) {
				diagnostics.Report(DiagnosticStage::Lowering,
					"expected analyzed function definition", node);
				return false;
			}

			uni::Nnode* body = node->next;
			for (const FrontendCFunction& previous : visible_functions)
				if (!StrCompare(previous.name.reference(), node->addr)) {
					diagnostics.Report(DiagnosticStage::Semantic,
						"redefinition of function", node);
					return false;
				}
			for (const FrontendCGlobal& previous : visible_globals)
				if (!StrCompare(previous.name.reference(), node->addr)) {
					diagnostics.Report(DiagnosticStage::Semantic,
						"function conflicts with global variable", node);
					return false;
				}

			Function function;
			function.name = node->addr;
			const FrontendCType* function_return_type = type_info.Find(node);
			if (!function_return_type) {
				diagnostics.Report(DiagnosticStage::Semantic,
					"missing function return type", node);
				return false;
			}
			function.return_type = CloverTypeFor(*function_return_type, target);
			FrontendCFunction signature;
			signature.name = node->addr;
			signature.return_type = *function_return_type;
			for (uni::Nnode* parameter_node = node->subf;
				parameter_node; parameter_node = parameter_node->next) {
				if (parameter_node->type != ToToken(FrontendCToken::Parameter) ||
					!parameter_node->addr) {
					diagnostics.Report(DiagnosticStage::Semantic,
						"invalid function parameter", parameter_node);
					return false;
				}
				const FrontendCType* parameter_type = type_info.Find(parameter_node);
				if (!parameter_type) {
					diagnostics.Report(DiagnosticStage::Semantic,
						"missing function parameter type", parameter_node);
					return false;
				}
				signature.parameter_types.Append(*parameter_type);
				Parameter parameter;
				parameter.name = parameter_node->addr;
				parameter.value.type = CloverTypeFor(*parameter_type, target);
				parameter.value.id = function.parameters.Count();
				function.parameters.Append(parameter);
			}
			visible_functions.Append(signature);
			FunctionBuilder builder(function, visible_functions, visible_globals,
				*function_return_type,
				target, type_info, diagnostics, debug_info);
			if (!builder.Lower(body, node->subf)) return false;
			module.functions.Append(function);
			node = body->next;
		}
		return true;
	}

}

bool uni::lango::frontend_c_detail::Lower(uni::Nchain& syntax_tree,
	clover::Module& module, const FrontendCDiagnostics& diagnostics,
	const FrontendCTargetInfo& target, const FrontendCTypeInfo& type_info,
	const FrontendCDebugInfo* debug_info) {
	return clover::LowerFrontendC(syntax_tree, module, diagnostics, target,
		type_info, debug_info);
}
