// UTF-8 CPP-ISO11 TAB4 CRLF
// Docutitle: ACPI Machine Language Namespace and Bytecode
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../inc/cpp/Device/ACPI/AML.hpp"

namespace uni {

	bool AMLBytecode::DecodePackageLength(const uint8* aml, stduint available, uint32& package_length, uint8& encoded_length) {
		if (!aml || !available) return false;
		const uint8 following = aml[0] >> 6;
		encoded_length = following + 1;
		if (encoded_length > available) return false;
		package_length = following ? aml[0] & 0x0Fu : aml[0] & 0x3Fu;
		for (uint8 index = 0; index < following; ++index) package_length |= uint32(aml[index + 1]) << (4 + index * 8);
		return package_length >= encoded_length;
	}

	bool AMLBytecode::DecodeInteger(const uint8*& cursor, const uint8* end, uint64& value) {
		if (!cursor || cursor >= end) return false;
		switch (*cursor++) {
		case 0x00:
			value = 0;
			return true;
		case 0x01:
			value = 1;
			return true;
		case 0xFF:
			value = ~uint64(0);
			return true;
		case 0x0A:
			if (end - cursor < 1) return false;
			value = *cursor++;
			return true;
		case 0x0B:
			if (end - cursor < 2) return false;
			value = uint64(cursor[0]) | uint64(cursor[1]) << 8;
			cursor += 2;
			return true;
		case 0x0C:
			if (end - cursor < 4) return false;
			value = uint64(cursor[0]) | uint64(cursor[1]) << 8 | uint64(cursor[2]) << 16 | uint64(cursor[3]) << 24;
			cursor += 4;
			return true;
		case 0x0E:
			if (end - cursor < 8) return false;
			value = 0;
			for0(index, 8) value |= uint64(cursor[index]) << (index * 8);
			cursor += 8;
			return true;
		default:
			return false;
		}
	}

	bool AMLBytecode::DecodeName(const uint8*& cursor, const uint8* end, Name& name) {
		name = {};
		if (!cursor || cursor >= end) return false;
		if (*cursor == '\\') {
			name.rooted = true;
			++cursor;
		}
		while (cursor < end && *cursor == '^') {
			if (name.parent_count == 0xFFu) return false;
			++name.parent_count;
			++cursor;
		}
		if (cursor >= end) return false;
		if (*cursor == 0x00) {
			++cursor;
			return true;
		}
		uint8 segment_count = 1;
		if (*cursor == 0x2E) {
			segment_count = 2;
			++cursor;
		}
		else if (*cursor == 0x2F) {
			++cursor;
			if (cursor >= end) return false;
			segment_count = *cursor++;
			if (!segment_count) return false;
		}
		if (segment_count > NameSegmentCapacity) return false;
		for (uint8 segment_index = 0; segment_index < segment_count; ++segment_index) {
			if (end - cursor < 4 || !(*cursor == '_' || (*cursor >= 'A' && *cursor <= 'Z'))) return false;
			for0(character_index, 4) {
				const uint8 character = cursor[character_index];
				if (!(character == '_' || (character >= 'A' && character <= 'Z') || (character >= '0' && character <= '9'))) return false;
				name.segments[segment_index][character_index] = char(character);
			}
			name.segments[segment_index][4] = 0;
			cursor += 4;
		}
		name.segment_count = segment_count;
		return true;
	}

