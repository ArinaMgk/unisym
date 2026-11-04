// ASCII C++11 TAB4 LF
// Docutitle: Lango C Frontend Clover Statement Lowering
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

namespace clover::frontend_c_internal {

	bool FunctionBuilder::PredeclareLocal(const char* name, const FrontendCType& type,
		const uni::Nnode* declaration, const uni::Nnode* scope,
		FrontendCStorageClass storage_class) {
		for (const LocalValue& local : locals)
			if (local.scope == scope && !StrCompare(local.name.reference(), name)) {
				diagnostics->Report(DiagnosticStage::Semantic,
					"redefinition of local variable", declaration);
				failed = true;
				return false;
			}
		if (type.enum_id && !type.IsPointer() &&
			!type_info->SizeBits(type, *target)) {
			diagnostics->Report(DiagnosticStage::Semantic,
				"object has incomplete enum type", declaration);
			failed = true;
			return false;
		}
		if (type.HasIncompleteArray()) {
			diagnostics->Report(DiagnosticStage::Semantic,
				"object has incomplete array type", declaration);
			failed = true;
			return false;
		}
		Instruction instruction;
		bool is_static = storage_class == FrontendCStorageClass::Static;
		if (is_static && type.record_id && !type.IsPointer()) {
			diagnostics->Report(DiagnosticStage::Semantic,
				"block static record objects are not supported yet", declaration);
			failed = true;
			return false;
		}
		instruction.opcode = is_static ? Opcode::GlobalAddr : Opcode::Local;
	if (type.record_id && !type.IsPointer()) {
		stduint size_bits = type_info->SizeBits(type, *target);
		stduint align_bits = type_info->AlignBits(type, *target);
		if (!size_bits || !align_bits || size_bits % target->char_bits ||
			align_bits % target->char_bits) {
			diagnostics->Report(DiagnosticStage::Semantic,
				"invalid record storage layout", declaration);
			failed = true;
			return false;
		}
		instruction.type = Type::I8;
		instruction.immediate = (stdsint)(size_bits / target->char_bits);
		instruction.alignment = align_bits / target->char_bits;
	}
	else {
		instruction.type = CloverType(type);
		instruction.immediate = type.IsPointer() ? 1 :
			(stdsint)type.StorageElementCount();
	}
		instruction.has_result = true;
		instruction.result = NewValue(Type::Ptr);
		instruction.name = name;
		stduint global_index = 0;
		if (is_static) {
			Global global;
			global.name = ".L.clover.static.";
			global.name += function->name;
			global.name += ".";
			AppendUnsigned(global.name, next_static_local++);
			global.type = CloverType(type);
			global.is_read_only = type.IsConstObject();
			global.element_count = type.IsPointer() ? 1 :
				(stdsint)type.StorageElementCount();
			global.linkage = Linkage::Internal;
			module->globals.Append(global);
			global_index = module->globals.Count();
			instruction.name = global.name;
			instruction.type = Type::Ptr;
			instruction.immediate = 0;
		}
		AttachSource(instruction, declaration);
		EntryBlock().instructions.Append(instruction);

		LocalValue local;
		local.name = name;
		local.address = instruction.result;
		local.type = type;
		local.alignment = TypeAlignment(type);
		local.declaration = declaration;
		local.scope = scope;
		local.global_index = global_index;
		locals.Append(local);
		return true;
	}

