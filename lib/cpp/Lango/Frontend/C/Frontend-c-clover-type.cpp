// ASCII C++11 TAB4 LF
// Docutitle: Lango C Frontend Clover Type Lowering
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

	Type CloverTypeFor(const FrontendCType& type,
		const FrontendCTargetInfo& target) {
		if (type.IsVoid()) return Type::Void;
		if (type.IsPointer()) return Type::Ptr;
		unsigned bits = target.int_bits;
		switch (type.integer_rank) {
		case FrontendCIntegerRank::Char: bits = target.char_bits; break;
		case FrontendCIntegerRank::Short: bits = target.short_bits; break;
		case FrontendCIntegerRank::Int: bits = target.int_bits; break;
		case FrontendCIntegerRank::Long: bits = target.long_bits; break;
		case FrontendCIntegerRank::LongLong: bits = target.long_long_bits; break;
		}
		return bits == 8 ? Type::I8 : bits == 16 ? Type::I16 :
			bits == 64 ? Type::I64 : Type::I32;
	}

	unsigned FunctionBuilder::IntegerBits(const FrontendCType& type) const {
		switch (type.integer_rank) {
		case FrontendCIntegerRank::Char: return target->char_bits;
		case FrontendCIntegerRank::Short: return target->short_bits;
		case FrontendCIntegerRank::Int: return target->int_bits;
		case FrontendCIntegerRank::Long: return target->long_bits;
		case FrontendCIntegerRank::LongLong: return target->long_long_bits;
		}
		return 0;
	}

	Type FunctionBuilder::CloverType(const FrontendCType& type) const {
		return CloverTypeFor(type, *target);
	}

	stduint FunctionBuilder::TypeAlignment(const FrontendCType& type) const {
		if (type.IsVoid()) return 0;
		stduint bits = type_info->AlignBits(type, *target);
		return bits && !(bits % target->char_bits) ?
			bits / target->char_bits : 0;
	}

	stduint FunctionBuilder::TypeSize(const FrontendCType& type) const {
		stduint bits = type_info->SizeBits(type, *target);
		return bits && !(bits % target->char_bits) ?
			bits / target->char_bits : 0;
	}

	FrontendCType FunctionBuilder::IntType() const {
		return FrontendCType();
	}

	FrontendCType FunctionBuilder::SizeType() const {
		FrontendCType type;
		type.is_signed = false;
		type.integer_rank = target->long_bits == target->pointer_bits ?
			FrontendCIntegerRank::Long : FrontendCIntegerRank::LongLong;
		return type;
	}

	FrontendCType FunctionBuilder::PointerIntegerType(bool is_signed) const {
		FrontendCType type;
		type.is_signed = is_signed;
		if (target->int_bits == target->pointer_bits)
			type.integer_rank = FrontendCIntegerRank::Int;
		else if (target->long_bits == target->pointer_bits)
			type.integer_rank = FrontendCIntegerRank::Long;
		else type.integer_rank = FrontendCIntegerRank::LongLong;
		return type;
	}

	Value FunctionBuilder::EmitConversion(Opcode opcode, Type destination, Value operand) {
		Instruction instruction;
		instruction.opcode = opcode;
		instruction.type = destination;
		instruction.has_result = true;
		instruction.result = NewValue(destination);
		instruction.operands.Append(operand);
		CurrentBlock().instructions.Append(instruction);
		return instruction.result;
	}

	LoweredValue FunctionBuilder::ConvertInteger(LoweredValue lowered,
		const FrontendCType& destination, const uni::Nnode* node) {
		lowered = ToRValue(lowered, node);
		if (!lowered.valid || !lowered.type.IsInteger() || !destination.IsInteger())
			return Error("integer conversion requires integer operands", node);
		if (lowered.value.type == Type::I1 && CloverType(lowered.type) != Type::I1)
			lowered.value = EmitConversion(Opcode::ZExt,
				CloverType(lowered.type), lowered.value);
		unsigned source_bits = IntegerBits(lowered.type);
		unsigned destination_bits = IntegerBits(destination);
		if (source_bits < destination_bits)
			lowered.value = EmitConversion(lowered.type.is_signed ? Opcode::SExt :
				Opcode::ZExt, CloverType(destination), lowered.value);
		else if (source_bits > destination_bits)
			lowered.value = EmitConversion(Opcode::Trunc,
				CloverType(destination), lowered.value);
		lowered.type = destination;
		lowered.is_lvalue = false;
		return lowered;
	}

	FrontendCType FunctionBuilder::PromoteType(FrontendCType type) const {
		if (type.IsInteger() &&
			type.integer_rank < FrontendCIntegerRank::Int) return IntType();
		return type;
	}

	FrontendCType FunctionBuilder::CommonIntegerType(FrontendCType left,
		FrontendCType right) const {
		left = PromoteType(left);
		right = PromoteType(right);
		if (left.integer_rank == right.integer_rank) {
			if (!left.is_signed || !right.is_signed) left.is_signed = false;
			return left;
		}
		FrontendCType higher = left.integer_rank > right.integer_rank ? left : right;
		FrontendCType lower = left.integer_rank > right.integer_rank ? right : left;
		if (higher.is_signed == lower.is_signed || !higher.is_signed) return higher;
		if (IntegerBits(higher) > IntegerBits(lower)) return higher;
		higher.is_signed = false;
		return higher;
	}

	bool FunctionBuilder::CommonConditionalType(const FrontendCType& true_type,
		const FrontendCType& false_type, FrontendCType& result) const {
		if (true_type.IsInteger() && false_type.IsInteger()) {
			result = CommonIntegerType(true_type, false_type);
			return true;
		}
		if (true_type.IsPointer() && false_type.IsPointer()) {
			if (true_type.CanAssignFrom(false_type)) result = true_type;
			else if (false_type.CanAssignFrom(true_type)) result = false_type;
			else return false;
			return true;
		}
		return false;
	}

	bool FunctionBuilder::ResolveRecordMemberType(const FrontendCType& record_type,
		uni::Nnode* designator, FrontendCType& type) {
		if (!designator) return false;
		if (designator->type == tok_func && designator->addr &&
			!StrCompare(designator->addr, "operator[]")) {
			uni::Nnode* base = designator->subf;
			uni::Nnode* index = base ? base->next : nullptr;
			FrontendCType index_type;
			if (!base || !index || index->next ||
				!ResolveRecordMemberType(record_type, base, type) ||
				!ResolveExpressionType(index, index_type)) return false;
			if (type.IsArray()) type = type.DecayType();
			if (!type.IsPointer() || !index_type.IsInteger()) return false;
			type = type.DereferenceType();
			return true;
		}
		if (designator->type != tok_identy || !designator->addr) return false;
		const FrontendCRecordMember* member =
			FindRecordMember(record_type, designator->addr);
		if (!member) return false;
		type = MemberType(record_type, *member);
		return true;
	}

	bool FunctionBuilder::ResolveExpressionType(uni::Nnode* node, FrontendCType& type) {
		if (!node) return false;
		if (node->type == tok_number) {
			stdsint value;
			return ParseIntegerLiteral(node->addr, *target, value, type);
		}
		if (node->type == ToToken(FrontendCToken::CharacterLiteral)) {
			stdsint value;
			if (!ParseCharacterLiteral(node->addr, *target, value)) return false;
			type = IntType();
			return true;
		}
		if (node->type == tok_string) {
			uni::Vector<stdsint> bytes;
			if (!DecodeCString(node->addr, bytes)) return false;
			type = FrontendCType();
			type.integer_rank = FrontendCIntegerRank::Char;
			type.is_signed = target->char_is_signed;
			return type.AppendArrayDimension(bytes.Count() + 1);
		}
		if (node->type == tok_identy) {
			LocalValue* local = FindLocal(node->addr);
			if (local) type = local->type;
			else {
				const FrontendCGlobal* global = FindGlobal(node->addr);
				if (!global) return false;
				type = global->type;
			}
			return true;
		}
		if (node->type != tok_func) return false;
		if (!node->addr) return ResolveExpressionType(node->subf, type);
		if (!StrCompare(node->addr, "sizeof") ||
			!StrCompare(node->addr, "OP@SIZEOF")) {
			type = SizeType();
			return true;
		}
		if (!StrCompare(node->addr, "operator[]")) {
			uni::Nnode* left = node->subf;
			uni::Nnode* right = left ? left->next : nullptr;
			FrontendCType left_type, right_type;
			if (!left || !right || right->next ||
				!ResolveExpressionType(left, left_type) ||
				!ResolveExpressionType(right, right_type)) return false;
			if (left_type.IsArray()) left_type = left_type.DecayType();
			if (right_type.IsArray()) right_type = right_type.DecayType();
			if (left_type.IsInteger() && right_type.IsPointer()) left_type = right_type;
			if (!left_type.IsPointer()) return false;
			type = left_type.DereferenceType();
			return true;
		}
		if (StrLength(node->addr) < 3 || node->addr[0] != 'O' ||
			node->addr[1] != 'P' || node->addr[2] != '@') {
			const FrontendCFunction* signature = FindFunction(node->addr);
			if (!signature) return false;
			type = signature->return_type;
			return true;
		}
		if (!StrCompare(node->addr, "OP@CAST")) {
			const FrontendCType* cast_type = type_info->Find(node);
			if (!cast_type || !node->subf || node->subf->next) return false;
			type = *cast_type;
			return true;
		}
		uni::Nnode* left = node->subf;
		uni::Nnode* right = left ? left->next : nullptr;
		FrontendCType left_type;
		if (!ResolveExpressionType(left, left_type)) return false;
		if (!StrCompare(node->addr, "OP@SUFMEMB")) {
			return right && !right->next && left_type.IsRecord() &&
				ResolveRecordMemberType(left_type, right, type);
		}
		if (!StrCompare(node->addr, "OP@SUFMEMP")) {
			if (!right || right->next || !left_type.IsPointer()) return false;
			FrontendCType record_type = left_type.DereferenceType();
			if (!record_type.IsRecord()) return false;
			return ResolveRecordMemberType(record_type, right, type);
		}
		if (!StrCompare(node->addr, "OP@PREADDR")) {
			type = left_type.AddressType();
			return true;
		}
		if (!StrCompare(node->addr, "OP@PREMEMB")) {
			if (!left_type.IsPointer()) return false;
			type = left_type.DereferenceType();
			return true;
		}
		if (!StrCompare(node->addr, "OP@JEQUAL") ||
			!StrCompare(node->addr, "OP@JNOTEQ") ||
			!StrCompare(node->addr, "OP@JBELOW") ||
			!StrCompare(node->addr, "OP@JBEEQU") ||
			!StrCompare(node->addr, "OP@JGREAT") ||
			!StrCompare(node->addr, "OP@JGREQU")) {
			type = IntType();
			return true;
		}
		if (!StrCompare(node->addr, "OP@LOGAND") ||
			!StrCompare(node->addr, "OP@LOGIOR")) {
			type = IntType();
			return true;
		}
		if (!StrCompare(node->addr, "OP@TERNAR")) {
			uni::Nnode* false_node = right ? right->next : nullptr;
			FrontendCType true_type, false_type;
			return right && false_node && !false_node->next &&
				ResolveExpressionType(right, true_type) &&
				ResolveExpressionType(false_node, false_type) &&
				CommonConditionalType(true_type, false_type, type);
		}
		if (!right) {
			if (!StrCompare(node->addr, "OP@SUFADD") ||
				!StrCompare(node->addr, "OP@SUFSUB") ||
				!StrCompare(node->addr, "OP@PREADD") ||
				!StrCompare(node->addr, "OP@PRESUB")) type = left_type;
			else if (!StrCompare(node->addr, "OP@LOGNOT")) type = IntType();
			else type = PromoteType(left_type);
			return true;
		}
		FrontendCType right_type;
		if (!ResolveExpressionType(right, right_type)) return false;
		if (!StrCompare(node->addr, "OP@ASSIGN") ||
			!StrCompare(node->addr, "OP@AGNSUM") ||
			!StrCompare(node->addr, "OP@AGNDIF") ||
			!StrCompare(node->addr, "OP@AGNPRO") ||
			!StrCompare(node->addr, "OP@AGNQUO") ||
			!StrCompare(node->addr, "OP@AGNREM") ||
			!StrCompare(node->addr, "OP@AGNSHL") ||
			!StrCompare(node->addr, "OP@AGNSHR") ||
			!StrCompare(node->addr, "OP@AGNAND") ||
			!StrCompare(node->addr, "OP@AGNXOR") ||
			!StrCompare(node->addr, "OP@ASGNOR")) {
			type = left_type;
			return true;
		}
		if (!StrCompare(node->addr, "OP@COMMA")) {
			for (uni::Nnode* operand = right->next; operand; operand = operand->next)
				if (!ResolveExpressionType(operand, right_type)) return false;
			type = right_type;
			return true;
		}
		if ((!StrCompare(node->addr, "OP@ARIADD") ||
			!StrCompare(node->addr, "OP@ARISUB")) && left_type.IsPointer()) {
			type = left_type;
			return true;
		}
		if (!StrCompare(node->addr, "OP@ARIADD") && right_type.IsPointer()) {
			type = right_type;
			return true;
		}
		if (!left_type.IsInteger() || !right_type.IsInteger()) return false;
		type = CommonIntegerType(left_type, right_type);
		return true;
	}

	stdsint FunctionBuilder::SizeOf(const FrontendCType& type) const {
			stduint bits = type_info->SizeBits(type, *target);
			return bits ? (stdsint)(bits / target->char_bits) : 0;
	}

	LoweredValue FunctionBuilder::LowerSizeof(uni::Nnode* operand) {
		FrontendCType operand_type;
		if (!frontend_c_detail::ParseTypeName(operand, operand_type,
			*target, *type_info) &&
			!ResolveExpressionType(operand, operand_type))
			return Error("invalid sizeof operand", operand);
		if (operand_type.IsVoid()) return Error("sizeof cannot be applied to void", operand);
		FrontendCType result_type = SizeType();
		return { EmitConst(SizeOf(operand_type), result_type), true,
			result_type, false };
	}

	LoweredValue FunctionBuilder::LowerCast(uni::Nnode* node, uni::Nnode* operand) {
		const FrontendCType* destination = type_info->Find(node);
		if (!destination || !operand || operand->next)
			return Error("invalid cast expression", node);
		LoweredValue lowered = LowerExpression(operand);
		if (!lowered.valid) return lowered;
		if (destination->IsVoid()) {
			lowered.type = *destination;
			lowered.is_lvalue = false;
			return lowered;
		}
		lowered = ToRValue(lowered, operand);
		if (!lowered.valid) return lowered;
		if (destination->IsInteger() && lowered.type.IsInteger())
			return ConvertInteger(lowered, *destination, operand);
		if (destination->IsPointer() && lowered.type.IsPointer()) {
			lowered.type = *destination;
			return lowered;
		}
		if (destination->IsPointer() && lowered.type.IsInteger()) {
			FrontendCType pointer_integer =
				PointerIntegerType(lowered.type.is_signed);
			lowered = ConvertInteger(lowered, pointer_integer, operand);
			if (!lowered.valid) return lowered;
			Value value = EmitConversion(Opcode::IntToPtr,
				Type::Ptr, lowered.value);
			return { value, true, *destination, false, 1 };
		}
		if (destination->IsInteger() && lowered.type.IsPointer()) {
			Value value = EmitConversion(Opcode::PtrToInt,
				CloverType(*destination), lowered.value);
			return { value, true, *destination, false };
		}
		return Error("cast requires scalar operands", node);
	}

}