	const uint8* AMLBytecode::SkipSimpleDataObject(const uint8* cursor, const uint8* end) {
		if (!cursor || cursor >= end) return cursor;
		const uint8 opcode = *cursor;
		if (opcode == 0x00 || opcode == 0x01 || opcode == 0xFF) return cursor + 1;
		if (opcode == 0x0A) return end - cursor >= 2 ? cursor + 2 : end;
		if (opcode == 0x0B) return end - cursor >= 3 ? cursor + 3 : end;
		if (opcode == 0x0C) return end - cursor >= 5 ? cursor + 5 : end;
		if (opcode == 0x0E) return end - cursor >= 9 ? cursor + 9 : end;
		if (opcode == 0x0D) {
			++cursor;
			while (cursor < end && *cursor) ++cursor;
			return cursor < end ? cursor + 1 : end;
		}
		if (opcode == 0x11 || opcode == 0x12 || opcode == 0x13) {
			uint32 package_length = 0;
			uint8 encoded_length = 0;
			if (!DecodePackageLength(cursor + 1, stduint(end - cursor - 1), package_length, encoded_length) || package_length > stduint(end - cursor - 1)) return end;
			return cursor + 1 + package_length;
		}
		Name name = {};
		const uint8* name_end = cursor;
		if (DecodeName(name_end, end, name)) return name_end;
		return cursor + 1;
	}

	AMLNamespace::AMLNamespace(Object* storage, stduint storage_capacity) : objects(storage), capacity(storage_capacity) {
		Reset();
	}

	void AMLNamespace::Reset() {
		count = 0;
		conditional_count = 0;
		truncated = false;
		table_begin = nullptr;
		table_end = nullptr;
		current_table = 0;
		last_error = {};
		if (!objects || !capacity) return;
		Object& root = objects[count++];
		root = {};
		root.parent = InvalidIndex;
		root.reference = InvalidIndex;
		root.secondary_reference = InvalidIndex;
		root.name[0] = '\\';
		root.name[1] = 0;
		root.type = ObjectType::Root;
	}

	AMLNamespace::Status AMLNamespace::Fail(Status status, const uint8* address) {
		last_error.address = address;
		last_error.offset = address && table_begin && address >= table_begin ? stduint(address - table_begin) : 0;
		last_error.table_index = current_table;
		last_error.status = status;
		return status;
	}

	stduint AMLNamespace::Count() const {
		return count;
	}

	const AMLNamespace::Object* AMLNamespace::At(stduint index) const {
		return objects && index < count ? objects + index : nullptr;
	}

	stduint AMLNamespace::ConditionalCount() const {
		return conditional_count;
	}

	bool AMLNamespace::Truncated() const {
		return truncated;
	}

	const AMLNamespace::Error& AMLNamespace::LastError() const {
		return last_error;
	}

	stduint AMLNamespace::FindChild(stduint parent, const char name[5]) const {
		if (!objects || parent >= count || !name) return InvalidIndex;
		for (stduint index = 1; index < count; ++index) {
			if (objects[index].parent != parent) continue;
			bool equal = true;
			for0(character, 4) equal &= objects[index].name[character] == name[character];
			if (equal) return index;
		}
		return InvalidIndex;
	}

	stduint AMLNamespace::ParentAfterPrefixes(stduint scope, const AMLBytecode::Name& name) const {
		stduint parent = name.rooted ? 0 : scope;
		if (parent >= count) return InvalidIndex;
		for (uint8 prefix = 0; prefix < name.parent_count; ++prefix) {
			if (!parent) break;
			parent = objects[parent].parent;
		}
		return parent;
	}

	stduint AMLNamespace::Find(stduint scope, const AMLBytecode::Name& name) const {
		if (!objects || !count) return InvalidIndex;
		stduint base = ParentAfterPrefixes(scope, name);
		if (base == InvalidIndex) return InvalidIndex;
		if (!name.segment_count) return base;
		const bool search_parents = !name.rooted && !name.parent_count;
		while (true) {
			stduint object = base;
			for (uint8 segment = 0; segment < name.segment_count && object != InvalidIndex; ++segment) object = FindChild(object, name.segments[segment]);
			if (object != InvalidIndex) return object;
			if (!search_parents || !base) break;
			base = objects[base].parent;
		}
		return InvalidIndex;
	}

