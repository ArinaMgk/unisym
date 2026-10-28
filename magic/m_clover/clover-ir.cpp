// ASCII C++11 TAB4 LF
// Docutitle: Magice Clover IR Lowering
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

#include "../include/clover-ir.hpp"

#include "../../inc/cpp/parse.hpp"

namespace clover {

	namespace {

		// Result returned while recursively lowering one AST expression.
		struct LoweredValue {
			Value value;
			bool valid = false;
		};

		// Associates a source identifier with the Ptr value produced by Local.
		struct LocalValue {
			uni::String name;
			Value address;
		};

		// Lowers the current expression-only AST into one Clover function.
		class FunctionBuilder {
		private:
			BasicBlock* entry = nullptr;
			unsigned next_value = 0;
			uni::Vector<LocalValue> locals;
			bool failed = false;

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
				entry->instructions.Append(instruction);
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
				entry->instructions.Append(instruction);
				return instruction.result;
			}

			// Store is side-effecting and therefore has no SSA result.
			void EmitStore(Value value, Value address) {
				Instruction instruction;
				instruction.opcode = Opcode::Store;
				instruction.type = value.type;
				instruction.operands.Append(value);
				instruction.operands.Append(address);
				entry->instructions.Append(instruction);
			}

			// Promote a comparison result before using it as an integer operand.
			Value AsI64(Value value) {
				if (value.type == Type::I64) return value;
				if (value.type == Type::I1) {
					Instruction instruction;
					instruction.opcode = Opcode::ZExt;
					instruction.type = Type::I64;
					instruction.has_result = true;
					instruction.result = NewValue(Type::I64);
					instruction.operands.Append(value);
					entry->instructions.Append(instruction);
					return instruction.result;
				}
				failed = true;
				plogerro("Clover IR: expected integer value");
				return value;
			}

			// Materialize a source integer literal as a Const instruction.
			Value EmitConst(stdsint immediate) {
				Instruction instruction;
				instruction.opcode = Opcode::Const;
				instruction.type = Type::I64;
				instruction.has_result = true;
				instruction.result = NewValue(Type::I64);
				instruction.immediate = immediate;
				entry->instructions.Append(instruction);
				return instruction.result;
			}

			// Return an existing local address or declare it on first use.
			Value LocalAddress(const char* name) {
				for (LocalValue& local : locals)
					if (!StrCompare(local.name.reference(), name)) return local.address;

				Instruction instruction;
				instruction.opcode = Opcode::Local;
				instruction.type = Type::I64;
				instruction.has_result = true;
				instruction.result = NewValue(Type::Ptr);
				instruction.name = name;
				entry->instructions.Append(instruction);

				LocalValue local;
				local.name = name;
				local.address = instruction.result;
				locals.Append(local);
				return instruction.result;
			}

			// Declare every source identifier before emitting executable instructions,
			// so all Local instructions stay at the beginning of the entry block.
			void CollectLocals(uni::Nnode* node) {
				for (; node; node = node->next) {
					if (node->type == tok_identy && node->addr) LocalAddress(node->addr);
					if (node->subf) CollectLocals(node->subf);
				}
			}

			// Record a lowering error and return an invalid expression result.
			LoweredValue Error(const char* message, const uni::Nnode* node) {
				failed = true;
				plogerro("Clover IR: %s%s%s", message,
					node && node->addr ? " " : "", node && node->addr ? node->addr : "");
				return {};
			}

			// Translate one operator and recursively lower its operands.
			LoweredValue LowerOperator(uni::Nnode* node) {
				using namespace uni;
				if (!node->addr && node->subf) return LowerExpression(node->subf);
				if (!node->addr || !node->subf) return Error("bad operator", node);

				Nnode* left_node = node->subf;
				Nnode* right_node = left_node->next;
				const char* operation = node->addr;

				if (!StrCompare(operation, "OP@PREPOSI")) return LowerExpression(left_node);
				if (!StrCompare(operation, "OP@PRENEGA")) {
					LoweredValue operand = LowerExpression(left_node);
					if (!operand.valid) return operand;
					Value zero = EmitConst(0);
					Value value = EmitResult(Opcode::Sub, Type::I64, zero, AsI64(operand.value));
					return { value, !failed };
				}

				if (!StrCompare(operation, "OP@ASSIGN")) {
					if (!right_node || left_node->type != tok_identy || !left_node->addr)
						return Error("assignment requires an identifier", node);
					LoweredValue right = LowerExpression(right_node);
					if (!right.valid) return right;
					Value value = AsI64(right.value);
					EmitStore(value, LocalAddress(left_node->addr));
					return { value, !failed };
				}

				if (!right_node) return Error("binary operator requires two operands", node);
				LoweredValue left = LowerExpression(left_node);
				LoweredValue right = LowerExpression(right_node);
				if (!left.valid || !right.valid) return {};
				Value lhs = AsI64(left.value);
				Value rhs = AsI64(right.value);
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
					instruction.type = Type::I64;
					instruction.has_result = true;
					instruction.result = NewValue(Type::I1);
					instruction.operands.Append(lhs);
					instruction.operands.Append(rhs);
					instruction.predicate = predicate;
					entry->instructions.Append(instruction);
					return { instruction.result, true };
				}

				Value value = EmitResult(opcode, Type::I64, lhs, rhs);
				return { value, true };
			}

		public:
			// The expression frontend currently requires one entry block.
			FunctionBuilder(Function& function)
			{
				function.blocks.Append(BasicBlock());
				entry = &function.blocks[0];
				entry->name = "entry";
			}

			// Translate a number, identifier, grouping node, or operator node.
			LoweredValue LowerExpression(uni::Nnode* node) {
				using namespace uni;
				if (!node) return Error("missing expression", node);
				switch (node->type) {
				case tok_number:
					return { EmitConst(atoins(node->addr)), true };
				case tok_identy: {
					Value address = LocalAddress(node->addr);
					Value value = EmitResult(Opcode::Load, Type::I64, address);
					return { value, true };
				}
				case tok_func:
					return LowerOperator(node);
				default:
					return Error("unsupported syntax node", node);
				}
			}

			// Lower all expression statements and return the final expression value.
			bool Lower(uni::Nchain& syntax_tree) {
				CollectLocals(syntax_tree.Root());
				LoweredValue last;
				for (uni::Nnode* statement = syntax_tree.Root(); statement; statement = statement->next) {
					if (statement->type != tok_statement) {
						Error("expected statement", statement);
						return false;
					}
					if (statement->subf) last = LowerExpression(statement->subf);
					if (failed) return false;
				}

				if (!last.valid) last = { EmitConst(0), true };
				Value result = AsI64(last.value);
				if (failed) return false;

				Instruction instruction;
				instruction.opcode = Opcode::Ret;
				instruction.type = Type::I64;
				instruction.operands.Append(result);
				entry->instructions.Append(instruction);
				return true;
			}
		};

	}

	bool Generate(uni::Nchain& syntax_tree, Module& module) {
		module.functions.Clear();
		Function function;
		function.name = "main";
		function.return_type = Type::I64;
		FunctionBuilder builder(function);
		if (!builder.Lower(syntax_tree)) return false;
		module.functions.Append(function);
		return true;
	}

}
