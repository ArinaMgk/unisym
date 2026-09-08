// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: Audio Resampler Implementation
// Codifiers: @dosconio, @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../inc/cpp/System/Audiosys.hpp"
#include "../../../../inc/c/ustring.h"


namespace {
	constexpr uint32 kSourceChunkSize = 4096;

	static inline int32 ClampS16(int32 val) {
		if (val > 32767) return 32767;
		if (val < -32768) return -32768;
		return val;
	}

	static inline uint8 ClampU8(int32 val) {
		if (val > 255) return 255;
		if (val < 0) return 0;
		return (uint8)val;
	}
}

uni::ResamplerStream::ResamplerStream(
	uni::IAudioStream* src, uint32 targetRate, uni::trait::Malloc& alloc)
	: source(src), allocator(&alloc), source_buf(nullptr),
	  source_buf_cap(kSourceChunkSize), source_buf_pos(0),
	  source_buf_valid(0), has_frames(false), source_eos(false) {
	curr_frame[0] = curr_frame[1] = 0;
	next_frame[0] = next_frame[1] = 0;
	phase = 0;

	if (source) {
		source->GetInfo(info);
		source_rate = info.format.sample_rate ? info.format.sample_rate : 44100;
	} else {
		source_rate = 44100;
	}
	target_rate = targetRate ? targetRate : 44100;
	step = uint32(((uint64)source_rate << 16) / target_rate);

	if (allocator) {
		source_buf = (byte*)allocator->allocate(source_buf_cap);
	}
}

uni::ResamplerStream::~ResamplerStream() {
	if (source_buf && allocator) {
		allocator->deallocate(source_buf);
		source_buf = nullptr;
	}
	if (source) {
		source->Release();
		source = nullptr;
	}
}

bool uni::ResamplerStream::IsValid() const {
	return source != nullptr && source_buf != nullptr && step > 0;
}

void uni::ResamplerStream::Release() {
	uni::trait::Malloc* alloc = allocator;
	this->~ResamplerStream();
	if (alloc) {
		alloc->deallocate(this);
	}
}

uni::AudioResult uni::ResamplerStream::GetInfo(uni::AudioInfo& outInfo) const {
	outInfo = info;
	outInfo.format.sample_rate = target_rate;
	if (source_rate) {
		outInfo.totalSamples =
			uint32(((uint64)info.totalSamples * target_rate) / source_rate);
		uint32 bytes_per_sample =
			(info.bitsPerSample / 8) * (info.format.channels ? info.format.channels : 1);
		outInfo.dataByteLength = outInfo.totalSamples * (bytes_per_sample ? bytes_per_sample : 1);
	}
	return uni::AudioResult::OK;
}

bool uni::ResamplerStream::FetchSourceFrame(int32* frame) {
	const uint8 channels = info.format.channels ? (uint8)info.format.channels : 1;
	const bool is_16bit = (info.format.sample_format == uni::AudioSampleFormat::S16LE);
	const uint32 bytes_per_frame = (is_16bit ? 2 : 1) * channels;

	while (source_buf_pos + bytes_per_frame > source_buf_valid) {
		if (source_eos) return false;

		uint32 remain = source_buf_valid - source_buf_pos;
		if (remain > 0 && source_buf_pos > 0) {
			MemCopyN(source_buf, source_buf + source_buf_pos, remain);
		}
		source_buf_pos = 0;
		source_buf_valid = remain;

		uint32 bytes_read = 0;
		uni::AudioResult res = source->ReadSamples(
			source_buf + source_buf_valid,
			source_buf_cap - source_buf_valid,
			bytes_read);
		if (res != uni::AudioResult::OK || bytes_read == 0) {
			source_eos = true;
			if (source_buf_valid < bytes_per_frame) return false;
			break;
		}
		source_buf_valid += bytes_read;
	}

	if (source_buf_pos + bytes_per_frame > source_buf_valid) {
		return false;
	}

	const byte* src_ptr = source_buf + source_buf_pos;
	if (is_16bit) {
		if (channels == 1) {
			int16 s = (int16)(src_ptr[0] | (src_ptr[1] << 8));
			frame[0] = s;
			frame[1] = s;
		} else {
			int16 l = (int16)(src_ptr[0] | (src_ptr[1] << 8));
			int16 r = (int16)(src_ptr[2] | (src_ptr[3] << 8));
			frame[0] = l;
			frame[1] = r;
		}
	} else {
		if (channels == 1) {
			int32 s = (int32)src_ptr[0] - 128;
			frame[0] = s;
			frame[1] = s;
		} else {
			int32 l = (int32)src_ptr[0] - 128;
			int32 r = (int32)src_ptr[1] - 128;
			frame[0] = l;
			frame[1] = r;
		}
	}

	source_buf_pos += bytes_per_frame;
	return true;
}

