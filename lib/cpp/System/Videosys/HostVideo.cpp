// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: HostVideo Video Player Implementation
// Codifiers: @dosconio, @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0; Dosconio Mecocoa, BSD 3-Clause License

#include "../../../../inc/cpp/System/Videosys.hpp"
#include "../../../../inc/c/format/video/AVI.h"
#include "../../../../inc/c/format/video/MPEG.h"
#include "../../../../inc/c/ustring.h"
#include <stdio.h>
#include <stdlib.h>

#if defined(_ACCM)
#include "../../../../../mecocoa/include/devsman.com.hpp"
#include "../../../../../mecocoa/include/taskman.com.hpp"
#include "aaaaa.h"
#endif

namespace {

	class DefaultHeapAllocator : public uni::trait::Malloc {
	public:
		virtual void* allocate(stduint size, stduint alignment = 0, stduint boundary = 0) override {
			return malloc(size);
		}
		virtual bool deallocate(void* ptr, stduint size = 0) override {
			if (!ptr) return false;
			free(ptr);
			return true;
		}
	};

	static DefaultHeapAllocator s_video_allocator;

	class FileStorageDevice : public uni::StorageTrait {
	private:
		FILE*   m_fp;
		stduint m_size;
		stduint m_cur_offset;
		stduint m_cache_block;
		byte*   m_cache_buf;

	public:
		FileStorageDevice(FILE* fp, stduint size, stduint blockSize = 65536)
			: m_fp(fp), m_size(size), m_cur_offset(0),
			  m_cache_block((stduint)~0), m_cache_buf(nullptr) {
			Block_Size = blockSize ? blockSize : 65536;
			readable = true;
			writable = false;
			m_cache_buf = (byte*)malloc(Block_Size);
			Block_buffer = m_cache_buf;
		}

		virtual ~FileStorageDevice() {
			if (m_cache_buf) {
				free(m_cache_buf);
				m_cache_buf = nullptr;
				Block_buffer = nullptr;
			}
		}

		virtual bool Read(stduint BlockIden, void* Dest, stduint Times = 1) override {
			if (BlockIden + Times > getUnits()) return false;
			stduint target_offset = BlockIden * Block_Size;
			if (target_offset != m_cur_offset) {
				if (fseek(m_fp, (long)target_offset, SEEK_SET) != 0) return false;
				m_cur_offset = target_offset;
			}
			stduint total_bytes = Times * Block_Size;
			size_t rd = fread(Dest, 1, total_bytes, m_fp);
			m_cur_offset += rd;
			return rd == total_bytes || (rd > 0 && BlockIden + Times == getUnits());
		}

		virtual bool Write(stduint BlockIden, const void* Sors, stduint Times = 1) override {
			return false;
		}

		virtual stduint getUnits() override {
			return (m_size + Block_Size - 1) / Block_Size;
		}

		virtual int operator[](uint64 bytid) override {
			if (bytid >= m_size || !m_cache_buf) return -1;
			stduint b_size = Block_Size ? Block_Size : 65536;
			stduint blk = (stduint)(bytid / b_size);
			stduint off = (stduint)(bytid % b_size);
			if (blk != m_cache_block) {
				if (!Read(blk, m_cache_buf, 1)) return -1;
				m_cache_block = blk;
				Block_buffer = m_cache_buf;
			}
			return m_cache_buf[off];
		}
	};

	static uint32 GetSystemClockMs() {
#if defined(_ACCM)
		return (uint32)syscall(syscall_t::TIME, 1, 0, 0);
#else
		return 0;
#endif
	}

