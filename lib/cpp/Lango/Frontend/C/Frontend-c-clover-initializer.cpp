// ASCII C++11 TAB4 LF
// Docutitle: Lango C Frontend Clover Initializer Lowering
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
	namespace {
		bool IsCommaNode(const uni::Nnode* node) {
			return node && node->type == tok_func && node->addr &&
				!StrCompare(node->addr, "OP@COMMA");
		}

		void AppendInitializerItems(uni::Nnode* node,
			uni::Vector<uni::Nnode*>& items) {
			if (!node) return;
			if (IsCommaNode(node)) {
				for (uni::Nnode* child = node->subf; child; child = child->next)
					AppendInitializerItems(child, items);
				return;
			}
			items.Append(node);
		}

		void AppendDirectInitializerItems(uni::Nnode* node,
			uni::Vector<uni::Nnode*>& items) {
			if (node && node->type == tok_block) node = node->subf;
			AppendInitializerItems(node, items);
		}

		bool IsAggregateType(const FrontendCType& type) {
			return type.IsArray() || type.IsRecord();
		}

		FrontendCType ArrayElementType(const FrontendCType& type) {
			FrontendCType element = type;
			if (!element.array_dimension_count) return element;
			for (stduint index = 1; index < element.array_dimension_count; index++)
				element.array_dimensions[index - 1] = element.array_dimensions[index];
			element.array_dimensions[--element.array_dimension_count] = 0;
			return element;
		}

		class StaticInitializerBuilder {
		private:
			Global& global;
			const FrontendCTargetInfo& target;
			const FrontendCTypeInfo& type_info;
			const uni::Vector<FrontendCEnumerator>& constants;
			const FrontendCDiagnostics& diagnostics;

			bool Error(const char* message, const uni::Nnode* node,
				DiagnosticStage stage = DiagnosticStage::Semantic) const {
				diagnostics.Report(stage, message, node);
				return false;
			}

			stduint TypeSize(const FrontendCType& type) const {
				stduint bits = type_info.SizeBits(type, target);
				return bits && !(bits % target.char_bits) ?
					bits / target.char_bits : 0;
			}

			bool AppendScalar(const FrontendCType& type,
				uni::Nnode* initializer, stduint offset) {
				stdsint low = 0;
				stdsint high = 0;
				if (!ParseStaticScalarInitializer(initializer, target,
					constants, type, low, high))
					return Error("static initializer must contain constant expressions",
						initializer, DiagnosticStage::Lowering);
				if (type.integer_rank == FrontendCIntegerRank::Bool &&
					!type.enum_id) low = low != 0;
				GlobalInitializerFragment fragment;
				fragment.offset = offset;
				fragment.type = CloverTypeFor(type, target);
				fragment.initializer = low;
				fragment.initializer_high = high;
				global.initializer_fragments.Append(fragment);
				return true;
			}

			bool InitializeScalar(const FrontendCType& type,
				uni::Nnode* initializer, stduint offset) {
				if (initializer && initializer->type == tok_block) {
					uni::Vector<uni::Nnode*> items;
					AppendDirectInitializerItems(initializer, items);
					if (!items.Count()) return true;
					if (items.Count() != 1)
						return Error("excess elements in scalar initializer", items[1]);
					return InitializeScalar(type, items[0], offset);
				}
				return AppendScalar(type, initializer, offset);
			}

			bool InitializeStringArray(const FrontendCType& type,
				uni::Nnode* initializer, stduint offset) {
				uni::Vector<stdsint> values;
				if (!BuildStringArrayValues(type, initializer, values))
					return Error("invalid static character array string initializer",
						initializer);
				FrontendCType element_type = ArrayElementType(type);
				stduint element_size = TypeSize(element_type);
				for (stduint index = 0; index < values.Count(); index++) {
					GlobalInitializerFragment fragment;
					fragment.offset = offset + index * element_size;
					fragment.type = CloverTypeFor(element_type, target);
					fragment.initializer = values[index];
					global.initializer_fragments.Append(fragment);
				}
				return true;
			}

			bool InitializeAggregateFromItems(const FrontendCType& type,
				uni::Vector<uni::Nnode*>& items, stduint& cursor,
				stduint offset, const uni::Nnode* source) {
				if (type.IsArray()) {
					FrontendCType element_type = ArrayElementType(type);
					stduint element_size = TypeSize(element_type);
					if (!element_size) return Error("invalid array element size", source);
					for (stduint index = 0; index < type.array_dimensions[0]; index++)
						if (!InitializeObjectFromItems(element_type, items, cursor,
							offset + index * element_size, source)) return false;
					return true;
				}
				const FrontendCRecord* record = type_info.GetRecord(type.record_id);
				if (!record || !record->complete || !record->members.Count())
					return Error("invalid record initializer type", source);
				stduint member_count = record->is_union ? 1 : record->members.Count();
				for (stduint index = 0; index < member_count; index++) {
					const FrontendCRecordMember& member = record->members.begin()[index];
					if (member.offset_bits % target.char_bits)
						return Error("record member is not byte-addressable", source);
					if (!InitializeObjectFromItems(member.type, items, cursor,
						offset + member.offset_bits / target.char_bits,
						source)) return false;
				}
				return true;
			}

			bool InitializeObjectFromItems(const FrontendCType& type,
				uni::Vector<uni::Nnode*>& items, stduint& cursor,
				stduint offset, const uni::Nnode* source) {
				if (cursor >= items.Count()) return true;
				uni::Nnode* item = items[cursor];
				if (type.IsArray() && FindStringArrayInitializer(item)) {
					cursor++;
					return InitializeStringArray(type, item, offset);
				}
				if (!IsAggregateType(type)) {
					cursor++;
					return InitializeScalar(type, item, offset);
				}
				if (item->type == tok_block) {
					cursor++;
					return InitializeObject(type, item, offset);
				}
				return InitializeAggregateFromItems(type, items, cursor,
					offset, source);
			}

			bool InitializeObject(const FrontendCType& type,
				uni::Nnode* initializer, stduint offset) {
				if (type.IsArray() && FindStringArrayInitializer(initializer))
					return InitializeStringArray(type, initializer, offset);
				if (!IsAggregateType(type))
					return InitializeScalar(type, initializer, offset);
				if (!initializer || initializer->type != tok_block)
					return Error("static aggregate initializer requires braces", initializer);
				uni::Vector<uni::Nnode*> items;
				AppendDirectInitializerItems(initializer, items);
				stduint cursor = 0;
				if (!InitializeAggregateFromItems(type, items, cursor,
					offset, initializer)) return false;
				if (cursor == items.Count()) return true;
				if (type.IsArray())
					return Error("excess elements in array initializer", items[cursor]);
				const FrontendCRecord* record = type_info.GetRecord(type.record_id);
				return Error(record && record->is_union ?
					"excess elements in union initializer" :
					"excess elements in struct initializer", items[cursor]);
			}

		public:
			StaticInitializerBuilder(Global& output_global,
				const FrontendCTargetInfo& target_info,
				const FrontendCTypeInfo& semantic_types,
				const uni::Vector<FrontendCEnumerator>& visible_constants,
				const FrontendCDiagnostics& diagnostic_output) :
				global(output_global), target(target_info), type_info(semantic_types),
				constants(visible_constants), diagnostics(diagnostic_output) {}

			bool Build(const FrontendCType& type, uni::Nnode* initializer) {
				return InitializeObject(type, initializer, 0);
			}
		};
	}

	bool BuildStaticObjectInitializer(Global& global,
		const FrontendCType& type, uni::Nnode* initializer,
		const FrontendCTargetInfo& target,
		const FrontendCTypeInfo& type_info,
		const uni::Vector<FrontendCEnumerator>& constants,
		const FrontendCDiagnostics& diagnostics) {
		StaticInitializerBuilder builder(global, target, type_info,
			constants, diagnostics);
		return builder.Build(type, initializer);
	}

	bool FunctionBuilder::InitializeScalar(Value address,
		const FrontendCType& type, stduint alignment,
		uni::Nnode* initializer) {
		if (initializer && initializer->type == tok_block) {
			uni::Vector<uni::Nnode*> items;
			AppendDirectInitializerItems(initializer, items);
			if (!items.Count())
				return ZeroInitializeObject(address, type, alignment, initializer);
			if (items.Count() != 1)
				return Error("excess elements in scalar initializer", items[1]).valid;
			return InitializeScalar(address, type, alignment, items[0]);
		}
		LoweredValue lowered = ToRValue(
			LowerExpression(initializer), initializer);
		if (!lowered.valid) return false;
		if (type.IsArithmetic())
			lowered = ConvertArithmetic(lowered, type, initializer);
		else if (!type.CanAssignFrom(lowered.type))
			return Error("incompatible initializer type", initializer).valid;
		if (!lowered.valid) return false;
		EmitStore(lowered.value, address, alignment);
		return true;
	}

	bool FunctionBuilder::InitializeRecordCopy(Value address,
		const FrontendCType& type, stduint alignment,
		uni::Nnode* initializer) {
		LoweredValue source = LowerExpression(initializer);
		if (!source.valid) return false;
		if (!source.is_lvalue || !source.type.IsRecord() ||
			!type.HasSameShape(source.type))
			return Error("incompatible record initializer", initializer).valid;
		stduint size = TypeSize(type);
		if (!size) return Error("invalid record initializer size", initializer).valid;
		Instruction instruction;
		instruction.opcode = Opcode::MemCopy;
		instruction.type = Type::Void;
		instruction.operands.Append(address);
		instruction.operands.Append(source.value);
		instruction.immediate = (stdsint)size;
		instruction.alignment = alignment;
		instruction.source_alignment = source.alignment;
		AttachSource(instruction, initializer);
		CurrentBlock().instructions.Append(instruction);
		return true;
	}

	bool FunctionBuilder::ZeroInitializeObject(Value address,
		const FrontendCType& type, stduint alignment,
		const uni::Nnode* source) {
		if (type.IsArray()) {
			FrontendCType element_type = ArrayElementType(type);
			stduint element_size = TypeSize(element_type);
			if (!element_size)
				return Error("invalid array element size", source).valid;
			for (stduint index = 0; index < type.array_dimensions[0]; index++) {
				stduint offset = index * element_size;
				if (!ZeroInitializeObject(EmitByteOffset(address, offset), element_type,
					OffsetAlignment(alignment, offset), source)) return false;
			}
			return true;
		}
		if (type.IsRecord()) {
			const FrontendCRecord* record = type_info->GetRecord(type.record_id);
			if (!record || !record->complete || !record->members.Count())
				return Error("invalid record initializer type", source).valid;
			stduint member_count = record->is_union ? 1 : record->members.Count();
			for (stduint index = 0; index < member_count; index++) {
				const FrontendCRecordMember& member = record->members.begin()[index];
				if (member.offset_bits % target->char_bits)
					return Error("record member is not byte-addressable", source).valid;
				stduint offset = member.offset_bits / target->char_bits;
				if (!ZeroInitializeObject(EmitByteOffset(address, offset), member.type,
					OffsetAlignment(alignment, offset), source)) return false;
			}
			return true;
		}
		Value value;
		if (type.IsFloating()) value = EmitFloatConst(0, type);
		else if (type.IsPointer()) {
			FrontendCType integer_type = PointerIntegerType(false);
			value = EmitConversion(Opcode::IntToPtr, Type::Ptr,
				EmitConst(0, integer_type));
		}
		else value = EmitConst(0, type);
		EmitStore(value, address, alignment);
		return true;
	}

	bool FunctionBuilder::InitializeAggregateFromItems(Value address,
		const FrontendCType& type, stduint alignment,
		uni::Vector<uni::Nnode*>& items, stduint& cursor,
		const uni::Nnode* source) {
		if (type.IsArray()) {
			FrontendCType element_type = ArrayElementType(type);
			stduint element_size = TypeSize(element_type);
			if (!element_size)
				return Error("invalid array element size", source).valid;
			for (stduint index = 0; index < type.array_dimensions[0]; index++) {
				stduint offset = index * element_size;
				if (!InitializeObjectFromItems(EmitByteOffset(address, offset),
					element_type, OffsetAlignment(alignment, offset),
					items, cursor, source)) return false;
			}
			return true;
		}
		const FrontendCRecord* record = type_info->GetRecord(type.record_id);
		if (!record || !record->complete || !record->members.Count())
			return Error("invalid record initializer type", source).valid;
		stduint member_count = record->is_union ? 1 : record->members.Count();
		for (stduint index = 0; index < member_count; index++) {
			const FrontendCRecordMember& member = record->members.begin()[index];
			if (member.offset_bits % target->char_bits)
				return Error("record member is not byte-addressable", source).valid;
			stduint offset = member.offset_bits / target->char_bits;
			if (!InitializeObjectFromItems(EmitByteOffset(address, offset),
				member.type, OffsetAlignment(alignment, offset),
				items, cursor, source)) return false;
		}
		return true;
	}

	bool FunctionBuilder::InitializeObjectFromItems(Value address,
		const FrontendCType& type, stduint alignment,
		uni::Vector<uni::Nnode*>& items, stduint& cursor,
		const uni::Nnode* source) {
		if (cursor >= items.Count())
			return ZeroInitializeObject(address, type, alignment, source);
		uni::Nnode* item = items[cursor];
		if (type.IsArray() && FindStringArrayInitializer(item)) {
			cursor++;
			return InitializeObject(address, type, alignment, item);
		}
		if (!IsAggregateType(type)) {
			cursor++;
			return InitializeScalar(address, type, alignment, item);
		}
		if (item->type == tok_block) {
			cursor++;
			return InitializeObject(address, type, alignment, item);
		}
		if (type.IsRecord()) {
			FrontendCType source_type;
			if (ResolveExpressionType(item, source_type) &&
				source_type.IsRecord() && type.HasSameShape(source_type)) {
				cursor++;
				return InitializeRecordCopy(address, type, alignment, item);
			}
		}
		return InitializeAggregateFromItems(address, type, alignment,
			items, cursor, source);
	}

	bool FunctionBuilder::InitializeObject(Value address,
		const FrontendCType& type, stduint alignment,
		uni::Nnode* initializer) {
		if (type.IsArray() && FindStringArrayInitializer(initializer)) {
			uni::Vector<stdsint> values;
			if (!BuildStringArrayValues(type, initializer, values))
				return Error("invalid character array string initializer", initializer).valid;
			FrontendCType element_type = ArrayElementType(type);
			stduint element_size = TypeSize(element_type);
			for (stduint index = 0; index < values.Count(); index++) {
				stduint offset = index * element_size;
				EmitStore(EmitConst(values[index], element_type),
					EmitByteOffset(address, offset), OffsetAlignment(alignment, offset));
			}
			return true;
		}
		if (!IsAggregateType(type))
			return InitializeScalar(address, type, alignment, initializer);
		if (!initializer || initializer->type != tok_block) {
			if (type.IsRecord()) {
				FrontendCType source_type;
				if (ResolveExpressionType(initializer, source_type) &&
					source_type.IsRecord() && type.HasSameShape(source_type))
					return InitializeRecordCopy(address, type, alignment, initializer);
			}
			return Error("aggregate initializer requires braces", initializer).valid;
		}
		uni::Vector<uni::Nnode*> items;
		AppendDirectInitializerItems(initializer, items);
		stduint cursor = 0;
		if (!InitializeAggregateFromItems(address, type, alignment,
			items, cursor, initializer)) return false;
		if (cursor == items.Count()) return true;
		if (type.IsArray())
			return Error("excess elements in array initializer", items[cursor]).valid;
		const FrontendCRecord* record = type_info->GetRecord(type.record_id);
		return Error(record && record->is_union ?
			"excess elements in union initializer" :
			"excess elements in struct initializer", items[cursor]).valid;
	}
}
