// ASCII CPP-ISO11 TAB4 LF
// Docutitle: Audio capture buffer management
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#ifndef _INCPP_AUDIOSYS_CAPTURE
#define _INCPP_AUDIOSYS_CAPTURE
#include "../Audiosys.hpp"
#include "../../atomic"

namespace uni {
	// Foreground methods are serialized by the caller. IRQs only publish completion counters.
	class AudioDMACapture : public IAudioCapture {
		trait::Malloc& allocator;
		AudioBuffer* buffers = nullptr;
		byte* states = nullptr;
		uint32 count = 0, read_index = 0, write_index = 0, consumed = 0;
		Atomic<uint32> produced{0};
		Atomic<bool> source_error{false};
		AudioCaptureMode mode = AudioCaptureMode::Once;
		AudioResult result = AudioResult::EndOfStream;
		void FreeBuffers();
	protected:
		byte* raw = nullptr;
		uint32 half_bytes = 0;
		bool running = false;
		bool finishing = false;
		const AudioFormat& getCaptureFormat() const { return buffers[0].format; }
		virtual AudioResult BeginSource() = 0;
		virtual AudioResult EndSource() = 0;
		virtual bool IsHalfReadable(bool second) const = 0;
		virtual bool SourceFailed() const = 0;
		virtual void ConvertSamples(const byte* source, byte* dest, uint32 bytes) = 0;
		void NotifyHalf(bool second);
		void NotifyError() { source_error.store(true, MemoryOrder_Release); }
	public:
		explicit AudioDMACapture(trait::Malloc& allocator) : allocator(allocator) {}
		// Stop must succeed before destroying a live capture or freeing its output buffers.
		virtual ~AudioDMACapture();
		AudioDMACapture(const AudioDMACapture&) = delete;
		AudioDMACapture& operator=(const AudioDMACapture&) = delete;
		AudioResult Start(AudioCaptureMode mode, AudioBuffer* buffers, uint32 count) override;
		AudioResult AcquireBuffer(const AudioBuffer*& buffer) override;
		AudioResult ReturnBuffer(const AudioBuffer& buffer) override;
		AudioResult ServiceCapture() override;
		AudioResult Stop() override;
	};
}
#endif