	static void ApplySoftwareVolume(void* pcm_data, uint32 byte_count, uni::AudioSampleFormat format, uint32 vol_percent) {
		if (!pcm_data || !byte_count || vol_percent >= 100) return;
		if (vol_percent == 0) {
			if (format == uni::AudioSampleFormat::U8) {
				MemSet(pcm_data, 0x80, byte_count);
			} else {
				MemSet(pcm_data, 0x00, byte_count);
			}
			return;
		}

		if (format == uni::AudioSampleFormat::U8) {
			uint8* samples = static_cast<uint8*>(pcm_data);
			for (uint32 i = 0; i < byte_count; ++i) {
				int sample = (int)samples[i] - 128;
				sample = (sample * (int)vol_percent) / 100;
				samples[i] = (uint8)(sample + 128);
			}
		} else if (format == uni::AudioSampleFormat::S16LE) {
			int16* samples = static_cast<int16*>(pcm_data);
			const uint32 count = byte_count / 2;
			for (uint32 i = 0; i < count; ++i) {
				samples[i] = (int16)(((int32)samples[i] * (int32)vol_percent) / 100);
			}
		}
	}

	constexpr uint32 kAudioChunkSize = 4096;

	struct HostVideoInternal {
		FILE*               fp;
		FileStorageDevice*  storage;
		uni::IVideoStream*  stream;
		uni::VideoInfo      info;
		uint32              start_clock_ms;
		uint32              pause_clock_ms;
		uint32              total_paused_ms;
		uint32              current_audio_ms;
		uint64              total_audio_samples;
		uint32              next_frame_pts;
		uint32              curr_frame_idx;
		bool                owns_file;
		bool                owns_stream;
		uni::VideoFrame     cached_frame;
		bool                has_cached_frame;
		uint32              audio_chunk_read;
		uint32              audio_chunk_offset;
		byte                audio_raw_buf[kAudioChunkSize];
		byte                audio_proc_buf[kAudioChunkSize];
	};

#if defined(_ACCM)
#if __BITS__ > 32
#define Task_Audio_Serv 0
#endif

	static stdsint SendAudioRequest(AudioMsg type, const uni::AudioPlayRequest& request) {
		CommMsg send_msg{};
		send_msg.data.address = (stduint)&request;
		send_msg.data.length = sizeof(request);
		send_msg.type = (stduint)type;
		syscomm(1, Task_Audio_Serv, &send_msg);

		stdsint result = -1;
		CommMsg recv_msg{};
		recv_msg.data.address = (stduint)&result;
		recv_msg.data.length = sizeof(result);
		syscomm(0, Task_Audio_Serv, &recv_msg);
		return result;
	}

	static bool SendAudioSeek(uint32 target_ms, uint64 target_samples) {
		AudioSeekRequest req{};
		req.target_ms = target_ms;
		req.target_samples = target_samples;
		CommMsg send_msg{};
		send_msg.data.address = (stduint)&req;
		send_msg.data.length = sizeof(req);
		send_msg.type = (stduint)AudioMsg::STREAM_SEEK;
		syscomm(1, Task_Audio_Serv, &send_msg);

		stdsint result = -1;
		CommMsg recv_msg{};
		recv_msg.data.address = (stduint)&result;
		recv_msg.data.length = sizeof(result);
		syscomm(0, Task_Audio_Serv, &recv_msg);
		return result == 0;
	}

	static bool SendAudioGetPos(AudioStreamPosition& pos) {
		CommMsg send_msg{};
		send_msg.type = (stduint)AudioMsg::STREAM_GET_POS;
		syscomm(1, Task_Audio_Serv, &send_msg);

		CommMsg recv_msg{};
		recv_msg.data.address = (stduint)&pos;
		recv_msg.data.length = sizeof(pos);
		syscomm(0, Task_Audio_Serv, &recv_msg);
		return pos.is_active;
	}

	static bool SendAudioSetVolume(uni::SoundBlasterMixerChannel channel, uint8 left, uint8 right, bool mute = false) {
		AudioVolumeRequest req{};
		req.channel = channel;
		req.left = left;
		req.right = right;
		req.mute = mute;
		CommMsg send_msg{};
		send_msg.data.address = (stduint)&req;
		send_msg.data.length = sizeof(req);
		send_msg.type = (stduint)AudioMsg::SET_VOLUME;
		syscomm(1, Task_Audio_Serv, &send_msg);

		stdsint result = -1;
		CommMsg recv_msg{};
		recv_msg.data.address = (stduint)&result;
		recv_msg.data.length = sizeof(result);
		syscomm(0, Task_Audio_Serv, &recv_msg);
		return result == 0;
	}
#endif

} // namespace

