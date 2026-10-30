// ASCII C++11 TAB4 LF
// Docutitle: Clover Intermediate Representation Text Format
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

#include "../../../inc/cpp/Lango/CloverIR.hpp"

namespace clover {

	namespace {

		// Convert a Clover type to its textual spelling.
		const char* TypeName(Type type) {
			switch (type) {
			case Type::Void: return "void";
			case Type::I1: return "i1";
			case Type::I8: return "i8";
			case Type::I16: return "i16";
			case Type::I32: return "i32";
			case Type::I64: return "i64";
			case Type::Ptr: return "ptr";
			}
			return "?";
		}

		// Convert an arithmetic or bitwise opcode to text.
		const char* OpcodeName(Opcode opcode) {
			switch (opcode) {
			case Opcode::Mov: return "mov";
			case Opcode::Add: return "add";
			case Opcode::Sub: return "sub";
			case Opcode::Mul: return "mul";
			case Opcode::SDiv: return "sdiv";
			case Opcode::UDiv: return "udiv";
			case Opcode::SRem: return "srem";
			case Opcode::URem: return "urem";
			case Opcode::And: return "and";
			case Opcode::Or: return "or";
			case Opcode::Xor: return "xor";
			case Opcode::Shl: return "shl";
			case Opcode::AShr: return "ashr";
			case Opcode::LShr: return "lshr";
			case Opcode::PtrAdd: return "ptradd";
			default: return "?";
			}
		}

		// Convert an integer-comparison predicate to text.
		const char* PredicateName(ICmpPredicate predicate) {
			switch (predicate) {
			case ICmpPredicate::EQ: return "eq";
			case ICmpPredicate::NE: return "ne";
			case ICmpPredicate::SLT: return "slt";
			case ICmpPredicate::SLE: return "sle";
			case ICmpPredicate::SGT: return "sgt";
			case ICmpPredicate::SGE: return "sge";
			case ICmpPredicate::ULT: return "ult";
			case ICmpPredicate::ULE: return "ule";
			case ICmpPredicate::UGT: return "ugt";
			case ICmpPredicate::UGE: return "uge";
			}
			return "?";
		}

		// Print one SSA value reference.
		void PrintValue(uni::OstreamTrait& output, Value value) {
			output.OutFormat("%%%[u]", value.id);
		}

		void PrintBlockName(uni::OstreamTrait& output, const uni::String& name) {
			output.OutFormat("^%s", name.reference());
		}

