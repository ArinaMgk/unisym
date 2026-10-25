// UTF-8 CPP-ISO11 TAB4 CRLF
// Docutitle: ACPI Machine Language Executor
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../inc/cpp/Device/ACPI/AML.hpp"

namespace uni {

	namespace {

		AMLExecutor::Value aml_integer(uint64 integer) {
			AMLExecutor::Value value = {};
			value.type = AMLExecutor::ValueType::Integer;
			value.integer = integer;
			return value;
		}

		uint16 aml_object_opcode(const AMLNamespace::Object& object) {
			if (!object.value || !object.value_length) return 0;
			if (object.value[0] == 0x5B && object.value_length >= 2) return uint16(0x5B00u | object.value[1]);
			return object.value[0];
		}

	}

	AMLExecutor::AMLExecutor(const AMLNamespace& namespace_value, const Workspace& workspace_value, const Limits& limits_value) : aml_namespace(namespace_value), workspace(workspace_value), limits(limits_value) {
		Reset();
	}

	void AMLExecutor::Reset() {
		frame_count = 0;
		instruction_count = 0;
		loop_count = 0;
		object_depth = 0;
		dynamic_object_count = 0;
		mutex_top = AMLNamespace::InvalidIndex;
		buffer_used = 0;
		last_error = {};
		last_error.object = AMLNamespace::InvalidIndex;
		last_error.scope = AMLNamespace::InvalidIndex;
		if (workspace.object_values) for0(index, workspace.object_value_capacity) workspace.object_values[index] = {};
		if (workspace.dynamic_objects) for0(index, workspace.dynamic_object_capacity) workspace.dynamic_objects[index] = {};
		if (workspace.mutex_states) for0(index, workspace.mutex_capacity) workspace.mutex_states[index] = {};
	}

	const AMLExecutor::Error& AMLExecutor::LastError() const {
		return last_error;
	}

	AMLExecutor::Status AMLExecutor::Fail(Status status, const uint8* address, stduint object, uint16 opcode) {
		last_error.address = address;
		last_error.object = object;
		last_error.scope = AMLNamespace::InvalidIndex;
		last_error.opcode = opcode ? opcode : address ? *address : 0;
		last_error.status = status;
		return status;
	}

	AMLExecutor::Status AMLExecutor::ConsumeInstruction(const uint8* address) {
		if (++instruction_count > limits.instruction_count) return Fail(Status::InstructionLimit, address);
		return Status::Success;
	}

	bool AMLExecutor::PackageRange(const uint8* length_at, const uint8* limit, const uint8*& content, const uint8*& object_end) const {
		uint32 package_length = 0;
		uint8 encoded_length = 0;
		if (!length_at || length_at >= limit || !AMLBytecode::DecodePackageLength(length_at, stduint(limit - length_at), package_length, encoded_length) || package_length > stduint(limit - length_at)) return false;
		content = length_at + encoded_length;
		object_end = length_at + package_length;
		return content <= object_end;
	}

	const AMLNamespace::Object* AMLExecutor::AtObject(stduint object) const {
		const stduint namespace_count = aml_namespace.Count();
		if (object < namespace_count) return aml_namespace.At(object);
		const stduint dynamic_index = object - namespace_count;
		return workspace.dynamic_objects && dynamic_index < dynamic_object_count ? workspace.dynamic_objects + dynamic_index : nullptr;
	}

	AMLNamespace::Object* AMLExecutor::AtDynamicObject(stduint object) {
		const stduint namespace_count = aml_namespace.Count();
		if (object < namespace_count) return nullptr;
		const stduint dynamic_index = object - namespace_count;
		return workspace.dynamic_objects && dynamic_index < dynamic_object_count ? workspace.dynamic_objects + dynamic_index : nullptr;
	}

	stduint AMLExecutor::FindChildObject(stduint parent, const char name[5]) const {
		if (!name || !AtObject(parent)) return AMLNamespace::InvalidIndex;
		for0(index, dynamic_object_count) {
			const AMLNamespace::Object& candidate = workspace.dynamic_objects[index];
			if (candidate.parent != parent) continue;
			bool equal = true;
			for0(character, 4) equal &= candidate.name[character] == name[character];
			if (equal) return aml_namespace.Count() + index;
		}
		return parent < aml_namespace.Count() ? aml_namespace.FindChild(parent, name) : AMLNamespace::InvalidIndex;
	}

	stduint AMLExecutor::FindObject(stduint scope, const AMLBytecode::Name& name) const {
		if (!AtObject(scope)) return AMLNamespace::InvalidIndex;
		stduint base = name.rooted ? 0 : scope;
		for (uint8 prefix = 0; prefix < name.parent_count; ++prefix) {
			if (!base) break;
			const AMLNamespace::Object* object = AtObject(base);
			if (!object) return AMLNamespace::InvalidIndex;
			base = object->parent;
		}
		if (!name.segment_count) return base;
		const bool search_parents = !name.rooted && !name.parent_count;
		while (true) {
			stduint object = base;
			for (uint8 segment = 0; segment < name.segment_count && object != AMLNamespace::InvalidIndex; ++segment) object = FindChildObject(object, name.segments[segment]);
			if (object != AMLNamespace::InvalidIndex) return object;
			if (!search_parents || !base) break;
			const AMLNamespace::Object* parent = AtObject(base);
			if (!parent) break;
			base = parent->parent;
		}
		return AMLNamespace::InvalidIndex;
	}

	stduint AMLExecutor::DefineDynamicObject(stduint scope, const AMLBytecode::Name& name, AMLNamespace::ObjectType type, const uint8* value, stduint value_length) {
		if (!workspace.dynamic_objects || !name.segment_count || dynamic_object_count >= workspace.dynamic_object_capacity || aml_namespace.Count() > ~stduint(0) - dynamic_object_count) return AMLNamespace::InvalidIndex;
		stduint parent = name.rooted ? 0 : scope;
		for (uint8 prefix = 0; prefix < name.parent_count; ++prefix) {
			if (!parent) break;
			const AMLNamespace::Object* object = AtObject(parent);
			if (!object) return AMLNamespace::InvalidIndex;
			parent = object->parent;
		}
		for (uint8 segment = 0; segment + 1 < name.segment_count; ++segment) {
			parent = FindChildObject(parent, name.segments[segment]);
			if (parent == AMLNamespace::InvalidIndex) return parent;
		}
		if (FindChildObject(parent, name.segments[name.segment_count - 1]) != AMLNamespace::InvalidIndex) return AMLNamespace::InvalidIndex;
		AMLNamespace::Object& object = workspace.dynamic_objects[dynamic_object_count];
		object = {};
		object.value = value;
		object.value_length = value_length;
		object.parent = parent;
		object.reference = AMLNamespace::InvalidIndex;
		object.secondary_reference = AMLNamespace::InvalidIndex;
		for0(character, 4) object.name[character] = name.segments[name.segment_count - 1][character];
		object.name[4] = 0;
		object.type = type;
		object.flags = AMLNamespace::ObjectFlagRuntime;
		return aml_namespace.Count() + dynamic_object_count++;
	}

	void AMLExecutor::DiscardDynamicObjects(stduint count) {
		if (count > dynamic_object_count) count = dynamic_object_count;
		const stduint first_object = aml_namespace.Count() + count;
		const stduint object_end = aml_namespace.Count() + dynamic_object_count;
		if (workspace.object_values) for0(index, workspace.object_value_capacity) {
			ObjectValue& entry = workspace.object_values[index];
			if (entry.valid && entry.object >= first_object && entry.object < object_end) entry = {};
		}
		if (workspace.dynamic_objects) for (stduint index = count; index < dynamic_object_count; ++index) workspace.dynamic_objects[index] = {};
		dynamic_object_count = count;
	}

	AMLExecutor::Status AMLExecutor::ReadStoredObject(stduint object, Value& value) const {
		if (!workspace.object_values) return Status::NotFound;
		for0(index, workspace.object_value_capacity) {
			if (workspace.object_values[index].valid && workspace.object_values[index].object == object) {
				value = workspace.object_values[index].value;
				return Status::Success;
			}
		}
		return Status::NotFound;
	}

	AMLExecutor::Status AMLExecutor::StoreObject(stduint object, const Value& value) {
		if (!workspace.object_values) return Fail(Status::ObjectValueCapacity, nullptr, object);
		stduint free_index = AMLNamespace::InvalidIndex;
		for0(index, workspace.object_value_capacity) {
			if (workspace.object_values[index].valid && workspace.object_values[index].object == object) {
				workspace.object_values[index].value = value;
				return Status::Success;
			}
			if (!workspace.object_values[index].valid && free_index == AMLNamespace::InvalidIndex) free_index = index;
		}
		if (free_index == AMLNamespace::InvalidIndex) return Fail(Status::ObjectValueCapacity, nullptr, object);
		workspace.object_values[free_index].object = object;
		workspace.object_values[free_index].value = value;
		workspace.object_values[free_index].valid = true;
		return Status::Success;
	}

	AMLExecutor::Status AMLExecutor::RequireInteger(const Value& value, uint64& integer, const uint8* address) {
		if (value.type != ValueType::Integer) return Fail(Status::TypeMismatch, address);
		integer = value.integer;
		return Status::Success;
	}

	AMLExecutor::Status AMLExecutor::AllocateBuffer(stduint length, ValueType type, Value& value) {
		if (type != ValueType::Buffer && type != ValueType::String) return Fail(Status::InvalidArgument, nullptr);
		const stduint allocation_length = length + (type == ValueType::String ? 1 : 0);
		if (allocation_length < length || buffer_used > workspace.buffer_capacity || allocation_length > workspace.buffer_capacity - buffer_used || (allocation_length && !workspace.buffer_storage)) return Fail(Status::BufferCapacity, nullptr);
		uint8* data = allocation_length ? workspace.buffer_storage + buffer_used : workspace.buffer_storage;
		buffer_used += allocation_length;
		for0(index, allocation_length) data[index] = 0;
		value = {};
		value.type = type;
		value.data = data;
		value.writable_data = data;
		value.length = length;
		return Status::Success;
	}