namespace uni {

	HostVideo::HostVideo()
		: OnFinished(nullptr), OnProgress(nullptr), user_data(nullptr),
		  impl(nullptr), state(HostVideoState::Idle), volume(100),
		  is_loop(false), is_muted(false) {
	}

	HostVideo::HostVideo(const char* filepath, bool loop_mode)
		: OnFinished(nullptr), OnProgress(nullptr), user_data(nullptr),
		  impl(nullptr), state(HostVideoState::Idle), volume(100),
		  is_loop(loop_mode), is_muted(false) {
		Open(filepath, loop_mode);
	}

	HostVideo::HostVideo(StorageTrait& storage, bool loop_mode)
		: OnFinished(nullptr), OnProgress(nullptr), user_data(nullptr),
		  impl(nullptr), state(HostVideoState::Idle), volume(100),
		  is_loop(loop_mode), is_muted(false) {
		Open(storage, loop_mode);
	}

	HostVideo::HostVideo(IVideoStream* stream, bool loop_mode)
		: OnFinished(nullptr), OnProgress(nullptr), user_data(nullptr),
		  impl(nullptr), state(HostVideoState::Idle), volume(100),
		  is_loop(loop_mode), is_muted(false) {
		OpenStream(stream, loop_mode);
	}


	HostVideo::~HostVideo() {
		Close();
	}

	HostVideo::operator bool() const {
		return impl != nullptr && state != HostVideoState::Error && state != HostVideoState::Idle;
	}

	static uni::VideoResult TryOpenVideoStream(
		uni::StorageTrait& storage,
		uni::IVideoStream*& outStream,
		uni::trait::Malloc& allocator
	) {
		// 1. Try AVI probe
		{
			uni::AVICodec avi_codec;
			bool matched = false;
			if (avi_codec.Probe(storage, matched) == uni::VideoResult::OK && matched) {
				return avi_codec.OpenStream(storage, outStream, allocator);
			}
		}
		// 2. Try MPEG-1/2 probe
		{
			uni::MPEGCodec mpeg_codec;
			bool matched = false;
			if (mpeg_codec.Probe(storage, matched) == uni::VideoResult::OK && matched) {
				return mpeg_codec.OpenStream(storage, outStream, allocator);
			}
		}
		// Fallback: try AVI OpenStream, then MPEG OpenStream
		{
			uni::AVICodec avi_codec;
			uni::VideoResult res = avi_codec.OpenStream(storage, outStream, allocator);
			if (res == uni::VideoResult::OK && outStream) return res;
		}
		{
			uni::MPEGCodec mpeg_codec;
			uni::VideoResult res = mpeg_codec.OpenStream(storage, outStream, allocator);
			if (res == uni::VideoResult::OK && outStream) return res;
		}
		return uni::VideoResult::Unsupported;
	}

