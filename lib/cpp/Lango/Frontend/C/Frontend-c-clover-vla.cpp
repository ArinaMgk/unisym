// ASCII C++11 TAB4 LF
// Docutitle: Lango C Frontend Clover Variable-Length Array Lowering
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

	Value FunctionBuilder::EmitStackSave() {
		Instruction instruction;
		instruction.opcode = Opcode::StackSave;
		instruction.type = Type::Ptr;
		instruction.has_result = true;
		instruction.result = NewValue(Type::Ptr);
		CurrentBlock().instructions.Append(instruction);
		return instruction.result;
	}

	void FunctionBuilder::EmitStackRestore(Value marker) {
		Instruction instruction;
		instruction.opcode = Opcode::StackRestore;
		instruction.type = Type::Void;
		instruction.operands.Append(marker);
		CurrentBlock().instructions.Append(instruction);
	}

	void FunctionBuilder::EmitDynamicCleanup(stduint target_depth) {
		if (target_depth >= dynamic_allocations.Count()) return;
		EmitStackRestore(dynamic_allocations[target_depth].marker);
	}

	void FunctionBuilder::TruncateDynamicAllocations(stduint target_depth) {
		while (dynamic_allocations.Count() > target_depth)
			dynamic_allocations.Remove(dynamic_allocations.Count() - 1);
	}

	Value FunctionBuilder::RuntimeArraySize(stduint shape_id,
		stduint dimension) const {
		if (!shape_id || shape_id > runtime_array_shapes.Count()) return Value();
		const RuntimeArrayShape& shape = runtime_array_shapes.begin()[shape_id - 1];
		if (dimension >= shape.size_count) return Value();
		return runtime_array_sizes.begin()[shape.first_size + dimension];
	}

	bool FunctionBuilder::BuildRuntimeArrayShape(const FrontendCType& type,
		stduint first_array_layer, const uni::Nnode* source,
		stduint& shape_id, Value& element_count,
		FrontendCType& element_type) {
		stduint array_end = first_array_layer;
		while (array_end < type.layer_count &&
			type.layers[array_end].kind == FrontendCTypeLayerKind::Array)
			array_end++;
		stduint dimension_count = array_end - first_array_layer;
		if (!dimension_count)
			return Error("variably modified type requires an array dimension",
				source).valid;

		uni::Vector<Value> counts;
		for (stduint layer_index = first_array_layer;
			layer_index < array_end; layer_index++) {
			const FrontendCTypeLayer& layer = type.layers[layer_index];
			if (layer.array_bound_kind == FrontendCArrayBoundKind::Constant) {
				counts.Append(EmitConst((stdsint)layer.array_count, SizeType()));
				continue;
			}
			if (layer.array_bound_kind != FrontendCArrayBoundKind::Variable ||
				!layer.array_bound_expression)
				return Error("variable length array requires a runtime bound",
					source).valid;
			FrontendCType bound_type;
			if (!ResolveExpressionType(const_cast<uni::Nnode*>(
				layer.array_bound_expression), bound_type) ||
				!bound_type.IsInteger())
				return Error("array bound expression must have integer type",
					layer.array_bound_expression).valid;
			LoweredValue count = LowerExpression(const_cast<uni::Nnode*>(
				layer.array_bound_expression));
			if (!count.valid) return false;
			count = ConvertInteger(count, SizeType(), layer.array_bound_expression);
			if (!count.valid) return false;
			counts.Append(count.value);
		}

		element_type = type;
		for (stduint layer = array_end; layer < type.layer_count; layer++)
			element_type.layers[layer - array_end] = type.layers[layer];
		for (stduint layer = type.layer_count - array_end;
			layer < element_type.layer_count; layer++)
			element_type.layers[layer] = {};
		element_type.layer_count = type.layer_count - array_end;
		if (element_type.HasVariablyModifiedType() ||
			!type_info->SizeBits(element_type, *target))
			return Error("variable length array requires a complete fixed-size element type",
				source).valid;

		FrontendCType size_type = SizeType();
		element_count = EmitConst(1, size_type);
		for (Value count : counts)
			element_count = EmitResult(Opcode::Mul, CloverType(size_type),
				element_count, count);

		RuntimeArrayShape shape;
		shape.first_size = runtime_array_sizes.Count();
		shape.size_count = dimension_count;
		for (stduint dimension = 0; dimension < dimension_count; dimension++)
			runtime_array_sizes.Append(Value());
		Value byte_size = EmitConst((stdsint)TypeSize(element_type), size_type);
		for (stduint dimension = dimension_count; dimension; dimension--) {
			byte_size = EmitResult(Opcode::Mul, CloverType(size_type),
				counts[dimension - 1], byte_size);
			runtime_array_sizes[shape.first_size + dimension - 1] = byte_size;
		}
		runtime_array_shapes.Append(shape);
		shape_id = runtime_array_shapes.Count();
		return true;
	}

	LoweredValue FunctionBuilder::LowerRuntimeTypeSize(
		const FrontendCType& type, const uni::Nnode* source) {
		if (!type.IsArray())
			return Error("runtime sizeof requires a variable length array", source);
		stduint shape_id = 0;
		Value element_count;
		FrontendCType element_type;
		if (!BuildRuntimeArrayShape(type, 0, source, shape_id,
			element_count, element_type)) return LoweredValue();
		FrontendCType result_type = SizeType();
		return { RuntimeArraySize(shape_id, 0), true,
			result_type, false };
	}

	bool FunctionBuilder::LowerVariableLengthArray(LocalValue& local,
		const FrontendCObjectDeclarator& object) {
		if (!object.type.IsArray() || !object.type.HasVariableArrayBound())
			return Error("invalid variable length array declaration",
				object.identifier).valid;
		if (object.initializer)
			return Error("variable length array cannot have an initializer",
				object.initializer).valid;

		stduint shape_id = 0;
		Value element_count;
		FrontendCType element_type;
		if (!BuildRuntimeArrayShape(object.type, 0, object.identifier,
			shape_id, element_count, element_type)) return false;
		Value runtime_size = RuntimeArraySize(shape_id, 0);
		Value marker = EmitStackSave();

		Instruction allocation;
		allocation.opcode = Opcode::Local;
		allocation.object_type = CloverObjectTypeFor(
			element_type, *target, *type_info);
		allocation.has_object_type = true;
		allocation.type = allocation.object_type.IsScalar() ?
			allocation.object_type.scalar : Type::I8;
		allocation.operands.Append(element_count);
		allocation.immediate = 0;
		allocation.alignment = TypeAlignment(element_type);
		allocation.has_result = true;
		allocation.result = NewValue(Type::Ptr);
		allocation.name = local.name;
		CurrentBlock().instructions.Append(allocation);

		local.address = allocation.result;
		local.type = object.type;
		local.alignment = allocation.alignment;
		local.runtime_element_count = element_count;
		local.runtime_size = runtime_size;
		local.stack_marker = marker;
		local.runtime_shape_id = shape_id;
		local.runtime_dimension = 0;
		local.is_variable_length = true;
		local.address_ready = true;

		DynamicLocalAllocation dynamic;
		dynamic.declaration = object.declaration;
		dynamic.scope = local.scope;
		dynamic.marker = marker;
		dynamic_allocations.Append(dynamic);
		return true;
	}

}