uni::AudioResult uni::ResamplerStream::ReadSamples(
	void* destBuffer, uint32 maxBytes, uint32& bytesRead) {
	bytesRead = 0;
	if (!destBuffer || !maxBytes || !source) {
		return uni::AudioResult::InvalidArgument;
	}

	const uint8 channels = info.format.channels ? (uint8)info.format.channels : 1;
	const bool is_16bit = (info.format.sample_format == uni::AudioSampleFormat::S16LE);
	const uint32 bytes_per_frame = (is_16bit ? 2 : 1) * channels;
	const uint32 max_out_frames = maxBytes / (bytes_per_frame ? bytes_per_frame : 1);
	if (!max_out_frames) return uni::AudioResult::InvalidArgument;

	if (!has_frames) {
		if (!FetchSourceFrame(curr_frame)) {
			return uni::AudioResult::EndOfStream;
		}
		if (!FetchSourceFrame(next_frame)) {
			next_frame[0] = curr_frame[0];
			next_frame[1] = curr_frame[1];
		}
		phase = 0;
		has_frames = true;
	}

	byte* out_ptr = (byte*)destBuffer;
	uint32 frames_written = 0;

	while (frames_written < max_out_frames) {
		const uint32 frac = phase & 0xFFFF;
		const int32 interp_l = curr_frame[0] +
			(int32)(((int64)(next_frame[0] - curr_frame[0]) * frac) >> 16);
		const int32 interp_r = curr_frame[1] +
			(int32)(((int64)(next_frame[1] - curr_frame[1]) * frac) >> 16);

		if (is_16bit) {
			const int16 out_l = (int16)ClampS16(interp_l);
			const int16 out_r = (int16)ClampS16(interp_r);
			if (channels == 1) {
				out_ptr[0] = (byte)(out_l & 0xFF);
				out_ptr[1] = (byte)((out_l >> 8) & 0xFF);
				out_ptr += 2;
			} else {
				out_ptr[0] = (byte)(out_l & 0xFF);
				out_ptr[1] = (byte)((out_l >> 8) & 0xFF);
				out_ptr[2] = (byte)(out_r & 0xFF);
				out_ptr[3] = (byte)((out_r >> 8) & 0xFF);
				out_ptr += 4;
			}
		} else {
			const uint8 out_l = ClampU8(interp_l + 128);
			const uint8 out_r = ClampU8(interp_r + 128);
			if (channels == 1) {
				*out_ptr++ = out_l;
			} else {
				*out_ptr++ = out_l;
				*out_ptr++ = out_r;
			}
		}
		++frames_written;

		phase += step;
		uint32 advance = phase >> 16;
		phase &= 0xFFFF;

		while (advance > 0) {
			curr_frame[0] = next_frame[0];
			curr_frame[1] = next_frame[1];
			if (!FetchSourceFrame(next_frame)) {
				next_frame[0] = curr_frame[0];
				next_frame[1] = curr_frame[1];
				if (source_eos && advance >= 1) {
					break;
				}
			}
			--advance;
		}

		if (source_eos && curr_frame[0] == next_frame[0] &&
			curr_frame[1] == next_frame[1] && advance > 0) {
			break;
		}
	}

	bytesRead = (uint32)(out_ptr - (byte*)destBuffer);
	if (bytesRead == 0 && source_eos) {
		return uni::AudioResult::EndOfStream;
	}
	return uni::AudioResult::OK;
}

uni::AudioResult uni::ResamplerStream::Seek(uint32 sampleIndex) {
	if (!source_rate) return uni::AudioResult::Failed;
	uint64 src_sample = ((uint64)sampleIndex * source_rate) / target_rate;
	uni::AudioResult res = source->Seek((uint32)src_sample);
	if (res != uni::AudioResult::OK) return res;

	has_frames = false;
	phase = 0;
	source_buf_pos = 0;
	source_buf_valid = 0;
	source_eos = false;
	curr_frame[0] = curr_frame[1] = 0;
	next_frame[0] = next_frame[1] = 0;
	return uni::AudioResult::OK;
}