	bool HostVideo::Open(const char* filepath, bool loop_mode) {
		Close();
		is_loop = loop_mode;

		if (!filepath) {
			state = HostVideoState::Error;
			return false;
		}

		FILE* fp = fopen(filepath, "rb");
		if (!fp) {
			state = HostVideoState::Error;
			return false;
		}

		fseek(fp, 0, SEEK_END);
		long file_size = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		if (file_size <= 0) {
			fclose(fp);
			state = HostVideoState::Error;
			return false;
		}

		FileStorageDevice* storage = new FileStorageDevice(fp, (stduint)file_size);
		if (!storage) {
			fclose(fp);
			state = HostVideoState::Error;
			return false;
		}

		IVideoStream* stream = nullptr;
		if (TryOpenVideoStream(*storage, stream, s_video_allocator) != VideoResult::OK || !stream) {
			delete storage;
			fclose(fp);
			state = HostVideoState::Error;
			return false;
		}

		HostVideoInternal* internal = new HostVideoInternal();
		if (!internal) {
			stream->Release();
			delete storage;
			fclose(fp);
			state = HostVideoState::Error;
			return false;
		}

		internal->fp = fp;
		internal->storage = storage;
		internal->stream = stream;
		internal->start_clock_ms = 0;
		internal->pause_clock_ms = 0;
		internal->total_paused_ms = 0;
		internal->current_audio_ms = 0;
		internal->total_audio_samples = 0;
		internal->next_frame_pts = 0;
		internal->curr_frame_idx = 0;
		internal->owns_file = true;
		internal->owns_stream = true;
		internal->has_cached_frame = false;
		internal->audio_chunk_read = 0;
		internal->audio_chunk_offset = 0;
		VideoFrameClear(internal->cached_frame);

		stream->GetInfo(internal->info);
		impl = internal;
		state = HostVideoState::Stopped;
		return true;
	}

	bool HostVideo::Open(StorageTrait& storage, bool loop_mode) {
		Close();
		is_loop = loop_mode;

		IVideoStream* stream = nullptr;
		if (TryOpenVideoStream(storage, stream, s_video_allocator) != VideoResult::OK || !stream) {
			state = HostVideoState::Error;
			return false;
		}

		HostVideoInternal* internal = new HostVideoInternal();
		if (!internal) {
			stream->Release();
			state = HostVideoState::Error;
			return false;
		}

		internal->fp = nullptr;
		internal->storage = nullptr;
		internal->stream = stream;
		internal->start_clock_ms = 0;
		internal->pause_clock_ms = 0;
		internal->total_paused_ms = 0;
		internal->current_audio_ms = 0;
		internal->total_audio_samples = 0;
		internal->next_frame_pts = 0;
		internal->curr_frame_idx = 0;
		internal->owns_file = false;
		internal->owns_stream = true;
		internal->has_cached_frame = false;
		internal->audio_chunk_read = 0;
		internal->audio_chunk_offset = 0;
		VideoFrameClear(internal->cached_frame);

		stream->GetInfo(internal->info);
		impl = internal;
		state = HostVideoState::Stopped;
		return true;
	}

	bool HostVideo::OpenStream(IVideoStream* stream, bool loop_mode) {
		Close();
		is_loop = loop_mode;
		if (!stream) {
			state = HostVideoState::Error;
			return false;
		}

		HostVideoInternal* internal = new HostVideoInternal();
		if (!internal) {
			state = HostVideoState::Error;
			return false;
		}

		internal->fp = nullptr;
		internal->storage = nullptr;
		internal->stream = stream;
		internal->start_clock_ms = 0;
		internal->pause_clock_ms = 0;
		internal->total_paused_ms = 0;
		internal->current_audio_ms = 0;
		internal->total_audio_samples = 0;
		internal->next_frame_pts = 0;
		internal->curr_frame_idx = 0;
		internal->owns_file = false;
		internal->owns_stream = false;
		internal->has_cached_frame = false;
		internal->audio_chunk_read = 0;
		internal->audio_chunk_offset = 0;
		VideoFrameClear(internal->cached_frame);

		stream->GetInfo(internal->info);
		impl = internal;
		state = HostVideoState::Stopped;
		return true;
	}

	void HostVideo::Close() {
		if (!impl) {
			state = HostVideoState::Idle;
			return;
		}

		HostVideoInternal* internal = static_cast<HostVideoInternal*>(impl);
		if (state == HostVideoState::Playing || state == HostVideoState::Paused) {
			Stop();
		}

		if (internal->has_cached_frame) {
			VideoFrameFree(internal->cached_frame);
			internal->has_cached_frame = false;
		}
		if (internal->owns_stream && internal->stream) {
			internal->stream->Release();
			internal->stream = nullptr;
		}
		if (internal->storage) {
			delete internal->storage;
			internal->storage = nullptr;
		}
		if (internal->owns_file && internal->fp) {
			fclose(internal->fp);
			internal->fp = nullptr;
		}
		delete internal;
		impl = nullptr;
		state = HostVideoState::Idle;
	}

