// UTF-8 CPP-ISO11 TAB4 LF
// Docutitle: Human Interface Device Report Descriptor
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#ifndef _INCPP_Device_HID_Report
#define _INCPP_Device_HID_Report

#include "../../unisym"

namespace uni {
	namespace trait { class Malloc; }

	constexpr stduint HIDReportNoCollection = ~stduint(0);
	constexpr stduint HIDReportDescriptorMaximumLength = 0xFFFFu;
	constexpr stduint HIDReportMaximumPayloadLength = 0xFFFFu;

	enum class HIDReportType : uint8 { Input = 1, Output = 2, Feature = 3 };

	enum HIDReportMainFlags : uint16 {
		HIDReportMainFlag_Constant = 1u << 0,
		HIDReportMainFlag_Variable = 1u << 1,
		HIDReportMainFlag_Relative = 1u << 2,
		HIDReportMainFlag_Wrap = 1u << 3,
		HIDReportMainFlag_Nonlinear = 1u << 4,
		HIDReportMainFlag_NoPreferred = 1u << 5,
		HIDReportMainFlag_NullState = 1u << 6,
		HIDReportMainFlag_Volatile = 1u << 7,
		HIDReportMainFlag_BufferedBytes = 1u << 8,
	};

	enum class HIDReportDescriptorStatus : uint8 {
		Success,
		InvalidArgument,
		AllocationFailed,
		TruncatedItem,
		UnsupportedMainItem,
		UnsupportedGlobalItem,
		UnsupportedLocalItem,
		InvalidReportId,
		InvalidReportSize,
		InvalidReportCount,
		ArithmeticOverflow,
		GlobalStackUnderflow,
		GlobalStackLimit,
		CollectionUnderflow,
		CollectionNotClosed,
		CollectionLimit,
		InvalidUsageRange,
		InvalidDelimiter,
		ReportTooLarge,
	};

	struct HIDUsage {
		uint32 page;
		uint32 id;
	};

	struct HIDUsageSpan {
		HIDUsage minimum;
		HIDUsage maximum;
		uint8 delimiter_set;
	};

	struct HIDReportDefinition {
		HIDReportType type;
		uint8 report_id;
		uint64 payload_bits;
		stduint payload_bytes;
		bool has_report_id;
	};

	struct HIDReportField {
		HIDReportType type;
		uint8 report_id;
		uint16 flags;
		uint64 bit_offset;
		uint32 report_size;
		uint32 report_count;
		int64 logical_minimum;
		int64 logical_maximum;
		int64 physical_minimum;
		int64 physical_maximum;
		int32 unit_exponent;
		uint32 unit;
		stduint collection_index;
		stduint usage_span_index;
		stduint usage_span_count;
	};

	struct HIDReportCollection {
		uint8 type;
		HIDUsage usage;
		stduint parent_index;
	};

	struct HIDReportParseResult {
		HIDReportDescriptorStatus status;
		stduint offset;
		uint8 item_prefix;
		stduint definition_count;
		stduint field_count;
		stduint collection_count;
		stduint usage_span_count;
		stduint long_item_count;
	};

	enum class HIDReportValueStatus : uint8 {
		Success,
		InvalidArgument,
		DefinitionNotFound,
		ReportIdMismatch,
		BufferTooSmall,
		ElementOutOfRange,
		ValueTooWide,
	};

	class HIDReportLayout {
		trait::Malloc* allocator = nullptr;
		HIDReportDefinition* definitions = nullptr;
		HIDReportField* fields = nullptr;
		HIDReportCollection* collections = nullptr;
		HIDUsageSpan* usage_spans = nullptr;
		stduint definition_count = 0;
		stduint field_count = 0;
		stduint collection_count = 0;
		stduint usage_span_count = 0;

		bool ResolveUsageExact(const HIDReportField& field, uint64 ordinal, HIDUsage& usage) const;

	public:
		HIDReportLayout() = default;
		~HIDReportLayout();
		HIDReportLayout(const HIDReportLayout&) = delete;
		HIDReportLayout& operator=(const HIDReportLayout&) = delete;

		void Clear();
		HIDReportDescriptorStatus Parse(const byte* source, stduint length, trait::Malloc* allocator, HIDReportParseResult& result);
		const HIDReportDefinition* FindDefinition(HIDReportType type, uint8 report_id) const;
		HIDReportValueStatus ExtractUnsigned(const HIDReportField& field, stduint element, const byte* report, stduint length, uint64& value) const;
		HIDReportValueStatus ExtractSigned(const HIDReportField& field, stduint element, const byte* report, stduint length, int64& value) const;
		bool ResolveUsage(const HIDReportField& field, stduint element, HIDUsage& usage) const;
		bool ResolveArrayUsage(const HIDReportField& field, uint64 selector, HIDUsage& usage) const;

		const HIDReportDefinition* Definitions() const { return definitions; }
		const HIDReportField* Fields() const { return fields; }
		const HIDReportCollection* Collections() const { return collections; }
		const HIDUsageSpan* UsageSpans() const { return usage_spans; }
		stduint DefinitionCount() const { return definition_count; }
		stduint FieldCount() const { return field_count; }
		stduint CollectionCount() const { return collection_count; }
		stduint UsageSpanCount() const { return usage_span_count; }
	};
}

#endif
