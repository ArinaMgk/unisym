// ASCII CPP TAB4 CRLF
// Docutitle: (System) Video
// Codifiers: @dosconio: 20240513, @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Bit-32mode Non-Dependence
// Copyright: UNISYM, under Apache License 2.0; Dosconio Mecocoa, BSD 3-Clause License
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

#ifndef _INCPP_System_Videosys
#define _INCPP_System_Videosys

#include "../../c/stdinc.h"
#include "../../c/graphic/color.h"
#include "../trait/StorageTrait.hpp"
#include "../trait/MallocTrait.hpp"
#include "Audiosys.hpp"
#include "Picture.hpp"

namespace uni {

	enum class VideoResult {
		OK,
		Failed,
		Unsupported,
		InvalidFormat,
		InvalidArgument,
		OutOfMemory,
		IoError,
		NotFound,
		AccessDenied,
		BufferTooSmall,
		EndOfStream,
	};

	enum class VideoContainerFormat {
		Unknown,
		AVI,
		MP4,
		MPEG,
		WEBM,
		MKV,
		FLV,
	};

	enum class VideoCodecType {
		Unknown,
		MJPEG,
		MPEG1,
		MPEG2,
		MPEG4,
		H264,
		RGB,
		RAW,
	};

	struct VideoFormat {
		uint32          width = 0;
		uint32          height = 0;
		uint32          frame_rate_num = 0; // Numerator of frame rate (e.g. 30000 or 30)
		uint32          frame_rate_den = 1; // Denominator of frame rate (e.g. 1001 or 1)
		VideoCodecType  codec_type = VideoCodecType::Unknown;
		PixelFormat     pixel_format = PixelFormat::BGRA8888;
		uint32          bit_rate = 0;
	};

	struct VideoInfo {
		VideoFormat          videoFormat;
		VideoContainerFormat containerFormat = VideoContainerFormat::Unknown;
		uint32               durationMs = 0;
		uint32               totalFrames = 0;
		bool                 hasAudio = false;
		AudioInfo            audioInfo{};
	};

	struct VideoFrame {
		ImageBuffer image{};
		uint32      width = 0;
		uint32      height = 0;
		uint32      timestampMs = 0;
		uint32      frameIndex = 0;
		bool        isKeyFrame = true;
	};

	inline void VideoFrameClear(VideoFrame& frame) {
		ImageBufferClear(frame.image);
		frame.width = 0;
		frame.height = 0;
		frame.timestampMs = 0;
		frame.frameIndex = 0;
		frame.isKeyFrame = true;
	}

	inline void VideoFrameFree(VideoFrame& frame) {
		if (frame.image.pixels) {
			if (frame.image.allocator) {
				frame.image.allocator->deallocate(frame.image.pixels, frame.image.size);
			} else {
				free(frame.image.pixels);
			}
			frame.image.pixels = nullptr;
		}
		ImageBufferClear(frame.image);
		VideoFrameClear(frame);
	}

	class IVideoObject {
	public:
		// Release this object. Caller must not use this object after Release().
		virtual void Release() = 0;

	protected:
		virtual ~IVideoObject() = default;
	};

	class IVideoStream : public IVideoObject {
	public:
		// Get video and embedded audio metadata.
		virtual VideoResult GetInfo(VideoInfo& outInfo) const = 0;

		// Decode and read the next video frame into outFrame.
		virtual VideoResult ReadVideoFrame(VideoFrame& outFrame, trait::Malloc& allocator) = 0;

		// Read interleaved audio samples if audio stream is present.
		virtual VideoResult ReadAudioSamples(void* destBuffer, uint32 maxBytes, uint32& bytesRead) = 0;

		// Seek by frame index.
		virtual VideoResult SeekFrame(uint32 frameIndex) = 0;

		// Seek by timestamp in milliseconds.
		virtual VideoResult SeekTime(uint32 timestampMs) = 0;

	protected:
		virtual ~IVideoStream() = default;
	};

	class IVideoCodec {
	public:
		virtual ~IVideoCodec() = default;

		virtual const char* GetName() const = 0;
		virtual VideoContainerFormat GetFormat() const = 0;
		virtual const char* const* GetExtensions() const = 0;

		virtual VideoResult Probe(StorageTrait& storage, bool& matched) const = 0;
		virtual VideoResult ReadInfo(StorageTrait& storage, VideoInfo& outInfo) const = 0;
		virtual VideoResult OpenStream(
			StorageTrait& storage,
			IVideoStream*& outStream,
			trait::Malloc& allocator
		) const = 0;
	};

	class IVideoCodecManager {
	public:
		virtual ~IVideoCodecManager() = default;

		virtual VideoResult RegisterCodec(IVideoCodec* codec) = 0;
		virtual VideoResult UnregisterCodec(IVideoCodec* codec) = 0;
		virtual VideoResult FindDecoder(StorageTrait& storage, const IVideoCodec*& outCodec) const = 0;
		virtual VideoResult FindEncoder(VideoContainerFormat format, const IVideoCodec*& outCodec) const = 0;
		virtual uint32 GetCodecCount() const = 0;
		virtual const IVideoCodec* GetCodec(uint32 index) const = 0;
	};

}

// Inn

// Out

namespace uni {

	enum class HostVideoState {
		Idle,
		Playing,
		Paused,
		Stopped,
		Error,
	};

	class HostVideo {
	public:
		typedef void (*FinishedHandler)(HostVideo* sender, void* user_data);
		typedef void (*ProgressHandler)(HostVideo* sender, uint32 current_ms, uint32 total_ms, void* user_data);

		FinishedHandler OnFinished;
		ProgressHandler OnProgress;
		void*           user_data;

	private:
		void*          impl;
		HostVideoState state;
		uint32         volume;
		bool           is_loop;
		bool           is_muted;

	public:
		HostVideo();
		HostVideo(const char* filepath, bool loop_mode = false);
		HostVideo(StorageTrait& storage, bool loop_mode = false);
		HostVideo(IVideoStream* stream, bool loop_mode = false);
		~HostVideo();

		explicit operator bool() const;

		bool Open(const char* filepath, bool loop_mode = false);
		bool Open(StorageTrait& storage, bool loop_mode = false);
		bool OpenStream(IVideoStream* stream, bool loop_mode = false);
		void Close();

		bool Play();
		bool Pause();
		bool Resume();
		void Stop();

		bool Seek(uint32 target_ms);
		bool SeekFrame(uint32 frame_index);

		// Non-blocking tick for audio pumping and stream progress
		bool Update();

		// Synchronize and render/fetch current video frame if ready.
		// Returns true if a new frame was decoded and copied to destPixels.
		bool UpdateFrame(uni::Color* destPixels, uint32& outTimestampMs);

		// Read interleaved audio samples if audio stream is present.
		VideoResult ReadAudioSamples(void* destBuffer, uint32 maxBytes, uint32& bytesRead);
		IVideoStream* GetStream() const;

		bool GetInfo(VideoInfo& outInfo) const;
		uint32 GetPositionMs() const;
		uint32 getDurationMs() const;
		uint32 GetCurrentFrameIndex() const;

		HostVideoState GetState() const { return state; }
		bool isPlaying() const { return state == HostVideoState::Playing; }
		bool isPaused() const { return state == HostVideoState::Paused; }
		bool isLoop() const { return is_loop; }
		bool isMuted() const { return is_muted; }
		uint32 getVolume() const { return volume; }

		void setVolume(uint32 vol_percent);
		void setMute(bool mute);
		void setLoop(bool loop_mode);
	};

} // namespace uni

#endif // _INCPP_System_Videosys