	AMLExecutor::Status AMLExecutor::CopyValue(const Value& source, Value& value, const uint8* address) {
		if (source.type != ValueType::Buffer && source.type != ValueType::String) {
			value = source;
			return Status::Success;
		}
		if (source.length && !source.data) return Fail(Status::Malformed, address);
		Status status = AllocateBuffer(source.length, source.type, value);
		if (status != Status::Success) {
			last_error.address = address;
			return status;
		}
		for0(index, source.length) value.writable_data[index] = source.data[index];
		value.data_length = source.data_length < source.length ? source.data_length : source.length;
		return Status::Success;
	}

	AMLExecutor::Status AMLExecutor::ResourceTemplatePayloadLength(const Value& source, stduint& length, const uint8* address) {
		length = 0;
		if (source.type != ValueType::Buffer) return Fail(Status::TypeMismatch, address);
		if (!source.length) return Status::Success;
		if (source.length == 1 || !source.data) return Fail(Status::Malformed, address);
		const uint8* cursor = source.data;
		const uint8* end = source.data + source.length;
		while (cursor < end) {
			const uint8* descriptor = cursor;
			stduint descriptor_length = 0;
			if (*cursor & 0x80u) {
				if (end - cursor < 3) return Fail(Status::Malformed, address);
				descriptor_length = stduint(cursor[1]) | stduint(cursor[2]) << 8;
				descriptor_length += 3;
			}
			else descriptor_length = stduint(*cursor & 7u) + 1;
			if (!descriptor_length || descriptor_length > stduint(end - cursor)) return Fail(Status::Malformed, address);
			if (!(*cursor & 0x80u) && ((*cursor >> 3) & 0x0Fu) == 0x0Fu) {
				if (descriptor_length != 2) return Fail(Status::Malformed, address);
				length = stduint(descriptor - source.data);
				return Status::Success;
			}
			cursor += descriptor_length;
		}
		return Fail(Status::Malformed, address);
	}

	AMLExecutor::Status AMLExecutor::ReadReference(const Value& reference, Value& value) {
		if (reference.type != ValueType::Reference) return Fail(Status::TypeMismatch, nullptr);
		if (reference.reference_type == ReferenceType::Object) return EvaluateObject(reference.reference, nullptr, value);
		if (!workspace.frames || reference.reference_frame >= frame_count) return Fail(Status::InvalidTarget, nullptr);
		Frame& frame = workspace.frames[reference.reference_frame];
		if (reference.reference_type == ReferenceType::Local && reference.reference < numsof(frame.locals)) {
			value = frame.locals[reference.reference];
			return Status::Success;
		}
		if (reference.reference_type == ReferenceType::Argument && reference.reference < numsof(frame.arguments)) {
			value = frame.arguments[reference.reference];
			return Status::Success;
		}
		return Fail(Status::InvalidTarget, nullptr);
	}

	AMLExecutor::Status AMLExecutor::ReadTarget(const Target& target, Value& value) {
		if (target.type == TargetType::Null) {
			value = {};
			return Status::Success;
		}
		if (target.type == TargetType::Object) return EvaluateObject(target.index, nullptr, value);
		if (!workspace.frames || target.frame >= frame_count) return Fail(Status::InvalidTarget, nullptr);
		Frame& frame = workspace.frames[target.frame];
		if (target.type == TargetType::Local && target.index < numsof(frame.locals)) {
			value = frame.locals[target.index];
			return Status::Success;
		}
		if (target.type == TargetType::Argument && target.index < numsof(frame.arguments)) {
			value = frame.arguments[target.index];
			return Status::Success;
		}
		return Fail(Status::InvalidTarget, nullptr);
	}

	AMLExecutor::Status AMLExecutor::StoreTarget(const Target& target, const Value& value) {
		if (target.type == TargetType::Null) return Status::Success;
		if (target.type == TargetType::Object) {
			stduint object_index = target.index;
			for0(depth, AMLNamespace::MaximumDepth) {
				const AMLNamespace::Object* object = AtObject(object_index);
				if (!object) return Fail(Status::InvalidTarget, nullptr, object_index);
				if (object->flags & AMLNamespace::ObjectFlagConditional) return Fail(Status::ConditionalObject, object->value, object_index);
				if (object->type != AMLNamespace::ObjectType::Alias) {
					if (object->type == AMLNamespace::ObjectType::FieldUnit) return WriteField(object_index, frame_count ? workspace.frames + frame_count - 1 : nullptr, value);
					if (object->type == AMLNamespace::ObjectType::BufferField) return WriteBufferField(object_index, frame_count ? workspace.frames + frame_count - 1 : nullptr, value);
					if (object->type != AMLNamespace::ObjectType::Name) return Fail(Status::InvalidTarget, object->value, object_index);
					Value original = {};
					const Status status = EvaluateObject(object_index, nullptr, original);
					if (status != Status::Success) return status;
					if (original.type != ValueType::Uninitialized && original.type != value.type) return Fail(Status::TypeMismatch, object->value, object_index);
					return StoreObject(object_index, value);
				}
				if (object->reference != AMLNamespace::InvalidIndex) object_index = object->reference;
				else {
					const uint8* source = object->value;
					AMLBytecode::Name name = {};
					if (!AMLBytecode::DecodeName(source, object->value + object->value_length, name)) return Fail(Status::Malformed, object->value, object_index);
					object_index = FindObject(object->parent, name);
					if (object_index == AMLNamespace::InvalidIndex) return Fail(Status::NotFound, object->value, target.index);
				}
			}
			return Fail(Status::FrameLimit, AtObject(object_index) ? AtObject(object_index)->value : nullptr, object_index);
		}
		if (!workspace.frames || target.frame >= frame_count) return Fail(Status::InvalidTarget, nullptr);
		Frame& frame = workspace.frames[target.frame];
		if (target.type == TargetType::Local && target.index < numsof(frame.locals)) {
			frame.locals[target.index] = value;
			return Status::Success;
		}
		if (target.type == TargetType::Argument && target.index < numsof(frame.arguments)) {
			frame.arguments[target.index] = value;
			return Status::Success;
		}
		return Fail(Status::InvalidTarget, nullptr);
	}

	AMLExecutor::Status AMLExecutor::ResolveMutex(const Target& target, stduint& object_index) {
		if (target.type == TargetType::Object) object_index = target.index;
		else {
			Value reference = {};
			Status status = ReadTarget(target, reference);
			if (status != Status::Success) return status;
			if (reference.type != ValueType::Reference || reference.reference_type != ReferenceType::Object) return Fail(Status::TypeMismatch, nullptr);
			object_index = reference.reference;
		}
		for0(depth, AMLNamespace::MaximumDepth) {
			const AMLNamespace::Object* object = AtObject(object_index);
			if (!object) return Fail(Status::NotFound, nullptr, object_index);
			if (object->flags & AMLNamespace::ObjectFlagConditional) return Fail(Status::ConditionalObject, object->value, object_index);
			if (object->type == AMLNamespace::ObjectType::Mutex) {
				if (!object->value || object->value_length != 1 || (object->value[0] & 0xF0u)) return Fail(Status::Malformed, object->value, object_index);
				return Status::Success;
			}
			if (object->type != AMLNamespace::ObjectType::Alias) return Fail(Status::TypeMismatch, object->value, object_index);
			if (object->reference != AMLNamespace::InvalidIndex) object_index = object->reference;
			else {
				if (!object->value) return Fail(Status::Malformed, nullptr, object_index);
				const uint8* source = object->value;
				AMLBytecode::Name name = {};
				if (!AMLBytecode::DecodeName(source, object->value + object->value_length, name)) return Fail(Status::Malformed, object->value, object_index);
				object_index = FindObject(object->parent, name);
				if (object_index == AMLNamespace::InvalidIndex) return Fail(Status::NotFound, object->value, target.index);
			}
		}
		return Fail(Status::FrameLimit, nullptr, object_index);
	}

	AMLExecutor::Status AMLExecutor::AcquireMutex(stduint object, uint8 sync_level, const uint8* address) {
		if (!workspace.mutex_states) return Fail(Status::MutexCapacity, address, object);
		uint8 current_level = 0;
		bool already_owned = false;
		for (stduint entry = mutex_top; entry != AMLNamespace::InvalidIndex; entry = workspace.mutex_states[entry].previous) {
			if (entry >= workspace.mutex_capacity || !workspace.mutex_states[entry].valid) return Fail(Status::Malformed, address, object);
			if (workspace.mutex_states[entry].object == object) already_owned = true;
			if (workspace.mutex_states[entry].sync_level > current_level) current_level = workspace.mutex_states[entry].sync_level;
		}
		if (!already_owned && sync_level < current_level) return Fail(Status::InvalidArgument, address, object);
		// Each held acquisition occupies one workspace slot so recursive ownership retains release order.
		for0(index, workspace.mutex_capacity) {
			MutexState& entry = workspace.mutex_states[index];
			if (entry.valid) continue;
			entry = {};
			entry.valid = true;
			entry.object = object;
			entry.previous = mutex_top;
			entry.sync_level = sync_level;
			mutex_top = index;
			return Status::Success;
		}
		return Fail(Status::MutexCapacity, address, object);
	}

	AMLExecutor::Status AMLExecutor::ReleaseMutex(stduint object, const uint8* address) {
		if (mutex_top == AMLNamespace::InvalidIndex || !workspace.mutex_states || mutex_top >= workspace.mutex_capacity || !workspace.mutex_states[mutex_top].valid || workspace.mutex_states[mutex_top].object != object) return Fail(Status::InvalidArgument, address, object);
		MutexState& entry = workspace.mutex_states[mutex_top];
		mutex_top = entry.previous;
		entry = {};
		return Status::Success;
	}