		// Serialize one instruction according to its operand layout.
		bool PrintInstruction(const Instruction& instruction, uni::OstreamTrait& output) {
			if (instruction.debug_source.getByteCount())
				output.OutFormat("    ; %s\n", instruction.debug_source.reference());
			output.OutFormat("    ");
			if (instruction.has_result) {
				PrintValue(output, instruction.result);
				output.OutFormat(" = ");
			}

			switch (instruction.opcode) {
			case Opcode::Const:
				output.OutFormat("const %s %[i]\n", TypeName(instruction.type), instruction.immediate);
				break;
			case Opcode::Local:
				output.OutFormat("local %s", TypeName(instruction.type));
				if (instruction.immediate > 1)
					output.OutFormat(", %[i]", instruction.immediate);
				if (instruction.name.getByteCount())
					output.OutFormat(" ; %s", instruction.name.reference());
				output.OutFormat("\n");
				break;
			case Opcode::Load:
				output.OutFormat("load %s, ", TypeName(instruction.type));
				PrintValue(output, instruction.operands.begin()[0]);
				output.OutFormat("\n");
				break;
			case Opcode::Store:
				output.OutFormat("store %s ", TypeName(instruction.type));
				PrintValue(output, instruction.operands.begin()[0]);
				output.OutFormat(", ");
				PrintValue(output, instruction.operands.begin()[1]);
				output.OutFormat("\n");
				break;
			case Opcode::Mov:
			case Opcode::Add:
			case Opcode::Sub:
			case Opcode::Mul:
			case Opcode::SDiv:
			case Opcode::UDiv:
			case Opcode::SRem:
			case Opcode::URem:
			case Opcode::And:
			case Opcode::Or:
			case Opcode::Xor:
			case Opcode::Shl:
			case Opcode::AShr:
			case Opcode::LShr:
				output.OutFormat("%s %s ", OpcodeName(instruction.opcode), TypeName(instruction.type));
				PrintValue(output, instruction.operands.begin()[0]);
				if (instruction.operands.Count() > 1) {
					output.OutFormat(", ");
					PrintValue(output, instruction.operands.begin()[1]);
				}
				output.OutFormat("\n");
				break;
			case Opcode::PtrAdd:
				output.OutFormat("ptradd %s ", TypeName(instruction.type));
				PrintValue(output, instruction.operands.begin()[0]);
				output.OutFormat(", ");
				PrintValue(output, instruction.operands.begin()[1]);
				output.OutFormat("\n");
				break;
			case Opcode::ICmp:
				output.OutFormat("icmp %s %s ", PredicateName(instruction.predicate), TypeName(instruction.type));
				PrintValue(output, instruction.operands.begin()[0]);
				output.OutFormat(", ");
				PrintValue(output, instruction.operands.begin()[1]);
				output.OutFormat("\n");
				break;
			case Opcode::ZExt:
			case Opcode::SExt:
			case Opcode::Trunc:
				output.OutFormat("%s %s ", instruction.opcode == Opcode::ZExt ? "zext" :
					instruction.opcode == Opcode::SExt ? "sext" : "trunc",
					TypeName(instruction.operands.begin()[0].type));
				PrintValue(output, instruction.operands.begin()[0]);
				output.OutFormat(" to %s\n", TypeName(instruction.type));
				break;
			case Opcode::GlobalAddr:
				output.OutFormat("globaladdr @%s\n", instruction.name.reference());
				break;
			case Opcode::Call:
				output.OutFormat("call %s @%s(", TypeName(instruction.type),
					instruction.callee.reference());
				for (stduint index = 0; index < instruction.operands.Count(); index++) {
					if (index) output.OutFormat(", ");
					Value operand = instruction.operands.begin()[index];
					output.OutFormat("%s ", TypeName(operand.type));
					PrintValue(output, operand);
				}
				output.OutFormat(")\n");
				break;
			case Opcode::Jmp:
				output.OutFormat("jmp ");
				if (instruction.operands.Count()) {
					output.OutFormat("i1 ");
					PrintValue(output, instruction.operands.begin()[0]);
					output.OutFormat(", ");
					PrintBlockName(output, instruction.target);
					output.OutFormat(", ");
					PrintBlockName(output, instruction.target_false);
				}
				else PrintBlockName(output, instruction.target);
				output.OutFormat("\n");
				break;
			case Opcode::Ret:
				output.OutFormat("ret %s", TypeName(instruction.type));
				if (instruction.type != Type::Void) {
					output.OutFormat(" ");
					PrintValue(output, instruction.operands.begin()[0]);
				}
				output.OutFormat("\n");
				break;
			}
			return true;
		}

		// Tokenizer for the deliberately small Clover textual grammar.
		class Lexer {
		private:
			uni::IstreamTrait& input;
			int current = -2;
			stduint line = 1;

			void Read() {
				current = input.inn();
			}

			bool IsSpace(int ch) const {
				return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
			}

			bool IsPunctuation(int ch) const {
				return ch == '@' || ch == '%' || ch == '^' || ch == '(' || ch == ')' ||
					ch == '{' || ch == '}' || ch == ',' || ch == '=' || ch == ':';
			}

			void SkipIgnored() {
				for (;;) {
					while (IsSpace(current)) {
						if (current == '\n') line++;
						Read();
					}
					if (current != ';') break;
					while (current >= 0 && current != '\n') Read();
				}
			}

		public:
			Lexer(uni::IstreamTrait& source) : input(source) {}

			stduint Line() const { return line; }

			uni::String Next() {
				if (current == -2) Read();
				SkipIgnored();
				uni::String token;
				if (current < 0) return token;

				if (IsPunctuation(current)) {
					token << (char)current;
					Read();
					return token;
				}

				if (current == '-') {
					token << '-';
					Read();
					if (current == '>') {
						token << '>';
						Read();
						return token;
					}
				}

				while (current >= 0 && !IsSpace(current) && current != ';' &&
					!IsPunctuation(current) && current != '-') {
					token << (char)current;
					Read();
				}
				return token;
			}
		};

