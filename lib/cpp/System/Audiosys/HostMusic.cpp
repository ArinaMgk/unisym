// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: HostMusic Audio Player Implementation
// Codifiers: @dosconio, @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../inc/cpp/System/Audiosys.hpp"
#include "../../../../inc/c/format/audio/WAV.h"
#include "../../../../inc/c/format/audio/MP3.h"
#include "../../../../inc/c/format/audio/FLAC.h"
#include "../../../../inc/c/format/audio/OGG.h"
#include "../../../../inc/c/format/audio/MIDI.h"
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

	static DefaultHeapAllocator s_host_music_allocator;

	class FileStorageDevice : public uni::StorageTrait {
	private:
		FILE*   m_fp;
		stduint m_size;
		stduint m_cur_offset;
		stduint m_cache_block;
		byte*   m_cache_buf;

	public:
		FileStorageDevice(FILE* fp, stduint size, stduint blockSize = 4096)
			: m_fp(fp), m_size(size), m_cur_offset(0),
			  m_cache_block((stduint)~0), m_cache_buf(nullptr) {
			Block_Size = blockSize ? blockSize : 4096;
			readable = true;
			writable = false;
			m_cache_buf = (byte*)malloc(Block_Size);
		}

		virtual ~FileStorageDevice() {
			if (m_cache_buf) {
				free(m_cache_buf);
				m_cache_buf = nullptr;
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
			stduint b_size = Block_Size ? Block_Size : 4096;
			stduint blk = (stduint)(bytid / b_size);
			stduint off = (stduint)(bytid % b_size);
			if (blk != m_cache_block) {
				if (!Read(blk, m_cache_buf, 1)) return -1;
				m_cache_block = blk;
			}
			return m_cache_buf[off];
		}
	};

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

	constexpr uint32 kChunkBufferSize = 4096;

	struct HostMusicInternal {
		FILE*                 fp;
		FileStorageDevice*    storage;
		uni::IAudioStream*    source_stream;
		uni::IAudioStream*    active_stream;
		uni::ResamplerStream* resampler;
		uni::AudioInfo        info;
		uint32                current_ms;
		uint64                total_samples_played;
		bool                  owns_file;
		bool                  owns_stream;
		uint32                chunk_bytes_read;
		uint32                chunk_offset;
		byte                  raw_buffer[kChunkBufferSize];
		byte                  proc_buffer[kChunkBufferSize];
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

}

namespace uni {

	HostMusic::HostMusic()
		: OnFinished(nullptr), OnProgress(nullptr), user_data(nullptr),
		  impl(nullptr), state(HostMusicState::Idle), volume(100),
		  muted(false), is_looping(false) {}

	HostMusic::HostMusic(rostr filepath, bool loop_mode)
		: OnFinished(nullptr), OnProgress(nullptr), user_data(nullptr),
		  impl(nullptr), state(HostMusicState::Idle), volume(100),
		  muted(false), is_looping(loop_mode) {
		Open(filepath, loop_mode);
	}

	HostMusic::HostMusic(StorageTrait& storage, bool loop_mode)
		: OnFinished(nullptr), OnProgress(nullptr), user_data(nullptr),
		  impl(nullptr), state(HostMusicState::Idle), volume(100),
		  muted(false), is_looping(loop_mode) {
		Open(storage, loop_mode);
	}

	HostMusic::HostMusic(IAudioStream* stream, bool loop_mode)
		: OnFinished(nullptr), OnProgress(nullptr), user_data(nullptr),
		  impl(nullptr), state(HostMusicState::Idle), volume(100),
		  muted(false), is_looping(loop_mode) {
		OpenStream(stream, loop_mode);
	}

	HostMusic::~HostMusic() {
		Close();
	}

	HostMusic::operator bool() const {
		return impl != nullptr && state != HostMusicState::Error && state != HostMusicState::Idle;
	}

	static uni::AudioResult TryOpenAudioStream(
		uni::StorageTrait& storage,
		uni::IAudioStream*& outStream,
		uni::trait::Malloc& allocator
	) {
		// 1. Try WAV probe
		{
			uni::WAVCodec wav_codec;
			bool matched = false;
			if (wav_codec.Probe(storage, matched) == uni::AudioResult::OK && matched) {
				return wav_codec.OpenStream(storage, outStream, allocator);
			}
		}
		// 2. Try MP3 probe
		{
			uni::MP3Codec mp3_codec;
			bool matched = false;
			if (mp3_codec.Probe(storage, matched) == uni::AudioResult::OK && matched) {
				return mp3_codec.OpenStream(storage, outStream, allocator);
			}
		}
		// 3. Try FLAC probe
		{
			uni::FLACCodec flac_codec;
			bool matched = false;
			if (flac_codec.Probe(storage, matched) == uni::AudioResult::OK && matched) {
				return flac_codec.OpenStream(storage, outStream, allocator);
			}
		}
		// 4. Try OGG probe
		{
			uni::OGGCodec ogg_codec;
			bool matched = false;
			if (ogg_codec.Probe(storage, matched) == uni::AudioResult::OK && matched) {
				return ogg_codec.OpenStream(storage, outStream, allocator);
			}
		}
		// 5. Try MIDI probe
		{
			uni::MIDICodec midi_codec;
			bool matched = false;
			if (midi_codec.Probe(storage, matched) == uni::AudioResult::OK && matched) {
				return midi_codec.OpenStream(storage, outStream, allocator);
			}
		}
		// Fallback: try WAV OpenStream, then MP3 OpenStream, then FLAC OpenStream, then OGG OpenStream, then MIDI OpenStream
		{
			uni::WAVCodec wav_codec;
			uni::AudioResult res = wav_codec.OpenStream(storage, outStream, allocator);
			if (res == uni::AudioResult::OK && outStream) return res;
		}
		{
			uni::MP3Codec mp3_codec;
			uni::AudioResult res = mp3_codec.OpenStream(storage, outStream, allocator);
			if (res == uni::AudioResult::OK && outStream) return res;
		}
		{
			uni::FLACCodec flac_codec;
			uni::AudioResult res = flac_codec.OpenStream(storage, outStream, allocator);
			if (res == uni::AudioResult::OK && outStream) return res;
		}
		{
			uni::OGGCodec ogg_codec;
			uni::AudioResult res = ogg_codec.OpenStream(storage, outStream, allocator);
			if (res == uni::AudioResult::OK && outStream) return res;
		}
		{
			uni::MIDICodec midi_codec;
			uni::AudioResult res = midi_codec.OpenStream(storage, outStream, allocator);
			if (res == uni::AudioResult::OK && outStream) return res;
		}
		return uni::AudioResult::Unsupported;
	}

	bool HostMusic::Open(rostr filepath, bool loop_mode) {
		Close();
		this->is_looping = loop_mode;
		if (!filepath) {
			state = HostMusicState::Error;
			return false;
		}

		FILE* fp = fopen(filepath, "rb");
		if (!fp) {
			state = HostMusicState::Error;
			return false;
		}

		if (fseek(fp, 0, SEEK_END) != 0) {
			fclose(fp);
			state = HostMusicState::Error;
			return false;
		}
		long sz = ftell(fp);
		if (sz <= 0) {
			fclose(fp);
			state = HostMusicState::Error;
			return false;
		}
		fseek(fp, 0, SEEK_SET);

		FileStorageDevice* storage = new (s_host_music_allocator) FileStorageDevice(fp, (stduint)sz);
		if (!storage) {
			fclose(fp);
			state = HostMusicState::Error;
			return false;
		}

		IAudioStream* stream = nullptr;
		AudioResult res = TryOpenAudioStream(*storage, stream, s_host_music_allocator);
		if (res != AudioResult::OK || !stream) {
			storage->~FileStorageDevice();
			s_host_music_allocator.deallocate(storage);
			fclose(fp);
			state = HostMusicState::Error;
			return false;
		}

		AudioInfo info{};
		stream->GetInfo(info);

		IAudioStream* active_stream = stream;
		ResamplerStream* resampler = nullptr;

		if (info.format.sample_rate > 44100 || (info.format.sample_rate > 0 && info.format.sample_rate < 5000)) {
			const uint32 target_rate = (info.format.sample_rate > 44100) ? 44100 : 11025;
			void* resampler_mem = s_host_music_allocator.allocate(sizeof(ResamplerStream));
			if (resampler_mem) {
				resampler = new (resampler_mem) ResamplerStream(stream, target_rate, s_host_music_allocator);
				if (resampler && resampler->IsValid()) {
					active_stream = resampler;
					active_stream->GetInfo(info);
				} else if (resampler) {
					resampler->Release();
					resampler = nullptr;
				}
			}
		}

		HostMusicInternal* internal = (HostMusicInternal*)s_host_music_allocator.allocate(sizeof(HostMusicInternal));
		if (!internal) {
			if (resampler) resampler->Release();
			else stream->Release();
			storage->~FileStorageDevice();
			s_host_music_allocator.deallocate(storage);
			fclose(fp);
			state = HostMusicState::Error;
			return false;
		}

		internal->fp = fp;
		internal->storage = storage;
		internal->source_stream = stream;
		internal->active_stream = active_stream;
		internal->resampler = resampler;
		internal->info = info;
		internal->current_ms = 0;
		internal->total_samples_played = 0;
		internal->owns_file = true;
		internal->owns_stream = true;
		internal->chunk_bytes_read = 0;
		internal->chunk_offset = 0;

		impl = internal;
		state = HostMusicState::Stopped;
		return true;
	}

	bool HostMusic::Open(StorageTrait& storage, bool loop_mode) {
		Close();
		this->is_looping = loop_mode;
		IAudioStream* stream = nullptr;
		AudioResult res = TryOpenAudioStream(storage, stream, s_host_music_allocator);
		if (res != AudioResult::OK || !stream) {
			state = HostMusicState::Error;
			return false;
		}
		bool ok = OpenStream(stream, loop_mode);
		if (ok && impl) {
			static_cast<HostMusicInternal*>(impl)->owns_stream = true;
		}
		return ok;
	}

	bool HostMusic::OpenStream(IAudioStream* stream, bool loop_mode) {
		Close();
		this->is_looping = loop_mode;
		if (!stream) {
			state = HostMusicState::Error;
			return false;
		}

		AudioInfo info{};
		stream->GetInfo(info);

		IAudioStream* active_stream = stream;
		ResamplerStream* resampler = nullptr;

		if (info.format.sample_rate > 44100 || (info.format.sample_rate > 0 && info.format.sample_rate < 5000)) {
			const uint32 target_rate = (info.format.sample_rate > 44100) ? 44100 : 11025;
			void* resampler_mem = s_host_music_allocator.allocate(sizeof(ResamplerStream));
			if (resampler_mem) {
				resampler = new (resampler_mem) ResamplerStream(stream, target_rate, s_host_music_allocator);
				if (resampler && resampler->IsValid()) {
					active_stream = resampler;
					active_stream->GetInfo(info);
				} else if (resampler) {
					resampler->Release();
					resampler = nullptr;
				}
			}
		}

		HostMusicInternal* internal = (HostMusicInternal*)s_host_music_allocator.allocate(sizeof(HostMusicInternal));
		if (!internal) {
			if (resampler) resampler->Release();
			state = HostMusicState::Error;
			return false;
		}

		internal->fp = nullptr;
		internal->storage = nullptr;
		internal->source_stream = stream;
		internal->active_stream = active_stream;
		internal->resampler = resampler;
		internal->info = info;
		internal->current_ms = 0;
		internal->total_samples_played = 0;
		internal->owns_file = false;
		internal->owns_stream = false;
		internal->chunk_bytes_read = 0;
		internal->chunk_offset = 0;

		impl = internal;
		state = HostMusicState::Stopped;
		return true;
	}

	void HostMusic::Close() {
		if (!impl) {
			state = HostMusicState::Idle;
			return;
		}
		HostMusicInternal* internal = static_cast<HostMusicInternal*>(impl);
		if (state == HostMusicState::Playing || state == HostMusicState::Paused) {
			Stop();
		}
		if (internal->resampler) {
			internal->resampler->Release();
			internal->resampler = nullptr;
			internal->active_stream = nullptr;
			internal->source_stream = nullptr;
		} else if (internal->owns_stream && internal->source_stream) {
			internal->source_stream->Release();
			internal->source_stream = nullptr;
			internal->active_stream = nullptr;
		}
		if (internal->storage) {
			internal->storage->~FileStorageDevice();
			s_host_music_allocator.deallocate(internal->storage);
			internal->storage = nullptr;
		}
		if (internal->owns_file && internal->fp) {
			fclose(internal->fp);
			internal->fp = nullptr;
		}
		s_host_music_allocator.deallocate(internal);
		impl = nullptr;
		state = HostMusicState::Idle;
	}

	bool HostMusic::Play() {
		if (!impl || state == HostMusicState::Error) return false;
		if (state == HostMusicState::Paused) return Resume();
		HostMusicInternal* internal = static_cast<HostMusicInternal*>(impl);
#if defined(_ACCM)
		uni::AudioPlayRequest begin_request{};
		begin_request.format = internal->info.format;
		if (SendAudioRequest(AudioMsg::STREAM_BEGIN, begin_request) != 0) {
			state = HostMusicState::Error;
			return false;
		}
#endif
		state = HostMusicState::Playing;
		return true;
	}

	bool HostMusic::Pause() {
		if (state != HostMusicState::Playing || !impl) return false;
#if defined(_ACCM)
		uni::AudioPlayRequest req{};
		SendAudioRequest(AudioMsg::STREAM_PAUSE, req);
#endif
		state = HostMusicState::Paused;
		return true;
	}

	bool HostMusic::Resume() {
		if (state != HostMusicState::Paused || !impl) return false;
#if defined(_ACCM)
		uni::AudioPlayRequest req{};
		SendAudioRequest(AudioMsg::STREAM_RESUME, req);
#endif
		state = HostMusicState::Playing;
		return true;
	}

	void HostMusic::Stop() {
		if (!impl) return;
		HostMusicInternal* internal = static_cast<HostMusicInternal*>(impl);
#if defined(_ACCM)
		uni::AudioPlayRequest req{};
		SendAudioRequest(AudioMsg::STREAM_STOP, req);
#endif
		if (internal->active_stream) {
			internal->active_stream->Seek(0);
		}
		internal->chunk_bytes_read = 0;
		internal->chunk_offset = 0;
		internal->current_ms = 0;
		internal->total_samples_played = 0;
		state = HostMusicState::Stopped;
	}

	bool HostMusic::Seek(uint32 target_ms) {
		if (!impl) return false;
		HostMusicInternal* internal = static_cast<HostMusicInternal*>(impl);
		if (target_ms > internal->info.durationMs) {
			target_ms = internal->info.durationMs;
		}

		uint32 rate = internal->info.format.sample_rate ? internal->info.format.sample_rate : 44100;
		uint64 target_samples = ((uint64)target_ms * rate) / 1000;
		AudioResult res = internal->active_stream->Seek((uint32)target_samples);
		if (res != AudioResult::OK) return false;

		internal->chunk_bytes_read = 0;
		internal->chunk_offset = 0;
		internal->current_ms = target_ms;
		internal->total_samples_played = target_samples;

#if defined(_ACCM)
		SendAudioSeek(target_ms, target_samples);
#endif
		return true;
	}

	AudioResult HostMusic::ReadSamples(void* destBuffer, uint32 maxBytes, uint32& bytesRead) {
		bytesRead = 0;
		if (!impl || (state != HostMusicState::Playing && state != HostMusicState::Stopped && state != HostMusicState::Paused)) {
			return AudioResult::Failed;
		}
		HostMusicInternal* internal = static_cast<HostMusicInternal*>(impl);
		AudioResult res = internal->active_stream->ReadSamples(destBuffer, maxBytes, bytesRead);
		if (res == AudioResult::OK && bytesRead > 0) {
			uint32 effective_vol = muted ? 0 : volume;
			ApplySoftwareVolume(destBuffer, bytesRead, internal->info.format.sample_format, effective_vol);

			uint32 bytes_per_sample = (internal->info.bitsPerSample / 8) * (internal->info.format.channels ? internal->info.format.channels : 1);
			if (bytes_per_sample == 0) bytes_per_sample = 1;
			uint32 samples_in_chunk = bytesRead / bytes_per_sample;
			internal->total_samples_played += samples_in_chunk;

			uint32 rate = internal->info.format.sample_rate ? internal->info.format.sample_rate : 44100;
			internal->current_ms = (uint32)(((uint64)internal->total_samples_played * 1000) / rate);
		}
		return res;
	}

	bool HostMusic::Update() {
		if (state != HostMusicState::Playing || !impl) return false;
		HostMusicInternal* internal = static_cast<HostMusicInternal*>(impl);

		// Read next chunk from audio stream when current chunk is exhausted
		if (internal->chunk_offset >= internal->chunk_bytes_read) {
			internal->chunk_offset = 0;
			internal->chunk_bytes_read = 0;
			AudioResult res = internal->active_stream->ReadSamples(
				internal->raw_buffer, kChunkBufferSize, internal->chunk_bytes_read);

			if (res == AudioResult::EndOfStream || (res == AudioResult::OK && internal->chunk_bytes_read == 0)) {
				if (is_looping) {
					Seek(0);
					return true;
				} else {
#if defined(_ACCM)
					uni::AudioPlayRequest drain_req{};
					drain_req.format = internal->info.format;
					SendAudioRequest(AudioMsg::STREAM_DRAIN, drain_req);
#endif
					state = HostMusicState::Finished;
					if (OnFinished) {
						OnFinished(this, user_data);
					}
					return false;
				}
			}

			if (res != AudioResult::OK) {
				Stop();
				state = HostMusicState::Error;
				return false;
			}

			MemCopyN(internal->proc_buffer, internal->raw_buffer, internal->chunk_bytes_read);
			uint32 effective_vol = muted ? 0 : volume;
			ApplySoftwareVolume(internal->proc_buffer, internal->chunk_bytes_read,
				internal->info.format.sample_format, effective_vol);
		}

#if defined(_ACCM)
		uni::AudioPlayRequest write_req{};
		write_req.format = internal->info.format;
		write_req.buffer.data = internal->proc_buffer + internal->chunk_offset;
		write_req.buffer.byte_count = internal->chunk_bytes_read - internal->chunk_offset;
		stdsint accepted = SendAudioRequest(AudioMsg::STREAM_WRITE, write_req);
		if (accepted < 0) {
			Stop();
			state = HostMusicState::Error;
			return false;
		}
		if (accepted == 0) {
			sysrest(1, 10);
		} else {
			internal->chunk_offset += (uint32)accepted;
		}

		AudioStreamPosition pos{};
		if (SendAudioGetPos(pos)) {
			internal->current_ms = pos.played_ms;
			internal->total_samples_played = pos.played_samples;
		}
#else
		internal->chunk_offset = internal->chunk_bytes_read;
		uint32 bytes_per_sample = (internal->info.bitsPerSample / 8) * (internal->info.format.channels ? internal->info.format.channels : 1);
		if (bytes_per_sample == 0) bytes_per_sample = 1;
		uint32 samples_in_chunk = internal->chunk_bytes_read / bytes_per_sample;
		internal->total_samples_played += samples_in_chunk;
		uint32 rate = internal->info.format.sample_rate ? internal->info.format.sample_rate : 44100;
		internal->current_ms = (uint32)(((uint64)internal->total_samples_played * 1000) / rate);
#endif

		if (OnProgress) {
			OnProgress(this, internal->current_ms, internal->info.durationMs, user_data);
		}
		return true;
	}

	void HostMusic::setVolume(uint32 percent) {
		volume = (percent > 100) ? 100 : percent;
		if (impl) {
			HostMusicInternal* internal = static_cast<HostMusicInternal*>(impl);
			if (internal->chunk_bytes_read > internal->chunk_offset) {
				uint32 remain = internal->chunk_bytes_read - internal->chunk_offset;
				MemCopyN(internal->proc_buffer + internal->chunk_offset,
					internal->raw_buffer + internal->chunk_offset,
					remain);
				uint32 effective_vol = muted ? 0 : volume;
				ApplySoftwareVolume(internal->proc_buffer + internal->chunk_offset,
					remain,
					internal->info.format.sample_format,
					effective_vol);
			}
		}
	}

	uint32 HostMusic::getVolume() const {
		return volume;
	}

	void HostMusic::setMute(bool mute) {
		muted = mute;
		if (impl) {
			HostMusicInternal* internal = static_cast<HostMusicInternal*>(impl);
			if (internal->chunk_bytes_read > internal->chunk_offset) {
				uint32 remain = internal->chunk_bytes_read - internal->chunk_offset;
				MemCopyN(internal->proc_buffer + internal->chunk_offset,
					internal->raw_buffer + internal->chunk_offset,
					remain);
				uint32 effective_vol = muted ? 0 : volume;
				ApplySoftwareVolume(internal->proc_buffer + internal->chunk_offset,
					remain,
					internal->info.format.sample_format,
					effective_vol);
			}
		}
	}

	bool HostMusic::isMuted() const {
		return muted;
	}

	void HostMusic::setLoop(bool loop_mode) {
		this->is_looping = loop_mode;
	}

	bool HostMusic::isLoop() const {
		return is_looping;
	}

	bool HostMusic::isPlaying() const {
		return state == HostMusicState::Playing;
	}

	bool HostMusic::isPaused() const {
		return state == HostMusicState::Paused;
	}

	uint32 HostMusic::getPositionMs() const {
		if (!impl) return 0;
		return static_cast<const HostMusicInternal*>(impl)->current_ms;
	}

	uint32 HostMusic::getDurationMs() const {
		if (!impl) return 0;
		return static_cast<const HostMusicInternal*>(impl)->info.durationMs;
	}

	bool HostMusic::getInfo(AudioInfo& outInfo) const {
		if (!impl) return false;
		outInfo = static_cast<const HostMusicInternal*>(impl)->info;
		return true;
	}

	HostMusicState HostMusic::getState() const {
		return state;
	}

}