	void AMLExecutor::RestoreMutex(stduint top) {
		while (mutex_top != top && mutex_top != AMLNamespace::InvalidIndex && workspace.mutex_states && mutex_top < workspace.mutex_capacity) {
			MutexState& entry = workspace.mutex_states[mutex_top];
			mutex_top = entry.previous;
			entry = {};
		}
	}

	AMLExecutor::Status AMLExecutor::ParseTarget(const uint8*& cursor, const uint8* end, stduint scope, Frame* frame, Target& target) {
		target = {};
		target.type = TargetType::Null;
		if (!cursor || cursor >= end) return Fail(Status::Malformed, nullptr);
		if (*cursor == 0x00) {
			++cursor;
			return Status::Success;
		}
		if (*cursor >= 0x60 && *cursor <= 0x67) {
			if (!frame) return Fail(Status::InvalidTarget, cursor);
			target.type = TargetType::Local;
			target.index = *cursor++ - 0x60;
			target.frame = stduint(frame - workspace.frames);
			return Status::Success;
		}
		if (*cursor >= 0x68 && *cursor <= 0x6E) {
			if (!frame) return Fail(Status::InvalidTarget, cursor);
			target.type = TargetType::Argument;
			target.index = *cursor++ - 0x68;
			target.frame = stduint(frame - workspace.frames);
			return Status::Success;
		}
		if (end - cursor >= 2 && cursor[0] == 0x5B && cursor[1] == 0x31) {
			cursor += 2;
			return Status::Success;
		}
		const uint8* address = cursor;
		AMLBytecode::Name name = {};
		const uint8* name_end = cursor;
		if (!AMLBytecode::DecodeName(name_end, end, name)) return Fail(Status::InvalidTarget, cursor);
		const stduint object = FindObject(scope, name);
		cursor = name_end;
		if (object == AMLNamespace::InvalidIndex) {
			const Status status = Fail(Status::NotFound, address);
			last_error.scope = scope;
			return status;
		}
		const AMLNamespace::Object* namespace_object = AtObject(object);
		if (!namespace_object) return Fail(Status::NotFound, address, object);
		if (namespace_object->flags & AMLNamespace::ObjectFlagConditional) return Fail(Status::ConditionalObject, namespace_object->value, object);
		target.type = TargetType::Object;
		target.index = object;
		return Status::Success;
	}

	AMLExecutor::Status AMLExecutor::ResolveFieldWidth(const AMLNamespace::Object& field, stduint object_index, uint16 field_opcode, uint8& width) {
		width = 0;
		if (!field.bit_length || field.bit_length > 64) return Fail(Status::UnsupportedObject, field.value, object_index, field_opcode);
		switch (field.field_flags & 0x0Fu) {
		case 0: case 1: width = 8; break;
		case 2: width = 16; break;
		case 3: width = 32; break;
		case 4: width = 64; break;
		default: return Fail(Status::UnsupportedObject, field.value, object_index, field_opcode);
		}
		return Status::Success;
	}

	AMLExecutor::Status AMLExecutor::ResolveField(stduint object_index, Frame* frame, FieldAccess& access) {
		access = {};
		const AMLNamespace::Object* field = AtObject(object_index);
		if (!field || field->type != AMLNamespace::ObjectType::FieldUnit) return Fail(Status::UnsupportedObject, field ? field->value : nullptr, object_index);
		const uint16 field_opcode = aml_object_opcode(*field);
		if (!field->value || field->value_length < 2 || field->value[0] != 0x5B || field->value[1] != 0x81) return Fail(Status::UnsupportedObject, field->value, object_index, field_opcode);
		uint8 access_width = 0;
		Status status = ResolveFieldWidth(*field, object_index, field_opcode, access_width);
		if (status != Status::Success) return status;
		const AMLNamespace::Object* region = AtObject(field->reference);
		if (!region || region->type != AMLNamespace::ObjectType::OperationRegion || !region->value || !region->value_length) return Fail(Status::RegionUnavailable, field->value, object_index, field_opcode);
		if (region->flags & AMLNamespace::ObjectFlagConditional) return Fail(Status::ConditionalObject, region->value, field->reference);
		uint8 space = 0;
		uint64 region_offset = 0;
		uint64 region_length = 0;
		if (region->flags & AMLNamespace::ObjectFlagRuntime) {
			space = region->external_type;
			region_offset = region->bit_offset;
			region_length = region->bit_length;
		}
		else {
			space = region->value[0];
			const uint8* cursor = region->value + 1;
			const uint8* end = region->value + region->value_length;
			Value region_offset_value = {};
			status = EvaluateTerm(cursor, end, region->parent, frame, region_offset_value);
			if (status != Status::Success) return status;
			Value region_length_value = {};
			status = EvaluateTerm(cursor, end, region->parent, frame, region_length_value);
			if (status != Status::Success) return status;
			if ((status = RequireInteger(region_offset_value, region_offset, region->value)) != Status::Success) return status;
			if ((status = RequireInteger(region_length_value, region_length, region->value)) != Status::Success) return status;
		}
		if (field->bit_offset > ~uint64(0) - field->bit_length) return Fail(Status::Malformed, field->value, object_index, field_opcode);
		const uint64 field_end = field->bit_offset + field->bit_length;
		const uint64 required_bytes = field_end / 8 + (field_end & 7u ? 1 : 0);
		if (required_bytes > region_length) return Fail(Status::RegionUnavailable, field->value, object_index, field_opcode);
		access.field = field;
		access.region_object = field->reference;
		access.region_scope = region->parent;
		access.region_offset = region_offset;
		access.region_length = region_length;
		access.field_opcode = field_opcode;
		access.space = space;
		access.width = access_width;
		return Status::Success;
	}

	AMLExecutor::Status AMLExecutor::ReadField(stduint object_index, Frame* frame, Value& value) {
		const AMLNamespace::Object* field = AtObject(object_index);
		if (!field || field->type != AMLNamespace::ObjectType::FieldUnit) return Fail(Status::UnsupportedObject, field ? field->value : nullptr, object_index);
		if (aml_object_opcode(*field) == 0x5B86) return ReadIndexField(object_index, frame, value);
		FieldAccess access = {};
		Status status = ResolveField(object_index, frame, access);
		if (status != Status::Success) return status;
		if (!workspace.region_access.read) return Fail(Status::RegionUnavailable, access.field->value, object_index, access.field_opcode);
		const uint64 field_end = access.field->bit_offset + access.field->bit_length;
		const uint64 access_bytes = access.width / 8;
		uint64 bit = access.field->bit_offset;
		uint64 result_bit = 0;
		uint64 result = 0;
		while (bit < field_end) {
			const uint64 access_index = bit / access.width;
			if (access_index > ~uint64(0) / access_bytes) return Fail(Status::Malformed, access.field->value, object_index, access.field_opcode);
			const uint64 access_offset = access_index * access_bytes;
			if (access_offset > access.region_length || access_bytes > access.region_length - access_offset || access.region_offset > ~uint64(0) - access_offset) return Fail(Status::RegionUnavailable, access.field->value, object_index, access.field_opcode);
			uint64 access_value = 0;
			if (!workspace.region_access.read(workspace.region_access.context, aml_namespace, access.region_object, access.region_scope, access.space, access.region_offset + access_offset, access.width, access_value)) return Fail(Status::RegionUnavailable, access.field->value, object_index, access.field_opcode);
			const uint8 access_bit = uint8(bit % access.width);
			uint64 take = access.width - access_bit;
			if (take > field_end - bit) take = field_end - bit;
			const uint64 mask = take == 64 ? ~uint64(0) : (uint64(1) << take) - 1;
			const uint64 piece = (access_value >> access_bit) & mask;
			result |= result_bit ? piece << result_bit : piece;
			bit += take;
			result_bit += take;
		}
		value = aml_integer(result);
		return Status::Success;
	}

	AMLExecutor::Status AMLExecutor::WriteField(stduint object_index, Frame* frame, const Value& value) {
		if (value.type != ValueType::Integer) return Fail(Status::TypeMismatch, AtObject(object_index) ? AtObject(object_index)->value : nullptr, object_index);
		const AMLNamespace::Object* field = AtObject(object_index);
		if (!field || field->type != AMLNamespace::ObjectType::FieldUnit) return Fail(Status::UnsupportedObject, field ? field->value : nullptr, object_index);
		if (aml_object_opcode(*field) == 0x5B86) return WriteIndexField(object_index, frame, value);
		FieldAccess access = {};
		Status status = ResolveField(object_index, frame, access);
		if (status != Status::Success) return status;
		if (!workspace.region_access.write) return Fail(Status::RegionUnavailable, access.field->value, object_index, access.field_opcode);
		const uint8 update_rule = uint8((access.field->field_flags >> 5) & 3u);
		if (update_rule == 3) return Fail(Status::UnsupportedObject, access.field->value, object_index, access.field_opcode);
		const uint64 field_end = access.field->bit_offset + access.field->bit_length;
		const uint64 access_bytes = access.width / 8;
		uint64 bit = access.field->bit_offset;
		uint64 source_bit = 0;
		while (bit < field_end) {
			const uint64 access_index = bit / access.width;
			if (access_index > ~uint64(0) / access_bytes) return Fail(Status::Malformed, access.field->value, object_index, access.field_opcode);
			const uint64 access_offset = access_index * access_bytes;
			if (access_offset > access.region_length || access_bytes > access.region_length - access_offset || access.region_offset > ~uint64(0) - access_offset) return Fail(Status::RegionUnavailable, access.field->value, object_index, access.field_opcode);
			const uint8 access_bit = uint8(bit % access.width);
			uint64 take = access.width - access_bit;
			if (take > field_end - bit) take = field_end - bit;
			const uint64 piece_mask = take == 64 ? ~uint64(0) : (uint64(1) << take) - 1;
			const uint64 write_mask = take == 64 ? ~uint64(0) : piece_mask << access_bit;
			uint64 access_value = 0;
			if (write_mask != (access.width == 64 ? ~uint64(0) : (uint64(1) << access.width) - 1)) {
				if (!update_rule) {
					if (!workspace.region_access.read || !workspace.region_access.read(workspace.region_access.context, aml_namespace, access.region_object, access.region_scope, access.space, access.region_offset + access_offset, access.width, access_value)) return Fail(Status::RegionUnavailable, access.field->value, object_index, access.field_opcode);
				}
				else if (update_rule == 1) access_value = ~uint64(0);
			}
			const uint64 piece = source_bit ? (value.integer >> source_bit) & piece_mask : value.integer & piece_mask;
			access_value = (access_value & ~write_mask) | (piece << access_bit);
			if (!workspace.region_access.write(workspace.region_access.context, aml_namespace, access.region_object, access.region_scope, access.space, access.region_offset + access_offset, access.width, access_value)) return Fail(Status::RegionUnavailable, access.field->value, object_index, access.field_opcode);
			bit += take;
			source_bit += take;
		}
		return Status::Success;
	}