	stduint AMLNamespace::FindPath(const char* path) const {
		if (!objects || !count || !path) return InvalidIndex;
		const char* cursor = path;
		if (*cursor == '\\') ++cursor;
		stduint object = 0;
		if (!*cursor) return object;
		while (*cursor) {
			char name[5] = { '_', '_', '_', '_', 0 };
			stduint length = 0;
			while (*cursor && *cursor != '.') {
				if (length >= 4) return InvalidIndex;
				const char character = *cursor++;
				if (!((character >= 'A' && character <= 'Z') || (character >= '0' && character <= '9') || character == '_')) return InvalidIndex;
				name[length++] = character;
			}
			if (!length) return InvalidIndex;
			object = FindChild(object, name);
			if (object == InvalidIndex) return InvalidIndex;
			if (*cursor == '.') {
				++cursor;
				if (!*cursor) return InvalidIndex;
			}
		}
		return object;
	}

	stduint AMLNamespace::Add(stduint parent, const char name[5], ObjectType type, uint8 flags, const uint8* value, stduint value_length) {
		if (!objects || parent >= count || !name) return InvalidIndex;
		const stduint existing = FindChild(parent, name);
		if (existing != InvalidIndex) {
			Object& object = objects[existing];
			const bool replace = (object.flags & ObjectFlagPlaceholder) || object.type == ObjectType::External;
			object.flags |= flags;
			if (replace) {
				object.type = type;
				object.flags &= ~ObjectFlagPlaceholder;
				object.value = value;
				object.value_length = value_length;
				object.table_index = current_table;
			}
			return existing;
		}
		if (count >= capacity) {
			truncated = true;
			return InvalidIndex;
		}
		Object& object = objects[count];
		object = {};
		object.value = value;
		object.value_length = value_length;
		object.parent = parent;
		object.reference = InvalidIndex;
		object.secondary_reference = InvalidIndex;
		for0(character, 4) object.name[character] = name[character];
		object.name[4] = 0;
		object.table_index = current_table;
		object.type = type;
		object.flags = flags;
		return count++;
	}

	stduint AMLNamespace::ResolveDefinitionParent(stduint scope, const AMLBytecode::Name& name, uint8 flags) {
		stduint parent = ParentAfterPrefixes(scope, name);
		if (parent == InvalidIndex) return InvalidIndex;
		for (uint8 segment = 0; segment + 1 < name.segment_count; ++segment) {
			stduint child = FindChild(parent, name.segments[segment]);
			if (child == InvalidIndex) child = Add(parent, name.segments[segment], ObjectType::Scope, flags | ObjectFlagPlaceholder, nullptr, 0);
			if (child == InvalidIndex) return InvalidIndex;
			parent = child;
		}
		return parent;
	}

	stduint AMLNamespace::Define(stduint scope, const AMLBytecode::Name& name, ObjectType type, uint8 flags, const uint8* value, stduint value_length) {
		if (!name.segment_count) return InvalidIndex;
		const stduint parent = ResolveDefinitionParent(scope, name, flags);
		return parent == InvalidIndex ? InvalidIndex : Add(parent, name.segments[name.segment_count - 1], type, flags, value, value_length);
	}

	stduint AMLNamespace::ResolveOrCreateScope(stduint scope, const AMLBytecode::Name& name, uint8 flags) {
		const stduint found = Find(scope, name);
		if (found != InvalidIndex) return found;
		return Define(scope, name, ObjectType::Scope, flags | ObjectFlagPlaceholder, nullptr, 0);
	}

	bool AMLNamespace::PackageRange(const uint8* length_at, const uint8* limit, const uint8*& content, const uint8*& object_end) const {
		uint32 package_length = 0;
		uint8 encoded_length = 0;
		if (!length_at || length_at >= limit || !AMLBytecode::DecodePackageLength(length_at, stduint(limit - length_at), package_length, encoded_length) || package_length > stduint(limit - length_at)) return false;
		content = length_at + encoded_length;
		object_end = length_at + package_length;
		return content <= object_end;
	}