	bool FunctionBuilder::ParseDeclaratorNode(uni::Nnode* node, uni::Nnode*& identifier,
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
			if (!declarator || (dimension && dimension->next) ||
				!ParseDeclaratorNode(declarator, identifier, type)) return false;
			if (!dimension)
				return !type.array_dimension_count && type.AppendArrayDimension(0);
			stdsint count;
			FrontendCType literal_type;
			if (dimension->type != tok_number || !dimension->addr ||
				!uni::lango::frontend_c_detail::ParseIntegerLiteral(
					dimension->addr, *target, count, literal_type) || count <= 0) return false;
			return type.AppendArrayDimension((stduint)count);
		}
		if (node->type != tok_identy || !node->addr) return false;
		identifier = node;
		return true;
	}

	bool FunctionBuilder::ParseDeclarator(uni::Nnode* node, uni::Nnode*& identifier,
		FrontendCType& type, const FrontendCType& base_type) {
		type = base_type;
		return ParseDeclaratorNode(node, identifier, type);
	}

	bool FunctionBuilder::PredeclareDeclarator(uni::Nnode* node, const uni::Nnode* declaration,
		const uni::Nnode* scope, const FrontendCType& base_type) {
		if (!node) return false;
		if (node->type == tok_func && node->addr &&
			!StrCompare(node->addr, "OP@COMMA")) {
			for (uni::Nnode* child = node->subf; child; child = child->next)
				if (!PredeclareDeclarator(child, declaration, scope, base_type)) return false;
			return true;
		}
		uni::Nnode* declarator = node;
		if (node->type == tok_func && node->addr &&
			!StrCompare(node->addr, "OP@ASSIGN")) declarator = node->subf;
		uni::Nnode* identifier = nullptr;
		FrontendCType type;
		if (!ParseDeclarator(declarator, identifier, type, base_type)) return false;
		return PredeclareLocal(identifier->addr, type, declaration, scope,
			type_info->FindStorage(declaration));
	}

	bool FunctionBuilder::PredeclareBlock(uni::Nnode* block) {
		for (uni::Nnode* node = block ? block->subf : nullptr; node; node = node->next) {
			if (node->type == ToToken(FrontendCToken::Declaration)) {
				const FrontendCType* base_type = type_info->Find(node);
				if (!base_type || !PredeclareDeclarator(node->subf, node, block, *base_type)) {
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

	LocalValue* FunctionBuilder::ActivateLocal(const char* name, const uni::Nnode* declaration) {
		for (LocalValue& local : locals) {
			if (local.declaration == declaration &&
				!StrCompare(local.name.reference(), name)) {
				for (const FrontendCLocalConstant& constant : local_constants)
					if (constant.active && constant.scope == local.scope &&
						constant.constant && !StrCompare(
							constant.constant->name.reference(), name)) {
						diagnostics->Report(DiagnosticStage::Semantic,
							"local variable conflicts with enumerator", declaration);
						failed = true;
						return nullptr;
					}
				local.active = true;
				return &local;
			}
		}
		return nullptr;
	}

	void FunctionBuilder::AppendInitializerValues(uni::Nnode* node,
		uni::Vector<uni::Nnode*>& values) {
		if (node && node->type == tok_block && node->subf) {
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

	bool FunctionBuilder::InitializeArray(LocalValue& local, uni::Nnode* initializer) {
		uni::Vector<uni::Nnode*> values;
		AppendInitializerValues(initializer, values);
		stduint element_count = local.type.StorageElementCount();
		if (values.Count() > element_count)
			return Error("too many array initializer values", initializer).valid;
		FrontendCType element_type = local.type;
		element_type.array_dimension_count = 0;
		FrontendCType element_pointer = element_type.AddressType();
		for (stduint index = 0; index < element_count; index++) {
			Value value;
			if (index < values.Count()) {
				uni::Nnode* value_node = values.begin()[index];
				LoweredValue lowered = ToRValue(LowerExpression(value_node), value_node);
				if (!lowered.valid) return false;
				if (!lowered.type.IsArithmetic() || !element_type.IsArithmetic())
					return Error("array initializer requires arithmetic values", value_node).valid;
				lowered = ConvertArithmetic(lowered, element_type, value_node);
				if (!lowered.valid) return false;
				value = lowered.value;
			}
			else value = element_type.IsFloating() ?
				EmitFloatConst(0, element_type) : EmitConst(0, element_type);
			Value element_index = EmitConst((stdsint)index);
			Value address = EmitPtrAdd(local.address, element_index, element_pointer);
			EmitStore(value, address, OffsetAlignment(local.alignment,
				index * TypeSize(element_type)));
		}
		return true;
	}

	bool FunctionBuilder::InitializeStatic(LocalValue& local,
		uni::Nnode* initializer) {
		if (!local.global_index || local.global_index > module->globals.Count())
			return Error("invalid block static object", initializer).valid;
		if (!initializer) return true;
		Global& global = module->globals[local.global_index - 1];
		if (local.type.IsArray()) {
			FrontendCType element_type = local.type;
			element_type.array_dimension_count = 0;
			if (element_type.scalar_kind == FrontendCScalarKind::LongDouble)
				return Error("block static long double array initialization is not supported yet",
					initializer).valid;
			uni::Vector<uni::Nnode*> values;
			AppendInitializerValues(initializer, values);
			stduint count = local.type.StorageElementCount();
			if (values.Count() > count)
				return Error("too many block static array initializer values",
					initializer).valid;
			for (stduint index = 0; index < count; index++) {
				stdsint low = 0;
				stdsint high = 0;
				if (index < values.Count() && !ParseStaticScalarInitializer(
					values[index], *target, *visible_constants,
					element_type, low, high))
					return Error("block static array initializer must contain constant expressions",
						values[index]).valid;
				global.initializer_values.Append(low);
			}
			return true;
		}
		if (!ParseStaticScalarInitializer(initializer, *target,
			*visible_constants, local.type, global.initializer,
			global.initializer_high))
			return Error("block static initializer must be a constant expression",
				initializer).valid;
		if (local.type.integer_rank == FrontendCIntegerRank::Bool &&
			!local.type.enum_id) global.initializer = global.initializer != 0;
		return true;
	}

	bool FunctionBuilder::LowerDeclarator(uni::Nnode* node, const uni::Nnode* declaration) {
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
		const FrontendCType* base_type = type_info->Find(declaration);
		if (!base_type || !ParseDeclarator(declarator, identifier,
			declared_type, *base_type))
			return Error("invalid declarator", declarator).valid;
		LocalValue* local = ActivateLocal(identifier->addr, declaration);
		if (!local) return failed ? false :
			Error("invalid local declaration", identifier).valid;
		if (local->global_index) return InitializeStatic(*local, initializer_node);
		if (!initializer_node) return true;
		if (local->type.record_id && !local->type.IsPointer())
			return Error("record initialization is not supported yet", initializer_node).valid;
		if (local->type.IsArray())
			return InitializeArray(*local, initializer_node);
		LoweredValue initializer = ToRValue(
			LowerExpression(initializer_node), initializer_node);
		if (!initializer.valid) return false;
		if (local->type.IsArithmetic())
			initializer = ConvertArithmetic(initializer, local->type, initializer_node);
		else if (!local->type.CanAssignFrom(initializer.type))
			return Error("incompatible initializer type", initializer_node).valid;
		if (!initializer.valid) return false;
		EmitStore(initializer.value, local->address, local->alignment);
		return true;
	}

	void FunctionBuilder::LeaveScope(const uni::Nnode* block) {
		for (LocalValue& local : locals)
			if (local.scope == block) local.active = false;
		for (FrontendCLocalConstant& constant : local_constants)
			if (constant.scope == block) constant.active = false;
	}

	bool FunctionBuilder::ActivateEnum(
		const uni::Nnode* declaration, const uni::Nnode* scope) {
		const FrontendCEnum* enumeration = nullptr;
		for (const FrontendCEnum& candidate : type_info->enums)
			if (candidate.declaration == declaration) {
				enumeration = &candidate;
				break;
			}
		if (!enumeration || !enumeration->complete)
			return Error("incomplete enum definition", declaration).valid;
		for (const FrontendCEnumerator& enumerator : enumeration->enumerators) {
			for (const LocalValue& local : locals)
				if (local.active && local.scope == scope &&
					!StrCompare(local.name.reference(), enumerator.name.reference()))
					return Error("enumerator conflicts with local variable",
						enumerator.declaration).valid;
			FrontendCLocalConstant constant;
			constant.constant = &enumerator;
			constant.scope = scope;
			constant.active = true;
			local_constants.Append(constant);
		}
		return true;
	}

	FunctionBuilder::FunctionBuilder(Module& output_module, Function& function,
		const uni::Vector<FrontendCFunction>& functions,
		const uni::Vector<FrontendCGlobal>& globals,
		const uni::Vector<FrontendCEnumerator>& constants,
		const FrontendCType& function_return_type,
		const FrontendCTargetInfo& target_info,
		const FrontendCTypeInfo& semantic_types,
		const FrontendCDiagnostics& diagnostic_output,
		const FrontendCDebugInfo* debug_information,
		unsigned& string_literal_index, uint32 source_file_id)
		: module(&output_module), function(&function), return_type(function_return_type),
			next_value(function.parameters.Count()),
			visible_functions(&functions), visible_globals(&globals),
			visible_constants(&constants),
			target(&target_info), type_info(&semantic_types),
			debug_info(debug_information),
			diagnostics(&diagnostic_output),
			next_string_literal(&string_literal_index),
			debug_file_id(source_file_id)
	{
		function.blocks.Append(BasicBlock());
		function.blocks[0].name = "entry";
	}

	bool FunctionBuilder::EmitReturnExpression(uni::Nnode* expression) {
		if (!expression)
			return Error("return requires a value", expression).valid;
		LoweredValue lowered = LowerExpression(expression);
		if (!lowered.valid) return false;
		if (return_type.IsArithmetic())
			lowered = ConvertArithmetic(lowered, return_type, expression);
		else {
			lowered = ToRValue(lowered, expression);
			if (!lowered.valid || !return_type.CanAssignFrom(lowered.type))
				return Error("incompatible function return type", expression).valid;
		}
		if (!lowered.valid) return false;
		Instruction instruction;
		instruction.opcode = Opcode::Ret;
		instruction.type = CloverType(return_type);
		instruction.operands.Append(lowered.value);
		CurrentBlock().instructions.Append(instruction);
		return true;
	}

	bool FunctionBuilder::BeginIf(uni::Nnode* condition_node, const uni::Nnode* source_statement,
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

	void FunctionBuilder::EndIfBranch(const uni::String& end_block) {
		if (!BlockTerminated()) EmitJump(end_block);
	}

	void FunctionBuilder::AppendStatementAtoms(uni::Nnode* node,
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

	FunctionLabel* FunctionBuilder::FindLabel(const char* name) {
		if (!name) return nullptr;
		for (FunctionLabel& label : labels)
			if (!StrCompare(label.name.reference(), name)) return &label;
		return nullptr;
	}

	bool FunctionBuilder::CollectLabels(uni::Nnode* node) {
		for (; node; node = node->next) {
			if (node->type == ToToken(FrontendCToken::Label)) {
				if (!node->addr)
					return Error("label requires a name", node).valid;
				if (FindLabel(node->addr))
					return Error("redefinition of label", node).valid;
				FunctionLabel label;
				label.name = node->addr;
				label.block = NewBlock("label");
				label.declaration = node;
				labels.Append(label);
			}
			if (node->subf && !CollectLabels(node->subf)) return false;
		}
		return true;
	}

	bool FunctionBuilder::CollectSwitchBlock(
		uni::Nnode* block, SwitchContext& context) {
		uni::Vector<StatementAtom> atoms;
		AppendStatementAtoms(block ? block->subf : nullptr, atoms);
		for (stduint index = 0; index < atoms.Count();)
			if (!CollectSwitchStatement(atoms, index, context, true)) return false;
		return true;
	}

	bool FunctionBuilder::CollectSwitchStatement(
		uni::Vector<StatementAtom>& atoms, stduint& index,
		SwitchContext& context, bool collect) {
		if (index >= atoms.Count())
			return Error("control statement requires a body", nullptr).valid;
		uni::Nnode* node = atoms.begin()[index++].node;
		if (!node) return Error("missing statement", node).valid;

		if (node->type == ToToken(FrontendCToken::Case) ||
			node->type == ToToken(FrontendCToken::Default)) {
			if (collect) {
				SwitchLabelTarget label;
				label.node = node;
				label.is_default =
					node->type == ToToken(FrontendCToken::Default);
				context.labels.Append(label);
			}
			return CollectSwitchStatement(atoms, index, context, collect);
		}
		if (node->type == ToToken(FrontendCToken::Label))
			return CollectSwitchStatement(atoms, index, context, collect);
		if (node->type == ToToken(FrontendCToken::Switch))
			return CollectSwitchStatement(atoms, index, context, false);
		if (node->type == ToToken(FrontendCToken::If)) {
			if (!CollectSwitchStatement(atoms, index, context, collect)) return false;
			if (index < atoms.Count() && atoms.begin()[index].node &&
				atoms.begin()[index].node->type == ToToken(FrontendCToken::Else)) {
				index++;
				return CollectSwitchStatement(atoms, index, context, collect);
			}
			return true;
		}
		if (node->type == ToToken(FrontendCToken::While) ||
			node->type == ToToken(FrontendCToken::For))
			return CollectSwitchStatement(atoms, index, context, collect);
		if (node->type == tok_block)
			return collect ? CollectSwitchBlock(node, context) : true;
		if (node->type == ToToken(FrontendCToken::Goto)) {
			if (index < atoms.Count()) index++;
			return true;
		}
		if (node->type == ToToken(FrontendCToken::Return) && !node->subf) {
			if (index < atoms.Count()) index++;
			return true;
		}
		return true;
	}

	uni::Nnode* FunctionBuilder::ForClause(uni::Nnode* clause) {
		return clause && clause->type == tok_statement ? clause->subf : nullptr;
	}

	bool FunctionBuilder::LowerSwitch(uni::Nnode* node,
		const uni::Nnode* source_statement,
		uni::Vector<StatementAtom>& atoms, stduint& index) {
		if (!node->subf || node->subf->next)
			return Error("switch requires one controlling expression", node).valid;

		SwitchContext context;
		stduint scan = index;
		if (!CollectSwitchStatement(atoms, scan, context, true)) return false;

		stduint first_instruction = CurrentBlock().instructions.Count();
		LoweredValue controlling = ToRValue(
			LowerExpression(node->subf), node->subf);
		if (!controlling.valid) return false;
		if (!controlling.type.IsInteger())
			return Error("switch expression requires an integer type", node->subf).valid;
		FrontendCType comparison_type = PromoteType(controlling.type);
		controlling = ConvertInteger(controlling, comparison_type, node->subf);
		if (!controlling.valid) return false;
		AttachSource(first_instruction, source_statement);

		bool has_default = false;
		stduint default_block = 0;
		for (stduint label_index = 0;
			label_index < context.labels.Count(); label_index++) {
			SwitchLabelTarget& label = context.labels[label_index];
			if (label.is_default) {
				if (has_default)
					return Error("multiple default labels in one switch",
						const_cast<uni::Nnode*>(label.node)).valid;
				has_default = true;
				label.block = NewBlock("switch.default");
				default_block = label.block;
				continue;
			}
			if (!label.node->subf || label.node->subf->next ||
				!uni::lango::frontend_c_detail::EvaluateIntegerConstant(
					const_cast<uni::Nnode*>(label.node->subf), *target,
					*type_info, label.value) ||
				!NormalizeIntegerConstant(label.value, comparison_type))
				return Error("case requires an integer constant expression",
					const_cast<uni::Nnode*>(label.node)).valid;
			for (stduint previous = 0; previous < label_index; previous++)
				if (!context.labels[previous].is_default &&
					context.labels[previous].value == label.value)
					return Error("duplicate case value",
						const_cast<uni::Nnode*>(label.node)).valid;
			label.block = NewBlock("switch.case");
		}

		uni::String end_name = NewBlockName("switch.end");
		uni::String fallback = has_default ?
			function->blocks[default_block].name : end_name;
		bool emitted_test = false;
		for (stduint label_index = 0;
			label_index < context.labels.Count(); label_index++) {
			SwitchLabelTarget& label = context.labels[label_index];
			if (label.is_default) continue;
			emitted_test = true;
			bool has_more = false;
			for (stduint next = label_index + 1;
				next < context.labels.Count(); next++)
				if (!context.labels[next].is_default) {
					has_more = true;
					break;
				}
			stduint next_test = has_more ? NewBlock("switch.test") : 0;
			Value constant = EmitConst(label.value, comparison_type);
			Instruction comparison;
			comparison.opcode = Opcode::ICmp;
			comparison.type = CloverType(comparison_type);
			comparison.has_result = true;
			comparison.result = NewValue(Type::I1);
			comparison.operands.Append(controlling.value);
			comparison.operands.Append(constant);
			comparison.predicate = ICmpPredicate::EQ;
			CurrentBlock().instructions.Append(comparison);
			EmitJump(comparison.result, function->blocks[label.block].name,
				has_more ? function->blocks[next_test].name : fallback);
			if (has_more) current_block = next_test;
		}
		if (!emitted_test) EmitJump(fallback);

		current_block = NewBlock("switch.body");
		switch_contexts.Append(context);
		break_targets.Append(end_name);
		bool body_lowered = LowerStatement(atoms, index);
		break_targets.Remove(break_targets.Count() - 1);
		switch_contexts.Remove(switch_contexts.Count() - 1);
		if (!body_lowered) return false;
		if (!BlockTerminated()) EmitJump(end_name);
		current_block = AppendBlock(end_name);
		return true;
	}

	bool FunctionBuilder::LowerWhile(uni::Nnode* node, const uni::Nnode* source_statement,
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
		LoopContext loop;
		loop.continue_target = function->blocks[condition_block].name;
		loop_contexts.Append(loop);
		break_targets.Append(end_name);
		bool body_lowered = LowerStatement(atoms, index);
		break_targets.Remove(break_targets.Count() - 1);
		loop_contexts.Remove(loop_contexts.Count() - 1);
		if (!body_lowered) return false;
		if (!BlockTerminated()) EmitJump(condition_block);
		current_block = AppendBlock(end_name);
		return true;
	}

	bool FunctionBuilder::LowerFor(uni::Nnode* node, const uni::Nnode* source_statement,
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
		LoopContext loop;
		loop.continue_target = step_name;
		loop_contexts.Append(loop);
		break_targets.Append(end_name);
		bool body_lowered = LowerStatement(atoms, index);
		break_targets.Remove(break_targets.Count() - 1);
		loop_contexts.Remove(loop_contexts.Count() - 1);
		if (!body_lowered) return false;
		if (!BlockTerminated()) EmitJump(step_name);

		current_block = AppendBlock(step_name);
		if (step && !LowerExpression(step).valid) return false;
		if (!BlockTerminated()) EmitJump(condition_block);
		current_block = AppendBlock(end_name);
		return true;
	}

	bool FunctionBuilder::LowerStatement(uni::Vector<StatementAtom>& atoms, stduint& index) {
		if (index >= atoms.Count())
			return Error("if requires a statement", nullptr).valid;
		StatementAtom atom = atoms.begin()[index++];
		uni::Nnode* node = atom.node;
		if (!node) return Error("missing statement", node).valid;

		if (node->type == ToToken(FrontendCToken::Empty)) return true;
		if (node->type == ToToken(FrontendCToken::EnumDeclaration)) return true;
		if (node->type == ToToken(FrontendCToken::EnumDefinition))
			return ActivateEnum(node,
				const_cast<uni::Nnode*>(node)->getParent());
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
		if (node->type == ToToken(FrontendCToken::Switch))
			return LowerSwitch(node, atom.source, atoms, index);
		if (node->type == ToToken(FrontendCToken::Case) ||
			node->type == ToToken(FrontendCToken::Default)) {
			if (!switch_contexts.Count())
				return Error(node->type == ToToken(FrontendCToken::Case) ?
					"case label not within a switch" :
					"default label not within a switch", node).valid;
			SwitchContext& context =
				switch_contexts[switch_contexts.Count() - 1];
			SwitchLabelTarget* target_label = nullptr;
			for (SwitchLabelTarget& candidate : context.labels)
				if (candidate.node == node) {
					target_label = &candidate;
					break;
				}
			if (!target_label)
				return Error("switch label belongs to another switch", node).valid;
			if (current_block != target_label->block && !BlockTerminated())
				EmitJump(target_label->block);
			current_block = target_label->block;
			if (index >= atoms.Count())
				return Error("switch label requires a statement", node).valid;
			return LowerStatement(atoms, index);
		}
		if (node->type == ToToken(FrontendCToken::Label)) {
			FunctionLabel* label = FindLabel(node->addr);
			if (!label) return Error("undefined label", node).valid;
			stduint first_instruction = CurrentBlock().instructions.Count();
			if (current_block != label->block && !BlockTerminated()) {
				EmitJump(label->block);
				AttachSource(first_instruction, atom.source);
			}
			current_block = label->block;
			if (index >= atoms.Count())
				return Error("label requires a statement", node).valid;
			return LowerStatement(atoms, index);
		}
		if (node->type == ToToken(FrontendCToken::Goto)) {
			if (index >= atoms.Count())
				return Error("goto requires a label name", node).valid;
			uni::Nnode* target_node = atoms.begin()[index++].node;
			if (!target_node || target_node->type != tok_identy || !target_node->addr)
				return Error("goto requires a label name", target_node).valid;
			FunctionLabel* label = FindLabel(target_node->addr);
			if (!label) return Error("use of undefined label", target_node).valid;
			stduint first_instruction = CurrentBlock().instructions.Count();
			EmitJump(label->block);
			AttachSource(first_instruction, atom.source);
			return true;
		}
		if (node->type == ToToken(FrontendCToken::Break)) {
			if (!break_targets.Count())
				return Error("break statement not within a loop or switch", node).valid;
			stduint first_instruction = CurrentBlock().instructions.Count();
			EmitJump(break_targets[break_targets.Count() - 1]);
			AttachSource(first_instruction, atom.source);
			return true;
		}
		if (node->type == ToToken(FrontendCToken::Continue)) {
			if (!loop_contexts.Count())
				return Error("continue statement not within a loop", node).valid;
			const LoopContext& loop = loop_contexts[loop_contexts.Count() - 1];
			stduint first_instruction = CurrentBlock().instructions.Count();
			EmitJump(loop.continue_target);
			AttachSource(first_instruction, atom.source);
			return true;
		}

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
		if (node->type == ToToken(FrontendCToken::ReturnVoid)) {
			if (!return_type.IsVoid())
				return Error("non-void function requires a return value", node).valid;
			Instruction instruction;
			instruction.opcode = Opcode::Ret;
			instruction.type = Type::Void;
			CurrentBlock().instructions.Append(instruction);
			AttachSource(first_instruction, atom.source);
			return true;
		}

		bool success = LowerExpression(node).valid;
		if (success) AttachSource(first_instruction, atom.source);
		return success;
	}

	bool FunctionBuilder::LowerBlock(uni::Nnode* block) {
		if (!block || block->type != tok_block) {
			Error("expected block", block);
			return false;
		}
		uni::Vector<StatementAtom> atoms;
		AppendStatementAtoms(block->subf, atoms);
		for (stduint index = 0; index < atoms.Count();) {
			uni::Nnode* next = atoms.begin()[index].node;
			if (BlockTerminated() && next &&
				next->type != ToToken(FrontendCToken::Label) &&
				next->type != ToToken(FrontendCToken::Case) &&
				next->type != ToToken(FrontendCToken::Default))
				current_block = NewBlock("unreachable");
			if (!LowerStatement(atoms, index) || failed) return false;
		}
		LeaveScope(block);
		return true;
	}

	bool FunctionBuilder::InitializeParameters(uni::Nnode* parameter_node,
		const uni::Nnode* body) {
		stduint index = 0;
		for (; parameter_node; parameter_node = parameter_node->next) {
			if (parameter_node->addr && !StrCompare(parameter_node->addr, "...")) break;
			if (!parameter_node->addr)
				return Error("invalid function parameter", parameter_node).valid;
			const FrontendCType* parameter_type = type_info->Find(parameter_node);
			if (!parameter_type)
				return Error("missing function parameter type", parameter_node).valid;
			FrontendCType type = *parameter_type;
			if (!PredeclareLocal(parameter_node->addr, type,
				parameter_node, body)) return false;
			LocalValue* local = ActivateLocal(parameter_node->addr, parameter_node);
			if (!local) return Error("invalid function parameter", parameter_node).valid;
			if (type.IsRecord()) {
				const FrontendCRecord* record = type_info->GetRecord(type.record_id);
				if (!record) return Error("missing record parameter layout", parameter_node).valid;
				for (const FrontendCRecordMember& member : record->members) {
					if (index >= function->parameters.Count())
						return Error("invalid flattened record parameter", parameter_node).valid;
					Value address = EmitByteOffset(local->address,
						member.offset_bits / target->char_bits);
					EmitStore(function->parameters.begin()[index++].value, address,
						OffsetAlignment(local->alignment,
							member.offset_bits / target->char_bits));
				}
			}
			else {
				if (index >= function->parameters.Count())
					return Error("invalid function parameter", parameter_node).valid;
				EmitStore(function->parameters.begin()[index++].value,
					local->address, local->alignment);
			}
		}
		return index == function->parameters.Count();
	}

	bool FunctionBuilder::Lower(uni::Nnode* body, uni::Nnode* parameters) {
		if (!CollectLabels(body)) return false;
		if (!InitializeParameters(parameters, body)) return false;
		if (!PredeclareBlock(body)) return false;
		if (!LowerBlock(body)) return false;
		if (return_type.IsVoid() && !BlockTerminated()) {
			Instruction instruction;
			instruction.opcode = Opcode::Ret;
			instruction.type = Type::Void;
			CurrentBlock().instructions.Append(instruction);
		}
		return true;
	}
}