	AMLExecutor::Status AMLExecutor::ReadIndexField(stduint object_index, Frame* frame, Value& value) {
		const AMLNamespace::Object* field = AtObject(object_index);
		const uint16 field_opcode = field ? aml_object_opcode(*field) : 0x5B86;
		if (!field || field->type != AMLNamespace::ObjectType::FieldUnit || field_opcode != 0x5B86) return Fail(Status::UnsupportedObject, field ? field->value : nullptr, object_index, field_opcode);
		if (field->reference == AMLNamespace::InvalidIndex || field->secondary_reference == AMLNamespace::InvalidIndex || !AtObject(field->reference) || !AtObject(field->secondary_reference)) return Fail(Status::NotFound, field->value, object_index, field_opcode);
		if (field->reference == object_index || field->secondary_reference == object_index) return Fail(Status::Malformed, field->value, object_index, field_opcode);
		uint8 access_width = 0;
		Status status = ResolveFieldWidth(*field, object_index, field_opcode, access_width);
		if (status != Status::Success) return status;
		if (field->bit_offset > ~uint64(0) - field->bit_length) return Fail(Status::Malformed, field->value, object_index, field_opcode);
		const uint64 field_end = field->bit_offset + field->bit_length;
		const uint64 access_bytes = access_width / 8;
		Target index_target = {};
		index_target.type = TargetType::Object;
		index_target.index = field->reference;
		uint64 bit = field->bit_offset;
		uint64 result_bit = 0;
		uint64 result = 0;
		while (bit < field_end) {
			const uint64 access_index = bit / access_width;
			if (access_index > ~uint64(0) / access_bytes) return Fail(Status::Malformed, field->value, object_index, field_opcode);
			status = StoreTarget(index_target, aml_integer(access_index * access_bytes));
			if (status != Status::Success) return status;
			Value data_value = {};
			status = EvaluateObject(field->secondary_reference, frame, data_value);
			if (status != Status::Success) return status;
			uint64 access_value = 0;
			if ((status = RequireInteger(data_value, access_value, field->value)) != Status::Success) return status;
			const uint8 access_bit = uint8(bit % access_width);
			uint64 take = access_width - access_bit;
			if (take > field_end - bit) take = field_end - bit;
			const uint64 mask = take == 64 ? ~uint64(0) : (uint64(1) << take) - 1;
			const uint64 piece = (access_value >> access_bit) & mask;
			result |= result_bit ? piece << result_bit : piece;
			bit += take;
			result_bit += take;
		}
		value = aml_integer(result);
		return Status::Success;
	}

	AMLExecutor::Status AMLExecutor::WriteIndexField(stduint object_index, Frame* frame, const Value& value) {
		const AMLNamespace::Object* field = AtObject(object_index);
		const uint16 field_opcode = field ? aml_object_opcode(*field) : 0x5B86;
		if (!field || field->type != AMLNamespace::ObjectType::FieldUnit || field_opcode != 0x5B86) return Fail(Status::UnsupportedObject, field ? field->value : nullptr, object_index, field_opcode);
		if (field->reference == AMLNamespace::InvalidIndex || field->secondary_reference == AMLNamespace::InvalidIndex || !AtObject(field->reference) || !AtObject(field->secondary_reference)) return Fail(Status::NotFound, field->value, object_index, field_opcode);
		if (field->reference == object_index || field->secondary_reference == object_index) return Fail(Status::Malformed, field->value, object_index, field_opcode);
		uint8 access_width = 0;
		Status status = ResolveFieldWidth(*field, object_index, field_opcode, access_width);
		if (status != Status::Success) return status;
		const uint8 update_rule = uint8((field->field_flags >> 5) & 3u);
		if (update_rule == 3) return Fail(Status::UnsupportedObject, field->value, object_index, field_opcode);
		if (field->bit_offset > ~uint64(0) - field->bit_length) return Fail(Status::Malformed, field->value, object_index, field_opcode);
		const uint64 field_end = field->bit_offset + field->bit_length;
		const uint64 access_bytes = access_width / 8;
		Target index_target = {};
		index_target.type = TargetType::Object;
		index_target.index = field->reference;
		Target data_target = {};
		data_target.type = TargetType::Object;
		data_target.index = field->secondary_reference;
		uint64 bit = field->bit_offset;
		uint64 source_bit = 0;
		while (bit < field_end) {
			const uint64 access_index = bit / access_width;
			if (access_index > ~uint64(0) / access_bytes) return Fail(Status::Malformed, field->value, object_index, field_opcode);
			status = StoreTarget(index_target, aml_integer(access_index * access_bytes));
			if (status != Status::Success) return status;
			const uint8 access_bit = uint8(bit % access_width);
			uint64 take = access_width - access_bit;
			if (take > field_end - bit) take = field_end - bit;
			const uint64 piece_mask = take == 64 ? ~uint64(0) : (uint64(1) << take) - 1;
			const uint64 write_mask = take == 64 ? ~uint64(0) : piece_mask << access_bit;
			uint64 access_value = 0;
			if (write_mask != (access_width == 64 ? ~uint64(0) : (uint64(1) << access_width) - 1)) {
				if (!update_rule) {
					Value data_value = {};
					status = EvaluateObject(field->secondary_reference, frame, data_value);
					if (status != Status::Success) return status;
					if ((status = RequireInteger(data_value, access_value, field->value)) != Status::Success) return status;
				}
				else if (update_rule == 1) access_value = ~uint64(0);
			}
			const uint64 piece = source_bit ? (value.integer >> source_bit) & piece_mask : value.integer & piece_mask;
			access_value = (access_value & ~write_mask) | (piece << access_bit);
			status = StoreTarget(data_target, aml_integer(access_value));
			if (status != Status::Success) return status;
			bit += take;
			source_bit += take;
		}
		return Status::Success;
	}

	AMLExecutor::Status AMLExecutor::ResolveBufferField(stduint object_index, Frame* frame, Value& source, uint64& bit_offset, uint64& bit_length) {
		const AMLNamespace::Object* field = AtObject(object_index);
		if (!field || field->type != AMLNamespace::ObjectType::BufferField || !field->value) return Fail(Status::UnsupportedObject, field ? field->value : nullptr, object_index);
		if (field->flags & AMLNamespace::ObjectFlagRuntime) {
			source = {};
			source.type = ValueType::Buffer;
			source.data = field->value;
			source.writable_data = const_cast<uint8*>(field->value);
			source.length = field->value_length;
			source.data_length = source.length;
			bit_offset = field->bit_offset;
			bit_length = field->bit_length;
		}
		else {
			const uint16 opcode = aml_object_opcode(*field);
			const uint8* cursor = field->value + (opcode > 0xFF ? 2 : 1);
			const uint8* end = field->value + field->value_length;
			Status status = EvaluateTerm(cursor, end, field->parent, frame, source);
			if (status != Status::Success) return status;
			if (source.type != ValueType::Buffer) return Fail(Status::TypeMismatch, field->value, object_index, opcode);
			Value index_value = {};
			status = EvaluateTerm(cursor, end, field->parent, frame, index_value);
			if (status != Status::Success) return status;
			if ((status = RequireInteger(index_value, bit_offset, field->value)) != Status::Success) return status;
			if (opcode == 0x5B13) {
				Value length_value = {};
				status = EvaluateTerm(cursor, end, field->parent, frame, length_value);
				if (status != Status::Success) return status;
				if ((status = RequireInteger(length_value, bit_length, field->value)) != Status::Success) return status;
			}
			else {
				if (opcode != 0x8D) {
					if (bit_offset > ~uint64(0) / 8) return Fail(Status::Malformed, field->value, object_index, opcode);
					bit_offset *= 8;
				}
				switch (opcode) {
				case 0x8A: bit_length = 32; break;
				case 0x8B: bit_length = 16; break;
				case 0x8C: bit_length = 8; break;
				case 0x8D: bit_length = 1; break;
				case 0x8F: bit_length = 64; break;
				default: return Fail(Status::UnsupportedOpcode, field->value, object_index, opcode);
				}
			}
		}
		const uint64 source_bit_length = uint64(source.length) << 3;
		if ((source_bit_length >> 3) != uint64(source.length) || bit_offset > ~uint64(0) - bit_length || bit_offset + bit_length > source_bit_length || (source.length && !source.data)) return Fail(Status::Malformed, field->value, object_index);
		return Status::Success;
	}

