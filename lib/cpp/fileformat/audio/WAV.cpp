// ASCII C/C++ TAB4 CRLF
// Docutitle: RIFF WAVE Codec Implementation
// Attribute: Env-Freestanding Non-Dependence
// Copyright: UNISYM

#include "../../../../inc/c/format/audio/WAV.h"
#include "../../../../inc/c/ustring.h"
#include <stdlib.h>

#if defined(_INC_CPP) || defined(__cplusplus)

namespace {

	class WAVStream : public uni::IAudioStream {
	private:
		uni::StorageTrait* storage;
		uni::AudioInfo     info;
		uint32             dataByteOffset;
		uint32             dataByteLength;
		uint32             currentByteOffset;
		uni::trait::Malloc* allocator;
		byte*              sector_buf;
		stduint            cached_block;

	public:
		WAVStream(uni::StorageTrait& stg, const uni::AudioInfo& inf, uni::trait::Malloc& alloc)
			: storage(&stg), info(inf), dataByteOffset(inf.dataByteOffset),
			  dataByteLength(inf.dataByteLength), currentByteOffset(inf.dataByteOffset),
			  allocator(&alloc), sector_buf(nullptr), cached_block((stduint)~0) {
			stduint block_size = storage->Block_Size ? storage->Block_Size : 512;
			sector_buf = (byte*)allocator->allocate(block_size);
		}

		virtual ~WAVStream() {
			if (sector_buf && allocator) {
				allocator->deallocate(sector_buf);
				sector_buf = nullptr;
			}
		}

		bool IsValid() const {
			return sector_buf != nullptr;
		}

		virtual void Release() override {
			uni::trait::Malloc* alloc = allocator;
			this->~WAVStream();
			if (alloc) {
				alloc->deallocate(this);
			}
		}

		virtual uni::AudioResult GetInfo(uni::AudioInfo& outInfo) const override {
			outInfo = info;
			return uni::AudioResult::OK;
		}

		virtual uni::AudioResult ReadSamples(void* destBuffer, uint32 maxBytes, uint32& bytesRead) override {
			bytesRead = 0;
			if (!destBuffer || !maxBytes) return uni::AudioResult::InvalidArgument;
			if (!storage) return uni::AudioResult::IoError;
			if (currentByteOffset >= dataByteOffset + dataByteLength) {
				return uni::AudioResult::EndOfStream;
			}

			uint32 remain = (dataByteOffset + dataByteLength) - currentByteOffset;
			uint32 to_read = maxBytes > remain ? remain : maxBytes;
			stduint block_size = storage->Block_Size ? storage->Block_Size : 512;

			byte* out = (byte*)destBuffer;
			uint32 done = 0;

			while (done < to_read) {
				stduint cur_offset = currentByteOffset + done;
				stduint block_idx = cur_offset / block_size;
				stduint block_off = cur_offset % block_size;

				// Direct full sector read
				if (block_off == 0 && (to_read - done) >= block_size) {
					stduint num_blocks = (to_read - done) / block_size;
					if (!storage->Read(block_idx, out + done, num_blocks)) {
						if (done == 0) return uni::AudioResult::IoError;
						break;
					}
					done += (uint32)(num_blocks * block_size);
					continue;
				}

				// Cached boundary sector read
				if (cached_block != block_idx) {
					if (!sector_buf) return uni::AudioResult::OutOfMemory;
					if (!storage->Read(block_idx, sector_buf, 1)) {
						if (done == 0) return uni::AudioResult::IoError;
						break;
					}
					cached_block = block_idx;
				}

				stduint take = block_size - block_off;
				if (take > (to_read - done)) take = to_read - done;
				MemCopyN(out + done, sector_buf + block_off, take);
				done += (uint32)take;
			}

			currentByteOffset += done;
			bytesRead = done;
			return uni::AudioResult::OK;
		}

		virtual uni::AudioResult Seek(uint32 sampleIndex) override {
			uint32 bytes_per_sample = (info.bitsPerSample / 8) * info.format.channels;
			if (!bytes_per_sample) bytes_per_sample = 1;
			uint32 target_offset = dataByteOffset + sampleIndex * bytes_per_sample;
			if (target_offset > dataByteOffset + dataByteLength) {
				return uni::AudioResult::InvalidArgument;
			}
			currentByteOffset = target_offset;
			return uni::AudioResult::OK;
		}
	};

}

const char* uni::WAVCodec::GetName() const {
	return "WAV";
}

uni::AudioContainerFormat uni::WAVCodec::GetFormat() const {
	return uni::AudioContainerFormat::WAV;
}

const char* const* uni::WAVCodec::GetExtensions() const {
	static const char* const extensions[] = { "wav", "wave", nullptr };
	return extensions;
}

