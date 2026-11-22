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
			case Type::F32: return "f32";
			case Type::F64: return "f64";
			case Type::F128: return "f128";
			case Type::Ptr: return "ptr";
			}
			return "?";
		}

		void PrintTypeReference(uni::OstreamTrait& output,
			const TypeReference& type) {
			if (type.IsBits()) output.OutFormat("b%[u]", type.bit_width);
			else if (type.IsDefined())
				output.OutFormat("@%s", type.defined_name.reference());
			else output.OutFormat("%s", TypeName(type.scalar));
			for (unsigned index = 0; index < type.array_depth; index++)
				output.OutFormat("[%[u]]", type.array_counts[index]);
		}

		void PrintLayoutSize(uni::OstreamTrait& output, stduint bits) {
			if (!(bits % 8)) output.OutFormat("byt %[u]", bits / 8);
			else output.OutFormat("bit %[u]", bits);
		}

		bool TypeArrayElementCount(const TypeReference& type, stduint& count) {
			count = 1;
			for (unsigned index = 0; index < type.array_depth; index++) {
				if (!type.array_counts[index] ||
					count > stduint(-1) / type.array_counts[index]) return false;
				count *= type.array_counts[index];
			}
			return true;
		}

		bool IsVoidObjectType(const TypeReference& type) {
			return !type.IsBits() && !type.IsDefined() && type.scalar == Type::Void;
		}

		void PrintTypeDefinitions(const Module& module, uni::OstreamTrait& output) {
			for (const TypeDefinition& definition : module.type_definitions) {
				output.OutFormat("type @%s = %s size ", definition.name.reference(),
					definition.kind == TypeDefinitionKind::Structure ? "struct" : "union");
				PrintLayoutSize(output, definition.size_bits);
				if (definition.alignment_bits) {
					output.OutFormat(" align ");
					PrintLayoutSize(output, definition.alignment_bits);
				}
				output.OutFormat(" {\n");
				for (const TypeMember& member : definition.members) {
					output.OutFormat("    ");
					if (member.name.getByteCount())
						output.OutFormat("%s: ", member.name.reference());
					PrintTypeReference(output, member.type);
					output.OutChar('\n');
				}
				output.OutFormat("}\n");
			}
		}

		const char* RelocationModelName(RelocationModel model) {
			switch (model) {
			case RelocationModel::Static: return "static";
			case RelocationModel::PIC: return "pic";
			case RelocationModel::PIE: return "pie";
			}
			return "?";
		}

		void PrintSymbolAttributes(uni::OstreamTrait& output,
			SymbolVisibility visibility, SymbolBinding binding) {
			if (visibility != SymbolVisibility::Default)
				output.OutFormat(" visibility %s", visibility == SymbolVisibility::Hidden ?
					"hidden" : "protected");
			if (binding == SymbolBinding::Weak)
				output.OutFormat(" binding weak");
		}

		const char* FloatPredicateName(FCmpPredicate predicate) {
			switch (predicate) {
			case FCmpPredicate::EQ: return "eq";
			case FCmpPredicate::NE: return "ne";
			case FCmpPredicate::LT: return "lt";
			case FCmpPredicate::LE: return "le";
			case FCmpPredicate::GT: return "gt";
			case FCmpPredicate::GE: return "ge";
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

		void PrintHardwareValue(uni::OstreamTrait& output, HardwareValue value) {
			output.OutFormat("%%%[u]", value.id);
		}

		const char* PortDirectionName(PortDirection direction) {
			switch (direction) {
			case PortDirection::Input: return "input";
			case PortDirection::Output: return "output";
			case PortDirection::InOut: return "inout";
			}
			return "?";
		}

		void PrintQuoted(uni::OstreamTrait& output, const uni::String& text) {
			output.OutChar('"');
			for (const char* current = text.reference(); current && *current; current++) {
				if (*current == '"' || *current == '\\') output.OutChar('\\');
				output.OutChar(*current);
			}
			output.OutChar('"');
		}

		void PrintSymbolInitializer(uni::OstreamTrait& output,
			const uni::String& symbol, stdsint addend) {
			output.OutFormat("@%s", symbol.reference());
			if (addend > 0) output.OutFormat(" + %[i]", addend);
			else if (addend < 0) {
				stduint magnitude = (stduint)(-(addend + 1));
				magnitude++;
				output.OutFormat(" - %[u]", magnitude);
			}
		}

		// Serialize one instruction according to its operand layout.
		bool PrintInstruction(const Instruction& instruction, uni::OstreamTrait& output,
			uint32& current_file, uint32& current_line) {
			if (instruction.debug_file && instruction.debug_file != current_file) {
				output.OutFormat("    file %[u]\n", instruction.debug_file);
				current_file = instruction.debug_file;
			}
			if (instruction.debug_line && instruction.debug_line != current_line) {
				output.OutFormat("    line %[u]\n", instruction.debug_line);
				current_line = instruction.debug_line;
			}
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
			case Opcode::FConst:
				output.OutFormat("fconst %s %[i]", TypeName(instruction.type), instruction.immediate);
				if (instruction.type == Type::F128)
					output.OutFormat(", %[i]", instruction.immediate_high);
				output.OutFormat("\n");
				break;
			case Opcode::Local:
				output.OutFormat("local ");
				if (instruction.has_object_type)
					PrintTypeReference(output, instruction.object_type);
				else output.OutFormat("%s", TypeName(instruction.type));
				if (instruction.operands.Count()) {
					output.OutFormat(", ");
					PrintValue(output, instruction.operands.begin()[0]);
				}
				else if (instruction.immediate > 1)
					output.OutFormat(", %[i]", instruction.immediate);
				if (instruction.alignment)
					output.OutFormat(" align %[u]", instruction.alignment);
				if (instruction.name.getByteCount())
					output.OutFormat(" ; %s", instruction.name.reference());
				output.OutFormat("\n");
				break;
			case Opcode::StackSave:
				output.OutFormat("stacksave\n");
				break;
			case Opcode::StackRestore:
				output.OutFormat("stackrestore ");
				PrintValue(output, instruction.operands.begin()[0]);
				output.OutFormat("\n");
				break;
			case Opcode::Load:
				output.OutFormat("load %s, ", TypeName(instruction.type));
				PrintValue(output, instruction.operands.begin()[0]);
				if (instruction.alignment)
					output.OutFormat(" align %[u]", instruction.alignment);
				output.OutFormat("\n");
				break;
			case Opcode::Store:
				output.OutFormat("store %s ", TypeName(instruction.type));
				PrintValue(output, instruction.operands.begin()[0]);
				output.OutFormat(", ");
				PrintValue(output, instruction.operands.begin()[1]);
				if (instruction.alignment)
					output.OutFormat(" align %[u]", instruction.alignment);
				output.OutFormat("\n");
				break;
			case Opcode::MemCopy:
				output.OutFormat("memcopy ");
				PrintValue(output, instruction.operands.begin()[0]);
				output.OutFormat(", ");
				PrintValue(output, instruction.operands.begin()[1]);
				output.OutFormat(", %[i]", instruction.immediate);
				if (instruction.alignment || instruction.source_alignment)
					output.OutFormat(" align %[u], %[u]",
						instruction.alignment ? instruction.alignment : 1,
						instruction.source_alignment ? instruction.source_alignment : 1);
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
				output.OutFormat("ptradd ");
				if (instruction.has_object_type)
					PrintTypeReference(output, instruction.object_type);
				else output.OutFormat("%s", TypeName(instruction.type));
				output.OutFormat(" ");
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
			case Opcode::FAdd:
			case Opcode::FSub:
			case Opcode::FMul:
			case Opcode::FDiv:
				output.OutFormat("%s %s ", instruction.opcode == Opcode::FAdd ? "fadd" :
					instruction.opcode == Opcode::FSub ? "fsub" :
					instruction.opcode == Opcode::FMul ? "fmul" : "fdiv",
					TypeName(instruction.type));
				PrintValue(output, instruction.operands.begin()[0]);
				output.OutFormat(", ");
				PrintValue(output, instruction.operands.begin()[1]);
				output.OutFormat("\n");
				break;
			case Opcode::FCmp:
				output.OutFormat("fcmp %s %s ",
					FloatPredicateName(instruction.float_predicate),
					TypeName(instruction.type));
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
			case Opcode::IntToPtr:
			case Opcode::PtrToInt:
				output.OutFormat("%s %s ",
					instruction.opcode == Opcode::IntToPtr ? "inttoptr" : "ptrtoint",
					TypeName(instruction.operands.begin()[0].type));
				PrintValue(output, instruction.operands.begin()[0]);
				output.OutFormat(" to %s\n", TypeName(instruction.type));
				break;
			case Opcode::SIToFP:
			case Opcode::UIToFP:
			case Opcode::FPToSI:
			case Opcode::FPToUI:
			case Opcode::FPExt:
			case Opcode::FPTrunc:
				output.OutFormat("%s %s ",
					instruction.opcode == Opcode::SIToFP ? "sitofp" :
					instruction.opcode == Opcode::UIToFP ? "uitofp" :
					instruction.opcode == Opcode::FPToSI ? "fptosi" :
					instruction.opcode == Opcode::FPToUI ? "fptoui" :
					instruction.opcode == Opcode::FPExt ? "fpext" : "fptrunc",
					TypeName(instruction.operands.begin()[0].type));
				PrintValue(output, instruction.operands.begin()[0]);
				output.OutFormat(" to %s\n", TypeName(instruction.type));
				break;
			case Opcode::GlobalAddr:
				output.OutFormat("globaladdr @%s\n", instruction.name.reference());
				break;
			case Opcode::FunctionAddr:
				output.OutFormat("funcaddr @%s\n", instruction.name.reference());
				break;
			case Opcode::Call:
				output.OutFormat("call ");
				if (instruction.by_value_return) {
					output.OutFormat("byval ");
					PrintTypeReference(output, instruction.object_type);
					output.OutFormat(" ");
				}
				else output.OutFormat("%s ", TypeName(instruction.type));
				if (instruction.has_callee_value)
					PrintValue(output, instruction.callee_value);
				else output.OutFormat("@%s", instruction.callee.reference());
				output.OutFormat("(");
				for (stduint index = 0; index < instruction.operands.Count(); index++) {
					if (index) output.OutFormat(", ");
					if (instruction.call_arguments.Count() &&
						instruction.call_arguments.begin()[index].by_value) {
						const CallArgument& argument = instruction.call_arguments.begin()[index];
						output.OutFormat("byval ");
						PrintTypeReference(output, argument.object_type);
						output.OutFormat(" ");
						PrintValue(output, argument.value);
					}
					else {
						Value operand = instruction.operands.begin()[index];
						output.OutFormat("%s ", TypeName(operand.type));
						PrintValue(output, operand);
					}
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
				output.OutFormat("ret ");
				if (instruction.by_value_return) {
					output.OutFormat("byval ");
					PrintTypeReference(output, instruction.object_type);
				}
				else output.OutFormat("%s", TypeName(instruction.type));
				if (instruction.type != Type::Void || instruction.by_value_return) {
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
					ch == '{' || ch == '}' || ch == '[' || ch == ']' ||
					ch == ',' || ch == '=' || ch == ':' ||
					ch == '+';
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
				if (current == '"') {
					Read();
					while (current >= 0 && current != '"') {
						if (current == '\\') {
							Read();
							if (current < 0) break;
						}
						token << (char)current;
						Read();
					}
					if (current == '"') Read();
					return token;
				}

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

		struct DefinedHardwareValue {
			HardwareValue value;
		};

		// Recursive-descent parser for the printed Clover IR syntax.
		class Parser {
		private:
			Lexer lexer;
			uni::String token;
			uni::Vector<DefinedValue> definitions;
			uni::Vector<DefinedHardwareValue> hardware_definitions;
			uint32 debug_file = 0;
			uint32 debug_line = 0;
			bool failed = false;
			bool relocation_model_seen = false;
			bool ir_class_seen = false;
			Module* current_module = nullptr;

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
				else if (Is("f32")) type = Type::F32;
				else if (Is("f64")) type = Type::F64;
				else if (Is("f128")) type = Type::F128;
				else if (Is("ptr")) type = Type::Ptr;
				else return Error("expected Clover type");
				Advance();
				return true;
			}

			bool ReadTypeSuffix(TypeReference& type) {
				while (Is("[")) {
					Advance();
					stdsint count;
					if (!ReadNumber(count) || count <= 0 || !Expect("]"))
						return Error("invalid array type dimension");
					if (type.array_depth >= TypeReference::MaxArrayDepth)
						return Error("array type nesting is too deep");
					type.array_counts[type.array_depth++] = (stduint)count;
				}
				return true;
			}

			bool ReadTypeFromFirst(const uni::String& first, TypeReference& type) {
				const char* text = first.reference();
				if (!StrCompare(text, "void")) type = Type::Void;
				else if (!StrCompare(text, "i1")) type = Type::I1;
				else if (!StrCompare(text, "i8")) type = Type::I8;
				else if (!StrCompare(text, "i16")) type = Type::I16;
				else if (!StrCompare(text, "i32")) type = Type::I32;
				else if (!StrCompare(text, "i64")) type = Type::I64;
				else if (!StrCompare(text, "f32")) type = Type::F32;
				else if (!StrCompare(text, "f64")) type = Type::F64;
				else if (!StrCompare(text, "f128")) type = Type::F128;
				else if (!StrCompare(text, "ptr")) type = Type::Ptr;
				else if (text[0] == 'b' && text[1]) {
					char* end = nullptr;
					uint64 width = StrTokenULong(text + 1, &end, 10);
					if (*end || !width) return Error("invalid bit-vector type");
					type = Type::Void;
					type.bit_width = (stduint)width;
				}
				else return Error("expected Clover type");
				return ReadTypeSuffix(type);
			}

			bool ReadTypeReference(Module& module, TypeReference& type) {
				if (Is("@")) {
					Advance();
					if (!ReadName(type.defined_name)) return false;
					if (!FindTypeDefinition(module, type.defined_name.reference()))
						return Error("reference to undefined Clover type");
					return ReadTypeSuffix(type);
				}
				uni::String first = token;
				if (!first.getByteCount()) return Error("expected Clover type");
				Advance();
				return ReadTypeFromFirst(first, type);
			}

			bool ReadHardwareType(Module& module, TypeReference& type) {
				if (!ReadTypeReference(module, type) || !type.IsBits() || type.IsArray() ||
					type.bit_width > 64)
					return Error("hardware value type must be b1 through b64");
				return true;
			}

			bool ReadNumber(stdsint& number) {
				if (!token.getByteCount()) return Error("expected integer");
				number = atoins(token.reference());
				Advance();
				return true;
			}

			bool ReadHardwareNumber(uint64& number) {
				if (!token.getByteCount()) return Error("expected hardware integer");
				char* end = nullptr;
				number = StrTokenULong(token.reference(), &end, 10);
				if (*end) return Error("invalid hardware integer");
				Advance();
				return true;
			}

			bool ReadSymbolInitializer(uni::String& symbol, stdsint& addend) {
				if (!Expect("@") || !ReadName(symbol)) return false;
				addend = 0;
				if (!Is("+") && !Is("-")) return true;
				bool subtract = Is("-");
				Advance();
				stdsint magnitude;
				if (!ReadNumber(magnitude) || magnitude < 0)
					return Error("symbol addend requires a nonnegative integer");
				addend = subtract ? -magnitude : magnitude;
				return true;
			}

			bool ReadDebugNumber(uint32& number) {
				stdsint parsed;
				if (!ReadNumber(parsed) || parsed <= 0) return Error("invalid debug number");
				number = (uint32)parsed;
				return true;
			}

			bool ReadOptionalAlignment(Instruction& instruction) {
				if (!Is("align")) return true;
				Advance();
				stdsint alignment;
				if (!ReadNumber(alignment) || alignment <= 0 ||
					(alignment & (alignment - 1)))
					return Error("alignment must be a positive power of two");
				instruction.alignment = (stduint)alignment;
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

			bool FindHardwareValue(unsigned id, HardwareValue& value) {
				for (const DefinedHardwareValue& defined : hardware_definitions) {
					if (defined.value.id == id) {
						value = defined.value;
						return true;
					}
				}
				return Error("use of undefined hardware SSA value");
			}

			bool ReadHardwareValue(HardwareValue& value) {
				unsigned id;
				if (!ReadResultID(id)) return false;
				return FindHardwareValue(id, value);
			}

			bool DefineHardwareValue(HardwareValue value) {
				for (const DefinedHardwareValue& defined : hardware_definitions)
					if (defined.value.id == value.id)
						return Error("duplicate hardware SSA identifier");
				DefinedHardwareValue defined;
				defined.value = value;
				hardware_definitions.Append(defined);
				return true;
			}

			bool SelectIRClass(Module& module, IRClass ir_class) {
				if (ir_class_seen && module.ir_class != ir_class)
					return Error("mixed Clover IR classes are not supported");
				module.ir_class = ir_class;
				ir_class_seen = true;
				return true;
			}

			const HardwarePort* FindHardwarePort(const HardwareModule& module,
				const uni::String& name) const {
				for (const HardwarePort& port : module.ports)
					if (!StrCompare(port.name.reference(), name.reference())) return &port;
				return nullptr;
			}

			bool HasHardwareDriver(const HardwareModule& module,
				const uni::String& name) const {
				for (const HardwareBlock& block : module.blocks)
					for (const HardwareOperation& operation : block.operations)
						if (operation.opcode == HardwareOpcode::Assign &&
							!StrCompare(operation.symbol.reference(), name.reference())) return true;
				return false;
			}

			bool HasHardwareDriver(const HardwareBlock& block,
				const uni::String& name) const {
				for (const HardwareOperation& operation : block.operations)
					if (operation.opcode == HardwareOpcode::Assign &&
						!StrCompare(operation.symbol.reference(), name.reference())) return true;
				return false;
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

			bool ReadFloatPredicate(FCmpPredicate& predicate) {
				if (Is("eq")) predicate = FCmpPredicate::EQ;
				else if (Is("ne")) predicate = FCmpPredicate::NE;
				else if (Is("lt")) predicate = FCmpPredicate::LT;
				else if (Is("le")) predicate = FCmpPredicate::LE;
				else if (Is("gt")) predicate = FCmpPredicate::GT;
				else if (Is("ge")) predicate = FCmpPredicate::GE;
				else return Error("expected fcmp predicate");
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
				return type == Type::I1 || type == Type::I8 || type == Type::I16 ||
					type == Type::I32 || type == Type::I64;
			}

			bool IsFloatType(Type type) const {
				return type == Type::F32 || type == Type::F64 || type == Type::F128;
			}

			unsigned FloatBits(Type type) const {
				return type == Type::F32 ? 32 : type == Type::F64 ? 64 :
					type == Type::F128 ? 128 : 0;
			}

			bool ReadCallOperands(Instruction& instruction) {
				if (!Expect("(")) return false;
				while (!Is(")")) {
					if (Is("byval")) {
						Advance();
						CallArgument argument;
						argument.by_value = true;
						if (!ReadTypeReference(*current_module, argument.object_type) ||
							!ReadValue(argument.value) || argument.value.type != Type::Ptr)
							return Error("invalid by-value call argument");
						instruction.operands.Append(argument.value);
						instruction.call_arguments.Append(argument);
					}
					else {
						Type type;
						Value value;
						if (!ReadType(type) || !ReadValue(value)) return false;
						if (value.type != type) return Error("call argument type does not match value");
						instruction.operands.Append(value);
						CallArgument argument;
						argument.value = value;
						instruction.call_arguments.Append(argument);
					}
					if (Is(")")) break;
					if (!Expect(",")) return false;
				}
				return Expect(")");
			}

			bool ParseInstruction(BasicBlock& block) {
				while (Is("file") || Is("line")) {
					bool is_file = Is("file");
					Advance();
					uint32 value;
					if (!ReadDebugNumber(value)) return false;
					if (is_file) debug_file = value;
					else debug_line = value;
				}
				Instruction instruction;
				instruction.debug_file = debug_file;
				instruction.debug_line = debug_line;
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
					if (!IsIntegerType(instruction.type))
						return Error("const requires an integer type");
					instruction.result.type = instruction.type;
				}
				else if (!StrCompare(operation.reference(), "fconst")) {
					instruction.opcode = Opcode::FConst;
					if (!has_result || !ReadType(instruction.type) ||
						!ReadNumber(instruction.immediate) || !IsFloatType(instruction.type))
						return Error("invalid fconst instruction");
					if (instruction.type == Type::F32 &&
						(instruction.immediate < (stdsint)INT32_MIN ||
							(instruction.immediate >= 0 &&
								(uint64)instruction.immediate > (uint64)UINT32_MAX)))
						return Error("f32 constant bit pattern is out of range");
					if (instruction.type == Type::F128 &&
						(!Expect(",") || !ReadNumber(instruction.immediate_high)))
						return Error("f128 constant requires two 64-bit words");
					instruction.result.type = instruction.type;
				}
				else if (!StrCompare(operation.reference(), "local")) {
					instruction.opcode = Opcode::Local;
					if (!has_result || !ReadTypeReference(*current_module,
						instruction.object_type) ||
						IsVoidObjectType(instruction.object_type))
						return Error("invalid local instruction");
					instruction.has_object_type = true;
					if (instruction.object_type.IsScalar())
						instruction.type = instruction.object_type.scalar;
					instruction.immediate = 1;
					if (Is(",")) {
						Advance();
						if (Is("%")) {
							Value count;
							if (!ReadValue(count) || !IsIntegerType(count.type))
								return Error("dynamic local element count must be an integer value");
							instruction.immediate = 0;
							instruction.operands.Append(count);
						}
						else if (!ReadNumber(instruction.immediate) || instruction.immediate <= 0)
							return Error("invalid local element count");
					}
					if (!ReadOptionalAlignment(instruction)) return false;
					instruction.result.type = Type::Ptr;
				}
				else if (!StrCompare(operation.reference(), "stacksave")) {
					instruction.opcode = Opcode::StackSave;
					instruction.type = Type::Ptr;
					instruction.result.type = Type::Ptr;
					if (!has_result) return Error("stacksave requires a result");
				}
				else if (!StrCompare(operation.reference(), "stackrestore")) {
					instruction.opcode = Opcode::StackRestore;
					instruction.type = Type::Ptr;
					Value marker;
					if (has_result || !ReadValue(marker) || marker.type != Type::Ptr)
						return Error("stackrestore requires one ptr operand");
					instruction.operands.Append(marker);
				}
				else if (!StrCompare(operation.reference(), "load")) {
					instruction.opcode = Opcode::Load;
					Value address;
					if (!has_result || !ReadType(instruction.type) || !Expect(",") || !ReadValue(address))
						return Error("invalid load instruction");
					instruction.operands.Append(address);
					if (address.type != Type::Ptr || instruction.type == Type::Void)
						return Error("load type mismatch");
					if (!ReadOptionalAlignment(instruction)) return false;
					instruction.result.type = instruction.type;
				}
				else if (!StrCompare(operation.reference(), "store")) {
					instruction.opcode = Opcode::Store;
					if (has_result || !ReadType(instruction.type) || !ReadBinaryOperands(instruction))
						return Error("invalid store instruction");
					if (instruction.type == Type::Void ||
						instruction.operands.begin()[0].type != instruction.type ||
						instruction.operands.begin()[1].type != Type::Ptr)
						return Error("store type mismatch");
					if (!ReadOptionalAlignment(instruction)) return false;
				}
				else if (!StrCompare(operation.reference(), "memcopy")) {
					instruction.opcode = Opcode::MemCopy;
					instruction.type = Type::Void;
					if (has_result || !ReadBinaryOperands(instruction) ||
						!Expect(",") || !ReadNumber(instruction.immediate) ||
						instruction.immediate <= 0 ||
						instruction.operands.begin()[0].type != Type::Ptr ||
						instruction.operands.begin()[1].type != Type::Ptr)
						return Error("invalid memcopy instruction");
					if (Is("align")) {
						Advance();
						stdsint destination_alignment;
						stdsint source_alignment;
						if (!ReadNumber(destination_alignment) ||
							!Expect(",") || !ReadNumber(source_alignment) ||
							destination_alignment <= 0 || source_alignment <= 0 ||
							(destination_alignment & (destination_alignment - 1)) ||
							(source_alignment & (source_alignment - 1)))
							return Error("memcopy alignments must be positive powers of two");
						instruction.alignment = (stduint)destination_alignment;
						instruction.source_alignment = (stduint)source_alignment;
					}
				}
				else if (!StrCompare(operation.reference(), "mov")) {
					instruction.opcode = Opcode::Mov;
					Value operand;
					if (!has_result || !ReadType(instruction.type) || !ReadValue(operand))
						return Error("invalid mov instruction");
					instruction.operands.Append(operand);
					if (operand.type != instruction.type || instruction.type == Type::Void)
						return Error("mov type mismatch");
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
					bool is_shift = instruction.opcode == Opcode::Shl ||
						instruction.opcode == Opcode::AShr ||
						instruction.opcode == Opcode::LShr;
					if (!IsIntegerType(instruction.type) ||
						instruction.operands.begin()[0].type != instruction.type ||
						(is_shift ? !IsIntegerType(instruction.operands.begin()[1].type) :
							instruction.operands.begin()[1].type != instruction.type))
						return Error("integer binary type mismatch");
					instruction.result.type = instruction.type;
				}
				else if (!StrCompare(operation.reference(), "fadd") ||
					!StrCompare(operation.reference(), "fsub") ||
					!StrCompare(operation.reference(), "fmul") ||
					!StrCompare(operation.reference(), "fdiv")) {
					instruction.opcode = !StrCompare(operation.reference(), "fadd") ? Opcode::FAdd :
						!StrCompare(operation.reference(), "fsub") ? Opcode::FSub :
						!StrCompare(operation.reference(), "fmul") ? Opcode::FMul : Opcode::FDiv;
					if (!has_result || !ReadType(instruction.type) ||
						!ReadBinaryOperands(instruction) || !IsFloatType(instruction.type) ||
						instruction.operands.begin()[0].type != instruction.type ||
						instruction.operands.begin()[1].type != instruction.type)
						return Error("invalid floating binary instruction");
					instruction.result.type = instruction.type;
				}
				else if (!StrCompare(operation.reference(), "ptradd")) {
					instruction.opcode = Opcode::PtrAdd;
					if (!has_result || !ReadTypeReference(*current_module,
						instruction.object_type) || IsVoidObjectType(instruction.object_type) ||
						!ReadBinaryOperands(instruction))
						return Error("invalid ptradd instruction");
					instruction.has_object_type = true;
					if (instruction.object_type.IsScalar())
						instruction.type = instruction.object_type.scalar;
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
					if ((!IsIntegerType(instruction.type) && instruction.type != Type::Ptr) ||
						instruction.operands.begin()[0].type != instruction.type ||
						instruction.operands.begin()[1].type != instruction.type)
						return Error("icmp type mismatch");
					if (instruction.type == Type::Ptr &&
						instruction.predicate != ICmpPredicate::EQ &&
						instruction.predicate != ICmpPredicate::NE)
						return Error("pointer icmp requires eq or ne");
					instruction.result.type = Type::I1;
				}
				else if (!StrCompare(operation.reference(), "fcmp")) {
					instruction.opcode = Opcode::FCmp;
					if (!has_result || !ReadFloatPredicate(instruction.float_predicate) ||
						!ReadType(instruction.type) || !ReadBinaryOperands(instruction) ||
						!IsFloatType(instruction.type) ||
						instruction.operands.begin()[0].type != instruction.type ||
						instruction.operands.begin()[1].type != instruction.type)
						return Error("invalid fcmp instruction");
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
				else if (!StrCompare(operation.reference(), "inttoptr") ||
					!StrCompare(operation.reference(), "ptrtoint")) {
					bool integer_to_pointer = !StrCompare(
						operation.reference(), "inttoptr");
					instruction.opcode = integer_to_pointer ?
						Opcode::IntToPtr : Opcode::PtrToInt;
					Type source_type;
					Value operand;
					if (!has_result || !ReadType(source_type) || !ReadValue(operand) ||
						!Expect("to") || !ReadType(instruction.type))
						return Error("invalid pointer conversion instruction");
					if (operand.type != source_type ||
						(integer_to_pointer &&
							(!IsIntegerType(source_type) || instruction.type != Type::Ptr)) ||
						(!integer_to_pointer &&
							(source_type != Type::Ptr || !IsIntegerType(instruction.type))))
						return Error("pointer conversion type mismatch");
					instruction.operands.Append(operand);
					instruction.result.type = instruction.type;
				}
				else if (!StrCompare(operation.reference(), "sitofp") ||
					!StrCompare(operation.reference(), "uitofp") ||
					!StrCompare(operation.reference(), "fptosi") ||
					!StrCompare(operation.reference(), "fptoui") ||
					!StrCompare(operation.reference(), "fpext") ||
					!StrCompare(operation.reference(), "fptrunc")) {
					bool signed_to_float = !StrCompare(operation.reference(), "sitofp");
					bool unsigned_to_float = !StrCompare(operation.reference(), "uitofp");
					bool float_to_signed = !StrCompare(operation.reference(), "fptosi");
					bool float_to_unsigned = !StrCompare(operation.reference(), "fptoui");
					bool extend = !StrCompare(operation.reference(), "fpext");
					instruction.opcode = signed_to_float ? Opcode::SIToFP :
						unsigned_to_float ? Opcode::UIToFP :
						float_to_signed ? Opcode::FPToSI :
						float_to_unsigned ? Opcode::FPToUI :
						extend ? Opcode::FPExt : Opcode::FPTrunc;
					Type source_type;
					Value operand;
					if (!has_result || !ReadType(source_type) || !ReadValue(operand) ||
						!Expect("to") || !ReadType(instruction.type) || operand.type != source_type)
						return Error("invalid floating conversion instruction");
					if (((signed_to_float || unsigned_to_float) &&
						(!IsIntegerType(source_type) || !IsFloatType(instruction.type))) ||
						((float_to_signed || float_to_unsigned) &&
						(!IsFloatType(source_type) || !IsIntegerType(instruction.type))) ||
						(extend && (!IsFloatType(source_type) || !IsFloatType(instruction.type) ||
							FloatBits(source_type) >= FloatBits(instruction.type))) ||
						(!signed_to_float && !unsigned_to_float && !float_to_signed &&
							!float_to_unsigned && !extend &&
							(!IsFloatType(source_type) || !IsFloatType(instruction.type) ||
								FloatBits(source_type) <= FloatBits(instruction.type))))
						return Error("floating conversion type mismatch");
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
				else if (!StrCompare(operation.reference(), "funcaddr")) {
					instruction.opcode = Opcode::FunctionAddr;
					if (!has_result || !Expect("@") || !ReadName(instruction.name))
						return Error("invalid funcaddr instruction");
					instruction.type = Type::Ptr;
					instruction.result.type = Type::Ptr;
				}
				else if (!StrCompare(operation.reference(), "call")) {
					instruction.opcode = Opcode::Call;
					if (Is("byval")) {
						Advance();
						instruction.by_value_return = true;
						instruction.has_object_type = true;
						instruction.type = Type::Ptr;
						if (!ReadTypeReference(*current_module, instruction.object_type) ||
							!instruction.object_type.IsDefined())
							return Error("invalid aggregate call result type");
					}
					else if (!ReadType(instruction.type))
						return Error("invalid call instruction");
					if (Is("@")) {
						Advance();
						if (!ReadName(instruction.callee))
							return Error("invalid call target");
					}
					else {
						if (!ReadValue(instruction.callee_value) ||
							instruction.callee_value.type != Type::Ptr)
							return Error("indirect call target must be ptr");
						instruction.has_callee_value = true;
					}
					if (!ReadCallOperands(instruction))
						return Error("invalid call instruction");
					if ((instruction.type == Type::Void &&
						!instruction.by_value_return) == has_result)
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
					if (has_result)
						return Error("invalid ret instruction");
					if (Is("byval")) {
						Advance();
						instruction.by_value_return = true;
						instruction.has_object_type = true;
						instruction.type = Type::Ptr;
						if (!ReadTypeReference(*current_module, instruction.object_type) ||
							!instruction.object_type.IsDefined())
							return Error("invalid aggregate return type");
					}
					else if (!ReadType(instruction.type))
						return Error("invalid ret instruction");
					if (instruction.type != Type::Void || instruction.by_value_return) {
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

			bool ParseHardwareOperation(HardwareModule& module, HardwareBlock& block) {
				HardwareOperation operation;
				unsigned result_id = 0;
				bool has_result = false;
				if (Is("%")) {
					if (!ReadResultID(result_id) || !Expect("=")) return false;
					has_result = true;
				}
				uni::String operation_name;
				if (!ReadName(operation_name)) return false;
				operation.has_result = has_result;
				if (!StrCompare(operation_name.reference(), "read")) {
					operation.opcode = HardwareOpcode::Read;
					if (!has_result || !ReadHardwareType(*current_module, operation.result.type) ||
						!Expect("@") || !ReadName(operation.symbol))
						return Error("invalid hardware read operation");
					const HardwarePort* port = FindHardwarePort(module, operation.symbol);
					if (!port) return Error("read from undefined hardware port");
					if (!SameType(port->type, operation.result.type))
						return Error("hardware read width does not match port");
				}
				else if (!StrCompare(operation_name.reference(), "const")) {
					operation.opcode = HardwareOpcode::Const;
					if (!has_result || !ReadHardwareType(*current_module, operation.result.type) ||
						!ReadHardwareNumber(operation.immediate))
						return Error("invalid hardware constant operation");
					if (operation.result.type.bit_width < 64 &&
						operation.immediate >= ((uint64)1 << operation.result.type.bit_width))
						return Error("hardware constant does not fit result width");
				}
				else if (!StrCompare(operation_name.reference(), "add")) {
					operation.opcode = HardwareOpcode::Add;
					HardwareValue left, right;
					if (!has_result || !ReadHardwareType(*current_module, operation.result.type) ||
						!ReadHardwareValue(left) || !Expect(",") ||
						!ReadHardwareValue(right))
						return Error("invalid hardware add operation");
					if (!SameType(left.type, operation.result.type) ||
						!SameType(right.type, operation.result.type))
						return Error("hardware add operands must match result width");
					operation.operands.Append(left);
					operation.operands.Append(right);
				}
				else if (!StrCompare(operation_name.reference(), "assign")) {
					operation.opcode = HardwareOpcode::Assign;
					HardwareValue value;
					if (has_result || !Expect("@") || !ReadName(operation.symbol) ||
						!Expect(",") || !ReadHardwareValue(value))
						return Error("invalid hardware assign operation");
					const HardwarePort* port = FindHardwarePort(module, operation.symbol);
					if (!port) return Error("assign to undefined hardware port");
					if (port->direction == PortDirection::Input)
						return Error("cannot assign an input hardware port");
					if (!SameType(port->type, value.type))
						return Error("hardware assignment width does not match port");
					if (HasHardwareDriver(module, operation.symbol) ||
						HasHardwareDriver(block, operation.symbol))
						return Error("hardware port has multiple drivers");
					operation.operands.Append(value);
				}
				else return Error("unsupported hardware operation");
				if (has_result) {
					operation.result.id = result_id;
					if (!DefineHardwareValue(operation.result)) return false;
				}
				block.operations.Append(operation);
				return true;
			}

			bool ParseHardwareBlock(HardwareModule& module) {
				HardwareBlock block;
				if (!Expect("^") || !ReadName(block.name) || !Expect(":")) return false;
				for (const HardwareBlock& previous : module.blocks)
					if (!StrCompare(previous.name.reference(), block.name.reference()))
						return Error("duplicate hardware block name");
				while (token.getByteCount() && !Is("^") && !Is("}"))
					if (!ParseHardwareOperation(module, block)) return false;
				module.blocks.Append(block);
				return true;
			}

			bool ParseHardwareModule(Module& module) {
				HardwareModule hardware_module;
				hardware_definitions.Clear();
				if (!Expect("module") || !Expect("@") || !ReadName(hardware_module.name) ||
					!Expect("{")) return false;
				for (const HardwareModule& previous : module.hardware_modules)
					if (!StrCompare(previous.name.reference(), hardware_module.name.reference()))
						return Error("duplicate hardware module name");
				while (Is("port")) {
					HardwarePort port;
					Advance();
					if (!Expect("@") || !ReadName(port.name) ||
						!ReadHardwareType(module, port.type))
						return Error("invalid hardware port declaration");
					if (Is("input")) port.direction = PortDirection::Input;
					else if (Is("output")) port.direction = PortDirection::Output;
					else if (Is("inout")) port.direction = PortDirection::InOut;
					else return Error("expected hardware port direction");
					Advance();
					if (FindHardwarePort(hardware_module, port.name))
						return Error("duplicate hardware port name");
					hardware_module.ports.Append(port);
				}
				while (Is("^"))
					if (!ParseHardwareBlock(hardware_module)) return false;
				if (!hardware_module.blocks.Count()) return Error("hardware module requires a block");
				if (!Expect("}")) return false;
				module.hardware_modules.Append(hardware_module);
				return true;
			}

			bool ParseFunction(Module& module, Linkage linkage = Linkage::External) {
				Function function;
				function.linkage = linkage;
				definitions.Clear();
				debug_file = 0;
				debug_line = 0;
				if (!Expect("func") || !Expect("@") || !ReadName(function.name) ||
					!Expect("(")) return false;
				while (!Is(")")) {
					if (Is("...")) {
						Advance();
						function.is_variadic = true;
						if (!Is(")")) return Error("ellipsis must end function parameters");
						break;
					}
					Parameter parameter;
					if (Is("byval")) {
						Advance();
						parameter.by_value = true;
						parameter.value.type = Type::Ptr;
						if (!ReadTypeReference(module, parameter.object_type) ||
							!parameter.object_type.IsDefined() ||
						!ReadResultID(parameter.value.id) || !Define(parameter.value))
							return Error("invalid by-value function parameter");
					}
					else if (!ReadType(parameter.value.type) ||
						!ReadResultID(parameter.value.id) || !Define(parameter.value)) return false;
					if (!parameter.by_value && Is("abiagg")) {
						Advance();
						stdsint group;
						if (!ReadNumber(group) || group <= 0) return Error("invalid aggregate parameter group");
						parameter.aggregate_group = (unsigned)group;
					}
					function.parameters.Append(parameter);
					if (Is(")")) break;
					if (!Expect(",")) return false;
				}
				if (!Expect(")") || !Expect("->")) return false;
				if (Is("byval")) {
					Advance();
					function.returns_by_value = true;
					function.return_type = Type::Ptr;
					if (!ReadTypeReference(module, function.return_object_type) ||
						!function.return_object_type.IsDefined())
						return Error("invalid aggregate function return type");
				}
				else if (!ReadType(function.return_type)) return false;
				if (!ReadSymbolAttributes(function.visibility, function.binding) ||
					!Expect("{")) return false;
				while (Is("^"))
					if (!ParseBlock(function)) return false;
				if (!Expect("}")) return false;
				for (const BasicBlock& block : function.blocks) {
					for (const Instruction& instruction : block.instructions) {
						if (instruction.opcode != Opcode::Ret) continue;
						bool valid = instruction.type == function.return_type &&
							instruction.by_value_return == function.returns_by_value &&
							instruction.operands.Count() ==
								(function.return_type == Type::Void ? 0 : 1);
						if (valid && function.returns_by_value)
							valid = !StrCompare(instruction.object_type.defined_name.reference(),
								function.return_object_type.defined_name.reference()) &&
								instruction.operands.begin()[0].type == Type::Ptr;
						else if (valid && instruction.type != Type::Void)
							valid = instruction.operands.begin()[0].type == instruction.type;
						if (!valid) return Error("return instruction does not match function type");
					}
				}
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

			bool ParseFunctionDeclaration(Module& module) {
				Function function;
				function.is_declaration = true;
				if (!Expect("func") || !Expect("@") || !ReadName(function.name) ||
					!Expect("(")) return false;
				while (!Is(")")) {
					if (Is("...")) {
						Advance();
						function.is_variadic = true;
						if (!Is(")")) return Error("ellipsis must end function parameters");
						break;
					}
					Parameter parameter;
					if (Is("byval")) {
						Advance();
						parameter.by_value = true;
						parameter.value.type = Type::Ptr;
						if (!ReadTypeReference(module, parameter.object_type) ||
							!parameter.object_type.IsDefined())
							return Error("invalid by-value function parameter");
					}
					else if (!ReadType(parameter.value.type)) return false;
					parameter.value.id = function.parameters.Count();
					if (!parameter.by_value && Is("abiagg")) {
						Advance();
						stdsint group;
						if (!ReadNumber(group) || group <= 0)
							return Error("invalid aggregate parameter group");
						parameter.aggregate_group = (unsigned)group;
					}
					function.parameters.Append(parameter);
					if (Is(")")) break;
					if (!Expect(",")) return false;
				}
				if (!Expect(")") || !Expect("->")) return false;
				if (Is("byval")) {
					Advance();
					function.returns_by_value = true;
					function.return_type = Type::Ptr;
					if (!ReadTypeReference(module, function.return_object_type) ||
						!function.return_object_type.IsDefined())
						return Error("invalid aggregate function return type");
				}
				else if (!ReadType(function.return_type)) return false;
				if (!ReadSymbolAttributes(function.visibility, function.binding)) return false;
				module.functions.Append(function);
				return true;
			}

			bool ParseSource(Module& module) {
				SourceFile source;
				if (!Expect("source") || !ReadDebugNumber(source.id) ||
					!ReadName(source.path) || !source.path.getByteCount())
					return Error("invalid source declaration");
				for (const SourceFile& previous : module.source_files)
					if (previous.id == source.id) return Error("duplicate source identifier");
				module.source_files.Append(source);
				return true;
			}

			bool ReadLayoutSize(stduint& bits, bool alignment = false) {
				bool bytes = Is("byt");
				if (!bytes && !Is("bit"))
					return Error(alignment ? "alignment requires byt" :
						"size requires bit or byt");
				if (alignment && !bytes) return Error("alignment requires byt");
				Advance();
				stdsint value;
				if (!ReadNumber(value) || value <= 0)
					return Error("layout size must be positive");
				bits = (stduint)value;
				if (bytes) {
					if (bits > stduint(-1) / 8) return Error("layout size is too large");
					bits *= 8;
				}
				if (alignment) {
					stduint alignment_bytes = bits / 8;
					if (alignment_bytes & (alignment_bytes - 1))
						return Error("alignment must be a power of two bytes");
				}
				return true;
			}

			bool ParseTypeDefinition(Module& module) {
				TypeDefinition definition;
				if (!Expect("type") || !Expect("@") || !ReadName(definition.name) ||
					!Expect("=")) return false;
				if (FindTypeDefinition(module, definition.name.reference()))
					return Error("duplicate Clover type definition");
				if (Is("struct")) definition.kind = TypeDefinitionKind::Structure;
				else if (Is("union")) definition.kind = TypeDefinitionKind::Union;
				else return Error("expected struct or union type definition");
				Advance();
				if (!Expect("size") || !ReadLayoutSize(definition.size_bits)) return false;
				if (Is("align")) {
					Advance();
					if (!ReadLayoutSize(definition.alignment_bits, true)) return false;
				}
				if (!Expect("{")) return false;
				module.type_definitions.Append(definition);
				TypeDefinition& output = module.type_definitions[
					module.type_definitions.Count() - 1];
				stduint structure_offset = 0;
				stduint union_size = 0;
				while (!Is("}")) {
					if (!token.getByteCount()) return Error("unterminated type definition");
					TypeMember member;
					if (Is("@")) {
						if (!ReadTypeReference(module, member.type)) return false;
					}
					else {
						uni::String first = token;
						Advance();
						if (Is(":")) {
							member.name = first;
							Advance();
							if (!ReadTypeReference(module, member.type)) return false;
						}
						else if (!ReadTypeFromFirst(first, member.type)) return false;
					}
					if (member.type == Type::Void)
						return Error("void cannot be a type member");
					if (member.name.getByteCount())
						for (const TypeMember& previous : output.members)
							if (previous.name.getByteCount() && !StrCompare(
								previous.name.reference(), member.name.reference()))
								return Error("duplicate type member name");
					stduint member_size;
					if (!TypeSizeBits(module, member.type, member_size) || !member_size)
						return Error("type member has invalid size");
					if (output.kind == TypeDefinitionKind::Structure) {
						member.offset_bits = structure_offset;
						if (structure_offset > stduint(-1) - member_size)
							return Error("struct layout is too large");
						structure_offset += member_size;
					}
					else if (member_size > union_size) union_size = member_size;
					output.members.Append(member);
				}
				Advance();
				if (!output.members.Count()) return Error("empty aggregate type is not supported");
				stduint occupied = output.kind == TypeDefinitionKind::Structure ?
					structure_offset : union_size;
				if (occupied != output.size_bits)
					return Error("aggregate size does not match its members");
				return true;
			}

			bool ParseTarget(Module& module) {
				if (!Expect("target")) return false;
				if (Is("relocation")) {
					Advance();
					if (relocation_model_seen) return Error("duplicate relocation model");
					if (Is("static")) module.relocation_model = RelocationModel::Static;
					else if (Is("pic")) module.relocation_model = RelocationModel::PIC;
					else if (Is("pie")) module.relocation_model = RelocationModel::PIE;
					else return Error("expected static, pic, or pie relocation model");
					Advance();
					relocation_model_seen = true;
					return true;
				}
				if (!Expect("ptrbits")) return false;
				stdsint pointer_bits;
				if (!ReadNumber(pointer_bits) ||
					(pointer_bits != 32 && pointer_bits != 64))
					return Error("target pointer width must be 32 or 64");
				if (module.pointer_bits) return Error("duplicate target pointer width");
				module.pointer_bits = (unsigned)pointer_bits;
				return true;
			}

			bool ReadSymbolAttributes(SymbolVisibility& visibility,
				SymbolBinding& binding) {
				bool visibility_seen = false;
				bool binding_seen = false;
				while (Is("visibility") || Is("binding")) {
					if (Is("visibility")) {
						if (visibility_seen) return Error("duplicate symbol visibility");
						Advance();
						if (Is("default")) visibility = SymbolVisibility::Default;
						else if (Is("hidden")) visibility = SymbolVisibility::Hidden;
						else if (Is("protected")) visibility = SymbolVisibility::Protected;
						else return Error("expected default, hidden, or protected visibility");
						Advance();
						visibility_seen = true;
					}
					else {
						if (binding_seen) return Error("duplicate symbol binding");
						Advance();
						if (Is("strong")) binding = SymbolBinding::Strong;
						else if (Is("weak")) binding = SymbolBinding::Weak;
						else return Error("expected strong or weak binding");
						Advance();
						binding_seen = true;
					}
				}
				return true;
			}

			bool ParseGlobal(Module& module, bool read_only,
				Linkage linkage = Linkage::External, bool declaration = false) {
				Global global;
				global.is_read_only = read_only;
				global.linkage = linkage;
				global.is_declaration = declaration;
				if (!Expect(read_only ? "constant" : "global") ||
					!Expect("@") || !ReadName(global.name) ||
					!ReadTypeReference(module, global.type) || IsVoidObjectType(global.type) ||
					!ReadSymbolAttributes(global.visibility, global.binding))
					return Error("invalid global declaration");
				if (Is(",")) {
					Advance();
					if (!ReadNumber(global.element_count) || global.element_count <= 0)
						return Error("invalid global element count");
				}
				if (Is("align")) {
					Advance();
					stdsint alignment;
					if (!ReadNumber(alignment) || alignment <= 0 ||
						(alignment & (alignment - 1)))
						return Error("global alignment must be a positive power of two");
					global.alignment = (stduint)alignment;
				}
				if (declaration) {
					module.globals.Append(global);
					return true;
				}
				if (!Expect("=")) return Error("invalid global initializer");
				if (Is("data")) {
					Advance();
					if (!Expect("{")) return false;
					while (!Is("}")) {
						GlobalInitializerFragment fragment;
						stdsint offset;
						if (!ReadNumber(offset) || offset < 0 || !Expect(":") ||
							!ReadType(fragment.type) || fragment.type == Type::Void)
							return Error("invalid global initializer fragment");
						fragment.offset = (stduint)offset;
						if (Is("@")) {
							if (fragment.type != Type::Ptr || !ReadSymbolInitializer(
								fragment.initializer_symbol,
								fragment.initializer_addend))
								return Error("symbol fragment requires ptr type");
						}
						else if (!ReadNumber(fragment.initializer))
							return Error("invalid global initializer fragment");
						if (!fragment.initializer_symbol.getByteCount() &&
							fragment.type == Type::F128 &&
							(!Expect(",") || !ReadNumber(fragment.initializer_high)))
							return Error("f128 global fragment requires two 64-bit words");
						global.initializer_fragments.Append(fragment);
						if (Is("}")) break;
						if (!Expect(",")) return false;
					}
					if (!Expect("}")) return false;
				}
				else if (Is("{")) {
					Advance();
					while (!Is("}")) {
						stdsint value;
						if (!ReadNumber(value)) return false;
						global.initializer_values.Append(value);
						if (Is("}")) break;
						if (!Expect(",")) return false;
					}
					stduint array_elements;
					if (!Expect("}") || !TypeArrayElementCount(global.type, array_elements) ||
						array_elements > stduint(-1) / (stduint)global.element_count ||
						global.initializer_values.Count() !=
							array_elements * (stduint)global.element_count)
						return Error("global initializer element count mismatch");
				}
				else {
					if (Is("@")) {
						if (global.type != Type::Ptr || !ReadSymbolInitializer(
							global.initializer_symbol, global.initializer_addend))
							return Error("symbol initializer requires ptr type");
					}
					else if (!ReadNumber(global.initializer))
						return Error("invalid global initializer");
					if (!global.type.IsScalar() && global.initializer)
						return Error("aggregate direct initializer must be zero");
					if (global.type == Type::F128 &&
						(!Expect(",") || !ReadNumber(global.initializer_high)))
						return Error("f128 global initializer requires two 64-bit words");
				}
				module.globals.Append(global);
				return true;
			}

		public:
			Parser(uni::IstreamTrait& input) : lexer(input) {
				Advance();
			}

			bool ParseModule(Module& module) {
				current_module = &module;
				module.ir_class = IRClass::Soft;
				module.pointer_bits = 0;
				module.relocation_model = RelocationModel::Static;
				relocation_model_seen = false;
				ir_class_seen = false;
				module.source_files.Clear();
				module.type_definitions.Clear();
				module.globals.Clear();
				module.functions.Clear();
				module.hardware_modules.Clear();
				while (token.getByteCount()) {
					if (Is("type")) {
						if (!ParseTypeDefinition(module)) return false;
					}
					else if (Is("module")) {
						if (!SelectIRClass(module, IRClass::Hard) ||
							!ParseHardwareModule(module)) return false;
					}
					else if (Is("target")) {
						if (!SelectIRClass(module, IRClass::Soft)) return false;
						if (!ParseTarget(module)) return false;
					}
					else if (Is("source")) {
						if (!SelectIRClass(module, IRClass::Soft)) return false;
						if (!ParseSource(module)) return false;
					}
					else if (Is("internal")) {
						if (!SelectIRClass(module, IRClass::Soft)) return false;
						Advance();
						if (Is("global") || Is("constant")) {
							bool read_only = Is("constant");
							if (!ParseGlobal(module, read_only, Linkage::Internal)) return false;
						}
						else if (Is("func")) {
							if (!ParseFunction(module, Linkage::Internal)) return false;
						}
						else return Error("internal requires a global, constant, or function");
					}
					else if (Is("declare")) {
						if (!SelectIRClass(module, IRClass::Soft)) return false;
						Advance();
						if (Is("func")) {
							if (!ParseFunctionDeclaration(module)) return false;
						}
						else {
							if (!Is("global") && !Is("constant"))
								return Error("declare requires a global, constant, or function");
							bool read_only = Is("constant");
							if (!ParseGlobal(module, read_only,
								Linkage::External, true)) return false;
						}
					}
					else if (Is("global") || Is("constant")) {
						if (!SelectIRClass(module, IRClass::Soft)) return false;
						bool read_only = Is("constant");
						if (!ParseGlobal(module, read_only)) return false;
					}
					else {
						if (!SelectIRClass(module, IRClass::Soft) || !ParseFunction(module))
							return false;
					}
				}
				return !failed && ValidateTypeDefinitions(module);
			}

			bool ValidateTypeDefinition(const Module& module, stduint index,
				uni::Vector<unsigned>& states) {
				if (states[index] == 1) return Error("recursive aggregate value type");
				if (states[index] == 2) return true;
				states[index] = 1;
				const TypeDefinition& definition = module.type_definitions.begin()[index];
				for (const TypeMember& member : definition.members) {
					if (!member.type.IsDefined()) continue;
					stduint target = module.type_definitions.Count();
					for (stduint candidate = 0;
						candidate < module.type_definitions.Count(); candidate++)
						if (!StrCompare(module.type_definitions.begin()[candidate].name.reference(),
							member.type.defined_name.reference())) {
							target = candidate;
							break;
						}
					if (target == module.type_definitions.Count())
						return Error("reference to undefined Clover type");
					if (!ValidateTypeDefinition(module, target, states)) return false;
				}
				states[index] = 2;
				return true;
			}

			bool ValidateTypeDefinitions(const Module& module) {
				uni::Vector<unsigned> states = uni::Vector<unsigned>::From(
					0, module.type_definitions.Count());
				for (stduint index = 0; index < module.type_definitions.Count(); index++)
					if (!ValidateTypeDefinition(module, index, states)) return false;
				return true;
			}
		};

	}

	const TypeDefinition* FindTypeDefinition(const Module& module, const char* name) {
		if (!name) return nullptr;
		for (const TypeDefinition& definition : module.type_definitions)
			if (!StrCompare(definition.name.reference(), name)) return &definition;
		return nullptr;
	}

	TypeDefinition* FindTypeDefinition(Module& module, const char* name) {
		if (!name) return nullptr;
		for (TypeDefinition& definition : module.type_definitions)
			if (!StrCompare(definition.name.reference(), name)) return &definition;
		return nullptr;
	}

	bool SameType(const TypeReference& left, const TypeReference& right) {
		if (left.scalar != right.scalar || left.bit_width != right.bit_width ||
			left.array_depth != right.array_depth ||
			StrCompare(left.defined_name.reference(), right.defined_name.reference()))
			return false;
		for (unsigned index = 0; index < left.array_depth; index++)
			if (left.array_counts[index] != right.array_counts[index]) return false;
		return true;
	}

	bool TypeSizeBits(const Module& module, const TypeReference& type, stduint& bits) {
		if (type.IsBits()) bits = type.bit_width;
		else if (type.IsDefined()) {
			const TypeDefinition* definition = FindTypeDefinition(
				module, type.defined_name.reference());
			if (!definition || !definition->size_bits) return false;
			bits = definition->size_bits;
		}
		else switch (type.scalar) {
			case Type::Void: return false;
			case Type::I1: bits = 1; break;
			case Type::I8: bits = 8; break;
			case Type::I16: bits = 16; break;
			case Type::I32:
			case Type::F32: bits = 32; break;
			case Type::I64:
			case Type::F64: bits = 64; break;
			case Type::F128: bits = 128; break;
			case Type::Ptr:
				if (!module.pointer_bits) return false;
				bits = module.pointer_bits;
				break;
		}
		for (unsigned index = 0; index < type.array_depth; index++) {
			stduint count = type.array_counts[index];
			if (!count || bits > stduint(-1) / count) return false;
			bits *= count;
		}
		return bits != 0;
	}

	bool TypeAlignmentBits(const Module& module, const TypeReference& type,
		stduint& bits) {
		if (type.IsBits()) bits = 1;
		else if (type.IsDefined()) {
			const TypeDefinition* definition = FindTypeDefinition(
				module, type.defined_name.reference());
			if (!definition) return false;
			bits = definition->alignment_bits;
		}
		else switch (type.scalar) {
			case Type::Void: return false;
			case Type::I1: bits = 1; break;
			case Type::I8: bits = 8; break;
			case Type::I16: bits = 16; break;
			case Type::I32:
			case Type::F32: bits = 32; break;
			case Type::I64:
			case Type::F64: bits = 64; break;
			case Type::F128: bits = 128; break;
			case Type::Ptr:
				if (!module.pointer_bits) return false;
				bits = module.pointer_bits;
				break;
		}
		return true;
	}

	bool Parse(uni::IstreamTrait& input, Module& module) {
		Parser parser(input);
		return parser.ParseModule(module);
	}

	bool Print(const Module& module, uni::OstreamTrait& output) {
		if (module.ir_class == IRClass::Hard) {
			PrintTypeDefinitions(module, output);
			for (const HardwareModule& hardware_module : module.hardware_modules) {
				output.OutFormat("module @%s {\n", hardware_module.name.reference());
				for (const HardwarePort& port : hardware_module.ports) {
					output.OutFormat("    port @%s ", port.name.reference());
					PrintTypeReference(output, port.type);
					output.OutFormat(" %s\n", PortDirectionName(port.direction));
				}
				if (hardware_module.ports.Count() && hardware_module.blocks.Count())
					output.OutChar('\n');
				for (const HardwareBlock& block : hardware_module.blocks) {
					output.OutFormat("^%s:\n", block.name.reference());
					for (const HardwareOperation& operation : block.operations) {
						output.OutFormat("    ");
						if (operation.has_result) {
							PrintHardwareValue(output, operation.result);
							output.OutFormat(" = ");
						}
						switch (operation.opcode) {
						case HardwareOpcode::Read:
							output.OutFormat("read ");
							PrintTypeReference(output, operation.result.type);
							output.OutFormat(" @%s\n", operation.symbol.reference());
							break;
						case HardwareOpcode::Const:
							output.OutFormat("const ");
							PrintTypeReference(output, operation.result.type);
							output.OutFormat(" %[u]\n", (stduint)operation.immediate);
							break;
						case HardwareOpcode::Add:
							output.OutFormat("add ");
							PrintTypeReference(output, operation.result.type);
							output.OutFormat(" ");
							PrintHardwareValue(output, operation.operands.begin()[0]);
							output.OutFormat(", ");
							PrintHardwareValue(output, operation.operands.begin()[1]);
							output.OutChar('\n');
							break;
						case HardwareOpcode::Assign:
							output.OutFormat("assign @%s, ", operation.symbol.reference());
							PrintHardwareValue(output, operation.operands.begin()[0]);
							output.OutChar('\n');
							break;
						}
					}
				}
				output.OutFormat("}\n");
			}
			return true;
		}
		if (module.ir_class != IRClass::Soft) {
			plogerro("Clover IR: Exec IR text output is not implemented");
			return false;
		}
		if (module.relocation_model != RelocationModel::Static)
			output.OutFormat("target relocation %s\n",
				RelocationModelName(module.relocation_model));
		if (module.pointer_bits)
			output.OutFormat("target ptrbits %[u]\n", module.pointer_bits);
		for (const SourceFile& source : module.source_files) {
			output.OutFormat("source %[u] ", source.id);
			PrintQuoted(output, source.path);
			output.OutChar('\n');
		}
		PrintTypeDefinitions(module, output);
		for (const Global& global : module.globals) {
			if (global.is_declaration) {
				output.OutFormat("declare %s @%s ", global.is_read_only ?
					"constant" : "global", global.name.reference());
				PrintTypeReference(output, global.type);
				PrintSymbolAttributes(output, global.visibility, global.binding);
				if (global.element_count > 1) output.OutFormat(", %[i]", global.element_count);
				if (global.alignment) output.OutFormat(" align %[u]", global.alignment);
				output.OutChar('\n');
				continue;
			}
			if (global.linkage == Linkage::Internal) output.OutFormat("internal ");
			output.OutFormat("%s @%s ", global.is_read_only ? "constant" : "global",
				global.name.reference());
			PrintTypeReference(output, global.type);
			PrintSymbolAttributes(output, global.visibility, global.binding);
			if (global.element_count > 1) output.OutFormat(", %[i]", global.element_count);
			if (global.alignment) output.OutFormat(" align %[u]", global.alignment);
			output.OutFormat(" = ");
			if (global.initializer_fragments.Count()) {
				output.OutFormat("data {");
				for (stduint index = 0;
					index < global.initializer_fragments.Count(); index++) {
					if (index) output.OutFormat(", ");
					const GlobalInitializerFragment& fragment =
						global.initializer_fragments.begin()[index];
					output.OutFormat("%[u]: %s ", fragment.offset,
						TypeName(fragment.type));
					if (fragment.initializer_symbol.getByteCount())
						PrintSymbolInitializer(output, fragment.initializer_symbol,
							fragment.initializer_addend);
					else output.OutFormat("%[i]", fragment.initializer);
					if (!fragment.initializer_symbol.getByteCount() &&
						fragment.type == Type::F128)
						output.OutFormat(", %[i]", fragment.initializer_high);
				}
				output.OutFormat("}\n");
			}
			else if (global.initializer_values.Count()) {
				output.OutFormat("{");
				for (stduint index = 0; index < global.initializer_values.Count(); index++) {
					if (index) output.OutFormat(", ");
					output.OutFormat("%[i]", global.initializer_values.begin()[index]);
				}
				output.OutFormat("}\n");
			}
			else {
				if (global.initializer_symbol.getByteCount())
					PrintSymbolInitializer(output, global.initializer_symbol,
						global.initializer_addend);
				else output.OutFormat("%[i]", global.initializer);
				if (global.type == Type::F128)
					output.OutFormat(", %[i]", global.initializer_high);
				output.OutFormat("\n");
			}
		}
		for (const Function& function : module.functions) {
			uint32 current_file = 0;
			uint32 current_line = 0;
			if (function.is_declaration) output.OutFormat("declare ");
			else if (function.linkage == Linkage::Internal) output.OutFormat("internal ");
			output.OutFormat("func @%s(", function.name.reference());
			for (stduint index = 0; index < function.parameters.Count(); index++) {
				if (index) output.OutFormat(", ");
				const Parameter& parameter = function.parameters.begin()[index];
				if (parameter.by_value) {
					output.OutFormat("byval ");
					PrintTypeReference(output, parameter.object_type);
				}
				else output.OutFormat("%s", TypeName(parameter.value.type));
				if (!function.is_declaration) {
					output.OutFormat(" ");
					PrintValue(output, parameter.value);
				}
				if (parameter.aggregate_group)
					output.OutFormat(" abiagg %[u]", parameter.aggregate_group);
			}
			if (function.is_variadic)
				output.OutFormat(function.parameters.Count() ? ", ..." : "...");
			output.OutFormat(") -> ");
			if (function.returns_by_value) {
				output.OutFormat("byval ");
				PrintTypeReference(output, function.return_object_type);
			}
			else output.OutFormat("%s", TypeName(function.return_type));
			PrintSymbolAttributes(output, function.visibility, function.binding);
			if (function.is_declaration) {
				output.OutFormat("\n");
				continue;
			}
			output.OutFormat(" {\n");
			for (const BasicBlock& block : function.blocks) {
				output.OutFormat("^%s:\n", block.name.reference());
				for (const Instruction& instruction : block.instructions)
					if (!PrintInstruction(instruction, output,
						current_file, current_line)) return false;
			}
			output.OutFormat("}\n");
		}
		return true;
	}

}
