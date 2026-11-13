// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: Chunked Byte Buffer
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Host[Allocation]
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

#include "../../../../inc/cpp/Datype/Buffer/ChunkBuffer.hpp"
#include "../../../../inc/c/ustring.h"

namespace uni {

	ChunkBuffer::ChunkBuffer(stduint requested_chunk_size)
		: chunks(nullptr), chunk_count(0), chunk_pointer_capacity(0),
		chunk_size(requested_chunk_size ? requested_chunk_size : 0x10000),
		data_size(0) {
	}

	ChunkBuffer::~ChunkBuffer() {
		Clear();
	}

	ChunkBuffer::ChunkBuffer(ChunkBuffer&& other) noexcept
		: chunks(other.chunks), chunk_count(other.chunk_count),
		chunk_pointer_capacity(other.chunk_pointer_capacity),
		chunk_size(other.chunk_size), data_size(other.data_size) {
		other.chunks = nullptr;
		other.chunk_count = 0;
		other.chunk_pointer_capacity = 0;
		other.data_size = 0;
	}

	ChunkBuffer& ChunkBuffer::operator=(ChunkBuffer&& other) noexcept {
		if (this == &other) return *this;

		Clear();
		chunks = other.chunks;
		chunk_count = other.chunk_count;
		chunk_pointer_capacity = other.chunk_pointer_capacity;
		chunk_size = other.chunk_size;
		data_size = other.data_size;

		other.chunks = nullptr;
		other.chunk_count = 0;
		other.chunk_pointer_capacity = 0;
		other.data_size = 0;
		return *this;
	}

	bool ChunkBuffer::AddSize(stduint left, stduint right, stduint& result) {
		if (left > ~_IMM0 - right) return false;
		result = left + right;
		return true;
	}

	bool ChunkBuffer::EnsureChunkPointers(stduint count) {
		if (count <= chunk_pointer_capacity) return true;

		const stduint maximum = ~_IMM0 / sizeof(byte*);
		if (count > maximum) return false;

		stduint capacity = chunk_pointer_capacity ?
			chunk_pointer_capacity : 4;
		while (capacity < count) {
			if (capacity > maximum / 2) {
				capacity = maximum;
				break;
			}
			capacity *= 2;
		}
		if (capacity < count) return false;

		byte** replacement = (byte**)malc(capacity * sizeof(byte*));
		if (!replacement) return false;
		if (chunk_count)
			MemCopyN(replacement, chunks, chunk_count * sizeof(byte*));
		memf(chunks);
		chunks = replacement;
		chunk_pointer_capacity = capacity;
		return true;
	}

	bool ChunkBuffer::EnsureCapacity(stduint capacity) {
		if (capacity <= Capacity()) return true;

		stduint required_chunks = capacity / chunk_size;
		if (capacity % chunk_size) {
			if (required_chunks == ~_IMM0) return false;
			++required_chunks;
		}
		if (required_chunks > ~_IMM0 / chunk_size) return false;
		if (!EnsureChunkPointers(required_chunks)) return false;

		while (chunk_count < required_chunks) {
			byte* chunk = (byte*)malc(chunk_size);
			if (!chunk) return false;
			chunks[chunk_count++] = chunk;
		}
		return true;
	}

	bool ChunkBuffer::Reserve(stduint capacity) {
		return EnsureCapacity(capacity);
	}

	void ChunkBuffer::CopyIn(stduint offset, const byte* source,
		stduint size) {
		while (size) {
			const stduint index = offset / chunk_size;
			const stduint inner_offset = offset % chunk_size;
			stduint length = chunk_size - inner_offset;
			if (length > size) length = size;
			MemCopyN(chunks[index] + inner_offset, source, length);
			offset += length;
			source += length;
			size -= length;
		}
	}

	void ChunkBuffer::CopyOut(stduint offset, byte* destination,
		stduint size) const {
		while (size) {
			const stduint index = offset / chunk_size;
			const stduint inner_offset = offset % chunk_size;
			stduint length = chunk_size - inner_offset;
			if (length > size) length = size;
			MemCopyN(destination, chunks[index] + inner_offset, length);
			offset += length;
			destination += length;
			size -= length;
		}
	}

	void ChunkBuffer::FillZero(stduint offset, stduint size) {
		while (size) {
			const stduint index = offset / chunk_size;
			const stduint inner_offset = offset % chunk_size;
			stduint length = chunk_size - inner_offset;
			if (length > size) length = size;
			MemSet(chunks[index] + inner_offset, 0, length);
			offset += length;
			size -= length;
		}
	}

	bool ChunkBuffer::Append(const void* source, stduint size) {
		return Write(data_size, source, size);
	}

	bool ChunkBuffer::AppendZero(stduint size) {
		return WriteZero(data_size, size);
	}

	bool ChunkBuffer::Read(stduint offset, void* destination,
		stduint size) const {
		if (!size) return offset <= data_size;
		if (!destination) return false;

		stduint end;
		if (!AddSize(offset, size, end) || end > data_size) return false;
		CopyOut(offset, (byte*)destination, size);
		return true;
	}

	bool ChunkBuffer::Write(stduint offset, const void* source,
		stduint size) {
		if (!size) return offset <= data_size;
		if (!source || offset > data_size) return false;

		stduint end;
		if (!AddSize(offset, size, end) || !EnsureCapacity(end)) return false;
		CopyIn(offset, (const byte*)source, size);
		if (end > data_size) data_size = end;
		return true;
	}

	bool ChunkBuffer::WriteZero(stduint offset, stduint size) {
		if (!size) return offset <= data_size;
		if (offset > data_size) return false;

		stduint end;
		if (!AddSize(offset, size, end) || !EnsureCapacity(end)) return false;
		FillZero(offset, size);
		if (end > data_size) data_size = end;
		return true;
	}

	byte* ChunkBuffer::GetSpan(stduint offset, stduint& length) {
		if (offset >= data_size) {
			length = 0;
			return nullptr;
		}

		const stduint inner_offset = offset % chunk_size;
		length = chunk_size - inner_offset;
		if (length > data_size - offset) length = data_size - offset;
		return chunks[offset / chunk_size] + inner_offset;
	}

	const byte* ChunkBuffer::GetSpan(stduint offset,
		stduint& length) const {
		if (offset >= data_size) {
			length = 0;
			return nullptr;
		}

		const stduint inner_offset = offset % chunk_size;
		length = chunk_size - inner_offset;
		if (length > data_size - offset) length = data_size - offset;
		return chunks[offset / chunk_size] + inner_offset;
	}

	void ChunkBuffer::Reset() {
		data_size = 0;
	}

	void ChunkBuffer::Clear() {
		for (stduint index = 0; index < chunk_count; ++index)
			memf(chunks[index]);
		memf(chunks);
		chunks = nullptr;
		chunk_count = 0;
		chunk_pointer_capacity = 0;
		data_size = 0;
	}

}
