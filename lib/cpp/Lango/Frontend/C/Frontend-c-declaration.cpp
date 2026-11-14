// ASCII C++11 TAB4 LF
// Docutitle: Lango C Frontend Declarators
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

namespace uni::lango::frontend_c_detail {

namespace {
	struct DeclaratorContext {
		uni::Nchain* syntax_tree = nullptr;
		const FrontendCDiagnostics* diagnostics = nullptr;
		const FrontendCTargetInfo* target = nullptr;
		FrontendCTypeInfo* prepared_types = nullptr;
		const FrontendCTypeInfo* type_info = nullptr;
		bool require_function_parameter_names = false;
	};

	bool IsOperator(const uni::Nnode* node, const char* name) {
		return node && node->type == tok_func && node->addr &&
			!StrCompare(node->addr, name);
	}

	bool ParseDeclaratorSequence(uni::Nnode* node, const uni::Nnode* end,
		FrontendCType& type, uni::Nnode*& identifier,
		const FrontendCTargetInfo& target, DeclaratorContext& context);

	bool AppendFunctionLayer(uni::Nnode* suffix, uni::Nnode*& parameters,
		FrontendCType& type, DeclaratorContext& context) {
		if (type.layer_count &&
			(type.layers[type.layer_count - 1].kind ==
				FrontendCTypeLayerKind::Array ||
			type.layers[type.layer_count - 1].kind ==
				FrontendCTypeLayerKind::Function)) return false;

		stduint function_type_id = context.type_info ?
			context.type_info->FindFunctionTypeNode(suffix) : 0;
		if (!function_type_id && context.prepared_types) {
			FrontendCFunctionType function_type;
			bool require_names = context.require_function_parameter_names &&
				type.layer_count == 0;
			if (!PrepareFunctionType(*context.syntax_tree, parameters,
				*context.diagnostics, *context.target, *context.prepared_types,
				function_type, require_names)) return false;
			function_type_id = context.prepared_types->InternFunctionType(function_type);
			context.prepared_types->AddFunctionTypeNode(suffix, function_type_id);
		}
		if (!function_type_id) return false;
		FrontendCTypeLayer layer;
		layer.kind = FrontendCTypeLayerKind::Function;
		layer.function_type_id = function_type_id;
		return type.AppendLayer(layer);
	}

	bool ParseDeclaratorNode(uni::Nnode* node, const uni::Nnode* end,
		FrontendCType& type, uni::Nnode*& identifier,
		const FrontendCTargetInfo& target, DeclaratorContext& context) {
		if (!node) return false;
		if (node->type == tok_identy && node->addr && node->next == end) {
			identifier = node;
			return true;
		}
		if (node->type == tok_func && !node->addr && node->subf) {
			uni::Nnode* direct = node->subf;
			uni::Nnode* suffix = direct ? direct->next : nullptr;
			if (suffix && !suffix->next && suffix->type == tok_func &&
				!suffix->addr) {
				if (!ParseDeclaratorNode(direct, suffix, type, identifier,
					target, context)) return false;
				return AppendFunctionLayer(suffix, suffix->subf, type, context);
			}
			return node->next == end && ParseDeclaratorSequence(
				node->subf, nullptr, type, identifier, target, context);
		}
		if (IsOperator(node, "OP@PREMEMB") && node->subf &&
			!node->subf->next && node->next == end) {
			if (!ParseDeclaratorNode(node->subf, nullptr, type, identifier,
				target, context))
				return false;
			return type.AppendPointer();
		}
		if (IsOperator(node, "operator[]") && node->next == end) {
			uni::Nnode* declarator = node->subf;
			uni::Nnode* dimension = declarator ? declarator->next : nullptr;
			if (!declarator || (dimension && dimension->next) ||
				!ParseDeclaratorNode(declarator, dimension, type, identifier,
					target, context))
				return false;
			if (type.layer_count && type.layers[type.layer_count - 1].kind ==
				FrontendCTypeLayerKind::Function) return false;
			if (!dimension) return type.AppendArrayDimension(0);
			stdsint count;
			FrontendCType literal_type;
			if (dimension->type != tok_number || !dimension->addr ||
				!ParseIntegerLiteral(dimension->addr, target, count, literal_type) ||
				count <= 0) return false;
			return type.AppendArrayDimension((stduint)count);
		}
		if (node->type == tok_func && node->addr && node->next == end &&
			StrCompare(node->addr, "operator[]") &&
			StrCompare(node->addr, "OP@PREMEMB")) {
			identifier = node;
			return AppendFunctionLayer(node, node->subf, type, context);
		}
		return false;
	}