		// Associates an already defined SSA ID with its type.
		struct DefinedValue {
			Value value;
		};

		// Recursive-descent parser for the printed Clover IR syntax.
		class Parser {
		private:
			Lexer lexer;
			uni::String token;
			uni::Vector<DefinedValue> definitions;
			bool failed = false;

			void Advance() {
				token = lexer.Next();
			}

			bool Is(const char* text) const {
				return !StrCompare(token.reference(), text);
			}

			bool Error(const char* message) {
				if (!failed)
					plogerro("Clover IR line %[u]: %s near '%s'", lexer.Line(), message, token.reference());
				failed = true;
				return false;
			}

			bool Expect(const char* text) {
				if (!Is(text)) return Error("unexpected token");
				Advance();
				return true;
			}

			bool ReadName(uni::String& name) {
				if (!token.getByteCount()) return Error("expected name");
				name = token;
				Advance();
				return true;
			}

			bool ReadType(Type& type) {
				if (Is("void")) type = Type::Void;
				else if (Is("i1")) type = Type::I1;
				else if (Is("i8")) type = Type::I8;
				else if (Is("i16")) type = Type::I16;
				else if (Is("i32")) type = Type::I32;
				else if (Is("i64")) type = Type::I64;
				else if (Is("ptr")) type = Type::Ptr;
				else return Error("expected Clover type");
				Advance();
				return true;
			}

			bool ReadNumber(stdsint& number) {
				if (!token.getByteCount()) return Error("expected integer");
				number = atoins(token.reference());
				Advance();
				return true;
			}

			bool ReadResultID(unsigned& id) {
				if (!Expect("%")) return false;
				if (!token.getByteCount()) return Error("expected SSA identifier");
				stdsint parsed = atoins(token.reference());
				if (parsed < 0) return Error("negative SSA identifier");
				id = (unsigned)parsed;
				Advance();
				return true;
			}

			bool FindValue(unsigned id, Value& value) {
				for (const DefinedValue& defined : definitions) {
					if (defined.value.id == id) {
						value = defined.value;
						return true;
					}
				}
				return Error("use of undefined SSA value");
			}

			bool ReadValue(Value& value) {
				unsigned id;
				if (!ReadResultID(id)) return false;
				return FindValue(id, value);
			}

			bool ReadBlockName(uni::String& name) {
				return Expect("^") && ReadName(name);
			}

			bool Define(Value value) {
				for (const DefinedValue& defined : definitions)
					if (defined.value.id == value.id) return Error("duplicate SSA identifier");
				DefinedValue defined;
				defined.value = value;
				definitions.Append(defined);
				return true;
			}

			bool ReadPredicate(ICmpPredicate& predicate) {
				if (Is("eq")) predicate = ICmpPredicate::EQ;
				else if (Is("ne")) predicate = ICmpPredicate::NE;
				else if (Is("slt")) predicate = ICmpPredicate::SLT;
				else if (Is("sle")) predicate = ICmpPredicate::SLE;
				else if (Is("sgt")) predicate = ICmpPredicate::SGT;
				else if (Is("sge")) predicate = ICmpPredicate::SGE;
				else if (Is("ult")) predicate = ICmpPredicate::ULT;
				else if (Is("ule")) predicate = ICmpPredicate::ULE;
				else if (Is("ugt")) predicate = ICmpPredicate::UGT;
				else if (Is("uge")) predicate = ICmpPredicate::UGE;
				else return Error("expected icmp predicate");
				Advance();
				return true;
			}

			bool ReadBinaryOperands(Instruction& instruction) {
				Value left, right;
				if (!ReadValue(left) || !Expect(",") || !ReadValue(right)) return false;
				instruction.operands.Append(left);
				instruction.operands.Append(right);
				return true;
			}

			bool IsIntegerType(Type type) const {
				return type != Type::Void && type != Type::Ptr;
			}

