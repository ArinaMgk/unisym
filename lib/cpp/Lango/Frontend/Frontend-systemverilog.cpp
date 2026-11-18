// ASCII C++11 TAB4 LF
// Docutitle: Lango SystemVerilog Frontend
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

#include "../../../../inc/cpp/Lango/Frontend/Frontend-systemverilog.hpp"

namespace uni::lango {

	namespace {

		struct SVToken {
			uni::String text;
			stduint row = 1;
			stduint column = 1;
		};

		class SVLexer {
		private:
			uni::IstreamTrait& input;
			int current = -2;
			stduint row = 1;
			stduint column = 0;

			void Read() {
				current = input.inn();
				if (current == '\n') {
					row++;
					column = 0;
				}
				else if (current >= 0) column++;
			}

			bool IsIdentifierStart(int ch) const {
				return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
					ch == '_';
			}

			bool IsIdentifierContinue(int ch) const {
				return IsIdentifierStart(ch) || (ch >= '0' && ch <= '9') || ch == '$';
			}

			void SkipIgnored() {
				for (;;) {
					while (current == ' ' || current == '\t' || current == '\r' ||
						current == '\n') Read();
					if (current != '/') return;
					Read();
					if (current == '/') {
						while (current >= 0 && current != '\n') Read();
						continue;
					}
					if (current == '*') {
						int previous = 0;
						Read();
						while (current >= 0 && !(previous == '*' && current == '/')) {
							previous = current;
							Read();
						}
						if (current == '/') Read();
						continue;
					}
					return;
				}
			}

		public:
			SVLexer(uni::IstreamTrait& source) : input(source) {}

			SVToken Next() {
				if (current == -2) Read();
				SkipIgnored();
				SVToken token;
				token.row = row;
				token.column = column;
				if (current < 0) return token;
				if (IsIdentifierStart(current)) {
					while (IsIdentifierContinue(current)) {
						token.text << (char)current;
						Read();
					}
					return token;
				}
				if (current >= '0' && current <= '9') {
					while (current >= '0' && current <= '9') {
						token.text << (char)current;
						Read();
					}
					return token;
				}
				token.text << (char)current;
				Read();
				return token;
			}
		};

		enum class SVOperandKind {
			Identifier,
			Integer
		};

		struct SVOperand {
			SVOperandKind kind = SVOperandKind::Identifier;
			uni::String name;
			uint64 immediate = 0;
		};

		struct SVAssign {
			uni::String target;
			SVOperand left;
			SVOperand right;
			bool has_add = false;
		};

		struct SVPort {
			uni::String name;
			stduint width = 1;
			clover::PortDirection direction = clover::PortDirection::Input;
		};

		struct SVModule {
			uni::String name;
			uni::Vector<SVPort> ports;
			uni::Vector<SVAssign> assignments;
		};

		class SVParser {
		private:
			SVLexer lexer;
			SVToken token;
			DiagnosticInterface* diagnostics;
			const char* source_name;
			bool failed = false;

			void Advance() {
				token = lexer.Next();
			}

			bool Is(const char* text) const {
				return !StrCompare(token.text.reference(), text);
			}

			bool Error(const char* message) {
				if (!failed) {
					Diagnostic diagnostic;
					diagnostic.stage = DiagnosticStage::Parser;
					diagnostic.location.file = source_name ? source_name : "<input>";
					diagnostic.location.row = token.row;
					diagnostic.location.column = token.column;
					diagnostic.message = message;
					(diagnostics ? diagnostics : &DefaultDiagnosticInterface())->Report(diagnostic);
				}
				failed = true;
				return false;
			}

			bool Expect(const char* text) {
				if (!Is(text)) return Error("unexpected SystemVerilog token");
				Advance();
				return true;
			}

			bool ReadIdentifier(uni::String& name) {
				const char* text = token.text.reference();
				if (!text[0] || !((text[0] >= 'a' && text[0] <= 'z') ||
					(text[0] >= 'A' && text[0] <= 'Z') || text[0] == '_'))
					return Error("expected SystemVerilog identifier");
				name = token.text;
				Advance();
				return true;
			}

			bool ReadUnsigned(uint64& value) {
				char* end = nullptr;
				value = StrTokenULong(token.text.reference(), &end, 10);
				if (!token.text.getByteCount() || *end)
					return Error("expected decimal integer");
				Advance();
				return true;
			}

			bool ReadOperand(SVOperand& operand) {
				const char* text = token.text.reference();
				if (text[0] >= '0' && text[0] <= '9') {
					operand.kind = SVOperandKind::Integer;
					return ReadUnsigned(operand.immediate);
				}
				operand.kind = SVOperandKind::Identifier;
				return ReadIdentifier(operand.name);
			}

			const SVPort* FindPort(const SVModule& module, const uni::String& name) const {
				for (const SVPort& port : module.ports)
					if (!StrCompare(port.name.reference(), name.reference())) return &port;
				return nullptr;
			}