	const uint8* AMLNamespace::SkipTermArgument(const uint8* cursor, const uint8* end, stduint scope, stduint depth) const {
		if (!cursor || cursor >= end || depth >= MaximumDepth) return cursor;
		if (*cursor == 0x00 || *cursor == 0x01 || *cursor == 0x0A || *cursor == 0x0B || *cursor == 0x0C || *cursor == 0x0D || *cursor == 0x0E || *cursor == 0x11 || *cursor == 0x12 || *cursor == 0x13 || *cursor == 0xFF) return AMLBytecode::SkipSimpleDataObject(cursor, end);
		if ((*cursor >= 0x60 && *cursor <= 0x6E) || *cursor == 0xCC) return cursor + 1;
		if (end - cursor >= 2 && cursor[0] == 0x5B) {
			uint8 argument_count = 0;
			uint8 target_count = 0;
			switch (cursor[1]) {
			case 0x12: argument_count = 1; target_count = 1; break;
			case 0x13: argument_count = 3; target_count = 1; break;
			case 0x1F: argument_count = 6; break;
			case 0x20: argument_count = 1; target_count = 1; break;
			case 0x23: case 0x25: argument_count = 2; break;
			case 0x28: case 0x29: argument_count = 1; target_count = 1; break;
			case 0x30: case 0x31: case 0x33: return cursor + 2;
			default: return cursor + 2;
			}
			const uint8* operand = cursor + 2;
			for (uint8 index = 0; index < uint8(argument_count + target_count) && operand < end; ++index) {
				const uint8* next = SkipTermArgument(operand, end, scope, depth + 1);
				if (next <= operand) return end;
				operand = next;
			}
			return operand;
		}
		AMLBytecode::Name name = {};
		const uint8* after_name = cursor;
		if (AMLBytecode::DecodeName(after_name, end, name)) {
			const stduint object_index = Find(scope, name);
			const Object* object = At(object_index);
			uint8 argument_count = object && (object->type == ObjectType::Method || object->type == ObjectType::External) ? object->argument_count : 0;
			while (argument_count-- && after_name < end) {
				const uint8* next = SkipTermArgument(after_name, end, scope, depth + 1);
				if (next <= after_name) return end;
				after_name = next;
			}
			return after_name;
		}
		uint8 argument_count = 0;
		uint8 target_count = 0;
		switch (*cursor) {
		case 0x70: argument_count = 1; target_count = 1; break;
		case 0x71: argument_count = 1; break;
		case 0x72: case 0x73: case 0x74: case 0x77: case 0x79: case 0x7A:
		case 0x7B: case 0x7C: case 0x7D: case 0x7E: case 0x7F: case 0x84:
		case 0x85: argument_count = 2; target_count = 1; break;
		case 0x75: case 0x76: argument_count = 1; break;
		case 0x78: argument_count = 2; target_count = 2; break;
		case 0x80: case 0x81: case 0x82: argument_count = 1; target_count = 1; break;
		case 0x83: case 0x87: case 0x8E: case 0x92: argument_count = 1; break;
		case 0x86: argument_count = 2; break;
		case 0x88: argument_count = 2; target_count = 1; break;
		case 0x8A: case 0x8B: case 0x8C: case 0x8D: case 0x8F: argument_count = 2; target_count = 1; break;
		case 0x89: argument_count = 6; break;
		case 0x90: case 0x91: case 0x93: case 0x94: case 0x95: argument_count = 2; break;
		case 0x96: case 0x97: case 0x98: case 0x99: argument_count = 1; target_count = 1; break;
		case 0x9C: argument_count = 2; target_count = 1; break;
		case 0x9D: argument_count = 1; target_count = 1; break;
		case 0x9E: argument_count = 3; target_count = 1; break;
		default: return cursor + 1;
		}
		const uint8* operand = cursor + 1;
		for (uint8 index = 0; index < uint8(argument_count + target_count) && operand < end; ++index) {
			const uint8* next = SkipTermArgument(operand, end, scope, depth + 1);
			if (next <= operand) return end;
			operand = next;
		}
		return operand;
	}