uni::AudioResult uni::WAVCodec::Probe(StorageTrait& storage, bool& matched) const {
	matched = false;

	WAVRIFFHEADER riff;
	stduint block_size = storage.Block_Size ? storage.Block_Size : 512;
	byte* block_buf = (byte*)malloc(block_size);
	if (!block_buf) {
		return uni::AudioResult::OutOfMemory;
	}

	stduint read_bytes = storage.Read(0, &riff, sizeof(riff), block_buf);
	free(block_buf);

	if (read_bytes == sizeof(riff) &&
		riff.riff_tag == WAV_FOURCC_RIFF &&
		riff.wave_tag == WAV_FOURCC_WAVE) {
		matched = true;
	}

	return uni::AudioResult::OK;
}

uni::AudioResult uni::WAVCodec::ReadInfo(StorageTrait& storage, AudioInfo& outInfo) const {
	bool matched = false;
	uni::AudioResult res = Probe(storage, matched);
	if (res != uni::AudioResult::OK) return res;
	if (!matched) return uni::AudioResult::InvalidFormat;

	stduint block_size = storage.Block_Size ? storage.Block_Size : 512;
	byte* block_buf = (byte*)malloc(block_size);
	if (!block_buf) {
		return uni::AudioResult::OutOfMemory;
	}

	stduint total_storage_size = storage.getUnits() * storage.Block_Size;
	stduint offset = sizeof(WAVRIFFHEADER);

	bool has_fmt = false;
	bool has_data = false;
	WAVFMTPCM fmt{};
	uint32 data_offset = 0;
	uint32 data_size = 0;

	while (offset + sizeof(WAVCHUNKHEADER) <= total_storage_size) {
		WAVCHUNKHEADER chunk;
		stduint rd = storage.Read(offset, &chunk, sizeof(chunk), block_buf);
		if (rd != sizeof(chunk)) break;

		uint32 chunk_data_offset = (uint32)(offset + sizeof(WAVCHUNKHEADER));
		uint32 padded_size = chunk.chunk_size + (chunk.chunk_size & 1u);

		if (chunk.chunk_tag == WAV_FOURCC_FMT) {
			if (chunk.chunk_size >= sizeof(WAVFMTPCM)) {
				rd = storage.Read(chunk_data_offset, &fmt, sizeof(fmt), block_buf);
				if (rd == sizeof(fmt)) {
					has_fmt = true;
				}
			}
		}
		else if (chunk.chunk_tag == WAV_FOURCC_DATA) {
			data_offset = chunk_data_offset;
			data_size = chunk.chunk_size;
			has_data = true;
			if (has_fmt) break;
		}

		offset = chunk_data_offset + padded_size;
	}

	free(block_buf);

	if (!has_fmt || !has_data) {
		return uni::AudioResult::InvalidFormat;
	}

	if (fmt.audio_format != WAV_FORMAT_PCM) {
		return uni::AudioResult::Unsupported;
	}

	outInfo.format.sample_format = (fmt.bits_per_sample == 8) ?
		uni::AudioSampleFormat::U8 : uni::AudioSampleFormat::S16LE;
	outInfo.format.channels = fmt.channel_count;
	outInfo.format.sample_rate = fmt.sample_rate;
	outInfo.containerFormat = uni::AudioContainerFormat::WAV;
	outInfo.bitsPerSample = fmt.bits_per_sample;
	outInfo.dataByteOffset = data_offset;
	outInfo.dataByteLength = data_size;

	uint32 bytes_per_sample = (fmt.bits_per_sample / 8) * fmt.channel_count;
	outInfo.totalSamples = bytes_per_sample ? (data_size / bytes_per_sample) : 0;
	outInfo.durationMs = fmt.sample_rate ?
		(uint32)(((uint64)outInfo.totalSamples * 1000) / fmt.sample_rate) : 0;

	return uni::AudioResult::OK;
}

uni::AudioResult uni::WAVCodec::OpenStream(
	StorageTrait& storage,
	IAudioStream*& outStream,
	trait::Malloc& allocator
) const {
	AudioInfo info{};
	uni::AudioResult res = ReadInfo(storage, info);
	if (res != uni::AudioResult::OK) return res;

	void* mem = allocator.allocate(sizeof(WAVStream));
	if (!mem) return uni::AudioResult::OutOfMemory;

	WAVStream* stream = new (mem) WAVStream(storage, info, allocator);
	if (!stream->IsValid()) {
		stream->Release();
		return uni::AudioResult::OutOfMemory;
	}
	outStream = stream;
	return uni::AudioResult::OK;
}

#endif
