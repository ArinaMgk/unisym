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

#include "../../../../../inc/cpp/Lango/Frontend/C/Frontend-c-clover-internal.hpp"

namespace clover {
	using namespace uni::lango;
	using namespace frontend_c_internal;

	namespace {
		const FrontendCEnumerator* FindConstant(
			const uni::Vector<FrontendCEnumerator>& constants, const char* name) {
			for (const FrontendCEnumerator& constant : constants)
				if (!StrCompare(constant.name.reference(), name)) return &constant;
			return nullptr;
		}

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

		bool ParseGlobalConstant(uni::Nnode* node,
			const FrontendCTargetInfo& target,
			const uni::Vector<FrontendCEnumerator>& constants, stdsint& value) {
			if (!node) return false;
			if (node->type == tok_number) {
				FrontendCType type;
				return ParseIntegerLiteral(node->addr, target, value, type);
			}
			if (node->type == ToToken(FrontendCToken::CharacterLiteral))
				return ParseCharacterLiteral(node->addr, target, value);
			if (node->type == tok_identy) {
				const FrontendCEnumerator* constant = FindConstant(constants, node->addr);
				if (!constant) return false;
				value = constant->value;
				return true;
			}
			if (node->type == tok_func && node->addr && node->subf &&
				!node->subf->next && !StrCompare(node->addr, "OP@PRENEGA")) {
				if (!ParseGlobalConstant(
					node->subf, target, constants, value)) return false;
				value = -value;
				return true;
			}
			return false;
		}

		bool LowerGlobalDeclarator(uni::Nnode* node, const FrontendCType& base_type,
			Module& module, uni::Vector<FrontendCGlobal>& globals,
			const uni::Vector<FrontendCEnumerator>& constants,
			const FrontendCTargetInfo& target, const FrontendCTypeInfo& type_info,
			const FrontendCDiagnostics& diagnostics, const uni::Nnode* declaration) {
			if (node && node->type == tok_func && node->addr &&
				!StrCompare(node->addr, "OP@COMMA")) {
				for (uni::Nnode* child = node->subf; child; child = child->next)
					if (!LowerGlobalDeclarator(child, base_type, module, globals,
						constants, target, type_info, diagnostics, declaration)) return false;
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
			if (type.record_id && !type.IsPointer()) {
				diagnostics.Report(DiagnosticStage::Semantic,
					"global record objects are not supported yet", identifier);
				return false;
			}
			if (type.enum_id && !type.IsPointer() &&
				!type_info.SizeBits(type, target)) {
				diagnostics.Report(DiagnosticStage::Semantic,
					"global object has incomplete enum type", identifier);
				return false;
			}
			for (const FrontendCGlobal& previous : globals)
				if (!StrCompare(previous.name.reference(), identifier->addr)) {
					diagnostics.Report(DiagnosticStage::Semantic,
						"redefinition of global variable", identifier);
					return false;
				}
			if (FindConstant(constants, identifier->addr)) {
				diagnostics.Report(DiagnosticStage::Semantic,
					"global variable conflicts with enumerator", identifier);
				return false;
			}
			Global global;
			global.name = identifier->addr;
			global.type = CloverTypeFor(type, target);
			global.element_count = (stdsint)type.StorageElementCount();
			if (initializer_node && !ParseGlobalConstant(
				initializer_node, target, constants, global.initializer)) {
				diagnostics.Report(DiagnosticStage::Lowering,
					"global initializer must be an integer constant", initializer_node);
				return false;
			}
			if (type.integer_rank == FrontendCIntegerRank::Bool && !type.enum_id)
				global.initializer = global.initializer != 0;
			module.globals.Append(global);
			FrontendCGlobal frontend_global;
			frontend_global.name = identifier->addr;
			frontend_global.type = type;
			stduint alignment_bits = type_info.AlignBits(type, target);
			frontend_global.alignment = alignment_bits &&
				!(alignment_bits % target.char_bits) ?
				alignment_bits / target.char_bits : 0;
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
		module.source_files.Clear();
		module.pointer_bits = target.pointer_bits;
		uint32 debug_file_id = 0;
		if (debug_info) {
			SourceFile source;
			source.id = debug_file_id = 1;
			source.path = diagnostics.source_name ? diagnostics.source_name : "<input>";
			module.source_files.Append(source);
		}
		uni::Vector<FrontendCFunction> visible_functions;
		uni::Vector<FrontendCGlobal> visible_globals;
		uni::Vector<FrontendCEnumerator> visible_constants;
		unsigned next_string_literal = 0;
		for (uni::Nnode* node = syntax_tree.Root(); node;) {
			if (node->type == ToToken(FrontendCToken::EnumDeclaration)) {
				node = node->next;
				continue;
			}
			if (node->type == ToToken(FrontendCToken::EnumDefinition)) {
				const FrontendCEnum* enumeration = nullptr;
				for (const FrontendCEnum& candidate : type_info.enums)
					if (candidate.declaration == node) {
						enumeration = &candidate;
						break;
					}
				if (!enumeration || !enumeration->complete) {
					diagnostics.Report(DiagnosticStage::Semantic,
						"incomplete enum definition", node);
					return false;
				}
				for (const FrontendCEnumerator& enumerator : enumeration->enumerators) {
					for (const FrontendCGlobal& global : visible_globals)
						if (!StrCompare(global.name.reference(), enumerator.name.reference())) {
							diagnostics.Report(DiagnosticStage::Semantic,
								"enumerator conflicts with global variable", enumerator.declaration);
							return false;
						}
					for (const FrontendCFunction& function : visible_functions)
						if (!StrCompare(function.name.reference(), enumerator.name.reference())) {
							diagnostics.Report(DiagnosticStage::Semantic,
								"enumerator conflicts with function", enumerator.declaration);
							return false;
						}
					visible_constants.Append(enumerator);
				}
				node = node->next;
				continue;
			}
			if (node->type == ToToken(FrontendCToken::RecordDeclaration) ||
				node->type == ToToken(FrontendCToken::RecordDefinition)) {
					node = node->next;
					continue;
				}
			if (node->type == ToToken(FrontendCToken::GlobalDeclaration)) {
				const FrontendCType* base_type = type_info.Find(node);
				if (!base_type || !LowerGlobalDeclarator(node->subf, *base_type,
					module, visible_globals, visible_constants,
					target, type_info, diagnostics, node)) return false;
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
			if (FindConstant(visible_constants, node->addr)) {
				diagnostics.Report(DiagnosticStage::Semantic,
					"function conflicts with enumerator", node);
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
			if (function_return_type->enum_id && !function_return_type->IsPointer() &&
				!type_info.SizeBits(*function_return_type, target)) {
				diagnostics.Report(DiagnosticStage::Semantic,
					"function returns incomplete enum type", node);
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
				if (parameter_type->enum_id && !parameter_type->IsPointer() &&
					!type_info.SizeBits(*parameter_type, target)) {
					diagnostics.Report(DiagnosticStage::Semantic,
						"parameter has incomplete enum type", parameter_node);
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
			FunctionBuilder builder(module, function, visible_functions, visible_globals,
				visible_constants,
				*function_return_type,
				target, type_info, diagnostics, debug_info, next_string_literal,
				debug_file_id);
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