			bool ValidateOperand(const SVModule& module, const SVOperand& operand,
				stduint width) {
				if (operand.kind == SVOperandKind::Identifier) {
					const SVPort* port = FindPort(module, operand.name);
					if (!port) return Error("SystemVerilog expression references an undefined port");
					if (port->width != width)
						return Error("SystemVerilog expression width does not match assignment target");
					return true;
				}
				if (width < 64 && operand.immediate >= ((uint64)1 << width))
					return Error("SystemVerilog integer does not fit assignment width");
				return true;
			}

			bool ParsePort(SVModule& module) {
				SVPort port;
				if (Is("input")) port.direction = clover::PortDirection::Input;
				else if (Is("output")) port.direction = clover::PortDirection::Output;
				else if (Is("inout")) port.direction = clover::PortDirection::InOut;
				else return Error("expected input, output, or inout port");
				Advance();
				if (!Expect("logic")) return false;
				if (Is("[")) {
					Advance();
					uint64 left, right;
					if (!ReadUnsigned(left) || !Expect(":") || !ReadUnsigned(right) ||
						!Expect("]")) return false;
					if (right != 0 || left >= 64)
						return Error("supported packed range is [N:0] with N below 64");
					port.width = (stduint)left + 1;
				}
				if (!ReadIdentifier(port.name)) return false;
				if (FindPort(module, port.name)) return Error("duplicate SystemVerilog port");
				module.ports.Append(port);
				return true;
			}

			bool ParseAssignment(SVModule& module) {
				SVAssign assignment;
				if (!Expect("assign") || !ReadIdentifier(assignment.target) ||
					!Expect("=") || !ReadOperand(assignment.left)) return false;
				if (Is("+")) {
					Advance();
					assignment.has_add = true;
					if (!ReadOperand(assignment.right)) return false;
				}
				if (!Expect(";")) return false;
				const SVPort* target = FindPort(module, assignment.target);
				if (!target) return Error("assignment target is not a module port");
				if (target->direction == clover::PortDirection::Input)
					return Error("continuous assignment cannot drive an input port");
				if (!ValidateOperand(module, assignment.left, target->width) ||
					(assignment.has_add &&
						!ValidateOperand(module, assignment.right, target->width))) return false;
				for (const SVAssign& previous : module.assignments)
					if (!StrCompare(previous.target.reference(), assignment.target.reference()))
						return Error("module port has multiple continuous assignments");
				module.assignments.Append(assignment);
				return true;
			}

			bool ParseModule(SVModule& module) {
				if (!Expect("module") || !ReadIdentifier(module.name) || !Expect("("))
					return false;
				while (!Is(")")) {
					if (!ParsePort(module)) return false;
					if (Is(")")) break;
					if (!Expect(",")) return false;
				}
				if (!Expect(")") || !Expect(";")) return false;
				while (!Is("endmodule")) {
					if (!token.text.getByteCount()) return Error("unterminated SystemVerilog module");
					if (!Is("assign")) return Error("unsupported SystemVerilog module item");
					if (!ParseAssignment(module)) return false;
				}
				Advance();
				return true;
			}

		public:
			SVParser(uni::IstreamTrait& input, DiagnosticInterface* output,
				const char* file) : lexer(input), diagnostics(output), source_name(file) {
				Advance();
			}

			bool Parse(uni::Vector<SVModule>& modules) {
				while (token.text.getByteCount()) {
					if (!Is("module")) return Error("expected SystemVerilog module");
					SVModule module;
					if (!ParseModule(module)) return false;
					for (const SVModule& previous : modules)
						if (!StrCompare(previous.name.reference(), module.name.reference()))
							return Error("duplicate SystemVerilog module name");
					modules.Append(module);
				}
				return !failed && modules.Count();
			}
		};

		bool ResolveOptions(const FrontendOptions& options) {
			if (options.language_standard &&
				StrCompare(options.language_standard, "systemverilog-subset") &&
				StrCompare(options.language_standard, "systemverilog")) {
				Diagnostic diagnostic;
				diagnostic.stage = DiagnosticStage::Semantic;
				diagnostic.location.file = options.source_name ? options.source_name : "<input>";
				diagnostic.message = "unsupported SystemVerilog standard; supported value: systemverilog-subset";
				(options.diagnostics ? options.diagnostics :
					&DefaultDiagnosticInterface())->Report(diagnostic);
				return false;
			}
			if (options.language_options.Count()) {
				Diagnostic diagnostic;
				diagnostic.stage = DiagnosticStage::Semantic;
				diagnostic.location.file = options.source_name ? options.source_name : "<input>";
				diagnostic.message = "SystemVerilog frontend options are not implemented";
				(options.diagnostics ? options.diagnostics :
					&DefaultDiagnosticInterface())->Report(diagnostic);
				return false;
			}
			return true;
		}

		const SVPort* FindPort(const SVModule& module, const uni::String& name) {
			for (const SVPort& port : module.ports)
				if (!StrCompare(port.name.reference(), name.reference())) return &port;
			return nullptr;
		}

