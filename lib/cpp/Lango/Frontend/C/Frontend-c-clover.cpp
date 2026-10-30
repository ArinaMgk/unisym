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

		// Lowers one C function body into one Clover function.
		class FunctionBuilder {
		private:
			Function* function = nullptr;
			stduint current_block = 0;
			unsigned next_block = 0;
			unsigned next_value = 0;
			uni::Vector<LocalValue> locals;
			uni::Vector<const uni::Nnode*> emitted_sources;
			const uni::Vector<FrontendCFunction>* visible_functions = nullptr;
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
					index = EmitResult(Opcode::Mul, Type::I32, index, scale);
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

			Type CloverType(const FrontendCType& type) const {
				return type.IsPointer() ? Type::Ptr : Type::I32;
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

			// Convert a comparison result to the C99 int type when required.
			Value AsInt(Value value) {
				if (value.type == Type::I32) return value;
				if (value.type == Type::I1) {
					Instruction instruction;
					instruction.opcode = Opcode::ZExt;
					instruction.type = Type::I32;
					instruction.has_result = true;
					instruction.result = NewValue(Type::I32);
					instruction.operands.Append(value);
					CurrentBlock().instructions.Append(instruction);
					return instruction.result;
				}
				failed = true;
				diagnostics->Report(DiagnosticStage::Lowering,
					"expected integer value");
				return value;
			}

			Value AsInt(LoweredValue lowered, const uni::Nnode* node) {
				lowered = ToRValue(lowered, node);
				if (!lowered.valid || !lowered.type.IsInteger()) {
					Error("expected integer expression", node);
					return lowered.value;
				}
				return AsInt(lowered.value);
			}

			Value AsCondition(LoweredValue lowered, const uni::Nnode* node) {
				lowered = ToRValue(lowered, node);
				if (!lowered.valid || !lowered.type.IsInteger()) {
					Error("condition requires an integer expression", node);
					return lowered.value;
				}
				if (lowered.value.type == Type::I1) return lowered.value;
				Value integer = AsInt(lowered.value);
				Value zero = EmitConst(0);
				Instruction instruction;
				instruction.opcode = Opcode::ICmp;
				instruction.type = Type::I32;
				instruction.has_result = true;
				instruction.result = NewValue(Type::I1);
				instruction.operands.Append(integer);
				instruction.operands.Append(zero);
				instruction.predicate = ICmpPredicate::NE;
				CurrentBlock().instructions.Append(instruction);
				return instruction.result;
			}

			// Materialize a source integer literal as a Const instruction.
			Value EmitConst(stdsint immediate) {
				Instruction instruction;
				instruction.opcode = Opcode::Const;
				instruction.type = Type::I32;
				instruction.has_result = true;
				instruction.result = NewValue(Type::I32);
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
				FrontendCType& type) {
				type = FrontendCType();
				return ParseDeclaratorNode(node, identifier, type);
			}

			bool PredeclareDeclarator(uni::Nnode* node, const uni::Nnode* declaration,
				const uni::Nnode* scope) {
				if (!node) return false;
				if (node->type == tok_func && node->addr &&
					!StrCompare(node->addr, "OP@COMMA")) {
					for (uni::Nnode* child = node->subf; child; child = child->next)
						if (!PredeclareDeclarator(child, declaration, scope)) return false;
					return true;
				}
				uni::Nnode* declarator = node;
				if (node->type == tok_func && node->addr &&
					!StrCompare(node->addr, "OP@ASSIGN")) declarator = node->subf;
				uni::Nnode* identifier = nullptr;
				FrontendCType type;
				if (!ParseDeclarator(declarator, identifier, type)) return false;
				return PredeclareLocal(identifier->addr, type, declaration, scope);
			}

			bool PredeclareBlock(uni::Nnode* block) {
				for (uni::Nnode* node = block ? block->subf : nullptr; node; node = node->next) {
					if (node->type == ToToken(FrontendCToken::Declaration)) {
						if (!PredeclareDeclarator(node->subf, node, block)) {
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
				FrontendCType element_pointer = FrontendCType().AddressType();
				for (stduint index = 0; index < element_count; index++) {
					Value value;
					if (index < values.Count()) {
						uni::Nnode* value_node = values.begin()[index];
						LoweredValue lowered = ToRValue(LowerExpression(value_node), value_node);
						if (!lowered.valid) return false;
						if (!lowered.type.IsInteger())
							return Error("array initializer requires an integer", value_node).valid;
						value = AsInt(lowered.value);
					}
					else value = EmitConst(0);
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
				if (!ParseDeclarator(declarator, identifier, declared_type))
					return Error("invalid declarator", declarator).valid;
				LocalValue* local = ActivateLocal(identifier->addr, declaration);
				if (!local) return Error("invalid local declaration", identifier).valid;
				if (!initializer_node) return true;
				if (local->type.IsArray())
					return InitializeArray(*local, initializer_node);
				LoweredValue initializer = ToRValue(
					LowerExpression(initializer_node), initializer_node);
				if (!initializer.valid) return false;
				if (!local->type.IsSame(initializer.type))
					return Error("incompatible initializer type", initializer_node).valid;
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
					if (!argument.type.IsSame(signature->parameter_types.begin()[index]))
						return Error("incompatible function argument type", argument_node);
					instruction.operands.Append(argument.value);
				}
				instruction.has_result = true;
				instruction.result = NewValue(instruction.type);
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
				Value index = AsInt(right.value);
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
				Value one = EmitConst(1);
				Value updated;
				if (lowered.type.IsPointer()) {
					Value index = one;
					if (!increment) {
						Value zero = EmitConst(0);
						index = EmitResult(Opcode::Sub, Type::I32, zero, one);
					}
					updated = EmitPtrAdd(previous, index, lowered.type);
				}
				else updated = EmitResult(increment ? Opcode::Add : Opcode::Sub,
					Type::I32, previous, one);
				EmitStore(updated, lowered.value);
				return { return_updated ? updated : previous, true, lowered.type, false };
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
					return operand;
				}
				if (!StrCompare(operation, "OP@PRENEGA")) {
					LoweredValue operand = LowerExpression(left_node);
					if (!operand.valid) return operand;
					Value zero = EmitConst(0);
					Value value = EmitResult(Opcode::Sub, Type::I32, zero,
						AsInt(operand, left_node));
					return { value, !failed, FrontendCType(), false };
				}

				if (!StrCompare(operation, "OP@ASSIGN")) {
					if (!right_node) return Error("assignment requires two operands", node);
					LoweredValue left = LowerExpression(left_node);
					if (!left.valid) return left;
					if (!left.is_lvalue) return Error("assignment requires an lvalue", left_node);
					LoweredValue right = ToRValue(LowerExpression(right_node), right_node);
					if (!right.valid) return right;
					if (!left.type.IsSame(right.type))
						return Error("incompatible assignment types", node);
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
						Value index = AsInt(right, right_node);
						if (subtract) {
							Value zero = EmitConst(0);
							index = EmitResult(Opcode::Sub, Type::I32, zero, index);
						}
						Value value = EmitPtrAdd(left.value, index, left.type);
						return { value, !failed, left.type, false };
					}
					if (!subtract && left.type.IsInteger() && right.type.IsPointer()) {
						Value index = AsInt(left, left_node);
						Value value = EmitPtrAdd(right.value, index, right.type);
						return { value, !failed, right.type, false };
					}
					if (left.type.IsPointer() || right.type.IsPointer())
						return Error("invalid operands to pointer arithmetic", node);
				}

				Value lhs = AsInt(left, left_node);
				Value rhs = AsInt(right, right_node);
				if (failed) return {};

				Opcode opcode;
				if (!StrCompare(operation, "OP@ARIADD")) opcode = Opcode::Add;
				else if (!StrCompare(operation, "OP@ARISUB")) opcode = Opcode::Sub;
				else if (!StrCompare(operation, "OP@ARIMUL")) opcode = Opcode::Mul;
				else if (!StrCompare(operation, "OP@ARIDIV")) opcode = Opcode::SDiv;
				else if (!StrCompare(operation, "OP@ARIREM")) opcode = Opcode::SRem;
				else if (!StrCompare(operation, "OP@BITAND")) opcode = Opcode::And;
				else if (!StrCompare(operation, "OP@BITWOR")) opcode = Opcode::Or;
				else if (!StrCompare(operation, "OP@BITXOR")) opcode = Opcode::Xor;
				else if (!StrCompare(operation, "OP@BITSHL")) opcode = Opcode::Shl;
				else if (!StrCompare(operation, "OP@BITSHR")) opcode = Opcode::AShr;
				else {
					ICmpPredicate predicate;
					if (!StrCompare(operation, "OP@JEQUAL")) predicate = ICmpPredicate::EQ;
					else if (!StrCompare(operation, "OP@JNOTEQ")) predicate = ICmpPredicate::NE;
					else if (!StrCompare(operation, "OP@JBELOW")) predicate = ICmpPredicate::SLT;
					else if (!StrCompare(operation, "OP@JBEEQU")) predicate = ICmpPredicate::SLE;
					else if (!StrCompare(operation, "OP@JGREAT")) predicate = ICmpPredicate::SGT;
					else if (!StrCompare(operation, "OP@JGREQU")) predicate = ICmpPredicate::SGE;
					else return Error("unsupported operator", node);

					Instruction instruction;
					instruction.opcode = Opcode::ICmp;
					instruction.type = Type::I32;
					instruction.has_result = true;
					instruction.result = NewValue(Type::I1);
					instruction.operands.Append(lhs);
					instruction.operands.Append(rhs);
					instruction.predicate = predicate;
					CurrentBlock().instructions.Append(instruction);
					return { instruction.result, true, FrontendCType(), false };
				}

				Value value = EmitResult(opcode, Type::I32, lhs, rhs);
				return { value, true, FrontendCType(), false };
			}

		public:
			// The expression frontend currently requires one entry block.
			FunctionBuilder(Function& function,
				const uni::Vector<FrontendCFunction>& functions,
				const FrontendCDiagnostics& diagnostic_output,
				const FrontendCDebugInfo* debug_information)
				: function(&function), next_value(function.parameters.Count()),
					visible_functions(&functions), debug_info(debug_information),
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
				case tok_number:
					return { EmitConst(atoins(node->addr)), true };
				case tok_identy: {
					LocalValue* local = FindLocal(node->addr);
					if (!local) return Error("use of undeclared identifier", node);
					return { local->address, true, local->type, true };
				}
				case tok_func:
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
				Value result = AsInt(lowered, expression);
				if (failed) return false;
				Instruction instruction;
				instruction.opcode = Opcode::Ret;
				instruction.type = Type::I32;
				instruction.operands.Append(result);
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
					FrontendCType type;
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
				return LowerBlock(body);
			}
		};

	}

	bool LowerFrontendC(uni::Nchain& syntax_tree, Module& module,
		const FrontendCDiagnostics& diagnostics,
		const FrontendCDebugInfo* debug_info) {
		module.functions.Clear();
		uni::Vector<FrontendCFunction> visible_functions;
		for (uni::Nnode* node = syntax_tree.Root(); node;) {
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

			Function function;
			function.name = node->addr;
			function.return_type = Type::I32;
			FrontendCFunction signature;
			signature.name = node->addr;
			for (uni::Nnode* parameter_node = node->subf;
				parameter_node; parameter_node = parameter_node->next) {
				if (parameter_node->type != ToToken(FrontendCToken::Parameter) ||
					!parameter_node->addr) {
					diagnostics.Report(DiagnosticStage::Semantic,
						"invalid function parameter", parameter_node);
					return false;
				}
				FrontendCType parameter_type;
				signature.parameter_types.Append(parameter_type);
				Parameter parameter;
				parameter.name = parameter_node->addr;
				parameter.value.type = Type::I32;
				parameter.value.id = function.parameters.Count();
				function.parameters.Append(parameter);
			}
			visible_functions.Append(signature);
			FunctionBuilder builder(function, visible_functions, diagnostics, debug_info);
			if (!builder.Lower(body, node->subf)) return false;
			module.functions.Append(function);
			node = body->next;
		}
		return true;
	}

}

bool uni::lango::frontend_c_detail::Lower(uni::Nchain& syntax_tree,
	clover::Module& module, const FrontendCDiagnostics& diagnostics,
	const FrontendCDebugInfo* debug_info) {
	return clover::LowerFrontendC(syntax_tree, module, diagnostics, debug_info);
}
