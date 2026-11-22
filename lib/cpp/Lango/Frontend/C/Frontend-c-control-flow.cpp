// ASCII C++11 TAB4 LF
// Docutitle: Lango C Frontend Control Flow Analysis
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

namespace uni::lango {

namespace {

	struct ControlStatementAtom {
		uni::Nnode* node = nullptr;
	};

	struct SwitchControlContext {
		stduint entry_state = 0;
	};

	class FunctionControlFlowAnalyzer {
	private:
		const FrontendCDiagnostics& diagnostics;
		FrontendCTypeInfo& type_info;
		uni::Vector<SwitchControlContext> switch_contexts;
		stduint first_label = 0;
		stduint first_goto = 0;

		bool Report(const char* message, const uni::Nnode* node) const {
			diagnostics.Report(DiagnosticStage::Semantic, message, node);
			return false;
		}

		void AppendStatementAtoms(uni::Nnode* node,
			uni::Vector<ControlStatementAtom>& atoms) const {
			for (; node; node = node->next) {
				if (node->type == tok_statement) {
					for (uni::Nnode* child = node->subf; child; child = child->next) {
						ControlStatementAtom atom;
						atom.node = child;
						atoms.Append(atom);
					}
					continue;
				}
				ControlStatementAtom atom;
				atom.node = node;
				atoms.Append(atom);
			}
		}

		stduint DynamicDepth(stduint state) const {
			const FrontendCVariablyModifiedState* value =
				type_info.GetVariablyModifiedState(state);
			return value ? value->dynamic_depth : 0;
		}

		stduint AppendState(stduint parent, const uni::Nnode* declaration,
			const uni::Nnode* scope, bool owns_dynamic_storage) {
			FrontendCVariablyModifiedState state;
			state.parent = parent;
			state.declaration = declaration;
			state.scope = scope;
			state.dynamic_depth = DynamicDepth(parent) +
				(owns_dynamic_storage ? 1 : 0);
			state.owns_dynamic_storage = owns_dynamic_storage;
			type_info.variably_modified_states.Append(state);
			return type_info.variably_modified_states.Count();
		}

		stduint AppendDeclarationStates(const uni::Nnode* declaration,
			const uni::Nnode* scope, stduint state) {
			for (const FrontendCObjectDeclarator& object :
				type_info.object_declarators) {
				if (object.declaration != declaration ||
					!object.type.HasVariablyModifiedType()) continue;
				bool owns_dynamic_storage = object.type.IsArray() &&
					object.type.HasVariableArrayBound();
				state = AppendState(state, declaration, scope,
					owns_dynamic_storage);
			}
			for (const FrontendCTypedef& alias : type_info.typedefs) {
				if (alias.declaration != declaration ||
					!alias.type.HasVariablyModifiedType()) continue;
				state = AppendState(state, declaration, scope, false);
			}
			return state;
		}

		bool StateContains(stduint outer, stduint inner) const {
			while (inner != outer && inner) {
				const FrontendCVariablyModifiedState* state =
					type_info.GetVariablyModifiedState(inner);
				if (!state) return false;
				inner = state->parent;
			}
			return inner == outer;
		}

		FrontendCLabelControl* FindLabel(const char* name) {
			if (!name) return nullptr;
			for (stduint index = first_label;
				index < type_info.label_controls.Count(); index++) {
				FrontendCLabelControl& label = type_info.label_controls[index];
				if (!StrCompare(label.name.reference(), name)) return &label;
			}
			return nullptr;
		}

		bool AddLabel(uni::Nnode* statement, stduint state) {
			if (!statement || !statement->addr)
				return Report("label requires a name", statement);
			if (FindLabel(statement->addr))
				return Report("redefinition of label", statement);
			FrontendCLabelControl label;
			label.statement = statement;
			label.name = statement->addr;
			label.state = state;
			type_info.label_controls.Append(label);
			return true;
		}