	bool HostVideo::Play() {
		if (!impl || state == HostVideoState::Error) return false;
		if (state == HostVideoState::Paused) return Resume();

		HostVideoInternal* internal = static_cast<HostVideoInternal*>(impl);
		internal->start_clock_ms = GetSystemClockMs();
		internal->pause_clock_ms = 0;
		internal->total_paused_ms = 0;
		internal->next_frame_pts = 0;
		internal->current_audio_ms = 0;
		internal->total_audio_samples = 0;
		internal->audio_chunk_read = 0;
		internal->audio_chunk_offset = 0;

#if defined(_ACCM)
		if (internal->info.hasAudio) {
			uni::AudioPlayRequest req{};
			req.format = internal->info.audioInfo.format;
			SendAudioRequest(AudioMsg::STREAM_BEGIN, req);
		}
#endif

		state = HostVideoState::Playing;
		if (internal->info.hasAudio) {
			Update();
		}
		return true;
	}


	bool HostVideo::Pause() {
		if (state != HostVideoState::Playing || !impl) return false;
		HostVideoInternal* internal = static_cast<HostVideoInternal*>(impl);
		internal->pause_clock_ms = GetSystemClockMs();

#if defined(_ACCM)
		if (internal->info.hasAudio) {
			uni::AudioPlayRequest req{};
			SendAudioRequest(AudioMsg::STREAM_PAUSE, req);
		}
#endif

		state = HostVideoState::Paused;
		return true;
	}

	bool HostVideo::Resume() {
		if (state != HostVideoState::Paused || !impl) return false;
		HostVideoInternal* internal = static_cast<HostVideoInternal*>(impl);
		uint32 now = GetSystemClockMs();
		if (now >= internal->pause_clock_ms) {
			internal->total_paused_ms += (now - internal->pause_clock_ms);
		}
		internal->pause_clock_ms = 0;

#if defined(_ACCM)
		if (internal->info.hasAudio) {
			uni::AudioPlayRequest req{};
			SendAudioRequest(AudioMsg::STREAM_RESUME, req);
		}
#endif

		state = HostVideoState::Playing;
		return true;
	}

	void HostVideo::Stop() {
		if (!impl) return;
		HostVideoInternal* internal = static_cast<HostVideoInternal*>(impl);
#if defined(_ACCM)
		if (internal->info.hasAudio) {
			uni::AudioPlayRequest req{};
			SendAudioRequest(AudioMsg::STREAM_STOP, req);
		}
#endif
		if (internal->stream) {
			internal->stream->SeekFrame(0);
		}
		if (internal->has_cached_frame) {
			VideoFrameFree(internal->cached_frame);
			internal->has_cached_frame = false;
		}

		internal->curr_frame_idx = 0;
		internal->next_frame_pts = 0;
		internal->current_audio_ms = 0;
		internal->total_audio_samples = 0;
		internal->audio_chunk_read = 0;
		internal->audio_chunk_offset = 0;
		state = HostVideoState::Stopped;
	}