	AMLExecutor::Status AMLExecutor::ReadBufferField(stduint object_index, Frame* frame, Value& value) {
		Value source = {};
		uint64 bit_offset = 0;
		uint64 bit_length = 0;
		Status status = ResolveBufferField(object_index, frame, source, bit_offset, bit_length);
		if (status != Status::Success) return status;
		if (bit_length <= 64) {
			uint64 result = 0;
			for (uint64 bit = 0; bit < bit_length; ++bit) {
				const uint64 source_bit = bit_offset + bit;
				if (source.data[source_bit / 8] & uint8(1u << (source_bit & 7u))) result |= uint64(1) << bit;
			}
			value = aml_integer(result);
			return Status::Success;
		}
		const uint64 byte_length = bit_length / 8 + (bit_length & 7u ? 1 : 0);
		if (byte_length > uint64(~stduint(0))) return Fail(Status::BufferCapacity, AtObject(object_index)->value, object_index);
		status = AllocateBuffer(stduint(byte_length), ValueType::Buffer, value);
		if (status != Status::Success) return status;
		for (uint64 bit = 0; bit < bit_length; ++bit) {
			const uint64 source_bit = bit_offset + bit;
			if (source.data[source_bit / 8] & uint8(1u << (source_bit & 7u))) value.writable_data[bit / 8] |= uint8(1u << (bit & 7u));
		}
		value.data_length = value.length;
		return Status::Success;
	}

	AMLExecutor::Status AMLExecutor::WriteBufferField(stduint object_index, Frame* frame, const Value& value) {
		const AMLNamespace::Object* field = AtObject(object_index);
		if (value.type != ValueType::Integer && value.type != ValueType::Buffer) return Fail(Status::TypeMismatch, field ? field->value : nullptr, object_index);
		if (value.type == ValueType::Buffer && value.length && !value.data) return Fail(Status::Malformed, field ? field->value : nullptr, object_index);
		Value destination_value = {};
		uint64 bit_offset = 0;
		uint64 bit_length = 0;
		Status status = ResolveBufferField(object_index, frame, destination_value, bit_offset, bit_length);
		if (status != Status::Success) return status;
		if (!destination_value.writable_data) return Fail(Status::InvalidTarget, field ? field->value : nullptr, object_index);
		uint8* destination = destination_value.writable_data;
		for (uint64 bit = 0; bit < bit_length; ++bit) {
			bool set = false;
			if (value.type == ValueType::Integer) set = bit < 64 && (value.integer & (uint64(1) << bit));
			else set = bit < uint64(value.length) * 8 && (value.data[bit / 8] & uint8(1u << (bit & 7u)));
			const uint64 destination_bit = bit_offset + bit;
			const uint8 mask = uint8(1u << (destination_bit & 7u));
			if (set) destination[destination_bit / 8] |= mask;
			else destination[destination_bit / 8] &= uint8(~mask);
		}
		return Status::Success;
	}

	AMLExecutor::Status AMLExecutor::DefineBufferField(const uint8* address, const uint8*& cursor, const uint8* end, stduint scope, Frame* frame, uint16 opcode) {
		Value source = {};
		Status status = EvaluateTerm(cursor, end, scope, frame, source);
		if (status != Status::Success) return status;
		if (source.type != ValueType::Buffer || !source.writable_data) return Fail(Status::TypeMismatch, address, AMLNamespace::InvalidIndex, opcode);
		Value index_value = {};
		status = EvaluateTerm(cursor, end, scope, frame, index_value);
		if (status != Status::Success) return status;
		uint64 bit_offset = 0;
		if ((status = RequireInteger(index_value, bit_offset, address)) != Status::Success) return status;
		uint64 bit_length = 0;
		if (opcode == 0x5B13) {
			Value length_value = {};
			status = EvaluateTerm(cursor, end, scope, frame, length_value);
			if (status != Status::Success) return status;
			if ((status = RequireInteger(length_value, bit_length, address)) != Status::Success) return status;
		}
		else {
			if (opcode != 0x8D) {
				if (bit_offset > ~uint64(0) / 8) return Fail(Status::Malformed, address, AMLNamespace::InvalidIndex, opcode);
				bit_offset *= 8;
			}
			switch (opcode) {
			case 0x8A: bit_length = 32; break;
			case 0x8B: bit_length = 16; break;
			case 0x8C: bit_length = 8; break;
			case 0x8D: bit_length = 1; break;
			case 0x8F: bit_length = 64; break;
			default: return Fail(Status::UnsupportedOpcode, address, AMLNamespace::InvalidIndex, opcode);
			}
		}
		const uint64 source_bit_length = uint64(source.length) << 3;
		if ((source_bit_length >> 3) != uint64(source.length) || bit_offset > ~uint64(0) - bit_length || bit_offset + bit_length > source_bit_length) return Fail(Status::Malformed, address, AMLNamespace::InvalidIndex, opcode);
		AMLBytecode::Name name = {};
		if (!AMLBytecode::DecodeName(cursor, end, name)) return Fail(Status::Malformed, address, AMLNamespace::InvalidIndex, opcode);
		if (!workspace.dynamic_objects || dynamic_object_count >= workspace.dynamic_object_capacity) return Fail(Status::DynamicObjectCapacity, address, AMLNamespace::InvalidIndex, opcode);
		const stduint object_index = DefineDynamicObject(scope, name, AMLNamespace::ObjectType::BufferField, source.writable_data, source.length);
		if (object_index == AMLNamespace::InvalidIndex) return Fail(Status::Malformed, address, object_index, opcode);
		AMLNamespace::Object* field = AtDynamicObject(object_index);
		if (!field) return Fail(Status::Malformed, address, object_index, opcode);
		field->bit_offset = bit_offset;
		field->bit_length = bit_length;
		return Status::Success;
	}

	AMLExecutor::Status AMLExecutor::DefineOperationRegion(const uint8* address, const uint8*& cursor, const uint8* end, stduint scope, Frame* frame) {
		AMLBytecode::Name name = {};
		if (!AMLBytecode::DecodeName(cursor, end, name) || cursor >= end) return Fail(Status::Malformed, address, AMLNamespace::InvalidIndex, 0x5B80);
		const uint8 space = *cursor++;
		Value offset_value = {};
		Status status = EvaluateTerm(cursor, end, scope, frame, offset_value);
		if (status != Status::Success) return status;
		Value length_value = {};
		status = EvaluateTerm(cursor, end, scope, frame, length_value);
		if (status != Status::Success) return status;
		uint64 offset = 0;
		uint64 length = 0;
		if ((status = RequireInteger(offset_value, offset, address)) != Status::Success) return status;
		if ((status = RequireInteger(length_value, length, address)) != Status::Success) return status;
		if (!workspace.dynamic_objects || dynamic_object_count >= workspace.dynamic_object_capacity) return Fail(Status::DynamicObjectCapacity, address, AMLNamespace::InvalidIndex, 0x5B80);
		const stduint object_index = DefineDynamicObject(scope, name, AMLNamespace::ObjectType::OperationRegion, address, stduint(cursor - address));
		if (object_index == AMLNamespace::InvalidIndex) return Fail(Status::Malformed, address, object_index, 0x5B80);
		AMLNamespace::Object* object = AtDynamicObject(object_index);
		if (!object) return Fail(Status::Malformed, address, object_index, 0x5B80);
		object->external_type = space;
		object->bit_offset = offset;
		object->bit_length = length;
		return Status::Success;
	}

	AMLExecutor::Status AMLExecutor::DefineFieldList(const uint8* address, const uint8* begin, const uint8* end, stduint scope, stduint region, uint8 field_flags) {
		const uint8* cursor = begin;
		uint64 bit_offset = 0;
		while (cursor < end) {
			if (*cursor == 0x00) {
				uint32 bit_count = 0;
				uint8 encoded_length = 0;
				if (!AMLBytecode::DecodePackageLength(cursor + 1, stduint(end - cursor - 1), bit_count, encoded_length)) return Fail(Status::Malformed, cursor, AMLNamespace::InvalidIndex, 0x5B81);
				if (bit_offset > ~uint64(0) - bit_count) return Fail(Status::Malformed, cursor, AMLNamespace::InvalidIndex, 0x5B81);
				bit_offset += bit_count;
				cursor += 1 + encoded_length;
				continue;
			}
			if (*cursor == 0x01) {
				if (end - cursor < 3) return Fail(Status::Malformed, cursor, AMLNamespace::InvalidIndex, 0x5B81);
				field_flags = (field_flags & 0xF0u) | (cursor[1] & 0x0Fu);
				cursor += 3;
				continue;
			}
			if (*cursor == 0x02) {
				const uint8* value = cursor + 1;
				if (value < end && *value == 0x11) cursor = AMLBytecode::SkipSimpleDataObject(value, end);
				else {
					AMLBytecode::Name connection = {};
					if (!AMLBytecode::DecodeName(value, end, connection)) return Fail(Status::Malformed, cursor, AMLNamespace::InvalidIndex, 0x5B81);
					cursor = value;
				}
				if (!cursor || cursor > end) return Fail(Status::Malformed, address, AMLNamespace::InvalidIndex, 0x5B81);
				continue;
			}
			if (*cursor == 0x03) {
				if (end - cursor < 4) return Fail(Status::Malformed, cursor, AMLNamespace::InvalidIndex, 0x5B81);
				field_flags = (field_flags & 0xF0u) | (cursor[1] & 0x0Fu);
				cursor += 4;
				continue;
			}
			const uint8* field_begin = cursor;
			AMLBytecode::Name name = {};
			if (!AMLBytecode::DecodeName(cursor, end, name) || name.segment_count != 1 || name.rooted || name.parent_count) return Fail(Status::Malformed, field_begin, AMLNamespace::InvalidIndex, 0x5B81);
			uint32 bit_count = 0;
			uint8 encoded_length = 0;
			if (!AMLBytecode::DecodePackageLength(cursor, stduint(end - cursor), bit_count, encoded_length)) return Fail(Status::Malformed, cursor, AMLNamespace::InvalidIndex, 0x5B81);
			cursor += encoded_length;
			if (!workspace.dynamic_objects || dynamic_object_count >= workspace.dynamic_object_capacity) return Fail(Status::DynamicObjectCapacity, field_begin, AMLNamespace::InvalidIndex, 0x5B81);
			const stduint field_index = DefineDynamicObject(scope, name, AMLNamespace::ObjectType::FieldUnit, address, stduint(end - address));
			if (field_index == AMLNamespace::InvalidIndex) return Fail(Status::Malformed, field_begin, field_index, 0x5B81);
			AMLNamespace::Object* field = AtDynamicObject(field_index);
			if (!field) return Fail(Status::Malformed, field_begin, field_index, 0x5B81);
			field->reference = region;
			field->bit_offset = bit_offset;
			field->bit_length = bit_count;
			field->field_flags = field_flags;
			if (bit_offset > ~uint64(0) - bit_count) return Fail(Status::Malformed, field_begin, field_index, 0x5B81);
			bit_offset += bit_count;
		}
		return Status::Success;
	}