			bool ReadCallOperands(Instruction& instruction) {
				if (!Expect("(")) return false;
				while (!Is(")")) {
					Type type;
					Value value;
					if (!ReadType(type) || !ReadValue(value)) return false;
					if (value.type != type) return Error("call argument type does not match value");
					instruction.operands.Append(value);
					if (Is(")")) break;
					if (!Expect(",")) return false;
				}
				return Expect(")");
			}

			bool ParseInstruction(BasicBlock& block) {
				Instruction instruction;
				unsigned result_id = 0;
				bool has_result = false;
				if (Is("%")) {
					if (!ReadResultID(result_id) || !Expect("=")) return false;
					has_result = true;
				}

				uni::String operation;
				if (!ReadName(operation)) return false;
				instruction.has_result = has_result;

				if (!StrCompare(operation.reference(), "const")) {
					instruction.opcode = Opcode::Const;
					if (!has_result || !ReadType(instruction.type) || !ReadNumber(instruction.immediate))
						return Error("invalid const instruction");
					instruction.result.type = instruction.type;
				}
				else if (!StrCompare(operation.reference(), "local")) {
					instruction.opcode = Opcode::Local;
					if (!has_result || !ReadType(instruction.type)) return Error("invalid local instruction");
					instruction.immediate = 1;
					if (Is(",")) {
						Advance();
						if (!ReadNumber(instruction.immediate) || instruction.immediate <= 0)
							return Error("invalid local element count");
					}
					instruction.result.type = Type::Ptr;
				}
				else if (!StrCompare(operation.reference(), "load")) {
					instruction.opcode = Opcode::Load;
					Value address;
					if (!has_result || !ReadType(instruction.type) || !Expect(",") || !ReadValue(address))
						return Error("invalid load instruction");
					instruction.operands.Append(address);
					instruction.result.type = instruction.type;
				}
				else if (!StrCompare(operation.reference(), "store")) {
					instruction.opcode = Opcode::Store;
					if (has_result || !ReadType(instruction.type) || !ReadBinaryOperands(instruction))
						return Error("invalid store instruction");
				}
				else if (!StrCompare(operation.reference(), "mov")) {
					instruction.opcode = Opcode::Mov;
					Value operand;
					if (!has_result || !ReadType(instruction.type) || !ReadValue(operand))
						return Error("invalid mov instruction");
					instruction.operands.Append(operand);
					instruction.result.type = instruction.type;
				}
				else if (!StrCompare(operation.reference(), "add") || !StrCompare(operation.reference(), "sub") ||
					!StrCompare(operation.reference(), "mul") || !StrCompare(operation.reference(), "sdiv") ||
					!StrCompare(operation.reference(), "udiv") || !StrCompare(operation.reference(), "srem") ||
					!StrCompare(operation.reference(), "urem") || !StrCompare(operation.reference(), "and") ||
					!StrCompare(operation.reference(), "or") || !StrCompare(operation.reference(), "xor") ||
					!StrCompare(operation.reference(), "shl") || !StrCompare(operation.reference(), "ashr") ||
					!StrCompare(operation.reference(), "lshr")) {
					if (!StrCompare(operation.reference(), "add")) instruction.opcode = Opcode::Add;
					else if (!StrCompare(operation.reference(), "sub")) instruction.opcode = Opcode::Sub;
					else if (!StrCompare(operation.reference(), "mul")) instruction.opcode = Opcode::Mul;
					else if (!StrCompare(operation.reference(), "sdiv")) instruction.opcode = Opcode::SDiv;
					else if (!StrCompare(operation.reference(), "udiv")) instruction.opcode = Opcode::UDiv;
					else if (!StrCompare(operation.reference(), "srem")) instruction.opcode = Opcode::SRem;
					else if (!StrCompare(operation.reference(), "urem")) instruction.opcode = Opcode::URem;
					else if (!StrCompare(operation.reference(), "and")) instruction.opcode = Opcode::And;
					else if (!StrCompare(operation.reference(), "or")) instruction.opcode = Opcode::Or;
					else if (!StrCompare(operation.reference(), "xor")) instruction.opcode = Opcode::Xor;
					else if (!StrCompare(operation.reference(), "shl")) instruction.opcode = Opcode::Shl;
					else if (!StrCompare(operation.reference(), "ashr")) instruction.opcode = Opcode::AShr;
					else instruction.opcode = Opcode::LShr;
					if (!has_result || !ReadType(instruction.type) || !ReadBinaryOperands(instruction))
						return Error("invalid binary instruction");
					instruction.result.type = instruction.type;
				}
				else if (!StrCompare(operation.reference(), "ptradd")) {
					instruction.opcode = Opcode::PtrAdd;
					if (!has_result || !ReadType(instruction.type) ||
						!ReadBinaryOperands(instruction))
						return Error("invalid ptradd instruction");
					if (instruction.operands.begin()[0].type != Type::Ptr ||
						!IsIntegerType(instruction.operands.begin()[1].type))
						return Error("ptradd requires a pointer and an integer index");
					instruction.result.type = Type::Ptr;
				}
				else if (!StrCompare(operation.reference(), "icmp")) {
					instruction.opcode = Opcode::ICmp;
					if (!has_result || !ReadPredicate(instruction.predicate) ||
						!ReadType(instruction.type) || !ReadBinaryOperands(instruction))
						return Error("invalid icmp instruction");
					instruction.result.type = Type::I1;
				}
				else if (!StrCompare(operation.reference(), "zext") ||
					!StrCompare(operation.reference(), "sext") ||
					!StrCompare(operation.reference(), "trunc")) {
					instruction.opcode = !StrCompare(operation.reference(), "zext") ? Opcode::ZExt :
						!StrCompare(operation.reference(), "sext") ? Opcode::SExt : Opcode::Trunc;
					Type source_type;
					Value operand;
					if (!has_result || !ReadType(source_type) || !ReadValue(operand) ||
						!Expect("to") || !ReadType(instruction.type))
						return Error("invalid integer conversion instruction");
					if (operand.type != source_type)
						return Error("integer conversion source type does not match value");
					instruction.operands.Append(operand);
					instruction.result.type = instruction.type;
				}
				else if (!StrCompare(operation.reference(), "globaladdr")) {
					instruction.opcode = Opcode::GlobalAddr;
					if (!has_result || !Expect("@") || !ReadName(instruction.name))
						return Error("invalid globaladdr instruction");
					instruction.type = Type::Ptr;
					instruction.result.type = Type::Ptr;
				}
				else if (!StrCompare(operation.reference(), "call")) {
					instruction.opcode = Opcode::Call;
					if (!ReadType(instruction.type) || !Expect("@") ||
						!ReadName(instruction.callee) || !ReadCallOperands(instruction))
						return Error("invalid call instruction");
					if ((instruction.type == Type::Void) == has_result)
						return Error("void call result mismatch");
					instruction.result.type = instruction.type;
				}
				else if (!StrCompare(operation.reference(), "jmp")) {
					instruction.opcode = Opcode::Jmp;
					if (has_result) return Error("invalid jmp instruction");
					if (Is("^")) {
						if (!ReadBlockName(instruction.target)) return false;
					}
					else {
						Value condition;
						if (!ReadType(instruction.type) || instruction.type != Type::I1 ||
							!ReadValue(condition) || !Expect(",") ||
							!ReadBlockName(instruction.target) || !Expect(",") ||
							!ReadBlockName(instruction.target_false))
							return Error("invalid conditional jmp instruction");
						if (condition.type != Type::I1)
							return Error("conditional jmp requires i1 value");
						instruction.operands.Append(condition);
					}
				}
				else if (!StrCompare(operation.reference(), "ret")) {
					instruction.opcode = Opcode::Ret;
					if (has_result || !ReadType(instruction.type))
						return Error("invalid ret instruction");
					if (instruction.type != Type::Void) {
						Value operand;
						if (!ReadValue(operand)) return Error("invalid ret instruction");
						instruction.operands.Append(operand);
					}
				}
				else return Error("unknown opcode");

				if (has_result) {
					instruction.result.id = result_id;
					if (!Define(instruction.result)) return false;
				}
				block.instructions.Append(instruction);
				return true;
			}