	AMLNamespace::Status AMLNamespace::ParseFieldList(const uint8* begin, const uint8* end, stduint scope, stduint reference, stduint secondary_reference, const uint8* declaration, stduint declaration_length, uint8 field_flags, uint8 flags) {
		const uint8* cursor = begin;
		uint64 bit_offset = 0;
		while (cursor < end) {
			if (*cursor == 0x00) {
				uint32 bit_count = 0;
				uint8 encoded_length = 0;
				if (!AMLBytecode::DecodePackageLength(cursor + 1, stduint(end - cursor - 1), bit_count, encoded_length)) return Fail(Status::MalformedPackage, cursor);
				bit_offset += bit_count;
				cursor += 1 + encoded_length;
				continue;
			}
			if (*cursor == 0x01) {
				if (end - cursor < 3) return Fail(Status::MalformedObject, cursor);
				field_flags = (field_flags & 0xF0u) | (cursor[1] & 0x0Fu);
				cursor += 3;
				continue;
			}
			if (*cursor == 0x02) {
				const uint8* value = cursor + 1;
				if (value < end && *value == 0x11) cursor = AMLBytecode::SkipSimpleDataObject(value, end);
				else {
					AMLBytecode::Name connection = {};
					if (!AMLBytecode::DecodeName(value, end, connection)) return Fail(Status::MalformedName, cursor + 1);
					cursor = value;
				}
				continue;
			}
			if (*cursor == 0x03) {
				if (end - cursor < 4) return Fail(Status::MalformedObject, cursor);
				field_flags = (field_flags & 0xF0u) | (cursor[1] & 0x0Fu);
				cursor += 4;
				continue;
			}
			const uint8* field_begin = cursor;
			AMLBytecode::Name field_name = {};
			if (!AMLBytecode::DecodeName(cursor, end, field_name) || field_name.segment_count != 1 || field_name.rooted || field_name.parent_count) return Fail(Status::MalformedName, field_begin);
			uint32 bit_count = 0;
			uint8 encoded_length = 0;
			if (!AMLBytecode::DecodePackageLength(cursor, stduint(end - cursor), bit_count, encoded_length)) return Fail(Status::MalformedPackage, cursor);
			cursor += encoded_length;
			const stduint field_index = Define(scope, field_name, ObjectType::FieldUnit, flags, declaration, declaration_length);
			if (field_index == InvalidIndex) return Fail(Status::Capacity, field_begin);
			objects[field_index].reference = reference;
			objects[field_index].secondary_reference = secondary_reference;
			objects[field_index].bit_offset = bit_offset;
			objects[field_index].bit_length = bit_count;
			objects[field_index].field_flags = field_flags;
			bit_offset += bit_count;
		}
		return Status::Success;
	}