	AMLExecutor::Status AMLExecutor::DefineField(const uint8* address, const uint8*& cursor, const uint8* end, stduint scope) {
		const uint8* content = nullptr;
		const uint8* object_end = nullptr;
		if (!PackageRange(cursor, end, content, object_end)) return Fail(Status::Malformed, address, AMLNamespace::InvalidIndex, 0x5B81);
		AMLBytecode::Name region_name = {};
		if (!AMLBytecode::DecodeName(content, object_end, region_name) || content >= object_end) return Fail(Status::Malformed, address, AMLNamespace::InvalidIndex, 0x5B81);
		const stduint region = FindObject(scope, region_name);
		const AMLNamespace::Object* region_object = AtObject(region);
		if (!region_object || region_object->type != AMLNamespace::ObjectType::OperationRegion) return Fail(Status::NotFound, address, region, 0x5B81);
		const uint8 field_flags = *content++;
		const Status status = DefineFieldList(address, content, object_end, scope, region, field_flags);
		if (status != Status::Success) return status;
		cursor = object_end;
		return Status::Success;
	}

	AMLExecutor::Status AMLExecutor::EvaluateObject(stduint object_index, Frame* frame, Value& value) {
		if (object_index == AMLNamespace::InvalidIndex) return Fail(Status::NotFound, nullptr);
		const AMLNamespace::Object* object = AtObject(object_index);
		if (!object) return Fail(Status::NotFound, nullptr, object_index);
		if (object->flags & AMLNamespace::ObjectFlagConditional) return Fail(Status::ConditionalObject, object->value, object_index);
		if (object->type == AMLNamespace::ObjectType::Mutex) return Fail(Status::UnsupportedObject, object->value, object_index);
		if (ReadStoredObject(object_index, value) == Status::Success) return Status::Success;
		if (++object_depth > AMLNamespace::MaximumDepth) {
			--object_depth;
			return Fail(Status::FrameLimit, object->value, object_index);
		}
		Status status = Status::UnsupportedObject;
		switch (object->type) {
		case AMLNamespace::ObjectType::Name: {
			const uint8* cursor = object->value;
			status = EvaluateTerm(cursor, object->value + object->value_length, object->parent, frame, value);
			if (status == Status::Success) status = StoreObject(object_index, value);
			break;
		}
		case AMLNamespace::ObjectType::Method:
			status = ExecuteMethod(object_index, nullptr, 0, value);
			break;
		case AMLNamespace::ObjectType::Alias:
			if (object->reference != AMLNamespace::InvalidIndex) status = EvaluateObject(object->reference, frame, value);
			else {
				const uint8* source = object->value;
				AMLBytecode::Name name = {};
				if (!AMLBytecode::DecodeName(source, object->value + object->value_length, name)) status = Fail(Status::Malformed, object->value, object_index);
				else {
					const stduint target = FindObject(object->parent, name);
					status = target == AMLNamespace::InvalidIndex ? Fail(Status::NotFound, object->value, object_index) : EvaluateObject(target, frame, value);
				}
			}
			break;
		case AMLNamespace::ObjectType::FieldUnit:
			status = ReadField(object_index, frame, value);
			break;
		case AMLNamespace::ObjectType::BufferField:
			status = ReadBufferField(object_index, frame, value);
			break;
		default:
			status = Fail(Status::UnsupportedObject, object->value, object_index);
			break;
		}
		--object_depth;
		return status;
	}

	AMLExecutor::Status AMLExecutor::ExecuteMethod(stduint object_index, const Value* arguments, stduint argument_count, Value& value) {
		const AMLNamespace::Object* object = AtObject(object_index);
		if (!object || object->type != AMLNamespace::ObjectType::Method) return Fail(Status::UnsupportedObject, object ? object->value : nullptr, object_index);
		if (object->flags & AMLNamespace::ObjectFlagConditional) return Fail(Status::ConditionalObject, object->value, object_index);
		if (argument_count != object->argument_count || (argument_count && !arguments)) return Fail(Status::InvalidArgument, object->value, object_index);
		if (!workspace.frames || frame_count >= workspace.frame_capacity) return Fail(Status::FrameLimit, object->value, object_index);
		const stduint initial_mutex_top = mutex_top;
		const stduint initial_dynamic_count = dynamic_object_count;
		Status status = Status::Success;
		if (object->method_flags & 0x08u) {
			status = AcquireMutex(object_index, uint8(object->method_flags >> 4), object->value);
			if (status != Status::Success) return status;
		}
		const stduint method_mutex_top = mutex_top;
		Frame& frame = workspace.frames[frame_count++];
		frame = {};
		frame.scope = object->parent;
		const stduint copy_count = argument_count < numsof(frame.arguments) ? argument_count : numsof(frame.arguments);
		for0(index, copy_count) frame.arguments[index] = arguments[index];
		const uint8* cursor = object->value;
		const uint8* end = object->value + object->value_length;
		bool returned = false;
		bool broke = false;
		bool continued = false;
		status = ExecuteBlock(cursor, end, frame.scope, &frame, value, returned, broke, continued);
		if (status == Status::Success && mutex_top != method_mutex_top) status = Fail(Status::InvalidArgument, object->value, object_index);
		if (status == Status::Success && (broke || continued)) status = Fail(Status::InvalidArgument, object->value_length ? end - 1 : nullptr, object_index);
		if (status == Status::Success && !returned) value = {};
		RestoreMutex(initial_mutex_top);
		DiscardDynamicObjects(initial_dynamic_count);
		--frame_count;
		if (status != Status::Success) {
			if (last_error.object == AMLNamespace::InvalidIndex) last_error.object = object_index;
			return status;
		}
		return Status::Success;
	}

	AMLExecutor::Status AMLExecutor::Evaluate(stduint object, Value& value) {
		if (!workspace.frames || !workspace.frame_capacity || !limits.instruction_count || !limits.loop_count) return Fail(Status::InvalidArgument, nullptr, object);
		Reset();
		value = {};
		return EvaluateObject(object, nullptr, value);
	}

	AMLExecutor::Status AMLExecutor::EvaluateInteger(stduint object, uint64& integer) {
		Value value = {};
		const Status status = Evaluate(object, value);
		if (status != Status::Success) return status;
		if (value.type != ValueType::Integer) return Fail(Status::TypeMismatch, AtObject(object) ? AtObject(object)->value : nullptr, object);
		integer = value.integer;
		return Status::Success;
	}

	AMLExecutor::Status AMLExecutor::EvaluateBinary(const uint8*& cursor, const uint8* end, stduint scope, Frame* frame, uint8 opcode, Value& value) {
		const uint8* address = cursor - 1;
		Value left = {};
		Value right = {};
		Status status = EvaluateTerm(cursor, end, scope, frame, left);
		if (status != Status::Success) return status;
		status = EvaluateTerm(cursor, end, scope, frame, right);
		if (status != Status::Success) return status;
		uint64 left_integer = 0;
		uint64 right_integer = 0;
		status = RequireInteger(left, left_integer, address);
		if (status != Status::Success) return status;
		status = RequireInteger(right, right_integer, address);
		if (status != Status::Success) return status;
		uint64 result = 0;
		bool has_target = true;
		switch (opcode) {
		case 0x72: result = left_integer + right_integer; break;
		case 0x74: result = left_integer - right_integer; break;
		case 0x77: result = left_integer * right_integer; break;
		case 0x79: result = right_integer < 64 ? left_integer << right_integer : 0; break;
		case 0x7A: result = right_integer < 64 ? left_integer >> right_integer : 0; break;
		case 0x7B: result = left_integer & right_integer; break;
		case 0x7C: result = ~(left_integer & right_integer); break;
		case 0x7D: result = left_integer | right_integer; break;
		case 0x7E: result = ~(left_integer | right_integer); break;
		case 0x7F: result = left_integer ^ right_integer; break;
		case 0x85:
			if (!right_integer) return Fail(Status::InvalidArgument, address);
			result = left_integer % right_integer;
			break;
		case 0x90: result = left_integer && right_integer; has_target = false; break;
		case 0x91: result = left_integer || right_integer; has_target = false; break;
		case 0x93: result = left_integer == right_integer; has_target = false; break;
		case 0x94: result = left_integer > right_integer; has_target = false; break;
		case 0x95: result = left_integer < right_integer; has_target = false; break;
		default: return Fail(Status::UnsupportedOpcode, address);
		}
		value = aml_integer(result);
		if (!has_target) return Status::Success;
		Target target = {};
		status = ParseTarget(cursor, end, scope, frame, target);
		return status == Status::Success ? StoreTarget(target, value) : status;
	}

