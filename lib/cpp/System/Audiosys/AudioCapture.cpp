// ASCII CPP-ISO11 TAB4 LF
// Docutitle: Audio capture buffer management
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../inc/cpp/System/Audiosys/AudioCapture.hpp"
#include "../../../../inc/c/ustring.h"

namespace uni {
	void AudioDMACapture::FreeBuffers() {
		if (raw) allocator.deallocate(raw, stduint(half_bytes) * 2);
		if (states) allocator.deallocate(states, count);
		raw = nullptr;
		states = nullptr;
		buffers = nullptr;
		count = half_bytes = 0;
	}
	AudioDMACapture::~AudioDMACapture() {
		// A failed hardware stop must leave the DMA staging allocation pinned.
		if (!running) FreeBuffers();
	}
	AudioResult AudioDMACapture::Start(AudioCaptureMode requested_mode, AudioBuffer* output, uint32 size) {
		if (running) return AudioResult::Busy;
		for (uint32 i = 0; i < count; ++i) if (states[i]) return AudioResult::Busy;
		if (!output || !size || (requested_mode != AudioCaptureMode::Once &&
			requested_mode != AudioCaptureMode::Continuous) ||
			(requested_mode == AudioCaptureMode::Continuous && size < 2) ||
			(_IMM(output) & 31U) || size > (stduint)-1 / sizeof(AudioBuffer) ||
			_IMM(output) > (stduint)-1 - stduint(size) * sizeof(AudioBuffer))
			return AudioResult::InvalidArgument;
		const AudioFormat& format = output[0].format;
		if (format.sample_format != AudioSampleFormat::S16LE || format.channels != 2 ||
			!format.sample_rate) return AudioResult::Unsupported;
		uint32 bytes = output[0].capacity;
		// Both 16-bit SAI and 32-bit SPDIF DMA counts must fit NDTR.
		if (!bytes || (bytes & 31U) || bytes > 0xFFFFU) return AudioResult::InvalidArgument;
		for (uint32 i = 0; i < size; ++i) {
			const AudioBuffer& buffer = output[i];
			if (!buffer.data || (_IMM(buffer.data) & 31U) || buffer.capacity != bytes ||
				buffer.format.sample_format != format.sample_format ||
				buffer.format.channels != format.channels || buffer.format.sample_rate != format.sample_rate ||
				_IMM(buffer.data) > (stduint)-1 - bytes) return AudioResult::InvalidArgument;
			if (_IMM(buffer.data) < _IMM(output) + stduint(size) * sizeof(AudioBuffer) &&
				_IMM(output) < _IMM(buffer.data) + bytes) return AudioResult::InvalidArgument;
			for (uint32 j = 0; j < i; ++j)
				if (_IMM(buffer.data) < _IMM(output[j].data) + bytes &&
					_IMM(output[j].data) < _IMM(buffer.data) + bytes) return AudioResult::InvalidArgument;
		}
		FreeBuffers();
		half_bytes = bytes;
		count = size;
		states = (byte*)allocator.allocate(count);
		raw = (byte*)allocator.allocate(stduint(bytes) * 2, 5);
		if (!states || !raw || (_IMM(raw) & 31U)) {
			FreeBuffers();
			return AudioResult::OutOfMemory;
		}
		buffers = output;
		MemSet(states, 0, count);
		for (uint32 i = 0; i < count; ++i) buffers[i].byte_count = 0;
		read_index = write_index = consumed = 0;
		produced.store(0, MemoryOrder_Release);
		source_error.store(false, MemoryOrder_Release);
		finishing = false;
		mode = requested_mode;
		result = AudioResult::OK;
		running = true;
		AudioResult started = BeginSource();
		if (started != AudioResult::OK) {
			result = started;
			// BeginSource must remain safely stoppable even after partial initialization.
			if (EndSource() == AudioResult::OK) {
				running = false;
				FreeBuffers();
			}
			return started;
		}
		return AudioResult::OK;
	}
	void AudioDMACapture::NotifyHalf(bool second) {
		uint32 sequence = produced.load(MemoryOrder_Relaxed);
		if (bool(sequence & 1U) != second) NotifyError();
		produced.fetch_add(1, MemoryOrder_Release);
	}
	AudioResult AudioDMACapture::ServiceCapture() {
		if (!running) return result;
		if (finishing) {
			AudioResult stopped = Stop();
			return stopped == AudioResult::OK ? result : stopped;
		}
		if (source_error.load(MemoryOrder_Acquire) || SourceFailed()) {
			result = AudioResult::Failed;
			Stop();
			return result;
		}
		uint32 ready = produced.load(MemoryOrder_Acquire);
		if (ready == consumed) return AudioResult::WouldBlock;
		bool second = bool(consumed & 1U);
		if (ready - consumed != 1 || states[write_index] || !IsHalfReadable(second)) {
			result = AudioResult::Overrun;
			Stop();
			return result;
		}
		AudioBuffer& buffer = buffers[write_index];
		ConvertSamples(raw + (second ? half_bytes : 0), (byte*)buffer.data, half_bytes);
		// Do not publish a copy if DMA reused its source while the foreground was copying.
		if (source_error.load(MemoryOrder_Acquire) || produced.load(MemoryOrder_Acquire) != ready ||
			!IsHalfReadable(second)) {
			result = AudioResult::Overrun;
			Stop();
			return result;
		}
		++consumed;
		buffer.byte_count = half_bytes;
		states[write_index] = 1;
		write_index = (write_index + 1) % count;
		if (mode == AudioCaptureMode::Once) return Stop();
		return AudioResult::OK;
	}
	AudioResult AudioDMACapture::AcquireBuffer(const AudioBuffer*& buffer) {
		buffer = nullptr;
		if (count && states[read_index] == 1) {
			states[read_index] = 2;
			buffer = &buffers[read_index];
			read_index = (read_index + 1) % count;
			return AudioResult::OK;
		}
		if (result != AudioResult::OK && result != AudioResult::EndOfStream) return result;
		return running ? AudioResult::WouldBlock : AudioResult::EndOfStream;
	}
	AudioResult AudioDMACapture::ReturnBuffer(const AudioBuffer& buffer) {
		for (uint32 i = 0; i < count; ++i) {
			if (&buffers[i] != &buffer) continue;
			if (states[i] != 2) return AudioResult::InvalidArgument;
			states[i] = 0;
			buffers[i].byte_count = 0;
			return AudioResult::OK;
		}
		return AudioResult::InvalidArgument;
	}
	AudioResult AudioDMACapture::Stop() {
		if (!running) return AudioResult::OK;
		finishing = true;
		AudioResult stopped = EndSource();
		if (stopped != AudioResult::OK) return stopped;
		running = false;
		if (result == AudioResult::OK) result = AudioResult::EndOfStream;
		return AudioResult::OK;
	}
}
