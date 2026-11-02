// ASCII C++11 TAB4 LF
// Docutitle: Lango C Frontend Clover Expression Lowering
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

	stduint GreatestCommonDivisor(stduint left, stduint right) {
		while (right) {
			stduint remainder = left % right;
			left = right;
			right = remainder;
		}
		return left;
	}

	BasicBlock& FunctionBuilder::CurrentBlock() {
		return function->blocks[current_block];
	}

	BasicBlock& FunctionBuilder::EntryBlock() {
		return function->blocks[0];
	}

	bool FunctionBuilder::IsKeyword(const uni::Nnode* node, const char* keyword) const {
		return node && node->addr && !StrCompare(node->addr, keyword);
	}

	void FunctionBuilder::AppendUnsigned(uni::String& text, unsigned value) {
		char digits[16];
		stduint count = 0;
		do {
			digits[count++] = (char)('0' + value % 10);
			value /= 10;
		} while (value);
		while (count) text << digits[--count];
	}

	uni::String FunctionBuilder::NewBlockName(const char* prefix) {
		uni::String name;
		while (*prefix) name << *prefix++;
		name << '.';
		AppendUnsigned(name, next_block++);
		return name;
	}

	stduint FunctionBuilder::AppendBlock(const uni::String& name) {
		BasicBlock block;
		block.name = name;
		function->blocks.Append(block);
		return function->blocks.Count() - 1;
	}

	stduint FunctionBuilder::NewBlock(const char* prefix) {
		return AppendBlock(NewBlockName(prefix));
	}

	bool FunctionBuilder::BlockTerminated() const {
		const BasicBlock& block = function->blocks.begin()[current_block];
		if (!block.instructions.Count()) return false;
		Opcode opcode = block.instructions.begin()[block.instructions.Count() - 1].opcode;
		return opcode == Opcode::Jmp || opcode == Opcode::Ret;
	}

	void FunctionBuilder::EmitJump(stduint target) {
		EmitJump(function->blocks[target].name);
	}

	void FunctionBuilder::EmitJump(const uni::String& target) {
		Instruction instruction;
		instruction.opcode = Opcode::Jmp;
		instruction.target = target;
		CurrentBlock().instructions.Append(instruction);
	}

	void FunctionBuilder::EmitJump(Value condition, stduint target_true, stduint target_false) {
		EmitJump(condition, function->blocks[target_true].name,
			function->blocks[target_false].name);
	}

	void FunctionBuilder::EmitJump(Value condition, const uni::String& target_true,
		const uni::String& target_false) {
		Instruction instruction;
		instruction.opcode = Opcode::Jmp;
		instruction.type = Type::I1;
		instruction.operands.Append(condition);
		instruction.target = target_true;
		instruction.target_false = target_false;
		CurrentBlock().instructions.Append(instruction);
	}

	bool FunctionBuilder::SourceEmitted(const uni::Nnode* statement) const {
		for (const uni::Nnode* emitted : emitted_sources)
			if (emitted == statement) return true;
		return false;
	}

	void FunctionBuilder::AttachSource(Instruction& instruction, const uni::Nnode* statement) {
		if (!debug_info || !statement || SourceEmitted(statement)) return;
		const uni::String* source = debug_info->Find(statement);
		if (!source) return;
		instruction.debug_source = *source;
		const uni::TnodeField* field =
			const_cast<uni::Nnode*>(statement)->GetTnodeField();
		instruction.debug_file = debug_file_id;
		instruction.debug_line = (uint32)field->row;
		emitted_sources.Append(statement);
	}

	void FunctionBuilder::AttachSource(stduint instruction_index, const uni::Nnode* statement) {
		if (instruction_index >= CurrentBlock().instructions.Count()) return;
		AttachSource(CurrentBlock().instructions[instruction_index], statement);
	}

	Value FunctionBuilder::NewValue(Type type) {
		Value value;
		value.type = type;
		value.id = next_value++;
		return value;
	}

	Value FunctionBuilder::EmitResult(Opcode opcode, Type type, Value operand) {
		Instruction instruction;
		instruction.opcode = opcode;
		instruction.type = type;
		instruction.has_result = true;
		instruction.result = NewValue(type);
		instruction.operands.Append(operand);
		CurrentBlock().instructions.Append(instruction);
		return instruction.result;
	}

	Value FunctionBuilder::EmitResult(Opcode opcode, Type type, Value left, Value right) {
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

	Value FunctionBuilder::EmitPtrAdd(Value pointer, Value index, const FrontendCType& pointer_type) {
		FrontendCType pointee_type = pointer_type.DereferenceType();
		if (pointee_type.record_id) {
			stduint size_bits = type_info->SizeBits(pointee_type, *target);
			stduint size_bytes = size_bits / target->char_bits;
			Value scale = EmitConst((stdsint)size_bytes);
			if (index.type == Type::I64)
				scale = EmitConversion(Opcode::SExt, Type::I64, scale);
			index = EmitResult(Opcode::Mul, index.type, index, scale);
			Instruction instruction;
			instruction.opcode = Opcode::PtrAdd;
			instruction.type = Type::I8;
			instruction.has_result = true;
			instruction.result = NewValue(Type::Ptr);
			instruction.operands.Append(pointer);
			instruction.operands.Append(index);
			CurrentBlock().instructions.Append(instruction);
			return instruction.result;
		}
		stduint scalar_count = pointee_type.StorageElementCount();
		if (scalar_count > 1) {
			Value scale = EmitConst((stdsint)scalar_count);
			if (index.type == Type::I64)
				scale = EmitConversion(Opcode::SExt, Type::I64, scale);
			index = EmitResult(Opcode::Mul, index.type, index, scale);
		}
		Instruction instruction;
		instruction.opcode = Opcode::PtrAdd;
		instruction.type = CloverType(pointee_type);
		instruction.has_result = true;
		instruction.result = NewValue(Type::Ptr);
		instruction.operands.Append(pointer);
		instruction.operands.Append(index);
		CurrentBlock().instructions.Append(instruction);
		return instruction.result;
	}

	Value FunctionBuilder::EmitByteOffset(Value pointer, stduint offset) {
		if (!offset) return pointer;
		Value index = EmitConst((stdsint)offset);
		Instruction instruction;
		instruction.opcode = Opcode::PtrAdd;
		instruction.type = Type::I8;
		instruction.has_result = true;
		instruction.result = NewValue(Type::Ptr);
		instruction.operands.Append(pointer);
		instruction.operands.Append(index);
		CurrentBlock().instructions.Append(instruction);
		return instruction.result;
	}

	Value FunctionBuilder::EmitLoad(Type type, Value address, stduint alignment) {
		Instruction instruction;
		instruction.opcode = Opcode::Load;
		instruction.type = type;
		instruction.has_result = true;
		instruction.result = NewValue(type);
		instruction.operands.Append(address);
		instruction.alignment = alignment;
		CurrentBlock().instructions.Append(instruction);
		return instruction.result;
	}

	void FunctionBuilder::EmitStore(Value value, Value address, stduint alignment) {
		Instruction instruction;
		instruction.opcode = Opcode::Store;
		instruction.type = value.type;
		instruction.operands.Append(value);
		instruction.operands.Append(address);
		instruction.alignment = alignment;
		CurrentBlock().instructions.Append(instruction);
	}

	stduint FunctionBuilder::OffsetAlignment(stduint base_alignment, stduint offset) const {
		if (!base_alignment) base_alignment = 1;
		return offset ? GreatestCommonDivisor(base_alignment, offset) :
			base_alignment;
	}

	stduint FunctionBuilder::PointerStepAlignment(stduint pointer_alignment,
		const FrontendCType& pointer_type) const {
		if (!pointer_type.IsPointer()) return 0;
		if (!pointer_alignment) pointer_alignment = 1;
		stduint element_size = TypeSize(pointer_type.DereferenceType());
		return element_size ? GreatestCommonDivisor(
			pointer_alignment, element_size) : 0;
	}

	LoweredValue FunctionBuilder::ToRValue(LoweredValue lowered, const uni::Nnode* node) {
		if (lowered.valid && lowered.type.IsArray()) {
			lowered.type = lowered.type.DecayType();
			lowered.is_lvalue = false;
			return lowered;
		}
		if (lowered.valid && lowered.type.IsRecord())
			return Error("record value operations are not supported yet", node);
		if (!lowered.valid || !lowered.is_lvalue) return lowered;
		lowered.value = EmitLoad(CloverType(lowered.type), lowered.value,
			lowered.alignment);
		lowered.is_lvalue = false;
		// Pointer alignment is not encoded in a loaded pointer value yet.
		lowered.alignment = lowered.type.IsPointer() ? 1 : 0;
		return lowered;
	}

	Value FunctionBuilder::AsCondition(LoweredValue lowered, const uni::Nnode* node) {
		lowered = ToRValue(lowered, node);
		if (!lowered.valid || !lowered.type.IsScalar()) {
			Error("condition requires a scalar expression", node);
			return lowered.value;
		}
		if (lowered.value.type == Type::I1) return lowered.value;
		Value scalar = lowered.value;
		Instruction instruction;
		instruction.opcode = lowered.type.IsFloating() ? Opcode::FCmp : Opcode::ICmp;
		instruction.type = CloverType(lowered.type);
		instruction.has_result = true;
		instruction.result = NewValue(Type::I1);
		instruction.operands.Append(scalar);
		Value zero;
		if (lowered.type.IsFloating()) zero = EmitFloatConst(0, lowered.type);
		else if (lowered.type.IsPointer()) {
			Type integer_type = target->pointer_bits == 32 ? Type::I32 : Type::I64;
			zero = EmitConversion(Opcode::IntToPtr, Type::Ptr,
				EmitTypedConst(integer_type, 0));
		}
		else zero = EmitConst(0, lowered.type);
		instruction.operands.Append(zero);
		if (lowered.type.IsFloating())
			instruction.float_predicate = FCmpPredicate::NE;
		else instruction.predicate = ICmpPredicate::NE;
		CurrentBlock().instructions.Append(instruction);
		return instruction.result;
	}

	Value FunctionBuilder::EmitConst(stdsint immediate,
		const FrontendCType& type) {
		Instruction instruction;
		instruction.opcode = Opcode::Const;
		instruction.type = CloverType(type);
		instruction.has_result = true;
		instruction.result = NewValue(instruction.type);
		instruction.immediate = immediate;
		CurrentBlock().instructions.Append(instruction);
		return instruction.result;
	}

	Value FunctionBuilder::EmitTypedConst(Type type, stdsint immediate) {
		Instruction instruction;
		instruction.opcode = Opcode::Const;
		instruction.type = type;
		instruction.has_result = true;
		instruction.result = NewValue(type);
		instruction.immediate = immediate;
		CurrentBlock().instructions.Append(instruction);
		return instruction.result;
	}

	Value FunctionBuilder::EmitFloatConst(stdsint bits,
		const FrontendCType& type) {
		Instruction instruction;
		instruction.opcode = Opcode::FConst;
		instruction.type = CloverType(type);
		instruction.has_result = true;
		instruction.result = NewValue(instruction.type);
		instruction.immediate = bits;
		CurrentBlock().instructions.Append(instruction);
		return instruction.result;
	}

	LoweredValue FunctionBuilder::CreateTemporaryStorage(const FrontendCType& type) {
		Instruction instruction;
		instruction.opcode = Opcode::Local;
		instruction.type = CloverType(type);
		instruction.immediate = 1;
		instruction.alignment = TypeAlignment(type);
		instruction.has_result = true;
		instruction.result = NewValue(Type::Ptr);
		CurrentBlock().instructions.Append(instruction);
		return { instruction.result, true, type, true,
			instruction.alignment };
	}

	LocalValue* FunctionBuilder::FindLocal(const char* name) {
		for (stduint index = locals.Count(); index; index--) {
			LocalValue& local = locals[index - 1];
			if (local.active && !StrCompare(local.name.reference(), name)) return &local;
		}
		return nullptr;
	}

	const FrontendCFunction* FunctionBuilder::FindFunction(const char* name) const {
		if (!visible_functions) return nullptr;
		for (const FrontendCFunction& candidate : *visible_functions)
			if (!StrCompare(candidate.name.reference(), name)) return &candidate;
		return nullptr;
	}

	const FrontendCGlobal* FunctionBuilder::FindGlobal(const char* name) const {
		if (!visible_globals) return nullptr;
		for (const FrontendCGlobal& candidate : *visible_globals)
			if (!StrCompare(candidate.name.reference(), name)) return &candidate;
		return nullptr;
	}

	const FrontendCEnumerator* FunctionBuilder::FindConstant(const char* name) const {
		for (stduint index = local_constants.Count(); index; index--) {
			const FrontendCLocalConstant& candidate = local_constants.begin()[index - 1];
			if (candidate.active && candidate.constant &&
				!StrCompare(candidate.constant->name.reference(), name))
				return candidate.constant;
		}
		if (!visible_constants) return nullptr;
		for (const FrontendCEnumerator& candidate : *visible_constants)
			if (!StrCompare(candidate.name.reference(), name)) return &candidate;
		return nullptr;
	}

	Value FunctionBuilder::EmitGlobalAddress(const uni::String& name) {
		Instruction instruction;
		instruction.opcode = Opcode::GlobalAddr;
		instruction.type = Type::Ptr;
		instruction.has_result = true;
		instruction.result = NewValue(Type::Ptr);
		instruction.name = name;
		CurrentBlock().instructions.Append(instruction);
		return instruction.result;
	}

	Value FunctionBuilder::EmitGlobalAddress(const FrontendCGlobal& global) {
		return EmitGlobalAddress(global.name);
	}

	LoweredValue FunctionBuilder::Error(const char* message, const uni::Nnode* node) {
		failed = true;
		diagnostics->Report(DiagnosticStage::Lowering, message, node);
		return {};
	}

	void FunctionBuilder::AppendCallArguments(uni::Nnode* node,
		uni::Vector<uni::Nnode*>& arguments) {
		if (node && node->type == tok_func && node->addr &&
			!StrCompare(node->addr, "OP@COMMA")) {
			for (uni::Nnode* child = node->subf; child; child = child->next)
				AppendCallArguments(child, arguments);
			return;
		}
		if (node) arguments.Append(node);
	}

	LoweredValue FunctionBuilder::LowerCall(uni::Nnode* node) {
		const FrontendCFunction* signature = FindFunction(node->addr);
		if (!signature) return Error("call to undeclared function", node);
		uni::Vector<uni::Nnode*> arguments;
		AppendCallArguments(node->subf, arguments);
		if (arguments.Count() != signature->parameter_types.Count())
			return Error("function argument count mismatch", node);

		Instruction instruction;
		instruction.opcode = Opcode::Call;
		instruction.type = CloverType(signature->return_type);
		instruction.callee = signature->name;
		for (stduint index = 0; index < arguments.Count(); index++) {
			uni::Nnode* argument_node = arguments.begin()[index];
			LoweredValue argument = ToRValue(
				LowerExpression(argument_node), argument_node);
			if (!argument.valid) return argument;
			const FrontendCType& parameter_type =
				signature->parameter_types.begin()[index];
			if (parameter_type.IsArithmetic())
				argument = ConvertArithmetic(argument, parameter_type, argument_node);
			else if (!parameter_type.CanAssignFrom(argument.type))
				return Error("incompatible function argument type", argument_node);
			if (!argument.valid) return argument;
			instruction.operands.Append(argument.value);
		}
		instruction.has_result = !signature->return_type.IsVoid();
		if (instruction.has_result) instruction.result = NewValue(instruction.type);
		CurrentBlock().instructions.Append(instruction);
		return { instruction.result, true, signature->return_type, false };
	}

	LoweredValue FunctionBuilder::LowerSubscriptOperands(LoweredValue left,
		uni::Nnode* left_node, uni::Nnode* right_node, uni::Nnode* node) {
		left = ToRValue(left, left_node);
		LoweredValue right = ToRValue(LowerExpression(right_node), right_node);
		if (!left.valid || !right.valid) return {};
		if (left.type.IsInteger() && right.type.IsPointer()) {
			LoweredValue temporary = left;
			left = right;
			right = temporary;
		}
		if (!left.type.IsPointer() || !right.type.IsInteger())
			return Error("array subscript requires a pointer and an integer", node);
		FrontendCType index_type = PromoteType(right.type);
		right = ConvertInteger(right, index_type, right_node);
		if (!right.valid) return right;
		Value index = right.value;
		Value address = EmitPtrAdd(left.value, index, left.type);
		return { address, !failed, left.type.DereferenceType(), true,
			PointerStepAlignment(left.alignment, left.type) };
	}

	LoweredValue FunctionBuilder::LowerSubscript(uni::Nnode* node) {
		uni::Nnode* left_node = node->subf;
		uni::Nnode* right_node = left_node ? left_node->next : nullptr;
		if (!left_node || !right_node || right_node->next)
			return Error("array subscript requires two operands", node);
		return LowerSubscriptOperands(LowerExpression(left_node),
			left_node, right_node, node);
	}

	const FrontendCRecordMember* FunctionBuilder::FindRecordMember(
		const FrontendCType& type, const char* name) const {
		if (!type.IsRecord() || !name) return nullptr;
		const FrontendCRecord* record = type_info->GetRecord(type.record_id);
		if (!record || !record->complete) return nullptr;
		for (const FrontendCRecordMember& member : record->members)
			if (!StrCompare(member.name.reference(), name)) return &member;
		return nullptr;
	}

	FrontendCType FunctionBuilder::MemberType(const FrontendCType& record_type,
		const FrontendCRecordMember& member) const {
		FrontendCType type = member.type;
		if (record_type.IsConstObject()) {
			if (type.pointer_depth)
				type.pointer_qualifiers[type.pointer_depth - 1].is_const = true;
			else type.base_qualifiers.is_const = true;
		}
		return type;
	}

	LoweredValue FunctionBuilder::LowerRecordMember(Value record_address,
		const FrontendCType& record_type, stduint record_alignment,
		uni::Nnode* designator) {
		if (!designator)
			return Error("member access requires a member name", designator);
		if (designator->type == tok_func && designator->addr &&
			!StrCompare(designator->addr, "operator[]")) {
			uni::Nnode* base = designator->subf;
			uni::Nnode* index = base ? base->next : nullptr;
			if (!base || !index || index->next)
				return Error("array subscript requires two operands", designator);
			LoweredValue member = LowerRecordMember(
				record_address, record_type, record_alignment, base);
			if (!member.valid) return member;
			return LowerSubscriptOperands(member, base, index, designator);
		}
		if (designator->type != tok_identy || !designator->addr)
			return Error("member access requires a member name", designator);
		const FrontendCRecordMember* member =
			FindRecordMember(record_type, designator->addr);
		if (!member) return Error("record has no such member", designator);
		if (member->offset_bits % target->char_bits)
			return Error("record member is not byte-addressable", designator);
		Value address = EmitByteOffset(record_address,
			member->offset_bits / target->char_bits);
		return { address, true, MemberType(record_type, *member), true,
			OffsetAlignment(record_alignment,
				member->offset_bits / target->char_bits) };
	}

	LoweredValue FunctionBuilder::LowerMember(uni::Nnode* node) {
		uni::Nnode* object_node = node ? node->subf : nullptr;
		uni::Nnode* member_node = object_node ? object_node->next : nullptr;
		if (!object_node || !member_node || member_node->next)
			return Error("member access requires a member name", node);
		LoweredValue object = LowerExpression(object_node);
		if (!object.valid) return object;
		if (!object.is_lvalue || !object.type.IsRecord())
			return Error("member access requires a record object", object_node);
		return LowerRecordMember(object.value, object.type,
			object.alignment, member_node);
	}

	LoweredValue FunctionBuilder::LowerPointerMember(uni::Nnode* node) {
		uni::Nnode* pointer_node = node ? node->subf : nullptr;
		uni::Nnode* member_node = pointer_node ? pointer_node->next : nullptr;
		if (!pointer_node || !member_node || member_node->next)
			return Error("pointer member access requires a member name", node);
		LoweredValue pointer = ToRValue(
			LowerExpression(pointer_node), pointer_node);
		if (!pointer.valid) return pointer;
		if (!pointer.type.IsPointer())
			return Error("pointer member access requires a pointer", pointer_node);
		FrontendCType record_type = pointer.type.DereferenceType();
		if (!record_type.IsRecord())
			return Error("pointer member access requires a record pointer", pointer_node);
		return LowerRecordMember(pointer.value, record_type,
			pointer.alignment, member_node);
	}

	LoweredValue FunctionBuilder::LowerIncrement(uni::Nnode* operand, bool increment,
		bool return_updated) {
		LoweredValue lowered = LowerExpression(operand);
		if (!lowered.valid) return lowered;
		if (!lowered.is_lvalue || lowered.type.IsConstObject() ||
			(!lowered.type.IsArithmetic() && !lowered.type.IsPointer()))
			return Error("increment requires an arithmetic or pointer lvalue", operand);
		Value previous = EmitLoad(CloverType(lowered.type), lowered.value,
			lowered.alignment);
		FrontendCType arithmetic_type = lowered.type.IsInteger() ?
			PromoteType(lowered.type) : lowered.type;
		Value one;
		if (lowered.type.IsFloating()) {
			stdsint bits;
			FrontendCType literal_type;
			if (!ParseFloatingLiteral(lowered.type.scalar_kind ==
				FrontendCScalarKind::Float ? "1.0f" : "1.0", bits, literal_type))
				return Error("cannot create floating increment constant", operand);
			one = EmitFloatConst(bits, literal_type);
		}
		else one = EmitConst(1, lowered.type.IsInteger() ?
			arithmetic_type : IntType());
		Value updated;
		stduint previous_alignment = lowered.type.IsPointer() ? 1 : 0;
		stduint updated_alignment = 0;
		if (lowered.type.IsPointer()) {
			Value index = one;
			if (!increment) {
				Value zero = EmitConst(0);
				index = EmitResult(Opcode::Sub, Type::I32, zero, one);
			}
			updated = EmitPtrAdd(previous, index, lowered.type);
			updated_alignment = PointerStepAlignment(
				previous_alignment, lowered.type);
		}
		else {
			LoweredValue promoted = ConvertArithmetic(
				{ previous, true, lowered.type, false }, arithmetic_type, operand);
			updated = EmitResult(lowered.type.IsFloating() ?
				(increment ? Opcode::FAdd : Opcode::FSub) :
				(increment ? Opcode::Add : Opcode::Sub),
				CloverType(arithmetic_type), promoted.value, one);
			LoweredValue converted = ConvertArithmetic(
				{ updated, true, arithmetic_type, false }, lowered.type, operand);
			if (!converted.valid) return converted;
			updated = converted.value;
		}
		EmitStore(updated, lowered.value, lowered.alignment);
		return { return_updated ? updated : previous, true, lowered.type, false,
			return_updated ? updated_alignment : previous_alignment };
	}

	bool FunctionBuilder::IsCompoundAssignment(const char* operation) const {
		return !StrCompare(operation, "OP@AGNSUM") ||
			!StrCompare(operation, "OP@AGNDIF") ||
			!StrCompare(operation, "OP@AGNPRO") ||
			!StrCompare(operation, "OP@AGNQUO") ||
			!StrCompare(operation, "OP@AGNREM") ||
			!StrCompare(operation, "OP@AGNSHL") ||
			!StrCompare(operation, "OP@AGNSHR") ||
			!StrCompare(operation, "OP@AGNAND") ||
			!StrCompare(operation, "OP@AGNXOR") ||
			!StrCompare(operation, "OP@ASGNOR");
	}

	LoweredValue FunctionBuilder::LowerCompoundAssignment(uni::Nnode* node,
		const char* operation, uni::Nnode* left_node, uni::Nnode* right_node) {
		if (!right_node || right_node->next)
			return Error("compound assignment requires two operands", node);
		LoweredValue left = LowerExpression(left_node);
		if (!left.valid) return left;
		if (!left.is_lvalue)
			return Error("compound assignment requires an lvalue", left_node);
		if (left.type.IsRecord())
			return Error("record compound assignment is not supported", node);
		if (left.type.IsConstObject())
			return Error("assignment to const-qualified object", left_node);

		Value previous = EmitLoad(CloverType(left.type), left.value, left.alignment);
		LoweredValue right = ToRValue(LowerExpression(right_node), right_node);
		if (!right.valid) return right;

		if (left.type.IsPointer()) {
			bool subtract = !StrCompare(operation, "OP@AGNDIF");
			if ((!StrCompare(operation, "OP@AGNSUM") || subtract) &&
				right.type.IsInteger()) {
				FrontendCType index_type = PromoteType(right.type);
				right = ConvertInteger(right, index_type, right_node);
				if (!right.valid) return right;
				Value index = right.value;
				if (subtract) {
					Value zero = EmitConst(0, index_type);
					index = EmitResult(Opcode::Sub,
						CloverType(index_type), zero, index);
				}
				Value updated = EmitPtrAdd(previous, index, left.type);
				EmitStore(updated, left.value, left.alignment);
				return { updated, !failed, left.type, false,
					PointerStepAlignment(1, left.type) };
			}
			return Error("pointer compound assignment requires += or -= with an integer",
				node);
		}

		if (!left.type.IsArithmetic() || !right.type.IsArithmetic())
			return Error("compound assignment requires arithmetic operands", node);
		FrontendCType arithmetic_type;
		bool is_shift = !StrCompare(operation, "OP@AGNSHL") ||
			!StrCompare(operation, "OP@AGNSHR");
		bool integer_only = is_shift || !StrCompare(operation, "OP@AGNREM") ||
			!StrCompare(operation, "OP@AGNAND") ||
			!StrCompare(operation, "OP@AGNXOR") ||
			!StrCompare(operation, "OP@ASGNOR");
		if (integer_only && (!left.type.IsInteger() || !right.type.IsInteger()))
			return Error("integer-only compound assignment used with floating operands", node);
		FrontendCType right_arithmetic_type;
		if (is_shift) {
			arithmetic_type = PromoteType(left.type);
			right_arithmetic_type = PromoteType(right.type);
		}
		else {
			arithmetic_type = CommonArithmeticType(left.type, right.type);
			right_arithmetic_type = arithmetic_type;
		}
		LoweredValue lhs = ConvertArithmetic(
			{ previous, true, left.type, false }, arithmetic_type, left_node);
		right = ConvertArithmetic(right, right_arithmetic_type, right_node);
		if (!lhs.valid || !right.valid) return {};

		Opcode opcode;
		if (!StrCompare(operation, "OP@AGNSUM"))
			opcode = arithmetic_type.IsFloating() ? Opcode::FAdd : Opcode::Add;
		else if (!StrCompare(operation, "OP@AGNDIF"))
			opcode = arithmetic_type.IsFloating() ? Opcode::FSub : Opcode::Sub;
		else if (!StrCompare(operation, "OP@AGNPRO"))
			opcode = arithmetic_type.IsFloating() ? Opcode::FMul : Opcode::Mul;
		else if (!StrCompare(operation, "OP@AGNQUO"))
			opcode = arithmetic_type.IsFloating() ? Opcode::FDiv :
				arithmetic_type.is_signed ? Opcode::SDiv : Opcode::UDiv;
		else if (!StrCompare(operation, "OP@AGNREM"))
			opcode = arithmetic_type.is_signed ? Opcode::SRem : Opcode::URem;
		else if (!StrCompare(operation, "OP@AGNSHL")) opcode = Opcode::Shl;
		else if (!StrCompare(operation, "OP@AGNSHR"))
			opcode = arithmetic_type.is_signed ? Opcode::AShr : Opcode::LShr;
		else if (!StrCompare(operation, "OP@AGNAND")) opcode = Opcode::And;
		else if (!StrCompare(operation, "OP@AGNXOR")) opcode = Opcode::Xor;
		else opcode = Opcode::Or;

		Value calculated = EmitResult(opcode, CloverType(arithmetic_type),
			lhs.value, right.value);
		LoweredValue converted = ConvertArithmetic(
			{ calculated, true, arithmetic_type, false }, left.type, node);
		if (!converted.valid) return converted;
		EmitStore(converted.value, left.value, left.alignment);
		return converted;
	}

	LoweredValue FunctionBuilder::LowerLogical(uni::Nnode* node, bool is_and,
		uni::Nnode* left_node, uni::Nnode* right_node) {
		if (!right_node || right_node->next)
			return Error("logical operator requires two operands", node);
		FrontendCType result_type = IntType();
		LoweredValue storage = CreateTemporaryStorage(result_type);
		LoweredValue left = LowerExpression(left_node);
		if (!left.valid) return left;
		Value condition = AsCondition(left, left_node);
		if (failed) return {};

		stduint right_block = NewBlock(is_and ? "land.right" : "lor.right");
		stduint short_block = NewBlock(is_and ? "land.short" : "lor.short");
		stduint end_block = NewBlock(is_and ? "land.end" : "lor.end");
		if (is_and) EmitJump(condition, right_block, short_block);
		else EmitJump(condition, short_block, right_block);

		current_block = short_block;
		Value short_value = EmitConst(is_and ? 0 : 1, result_type);
		EmitStore(short_value, storage.value, storage.alignment);
		EmitJump(end_block);

		current_block = right_block;
		LoweredValue right = LowerExpression(right_node);
		if (!right.valid) return right;
		Value right_condition = AsCondition(right, right_node);
		if (failed) return {};
		LoweredValue right_value = ConvertInteger(
			{ right_condition, true, result_type, false }, result_type, right_node);
		if (!right_value.valid) return right_value;
		EmitStore(right_value.value, storage.value, storage.alignment);
		EmitJump(end_block);

		current_block = end_block;
		Value result = EmitLoad(Type::I32, storage.value, storage.alignment);
		return { result, true, result_type, false };
	}

	LoweredValue FunctionBuilder::ConvertConditionalBranch(LoweredValue branch,
		const FrontendCType& result_type, uni::Nnode* branch_node) {
		branch = ToRValue(branch, branch_node);
		if (!branch.valid) return branch;
		if (result_type.IsArithmetic())
			return ConvertArithmetic(branch, result_type, branch_node);
		if (result_type.IsPointer() && result_type.CanAssignFrom(branch.type)) {
			branch.type = result_type;
			return branch;
		}
		return Error("incompatible conditional branch type", branch_node);
	}

	LoweredValue FunctionBuilder::LowerConditional(uni::Nnode* node, uni::Nnode* condition_node,
		uni::Nnode* true_node, uni::Nnode* false_node) {
		if (!true_node || !false_node || false_node->next)
			return Error("conditional operator requires three operands", node);
		FrontendCType true_type, false_type, result_type;
		if (!ResolveExpressionType(true_node, true_type) ||
			!ResolveExpressionType(false_node, false_type) ||
			!CommonConditionalType(true_type, false_type, result_type))
			return Error("incompatible conditional branch types", node);

		LoweredValue storage = CreateTemporaryStorage(result_type);
		LoweredValue condition_value = LowerExpression(condition_node);
		if (!condition_value.valid) return condition_value;
		Value condition = AsCondition(condition_value, condition_node);
		if (failed) return {};
		stduint true_block = NewBlock("cond.true");
		stduint false_block = NewBlock("cond.false");
		stduint end_block = NewBlock("cond.end");
		EmitJump(condition, true_block, false_block);

		current_block = true_block;
		LoweredValue true_value = ConvertConditionalBranch(
			LowerExpression(true_node), result_type, true_node);
		if (!true_value.valid) return true_value;
		EmitStore(true_value.value, storage.value, storage.alignment);
		EmitJump(end_block);

		current_block = false_block;
		LoweredValue false_value = ConvertConditionalBranch(
			LowerExpression(false_node), result_type, false_node);
		if (!false_value.valid) return false_value;
		EmitStore(false_value.value, storage.value, storage.alignment);
		EmitJump(end_block);

		current_block = end_block;
		Value result = EmitLoad(CloverType(result_type),
			storage.value, storage.alignment);
		return { result, true, result_type, false,
			result_type.IsPointer() ? 1u : 0u };
	}

	LoweredValue FunctionBuilder::LowerOperator(uni::Nnode* node) {
		using namespace uni;
		if (!node->addr && node->subf) return LowerExpression(node->subf);
		if (!node->addr || !node->subf) return Error("bad operator", node);

		Nnode* left_node = node->subf;
		Nnode* right_node = left_node->next;
		const char* operation = node->addr;
		if (!StrCompare(operation, "OP@LOGAND"))
			return LowerLogical(node, true, left_node, right_node);
		if (!StrCompare(operation, "OP@LOGIOR"))
			return LowerLogical(node, false, left_node, right_node);
		if (!StrCompare(operation, "OP@TERNAR"))
			return LowerConditional(node, left_node, right_node,
				right_node ? right_node->next : nullptr);
		if (!StrCompare(operation, "OP@CAST"))
			return LowerCast(node, left_node);

		if (!StrCompare(operation, "OP@SUFADD"))
			return LowerIncrement(left_node, true, false);
		if (!StrCompare(operation, "OP@SUFSUB"))
			return LowerIncrement(left_node, false, false);
		if (!StrCompare(operation, "OP@PREADD"))
			return LowerIncrement(left_node, true, true);
		if (!StrCompare(operation, "OP@PRESUB"))
			return LowerIncrement(left_node, false, true);
		if (!StrCompare(operation, "OP@SIZEOF"))
			return LowerSizeof(left_node);
		if (!StrCompare(operation, "OP@SUFMEMB"))
			return LowerMember(node);
		if (!StrCompare(operation, "OP@SUFMEMP"))
			return LowerPointerMember(node);
		if (!StrCompare(operation, "OP@PREADDR")) {
			LoweredValue operand = LowerExpression(left_node);
			if (!operand.valid) return operand;
			if (!operand.is_lvalue)
				return Error("cannot take the address of an rvalue", left_node);
			return { operand.value, true, operand.type.AddressType(), false,
				operand.alignment };
		}
		if (!StrCompare(operation, "OP@PREMEMB")) {
			LoweredValue operand = ToRValue(LowerExpression(left_node), left_node);
			if (!operand.valid) return operand;
			if (!operand.type.IsPointer())
				return Error("dereference requires a pointer", left_node);
			return { operand.value, true, operand.type.DereferenceType(), true,
				operand.alignment ? operand.alignment : 1 };
		}
		if (!StrCompare(operation, "OP@PREPOSI")) {
			LoweredValue operand = ToRValue(LowerExpression(left_node), left_node);
			if (!operand.valid) return operand;
			if (!operand.type.IsArithmetic())
				return Error("unary plus requires an arithmetic operand", left_node);
			return operand.type.IsInteger() ?
				ConvertInteger(operand, PromoteType(operand.type), left_node) : operand;
		}
		if (!StrCompare(operation, "OP@PRENEGA")) {
			LoweredValue operand = ToRValue(LowerExpression(left_node), left_node);
			if (!operand.valid) return operand;
			if (!operand.type.IsArithmetic())
				return Error("unary minus requires an arithmetic operand", left_node);
			FrontendCType arithmetic_type = operand.type.IsInteger() ?
				PromoteType(operand.type) : operand.type;
			operand = ConvertArithmetic(operand, arithmetic_type, left_node);
			if (!operand.valid) return operand;
			Value zero = arithmetic_type.IsFloating() ?
				EmitFloatConst(0, arithmetic_type) : EmitConst(0, arithmetic_type);
			Value value = EmitResult(arithmetic_type.IsFloating() ?
				Opcode::FSub : Opcode::Sub, CloverType(arithmetic_type),
				zero, operand.value);
			return { value, !failed, arithmetic_type, false };
		}
		if (!StrCompare(operation, "OP@LOGNOT")) {
			Value condition = AsCondition(LowerExpression(left_node), left_node);
			if (failed) return {};
			Value zero = EmitTypedConst(Type::I1, 0);
			Instruction instruction;
			instruction.opcode = Opcode::ICmp;
			instruction.type = Type::I1;
			instruction.has_result = true;
			instruction.result = NewValue(Type::I1);
			instruction.operands.Append(condition);
			instruction.operands.Append(zero);
			instruction.predicate = ICmpPredicate::EQ;
			CurrentBlock().instructions.Append(instruction);
			return { instruction.result, true, IntType(), false };
		}
		if (!StrCompare(operation, "OP@BITNOT")) {
			LoweredValue operand = ToRValue(LowerExpression(left_node), left_node);
			if (!operand.valid || !operand.type.IsInteger())
				return Error("bitwise not requires an integer operand", left_node);
			FrontendCType arithmetic_type = PromoteType(operand.type);
			operand = ConvertInteger(operand, arithmetic_type, left_node);
			if (!operand.valid) return operand;
			Value ones = EmitConst(-1, arithmetic_type);
			Value value = EmitResult(Opcode::Xor, CloverType(arithmetic_type),
				operand.value, ones);
			return { value, true, arithmetic_type, false };
		}

		if (IsCompoundAssignment(operation))
			return LowerCompoundAssignment(node, operation, left_node, right_node);

		if (!StrCompare(operation, "OP@ASSIGN")) {
			if (!right_node) return Error("assignment requires two operands", node);
			LoweredValue left = LowerExpression(left_node);
			if (!left.valid) return left;
			if (!left.is_lvalue) return Error("assignment requires an lvalue", left_node);
			if (left.type.IsRecord())
				return Error("record assignment is not supported yet", node);
			if (left.type.IsConstObject())
				return Error("assignment to const-qualified object", left_node);
			LoweredValue right = ToRValue(LowerExpression(right_node), right_node);
			if (!right.valid) return right;
			if (left.type.IsArithmetic())
				right = ConvertArithmetic(right, left.type, right_node);
			else if (!left.type.CanAssignFrom(right.type))
				return Error("incompatible assignment types", node);
			if (!right.valid) return right;
			EmitStore(right.value, left.value, left.alignment);
			return right;
		}
		if (!StrCompare(operation, "OP@COMMA")) {
			if (!right_node) return Error("comma operator requires two operands", node);
			LoweredValue result;
			for (Nnode* operand = left_node; operand; operand = operand->next) {
				result = ToRValue(LowerExpression(operand), operand);
				if (!result.valid) return result;
			}
			return result;
		}

		if (!right_node) return Error("binary operator requires two operands", node);
		LoweredValue left = LowerExpression(left_node);
		LoweredValue right = LowerExpression(right_node);
		if (!left.valid || !right.valid) return {};

		if (!StrCompare(operation, "OP@ARIADD") ||
			!StrCompare(operation, "OP@ARISUB")) {
			left = ToRValue(left, left_node);
			right = ToRValue(right, right_node);
			bool subtract = !StrCompare(operation, "OP@ARISUB");
			if (left.type.IsPointer() && right.type.IsInteger()) {
				FrontendCType index_type = PromoteType(right.type);
				right = ConvertInteger(right, index_type, right_node);
				Value index = right.value;
				if (subtract) {
					Value zero = EmitConst(0, index_type);
					index = EmitResult(Opcode::Sub, CloverType(index_type), zero, index);
				}
				Value value = EmitPtrAdd(left.value, index, left.type);
				return { value, !failed, left.type, false,
					PointerStepAlignment(left.alignment, left.type) };
			}
			if (!subtract && left.type.IsInteger() && right.type.IsPointer()) {
				FrontendCType index_type = PromoteType(left.type);
				left = ConvertInteger(left, index_type, left_node);
				Value index = left.value;
				Value value = EmitPtrAdd(right.value, index, right.type);
				return { value, !failed, right.type, false,
					PointerStepAlignment(right.alignment, right.type) };
			}
			if (left.type.IsPointer() || right.type.IsPointer())
				return Error("invalid operands to pointer arithmetic", node);
		}

		left = ToRValue(left, left_node);
		right = ToRValue(right, right_node);
		if (!left.valid || !right.valid || !left.type.IsArithmetic() ||
			!right.type.IsArithmetic()) return Error("expected arithmetic operands", node);
		bool is_shift = !StrCompare(operation, "OP@BITSHL") ||
			!StrCompare(operation, "OP@BITSHR");
		FrontendCType arithmetic_type;
		FrontendCType right_arithmetic_type;
		if (is_shift) {
			if (!left.type.IsInteger() || !right.type.IsInteger())
				return Error("shift requires integer operands", node);
			arithmetic_type = PromoteType(left.type);
			right_arithmetic_type = PromoteType(right.type);
		}
		else {
			arithmetic_type = CommonArithmeticType(left.type, right.type);
			right_arithmetic_type = arithmetic_type;
		}
		left = ConvertArithmetic(left, arithmetic_type, left_node);
		right = ConvertArithmetic(right, right_arithmetic_type, right_node);
		if (!left.valid || !right.valid) return {};
		Value lhs = left.value;
		Value rhs = right.value;
		if (arithmetic_type.IsFloating() &&
			(!StrCompare(operation, "OP@ARIREM") ||
			!StrCompare(operation, "OP@BITAND") ||
			!StrCompare(operation, "OP@BITWOR") ||
			!StrCompare(operation, "OP@BITXOR") ||
			!StrCompare(operation, "OP@BITSHL") ||
			!StrCompare(operation, "OP@BITSHR")))
			return Error("integer-only operator used with floating operands", node);

		Opcode opcode;
		if (!StrCompare(operation, "OP@ARIADD"))
			opcode = arithmetic_type.IsFloating() ? Opcode::FAdd : Opcode::Add;
		else if (!StrCompare(operation, "OP@ARISUB"))
			opcode = arithmetic_type.IsFloating() ? Opcode::FSub : Opcode::Sub;
		else if (!StrCompare(operation, "OP@ARIMUL"))
			opcode = arithmetic_type.IsFloating() ? Opcode::FMul : Opcode::Mul;
		else if (!StrCompare(operation, "OP@ARIDIV"))
			opcode = arithmetic_type.IsFloating() ? Opcode::FDiv :
				arithmetic_type.is_signed ? Opcode::SDiv : Opcode::UDiv;
		else if (!StrCompare(operation, "OP@ARIREM"))
			if (arithmetic_type.IsFloating())
				return Error("remainder requires integer operands", node);
			else opcode = arithmetic_type.is_signed ? Opcode::SRem : Opcode::URem;
		else if (!StrCompare(operation, "OP@BITAND")) opcode = Opcode::And;
		else if (!StrCompare(operation, "OP@BITWOR")) opcode = Opcode::Or;
		else if (!StrCompare(operation, "OP@BITXOR")) opcode = Opcode::Xor;
		else if (!StrCompare(operation, "OP@BITSHL")) opcode = Opcode::Shl;
		else if (!StrCompare(operation, "OP@BITSHR"))
			opcode = arithmetic_type.is_signed ? Opcode::AShr : Opcode::LShr;
		else {
			if (arithmetic_type.IsFloating()) {
				FCmpPredicate predicate;
				if (!StrCompare(operation, "OP@JEQUAL")) predicate = FCmpPredicate::EQ;
				else if (!StrCompare(operation, "OP@JNOTEQ")) predicate = FCmpPredicate::NE;
				else if (!StrCompare(operation, "OP@JBELOW")) predicate = FCmpPredicate::LT;
				else if (!StrCompare(operation, "OP@JBEEQU")) predicate = FCmpPredicate::LE;
				else if (!StrCompare(operation, "OP@JGREAT")) predicate = FCmpPredicate::GT;
				else if (!StrCompare(operation, "OP@JGREQU")) predicate = FCmpPredicate::GE;
				else return Error("integer-only operator used with floating operands", node);
				Instruction instruction;
				instruction.opcode = Opcode::FCmp;
				instruction.type = CloverType(arithmetic_type);
				instruction.has_result = true;
				instruction.result = NewValue(Type::I1);
				instruction.operands.Append(lhs);
				instruction.operands.Append(rhs);
				instruction.float_predicate = predicate;
				CurrentBlock().instructions.Append(instruction);
				return { instruction.result, true, IntType(), false };
			}
			ICmpPredicate predicate;
			if (!StrCompare(operation, "OP@JEQUAL")) predicate = ICmpPredicate::EQ;
			else if (!StrCompare(operation, "OP@JNOTEQ")) predicate = ICmpPredicate::NE;
			else if (!StrCompare(operation, "OP@JBELOW")) predicate =
				arithmetic_type.is_signed ? ICmpPredicate::SLT : ICmpPredicate::ULT;
			else if (!StrCompare(operation, "OP@JBEEQU")) predicate =
				arithmetic_type.is_signed ? ICmpPredicate::SLE : ICmpPredicate::ULE;
			else if (!StrCompare(operation, "OP@JGREAT")) predicate =
				arithmetic_type.is_signed ? ICmpPredicate::SGT : ICmpPredicate::UGT;
			else if (!StrCompare(operation, "OP@JGREQU")) predicate =
				arithmetic_type.is_signed ? ICmpPredicate::SGE : ICmpPredicate::UGE;
			else return Error("unsupported operator", node);

			Instruction instruction;
			instruction.opcode = Opcode::ICmp;
			instruction.type = CloverType(arithmetic_type);
			instruction.has_result = true;
			instruction.result = NewValue(Type::I1);
			instruction.operands.Append(lhs);
			instruction.operands.Append(rhs);
			instruction.predicate = predicate;
			CurrentBlock().instructions.Append(instruction);
			return { instruction.result, true, IntType(), false };
		}

		Value value = EmitResult(opcode, CloverType(arithmetic_type), lhs, rhs);
		return { value, true, arithmetic_type, false };
	}

	LoweredValue FunctionBuilder::LowerExpression(uni::Nnode* node) {
		using namespace uni;
		if (!node) return Error("missing expression", node);
		switch (node->type) {
		case tok_number: {
			stdsint immediate;
			FrontendCType type;
			if (ParseIntegerLiteral(node->addr, *target, immediate, type))
				return { EmitConst(immediate, type), true, type, false };
			if (ParseFloatingLiteral(node->addr, immediate, type))
				return { EmitFloatConst(immediate, type), true, type, false };
			return Error("invalid numeric literal", node);
		}
		case ToToken(FrontendCToken::CharacterLiteral): {
			stdsint immediate;
			if (!ParseCharacterLiteral(node->addr, *target, immediate))
				return Error("invalid character literal", node);
			FrontendCType type = IntType();
			return { EmitConst(immediate, type), true, type, false };
		}
		case tok_string:
			return LowerStringLiteral(node);
		case tok_identy: {
			LocalValue* local = FindLocal(node->addr);
			if (local) return { local->address, true, local->type, true,
				local->alignment };
			const FrontendCEnumerator* constant = FindConstant(node->addr);
			if (constant) {
				FrontendCType type = constant->type;
				return { EmitConst(constant->value, type), true, type, false };
			}
			const FrontendCGlobal* global = FindGlobal(node->addr);
			if (!global) return Error("use of undeclared identifier", node);
			return { EmitGlobalAddress(*global), true, global->type, true,
				global->alignment };
		}
		case tok_func:
			if (node->addr && !StrCompare(node->addr, "sizeof"))
				return LowerSizeof(node->subf);
			if (node->addr && !StrCompare(node->addr, "operator[]"))
				return LowerSubscript(node);
			if (node->addr && StrLength(node->addr) >= 3 &&
				!(node->addr[0] == 'O' && node->addr[1] == 'P' &&
				node->addr[2] == '@')) return LowerCall(node);
			if (node->addr && StrLength(node->addr) < 3) return LowerCall(node);
			return LowerOperator(node);
		default:
			return Error("unsupported syntax node", node);
		}
	}

}