		bool AddGoto(uni::Nnode* statement, uni::Nnode* target, stduint state) {
			if (!target || target->type != tok_identy || !target->addr)
				return Report("goto requires a label name", target);
			FrontendCGotoControl jump;
			jump.statement = statement;
			jump.target_name = target;
			jump.source_state = state;
			type_info.goto_controls.Append(jump);
			return true;
		}

		bool ResolveGotos() {
			for (stduint index = first_goto;
				index < type_info.goto_controls.Count(); index++) {
				FrontendCGotoControl& jump = type_info.goto_controls[index];
				FrontendCLabelControl* label = FindLabel(
					jump.target_name ? jump.target_name->addr : nullptr);
				if (!label)
					return Report("use of undefined label", jump.target_name);
				jump.target_label = label->statement;
				jump.target_state = label->state;
				jump.target_dynamic_depth = DynamicDepth(label->state);
				// C99 6.8.6.1 forbids entering the scope of an identifier
				// whose type is variably modified.
				if (!StateContains(label->state, jump.source_state))
					return Report(
						"goto enters scope of variably modified identifier",
						jump.target_name);
			}
			return true;
		}

		bool AnalyzeBlock(uni::Nnode* block, stduint entry_state) {
			if (!block || block->type != tok_block)
				return Report("expected block", block);
			uni::Vector<ControlStatementAtom> atoms;
			AppendStatementAtoms(block->subf, atoms);
			stduint state = entry_state;
			for (stduint index = 0; index < atoms.Count();)
				if (!AnalyzeStatement(atoms, index, state, block)) return false;
			return true;
		}

		bool AnalyzeStatement(uni::Vector<ControlStatementAtom>& atoms,
			stduint& index, stduint& state, const uni::Nnode* scope) {
			if (index >= atoms.Count())
				return Report("control statement requires a body", scope);
			uni::Nnode* node = atoms[index++].node;
			if (!node) return Report("missing statement", node);

			if (node->type == ToToken(FrontendCToken::Empty) ||
				node->type == ToToken(FrontendCToken::RecordDeclaration) ||
				node->type == ToToken(FrontendCToken::RecordDefinition) ||
				node->type == ToToken(FrontendCToken::EnumDeclaration) ||
				node->type == ToToken(FrontendCToken::EnumDefinition) ||
				node->type == ToToken(FrontendCToken::FunctionDeclaration)) return true;

			if (node->type == ToToken(FrontendCToken::Declaration) ||
				node->type == ToToken(FrontendCToken::TypedefDeclaration)) {
				state = AppendDeclarationStates(node, scope, state);
				return true;
			}

			if (node->type == ToToken(FrontendCToken::If)) {
				stduint branch_state = state;
				if (!AnalyzeStatement(atoms, index, branch_state, scope)) return false;
				if (index < atoms.Count() && atoms[index].node &&
					atoms[index].node->type == ToToken(FrontendCToken::Else)) {
					index++;
					branch_state = state;
					if (!AnalyzeStatement(atoms, index, branch_state, scope)) return false;
				}
				return true;
			}

			if (node->type == ToToken(FrontendCToken::While)) {
				stduint body_state = state;
				return AnalyzeStatement(atoms, index, body_state, scope);
			}

			if (node->type == ToToken(FrontendCToken::Do)) {
				stduint body_state = state;
				if (!AnalyzeStatement(atoms, index, body_state, scope)) return false;
				if (index >= atoms.Count() || !atoms[index].node ||
					atoms[index].node->type != ToToken(FrontendCToken::While))
					return Report("do statement requires trailing while", node);
				index++;
				if (index >= atoms.Count() || !atoms[index].node ||
					atoms[index].node->type != ToToken(FrontendCToken::Empty))
					return Report("do while requires a trailing semicolon", node);
				index++;
				return true;
			}

			if (node->type == ToToken(FrontendCToken::For)) {
				stduint for_state = state;
				uni::Nnode* initializer = node->subf;
				if (initializer &&
					initializer->type == ToToken(FrontendCToken::Declaration))
					for_state = AppendDeclarationStates(initializer, node, for_state);
				return AnalyzeStatement(atoms, index, for_state, node);
			}

			if (node->type == ToToken(FrontendCToken::Switch)) {
				SwitchControlContext context;
				context.entry_state = state;
				switch_contexts.Append(context);
				stduint body_state = state;
				bool result = AnalyzeStatement(atoms, index, body_state, scope);
				switch_contexts.Remove(switch_contexts.Count() - 1);
				return result;
			}

			if (node->type == ToToken(FrontendCToken::Case) ||
				node->type == ToToken(FrontendCToken::Default)) {
				if (!switch_contexts.Count())
					return Report(node->type == ToToken(FrontendCToken::Case) ?
						"case label not within a switch" :
						"default label not within a switch", node);
				if (state != switch_contexts[
					switch_contexts.Count() - 1].entry_state)
					return Report(node->type == ToToken(FrontendCToken::Case) ?
						"case label enters scope of variably modified identifier" :
						"default label enters scope of variably modified identifier", node);
				return AnalyzeStatement(atoms, index, state, scope);
			}

			if (node->type == ToToken(FrontendCToken::Label)) {
				if (!AddLabel(node, state)) return false;
				return AnalyzeStatement(atoms, index, state, scope);
			}

			if (node->type == ToToken(FrontendCToken::Goto)) {
				if (index >= atoms.Count())
					return Report("goto requires a label name", node);
				return AddGoto(node, atoms[index++].node, state);
			}

			if (node->type == ToToken(FrontendCToken::Return) && !node->subf) {
				if (index < atoms.Count()) index++;
				return true;
			}

			if (node->type == tok_block) return AnalyzeBlock(node, state);
			return true;
		}

