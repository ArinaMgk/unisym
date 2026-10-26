// UTF-8 CPP-ISO11 TAB4 LF
// Docutitle: Human Interface Device Report Descriptor
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../inc/cpp/Device/HID/HID-Report.hpp"
#include "../../../../inc/cpp/trait/MallocTrait.hpp"

namespace uni {
	namespace {
		enum class HIDItemType : uint8 { Main = 0, Global = 1, Local = 2, Reserved = 3 };

		struct HIDGlobalState {
			uint32 usage_page;
			int64 logical_minimum;
			int64 logical_maximum;
			int64 physical_minimum;
			int64 physical_maximum;
			int32 unit_exponent;
			uint32 unit;
			uint32 report_size;
			uint32 report_count;
			uint8 report_id;
		};

		struct HIDLocalState {
			HIDUsageSpan* spans;
			stduint span_count;
			stduint span_capacity;
			HIDUsage range_minimum;
			bool range_pending;
			uint8 delimiter_set;
			uint8 next_delimiter_set;
			bool delimiter_open;
		};

		struct HIDParseCounts {
			stduint definitions;
			stduint fields;
			stduint collections;
			stduint usage_spans;
			stduint long_items;
		};

		struct HIDParserWorkspace {
			trait::Malloc* allocator;
			HIDGlobalState* global_stack;
			stduint* collection_stack;
			HIDReportDefinition* definitions;
			HIDUsageSpan* local_spans;
			stduint capacity;

			HIDParserWorkspace(trait::Malloc* allocator) : allocator(allocator), global_stack(nullptr), collection_stack(nullptr), definitions(nullptr), local_spans(nullptr), capacity(0) {}

			~HIDParserWorkspace() { Release(); }

			void Release() {
				if (global_stack) allocator->deallocate(global_stack, capacity * sizeof(HIDGlobalState));
				if (collection_stack) allocator->deallocate(collection_stack, capacity * sizeof(stduint));
				if (definitions) allocator->deallocate(definitions, capacity * sizeof(HIDReportDefinition));
				if (local_spans) allocator->deallocate(local_spans, capacity * sizeof(HIDUsageSpan));
				global_stack = nullptr;
				collection_stack = nullptr;
				definitions = nullptr;
				local_spans = nullptr;
				capacity = 0;
			}

			bool Allocate(stduint requested_capacity) {
				capacity = requested_capacity;
				global_stack = static_cast<HIDGlobalState*>(allocator->allocate(capacity * sizeof(HIDGlobalState), alignof(HIDGlobalState)));
				collection_stack = static_cast<stduint*>(allocator->allocate(capacity * sizeof(stduint), alignof(stduint)));
				definitions = static_cast<HIDReportDefinition*>(allocator->allocate(capacity * sizeof(HIDReportDefinition), alignof(HIDReportDefinition)));
				local_spans = static_cast<HIDUsageSpan*>(allocator->allocate(capacity * sizeof(HIDUsageSpan), alignof(HIDUsageSpan)));
				return global_stack && collection_stack && definitions && local_spans;
			}
		};

		struct HIDPassOutput {
			HIDReportDefinition* definitions;
			stduint definition_capacity;
			HIDReportField* fields;
			stduint field_capacity;
			HIDReportCollection* collections;
			stduint collection_capacity;
			HIDUsageSpan* usage_spans;
			stduint usage_span_capacity;
		};

		struct HIDItem {
			stduint offset;
			stduint next_offset;
			uint32 value;
			int64 signed_value;
			uint8 prefix;
			uint8 tag;
			uint8 size;
			HIDItemType type;
			bool is_long;
		};

		HIDReportDescriptorStatus Fail(HIDReportParseResult& result, HIDReportDescriptorStatus status, stduint offset, uint8 prefix) {
			result.status = status;
			result.offset = offset;
			result.item_prefix = prefix;
			return status;
		}

		int64 SignExtend(uint32 value, uint8 size) {
			if (!size) return 0;
			const uint8 bits = uint8(size * 8);
			const uint64 sign = uint64(1) << (bits - 1);
			uint64 extended = value;
			if (extended & sign) extended |= ~uint64(0) << bits;
			return int64(extended);
		}