	bool ParseDeclaratorSequence(uni::Nnode* node, const uni::Nnode* end,
		FrontendCType& type, uni::Nnode*& identifier,
		const FrontendCTargetInfo& target, DeclaratorContext& context) {
		stduint pointer_count = 0;
		while (node != end && node && node->type == tok_symbol && node->addr) {
			stduint stars = 0;
			while (node->addr[stars] == '*') stars++;
			if (!stars || node->addr[stars]) break;
			pointer_count += stars;
			node = node->next;
		}
		if (!node || node == end) return false;
		uni::Nnode* assignment_suffix = node->next;
		if (assignment_suffix != end &&
			IsOperator(assignment_suffix, "OP@ASSIGN") &&
			assignment_suffix->subf &&
			assignment_suffix->subf->type == tok_func &&
			!assignment_suffix->subf->addr) {
			if (!ParseDeclaratorNode(node, assignment_suffix, type, identifier,
				target, context) || !AppendFunctionLayer(
					assignment_suffix->subf, assignment_suffix->subf->subf,
					type, context) || assignment_suffix->next != end) return false;
			while (pointer_count--)
				if (!type.AppendPointer()) return false;
			return true;
		}
		uni::Nnode* suffix = node->next;
		while (suffix != end && suffix && suffix->type == tok_func &&
			!suffix->addr) break;
		if (!suffix || suffix == end || suffix->type != tok_func || suffix->addr)
			suffix = const_cast<uni::Nnode*>(end);
		if (!ParseDeclaratorNode(node, suffix, type, identifier,
			target, context)) return false;
		while (suffix != end && suffix && suffix->type == tok_func &&
			!suffix->addr) {
			uni::Nnode* next = suffix->next;
			if (!AppendFunctionLayer(suffix, suffix->subf, type, context)) return false;
			suffix = next;
		}
		if (suffix != end) return false;
		while (pointer_count--)
			if (!type.AppendPointer()) return false;
		return true;
	}

	bool AppendBaseLayers(FrontendCType& type, const FrontendCType& base) {
		for (stduint index = 0; index < base.layer_count; index++)
			if (!type.AppendLayer(base.layers[index])) return false;
		for (stduint index = 0; index + 1 < type.layer_count; index++) {
			FrontendCTypeLayerKind outer = type.layers[index].kind;
			FrontendCTypeLayerKind inner = type.layers[index + 1].kind;
			if ((outer == FrontendCTypeLayerKind::Array &&
				inner == FrontendCTypeLayerKind::Function) ||
				(outer == FrontendCTypeLayerKind::Function &&
					(inner == FrontendCTypeLayerKind::Array ||
					inner == FrontendCTypeLayerKind::Function))) return false;
		}
		return true;
	}

}

bool ParseDeclarator(uni::Nnode* node, FrontendCType& type,
	uni::Nnode*& identifier, const FrontendCTargetInfo& target,
	const FrontendCTypeInfo& type_info, const uni::Nnode* end) {
	identifier = nullptr;
	FrontendCType base = type;
	type.layer_count = 0;
	DeclaratorContext context;
	context.target = &target;
	context.type_info = &type_info;
	return ParseDeclaratorSequence(node, end, type, identifier,
		target, context) && identifier && AppendBaseLayers(type, base);
}

bool PrepareDeclarator(uni::Nchain& syntax_tree, uni::Nnode* node,
	FrontendCType& type, uni::Nnode*& identifier,
	const FrontendCDiagnostics& diagnostics,
	const FrontendCTargetInfo& target, FrontendCTypeInfo& type_info,
	bool require_function_parameter_names, const uni::Nnode* end) {
	identifier = nullptr;
	FrontendCType base = type;
	type.layer_count = 0;
	DeclaratorContext context;
	context.syntax_tree = &syntax_tree;
	context.diagnostics = &diagnostics;
	context.target = &target;
	context.prepared_types = &type_info;
	context.type_info = &type_info;
	context.require_function_parameter_names = require_function_parameter_names;
	return ParseDeclaratorSequence(node, end, type, identifier,
		target, context) && identifier && AppendBaseLayers(type, base);
}

}