	AMLExecutor::Status AMLExecutor::EvaluateTerm(const uint8*& cursor, const uint8* end, stduint scope, Frame* frame, Value& value) {
		if (!cursor || cursor >= end) return Fail(Status::Malformed, nullptr);
		const uint8* address = cursor;
		Status status = ConsumeInstruction(address);
		if (status != Status::Success) return status;
		const uint8* integer_end = cursor;
		uint64 integer = 0;
		if (AMLBytecode::DecodeInteger(integer_end, end, integer)) {
			cursor = integer_end;
			value = aml_integer(integer);
			return Status::Success;
		}
		const uint8 opcode = *cursor++;
		if (opcode == 0x0D) {
			const uint8* text = cursor;
			while (cursor < end && *cursor) ++cursor;
			if (cursor >= end) return Fail(Status::Malformed, address);
			value = {};
			value.type = ValueType::String;
			value.data = text;
			value.length = stduint(cursor - text);
			value.data_length = value.length;
			++cursor;
			return Status::Success;
		}
		if (opcode == 0x11) {
			const uint8* content = nullptr;
			const uint8* object_end = nullptr;
			if (!PackageRange(cursor, end, content, object_end)) return Fail(Status::Malformed, address);
			cursor = content;
			Value length_value = {};
			status = EvaluateTerm(cursor, object_end, scope, frame, length_value);
			if (status != Status::Success) return status;
			uint64 length = 0;
			status = RequireInteger(length_value, length, address);
			if (status != Status::Success) return status;
			if (length > uint64(~stduint(0))) return Fail(Status::InvalidArgument, address);
			const stduint available = stduint(object_end - cursor);
			const stduint final_length = stduint(length) < available ? available : stduint(length);
			status = AllocateBuffer(final_length, ValueType::Buffer, value);
			if (status != Status::Success) {
				last_error.address = address;
				return status;
			}
			value.data_length = available;
			for0(index, value.data_length) value.writable_data[index] = cursor[index];
			cursor = object_end;
			return Status::Success;
		}
		if (opcode == 0x08) {
			AMLBytecode::Name name = {};
			if (!AMLBytecode::DecodeName(cursor, end, name)) return Fail(Status::Malformed, address, AMLNamespace::InvalidIndex, opcode);
			Value initial = {};
			status = EvaluateTerm(cursor, end, scope, frame, initial);
			if (status != Status::Success) return status;
			if (!workspace.dynamic_objects || dynamic_object_count >= workspace.dynamic_object_capacity) return Fail(Status::DynamicObjectCapacity, address, AMLNamespace::InvalidIndex, opcode);
			const stduint object_index = DefineDynamicObject(scope, name, AMLNamespace::ObjectType::Name, address, stduint(cursor - address));
			if (object_index == AMLNamespace::InvalidIndex) return Fail(Status::Malformed, address, object_index, opcode);
			status = StoreObject(object_index, initial);
			value = {};
			return status;
		}
		if (opcode >= 0x60 && opcode <= 0x67) {
			if (!frame) return Fail(Status::InvalidTarget, address);
			value = frame->locals[opcode - 0x60];
			return Status::Success;
		}
		if (opcode >= 0x68 && opcode <= 0x6E) {
			if (!frame) return Fail(Status::InvalidTarget, address);
			value = frame->arguments[opcode - 0x68];
			return Status::Success;
		}
		if (opcode == 0x70) {
			status = EvaluateTerm(cursor, end, scope, frame, value);
			if (status != Status::Success) return status;
			Target target = {};
			status = ParseTarget(cursor, end, scope, frame, target);
			return status == Status::Success ? StoreTarget(target, value) : status;
		}
		if (opcode == 0x71) {
			Target target = {};
			status = ParseTarget(cursor, end, scope, frame, target);
			if (status != Status::Success) return status;
			value = {};
			value.type = ValueType::Reference;
			value.reference = target.index;
			value.reference_frame = target.frame;
			if (target.type == TargetType::Object) value.reference_type = ReferenceType::Object;
			else if (target.type == TargetType::Local) value.reference_type = ReferenceType::Local;
			else if (target.type == TargetType::Argument) value.reference_type = ReferenceType::Argument;
			else value.reference_type = ReferenceType::None;
			return Status::Success;
		}
		if (opcode == 0x72 || opcode == 0x74 || opcode == 0x77 || opcode == 0x79 || opcode == 0x7A || opcode == 0x7B || opcode == 0x7C || opcode == 0x7D || opcode == 0x7E || opcode == 0x7F || opcode == 0x85 || opcode == 0x90 || opcode == 0x91 || opcode == 0x93 || opcode == 0x94 || opcode == 0x95) return EvaluateBinary(cursor, end, scope, frame, opcode, value);
		if (opcode == 0x75 || opcode == 0x76) {
			Target target = {};
			status = ParseTarget(cursor, end, scope, frame, target);
			if (status != Status::Success) return status;
			status = ReadTarget(target, value);
			if (status != Status::Success) return status;
			status = RequireInteger(value, integer, address);
			if (status != Status::Success) return status;
			value = aml_integer(opcode == 0x75 ? integer + 1 : integer - 1);
			return StoreTarget(target, value);
		}
		if (opcode == 0x78) {
			Value dividend = {};
			Value divisor = {};
			status = EvaluateTerm(cursor, end, scope, frame, dividend);
			if (status != Status::Success) return status;
			status = EvaluateTerm(cursor, end, scope, frame, divisor);
			if (status != Status::Success) return status;
			uint64 dividend_integer = 0;
			uint64 divisor_integer = 0;
			if ((status = RequireInteger(dividend, dividend_integer, address)) != Status::Success) return status;
			if ((status = RequireInteger(divisor, divisor_integer, address)) != Status::Success) return status;
			if (!divisor_integer) return Fail(Status::InvalidArgument, address);
			Target remainder_target = {};
			Target quotient_target = {};
			if ((status = ParseTarget(cursor, end, scope, frame, remainder_target)) != Status::Success) return status;
			if ((status = ParseTarget(cursor, end, scope, frame, quotient_target)) != Status::Success) return status;
			const Value remainder = aml_integer(dividend_integer % divisor_integer);
			value = aml_integer(dividend_integer / divisor_integer);
			if ((status = StoreTarget(remainder_target, remainder)) != Status::Success) return status;
			return StoreTarget(quotient_target, value);
		}
		if (opcode == 0x80 || opcode == 0x92) {
			status = EvaluateTerm(cursor, end, scope, frame, value);
			if (status != Status::Success) return status;
			status = RequireInteger(value, integer, address);
			if (status != Status::Success) return status;
			value = aml_integer(opcode == 0x80 ? ~integer : !integer);
			if (opcode == 0x92) return Status::Success;
			Target target = {};
			status = ParseTarget(cursor, end, scope, frame, target);
			return status == Status::Success ? StoreTarget(target, value) : status;
		}
		if (opcode == 0x83) {
			Value reference = {};
			status = EvaluateTerm(cursor, end, scope, frame, reference);
			return status == Status::Success ? ReadReference(reference, value) : status;
		}
		if (opcode == 0x87) {
			Value source = {};
			status = EvaluateTerm(cursor, end, scope, frame, source);
			if (status != Status::Success) return status;
			if (source.type != ValueType::Buffer && source.type != ValueType::String) return Fail(Status::TypeMismatch, address);
			value = aml_integer(source.length);
			return Status::Success;
		}
		if (opcode == 0x99 || opcode == 0x9D) {
			Value source = {};
			status = EvaluateTerm(cursor, end, scope, frame, source);
			if (status != Status::Success) return status;
			if (opcode == 0x99) {
				if (source.type != ValueType::Integer) return Fail(Status::TypeMismatch, address);
				value = source;
			}
			else {
				status = CopyValue(source, value, address);
				if (status != Status::Success) return status;
			}
			Target target = {};
			status = ParseTarget(cursor, end, scope, frame, target);
			return status == Status::Success ? StoreTarget(target, value) : status;
		}
		if (opcode == 0x84) {
			Value left = {};
			Value right = {};
			status = EvaluateTerm(cursor, end, scope, frame, left);
			if (status != Status::Success) return status;
			status = EvaluateTerm(cursor, end, scope, frame, right);
			if (status != Status::Success) return status;
			stduint left_length = 0;
			stduint right_length = 0;
			if ((status = ResourceTemplatePayloadLength(left, left_length, address)) != Status::Success) return status;
			if ((status = ResourceTemplatePayloadLength(right, right_length, address)) != Status::Success) return status;
			if (left_length > ~stduint(0) - right_length || left_length + right_length > ~stduint(0) - 2) return Fail(Status::BufferCapacity, address);
			status = AllocateBuffer(left_length + right_length + 2, ValueType::Buffer, value);
			if (status != Status::Success) {
				last_error.address = address;
				return status;
			}
			for0(index, left_length) value.writable_data[index] = left.data[index];
			for0(index, right_length) value.writable_data[left_length + index] = right.data[index];
			value.writable_data[value.length - 2] = 0x79;
			uint8 checksum = 0;
			for (stduint index = 0; index + 1 < value.length; ++index) checksum = uint8(checksum + value.writable_data[index]);
			value.writable_data[value.length - 1] = uint8(0u - checksum);
			value.data_length = value.length;
			Target target = {};
			status = ParseTarget(cursor, end, scope, frame, target);
			return status == Status::Success ? StoreTarget(target, value) : status;
		}
		if (opcode == 0x9E) {
			Value source = {};
			Value index_value = {};
			Value length_value = {};
			status = EvaluateTerm(cursor, end, scope, frame, source);
			if (status != Status::Success) return status;
			status = EvaluateTerm(cursor, end, scope, frame, index_value);
			if (status != Status::Success) return status;
			status = EvaluateTerm(cursor, end, scope, frame, length_value);
			if (status != Status::Success) return status;
			uint64 index = 0;
			uint64 requested = 0;
			if ((status = RequireInteger(index_value, index, address)) != Status::Success) return status;
			if ((status = RequireInteger(length_value, requested, address)) != Status::Success) return status;
			if (source.type != ValueType::Buffer && source.type != ValueType::String) return Fail(Status::TypeMismatch, address);
			stduint result_length = 0;
			if (index < source.length) {
				const uint64 available = uint64(source.length) - index;
				const uint64 selected = requested < available ? requested : available;
				if (selected > uint64(~stduint(0))) return Fail(Status::BufferCapacity, address);
				result_length = stduint(selected);
			}
			status = AllocateBuffer(result_length, source.type, value);
			if (status != Status::Success) {
				last_error.address = address;
				return status;
			}
			for0(copy_index, result_length) value.writable_data[copy_index] = source.data[stduint(index) + copy_index];
			value.data_length = result_length;
			Target target = {};
			status = ParseTarget(cursor, end, scope, frame, target);
			return status == Status::Success ? StoreTarget(target, value) : status;
		}
		if (opcode == 0x8A || opcode == 0x8B || opcode == 0x8C || opcode == 0x8D || opcode == 0x8F) {
			status = DefineBufferField(address, cursor, end, scope, frame, opcode);
			value = {};
			return status;
		}
		if (opcode == 0x5B) {
			if (cursor >= end) return Fail(Status::Malformed, address);
			const uint8 extended = *cursor++;
			if (extended == 0x80) {
				status = DefineOperationRegion(address, cursor, end, scope, frame);
				value = {};
				return status;
			}
			if (extended == 0x81) {
				status = DefineField(address, cursor, end, scope);
				value = {};
				return status;
			}
			if (extended == 0x13) {
				status = DefineBufferField(address, cursor, end, scope, frame, 0x5B13);
				value = {};
				return status;
			}
			if (extended == 0x23 || extended == 0x27) {
				Target target = {};
				status = ParseTarget(cursor, end, scope, frame, target);
				if (status != Status::Success) return status;
				stduint mutex_object = AMLNamespace::InvalidIndex;
				status = ResolveMutex(target, mutex_object);
				if (status != Status::Success) return status;
				if (extended == 0x23) {
					if (end - cursor < 2) return Fail(Status::Malformed, address, mutex_object, 0x5B23);
					// One executor evaluates one AML invocation at a time, so the timeout cannot expire on contention here.
					cursor += 2;
					const AMLNamespace::Object* mutex = AtObject(mutex_object);
					if (!mutex) return Fail(Status::NotFound, address, mutex_object, 0x5B23);
					status = AcquireMutex(mutex_object, uint8(mutex->value[0] & 0x0Fu), address);
					if (status != Status::Success) return status;
					value = aml_integer(0);
				}
				else {
					status = ReleaseMutex(mutex_object, address);
					if (status != Status::Success) return status;
					value = {};
				}
				return Status::Success;
			}
			if (extended == 0x12) {
				Target source = {};
				status = ParseTarget(cursor, end, scope, frame, source);
				if (status == Status::NotFound) {
					last_error = {};
					last_error.object = AMLNamespace::InvalidIndex;
					value = aml_integer(0);
					Target target = {};
					status = ParseTarget(cursor, end, scope, frame, target);
					return status;
				}
				if (status != Status::Success) return status;
				Value reference = {};
				reference.type = ValueType::Reference;
				reference.reference = source.index;
				reference.reference_frame = source.frame;
				if (source.type == TargetType::Object) reference.reference_type = ReferenceType::Object;
				else if (source.type == TargetType::Local) reference.reference_type = ReferenceType::Local;
				else if (source.type == TargetType::Argument) reference.reference_type = ReferenceType::Argument;
				Target target = {};
				status = ParseTarget(cursor, end, scope, frame, target);
				if (status != Status::Success) return status;
				status = StoreTarget(target, reference);
				if (status != Status::Success) return status;
				value = aml_integer(1);
				return Status::Success;
			}
			if (extended == 0x30) {
				value = aml_integer(2);
				return Status::Success;
			}
			if (extended == 0x31) {
				value = {};
				value.type = ValueType::Reference;
				value.reference_type = ReferenceType::None;
				return Status::Success;
			}
			return Fail(Status::UnsupportedOpcode, address, AMLNamespace::InvalidIndex, uint16(0x5B00u | extended));
		}
		--cursor;
		AMLBytecode::Name name = {};
		const uint8* name_end = cursor;
		if (!AMLBytecode::DecodeName(name_end, end, name)) return Fail(Status::UnsupportedOpcode, address);
		const stduint object_index = FindObject(scope, name);
		if (object_index == AMLNamespace::InvalidIndex) {
			const Status status = Fail(Status::NotFound, address);
			last_error.scope = scope;
			return status;
		}
		const AMLNamespace::Object* object = AtObject(object_index);
		if (!object) return Fail(Status::NotFound, address);
		cursor = name_end;
		if (object->type == AMLNamespace::ObjectType::Method) {
			Value arguments[7] = {};
			if (object->argument_count > numsof(arguments)) return Fail(Status::Malformed, address, object_index);
			for0(index, object->argument_count) {
				status = EvaluateTerm(cursor, end, scope, frame, arguments[index]);
				if (status != Status::Success) return status;
			}
			return ExecuteMethod(object_index, arguments, object->argument_count, value);
		}
		if (object->type == AMLNamespace::ObjectType::External) return Fail(Status::UnsupportedObject, address, object_index);
		return EvaluateObject(object_index, frame, value);
	}