	bool HostVideo::Seek(uint32 target_ms) {
		if (!impl) return false;
		HostVideoInternal* internal = static_cast<HostVideoInternal*>(impl);
		if (!internal->stream) return false;

		if (target_ms > internal->info.durationMs) {
			target_ms = internal->info.durationMs;
		}

		if (internal->stream->SeekTime(target_ms) != VideoResult::OK) {
			return false;
		}

		if (internal->has_cached_frame) {
			VideoFrameFree(internal->cached_frame);
			internal->has_cached_frame = false;
		}

		uint32 now = GetSystemClockMs();
		internal->start_clock_ms = (now >= target_ms) ? (now - target_ms) : now;
		internal->total_paused_ms = 0;
		internal->next_frame_pts = target_ms;
		internal->current_audio_ms = target_ms;
		internal->audio_chunk_read = 0;
		internal->audio_chunk_offset = 0;

		uint32 rate = internal->info.videoFormat.frame_rate_num ? internal->info.videoFormat.frame_rate_num : 30;
		uint32 scale = internal->info.videoFormat.frame_rate_den ? internal->info.videoFormat.frame_rate_den : 1;
		internal->curr_frame_idx = (uint32)(((uint64)target_ms * rate) / (1000ULL * scale));

#if defined(_ACCM)
		if (internal->info.hasAudio) {
			uint32 arate = internal->info.audioInfo.format.sample_rate ? internal->info.audioInfo.format.sample_rate : 44100;
			uint64 target_samples = ((uint64)target_ms * arate) / 1000;
			internal->total_audio_samples = target_samples;
			SendAudioSeek(target_ms, target_samples);
		}
#endif
		return true;
	}

	bool HostVideo::SeekFrame(uint32 frame_index) {
		if (!impl) return false;
		HostVideoInternal* internal = static_cast<HostVideoInternal*>(impl);
		if (!internal->stream) return false;

		if (internal->stream->SeekFrame(frame_index) != VideoResult::OK) {
			return false;
		}

		if (internal->has_cached_frame) {
			VideoFrameFree(internal->cached_frame);
			internal->has_cached_frame = false;
		}

		uint32 rate = internal->info.videoFormat.frame_rate_num ? internal->info.videoFormat.frame_rate_num : 30;
		uint32 scale = internal->info.videoFormat.frame_rate_den ? internal->info.videoFormat.frame_rate_den : 1;
		uint32 target_ms = (uint32)(((uint64)frame_index * 1000ULL * scale) / rate);

		uint32 now = GetSystemClockMs();
		internal->start_clock_ms = (now >= target_ms) ? (now - target_ms) : now;
		internal->total_paused_ms = 0;
		internal->next_frame_pts = target_ms;
		internal->curr_frame_idx = frame_index;
		internal->current_audio_ms = target_ms;
		internal->audio_chunk_read = 0;
		internal->audio_chunk_offset = 0;

#if defined(_ACCM)
		if (internal->info.hasAudio) {
			uint32 arate = internal->info.audioInfo.format.sample_rate ? internal->info.audioInfo.format.sample_rate : 44100;
			uint64 target_samples = ((uint64)target_ms * arate) / 1000;
			internal->total_audio_samples = target_samples;
			SendAudioSeek(target_ms, target_samples);
		}
#endif
		return true;
	}

	bool HostVideo::Update() {
		if (state != HostVideoState::Playing || !impl) return false;
		HostVideoInternal* internal = static_cast<HostVideoInternal*>(impl);
		if (!internal->stream) return false;

		if (internal->info.hasAudio) {
			// Continuously fill audio ring buffer while accepted > 0
			while (true) {
				if (internal->audio_chunk_offset >= internal->audio_chunk_read) {
					internal->audio_chunk_offset = 0;
					internal->audio_chunk_read = 0;
					VideoResult ares = internal->stream->ReadAudioSamples(
						internal->audio_raw_buf, kAudioChunkSize, internal->audio_chunk_read);

					if (ares == VideoResult::EndOfStream || (ares == VideoResult::OK && internal->audio_chunk_read == 0)) {
						break;
					} else if (ares == VideoResult::OK && internal->audio_chunk_read > 0) {
						MemCopyN(internal->audio_proc_buf, internal->audio_raw_buf, internal->audio_chunk_read);
						uint32 effective_vol = is_muted ? 0 : volume;
						ApplySoftwareVolume(internal->audio_proc_buf, internal->audio_chunk_read,
							internal->info.audioInfo.format.sample_format, effective_vol);
					} else {
						break;
					}
				}

#if defined(_ACCM)
				if (internal->audio_chunk_read > internal->audio_chunk_offset) {
					uni::AudioPlayRequest write_req{};
					write_req.format = internal->info.audioInfo.format;
					write_req.buffer.data = internal->audio_proc_buf + internal->audio_chunk_offset;
					write_req.buffer.byte_count = internal->audio_chunk_read - internal->audio_chunk_offset;
					stdsint accepted = SendAudioRequest(AudioMsg::STREAM_WRITE, write_req);
					if (accepted > 0) {
						internal->audio_chunk_offset += (uint32)accepted;
					} else {
						// Audio buffer in service is full
						break;
					}
				} else {
					break;
				}
#else
				internal->audio_chunk_offset = internal->audio_chunk_read;
				break;
#endif
			}

#if defined(_ACCM)
			AudioStreamPosition pos{};
			if (SendAudioGetPos(pos)) {
				internal->current_audio_ms = pos.played_ms;
				internal->total_audio_samples = pos.played_samples;
			}
#endif
		}

		if (OnProgress) {
			OnProgress(this, GetPositionMs(), internal->info.durationMs, user_data);
		}
		return true;
	}

