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

// upgraded from strbuff

#ifndef _INCPP_DATYPE_BUFFER_CHUNKBUFFER
#define _INCPP_DATYPE_BUFFER_CHUNKBUFFER

#include "../../unisym"

namespace uni {

	class ChunkBuffer {
	public:
		explicit ChunkBuffer(stduint chunk_size = 0x10000);
		~ChunkBuffer();

		ChunkBuffer(const ChunkBuffer&) = delete;
		ChunkBuffer& operator=(const ChunkBuffer&) = delete;

		ChunkBuffer(ChunkBuffer&& other) noexcept;
		ChunkBuffer& operator=(ChunkBuffer&& other) noexcept;

		bool Reserve(stduint capacity);
		bool Append(const void* source, stduint size);
		bool AppendZero(stduint size);

		bool Read(stduint offset, void* destination, stduint size) const;
		bool Write(stduint offset, const void* source, stduint size);
		bool WriteZero(stduint offset, stduint size);

		byte* GetSpan(stduint offset, stduint& length);
		const byte* GetSpan(stduint offset, stduint& length) const;

		void Reset();
		void Clear();

		stduint Size() const { return data_size; }
		stduint Capacity() const { return chunk_count * chunk_size; }
		stduint ChunkSize() const { return chunk_size; }
		stduint ChunkCount() const { return chunk_count; }
		bool IsEmpty() const { return data_size == 0; }

	private:
		byte** chunks;
		stduint chunk_count;
		stduint chunk_pointer_capacity;
		stduint chunk_size;
		stduint data_size;

		static bool AddSize(stduint left, stduint right, stduint& result);
		bool EnsureChunkPointers(stduint count);
		bool EnsureCapacity(stduint capacity);
		void CopyIn(stduint offset, const byte* source, stduint size);
		void CopyOut(stduint offset, byte* destination, stduint size) const;
		void FillZero(stduint offset, stduint size);
	};

}

#endif