	public:
		FunctionControlFlowAnalyzer(const FrontendCDiagnostics& diagnostic_output,
			FrontendCTypeInfo& semantic_types)
			: diagnostics(diagnostic_output), type_info(semantic_types),
				first_label(semantic_types.label_controls.Count()),
				first_goto(semantic_types.goto_controls.Count()) {}

		bool Analyze(uni::Nnode* body) {
			return AnalyzeBlock(body, 0) && ResolveGotos();
		}
	};

}

const FrontendCVariablyModifiedState*
FrontendCTypeInfo::GetVariablyModifiedState(stduint state) const {
	return state && state <= variably_modified_states.Count() ?
		&variably_modified_states.begin()[state - 1] : nullptr;
}

const FrontendCGotoControl* FrontendCTypeInfo::FindGotoControl(
	const uni::Nnode* statement) const {
	for (const FrontendCGotoControl& jump : goto_controls)
		if (jump.statement == statement) return &jump;
	return nullptr;
}

bool frontend_c_detail::AnalyzeControlFlow(uni::Nchain& syntax_tree,
	const FrontendCDiagnostics& diagnostics, FrontendCTypeInfo& type_info) {
	type_info.variably_modified_states.Clear();
	type_info.label_controls.Clear();
	type_info.goto_controls.Clear();
	for (uni::Nnode* node = syntax_tree.Root(); node;) {
		if (node->type == ToToken(FrontendCToken::GlobalDeclaration) ||
			node->type == ToToken(FrontendCToken::FunctionDeclaration) ||
			node->type == ToToken(FrontendCToken::TypedefDeclaration) ||
			node->type == ToToken(FrontendCToken::RecordDeclaration) ||
			node->type == ToToken(FrontendCToken::RecordDefinition) ||
			node->type == ToToken(FrontendCToken::EnumDeclaration) ||
			node->type == ToToken(FrontendCToken::EnumDefinition)) {
			node = node->next;
			continue;
		}
		uni::Nnode* body = node->next;
		if (node->type != ToToken(FrontendCToken::Function) ||
			!body || body->type != tok_block) return false;
		FunctionControlFlowAnalyzer analyzer(diagnostics, type_info);
		if (!analyzer.Analyze(body)) return false;
		node = body->next;
	}
	return true;
}

}
