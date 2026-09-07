// UTF-8 CPP-ISO11 TAB4 CRLF
// Docutitle: (Device) Audio Common
// Codifiers: @dosconio, @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#ifndef _INCPP_Device_Audio
#define _INCPP_Device_Audio

#include "../../c/stdinc.h"
#include "../trait/StorageTrait.hpp"
#include "../trait/MallocTrait.hpp"

namespace uni {

	enum class AudioSampleFormat : uint8 {
		U8,
		S16LE,
	};

	enum class AudioResult {
		OK,
		Failed,
		Unsupported,
		InvalidFormat,
		InvalidArgument,
		OutOfMemory,
		IoError,
		EndOfStream,
	};

	enum class AudioContainerFormat {
		Unknown,
		WAV,
		PcmRaw,
		MIDI,
		MP3,
		FLAC,
		OGG,
	};

	struct AudioFormat {
		AudioSampleFormat sample_format = AudioSampleFormat::U8;
		uint16 channels = 1;
		uint32 sample_rate = 0;
	};

	struct AudioBufferView {
		const void* data = nullptr;
		uint32 byte_count = 0;
	};

	struct AudioPlayRequest {
		AudioFormat format = {};
		AudioBufferView buffer = {};
	};

	struct AudioInfo {
		AudioFormat format;
		AudioContainerFormat containerFormat;
		uint32 bitsPerSample;
		uint32 dataByteOffset;
		uint32 dataByteLength;
		uint32 totalSamples;
		uint32 durationMs;
	};

	class IAudioObject {
	public:
		virtual void Release() = 0;

	protected:
		virtual ~IAudioObject() = default;
	};

	class IAudioStream : public IAudioObject {
	public:
		virtual AudioResult GetInfo(AudioInfo& outInfo) const = 0;
		virtual AudioResult ReadSamples(void* destBuffer, uint32 maxBytes, uint32& bytesRead) = 0;
		virtual AudioResult Seek(uint32 sampleIndex) = 0;

	protected:
		virtual ~IAudioStream() = default;
	};

	class IAudioCodec {
	public:
		virtual ~IAudioCodec() = default;

		virtual const char* GetName() const = 0;
		virtual AudioContainerFormat GetFormat() const = 0;
		virtual const char* const* GetExtensions() const = 0;

		virtual AudioResult Probe(StorageTrait& storage, bool& matched) const = 0;
		virtual AudioResult ReadInfo(StorageTrait& storage, AudioInfo& outInfo) const = 0;
		virtual AudioResult OpenStream(
			StorageTrait& storage,
			IAudioStream*& outStream,
			trait::Malloc& allocator
		) const = 0;
	};

	class IAudioCodecManager {
	public:
		virtual ~IAudioCodecManager() = default;

		virtual AudioResult RegisterCodec(IAudioCodec* codec) = 0;
		virtual AudioResult UnregisterCodec(IAudioCodec* codec) = 0;
		virtual AudioResult FindDecoder(StorageTrait& storage, const IAudioCodec*& outCodec) const = 0;
		virtual AudioResult FindEncoder(AudioContainerFormat format, const IAudioCodec*& outCodec) const = 0;
		virtual uint32 GetCodecCount() const = 0;
		virtual const IAudioCodec* GetCodec(uint32 index) const = 0;
	};

	class ResamplerStream : public IAudioStream {
	private:
		IAudioStream*       source;
		AudioInfo           info;
		uint32              source_rate;
		uint32              target_rate;
		uint32              step;         // 16.16 fixed point: (source_rate << 16) / target_rate
		uint32              phase;        // 16.16 accumulator: fraction between current and next frame
		trait::Malloc*      allocator;
		byte*               source_buf;
		uint32              source_buf_cap;
		uint32              source_buf_pos;
		uint32              source_buf_valid;
		int32               curr_frame[2]; // Cached current frame samples (L, R)
		int32               next_frame[2]; // Cached next frame samples (L, R)
		bool                has_frames;
		bool                source_eos;

		bool FetchSourceFrame(int32* frame);

	public:
		ResamplerStream(IAudioStream* src, uint32 targetRate, trait::Malloc& alloc);
		virtual ~ResamplerStream();

		bool IsValid() const;
		virtual void Release() override;
		virtual AudioResult GetInfo(AudioInfo& outInfo) const override;
		virtual AudioResult ReadSamples(void* destBuffer, uint32 maxBytes, uint32& bytesRead) override;
		virtual AudioResult Seek(uint32 sampleIndex) override;
	};

}

// Inn

// Out

// HostMusic

namespace uni {
	class HostMusic {

	};
}

#endif
