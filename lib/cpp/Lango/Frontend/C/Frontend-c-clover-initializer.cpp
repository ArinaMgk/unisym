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

		bool IsDesignatedInitializer(const uni::Nnode* node) {
			return node && node->type ==
				ToToken(FrontendCToken::DesignatedInitializer);
		}

		uni::Nnode* DesignatedInitializerValue(uni::Nnode* node) {
			for (uni::Nnode* child = node ? node->subf : nullptr;
				child; child = child->next)
				if (child->type == ToToken(FrontendCToken::InitializerValue))
					return child->subf;
			return nullptr;
		}

		uni::Nnode* UnwrapInitializerExpression(uni::Nnode* node) {
			if (node && node->next) node = node->Tail();
			while (node && !node->next && !node->addr && node->subf)
				node = node->subf;
			return node;
		}

		bool HasDesignatedInitializer(const uni::Vector<uni::Nnode*>& items) {
			for (uni::Nnode* item : items)
				if (IsDesignatedInitializer(item)) return true;
			return false;
		}

		FrontendCType ArrayElementType(const FrontendCType& type) {
			return type.IsArray() ? type.RemoveOuterLayer() : type;
		}

		struct InitializerTarget {
			FrontendCType type;
			stduint offset = 0;
		};

		struct InitializerCursorLevel {
			FrontendCType aggregate;
			stduint offset = 0;
			stduint child = 0;
		};

		class InitializerCursor {
		private:
			FrontendCType root_type;
			const FrontendCTargetInfo& target;
			const FrontendCTypeInfo& type_info;
			const FrontendCDiagnostics& diagnostics;
			uni::Vector<InitializerCursorLevel> levels;

			bool Error(const char* message, const uni::Nnode* node) const {
				diagnostics.Report(DiagnosticStage::Semantic, message, node);
				return false;
			}

			stduint TypeSize(const FrontendCType& type) const {
				stduint bits = type_info.SizeBits(type, target);
				return bits && !(bits % target.char_bits) ?
					bits / target.char_bits : 0;
			}

			stduint ChildCount(const FrontendCType& type) const {
				if (type.IsArray()) return type.ArrayDimension(0);
				if (!type.IsRecord()) return 0;
				const FrontendCRecord* record = type_info.GetRecord(type.record_id);
				return record && record->complete ? record->members.Count() : 0;
			}

			bool TargetAt(const InitializerCursorLevel& level,
				InitializerTarget& result) const {
				if (level.aggregate.IsArray()) {
					if (level.child >= level.aggregate.ArrayDimension(0)) return false;
					result.type = ArrayElementType(level.aggregate);
					stduint size = TypeSize(result.type);
					if (!size) return false;
					result.offset = level.offset + level.child * size;
					return true;
				}
				const FrontendCRecord* record =
					type_info.GetRecord(level.aggregate.record_id);
				if (!record || !record->complete ||
					level.child >= record->members.Count()) return false;
				const FrontendCRecordMember& member =
					record->members.begin()[level.child];
				if (member.offset_bits % target.char_bits) return false;
				result.type = member.type;
				result.offset = level.offset +
					member.offset_bits / target.char_bits;
				return true;
			}

			bool SelectChild(const FrontendCType& aggregate,
				uni::Nnode* designator, stduint& child) const {
				if (aggregate.IsArray()) {
					if (!designator || designator->type !=
						ToToken(FrontendCToken::IndexDesignator))
						return Error("array initializer requires an index designator",
							designator);
					stdsint value;
					if (!frontend_c_detail::EvaluateIntegerConstant(
						UnwrapInitializerExpression(designator->subf),
						target, type_info, value))
						return Error("array designator index must be an integer constant expression",
							designator);
					if (value < 0)
						return Error("array designator index cannot be negative", designator);
					if ((stduint)value >= aggregate.ArrayDimension(0))
						return Error("array designator index is out of bounds", designator);
					child = (stduint)value;
					return true;
				}
				if (!aggregate.IsRecord())
					return Error("initializer designator requires an aggregate object",
						designator);
				if (!designator || designator->type !=
					ToToken(FrontendCToken::MemberDesignator))
					return Error("record initializer requires a member designator",
						designator);
				const FrontendCRecord* record = type_info.GetRecord(aggregate.record_id);
				if (!record || !record->complete)
					return Error("member designator requires a complete record type",
						designator);
				for (stduint index = 0; index < record->members.Count(); index++)
					if (!StrCompare(record->members.begin()[index].name.reference(),
						designator->addr)) {
						child = index;
						return true;
					}
				return Error("record has no such designated member", designator);
			}

		public:
			InitializerCursor(const FrontendCType& type,
				const FrontendCTargetInfo& target_info,
				const FrontendCTypeInfo& semantic_types,
				const FrontendCDiagnostics& diagnostic_output) :
				root_type(type), target(target_info), type_info(semantic_types),
				diagnostics(diagnostic_output) {}

			bool Reset() {
				levels.Clear();
				if (!ChildCount(root_type)) return false;
				InitializerCursorLevel level;
				level.aggregate = root_type;
				return levels.Append(level);
			}

			bool Current(InitializerTarget& result) const {
				return levels.Count() &&
					TargetAt(levels.begin()[levels.Count() - 1], result);
			}

			bool OuterIndex(stduint& index) const {
				if (!levels.Count() || !levels.begin()[0].aggregate.IsArray())
					return false;
				index = levels.begin()[0].child;
				return true;
			}

			bool Descend() {
				InitializerTarget current;
				if (!Current(current) || !IsAggregateType(current.type) ||
					!ChildCount(current.type)) return false;
				InitializerCursorLevel level;
				level.aggregate = current.type;
				level.offset = current.offset;
				return levels.Append(level);
			}

			bool Advance() {
				while (levels.Count()) {
					stduint index = levels.Count() - 1;
					InitializerCursorLevel& level = levels[index];
					const FrontendCRecord* record = level.aggregate.IsRecord() ?
						type_info.GetRecord(level.aggregate.record_id) : nullptr;
					if (!record || !record->is_union) {
						level.child++;
						if (level.child < ChildCount(level.aggregate)) return true;
					}
					levels.Remove(index);
				}
				return false;
			}

			bool Select(uni::Nnode* initializer) {
				levels.Clear();
				FrontendCType aggregate = root_type;
				stduint offset = 0;
				bool found = false;
				for (uni::Nnode* designator = initializer ? initializer->subf : nullptr;
					designator && designator->type !=
						ToToken(FrontendCToken::InitializerValue);
					designator = designator->next) {
					stduint child;
					if (!SelectChild(aggregate, designator, child)) return false;
					InitializerCursorLevel level;
					level.aggregate = aggregate;
					level.offset = offset;
					level.child = child;
					if (!levels.Append(level)) return false;
					InitializerTarget selected;
					if (!Current(selected)) return false;
					aggregate = selected.type;
					offset = selected.offset;
					found = true;
				}
				return found;
			}
		};

		class StaticInitializerBuilder {
		private:
			Global& global;
			const FrontendCTargetInfo& target;
			const FrontendCTypeInfo& type_info;
			const uni::Vector<FrontendCEnumerator>& constants;
			const FrontendCDiagnostics& diagnostics;
			const StaticInitializerContext& context;

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

			bool AppendFragment(const GlobalInitializerFragment& fragment) {
				for (stduint index = 0;
					index < global.initializer_fragments.Count(); index++) {
					GlobalInitializerFragment& previous =
						global.initializer_fragments[index];
					if (previous.offset != fragment.offset ||
						previous.type != fragment.type) continue;
					previous = fragment;
					return true;
				}
				return global.initializer_fragments.Append(fragment);
			}

			stduint FragmentSize(const GlobalInitializerFragment& fragment) const {
				switch (fragment.type) {
				case Type::I1:
				case Type::I8: return 1;
				case Type::I16: return 2;
				case Type::I32:
				case Type::F32: return 4;
				case Type::I64:
				case Type::F64: return 8;
				case Type::F128: return 16;
				case Type::Ptr: return target.pointer_bits / target.char_bits;
				default: return 0;
				}
			}

			bool AppendScalar(const FrontendCType& type,
				uni::Nnode* initializer, stduint offset) {
				StaticScalarInitializer value;
				if (!ParseStaticScalarInitializer(initializer, type, target,
					type_info, constants, diagnostics, context, value))
					return Error("static initializer must contain constant expressions",
						initializer, DiagnosticStage::Lowering);
				if (type.integer_rank == FrontendCIntegerRank::Bool &&
					!type.enum_id) value.low = value.low != 0;
				GlobalInitializerFragment fragment;
				fragment.offset = offset;
				fragment.type = CloverTypeFor(type, target);
				fragment.initializer = value.low;
				fragment.initializer_high = value.high;
				fragment.initializer_symbol = value.symbol;
				fragment.initializer_addend = value.addend;
				return AppendFragment(fragment);
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
					if (!AppendFragment(fragment)) return false;
				}
				return true;
			}

			bool InitializeDesignatedAggregate(const FrontendCType& type,
				uni::Vector<uni::Nnode*>& items, stduint offset,
				const uni::Nnode* source) {
				InitializerCursor cursor(type, target, type_info, diagnostics);
				bool has_current = cursor.Reset();
				for (uni::Nnode* item : items) {
					if (IsDesignatedInitializer(item)) {
						if (!cursor.Select(item)) return false;
						InitializerTarget selected;
						uni::Nnode* value = DesignatedInitializerValue(item);
						if (!value || !cursor.Current(selected))
							return Error("invalid designated initializer", item);
						if (!InitializeObject(selected.type, value,
							offset + selected.offset)) return false;
						has_current = cursor.Advance();
						continue;
					}

					InitializerTarget selected;
					if (!has_current || !cursor.Current(selected))
						return Error(type.IsArray() ?
							"excess elements in array initializer" :
							"excess elements in record initializer", item);
					while (IsAggregateType(selected.type) &&
						item->type != tok_block &&
						!(selected.type.IsArray() &&
							FindStringArrayInitializer(item))) {
						if (!cursor.Descend() || !cursor.Current(selected))
							return Error("invalid aggregate initializer target", item);
					}
					if (!InitializeObject(selected.type, item,
						offset + selected.offset)) return false;
					has_current = cursor.Advance();
				}
				(void)source;
				return true;
			}

			bool InitializeAggregateFromItems(const FrontendCType& type,
				uni::Vector<uni::Nnode*>& items, stduint& cursor,
				stduint offset, const uni::Nnode* source) {
				if (type.IsArray()) {
					FrontendCType element_type = ArrayElementType(type);
					stduint element_size = TypeSize(element_type);
					if (!element_size) return Error("invalid array element size", source);
					for (stduint index = 0; index < type.ArrayDimension(0); index++)
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
				if (HasDesignatedInitializer(items))
					return InitializeDesignatedAggregate(type, items,
						offset, initializer);
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
				const FrontendCDiagnostics& diagnostic_output,
				const StaticInitializerContext& initializer_context) :
				global(output_global), target(target_info), type_info(semantic_types),
				constants(visible_constants), diagnostics(diagnostic_output),
				context(initializer_context) {}

			bool Build(const FrontendCType& type, uni::Nnode* initializer) {
				if (!InitializeObject(type, initializer, 0)) return false;
				for (stduint left = 0;
					left < global.initializer_fragments.Count(); left++)
					for (stduint right = left + 1;
						right < global.initializer_fragments.Count(); right++)
						if (global.initializer_fragments[right].offset <
							global.initializer_fragments[left].offset)
							global.initializer_fragments.Exchange(left, right);
				for (stduint index = 1;
					index < global.initializer_fragments.Count(); index++) {
					const GlobalInitializerFragment& previous =
						global.initializer_fragments.begin()[index - 1];
					const GlobalInitializerFragment& current =
						global.initializer_fragments.begin()[index];
					if (previous.offset + FragmentSize(previous) > current.offset)
						return Error("overlapping static designated initializers are not supported",
							initializer);
				}
				return true;
			}
		};
	}

	bool CompleteArrayTypeFromInitializer(FrontendCType& type,
		uni::Nnode* initializer, const FrontendCTargetInfo& target,
		const FrontendCTypeInfo& type_info,
		const FrontendCDiagnostics& diagnostics) {
		if (!type.IsArray() || type.ArrayDimension(0))
			return !type.HasIncompleteArray();
		for (stduint index = 1; index < type.ArrayDimensionCount(); index++)
			if (!type.ArrayDimension(index)) return false;
		if (!initializer) return false;
		uni::Nnode* string = FindStringArrayInitializer(initializer);
		if (string) {
			if (!IsCharacterArray(type) || type.ArrayDimensionCount() != 1)
				return false;
			uni::Vector<stdsint> bytes;
			if (!DecodeCString(string->addr, bytes)) return false;
			type.SetArrayDimension(0, bytes.Count() + 1);
			return true;
		}
		if (initializer->type != tok_block) return false;

		uni::Vector<uni::Nnode*> items;
		AppendDirectInitializerItems(initializer, items);
		if (!items.Count()) return false;
		stduint maximum_designated = 0;
		for (uni::Nnode* item : items) {
			if (!IsDesignatedInitializer(item)) continue;
			uni::Nnode* designator = item->subf;
			if (!designator || designator->type !=
				ToToken(FrontendCToken::IndexDesignator)) {
				diagnostics.Report(DiagnosticStage::Semantic,
					"incomplete array initializer requires an index designator",
					designator ? designator : item);
				return false;
			}
			stdsint value;
			if (!frontend_c_detail::EvaluateIntegerConstant(
				UnwrapInitializerExpression(designator->subf),
				target, type_info, value)) {
				diagnostics.Report(DiagnosticStage::Semantic,
					"array designator index must be an integer constant expression",
					designator);
				return false;
			}
			if (value < 0) {
				diagnostics.Report(DiagnosticStage::Semantic,
					"array designator index cannot be negative", designator);
				return false;
			}
			if ((stduint)value > maximum_designated)
				maximum_designated = (stduint)value;
		}

		stduint spare = items.Count() + 1;
		if (maximum_designated > ~stduint(0) - spare) return false;
		FrontendCType provisional = type;
		provisional.SetArrayDimension(0, maximum_designated + spare);
		InitializerCursor cursor(provisional, target, type_info, diagnostics);
		bool has_current = cursor.Reset();
		bool touched = false;
		stduint maximum_touched = 0;
		for (uni::Nnode* item : items) {
			if (IsDesignatedInitializer(item)) {
				if (!cursor.Select(item)) return false;
			}
			else if (!has_current) return false;

			InitializerTarget selected;
			if (!cursor.Current(selected)) return false;
			bool whole_object = IsDesignatedInitializer(item) ||
				item->type == tok_block ||
				(selected.type.IsArray() && FindStringArrayInitializer(item));
			while (IsAggregateType(selected.type) && !whole_object) {
				if (!cursor.Descend() || !cursor.Current(selected)) return false;
				whole_object = item->type == tok_block ||
					(selected.type.IsArray() && FindStringArrayInitializer(item));
			}
			stduint outer_index;
			if (!cursor.OuterIndex(outer_index)) return false;
			if (!touched || outer_index > maximum_touched)
				maximum_touched = outer_index;
			touched = true;
			has_current = cursor.Advance();
		}
		if (!touched || maximum_touched == ~stduint(0)) return false;
		type.SetArrayDimension(0, maximum_touched + 1);
		return true;
	}

	bool BuildStaticObjectInitializer(Global& global,
		const FrontendCType& type, uni::Nnode* initializer,
		const FrontendCTargetInfo& target,
		const FrontendCTypeInfo& type_info,
		const uni::Vector<FrontendCEnumerator>& constants,
		const FrontendCDiagnostics& diagnostics,
		const StaticInitializerContext& context) {
		StaticInitializerBuilder builder(global, target, type_info,
			constants, diagnostics, context);
		return builder.Build(type, initializer);
	}

	bool CompleteCompoundLiteralObjectType(uni::Nnode* node,
		FrontendCType& type, const FrontendCTargetInfo& target,
		const FrontendCTypeInfo& type_info,
		const FrontendCDiagnostics& diagnostics) {
		const FrontendCType* stored = type_info.Find(node);
		uni::Nnode* initializer = node ? node->subf : nullptr;
		if (!stored || !initializer || initializer->next ||
			initializer->type != tok_block) return false;
		type = *stored;
		if (type.HasIncompleteArray() && !CompleteArrayTypeFromInitializer(
			type, initializer, target, type_info, diagnostics)) return false;
		return !type.IsVoid() && !type.IsFunction() &&
			type_info.SizeBits(type, target) && type_info.AlignBits(type, target);
	}

	bool FunctionBuilder::CompleteCompoundLiteralType(
		uni::Nnode* node, FrontendCType& type) {
		return CompleteCompoundLiteralObjectType(node, type, *target,
			*type_info, *diagnostics);
	}

	LoweredValue FunctionBuilder::LowerCompoundLiteral(uni::Nnode* node) {
		FrontendCType type;
		if (!CompleteCompoundLiteralType(node, type))
			return Error("invalid compound literal type or initializer", node);
		LoweredValue storage = CreateTemporaryStorage(type);
		if (!storage.valid || !InitializeObject(storage.value, type,
			storage.alignment, node->subf)) return LoweredValue();
		return storage;
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
			for (stduint index = 0; index < type.ArrayDimension(0); index++) {
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

	bool FunctionBuilder::InitializeDesignatedAggregate(Value address,
		const FrontendCType& type, stduint alignment,
		uni::Vector<uni::Nnode*>& items, const uni::Nnode* source) {
		if (!ZeroInitializeObject(address, type, alignment, source)) return false;
		InitializerCursor cursor(type, *target, *type_info, *diagnostics);
		bool has_current = cursor.Reset();
		for (uni::Nnode* item : items) {
			if (IsDesignatedInitializer(item)) {
				if (!cursor.Select(item)) return false;
				InitializerTarget selected;
				uni::Nnode* value = DesignatedInitializerValue(item);
				if (!value || !cursor.Current(selected))
					return Error("invalid designated initializer", item).valid;
				stduint selected_alignment =
					OffsetAlignment(alignment, selected.offset);
				if (!InitializeObject(EmitByteOffset(address, selected.offset),
					selected.type, selected_alignment, value)) return false;
				has_current = cursor.Advance();
				continue;
			}

			InitializerTarget selected;
			if (!has_current || !cursor.Current(selected))
				return Error(type.IsArray() ?
					"excess elements in array initializer" :
					"excess elements in record initializer", item).valid;
			bool whole_object = item->type == tok_block ||
				(selected.type.IsArray() && FindStringArrayInitializer(item));
			if (!whole_object && selected.type.IsRecord()) {
				FrontendCType source_type;
				whole_object = ResolveExpressionType(item, source_type) &&
					source_type.IsRecord() &&
					selected.type.HasSameShape(source_type);
			}
			while (IsAggregateType(selected.type) && !whole_object) {
				if (!cursor.Descend() || !cursor.Current(selected))
					return Error("invalid aggregate initializer target", item).valid;
				whole_object = item->type == tok_block ||
					(selected.type.IsArray() && FindStringArrayInitializer(item));
				if (!whole_object && selected.type.IsRecord()) {
					FrontendCType source_type;
					whole_object = ResolveExpressionType(item, source_type) &&
						source_type.IsRecord() &&
						selected.type.HasSameShape(source_type);
				}
			}
			stduint selected_alignment = OffsetAlignment(alignment, selected.offset);
			if (!InitializeObject(EmitByteOffset(address, selected.offset),
				selected.type, selected_alignment, item)) return false;
			has_current = cursor.Advance();
		}
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
			for (stduint index = 0; index < type.ArrayDimension(0); index++) {
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
		if (HasDesignatedInitializer(items))
			return InitializeDesignatedAggregate(address, type, alignment,
				items, initializer);
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