	AMLNamespace::Status AMLNamespace::ParseExtended(const uint8*& cursor, const uint8* end, stduint scope, uint8 flags, stduint depth) {
		const uint8* opcode = cursor;
		if (end - cursor < 2 || *cursor != 0x5B) return Fail(Status::MalformedObject, cursor);
		const uint8 extended = cursor[1];
		if (extended == 0x01 || extended == 0x02) {
			const uint8* operand = cursor + 2;
			AMLBytecode::Name name = {};
			if (!AMLBytecode::DecodeName(operand, end, name)) return Fail(Status::MalformedName, cursor + 2);
			const stduint length = extended == 0x01 ? 1 : 0;
			if (stduint(end - operand) < length) return Fail(Status::MalformedObject, opcode);
			if (Define(scope, name, extended == 0x01 ? ObjectType::Mutex : ObjectType::Event, flags, operand, length) == InvalidIndex) return Fail(Status::Capacity, opcode);
			cursor = operand + length;
			return Status::Success;
		}
		if (extended == 0x80) {
			const uint8* operand = cursor + 2;
			AMLBytecode::Name name = {};
			if (!AMLBytecode::DecodeName(operand, end, name) || operand >= end) return Fail(Status::MalformedName, cursor + 2);
			const uint8* value = operand;
			++operand;
			operand = SkipTermArgument(operand, end, scope, depth + 1);
			operand = SkipTermArgument(operand, end, scope, depth + 1);
			if (operand > end) return Fail(Status::MalformedObject, opcode);
			if (Define(scope, name, ObjectType::OperationRegion, flags, value, stduint(operand - value)) == InvalidIndex) return Fail(Status::Capacity, opcode);
			cursor = operand;
			return Status::Success;
		}
		if (extended == 0x13) {
			const uint8* operand = cursor + 2;
			const uint8* value = cursor;
			for0(index, 3) operand = SkipTermArgument(operand, end, scope, depth + 1);
			AMLBytecode::Name name = {};
			if (!AMLBytecode::DecodeName(operand, end, name)) return Fail(Status::MalformedName, operand);
			if (Define(scope, name, ObjectType::BufferField, flags, value, stduint(operand - value)) == InvalidIndex) return Fail(Status::Capacity, opcode);
			cursor = operand;
			return Status::Success;
		}
		if (extended == 0x81 || extended == 0x86 || extended == 0x87) {
			const uint8* content = nullptr;
			const uint8* object_end = nullptr;
			if (!PackageRange(cursor + 2, end, content, object_end)) return Fail(Status::MalformedPackage, opcode);
			AMLBytecode::Name reference_name = {};
			if (!AMLBytecode::DecodeName(content, object_end, reference_name)) return Fail(Status::MalformedName, content);
			const stduint reference = Find(scope, reference_name);
			stduint secondary_reference = InvalidIndex;
			if (extended == 0x86) {
				AMLBytecode::Name data_name = {};
				if (!AMLBytecode::DecodeName(content, object_end, data_name)) return Fail(Status::MalformedName, content);
				secondary_reference = Find(scope, data_name);
			}
			else if (extended == 0x87) {
				AMLBytecode::Name bank_name = {};
				if (!AMLBytecode::DecodeName(content, object_end, bank_name)) return Fail(Status::MalformedName, content);
				secondary_reference = Find(scope, bank_name);
				content = SkipTermArgument(content, object_end, scope, depth + 1);
			}
			if (content >= object_end) return Fail(Status::MalformedObject, opcode);
			const uint8 field_flags = *content++;
			const Status status = ParseFieldList(content, object_end, scope, reference, secondary_reference, opcode, stduint(object_end - opcode), field_flags, flags);
			if (status != Status::Success) return status;
			cursor = object_end;
			return Status::Success;
		}
		if (extended >= 0x82 && extended <= 0x85) {
			const uint8* content = nullptr;
			const uint8* object_end = nullptr;
			if (!PackageRange(cursor + 2, end, content, object_end)) return Fail(Status::MalformedPackage, opcode);
			AMLBytecode::Name name = {};
			if (!AMLBytecode::DecodeName(content, object_end, name)) return Fail(Status::MalformedName, content);
			const uint8* body = content;
			ObjectType type = ObjectType::Device;
			if (extended == 0x83) {
				type = ObjectType::Processor;
				if (object_end - content < 6) return Fail(Status::MalformedObject, opcode);
				content += 6;
			}
			else if (extended == 0x84) {
				type = ObjectType::PowerResource;
				if (object_end - content < 3) return Fail(Status::MalformedObject, opcode);
				content += 3;
			}
			else if (extended == 0x85) type = ObjectType::ThermalZone;
			const stduint object_index = Define(scope, name, type, flags, body, stduint(object_end - body));
			if (object_index == InvalidIndex) return Fail(Status::Capacity, opcode);
			const Status status = ParseTermList(content, object_end, object_index, flags, depth + 1);
			if (status != Status::Success) return status;
			cursor = object_end;
			return Status::Success;
		}
		if (extended == 0x88) {
			const uint8* operand = cursor + 2;
			AMLBytecode::Name name = {};
			if (!AMLBytecode::DecodeName(operand, end, name)) return Fail(Status::MalformedName, operand);
			const uint8* value = operand;
			for0(index, 3) operand = SkipTermArgument(operand, end, scope, depth + 1);
			if (Define(scope, name, ObjectType::Name, flags, value, stduint(operand - value)) == InvalidIndex) return Fail(Status::Capacity, opcode);
			cursor = operand;
			return Status::Success;
		}
		cursor += 2;
		return Status::Success;
	}