		HIDReportDescriptorStatus ReadItem(const byte* source, stduint length, stduint offset, HIDItem& item, HIDReportParseResult& result) {
			item = {};
			item.offset = offset;
			item.prefix = source[offset];
			if (item.prefix == 0xFEu) {
				if (length - offset < 3) return Fail(result, HIDReportDescriptorStatus::TruncatedItem, offset, item.prefix);
				const stduint payload_length = source[offset + 1];
				if (payload_length > length - offset - 3) return Fail(result, HIDReportDescriptorStatus::TruncatedItem, offset, item.prefix);
				item.is_long = true;
				item.tag = source[offset + 2];
				item.next_offset = offset + 3 + payload_length;
				return HIDReportDescriptorStatus::Success;
			}
			const uint8 size_code = item.prefix & 3u;
			item.size = size_code == 3 ? 4 : size_code;
			if (item.size > length - offset - 1) return Fail(result, HIDReportDescriptorStatus::TruncatedItem, offset, item.prefix);
			item.type = HIDItemType((item.prefix >> 2) & 3u);
			item.tag = item.prefix >> 4;
			for0(index, item.size) item.value |= uint32(source[offset + 1 + index]) << (index * 8);
			item.signed_value = SignExtend(item.value, item.size);
			item.next_offset = offset + 1 + item.size;
			return HIDReportDescriptorStatus::Success;
		}

		HIDUsage DecodeUsage(uint32 value, uint8 size, uint32 usage_page) {
			if (size == 4) return { value >> 16, value & 0xFFFFu };
			return { usage_page, value };
		}

		void ResetLocal(HIDLocalState& local) {
			local.span_count = 0;
			local.range_pending = false;
			local.delimiter_set = 0;
			local.next_delimiter_set = 1;
			local.delimiter_open = false;
		}

		HIDReportDescriptorStatus AddUsageSpan(HIDLocalState& local, const HIDUsage& minimum, const HIDUsage& maximum, HIDReportParseResult& result, const HIDItem& item) {
			if (local.span_count >= local.span_capacity) return Fail(result, HIDReportDescriptorStatus::ArithmeticOverflow, item.offset, item.prefix);
			if (minimum.page != maximum.page || minimum.id > maximum.id) return Fail(result, HIDReportDescriptorStatus::InvalidUsageRange, item.offset, item.prefix);
			local.spans[local.span_count++] = { minimum, maximum, local.delimiter_set };
			return HIDReportDescriptorStatus::Success;
		}

		HIDReportDefinition* FindOrAddDefinition(HIDReportDefinition* definitions, stduint& count, stduint capacity, HIDReportType type, uint8 report_id) {
			for0(index, count) {
				if (definitions[index].type == type && definitions[index].report_id == report_id) return definitions + index;
			}
			if (count >= capacity) return nullptr;
			auto* definition = definitions + count++;
			*definition = {};
			definition->type = type;
			definition->report_id = report_id;
			definition->has_report_id = report_id != 0;
			return definition;
		}