	AMLExecutor::Status AMLExecutor::ExecuteBlock(const uint8*& cursor, const uint8* end, stduint scope, Frame* frame, Value& value, bool& returned, bool& broke, bool& continued) {
		while (cursor < end) {
			const uint8* address = cursor;
			if (*cursor == 0xA3) {
				Status status = ConsumeInstruction(cursor++);
				if (status != Status::Success) return status;
				continue;
			}
			if (*cursor == 0xA4) {
				Status status = ConsumeInstruction(cursor++);
				if (status != Status::Success) return status;
				status = EvaluateTerm(cursor, end, scope, frame, value);
				if (status != Status::Success) return status;
				returned = true;
				return Status::Success;
			}
			if (*cursor == 0xA5 || *cursor == 0x9F) {
				Status status = ConsumeInstruction(cursor);
				if (status != Status::Success) return status;
				broke = *cursor == 0xA5;
				continued = *cursor == 0x9F;
				++cursor;
				return Status::Success;
			}
			if (*cursor == 0xA0) {
				Status status = ConsumeInstruction(cursor);
				if (status != Status::Success) return status;
				const uint8* content = nullptr;
				const uint8* object_end = nullptr;
				if (!PackageRange(cursor + 1, end, content, object_end)) return Fail(Status::Malformed, cursor);
				const uint8* predicate = content;
				Value predicate_value = {};
				status = EvaluateTerm(predicate, object_end, scope, frame, predicate_value);
				if (status != Status::Success) return status;
				uint64 predicate_integer = 0;
				status = RequireInteger(predicate_value, predicate_integer, address);
				if (status != Status::Success) return status;
				if (predicate_integer) {
					const uint8* body = predicate;
					status = ExecuteBlock(body, object_end, scope, frame, value, returned, broke, continued);
					if (status != Status::Success || returned || broke || continued) return status;
				}
				cursor = object_end;
				if (cursor < end && *cursor == 0xA1) {
					const uint8* else_content = nullptr;
					const uint8* else_end = nullptr;
					if (!PackageRange(cursor + 1, end, else_content, else_end)) return Fail(Status::Malformed, cursor);
					if (!predicate_integer) {
						status = ConsumeInstruction(cursor);
						if (status != Status::Success) return status;
						const uint8* else_body = else_content;
						status = ExecuteBlock(else_body, else_end, scope, frame, value, returned, broke, continued);
						if (status != Status::Success || returned || broke || continued) return status;
					}
					cursor = else_end;
				}
				continue;
			}
			if (*cursor == 0xA1) {
				Status status = ConsumeInstruction(cursor);
				if (status != Status::Success) return status;
				const uint8* content = nullptr;
				const uint8* object_end = nullptr;
				if (!PackageRange(cursor + 1, end, content, object_end)) return Fail(Status::Malformed, cursor);
				cursor = object_end;
				continue;
			}
			if (*cursor == 0xA2) {
				Status status = ConsumeInstruction(cursor);
				if (status != Status::Success) return status;
				const uint8* content = nullptr;
				const uint8* object_end = nullptr;
				if (!PackageRange(cursor + 1, end, content, object_end)) return Fail(Status::Malformed, cursor);
				while (true) {
					if (++loop_count > limits.loop_count) return Fail(Status::LoopLimit, address);
					const uint8* predicate = content;
					Value predicate_value = {};
					status = EvaluateTerm(predicate, object_end, scope, frame, predicate_value);
					if (status != Status::Success) return status;
					uint64 predicate_integer = 0;
					status = RequireInteger(predicate_value, predicate_integer, address);
					if (status != Status::Success) return status;
					if (!predicate_integer) break;
					const uint8* body = predicate;
					status = ExecuteBlock(body, object_end, scope, frame, value, returned, broke, continued);
					if (status != Status::Success || returned) return status;
					if (broke) {
						broke = false;
						break;
					}
					continued = false;
				}
				cursor = object_end;
				continue;
			}
			Status status = EvaluateTerm(cursor, end, scope, frame, value);
			if (status != Status::Success) return status;
		}
		return Status::Success;
	}

}