			bool ParseBlock(Function& function) {
				BasicBlock block;
				if (!Expect("^") || !ReadName(block.name) || !Expect(":")) return false;
				while (token.getByteCount() && !Is("^") && !Is("}"))
					if (!ParseInstruction(block)) return false;
				function.blocks.Append(block);
				return true;
			}

			bool ParseFunction(Module& module) {
				Function function;
				definitions.Clear();
				if (!Expect("func") || !Expect("@") || !ReadName(function.name) ||
					!Expect("(")) return false;
				while (!Is(")")) {
					Parameter parameter;
					if (!ReadType(parameter.value.type) ||
						!ReadResultID(parameter.value.id) || !Define(parameter.value)) return false;
					function.parameters.Append(parameter);
					if (Is(")")) break;
					if (!Expect(",")) return false;
				}
				if (!Expect(")") || !Expect("->") ||
					!ReadType(function.return_type) || !Expect("{")) return false;
				while (Is("^"))
					if (!ParseBlock(function)) return false;
				if (!Expect("}")) return false;
				for (stduint index = 0; index < function.blocks.Count(); index++)
					for (stduint other = index + 1; other < function.blocks.Count(); other++)
						if (!StrCompare(function.blocks[index].name.reference(),
							function.blocks[other].name.reference()))
							return Error("duplicate block name");
				for (const BasicBlock& block : function.blocks) {
					for (const Instruction& instruction : block.instructions) {
						if (instruction.opcode != Opcode::Jmp) continue;
						bool target_found = false;
						bool false_found = !instruction.target_false.getByteCount();
						for (const BasicBlock& candidate : function.blocks) {
							if (!StrCompare(candidate.name.reference(), instruction.target.reference()))
								target_found = true;
							if (instruction.target_false.getByteCount() &&
								!StrCompare(candidate.name.reference(), instruction.target_false.reference()))
								false_found = true;
						}
						if (!target_found || !false_found) return Error("jmp to undefined block");
					}
				}
				module.functions.Append(function);
				return true;
			}