		HIDReportDescriptorStatus AddField(const HIDGlobalState& global, const HIDLocalState& local, HIDReportType type, uint16 flags, stduint current_collection, HIDPassOutput& output, HIDParseCounts& counts, HIDReportParseResult& result, const HIDItem& item) {
			if (!global.report_size) return Fail(result, HIDReportDescriptorStatus::InvalidReportSize, item.offset, item.prefix);
			if (!global.report_count) return Fail(result, HIDReportDescriptorStatus::InvalidReportCount, item.offset, item.prefix);
			if (local.range_pending) return Fail(result, HIDReportDescriptorStatus::InvalidUsageRange, item.offset, item.prefix);
			if (local.delimiter_open) return Fail(result, HIDReportDescriptorStatus::InvalidDelimiter, item.offset, item.prefix);
			if (uint64(global.report_size) > (~uint64(0) / uint64(global.report_count))) return Fail(result, HIDReportDescriptorStatus::ArithmeticOverflow, item.offset, item.prefix);
			const uint64 field_bits = uint64(global.report_size) * uint64(global.report_count);
			if (counts.definitions > output.definition_capacity) return Fail(result, HIDReportDescriptorStatus::ArithmeticOverflow, item.offset, item.prefix);
			auto* definition = FindOrAddDefinition(output.definitions, counts.definitions, output.definition_capacity, type, global.report_id);
			if (!definition) return Fail(result, HIDReportDescriptorStatus::ArithmeticOverflow, item.offset, item.prefix);
			if (definition->payload_bits > ~uint64(0) - field_bits) return Fail(result, HIDReportDescriptorStatus::ArithmeticOverflow, item.offset, item.prefix);
			const uint64 bit_offset = definition->payload_bits;
			definition->payload_bits += field_bits;
			if ((definition->payload_bits + 7u) / 8u > HIDReportMaximumPayloadLength) return Fail(result, HIDReportDescriptorStatus::ReportTooLarge, item.offset, item.prefix);
			if (counts.fields >= output.field_capacity) return Fail(result, HIDReportDescriptorStatus::ArithmeticOverflow, item.offset, item.prefix);
			if (local.span_count > output.usage_span_capacity - counts.usage_spans) return Fail(result, HIDReportDescriptorStatus::ArithmeticOverflow, item.offset, item.prefix);
			if (output.fields) {
				auto& field = output.fields[counts.fields];
				field = {};
				field.type = type;
				field.report_id = global.report_id;
				field.flags = flags;
				field.bit_offset = bit_offset;
				field.report_size = global.report_size;
				field.report_count = global.report_count;
				field.logical_minimum = global.logical_minimum;
				field.logical_maximum = global.logical_maximum;
				field.physical_minimum = global.physical_minimum;
				field.physical_maximum = global.physical_maximum;
				field.unit_exponent = global.unit_exponent;
				field.unit = global.unit;
				field.collection_index = current_collection;
				field.usage_span_index = counts.usage_spans;
				field.usage_span_count = local.span_count;
				for0(index, local.span_count) output.usage_spans[counts.usage_spans + index] = local.spans[index];
			}
			++counts.fields;
			counts.usage_spans += local.span_count;
			return HIDReportDescriptorStatus::Success;
		}

		HIDReportDescriptorStatus ParseMain(const HIDItem& item, const HIDGlobalState& global, HIDLocalState& local, stduint* collection_stack, stduint& collection_depth, HIDPassOutput& output, HIDParseCounts& counts, HIDReportParseResult& result) {
			const stduint current_collection = collection_depth ? collection_stack[collection_depth - 1] : HIDReportNoCollection;
			HIDReportDescriptorStatus status = HIDReportDescriptorStatus::Success;
			switch (item.tag) {
			case 8:
				status = AddField(global, local, HIDReportType::Input, uint16(item.value), current_collection, output, counts, result, item);
				break;
			case 9:
				status = AddField(global, local, HIDReportType::Output, uint16(item.value), current_collection, output, counts, result, item);
				break;
			case 10: {
				if (local.range_pending) return Fail(result, HIDReportDescriptorStatus::InvalidUsageRange, item.offset, item.prefix);
				if (local.delimiter_open) return Fail(result, HIDReportDescriptorStatus::InvalidDelimiter, item.offset, item.prefix);
				if (counts.collections >= output.collection_capacity || collection_depth >= output.collection_capacity) return Fail(result, HIDReportDescriptorStatus::CollectionLimit, item.offset, item.prefix);
				const stduint collection_index = counts.collections++;
				if (output.collections) {
					auto& collection = output.collections[collection_index];
					collection = {};
					collection.type = uint8(item.value);
					collection.parent_index = current_collection;
					if (local.span_count) collection.usage = local.spans[0].minimum;
				}
				collection_stack[collection_depth++] = collection_index;
				break;
			}
			case 11:
				status = AddField(global, local, HIDReportType::Feature, uint16(item.value), current_collection, output, counts, result, item);
				break;
			case 12:
				if (!collection_depth) return Fail(result, HIDReportDescriptorStatus::CollectionUnderflow, item.offset, item.prefix);
				--collection_depth;
				break;
			default:
				return Fail(result, HIDReportDescriptorStatus::UnsupportedMainItem, item.offset, item.prefix);
			}
			if (status == HIDReportDescriptorStatus::Success) ResetLocal(local);
			return status;
		}