	AMLNamespace::Status AMLNamespace::ParseTermList(const uint8* begin, const uint8* end, stduint scope, uint8 flags, stduint depth) {
		if (!begin || !end || begin > end) return Fail(Status::InvalidArgument, begin);
		if (depth >= MaximumDepth) return Fail(Status::DepthLimit, begin);
		const uint8* cursor = begin;
		while (cursor < end) {
			const uint8* opcode = cursor;
			if (*cursor == 0x06) {
				const uint8* operand = cursor + 1;
				AMLBytecode::Name source = {};
				AMLBytecode::Name alias = {};
				if (!AMLBytecode::DecodeName(operand, end, source) || !AMLBytecode::DecodeName(operand, end, alias)) return Fail(Status::MalformedName, opcode);
				const stduint alias_index = Define(scope, alias, ObjectType::Alias, flags, cursor + 1, stduint(operand - cursor - 1));
				if (alias_index == InvalidIndex) return Fail(Status::Capacity, opcode);
				objects[alias_index].reference = Find(scope, source);
				cursor = operand;
				continue;
			}
			if (*cursor == 0x08) {
				const uint8* operand = cursor + 1;
				AMLBytecode::Name name = {};
				if (!AMLBytecode::DecodeName(operand, end, name)) return Fail(Status::MalformedName, cursor + 1);
				const uint8* value = operand;
				operand = SkipTermArgument(operand, end, scope, depth + 1);
				if (operand <= value) return Fail(Status::MalformedObject, opcode);
				if (Define(scope, name, ObjectType::Name, flags, value, stduint(operand - value)) == InvalidIndex) return Fail(Status::Capacity, opcode);
				cursor = operand;
				continue;
			}
			if (*cursor == 0x10 || *cursor == 0x14) {
				const uint8* content = nullptr;
				const uint8* object_end = nullptr;
				if (!PackageRange(cursor + 1, end, content, object_end)) return Fail(Status::MalformedPackage, opcode);
				AMLBytecode::Name name = {};
				if (!AMLBytecode::DecodeName(content, object_end, name)) return Fail(Status::MalformedName, content);
				if (*cursor == 0x10) {
					const stduint target = ResolveOrCreateScope(scope, name, flags);
					if (target == InvalidIndex) return Fail(Status::Capacity, opcode);
					const Status status = ParseTermList(content, object_end, target, flags, depth + 1);
					if (status != Status::Success) return status;
				}
				else {
					if (content >= object_end) return Fail(Status::MalformedObject, opcode);
					const uint8 method_flags = *content++;
					const stduint method_index = Define(scope, name, ObjectType::Method, flags, content, stduint(object_end - content));
					if (method_index == InvalidIndex) return Fail(Status::Capacity, opcode);
					objects[method_index].argument_count = method_flags & 0x07u;
					objects[method_index].method_flags = method_flags;
				}
				cursor = object_end;
				continue;
			}
			if (*cursor == 0x15) {
				const uint8* operand = cursor + 1;
				AMLBytecode::Name name = {};
				if (!AMLBytecode::DecodeName(operand, end, name) || end - operand < 2) return Fail(Status::MalformedObject, opcode);
				const stduint external_index = Define(scope, name, ObjectType::External, flags, operand, 2);
				if (external_index == InvalidIndex) return Fail(Status::Capacity, opcode);
				objects[external_index].external_type = operand[0];
				objects[external_index].argument_count = operand[1];
				cursor = operand + 2;
				continue;
			}
			if (*cursor == 0x8A || *cursor == 0x8B || *cursor == 0x8C || *cursor == 0x8D || *cursor == 0x8F) {
				const uint8* operand = cursor + 1;
				const uint8* value = cursor;
				for0(index, 2) operand = SkipTermArgument(operand, end, scope, depth + 1);
				AMLBytecode::Name name = {};
				if (!AMLBytecode::DecodeName(operand, end, name)) return Fail(Status::MalformedName, operand);
				if (Define(scope, name, ObjectType::BufferField, flags, value, stduint(operand - value)) == InvalidIndex) return Fail(Status::Capacity, opcode);
				cursor = operand;
				continue;
			}
			if (*cursor == 0xA0 || *cursor == 0xA1 || *cursor == 0xA2) {
				const uint8* content = nullptr;
				const uint8* object_end = nullptr;
				if (!PackageRange(cursor + 1, end, content, object_end)) return Fail(Status::MalformedPackage, opcode);
				const uint8 nested_flags = flags | ObjectFlagConditional;
				++conditional_count;
				if (*cursor != 0xA1) content = SkipTermArgument(content, object_end, scope, depth + 1);
				const Status status = ParseTermList(content, object_end, scope, nested_flags, depth + 1);
				if (status != Status::Success) return status;
				cursor = object_end;
				continue;
			}
			if (*cursor == 0x11 || *cursor == 0x12 || *cursor == 0x13) {
				cursor = AMLBytecode::SkipSimpleDataObject(cursor, end);
				continue;
			}
			if (*cursor == 0x5B) {
				const Status status = ParseExtended(cursor, end, scope, flags, depth);
				if (status != Status::Success) return status;
				continue;
			}
			const uint8* next = SkipTermArgument(cursor, end, scope, depth + 1);
			cursor = next > cursor ? next : cursor + 1;
		}
		return Status::Success;
	}

