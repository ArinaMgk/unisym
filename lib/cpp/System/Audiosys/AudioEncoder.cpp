// ASCII CPP-ISO11 TAB4 LF
// Docutitle: Audio encoder selection and raw stream writing
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../inc/cpp/System/Audiosys/AudioEncoder.hpp"
#include "../../../../inc/c/format/audio/WAV.h"

namespace uni {
	namespace {
		class PCMEncoder : public IAudioEncoder {
			StorageTrait& output;
			trait::Malloc& allocator;
			uint32 frame;
			stduint offset = 0;
			bool failed = false, finished = false;
		public:
			PCMEncoder(StorageTrait& output, uint32 frame, trait::Malloc& allocator)
				: output(output), allocator(allocator), frame(frame) {}
			void Release() override {
				trait::Malloc& memory = allocator;
				this->~PCMEncoder();
				memory.deallocate(this, sizeof(PCMEncoder));
			}
			AudioResult WriteSamples(const AudioBufferView& samples) override {
				if (failed) return AudioResult::IoError;
				if (finished) return AudioResult::InvalidArgument;
				if (!samples.byte_count) return AudioResult::OK;
				if (!samples.data || samples.byte_count % frame) return AudioResult::InvalidArgument;
				if (samples.byte_count > (stduint)-1 - offset) return AudioResult::Unsupported;
				if (output.Write(offset, samples.data, samples.byte_count, nullptr) != samples.byte_count) {
					failed = true;
					return AudioResult::IoError;
				}
				offset += samples.byte_count;
				return AudioResult::OK;
			}
			AudioResult Finish() override {
				if (failed) return AudioResult::IoError;
				finished = true;
				return AudioResult::OK;
			}
		};
	}

	AudioResult OpenAudioEncoder(AudioContainerFormat container, StorageTrait& output,
		const AudioFormat& input, const AudioEncodeOptions& options,
		IAudioEncoder*& encoder, trait::Malloc& allocator) {
		encoder = nullptr;
		if (container == AudioContainerFormat::WAV) {
			WAVCodec codec;
			return codec.OpenEncoder(output, input, options, encoder, allocator);
		}
		if (container != AudioContainerFormat::PcmRaw) return AudioResult::Unsupported;
		uint32 frame = AudioFrameBytes(input);
		if (!frame) return AudioResult::InvalidFormat;
		if (options.bit_rate || options.quality || output.Block_Size != 1) return AudioResult::Unsupported;
		if (!output.writable) return AudioResult::IoError;
		void* memory = allocator.allocate(sizeof(PCMEncoder), 3);
		if (!memory) return AudioResult::OutOfMemory;
		encoder = new (memory) PCMEncoder(output, frame, allocator);
		return AudioResult::OK;
	}

	AudioResult WriteAudioPacket(StorageTrait& output, stduint& offset, const AudioPacketView& packet) {
		if (packet.encoding != AudioEncoding::IEC61937 || !packet.data ||
			!packet.byte_count || (packet.byte_count & 3U)) return AudioResult::InvalidArgument;
		if (output.Block_Size != 1) return AudioResult::Unsupported;
		if (!output.writable) return AudioResult::IoError;
		if (packet.byte_count > (stduint)-1 - offset) return AudioResult::InvalidArgument;
		// A short write is terminal for the caller's recording session.
		if (output.Write(offset, packet.data, packet.byte_count, nullptr) != packet.byte_count)
			return AudioResult::IoError;
		offset += packet.byte_count;
		return AudioResult::OK;
	}
}