		HIDReportDescriptorStatus ParseGlobal(const HIDItem& item, HIDGlobalState& global, HIDGlobalState* global_stack, stduint& global_depth, stduint capacity, HIDReportParseResult& result) {
			switch (item.tag) {
			case 0: global.usage_page = item.value; break;
			case 1: global.logical_minimum = item.signed_value; break;
			case 2: global.logical_maximum = global.logical_minimum < 0 ? item.signed_value : int64(item.value); break;
			case 3: global.physical_minimum = item.signed_value; break;
			case 4: global.physical_maximum = global.physical_minimum < 0 ? item.signed_value : int64(item.value); break;
			case 5: {
				const uint8 exponent = uint8(item.value & 0x0Fu);
				global.unit_exponent = exponent & 8u ? int32(exponent) - 16 : exponent;
				break;
			}
			case 6: global.unit = item.value; break;
			case 7:
				if (!item.value) return Fail(result, HIDReportDescriptorStatus::InvalidReportSize, item.offset, item.prefix);
				global.report_size = item.value;
				break;
			case 8:
				if (!item.value || item.value > 0xFFu) return Fail(result, HIDReportDescriptorStatus::InvalidReportId, item.offset, item.prefix);
				global.report_id = uint8(item.value);
				break;
			case 9:
				if (!item.value) return Fail(result, HIDReportDescriptorStatus::InvalidReportCount, item.offset, item.prefix);
				global.report_count = item.value;
				break;
			case 10:
				if (global_depth >= capacity) return Fail(result, HIDReportDescriptorStatus::GlobalStackLimit, item.offset, item.prefix);
				global_stack[global_depth++] = global;
				break;
			case 11:
				if (!global_depth) return Fail(result, HIDReportDescriptorStatus::GlobalStackUnderflow, item.offset, item.prefix);
				global = global_stack[--global_depth];
				break;
			default:
				return Fail(result, HIDReportDescriptorStatus::UnsupportedGlobalItem, item.offset, item.prefix);
			}
			return HIDReportDescriptorStatus::Success;
		}

		HIDReportDescriptorStatus ParseLocal(const HIDItem& item, const HIDGlobalState& global, HIDLocalState& local, HIDReportParseResult& result) {
			switch (item.tag) {
			case 0: {
				const auto usage = DecodeUsage(item.value, item.size, global.usage_page);
				return AddUsageSpan(local, usage, usage, result, item);
			}
			case 1:
				if (local.range_pending) return Fail(result, HIDReportDescriptorStatus::InvalidUsageRange, item.offset, item.prefix);
				local.range_minimum = DecodeUsage(item.value, item.size, global.usage_page);
				local.range_pending = true;
				break;
			case 2: {
				if (!local.range_pending) return Fail(result, HIDReportDescriptorStatus::InvalidUsageRange, item.offset, item.prefix);
				const auto maximum = DecodeUsage(item.value, item.size, global.usage_page);
				const auto status = AddUsageSpan(local, local.range_minimum, maximum, result, item);
				if (status != HIDReportDescriptorStatus::Success) return status;
				local.range_pending = false;
				break;
			}
			case 3:
			case 4:
			case 5:
			case 7:
			case 8:
			case 9:
				break;
			case 10:
				if (item.value == 1) {
					if (local.delimiter_open || local.next_delimiter_set == 0) return Fail(result, HIDReportDescriptorStatus::InvalidDelimiter, item.offset, item.prefix);
					local.delimiter_open = true;
					local.delimiter_set = local.next_delimiter_set++;
				}
				else if (item.value == 0) {
					if (!local.delimiter_open) return Fail(result, HIDReportDescriptorStatus::InvalidDelimiter, item.offset, item.prefix);
					local.delimiter_open = false;
					local.delimiter_set = 0;
				}
				else return Fail(result, HIDReportDescriptorStatus::InvalidDelimiter, item.offset, item.prefix);
				break;
			default:
				return Fail(result, HIDReportDescriptorStatus::UnsupportedLocalItem, item.offset, item.prefix);
			}
			return HIDReportDescriptorStatus::Success;
		}