	bool HostVideo::UpdateFrame(uni::Color* destPixels, uint32& outTimestampMs) {
		if (!impl || state != HostVideoState::Playing || !destPixels) return false;

		HostVideoInternal* internal = static_cast<HostVideoInternal*>(impl);
		if (!internal->stream) return false;

		uint32 current_ms = 0;
		if (internal->info.hasAudio && internal->current_audio_ms > 0) {
			current_ms = internal->current_audio_ms;
		} else {
			uint32 now = GetSystemClockMs();
			current_ms = (now >= (internal->start_clock_ms + internal->total_paused_ms))
				? (now - internal->start_clock_ms - internal->total_paused_ms)
				: 0;
		}

		uint32 frame_duration = 33;
		if (internal->info.videoFormat.frame_rate_num && internal->info.videoFormat.frame_rate_den) {
			frame_duration = (uint32)((1000ULL * internal->info.videoFormat.frame_rate_den) / internal->info.videoFormat.frame_rate_num);
			if (frame_duration == 0) frame_duration = 1;
		}

		// Ensure we have a candidate frame
		if (!internal->has_cached_frame) {
			VideoResult res = internal->stream->ReadVideoFrame(internal->cached_frame, s_video_allocator);
			if (res == VideoResult::EndOfStream) {
				if (is_loop) {
					Seek(0);
					return false;
				} else {
#if defined(_ACCM)
					if (internal->info.hasAudio) {
						uni::AudioPlayRequest drain_req{};
						drain_req.format = internal->info.audioInfo.format;
						SendAudioRequest(AudioMsg::STREAM_DRAIN, drain_req);
					}
#endif
					state = HostVideoState::Stopped;
					if (OnFinished) {
						OnFinished(this, user_data);
					}
					return false;
				}
			} else if (res != VideoResult::OK) {
				return false;
			}
			internal->has_cached_frame = true;
			internal->curr_frame_idx = internal->cached_frame.frameIndex;
			internal->next_frame_pts = internal->cached_frame.timestampMs;
		}

		// Frame dropping: If current playback time is already ahead of this frame by more than 1 frame duration,
		// skip past late frames to keep video tightly synchronized with audio
		while (internal->has_cached_frame && (current_ms > internal->next_frame_pts + frame_duration)) {
			VideoFrameFree(internal->cached_frame);
			internal->has_cached_frame = false;

			VideoResult res = internal->stream->ReadVideoFrame(internal->cached_frame, s_video_allocator);
			if (res != VideoResult::OK) {
				break;
			}
			internal->has_cached_frame = true;
			internal->curr_frame_idx = internal->cached_frame.frameIndex;
			internal->next_frame_pts = internal->cached_frame.timestampMs;
		}

		if (!internal->has_cached_frame) {
			return false;
		}

		if (current_ms < internal->next_frame_pts) {
			return false;
		}

		if (internal->cached_frame.image.pixels) {
			uint32 target_w = internal->info.videoFormat.width;
			uint32 target_h = internal->info.videoFormat.height;
			uint32 fw = internal->cached_frame.width;
			uint32 fh = internal->cached_frame.height;
			if (fw == target_w && fh == target_h) {
				MemCopyN(destPixels, internal->cached_frame.image.pixels, (size_t)target_w * (size_t)target_h * sizeof(uni::Color));
			} else {
				uint32 copy_w = fw < target_w ? fw : target_w;
				uint32 copy_h = fh < target_h ? fh : target_h;
				for (uint32 y = 0; y < copy_h; ++y) {
					MemCopyN(destPixels + y * target_w, (const uni::Color*)internal->cached_frame.image.pixels + y * fw, (size_t)copy_w * sizeof(uni::Color));
				}
			}
			outTimestampMs = internal->cached_frame.timestampMs;
		}

		VideoFrameFree(internal->cached_frame);
		internal->has_cached_frame = false;
		return true;
	}


