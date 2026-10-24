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
		last_error = {};
		last_error.object = AMLNamespace::InvalidIndex;
		if (!workspace.object_values) return;
		for0(index, workspace.object_value_capacity) workspace.object_values[index] = {};
	}

	const AMLExecutor::Error& AMLExecutor::LastError() const {
		return last_error;
	}

	AMLExecutor::Status AMLExecutor::Fail(Status status, const uint8* address, stduint object, uint16 opcode) {
		last_error.address = address;
		last_error.object = object;
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
				const AMLNamespace::Object* object = aml_namespace.At(object_index);
				if (!object) return Fail(Status::InvalidTarget, nullptr, object_index);
				if (object->flags & AMLNamespace::ObjectFlagConditional) return Fail(Status::ConditionalObject, object->value, object_index);
				if (object->type != AMLNamespace::ObjectType::Alias) {
					if (object->type == AMLNamespace::ObjectType::FieldUnit) return Fail(Status::RegionUnavailable, object->value, object_index, aml_object_opcode(*object));
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
					object_index = aml_namespace.Find(object->parent, name);
					if (object_index == AMLNamespace::InvalidIndex) return Fail(Status::NotFound, object->value, target.index);
				}
			}
			return Fail(Status::FrameLimit, aml_namespace.At(object_index) ? aml_namespace.At(object_index)->value : nullptr, object_index);
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
		const stduint object = aml_namespace.Find(scope, name);
		cursor = name_end;
		if (object == AMLNamespace::InvalidIndex) return Fail(Status::NotFound, address);
		const AMLNamespace::Object* namespace_object = aml_namespace.At(object);
		if (!namespace_object) return Fail(Status::NotFound, address, object);
		if (namespace_object->flags & AMLNamespace::ObjectFlagConditional) return Fail(Status::ConditionalObject, namespace_object->value, object);
		target.type = TargetType::Object;
		target.index = object;
		return Status::Success;
	}

	AMLExecutor::Status AMLExecutor::ReadField(stduint object_index, Frame* frame, Value& value) {
		const AMLNamespace::Object* field = aml_namespace.At(object_index);
		if (!field || field->type != AMLNamespace::ObjectType::FieldUnit) return Fail(Status::UnsupportedObject, field ? field->value : nullptr, object_index);
		const uint16 field_opcode = aml_object_opcode(*field);
		if (!field->value || field->value_length < 2 || field->value[0] != 0x5B || field->value[1] != 0x81) return Fail(Status::UnsupportedObject, field->value, object_index, field_opcode);
		if (!field->bit_length || field->bit_length > 64) return Fail(Status::UnsupportedObject, field->value, object_index, field_opcode);
		const AMLNamespace::Object* region = aml_namespace.At(field->reference);
		if (!region || region->type != AMLNamespace::ObjectType::OperationRegion || !region->value || !region->value_length) return Fail(Status::RegionUnavailable, field->value, object_index, field_opcode);
		if (region->flags & AMLNamespace::ObjectFlagConditional) return Fail(Status::ConditionalObject, region->value, field->reference);
		const uint8 space = region->value[0];
		const uint8* cursor = region->value + 1;
		const uint8* end = region->value + region->value_length;
		Value region_offset_value = {};
		Status status = EvaluateTerm(cursor, end, region->parent, frame, region_offset_value);
		if (status != Status::Success) return status;
		Value region_length_value = {};
		status = EvaluateTerm(cursor, end, region->parent, frame, region_length_value);
		if (status != Status::Success) return status;
		uint64 region_offset = 0;
		uint64 region_length = 0;
		if ((status = RequireInteger(region_offset_value, region_offset, region->value)) != Status::Success) return status;
		if ((status = RequireInteger(region_length_value, region_length, region->value)) != Status::Success) return status;
		const uint8 access_type = field->field_flags & 0x0Fu;
		uint8 access_width = 0;
		switch (access_type) {
		case 0: access_width = 8; break;
		case 1: access_width = 8; break;
		case 2: access_width = 16; break;
		case 3: access_width = 32; break;
		case 4: access_width = 64; break;
		default: return Fail(Status::UnsupportedObject, field->value, object_index, field_opcode);
		}
		if (field->bit_offset > ~uint64(0) - field->bit_length) return Fail(Status::Malformed, field->value, object_index, field_opcode);
		const uint64 field_end = field->bit_offset + field->bit_length;
		const uint64 required_bytes = field_end / 8 + (field_end & 7u ? 1 : 0);
		if (required_bytes > region_length || !workspace.region_access.read) return Fail(Status::RegionUnavailable, field->value, object_index, field_opcode);
		const uint64 access_bytes = access_width / 8;
		uint64 bit = field->bit_offset;
		uint64 result_bit = 0;
		uint64 result = 0;
		while (bit < field_end) {
			const uint64 access_index = bit / access_width;
			if (access_index > ~uint64(0) / access_bytes) return Fail(Status::Malformed, field->value, object_index, field_opcode);
			const uint64 access_offset = access_index * access_bytes;
			if (access_offset > region_length || access_bytes > region_length - access_offset || region_offset > ~uint64(0) - access_offset) return Fail(Status::RegionUnavailable, field->value, object_index, field_opcode);
			uint64 access_value = 0;
			if (!workspace.region_access.read(workspace.region_access.context, aml_namespace, field->reference, space, region_offset + access_offset, access_width, access_value)) return Fail(Status::RegionUnavailable, field->value, object_index, field_opcode);
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

	AMLExecutor::Status AMLExecutor::EvaluateObject(stduint object_index, Frame* frame, Value& value) {
		if (object_index == AMLNamespace::InvalidIndex) return Fail(Status::NotFound, nullptr);
		const AMLNamespace::Object* object = aml_namespace.At(object_index);
		if (!object) return Fail(Status::NotFound, nullptr, object_index);
		if (object->flags & AMLNamespace::ObjectFlagConditional) return Fail(Status::ConditionalObject, object->value, object_index);
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
					const stduint target = aml_namespace.Find(object->parent, name);
					status = target == AMLNamespace::InvalidIndex ? Fail(Status::NotFound, object->value, object_index) : EvaluateObject(target, frame, value);
				}
			}
			break;
		case AMLNamespace::ObjectType::FieldUnit:
			status = ReadField(object_index, frame, value);
			break;
		default:
			status = Fail(Status::UnsupportedObject, object->value, object_index);
			break;
		}
		--object_depth;
		return status;
	}

	AMLExecutor::Status AMLExecutor::ExecuteMethod(stduint object_index, const Value* arguments, stduint argument_count, Value& value) {
		const AMLNamespace::Object* object = aml_namespace.At(object_index);
		if (!object || object->type != AMLNamespace::ObjectType::Method) return Fail(Status::UnsupportedObject, object ? object->value : nullptr, object_index);
		if (object->flags & AMLNamespace::ObjectFlagConditional) return Fail(Status::ConditionalObject, object->value, object_index);
		if (argument_count != object->argument_count || (argument_count && !arguments)) return Fail(Status::InvalidArgument, object->value, object_index);
		if (!workspace.frames || frame_count >= workspace.frame_capacity) return Fail(Status::FrameLimit, object->value, object_index);
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
		Status status = ExecuteBlock(cursor, end, frame.scope, &frame, value, returned, broke, continued);
		--frame_count;
		if (status != Status::Success) {
			if (last_error.object == AMLNamespace::InvalidIndex) last_error.object = object_index;
			return status;
		}
		if (!returned) return Fail(Status::NoReturn, object->value_length ? end - 1 : nullptr, object_index);
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
		if (value.type != ValueType::Integer) return Fail(Status::TypeMismatch, aml_namespace.At(object) ? aml_namespace.At(object)->value : nullptr, object);
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
			value = {};
			value.type = ValueType::Buffer;
			value.data = cursor;
			value.length = stduint(length);
			value.data_length = value.length < available ? value.length : available;
			cursor = object_end;
			return Status::Success;
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
			status = EvaluateTerm(cursor, end, scope, frame, value);
			if (status != Status::Success) return status;
			if (opcode == 0x99 && value.type != ValueType::Integer) return Fail(Status::TypeMismatch, address);
			Target target = {};
			status = ParseTarget(cursor, end, scope, frame, target);
			return status == Status::Success ? StoreTarget(target, value) : status;
		}
		if (opcode == 0x5B) {
			if (cursor >= end) return Fail(Status::Malformed, address);
			const uint8 extended = *cursor++;
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
		const stduint object_index = aml_namespace.Find(scope, name);
		if (object_index == AMLNamespace::InvalidIndex) return Fail(Status::NotFound, address);
		const AMLNamespace::Object* object = aml_namespace.At(object_index);
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



