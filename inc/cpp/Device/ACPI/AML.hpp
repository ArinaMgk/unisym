// UTF-8 CPP-ISO11 TAB4 CRLF
// Docutitle: ACPI Machine Language Namespace and Bytecode
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
/*
	Copyright 2023 ArinaMgk

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

#ifndef _INCPP_Device_ACPI_AML
#define _INCPP_Device_ACPI_AML

#include "../../../c/stdinc.h"

namespace uni {

	class AMLBytecode {
	public:
		static constexpr stduint NameSegmentCapacity = 16;

		struct Name {
			char segments[NameSegmentCapacity][5];
			uint8 segment_count;
			uint8 parent_count;
			bool rooted;
		};

		static bool DecodePackageLength(const uint8* aml, stduint available, uint32& package_length, uint8& encoded_length);
		static bool DecodeInteger(const uint8*& cursor, const uint8* end, uint64& value);
		static bool DecodeName(const uint8*& cursor, const uint8* end, Name& name);
		static const uint8* SkipSimpleDataObject(const uint8* cursor, const uint8* end);
	};

	class AMLNamespace {
	public:
		static constexpr stduint InvalidIndex = ~stduint(0);
		static constexpr stduint MaximumDepth = 32;

		enum class ObjectType : uint8 {
			Root,
			Scope,
			Device,
			Processor,
			PowerResource,
			ThermalZone,
			Name,
			Method,
			Alias,
			External,
			Mutex,
			Event,
			OperationRegion,
			FieldUnit,
			BufferField,
		};

		enum ObjectFlag : uint8 {
			ObjectFlagNone = 0,
			ObjectFlagConditional = 1,
			ObjectFlagPlaceholder = 2,
			ObjectFlagRuntime = 4,
		};

		enum class Status : uint8 {
			Success,
			InvalidArgument,
			MalformedPackage,
			MalformedName,
			MalformedObject,
			Capacity,
			DepthLimit,
		};

		struct Object {
			const uint8* value;
			stduint value_length;
			stduint parent;
			stduint reference;
			stduint secondary_reference;
			uint64 bit_offset;
			uint64 bit_length;
			char name[5];
			uint16 table_index;
			ObjectType type;
			uint8 flags;
			uint8 argument_count;
			uint8 method_flags;
			uint8 external_type;
			uint8 field_flags;
		};

		struct Error {
			const uint8* address;
			stduint offset;
			uint16 table_index;
			Status status;
		};

		AMLNamespace(Object* storage, stduint capacity);

		void Reset();
		Status Load(const uint8* aml, stduint length, uint16 table_index = 0);
		stduint Count() const;
		const Object* At(stduint index) const;
		stduint FindChild(stduint parent, const char name[5]) const;
		stduint Find(stduint scope, const AMLBytecode::Name& name) const;
		stduint FindPath(const char* path) const;
		bool FormatPath(stduint index, char* path, stduint capacity) const;
		stduint ConditionalCount() const;
		bool Truncated() const;
		const Error& LastError() const;

	private:
		Object* objects;
		stduint capacity;
		stduint count;
		stduint conditional_count;
		bool truncated;
		const uint8* table_begin;
		const uint8* table_end;
		uint16 current_table;
		Error last_error;

		Status Fail(Status status, const uint8* address);
		stduint Add(stduint parent, const char name[5], ObjectType type, uint8 flags, const uint8* value, stduint value_length);
		stduint Define(stduint scope, const AMLBytecode::Name& name, ObjectType type, uint8 flags, const uint8* value, stduint value_length);
		stduint ResolveOrCreateScope(stduint scope, const AMLBytecode::Name& name, uint8 flags);
		stduint ResolveDefinitionParent(stduint scope, const AMLBytecode::Name& name, uint8 flags);
		stduint ParentAfterPrefixes(stduint scope, const AMLBytecode::Name& name) const;
		Status ParseTermList(const uint8* begin, const uint8* end, stduint scope, uint8 flags, stduint depth);
		Status ParseExtended(const uint8*& cursor, const uint8* end, stduint scope, uint8 flags, stduint depth);
		Status ParseFieldList(const uint8* begin, const uint8* end, stduint scope, stduint reference, stduint secondary_reference, const uint8* declaration, stduint declaration_length, uint8 field_flags, uint8 flags);
		const uint8* SkipTermArgument(const uint8* cursor, const uint8* end, stduint scope, stduint depth) const;
		bool PackageRange(const uint8* length_at, const uint8* limit, const uint8*& content, const uint8*& object_end) const;
	};

	class AMLExecutor {
	public:
		enum class ValueType : uint8 {
			Uninitialized,
			Integer,
			String,
			Buffer,
			Package,
			Reference,
		};

		enum class ReferenceType : uint8 {
			None,
			Object,
			Local,
			Argument,
		};

		enum class Status : uint8 {
			Success,
			InvalidArgument,
			NotFound,
			Malformed,
			UnsupportedOpcode,
			UnsupportedObject,
			ConditionalObject,
			TypeMismatch,
			InvalidTarget,
			RegionUnavailable,
			FrameLimit,
			InstructionLimit,
			LoopLimit,
			ObjectValueCapacity,
			DynamicObjectCapacity,
			MutexCapacity,
			BufferCapacity,
			PackageCapacity,
			Timeout,
			NoReturn,
			Count,
		};

		struct Value {
			const uint8* data;
			uint8* writable_data;
			stduint length;
			stduint data_length;
			uint64 integer;
			stduint reference;
			stduint reference_frame;
			const Value* elements;
			Value* writable_elements;
			stduint element_count;
			ValueType type;
			ReferenceType reference_type;
		};

		struct Frame {
			Value arguments[7];
			Value locals[8];
			stduint scope;
		};

		struct ObjectValue {
			Value value;
			stduint object;
			bool valid;
		};

		struct MutexState {
			stduint object;
			stduint previous;
			uint8 sync_level;
			bool valid;
		};

		using RegionReadRoutine = bool (*)(void* context, const AMLNamespace& aml_namespace, stduint region_object, stduint region_scope, uint8 space, uint64 address, uint8 width, uint64& value);
		using RegionWriteRoutine = bool (*)(void* context, const AMLNamespace& aml_namespace, stduint region_object, stduint region_scope, uint8 space, uint64 address, uint8 width, uint64 value);
		using TimeoutRoutine = bool (*)(void* context);

		struct RegionAccess {
			void* context;
			RegionReadRoutine read;
			RegionWriteRoutine write;
		};

		struct Workspace {
			Frame* frames;
			stduint frame_capacity;
			ObjectValue* object_values;
			stduint object_value_capacity;
			RegionAccess region_access;
			AMLNamespace::Object* dynamic_objects;
			stduint dynamic_object_capacity;
			MutexState* mutex_states;
			stduint mutex_capacity;
			uint8* buffer_storage;
			stduint buffer_capacity;
			Value* package_values;
			stduint package_value_capacity;
			Value* method_arguments;
			stduint method_argument_capacity;
		};

		struct ValueStorage {
			Value* values;
			stduint value_capacity;
			uint8* bytes;
			stduint byte_capacity;
		};

		struct Limits {
			stduint instruction_count;
			stduint loop_count;
			stduint method_depth;
			void* timeout_context;
			TimeoutRoutine timeout;
		};

		struct Error {
			const uint8* address;
			stduint object;
			stduint scope;
			uint16 opcode;
			Status status;
		};

		AMLExecutor(const AMLNamespace& aml_namespace, const Workspace& workspace, const Limits& limits);

		void Reset();
		Status Evaluate(stduint object, Value& value);
		Status Evaluate(stduint object, Value& value, const ValueStorage& value_storage);
		Status EvaluateInteger(stduint object, uint64& value);
		Status Invoke(stduint method, const Value* arguments, stduint argument_count, Value& value, const ValueStorage& value_storage);
		Status Invoke(const char* path, const Value* arguments, stduint argument_count, Value& value, const ValueStorage& value_storage);
		const Error& LastError() const;

	private:
		enum class TargetType : uint8 {
			Null,
			Object,
			Local,
			Argument,
		};

		struct Target {
			stduint index;
			stduint frame;
			TargetType type;
		};

		struct FieldAccess {
			const AMLNamespace::Object* field;
			stduint region_object;
			stduint region_scope;
			uint64 region_offset;
			uint64 region_length;
			uint16 field_opcode;
			uint8 space;
			uint8 width;
		};

		const AMLNamespace& aml_namespace;
		Workspace workspace;
		Limits limits;
		stduint frame_count;
		stduint instruction_count;
		stduint loop_count;
		stduint object_depth;
		stduint dynamic_object_count;
		stduint mutex_top;
		stduint buffer_used;
		stduint package_value_used;
		stduint method_argument_depth;
		Error last_error;

		Status Fail(Status status, const uint8* address, stduint object = AMLNamespace::InvalidIndex, uint16 opcode = 0);
		Status ConsumeInstruction(const uint8* address);
		Status EvaluateObject(stduint object, Frame* frame, Value& value);
		Status ResolveFieldWidth(const AMLNamespace::Object& field, stduint object, uint16 opcode, uint8& width);
		Status ResolveField(stduint object, Frame* frame, FieldAccess& access);
		Status ReadField(stduint object, Frame* frame, Value& value);
		Status WriteField(stduint object, Frame* frame, const Value& value);
		Status ReadIndexField(stduint object, Frame* frame, Value& value);
		Status WriteIndexField(stduint object, Frame* frame, const Value& value);
		Status ResolveBufferField(stduint object, Frame* frame, Value& source, uint64& bit_offset, uint64& bit_length);
		Status ReadBufferField(stduint object, Frame* frame, Value& value);
		Status WriteBufferField(stduint object, Frame* frame, const Value& value);
		Status EvaluateTerm(const uint8*& cursor, const uint8* end, stduint scope, Frame* frame, Value& value);
		Status ExecuteMethod(stduint object, const Value* arguments, stduint argument_count, Value& value);
		Status ExecuteBlock(const uint8*& cursor, const uint8* end, stduint scope, Frame* frame, Value& value, bool& returned, bool& broke, bool& continued);
		Status DefineOperationRegion(const uint8* address, const uint8*& cursor, const uint8* end, stduint scope, Frame* frame);
		Status DefineField(const uint8* address, const uint8*& cursor, const uint8* end, stduint scope);
		Status DefineFieldList(const uint8* address, const uint8* begin, const uint8* end, stduint scope, stduint region, uint8 field_flags);
		Status DefineBufferField(const uint8* address, const uint8*& cursor, const uint8* end, stduint scope, Frame* frame, uint16 opcode);
		Status ParseTarget(const uint8*& cursor, const uint8* end, stduint scope, Frame* frame, Target& target);
		Status ReadTarget(const Target& target, Value& value);
		Status StoreTarget(const Target& target, const Value& value);
		Status ResolveMutex(const Target& target, stduint& object);
		Status AcquireMutex(stduint object, uint8 sync_level, const uint8* address);
		Status ReleaseMutex(stduint object, const uint8* address);
		void RestoreMutex(stduint top);
		const AMLNamespace::Object* AtObject(stduint object) const;
		AMLNamespace::Object* AtDynamicObject(stduint object);
		stduint FindChildObject(stduint parent, const char name[5]) const;
		stduint FindObject(stduint scope, const AMLBytecode::Name& name) const;
		stduint DefineDynamicObject(stduint scope, const AMLBytecode::Name& name, AMLNamespace::ObjectType type, const uint8* value, stduint value_length);
		void DiscardDynamicObjects(stduint count);
		Status ReadReference(const Value& reference, Value& value);
		Status ReadStoredObject(stduint object, Value& value) const;
		Status StoreObject(stduint object, const Value& value);
		Status RequireInteger(const Value& value, uint64& integer, const uint8* address);
		Status AllocateBuffer(stduint length, ValueType type, Value& value);
		Status AllocatePackage(stduint count, Value& value);
		Status CopyValue(const Value& source, Value& value, const uint8* address);
		Status ImportValue(const Value& source, Value& value, const uint8* address, stduint depth = 0);
		Status NormalizeValue(const Value& source, Value& value, const uint8* address, stduint depth = 0);
		Status ExportValue(const Value& source, Value& value, const ValueStorage& value_storage, stduint& value_used, stduint& byte_used, stduint depth = 0);
		Status ResourceTemplatePayloadLength(const Value& source, stduint& length, const uint8* address);
		Status EvaluateBinary(const uint8*& cursor, const uint8* end, stduint scope, Frame* frame, uint8 opcode, Value& value);
		bool PackageRange(const uint8* length_at, const uint8* limit, const uint8*& content, const uint8*& object_end) const;
	};

	struct ACPIUUID {
		uint32 data1;
		uint16 data2;
		uint16 data3;
		uint8 data4[8];

		void Encode(uint8 output[16]) const;
	};

	class ACPIDSM {
	public:
		enum class Status : uint8 {
			Success,
			InvalidArgument,
			MethodNotFound,
			MalformedMethod,
			UUIDUnsupported,
			FunctionUnsupported,
			MalformedFunctionMask,
			ReturnTypeMismatch,
			ExecutionFailed,
		};

		enum class ReturnType : uint8 {
			Any,
			Integer,
			Buffer,
			Package,
		};

		struct Result {
			AMLExecutor::Value value;
			AMLExecutor::Status aml_status;
			Status status;
		};

		struct Workspace {
			AMLExecutor::Value arguments[4];
			AMLExecutor::Value query_values[16];
			uint8 uuid[16];
			uint8 query_bytes[64];
			Result query_result;
		};

		ACPIDSM(const AMLNamespace& aml_namespace, AMLExecutor& aml_executor);

		Status Query(stduint device, const ACPIUUID& uuid, uint64 revision, Result& result, Workspace& workspace, const AMLExecutor::ValueStorage& value_storage);
		Status Invoke(stduint device, const ACPIUUID& uuid, uint64 revision, uint64 function, const AMLExecutor::Value* parameters, ReturnType return_type, Result& result, Workspace& workspace, const AMLExecutor::ValueStorage& value_storage);

	private:
		const AMLNamespace& aml_namespace;
		AMLExecutor& aml_executor;

		Status ResolveMethod(stduint device, stduint& method) const;
		void PrepareArguments(const ACPIUUID& uuid, uint64 revision, uint64 function, const AMLExecutor::Value* parameters, Workspace& workspace) const;
		bool Accepts(ReturnType return_type, AMLExecutor::ValueType value_type) const;
	};

	class ACPIDSD {
	public:
		enum class Status : uint8 {
			Success,
			InvalidArgument,
			ObjectNotFound,
			MalformedObject,
			DevicePropertiesNotFound,
			PropertyNotFound,
			MalformedPackage,
			DuplicateDescriptor,
			DuplicateProperty,
			UnsupportedPropertyType,
			ExecutionFailed,
		};

		enum class ValueType : uint8 {
			Integer,
			String,
			Buffer,
			Array,
			DeviceReference,
		};

		enum ValueFlags : uint8 {
			ValueFlagNone = 0,
			ValueFlagNonStandard = 1 << 0,
		};

		struct Value {
			const uint8* data;
			const AMLExecutor::Value* elements;
			uint64 integer;
			stduint length;
			stduint element_count;
			stduint reference;
			ValueType type;
			uint8 flags;
		};

		struct Property {
			const uint8* name;
			stduint name_length;
			Value value;
		};

		struct Result {
			AMLExecutor::Value value;
			const AMLExecutor::Value* properties;
			stduint property_count;
			stduint nonstandard_count;
			AMLExecutor::Status aml_status;
			Status status;
		};

		ACPIDSD(const AMLNamespace& aml_namespace, AMLExecutor& aml_executor);

		Status Evaluate(stduint device, Result& result, const AMLExecutor::ValueStorage& value_storage);
		Status PropertyAt(const Result& result, stduint index, Property& property) const;
		Status Find(const Result& result, const char* name, Property& property) const;
		Status ArrayAt(const Value& array, stduint index, Value& value) const;

	private:
		const AMLNamespace& aml_namespace;
		AMLExecutor& aml_executor;

		Status ConvertValue(const AMLExecutor::Value& source, Value& value, stduint depth, bool& nonstandard) const;
		Status ValidateValue(const AMLExecutor::Value& value, stduint depth, bool& nonstandard) const;
		bool EqualUuid(const AMLExecutor::Value& value, const uint8 uuid[16]) const;
		bool EqualString(const AMLExecutor::Value& value, const char* text) const;
	};

}

#endif