	VideoResult HostVideo::ReadAudioSamples(void* destBuffer, uint32 maxBytes, uint32& bytesRead) {
		bytesRead = 0;
		if (!impl) return VideoResult::Failed;
		HostVideoInternal* internal = static_cast<HostVideoInternal*>(impl);
		if (!internal->stream) return VideoResult::Failed;
		return internal->stream->ReadAudioSamples(destBuffer, maxBytes, bytesRead);
	}

	IVideoStream* HostVideo::GetStream() const {
		if (!impl) return nullptr;
		return static_cast<HostVideoInternal*>(impl)->stream;
	}

	bool HostVideo::GetInfo(VideoInfo& outInfo) const {
		if (!impl) return false;
		outInfo = static_cast<HostVideoInternal*>(impl)->info;
		return true;
	}

	uint32 HostVideo::GetPositionMs() const {
		if (!impl) return 0;
		HostVideoInternal* internal = static_cast<HostVideoInternal*>(impl);
		if (state == HostVideoState::Playing) {
			if (internal->info.hasAudio && internal->current_audio_ms > 0) {
				return internal->current_audio_ms;
			}
			uint32 now = GetSystemClockMs();
			return (now >= (internal->start_clock_ms + internal->total_paused_ms))
				? (now - internal->start_clock_ms - internal->total_paused_ms)
				: 0;
		}
		return internal->next_frame_pts;
	}

	uint32 HostVideo::getDurationMs() const {
		if (!impl) return 0;
		return static_cast<HostVideoInternal*>(impl)->info.durationMs;
	}

	uint32 HostVideo::GetCurrentFrameIndex() const {
		if (!impl) return 0;
		return static_cast<HostVideoInternal*>(impl)->curr_frame_idx;
	}

	void HostVideo::setVolume(uint32 vol_percent) {
		volume = (vol_percent <= 100) ? vol_percent : 100;
#if defined(_ACCM)
		if (impl) {
			HostVideoInternal* internal = static_cast<HostVideoInternal*>(impl);
			if (internal->info.hasAudio) {
				uint8 vol_byte = (uint8)((volume * 255) / 100);
				SendAudioSetVolume(uni::SoundBlasterMixerChannel::MasterVolume, vol_byte, vol_byte, is_muted);
			}
		}
#endif
	}

	void HostVideo::setMute(bool mute) {
		is_muted = mute;
#if defined(_ACCM)
		if (impl) {
			HostVideoInternal* internal = static_cast<HostVideoInternal*>(impl);
			if (internal->info.hasAudio) {
				uint8 vol_byte = (uint8)((volume * 255) / 100);
				SendAudioSetVolume(uni::SoundBlasterMixerChannel::MasterVolume, vol_byte, vol_byte, is_muted);
			}
		}
#endif
	}

	void HostVideo::setLoop(bool loop_mode) {
		is_loop = loop_mode;
	}

} // namespace uni