		HIDReportDescriptorStatus RunPass(const byte* source, stduint length, HIDParserWorkspace& workspace, HIDPassOutput& output, HIDParseCounts& counts, HIDReportParseResult& result) {
			counts = {};
			HIDGlobalState global{};
			HIDLocalState local{ workspace.local_spans, 0, workspace.capacity, {}, false, 0, 1, false };
			stduint global_depth = 0;
			stduint collection_depth = 0;
			stduint offset = 0;
			while (offset < length) {
				HIDItem item{};
				auto status = ReadItem(source, length, offset, item, result);
				if (status != HIDReportDescriptorStatus::Success) return status;
				if (item.is_long) {
					++counts.long_items;
					offset = item.next_offset;
					continue;
				}
				switch (item.type) {
				case HIDItemType::Main: status = ParseMain(item, global, local, workspace.collection_stack, collection_depth, output, counts, result); break;
				case HIDItemType::Global: status = ParseGlobal(item, global, workspace.global_stack, global_depth, workspace.capacity, result); break;
				case HIDItemType::Local: status = ParseLocal(item, global, local, result); break;
				default: status = HIDReportDescriptorStatus::Success; break;
				}
				if (status != HIDReportDescriptorStatus::Success) return status;
				offset = item.next_offset;
			}
			if (local.range_pending) return Fail(result, HIDReportDescriptorStatus::InvalidUsageRange, length, 0);
			if (local.delimiter_open) return Fail(result, HIDReportDescriptorStatus::InvalidDelimiter, length, 0);
			if (collection_depth) return Fail(result, HIDReportDescriptorStatus::CollectionNotClosed, length, 0);
			for0(index, counts.definitions) {
				auto& definition = output.definitions[index];
				definition.payload_bytes = stduint((definition.payload_bits + 7u) / 8u);
			}
			return HIDReportDescriptorStatus::Success;
		}

		bool AllocateArray(trait::Malloc* allocator, stduint count, stduint size, stduint alignment, void*& destination) {
			destination = nullptr;
			if (!count) return true;
			if (count > ~stduint(0) / size) return false;
			destination = allocator->allocate(count * size, alignment);
			return destination != nullptr;
		}
	}

	HIDReportLayout::~HIDReportLayout() {
		Clear();
	}

	void HIDReportLayout::Clear() {
		if (allocator) {
			if (definitions) allocator->deallocate(definitions, definition_count * sizeof(HIDReportDefinition));
			if (fields) allocator->deallocate(fields, field_count * sizeof(HIDReportField));
			if (collections) allocator->deallocate(collections, collection_count * sizeof(HIDReportCollection));
			if (usage_spans) allocator->deallocate(usage_spans, usage_span_count * sizeof(HIDUsageSpan));
		}
		allocator = nullptr;
		definitions = nullptr;
		fields = nullptr;
		collections = nullptr;
		usage_spans = nullptr;
		definition_count = 0;
		field_count = 0;
		collection_count = 0;
		usage_span_count = 0;
	}