		bool LowerOperand(const SVModule& source, const SVOperand& operand,
			stduint width, clover::HardwareBlock& block, unsigned& next_id,
			clover::HardwareValue& value) {
			clover::HardwareOperation operation;
			operation.has_result = true;
			operation.result.id = next_id++;
			operation.result.type.bit_width = width;
			if (operand.kind == SVOperandKind::Identifier) {
				const SVPort* port = FindPort(source, operand.name);
				if (!port || port->width != width) return false;
				operation.opcode = clover::HardwareOpcode::Read;
				operation.symbol = operand.name;
			}
			else {
				if (width < 64 && operand.immediate >= ((uint64)1 << width)) return false;
				operation.opcode = clover::HardwareOpcode::Const;
				operation.immediate = operand.immediate;
			}
			value = operation.result;
			block.operations.Append(operation);
			return true;
		}

		bool Lower(const uni::Vector<SVModule>& source_modules, clover::Module& module,
			const FrontendOptions& options) {
			module.ir_class = clover::IRClass::Hard;
			for (const SVModule& source : source_modules) {
				clover::HardwareModule target;
				target.name = source.name;
				for (const SVPort& source_port : source.ports) {
					clover::HardwarePort port;
					port.name = source_port.name;
					port.type.bit_width = source_port.width;
					port.direction = source_port.direction;
					target.ports.Append(port);
				}
				clover::HardwareBlock block;
				block.name = "entry";
				unsigned next_id = 0;
				for (const SVAssign& assignment : source.assignments) {
					const SVPort* output = FindPort(source, assignment.target);
					clover::HardwareValue result;
					if (!output || !LowerOperand(source, assignment.left, output->width,
						block, next_id, result)) {
						Diagnostic diagnostic;
						diagnostic.stage = DiagnosticStage::Lowering;
						diagnostic.location.file = options.source_name ? options.source_name : "<input>";
						diagnostic.message = "SystemVerilog expression width does not match assignment target";
						(options.diagnostics ? options.diagnostics :
							&DefaultDiagnosticInterface())->Report(diagnostic);
						return false;
					}
					if (assignment.has_add) {
						clover::HardwareValue right;
						if (!LowerOperand(source, assignment.right, output->width,
							block, next_id, right)) return false;
						clover::HardwareOperation add;
						add.opcode = clover::HardwareOpcode::Add;
						add.has_result = true;
						add.result.id = next_id++;
						add.result.type.bit_width = output->width;
						add.operands.Append(result);
						add.operands.Append(right);
						block.operations.Append(add);
						result = add.result;
					}
					clover::HardwareOperation assign;
					assign.opcode = clover::HardwareOpcode::Assign;
					assign.symbol = assignment.target;
					assign.operands.Append(result);
					block.operations.Append(assign);
				}
				target.blocks.Append(block);
				module.hardware_modules.Append(target);
			}
			return true;
		}

		const char* DirectionName(clover::PortDirection direction) {
			switch (direction) {
			case clover::PortDirection::Input: return "input";
			case clover::PortDirection::Output: return "output";
			case clover::PortDirection::InOut: return "inout";
			}
			return "?";
		}

	}

	bool FrontendSystemVerilog::Generate(IstreamTrait& input, clover::Module& module,
		const FrontendOptions& options) {
		if (!ResolveOptions(options)) return false;
		uni::Vector<SVModule> modules;
		SVParser parser(input, options.diagnostics, options.source_name);
		return parser.Parse(modules) && Lower(modules, module, options);
	}

	bool FrontendSystemVerilog::PrintAST(IstreamTrait& input, OstreamTrait& output,
		const FrontendOptions& options) {
		if (!ResolveOptions(options)) return false;
		uni::Vector<SVModule> modules;
		SVParser parser(input, options.diagnostics, options.source_name);
		if (!parser.Parse(modules)) return false;
		for (const SVModule& module : modules) {
			output.OutFormat("Module %s\n", module.name.reference());
			for (const SVPort& port : module.ports)
				output.OutFormat("  Port %s logic[%[u]] %s\n",
					DirectionName(port.direction), port.width, port.name.reference());
			for (const SVAssign& assignment : module.assignments) {
				output.OutFormat("  Assign %s\n", assignment.target.reference());
				output.OutFormat("    %s\n", assignment.has_add ? "Add" : "Value");
				if (assignment.left.kind == SVOperandKind::Identifier)
					output.OutFormat("      Identifier %s\n", assignment.left.name.reference());
				else output.OutFormat("      Integer %[u]\n", (stduint)assignment.left.immediate);
				if (assignment.has_add) {
					if (assignment.right.kind == SVOperandKind::Identifier)
						output.OutFormat("      Identifier %s\n", assignment.right.name.reference());
					else output.OutFormat("      Integer %[u]\n", (stduint)assignment.right.immediate);
				}
			}
		}
		return true;
	}

}