			bool ParseGlobal(Module& module) {
				Global global;
				if (!Expect("global") || !Expect("@") || !ReadName(global.name) ||
					!ReadType(global.type) || global.type == Type::Void)
					return Error("invalid global declaration");
				if (Is(",")) {
					Advance();
					if (!ReadNumber(global.element_count) || global.element_count <= 0)
						return Error("invalid global element count");
				}
				if (!Expect("=") || !ReadNumber(global.initializer))
					return Error("invalid global initializer");
				module.globals.Append(global);
				return true;
			}

		public:
			Parser(uni::IstreamTrait& input) : lexer(input) {
				Advance();
			}

			bool ParseModule(Module& module) {
				module.globals.Clear();
				module.functions.Clear();
				while (token.getByteCount()) {
					if (Is("global")) {
						if (!ParseGlobal(module)) return false;
					}
					else if (!ParseFunction(module)) return false;
				}
				return !failed;
			}
		};

	}

	bool Parse(uni::IstreamTrait& input, Module& module) {
		Parser parser(input);
		return parser.ParseModule(module);
	}

	bool Print(const Module& module, uni::OstreamTrait& output) {
		for (const Global& global : module.globals) {
			output.OutFormat("global @%s %s", global.name.reference(), TypeName(global.type));
			if (global.element_count > 1) output.OutFormat(", %[i]", global.element_count);
			output.OutFormat(" = %[i]\n", global.initializer);
		}
		for (const Function& function : module.functions) {
			output.OutFormat("func @%s(", function.name.reference());
			for (stduint index = 0; index < function.parameters.Count(); index++) {
				if (index) output.OutFormat(", ");
				const Parameter& parameter = function.parameters.begin()[index];
				output.OutFormat("%s ", TypeName(parameter.value.type));
				PrintValue(output, parameter.value);
			}
			output.OutFormat(") -> %s {\n", TypeName(function.return_type));
			for (const BasicBlock& block : function.blocks) {
				output.OutFormat("^%s:\n", block.name.reference());
				for (const Instruction& instruction : block.instructions)
					if (!PrintInstruction(instruction, output)) return false;
			}
			output.OutFormat("}\n");
		}
		return true;
	}

}