	HIDReportDescriptorStatus HIDReportLayout::Parse(const byte* source, stduint length, trait::Malloc* requested_allocator, HIDReportParseResult& result) {
		result = {};
		result.status = HIDReportDescriptorStatus::InvalidArgument;
		Clear();
		if (!source || !length || !requested_allocator || length > HIDReportDescriptorMaximumLength) return result.status;
		HIDParserWorkspace workspace(requested_allocator);
		if (!workspace.Allocate(length + 1)) return Fail(result, HIDReportDescriptorStatus::AllocationFailed, 0, 0);
		HIDParseCounts first_counts{};
		HIDPassOutput first_output{ workspace.definitions, workspace.capacity, nullptr, workspace.capacity, nullptr, workspace.capacity, nullptr, workspace.capacity };
		auto status = RunPass(source, length, workspace, first_output, first_counts, result);
		if (status != HIDReportDescriptorStatus::Success) return status;
		allocator = requested_allocator;
		definition_count = first_counts.definitions;
		field_count = first_counts.fields;
		collection_count = first_counts.collections;
		usage_span_count = first_counts.usage_spans;
		void* storage = nullptr;
		if (!AllocateArray(allocator, definition_count, sizeof(HIDReportDefinition), alignof(HIDReportDefinition), storage)) status = HIDReportDescriptorStatus::AllocationFailed;
		definitions = static_cast<HIDReportDefinition*>(storage);
		if (status == HIDReportDescriptorStatus::Success && !AllocateArray(allocator, field_count, sizeof(HIDReportField), alignof(HIDReportField), storage)) status = HIDReportDescriptorStatus::AllocationFailed;
		fields = status == HIDReportDescriptorStatus::Success ? static_cast<HIDReportField*>(storage) : nullptr;
		if (status == HIDReportDescriptorStatus::Success && !AllocateArray(allocator, collection_count, sizeof(HIDReportCollection), alignof(HIDReportCollection), storage)) status = HIDReportDescriptorStatus::AllocationFailed;
		collections = status == HIDReportDescriptorStatus::Success ? static_cast<HIDReportCollection*>(storage) : nullptr;
		if (status == HIDReportDescriptorStatus::Success && !AllocateArray(allocator, usage_span_count, sizeof(HIDUsageSpan), alignof(HIDUsageSpan), storage)) status = HIDReportDescriptorStatus::AllocationFailed;
		usage_spans = status == HIDReportDescriptorStatus::Success ? static_cast<HIDUsageSpan*>(storage) : nullptr;
		if (status != HIDReportDescriptorStatus::Success) {
			Clear();
			return Fail(result, status, 0, 0);
		}
		HIDParseCounts second_counts{};
		HIDPassOutput second_output{ definitions, definition_count, fields, field_count, collections, collection_count, usage_spans, usage_span_count };
		status = RunPass(source, length, workspace, second_output, second_counts, result);
		if (status != HIDReportDescriptorStatus::Success || second_counts.definitions != definition_count || second_counts.fields != field_count || second_counts.collections != collection_count || second_counts.usage_spans != usage_span_count) {
			Clear();
			if (status == HIDReportDescriptorStatus::Success) status = Fail(result, HIDReportDescriptorStatus::ArithmeticOverflow, 0, 0);
			return status;
		}
		result.status = HIDReportDescriptorStatus::Success;
		result.offset = length;
		result.item_prefix = 0;
		result.definition_count = definition_count;
		result.field_count = field_count;
		result.collection_count = collection_count;
		result.usage_span_count = usage_span_count;
		result.long_item_count = second_counts.long_items;
		return result.status;
	}

	const HIDReportDefinition* HIDReportLayout::FindDefinition(HIDReportType type, uint8 report_id) const {
		for0(index, definition_count) {
			if (definitions[index].type == type && definitions[index].report_id == report_id) return definitions + index;
		}
		return nullptr;
	}

