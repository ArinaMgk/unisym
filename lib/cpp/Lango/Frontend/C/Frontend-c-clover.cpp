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

		FrontendCFunction* FindFunction(
			uni::Vector<FrontendCFunction>& functions, const char* name) {
			for (FrontendCFunction& function : functions)
				if (!StrCompare(function.name.reference(), name)) return &function;
			return nullptr;
		}

		const FrontendCFunction* FindFunction(
			const uni::Vector<FrontendCFunction>& functions, const char* name) {
			for (const FrontendCFunction& function : functions)
				if (!StrCompare(function.name.reference(), name)) return &function;
			return nullptr;
		}

		bool CompatibleFunctionTypes(const FrontendCFunction& previous,
			const FrontendCFunctionDeclaration& current) {
			if (!previous.return_type.IsSame(current.return_type)) return false;
			if (!previous.has_prototype || !current.has_prototype) return true;
			if (previous.is_variadic != current.is_variadic ||
				previous.parameter_types.Count() !=
				current.parameter_types.Count()) return false;
			for (stduint index = 0; index < previous.parameter_types.Count(); index++)
				if (!previous.parameter_types.begin()[index].IsSame(
					current.parameter_types.begin()[index])) return false;
			return true;
		}

		bool ParseGlobalConstant(uni::Nnode* node,
			const FrontendCTargetInfo& target,
			const uni::Vector<FrontendCEnumerator>& constants, stdsint& value) {
			if (!node) return false;
			while (node->type == tok_func && !node->addr && node->subf &&
				!node->subf->next) node = node->subf;
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

		bool ParseGlobalFunctionAddress(uni::Nnode* node,
			const FrontendCType& destination,
			const uni::Vector<FrontendCFunction>& functions,
			uni::String& symbol) {
			while (node && node->type == tok_func && !node->addr && node->subf &&
				!node->subf->next) node = node->subf;
			if (node && node->type == tok_func && node->addr && node->subf &&
				!node->subf->next && !StrCompare(node->addr, "OP@PREADDR"))
				node = node->subf;
			if (!node || node->type != tok_identy || !node->addr) return false;
			const FrontendCFunction* function = FindFunction(functions, node->addr);
			if (!function) return false;
			FrontendCType source = function->return_type;
			FrontendCTypeLayer function_layer;
			function_layer.kind = FrontendCTypeLayerKind::Function;
			function_layer.function_type_id = function->function_type_id;
			if (!source.PrependLayer(function_layer)) return false;
			source = source.DecayType();
			if (!destination.CanAssignFrom(source)) return false;
			symbol = function->name;
			return true;
		}

		bool EncodeFloatingConstant(double value, const FrontendCType& destination,
			stdsint& bits) {
			if (destination.scalar_kind == FrontendCScalarKind::Float) {
				union { float value; uint32 bits; } encoded;
				encoded.value = (float)value;
				bits = (stdsint)encoded.bits;
				return true;
			}
			if (destination.scalar_kind == FrontendCScalarKind::Double) {
				union { double value; uint64 bits; } encoded;
				encoded.value = value;
				bits = (stdsint)encoded.bits;
				return true;
			}
			return false;
		}

		bool ParseGlobalFloatingConstant(uni::Nnode* node,
			const FrontendCTargetInfo& target,
			const uni::Vector<FrontendCEnumerator>& constants,
			const FrontendCType& destination, stdsint& bits,
			stdsint& high_bits) {
			if (!node) return false;
			while (node->type == tok_func && !node->addr && node->subf &&
				!node->subf->next) node = node->subf;
			if (node->type == tok_func && node->addr && node->subf &&
				!node->subf->next && !StrCompare(node->addr, "OP@PRENEGA")) {
				if (!ParseGlobalFloatingConstant(node->subf, target, constants,
					destination, bits, high_bits)) return false;
				if (destination.scalar_kind == FrontendCScalarKind::LongDouble)
					high_bits ^= (stdsint)UINT64_C(0x8000000000000000);
				else bits ^= destination.scalar_kind == FrontendCScalarKind::Float ?
					(stdsint)UINT32_C(0x80000000) :
					(stdsint)UINT64_C(0x8000000000000000);
				return true;
			}
			double value;
			if (node->type == tok_number) {
				if (destination.scalar_kind == FrontendCScalarKind::LongDouble)
					return ParseLongDoubleConstant(node->addr, bits, high_bits);
				FrontendCType source_type;
				stdsint source_bits;
				if (ParseFloatingLiteral(node->addr, source_bits, source_type)) {
					if (source_type.scalar_kind == FrontendCScalarKind::Float) {
						union { uint32 bits; float value; } decoded;
						decoded.bits = (uint32)source_bits;
						value = decoded.value;
					}
					else {
						union { uint64 bits; double value; } decoded;
						decoded.bits = (uint64)source_bits;
						value = decoded.value;
					}
					return EncodeFloatingConstant(value, destination, bits);
				}
				stdsint integer;
				if (!ParseIntegerLiteral(node->addr, target, integer, source_type)) return false;
				value = source_type.is_signed ? (double)integer : (double)(uint64)integer;
				return EncodeFloatingConstant(value, destination, bits);
			}
			stdsint integer;
			if (!ParseGlobalConstant(node, target, constants, integer)) return false;
			return EncodeFloatingConstant((double)integer, destination, bits);
		}

		FrontendCGlobal* FindGlobal(uni::Vector<FrontendCGlobal>& globals,
			const char* name) {
			for (FrontendCGlobal& global : globals)
				if (!StrCompare(global.name.reference(), name)) return &global;
			return nullptr;
		}

		bool MergeGlobalType(const FrontendCType& previous,
			const FrontendCType& current, FrontendCType& merged) {
			if (previous.layer_count != current.layer_count)
				return false;
			FrontendCType left = previous;
			FrontendCType right = current;
			for (stduint index = 0; index < left.layer_count; index++) {
				if (left.layers[index].kind != FrontendCTypeLayerKind::Array) continue;
				if (left.layers[index].array_count && right.layers[index].array_count &&
					left.layers[index].array_count !=
					right.layers[index].array_count) return false;
				stduint dimension = left.layers[index].array_count ?
					left.layers[index].array_count : right.layers[index].array_count;
				left.layers[index].array_count = dimension;
				right.layers[index].array_count = dimension;
			}
			if (!left.IsSame(right)) return false;
			merged = left;
			return true;
		}

		void SetGlobalShape(Global& global, const FrontendCType& type,
			const FrontendCTargetInfo& target,
			const FrontendCTypeInfo& type_info) {
			stduint alignment_bits = type_info.AlignBits(type, target);
			global.alignment = alignment_bits &&
				!(alignment_bits % target.char_bits) ?
				alignment_bits / target.char_bits : 0;
			if (type.record_id && !type.StorageElementIsPointer()) {
				stduint size_bits = type_info.SizeBits(type, target);
				global.type = Type::I8;
				global.element_count = size_bits &&
					!(size_bits % target.char_bits) ?
					(stdsint)(size_bits / target.char_bits) : 0;
				return;
			}
			global.type = CloverTypeFor(type, target);
			global.element_count = type.HasIncompleteArray() ? 1 :
				(stdsint)type.StorageElementCount();
		}

		bool LowerGlobalDeclarator(const FrontendCObjectDeclarator& object,
			Module& module, uni::Vector<FrontendCGlobal>& globals,
			const uni::Vector<FrontendCFunction>& functions,
			const uni::Vector<FrontendCEnumerator>& constants,
			const FrontendCTargetInfo& target, const FrontendCTypeInfo& type_info,
			const FrontendCDiagnostics& diagnostics) {
			FrontendCType type = object.type;
			uni::Nnode* identifier = object.identifier;
			uni::Nnode* initializer_node = object.initializer;
			const uni::Nnode* declaration = object.declaration;
			if (type.HasIncompleteArray() && initializer_node &&
				!CompleteArrayTypeFromInitializer(type, initializer_node,
					target, type_info, diagnostics)) {
				diagnostics.Report(DiagnosticStage::Semantic,
					"cannot infer global array bound from initializer", identifier);
				return false;
			}
			if (type.record_id && !type.StorageElementIsPointer() &&
				!type_info.SizeBits(type, target)) {
				diagnostics.Report(DiagnosticStage::Semantic,
					"global object has incomplete record type", identifier);
				return false;
			}
			if (type.enum_id && !type.StorageElementIsPointer() &&
				!type_info.SizeBits(type, target)) {
				diagnostics.Report(DiagnosticStage::Semantic,
					"global object has incomplete enum type", identifier);
				return false;
			}
			FrontendCGlobal* previous = FindGlobal(globals, identifier->addr);
			if (!previous && FindConstant(constants, identifier->addr)) {
				diagnostics.Report(DiagnosticStage::Semantic,
					"global variable conflicts with enumerator", identifier);
				return false;
			}
			FrontendCStorageClass storage = type_info.FindStorage(declaration);
			Linkage requested_linkage = storage == FrontendCStorageClass::Static ?
				Linkage::Internal : Linkage::External;
			bool definition = initializer_node != nullptr;
			bool tentative = !definition && storage != FrontendCStorageClass::Extern;
			if (previous) {
				FrontendCType merged_type;
				FrontendCType previous_type = previous->type;
				if (previous->has_provisional_array_bound && type.IsArray() &&
					type.ArrayDimension(0)) previous_type.SetArrayDimension(0, 0);
				if (!MergeGlobalType(previous_type, type, merged_type)) {
					diagnostics.Report(DiagnosticStage::Semantic,
						"conflicting global variable types", identifier);
					return false;
				}
				if (storage == FrontendCStorageClass::Static &&
					previous->linkage == Linkage::External) {
					diagnostics.Report(DiagnosticStage::Semantic,
						"global variable linkage conflicts with previous declaration",
						identifier);
					return false;
				}
				if (definition && previous->has_definition) {
					diagnostics.Report(DiagnosticStage::Semantic,
						"redefinition of global variable", identifier);
					return false;
				}
				previous->type = merged_type;
				type = merged_type;
				if (type.IsArray() && type.ArrayDimension(0) &&
					(previous_type.ArrayDimension(0) == 0 ||
					initializer_node)) previous->has_provisional_array_bound = false;
			}
			else {
				FrontendCGlobal frontend_global;
				frontend_global.name = identifier->addr;
				if (tentative && type.HasIncompleteArray()) {
					type.SetArrayDimension(0, 1);
					frontend_global.has_provisional_array_bound = true;
				}
				frontend_global.type = type;
				frontend_global.linkage = requested_linkage;
				Global global;
				global.name = identifier->addr;
				global.is_read_only = type.IsConstObject();
				global.linkage = requested_linkage;
				global.is_declaration = !definition && !tentative;
				SetGlobalShape(global, type, target, type_info);
				module.globals.Append(global);
				frontend_global.module_index = module.globals.Count();
				globals.Append(frontend_global);
				previous = &globals[globals.Count() - 1];
			}
			if (tentative && type.HasIncompleteArray()) {
				type.SetArrayDimension(0, 1);
				previous->type = type;
				previous->has_provisional_array_bound = true;
			}
			Global& global = module.globals[previous->module_index - 1];
			SetGlobalShape(global, type, target, type_info);
			global.is_read_only = type.IsConstObject();
			global.linkage = previous->linkage;
			stduint alignment_bits = type_info.AlignBits(type, target);
			previous->alignment = alignment_bits &&
				!(alignment_bits % target.char_bits) ?
				alignment_bits / target.char_bits : 0;
			if (tentative) {
				global.is_declaration = false;
			}
			if (!definition) return true;
			global.is_declaration = false;
			global.initializer = 0;
			global.initializer_high = 0;
			global.initializer_symbol.Clear();
			global.initializer_values.Clear();
			global.initializer_fragments.Clear();
			bool initializer_valid = type.IsFunctionPointer() &&
				ParseGlobalFunctionAddress(initializer_node, type, functions,
					global.initializer_symbol);
			if (!initializer_valid) initializer_valid = type.IsArray() || type.IsRecord() ?
				BuildStaticObjectInitializer(global, type, initializer_node,
					target, type_info, constants, diagnostics) :
				(type.IsFloating() ? ParseGlobalFloatingConstant(initializer_node,
					target, constants, type, global.initializer,
					global.initializer_high) :
					ParseGlobalConstant(initializer_node, target,
						constants, global.initializer));
			if (!initializer_valid) {
				if (!type.IsArray() && !type.IsRecord()) diagnostics.Report(DiagnosticStage::Lowering,
					"global initializer must be a constant expression", initializer_node);
				return false;
			}
			if (!type.IsArray() && type.integer_rank == FrontendCIntegerRank::Bool &&
				!type.enum_id) global.initializer = global.initializer != 0;
			previous->has_definition = true;
			return true;
		}

		bool FlattenableFloatAggregate(const FrontendCType& type,
			const FrontendCTypeInfo& type_info,
			uni::Vector<FrontendCType>& members) {
			if (!type.IsRecord()) return false;
			const FrontendCRecord* record = type_info.GetRecord(type.record_id);
			if (!record || !record->complete || record->is_union ||
				record->layout == FrontendCRecordLayout::Packed ||
				!record->members.Count() || record->members.Count() > 2) return false;
			bool has_float = false;
			for (const FrontendCRecordMember& member : record->members) {
				if (!member.type.IsArithmetic() || member.type.IsArray() ||
					member.type.scalar_kind == FrontendCScalarKind::LongDouble)
					return false;
				has_float = has_float || member.type.IsFloating();
				members.Append(member.type);
			}
			return has_float;
		}

		bool AppendDeclarationParameters(const FrontendCFunctionDeclaration& source,
			Function& function, const FrontendCTypeInfo& type_info,
			const FrontendCTargetInfo& target,
			const FrontendCDiagnostics& diagnostics) {
			for (const FrontendCType& parameter_type : source.parameter_types) {
				if (parameter_type.enum_id && !parameter_type.IsPointer() &&
					!type_info.SizeBits(parameter_type, target)) {
					diagnostics.Report(DiagnosticStage::Semantic,
						"parameter has incomplete enum type", source.node);
					return false;
				}
				if (parameter_type.IsRecord()) {
					uni::Vector<FrontendCType> aggregate_members;
					if (!FlattenableFloatAggregate(parameter_type, type_info,
						aggregate_members)) {
						diagnostics.Report(DiagnosticStage::Semantic,
							"unsupported record parameter ABI shape", source.node);
						return false;
					}
					unsigned group = function.parameters.Count() + 1;
					for (const FrontendCType& member_type : aggregate_members) {
						Parameter parameter;
						parameter.value.type = CloverTypeFor(member_type, target);
						parameter.value.id = function.parameters.Count();
						parameter.aggregate_group = group;
						function.parameters.Append(parameter);
					}
				}
				else {
					Parameter parameter;
					parameter.value.type = CloverTypeFor(parameter_type, target);
					parameter.value.id = function.parameters.Count();
					function.parameters.Append(parameter);
				}
			}
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
			if (node->type == ToToken(FrontendCToken::TypedefDeclaration)) {
				node = node->next;
				continue;
			}
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
			if (node->type == ToToken(FrontendCToken::FunctionDeclaration)) {
				const FrontendCFunctionDeclaration* declaration =
					type_info.FindFunction(node);
				if (!declaration) {
					diagnostics.Report(DiagnosticStage::Semantic,
						"missing function declaration type", node);
					return false;
				}
				for (const FrontendCGlobal& previous : visible_globals)
					if (!StrCompare(previous.name.reference(),
						declaration->name.reference())) {
						diagnostics.Report(DiagnosticStage::Semantic,
							"function conflicts with global variable", node);
						return false;
					}
				if (FindConstant(visible_constants, declaration->name.reference())) {
					diagnostics.Report(DiagnosticStage::Semantic,
						"function conflicts with enumerator", node);
					return false;
				}
				Linkage linkage = type_info.FindStorage(node) ==
					FrontendCStorageClass::Static ? Linkage::Internal : Linkage::External;
				FrontendCFunction* previous = FindFunction(
					visible_functions, declaration->name.reference());
				if (previous) {
					if (!CompatibleFunctionTypes(*previous, *declaration)) {
						diagnostics.Report(DiagnosticStage::Semantic,
							"conflicting function types", node);
						return false;
					}
					if (previous->linkage != linkage && linkage == Linkage::Internal) {
						diagnostics.Report(DiagnosticStage::Semantic,
							"function linkage conflicts with previous declaration", node);
						return false;
					}
					if (!previous->has_prototype && declaration->has_prototype) {
						previous->parameter_types = declaration->parameter_types;
						previous->function_type_id = declaration->function_type_id;
						previous->is_variadic = declaration->is_variadic;
						previous->has_prototype = true;
					}
					node = node->next;
					continue;
				}

				Function function;
				function.name = declaration->name;
				function.return_type = CloverTypeFor(declaration->return_type, target);
				function.is_variadic = declaration->is_variadic ||
					!declaration->has_prototype;
				function.is_declaration = true;
				function.linkage = linkage;
				if (!AppendDeclarationParameters(*declaration, function,
					type_info, target, diagnostics)) return false;
				module.functions.Append(function);

				FrontendCFunction visible;
				visible.name = declaration->name;
				visible.return_type = declaration->return_type;
				visible.parameter_types = declaration->parameter_types;
				visible.function_type_id = declaration->function_type_id;
				visible.is_variadic = declaration->is_variadic;
				visible.has_prototype = declaration->has_prototype;
				visible.module_index = module.functions.Count();
				visible.linkage = linkage;
				visible_functions.Append(visible);
				node = node->next;
				continue;
			}
			if (node->type == ToToken(FrontendCToken::GlobalDeclaration)) {
				bool found = false;
				for (const FrontendCObjectDeclarator& object :
					type_info.object_declarators) {
					if (object.declaration != node) continue;
					found = true;
					if (!LowerGlobalDeclarator(object, module, visible_globals,
						visible_functions, visible_constants, target, type_info,
						diagnostics)) return false;
				}
				if (!found) {
					diagnostics.Report(DiagnosticStage::Lowering,
						"global declaration has no declarators", node);
					return false;
				}
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
			const FrontendCFunctionDeclaration* declaration =
				type_info.FindFunction(node);
			if (!declaration) {
				diagnostics.Report(DiagnosticStage::Semantic,
					"missing function definition type", node);
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
			Linkage linkage = type_info.FindStorage(node) ==
				FrontendCStorageClass::Static ? Linkage::Internal : Linkage::External;
			function.linkage = linkage;
			FrontendCFunction* previous_function = FindFunction(
				visible_functions, node->addr);
			if (previous_function) {
				if (!CompatibleFunctionTypes(*previous_function, *declaration)) {
					diagnostics.Report(DiagnosticStage::Semantic,
						"conflicting function types", node);
					return false;
				}
				if (previous_function->has_definition) {
					diagnostics.Report(DiagnosticStage::Semantic,
						"redefinition of function", node);
					return false;
				}
				if (previous_function->linkage != linkage &&
					linkage == Linkage::Internal) {
					diagnostics.Report(DiagnosticStage::Semantic,
						"function linkage conflicts with previous declaration", node);
					return false;
				}
			}
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
			signature.function_type_id = declaration->function_type_id;
			signature.has_prototype = declaration->has_prototype;
			signature.has_definition = true;
			signature.linkage = linkage;
			for (uni::Nnode* parameter_node = node->subf;
				parameter_node; parameter_node = parameter_node->next) {
				if ((parameter_node->type ==
					ToToken(FrontendCToken::VariadicParameter)) ||
					(parameter_node->addr && !StrCompare(parameter_node->addr, "..."))) {
					signature.is_variadic = true;
					function.is_variadic = true;
					continue;
				}
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
				if (parameter_type->IsRecord()) {
					uni::Vector<FrontendCType> aggregate_members;
					if (!FlattenableFloatAggregate(*parameter_type, type_info,
						aggregate_members)) {
						diagnostics.Report(DiagnosticStage::Semantic,
							"unsupported record parameter ABI shape", parameter_node);
						return false;
					}
					unsigned group = function.parameters.Count() + 1;
					for (const FrontendCType& member_type : aggregate_members) {
						Parameter parameter;
						parameter.name = parameter_node->addr;
						parameter.value.type = CloverTypeFor(member_type, target);
						parameter.value.id = function.parameters.Count();
						parameter.aggregate_group = group;
						function.parameters.Append(parameter);
					}
				}
				else {
					Parameter parameter;
					parameter.name = parameter_node->addr;
					parameter.value.type = CloverTypeFor(*parameter_type, target);
					parameter.value.id = function.parameters.Count();
					function.parameters.Append(parameter);
				}
			}
			if (previous_function) {
				signature.module_index = previous_function->module_index;
				*previous_function = signature;
				module.functions[signature.module_index - 1] = function;
			}
			else {
				module.functions.Append(function);
				signature.module_index = module.functions.Count();
				visible_functions.Append(signature);
			}
			FunctionBuilder builder(module, function, visible_functions, visible_globals,
				visible_constants,
				*function_return_type,
				target, type_info, diagnostics, debug_info, next_string_literal,
				debug_file_id);
			if (!builder.Lower(body, node->subf)) return false;
			module.functions[signature.module_index - 1] = function;
			node = body->next;
		}
		return true;
	}

	bool frontend_c_internal::ParseStaticScalarInitializer(uni::Nnode* node,
		const FrontendCTargetInfo& target,
		const uni::Vector<FrontendCEnumerator>& constants,
		const FrontendCType& destination, stdsint& low, stdsint& high) {
		high = 0;
		if (destination.IsFloating())
			return ParseGlobalFloatingConstant(node, target, constants,
				destination, low, high);
		if (destination.IsPointer()) {
			return ParseGlobalConstant(node, target, constants, low) && !low;
		}
		return destination.IsInteger() &&
			ParseGlobalConstant(node, target, constants, low);
	}

}

bool uni::lango::frontend_c_detail::Lower(uni::Nchain& syntax_tree,
	clover::Module& module, const FrontendCDiagnostics& diagnostics,
	const FrontendCTargetInfo& target, const FrontendCTypeInfo& type_info,
	const FrontendCDebugInfo* debug_info) {
	return clover::LowerFrontendC(syntax_tree, module, diagnostics, target,
		type_info, debug_info);
}