	AMLNamespace::Status AMLNamespace::Load(const uint8* aml, stduint length, uint16 table_index) {
		if (!objects || !capacity || !count || !aml || !length) return Fail(Status::InvalidArgument, aml);
		table_begin = aml;
		table_end = aml + length;
		current_table = table_index;
		last_error = {};
		const Status status = ParseTermList(table_begin, table_end, 0, ObjectFlagNone, 0);
		if (status == Status::Success) last_error.status = Status::Success;
		return status;
	}

	bool AMLNamespace::FormatPath(stduint index, char* path, stduint path_capacity) const {
		if (!objects || index >= count || !path || !path_capacity) return false;
		path[0] = 0;
		if (!index) {
			if (path_capacity < 2) return false;
			path[0] = '\\';
			path[1] = 0;
			return true;
		}
		stduint chain[MaximumDepth] = {};
		stduint depth = 0;
		while (index && index < count && depth < MaximumDepth) {
			chain[depth++] = index;
			index = objects[index].parent;
		}
		if (index || depth >= MaximumDepth || path_capacity < 2) return false;
		stduint used = 0;
		path[used++] = '\\';
		for (stduint level = depth; level; --level) {
			if (used > 1) {
				if (used + 1 >= path_capacity) return false;
				path[used++] = '.';
			}
			for0(character, 4) {
				if (used + 1 >= path_capacity) return false;
				path[used++] = objects[chain[level - 1]].name[character];
			}
		}
		path[used] = 0;
		return true;
	}

}