	HIDReportValueStatus HIDReportLayout::ExtractUnsigned(const HIDReportField& field, stduint element, const byte* report, stduint length, uint64& value) const {
		value = 0;
		if (!report) return HIDReportValueStatus::InvalidArgument;
		const auto* definition = FindDefinition(field.type, field.report_id);
		if (!definition) return HIDReportValueStatus::DefinitionNotFound;
		if (element >= field.report_count) return HIDReportValueStatus::ElementOutOfRange;
		if (!field.report_size || field.report_size > 64) return HIDReportValueStatus::ValueTooWide;
		const stduint prefix_length = definition->has_report_id ? 1 : 0;
		if (length < prefix_length + definition->payload_bytes) return HIDReportValueStatus::BufferTooSmall;
		if (definition->has_report_id && report[0] != definition->report_id) return HIDReportValueStatus::ReportIdMismatch;
		const uint64 element_offset = uint64(field.report_size) * element;
		if (field.bit_offset > ~uint64(0) - element_offset) return HIDReportValueStatus::ElementOutOfRange;
		const uint64 bit_offset = field.bit_offset + element_offset;
		if (bit_offset > definition->payload_bits || field.report_size > definition->payload_bits - bit_offset) return HIDReportValueStatus::ElementOutOfRange;
		const byte* payload = report + prefix_length;
		for0(bit, field.report_size) {
			const uint64 source_bit = bit_offset + bit;
			if (payload[source_bit / 8] & (1u << (source_bit & 7u))) value |= uint64(1) << bit;
		}
		return HIDReportValueStatus::Success;
	}

	HIDReportValueStatus HIDReportLayout::ExtractSigned(const HIDReportField& field, stduint element, const byte* report, stduint length, int64& value) const {
		uint64 unsigned_value = 0;
		const auto status = ExtractUnsigned(field, element, report, length, unsigned_value);
		if (status != HIDReportValueStatus::Success) {
			value = 0;
			return status;
		}
		if (field.logical_minimum < 0 && field.report_size < 64 && (unsigned_value & (uint64(1) << (field.report_size - 1)))) unsigned_value |= ~uint64(0) << field.report_size;
		value = int64(unsigned_value);
		return HIDReportValueStatus::Success;
	}

	bool HIDReportLayout::ResolveUsageExact(const HIDReportField& field, uint64 ordinal, HIDUsage& usage) const {
		if (field.usage_span_index > usage_span_count || field.usage_span_count > usage_span_count - field.usage_span_index) return false;
		uint8 selected_set = 0xFFu;
		for0(index, field.usage_span_count) {
			const uint8 set = usage_spans[field.usage_span_index + index].delimiter_set;
			if (set == 0) { selected_set = 0; break; }
			if (set < selected_set) selected_set = set;
		}
		if (selected_set == 0xFFu) return false;
		for0(index, field.usage_span_count) {
			const auto& span = usage_spans[field.usage_span_index + index];
			if (span.delimiter_set != selected_set) continue;
			const uint64 span_length = uint64(span.maximum.id) - span.minimum.id + 1;
			if (ordinal < span_length) {
				usage = { span.minimum.page, uint32(uint64(span.minimum.id) + ordinal) };
				return true;
			}
			ordinal -= span_length;
		}
		return false;
	}

	bool HIDReportLayout::ResolveUsage(const HIDReportField& field, stduint element, HIDUsage& usage) const {
		if (element >= field.report_count) return false;
		if (field.usage_span_index > usage_span_count || field.usage_span_count > usage_span_count - field.usage_span_index) return false;
		if (ResolveUsageExact(field, element, usage)) return true;
		if (!field.usage_span_count) return false;
		uint64 total = 0;
		uint8 selected_set = 0xFFu;
		for0(index, field.usage_span_count) {
			const auto& span = usage_spans[field.usage_span_index + index];
			if (span.delimiter_set == 0) { selected_set = 0; break; }
			if (span.delimiter_set < selected_set) selected_set = span.delimiter_set;
		}
		for0(index, field.usage_span_count) {
			const auto& span = usage_spans[field.usage_span_index + index];
			if (span.delimiter_set != selected_set) continue;
			total += uint64(span.maximum.id) - span.minimum.id + 1;
		}
		return total && ResolveUsageExact(field, total - 1, usage);
	}

	bool HIDReportLayout::ResolveArrayUsage(const HIDReportField& field, uint64 selector, HIDUsage& usage) const {
		if (selector < uint64(field.logical_minimum < 0 ? 0 : field.logical_minimum)) return false;
		const uint64 minimum = field.logical_minimum < 0 ? 0 : uint64(field.logical_minimum);
		return ResolveUsageExact(field, selector - minimum, usage);
	}
}
