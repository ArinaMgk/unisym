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
		};

		// Associates a source identifier with the Ptr value produced by Local.
		struct LocalValue {
			uni::String name;
			Value address;
		};

		// One C statement atom after transparent tok_statement wrappers are removed.
		struct StatementAtom {
			uni::Nnode* node = nullptr;
			const uni::Nnode* source = nullptr;
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

			// Store is side-effecting and therefore has no SSA result.
			void EmitStore(Value value, Value address) {
				Instruction instruction;
				instruction.opcode = Opcode::Store;
				instruction.type = value.type;
				instruction.operands.Append(value);
				instruction.operands.Append(address);
				CurrentBlock().instructions.Append(instruction);
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
					CurrentBlock().instructions.Append(instruction);
					return instruction.result;
				}
				failed = true;
				diagnostics->Report(DiagnosticStage::Lowering,
					"expected integer value");
				return value;
			}

			Value AsCondition(Value value) {
				if (value.type == Type::I1) return value;
				Value integer = AsI64(value);
				Value zero = EmitConst(0);
				Instruction instruction;
				instruction.opcode = Opcode::ICmp;
				instruction.type = Type::I64;
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
				instruction.type = Type::I64;
				instruction.has_result = true;
				instruction.result = NewValue(Type::I64);
				instruction.immediate = immediate;
				CurrentBlock().instructions.Append(instruction);
				return instruction.result;
			}

			// Return an existing local address or declare it on first use.
			Value LocalAddress(const char* name, const uni::Nnode* statement = nullptr) {
				for (LocalValue& local : locals)
					if (!StrCompare(local.name.reference(), name)) return local.address;

				Instruction instruction;
				instruction.opcode = Opcode::Local;
				instruction.type = Type::I64;
				instruction.has_result = true;
				instruction.result = NewValue(Type::Ptr);
				instruction.name = name;
				AttachSource(instruction, statement);
				EntryBlock().instructions.Append(instruction);

				LocalValue local;
				local.name = name;
				local.address = instruction.result;
				locals.Append(local);
				return instruction.result;
			}

			// Declare every source identifier before executable instructions so all
			// Local instructions stay at the beginning of the entry block.
			void CollectLocals(uni::Nnode* node, const uni::Nnode* statement = nullptr) {
				for (; node; node = node->next) {
					if (node->type == tok_statement ||
						node->type == ToToken(FrontendCToken::Return)) {
						CollectLocals(node->subf, node);
						continue;
					}
					if (node->type == tok_identy && node->addr &&
						!IsKeyword(node, "return") && !IsKeyword(node, "else"))
						LocalAddress(node->addr, statement);
					if (node->subf) CollectLocals(node->subf, statement);
				}
			}

			// Record a lowering error and return an invalid expression result.
			LoweredValue Error(const char* message, const uni::Nnode* node) {
				failed = true;
				diagnostics->Report(DiagnosticStage::Lowering, message, node);
				return {};
			}

			LoweredValue LowerIncrement(uni::Nnode* operand, bool increment,
				bool return_updated) {
				if (!operand || operand->type != tok_identy || !operand->addr)
					return Error("increment requires an identifier", operand);
				Value address = LocalAddress(operand->addr);
				Value previous = EmitResult(Opcode::Load, Type::I64, address);
				Value one = EmitConst(1);
				Value updated = EmitResult(increment ? Opcode::Add : Opcode::Sub,
					Type::I64, previous, one);
				EmitStore(updated, address);
				return { return_updated ? updated : previous, true };
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
					CurrentBlock().instructions.Append(instruction);
					return { instruction.result, true };
				}

				Value value = EmitResult(opcode, Type::I64, lhs, rhs);
				return { value, true };
			}

		public:
			// The expression frontend currently requires one entry block.
			FunctionBuilder(Function& function, const FrontendCDiagnostics& diagnostic_output,
				const FrontendCDebugInfo* debug_information)
				: function(&function), debug_info(debug_information),
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

			bool EmitReturnExpression(uni::Nnode* expression) {
				if (!expression)
					return Error("return requires a value", expression).valid;
				LoweredValue lowered = LowerExpression(expression);
				if (!lowered.valid) return false;
				Value result = AsI64(lowered.value);
				if (failed) return false;
				Instruction instruction;
				instruction.opcode = Opcode::Ret;
				instruction.type = Type::I64;
				instruction.operands.Append(result);
				CurrentBlock().instructions.Append(instruction);
				return true;
			}

			bool BeginIf(uni::Nnode* condition_node, const uni::Nnode* source_statement,
				stduint& then_block, stduint& else_block, uni::String& end_block) {
				stduint first_instruction = CurrentBlock().instructions.Count();
				LoweredValue lowered = LowerExpression(condition_node);
				if (!lowered.valid) return false;
				Value condition = AsCondition(lowered.value);
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
				Value condition = AsCondition(lowered.value);
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
					Value condition = AsCondition(lowered.value);
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
				return true;
			}

			// Lower one function body without requiring an explicit return statement.
			bool Lower(uni::Nnode* body) {
				CollectLocals(body ? body->subf : nullptr);
				return LowerBlock(body);
			}
		};

	}

	bool LowerFrontendC(uni::Nchain& syntax_tree, Module& module,
		const FrontendCDiagnostics& diagnostics,
		const FrontendCDebugInfo* debug_info) {
		module.functions.Clear();
		for (uni::Nnode* node = syntax_tree.Root(); node;) {
			if (node->type != ToToken(FrontendCToken::Function) || !node->addr ||
				!node->next || node->next->type != tok_block) {
				diagnostics.Report(DiagnosticStage::Lowering,
					"expected analyzed function definition", node);
				return false;
			}

			uni::Nnode* body = node->next;
			Function function;
			function.name = node->addr;
			function.return_type = Type::I64;
			FunctionBuilder builder(function, diagnostics, debug_info);
			if (!builder.Lower(body)) return false;
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
