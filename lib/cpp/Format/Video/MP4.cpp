// ASCII C/C++ TAB4 CRLF
// Docutitle: [Format.Video.Container] ISO Base Media File Format (ISOBMFF / MP4 / MOV) Codec Implementation
// Attribute: Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0; Dosconio Mecocoa, BSD 3-Clause License

#include "../../../../inc/c/format/video/MP4.h"
#include "../../../../inc/c/format/video/H264.h"
#include "../../../../inc/c/format/video/MPEG4.h"
#include "../../../../inc/c/format/audio/MP3.h"
#include "../../../../inc/c/format/audio/AAC.h"
#include "../../../../inc/c/ustring.h"
#include <stdlib.h>

#if defined(_INC_CPP)

namespace {

	static const char* const MP4_EXTENSIONS[] = { ".mp4", ".m4v", ".mov", ".3gp", nullptr };

	class MP4HeapAllocator : public uni::trait::Malloc {
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

	static MP4HeapAllocator s_mp4_allocator;

	class MP4MemoryStorageDevice : public uni::StorageTrait {
	private:
		const byte* m_data;
		stduint     m_size;

	public:
		MP4MemoryStorageDevice(const byte* data, stduint size)
			: m_data(data), m_size(size) {
			Block_Size = 1;
			Block_buffer = (void*)data;
			readable = true;
			writable = false;
		}

		virtual bool Read(stduint BlockIden, void* Dest, stduint Times = 1) override {
			if (!Dest || Times == 0) return false;
			if (BlockIden > m_size || Times > m_size - BlockIden) return false;
			MemCopyN(Dest, m_data + BlockIden, Times);
			return true;
		}

		virtual bool Write(stduint BlockIden, const void* Sors, stduint Times = 1) override {
			return false;
		}

		virtual stduint getUnits() override {
			return m_size;
		}

		virtual int operator[](uint64 bytid) override {
			if (bytid >= m_size) return -1;
			return m_data[bytid];
		}
	};

	static inline uint8 ReadUint8(uni::StorageTrait& storage, uint64 offset) {
		int b = storage[offset];
		return (b >= 0) ? (uint8)b : 0;
	}

	static inline uint16 ReadUint16BE(uni::StorageTrait& storage, uint64 offset) {
		uint32 b0 = ReadUint8(storage, offset);
		uint32 b1 = ReadUint8(storage, offset + 1);
		return (uint16)((b0 << 8) | b1);
	}

	static inline uint32 ReadUint32BE(uni::StorageTrait& storage, uint64 offset) {
		uint32 b0 = ReadUint8(storage, offset);
		uint32 b1 = ReadUint8(storage, offset + 1);
		uint32 b2 = ReadUint8(storage, offset + 2);
		uint32 b3 = ReadUint8(storage, offset + 3);
		return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
	}

	static inline uint64 ReadUint64BE(uni::StorageTrait& storage, uint64 offset) {
		uint64 hi = ReadUint32BE(storage, offset);
		uint64 lo = ReadUint32BE(storage, offset + 4);
		return (hi << 32) | lo;
	}

	static bool StorageReadExact(uni::StorageTrait& storage, uint64 offset, void* dst, size_t len) {
		if (len == 0) return true;
		byte* p = (byte*)dst;
		for (size_t i = 0; i < len; ++i) {
			int b = storage[offset + i];
			if (b < 0) return false;
			p[i] = (byte)b;
		}
		return true;
	}

	// avcC stores each NAL unit with a big-endian length field of (lengthSizeMinusOne + 1) bytes.
	// Rewrite such a run of NAL units as an Annex B stream so the decoder never has to know the
	// width. Returns a malloc'd buffer the caller frees, or nullptr if the run is malformed.
	static byte* LengthPrefixedToAnnexB(const byte* src, size_t size, int length_size, size_t* out_len) {
		if (out_len) *out_len = 0;
		if (!src || size == 0 || length_size < 1 || length_size > 4) return nullptr;
		const size_t n = (size_t)length_size;
		size_t cap = size + (size / n) * (4 - n) + 16; // each NAL grows by 4 - n bytes
		byte* dst = (byte*)malloc(cap);
		if (!dst) return nullptr;
		size_t pos = 0, out = 0;
		while (pos + n <= size) {
			uint32 nal_len = 0;
			for (size_t k = 0; k < n; ++k) nal_len = (nal_len << 8) | (uint32)src[pos + k];
			pos += n;
			if (nal_len == 0 || pos + nal_len > size || out + 4 + nal_len > cap) {
				free(dst);
				return nullptr;
			}
			dst[out++] = 0x00; dst[out++] = 0x00; dst[out++] = 0x00; dst[out++] = 0x01;
			MemCopyN(dst + out, src + pos, nal_len);
			out += nal_len;
			pos += nal_len;
		}
		if (out_len) *out_len = out;
		return dst;
	}

	struct MP4Sample {
		uint64 offset;
		uint32 size;
		uint32 pts_ms;
		bool   is_keyframe;
	};

	struct StscEntry {
		uint32 first_chunk;
		uint32 samples_per_chunk;
		uint32 sample_description_index;
	};



class MP4Stream : public uni::IVideoStream {
	private:
		uni::StorageTrait* m_storage;
		uni::trait::Malloc* m_allocator;
		uni::VideoInfo     m_info;

		uint32 m_video_track_id;
		uint32 m_video_codec_fourcc;
		uint32 m_width;
		uint32 m_height;
		uint32 m_timescale;
		uint64 m_duration;
		uint32 m_frame_rate_num;
		uint32 m_frame_rate_den;

		byte*  m_avcc_data;
		size_t m_avcc_len;
		int    m_nal_len_size;

		MP4Sample* m_samples;
		uint32     m_sample_count;
		uint32     m_curr_frame;
		// Next sample to read.  The decoder hands pictures out in display order (increasing POC),
		// so it can need several samples before one is due, and it still holds pictures after the
		// last sample; m_delivered counts what was actually handed to the caller.
		uint32     m_delivered;

		void*      m_h264_decoder;
		void*      m_mpeg4_decoder;
		bool       m_decoder_initialized;

		uint32 m_audio_track_id;
		uint32 m_audio_codec_fourcc;
		uint32 m_audio_timescale;
		uint64 m_audio_duration;
		uint32 m_audio_channels;
		uint32 m_audio_sample_rate;
		uint32 m_audio_bits_per_sample;

		MP4Sample* m_audio_samples;
		uint32     m_audio_sample_count;
		uint32     m_curr_audio_sample;
		uint32     m_curr_audio_sample_off;

		byte*      m_audio_es;
		stduint    m_audio_es_len;
		MP4MemoryStorageDevice* m_audio_storage;
		uni::IAudioStream*      m_audio_stream;
		// AAC access units carry no framing, so the container keeps their sample table and hands
		// it to the codec, which owns no table of its own for this path.
		uni::AACRawFrame*       m_aac_frames;

	public:
		MP4Stream(uni::StorageTrait* storage, uni::trait::Malloc* allocator)
			: m_storage(storage), m_allocator(allocator),
			  m_video_track_id(0), m_video_codec_fourcc(0),
			  m_width(0), m_height(0), m_timescale(1000), m_duration(0),
			  m_frame_rate_num(30), m_frame_rate_den(1),
			  m_avcc_data(nullptr), m_avcc_len(0), m_nal_len_size(4),
			  m_samples(nullptr), m_sample_count(0), m_curr_frame(0),
			  m_delivered(0),
			  m_h264_decoder(nullptr), m_mpeg4_decoder(nullptr),
			  m_decoder_initialized(false),
			  m_audio_track_id(0), m_audio_codec_fourcc(0),
			  m_audio_timescale(44100), m_audio_duration(0),
			  m_audio_channels(0), m_audio_sample_rate(0), m_audio_bits_per_sample(16),
			  m_audio_samples(nullptr), m_audio_sample_count(0),
			  m_curr_audio_sample(0), m_curr_audio_sample_off(0),
			  m_audio_es(nullptr), m_audio_es_len(0),
			  m_audio_storage(nullptr), m_audio_stream(nullptr),
			  m_aac_frames(nullptr) {
			m_info = {};
		}

		virtual ~MP4Stream() {
			if (m_h264_decoder) {
				H264Decoder_Destroy(m_h264_decoder);
				m_h264_decoder = nullptr;
			}
			if (m_mpeg4_decoder) {
				MPEG4Decoder_Destroy(m_mpeg4_decoder);
				m_mpeg4_decoder = nullptr;
			}
			if (m_samples) {
				free(m_samples);
				m_samples = nullptr;
			}
			if (m_audio_samples) {
				free(m_audio_samples);
				m_audio_samples = nullptr;
			}
			if (m_avcc_data) {
				free(m_avcc_data);
				m_avcc_data = nullptr;
			}
			if (m_audio_stream) {
				m_audio_stream->Release();
				m_audio_stream = nullptr;
			}
			// The AAC stream only borrows this table, so the container releases it.
			if (m_aac_frames) {
				s_mp4_allocator.deallocate(m_aac_frames,
					(stduint)m_audio_sample_count * sizeof(uni::AACRawFrame));
				m_aac_frames = nullptr;
			}
			if (m_audio_storage) {
				delete m_audio_storage;
				m_audio_storage = nullptr;
			}
			if (m_audio_es) {
				free(m_audio_es);
				m_audio_es = nullptr;
			}
		}

		virtual void Release() override {
			delete this;
		}

		bool Init() {
			if (!m_storage) return false;
			uint64 file_size = m_storage->getUnits() * (m_storage->Block_Size ? m_storage->Block_Size : 4096);
			if (file_size < 8) return false;

			if (!ParseRootBoxes(0, file_size)) {
				return false;
			}

			if (m_sample_count == 0 || m_width == 0 || m_height == 0) {
				return false;
			}

			m_info.containerFormat = uni::VideoContainerFormat::MP4;
			m_info.videoFormat.width = m_width;
			m_info.videoFormat.height = m_height;
			m_info.videoFormat.frame_rate_num = m_frame_rate_num ? m_frame_rate_num : 30;
			m_info.videoFormat.frame_rate_den = m_frame_rate_den ? m_frame_rate_den : 1;
			m_info.totalFrames = m_sample_count;

			if (m_video_codec_fourcc == MP4_CODEC_AVC1 || m_video_codec_fourcc == MP4_CODEC_AVC3) {
				m_info.videoFormat.codec_type = uni::VideoCodecType::H264;
			} else if (m_video_codec_fourcc == MP4_CODEC_MP4V) {
				m_info.videoFormat.codec_type = uni::VideoCodecType::MPEG4;
			} else {
				m_info.videoFormat.codec_type = uni::VideoCodecType::H264;
			}

			if (m_timescale > 0 && m_duration > 0) {
				m_info.durationMs = (uint32)((m_duration * 1000ULL) / m_timescale);
			} else if (m_sample_count > 0 && m_frame_rate_num > 0) {
				m_info.durationMs = (uint32)(((uint64)m_sample_count * 1000ULL * m_frame_rate_den) / m_frame_rate_num);
			}

			if (m_audio_sample_count > 0 && m_audio_channels > 0 && m_audio_sample_rate > 0) {
				OpenAudioStream();
			}

			return true;
		}

		virtual uni::VideoResult GetInfo(uni::VideoInfo& outInfo) const override {
			outInfo = m_info;
			return uni::VideoResult::OK;
		}

		virtual uni::VideoResult ReadVideoFrame(uni::VideoFrame& outFrame, uni::trait::Malloc& alloc) override {
			if (!m_samples) {
				return uni::VideoResult::EndOfStream;
			}

			int out_w = 0;
			int out_h = 0;
			uni::Color* pixels = nullptr;
			int frame_type = 0;

			// The decoder releases pictures in display order, so it holds a picture back until the
			// one that has to be shown first has been decoded, and it still holds pictures after
			// the last sample: keep feeding samples until a picture is due, then drain.
			while (!pixels) {
				if (m_curr_frame >= m_sample_count) {
					if (!m_h264_decoder) return uni::VideoResult::EndOfStream;
					pixels = H264Decoder_DecodeFrame(m_h264_decoder, nullptr, 0, &out_w, &out_h, &frame_type);
					if (!pixels) return uni::VideoResult::EndOfStream;
					break;
				}

				const MP4Sample& smp = m_samples[m_curr_frame];
				++m_curr_frame;
				if (smp.size == 0) continue;

				byte* sample_data = (byte*)malloc(smp.size);
				if (!sample_data) {
					return uni::VideoResult::OutOfMemory;
				}

				if (!StorageReadExact(*m_storage, smp.offset, sample_data, smp.size)) {
					free(sample_data);
					return uni::VideoResult::IoError;
				}

				size_t sample_len = smp.size;

				if (m_info.videoFormat.codec_type == uni::VideoCodecType::H264) {
					if (!m_h264_decoder) {
						m_h264_decoder = H264Decoder_Create((int)m_width, (int)m_height);
						if (!m_h264_decoder) {
							free(sample_data);
							return uni::VideoResult::OutOfMemory;
						}
					}

					if (!m_decoder_initialized && m_avcc_data && m_avcc_len >= 7) {
						m_decoder_initialized = true;
						FeedAVCCParameters(m_avcc_data, m_avcc_len);
					}

					// The decoder's length-prefixed path only understands 4-byte length fields, so a
					// sample using any other avcC width is handed over as Annex B instead.
					if (m_nal_len_size != 4) {
						size_t annexb_len = 0;
						byte* annexb = LengthPrefixedToAnnexB(sample_data, smp.size, m_nal_len_size, &annexb_len);
						if (annexb) {
							free(sample_data);
							sample_data = annexb;
							sample_len = annexb_len;
						}
					}

					pixels = H264Decoder_DecodeFrame(m_h264_decoder, sample_data, sample_len, &out_w, &out_h, &frame_type);
				} else if (m_info.videoFormat.codec_type == uni::VideoCodecType::MPEG4) {
					if (!m_mpeg4_decoder) {
						m_mpeg4_decoder = MPEG4Decoder_Create((int)m_width, (int)m_height);
					}
					pixels = MPEG4Decoder_DecodeFrame(m_mpeg4_decoder, sample_data, smp.size, &out_w, &out_h, &frame_type);
				}

				free(sample_data);
			}

			if (!pixels) {
				return uni::VideoResult::Failed;
			}

			// The picture was released in display order, not in the order the samples arrive, so the
			// timestamp follows the delivered frame index (a constant frame duration), not the
			// decoding time of a sample.
			const uint32 frate = m_frame_rate_num ? m_frame_rate_num : 30;
			const uint32 fscale = m_frame_rate_den ? m_frame_rate_den : 1;

			uni::VideoFrameClear(outFrame);
			outFrame.width = (uint32)out_w;
			outFrame.height = (uint32)out_h;
			outFrame.timestampMs = (uint32)(((uint64)m_delivered * 1000ULL * fscale) / frate);
			outFrame.frameIndex = m_delivered;
			outFrame.isKeyFrame = (frame_type == 0);

			outFrame.image.width = (uint32)out_w;
			outFrame.image.height = (uint32)out_h;
			outFrame.image.stride = (uint32)out_w * sizeof(uni::Color);
			outFrame.image.format = uni::PixelFormat::BGRA8888;
			outFrame.image.colorSpace = uni::ColorSpace::SRGB;
			outFrame.image.pixels = pixels;
			outFrame.image.size = (size_t)out_w * (size_t)out_h * sizeof(uni::Color);
			outFrame.image.allocator = nullptr;

			++m_delivered;
			return uni::VideoResult::OK;
		}

		virtual uni::VideoResult ReadAudioSamples(void* destBuffer, uint32 maxBytes, uint32& bytesRead) override {
			bytesRead = 0;
			if (!destBuffer || maxBytes == 0) {
				return uni::VideoResult::InvalidArgument;
			}

			if (m_audio_stream) {
				uni::AudioResult ares = m_audio_stream->ReadSamples(destBuffer, maxBytes, bytesRead);
				if (ares == uni::AudioResult::OK) return uni::VideoResult::OK;
				if (ares == uni::AudioResult::EndOfStream) return uni::VideoResult::EndOfStream;
				return uni::VideoResult::Failed;
			}

			if (!m_info.hasAudio || !m_audio_samples || m_audio_sample_count == 0) {
				return uni::VideoResult::InvalidArgument;
			}

			// Anything that did not get an IAudioStream is uncompressed PCM
			// (sowt / twos / raw / in24 / lpcm), which needs no codec at all.
			byte* out_ptr = (byte*)destBuffer;
			while (maxBytes > 0 && m_curr_audio_sample < m_audio_sample_count) {
				const MP4Sample& smp = m_audio_samples[m_curr_audio_sample];
				uint32 avail = (smp.size > m_curr_audio_sample_off) ? (smp.size - m_curr_audio_sample_off) : 0;

				if (avail == 0) {
					m_curr_audio_sample++;
					m_curr_audio_sample_off = 0;
					continue;
				}

				uint32 to_read = (maxBytes < avail) ? maxBytes : avail;
				if (!StorageReadExact(*m_storage, smp.offset + m_curr_audio_sample_off, out_ptr, to_read)) {
					return bytesRead > 0 ? uni::VideoResult::OK : uni::VideoResult::IoError;
				}

				if (m_audio_codec_fourcc == ISOBMFF_FOURCC('t', 'w', 'o', 's') && m_audio_bits_per_sample == 16) {
					for (uint32 i = 0; i + 1 < to_read; i += 2) {
						byte tmp = out_ptr[i];
						out_ptr[i] = out_ptr[i + 1];
						out_ptr[i + 1] = tmp;
					}
				}

				out_ptr += to_read;
				bytesRead += to_read;
				maxBytes -= to_read;
				m_curr_audio_sample_off += to_read;

				if (m_curr_audio_sample_off >= smp.size) {
					m_curr_audio_sample++;
					m_curr_audio_sample_off = 0;
				}
			}

			if (bytesRead == 0 && m_curr_audio_sample >= m_audio_sample_count) {
				return uni::VideoResult::EndOfStream;
			}
			return uni::VideoResult::OK;
		}

		virtual uni::VideoResult SeekFrame(uint32 frameIndex) override {
			if (frameIndex >= m_sample_count) return uni::VideoResult::EndOfStream;
			m_curr_frame = frameIndex;
			return uni::VideoResult::OK;
		}

		virtual uni::VideoResult SeekTime(uint32 timestampMs) override {
			if (!m_samples || m_sample_count == 0) return uni::VideoResult::Failed;
			for (uint32 i = 0; i < m_sample_count; ++i) {
				if (m_samples[i].pts_ms >= timestampMs) {
					m_curr_frame = i;
					break;
				}
			}

			if (m_audio_samples && m_audio_sample_count > 0) {
				for (uint32 i = 0; i < m_audio_sample_count; ++i) {
					if (m_audio_samples[i].pts_ms >= timestampMs) {
						m_curr_audio_sample = i;
						m_curr_audio_sample_off = 0;
						break;
					}
				}
			}

			if (m_audio_stream) {
				uint32 target_sample = (uint32)(((uint64)timestampMs * (m_audio_sample_rate ? m_audio_sample_rate : 44100)) / 1000);
				m_audio_stream->Seek(target_sample);
			}

			return uni::VideoResult::OK;
		}

	private:
		bool OpenAudioStream() {
			if (m_audio_sample_count == 0 || !m_audio_samples) return false;

			// Raw AAC access units are not self-delimiting, so the codec cannot walk the track on
			// its own: hand it the sample table this container already parsed.  Until this was
			// wired up the track was refused outright, which silenced every mp4 with mp4a audio.
			if (m_audio_codec_fourcc == MP4_CODEC_MP4A ||
				m_audio_codec_fourcc == ISOBMFF_FOURCC('m', 'p', '4', 'a')) {
				m_aac_frames = (uni::AACRawFrame*)s_mp4_allocator.allocate(
					(stduint)m_audio_sample_count * sizeof(uni::AACRawFrame), 3);
				if (!m_aac_frames) return false;
				for (uint32 i = 0; i < m_audio_sample_count; ++i) {
					m_aac_frames[i].offset = m_audio_samples[i].offset;
					m_aac_frames[i].size = m_audio_samples[i].size;
				}
				uint32 rate = m_audio_sample_rate ? m_audio_sample_rate : 48000;
				m_audio_stream = uni::AAC_OpenRawStream(
					*m_storage, m_aac_frames, m_audio_sample_count,
					rate, m_audio_channels, s_mp4_allocator);
				if (!m_audio_stream) return false;

				uni::AudioInfo a_info{};
				if (m_audio_stream->GetInfo(a_info) != uni::AudioResult::OK) return false;
				m_info.hasAudio = true;
				m_info.audioInfo = a_info;
				if (m_info.audioInfo.durationMs == 0) m_info.audioInfo.durationMs = m_info.durationMs;
				return true;
			}

			if (m_audio_codec_fourcc == ISOBMFF_FOURCC('s', 'o', 'w', 't') ||
				m_audio_codec_fourcc == ISOBMFF_FOURCC('t', 'w', 'o', 's') ||
				m_audio_codec_fourcc == ISOBMFF_FOURCC('r', 'a', 'w', ' ') ||
				m_audio_codec_fourcc == ISOBMFF_FOURCC('i', 'n', '2', '4') ||
				m_audio_codec_fourcc == ISOBMFF_FOURCC('l', 'p', 'c', 'm')) {
				m_info.hasAudio = true;
				m_info.audioInfo.format.channels = (uint16)m_audio_channels;
				m_info.audioInfo.format.sample_rate = m_audio_sample_rate;
				m_info.audioInfo.format.sample_format = (m_audio_bits_per_sample == 8) ? uni::AudioSampleFormat::U8 : uni::AudioSampleFormat::S16LE;
				m_info.audioInfo.bitsPerSample = m_audio_bits_per_sample ? m_audio_bits_per_sample : 16;
				m_info.audioInfo.containerFormat = uni::AudioContainerFormat::WAV;
				m_info.audioInfo.durationMs = m_info.durationMs;
				return true;
			}

			stduint total_bytes = 0;
			for (uint32 i = 0; i < m_audio_sample_count; ++i) {
				total_bytes += m_audio_samples[i].size;
			}
			if (total_bytes == 0) return false;

			m_audio_es = (byte*)malloc(total_bytes);
			if (!m_audio_es) return false;

			stduint off = 0;
			for (uint32 i = 0; i < m_audio_sample_count; ++i) {
				const uint32 len = m_audio_samples[i].size;
				if (len == 0) continue;
				if (!StorageReadExact(*m_storage, m_audio_samples[i].offset, m_audio_es + off, len)) {
					free(m_audio_es);
					m_audio_es = nullptr;
					return false;
				}
				off += len;
			}
			m_audio_es_len = off;

			m_audio_storage = new MP4MemoryStorageDevice(m_audio_es, m_audio_es_len);
			if (!m_audio_storage) {
				free(m_audio_es);
				m_audio_es = nullptr;
				return false;
			}

			uni::MP3Codec mp3_codec;
			bool matched = false;
			if (mp3_codec.Probe(*m_audio_storage, matched) == uni::AudioResult::OK && matched) {
				if (mp3_codec.OpenStream(*m_audio_storage, m_audio_stream, s_mp4_allocator) == uni::AudioResult::OK && m_audio_stream) {
					uni::AudioInfo a_info{};
					if (m_audio_stream->GetInfo(a_info) == uni::AudioResult::OK) {
						m_info.hasAudio = true;
						m_info.audioInfo = a_info;
						m_info.audioInfo.containerFormat = uni::AudioContainerFormat::MP3;
						if (m_info.audioInfo.durationMs == 0) m_info.audioInfo.durationMs = m_info.durationMs;
						return true;
					}
				}
			}

			m_info.hasAudio = true;
			m_info.audioInfo.format.channels = (uint16)m_audio_channels;
			m_info.audioInfo.format.sample_rate = m_audio_sample_rate;
			m_info.audioInfo.format.sample_format = (m_audio_bits_per_sample == 8) ? uni::AudioSampleFormat::U8 : uni::AudioSampleFormat::S16LE;
			m_info.audioInfo.bitsPerSample = m_audio_bits_per_sample ? m_audio_bits_per_sample : 16;
			m_info.audioInfo.durationMs = m_info.durationMs;
			return true;
		}

		void FeedAVCCParameters(const byte* avcc, size_t len) {
			if (!m_h264_decoder || !avcc || len < 7) return;

			uint8 num_sps = avcc[5] & 0x1F;
			size_t offset = 6;

			for (uint8 i = 0; i < num_sps && offset + 2 <= len; ++i) {
				uint16 sps_len = ((uint16)avcc[offset] << 8) | avcc[offset + 1];
				offset += 2;
				if (offset + sps_len <= len && sps_len > 0) {
					// Annex B start code, so the width of the avcC length field never matters here.
					byte* buf = (byte*)malloc(4 + sps_len);
					if (buf) {
						buf[0] = 0x00; buf[1] = 0x00; buf[2] = 0x00; buf[3] = 0x01;
						MemCopyN(buf + 4, avcc + offset, sps_len);
						int dummy_w = 0, dummy_h = 0, dummy_type = 0;
						uni::Color* p = H264Decoder_DecodeFrame(m_h264_decoder, buf, 4 + sps_len, &dummy_w, &dummy_h, &dummy_type);
						if (p) free(p);
						free(buf);
					}
				}
				offset += sps_len;
			}

			if (offset < len) {
				uint8 num_pps = avcc[offset++];
				for (uint8 i = 0; i < num_pps && offset + 2 <= len; ++i) {
					uint16 pps_len = ((uint16)avcc[offset] << 8) | avcc[offset + 1];
					offset += 2;
					if (offset + pps_len <= len && pps_len > 0) {
						byte* buf = (byte*)malloc(4 + pps_len);
						if (buf) {
							buf[0] = 0x00; buf[1] = 0x00; buf[2] = 0x00; buf[3] = 0x01;
							MemCopyN(buf + 4, avcc + offset, pps_len);
							int dummy_w = 0, dummy_h = 0, dummy_type = 0;
							uni::Color* p = H264Decoder_DecodeFrame(m_h264_decoder, buf, 4 + pps_len, &dummy_w, &dummy_h, &dummy_type);
							if (p) free(p);
							free(buf);
						}
					}
					offset += pps_len;
				}
			}
		}

		bool ParseRootBoxes(uint64 start, uint64 end) {
			uint64 cur = start;
			while (cur + 8 <= end) {
				uint32 size32 = ReadUint32BE(*m_storage, cur);
				uint32 type = ReadUint32BE(*m_storage, cur + 4);
				uint64 box_len = size32;
				uint64 hdr_len = 8;

				if (size32 == 1) {
					if (cur + 16 > end) break;
					box_len = ReadUint64BE(*m_storage, cur + 8);
					hdr_len = 16;
				} else if (size32 == 0) {
					box_len = end - cur;
				}

				if (box_len < hdr_len || cur + box_len > end) {
					break;
				}

				if (type == MP4_BOX_MOOV) {
					ParseMoov(cur + hdr_len, cur + box_len);
				}

				cur += box_len;
			}
			return m_sample_count > 0;
		}

		void ParseMoov(uint64 start, uint64 end) {
			uint64 cur = start;
			while (cur + 8 <= end) {
				uint32 size32 = ReadUint32BE(*m_storage, cur);
				uint32 type = ReadUint32BE(*m_storage, cur + 4);
				uint64 box_len = (size32 == 1) ? ReadUint64BE(*m_storage, cur + 8) : size32;
				uint64 hdr_len = (size32 == 1) ? 16 : 8;
				if (size32 == 0) box_len = end - cur;
				if (box_len < hdr_len || cur + box_len > end) break;

				if (type == MP4_BOX_MVHD) {
					uint8 ver = ReadUint8(*m_storage, cur + hdr_len);
					if (ver == 0) {
						m_timescale = ReadUint32BE(*m_storage, cur + hdr_len + 12);
						m_duration = ReadUint32BE(*m_storage, cur + hdr_len + 16);
					} else if (ver == 1) {
						m_timescale = ReadUint32BE(*m_storage, cur + hdr_len + 20);
						m_duration = ReadUint64BE(*m_storage, cur + hdr_len + 24);
					}
				} else if (type == MP4_BOX_TRAK) {
					ParseTrak(cur + hdr_len, cur + box_len);
				}

				cur += box_len;
			}
		}

		void ParseTrak(uint64 start, uint64 end) {
			uint64 cur = start;
			bool is_video = false;
			bool is_audio = false;
			uint32 t_width = 0, t_height = 0;
			uint32 t_timescale = m_timescale;
			uint64 t_duration = m_duration;
			uint64 mdia_pos = 0, mdia_end = 0;

			while (cur + 8 <= end) {
				uint32 size32 = ReadUint32BE(*m_storage, cur);
				uint32 type = ReadUint32BE(*m_storage, cur + 4);
				uint64 box_len = (size32 == 1) ? ReadUint64BE(*m_storage, cur + 8) : size32;
				uint64 hdr_len = (size32 == 1) ? 16 : 8;
				if (size32 == 0) box_len = end - cur;
				if (box_len < hdr_len || cur + box_len > end) break;

				if (type == MP4_BOX_TKHD) {
					uint8 ver = ReadUint8(*m_storage, cur + hdr_len);
					uint64 tkhd_payload = cur + hdr_len;
					uint64 w_offset = (ver == 0) ? (tkhd_payload + 76) : (tkhd_payload + 88);
					t_width = ReadUint32BE(*m_storage, w_offset) >> 16;
					t_height = ReadUint32BE(*m_storage, w_offset + 4) >> 16;
				} else if (type == MP4_BOX_MDIA) {
					mdia_pos = cur + hdr_len;
					mdia_end = cur + box_len;
				}

				cur += box_len;
			}

			if (mdia_pos > 0) {
				cur = mdia_pos;
				while (cur + 8 <= mdia_end) {
					uint32 size32 = ReadUint32BE(*m_storage, cur);
					uint32 type = ReadUint32BE(*m_storage, cur + 4);
					uint64 box_len = (size32 == 1) ? ReadUint64BE(*m_storage, cur + 8) : size32;
					uint64 hdr_len = (size32 == 1) ? 16 : 8;
					if (size32 == 0) box_len = mdia_end - cur;
					if (box_len < hdr_len || cur + box_len > mdia_end) break;

					if (type == MP4_BOX_HDLR) {
						uint32 h_type = ReadUint32BE(*m_storage, cur + hdr_len + 8);
						if (h_type == MP4_HDLR_VIDE) {
							is_video = true;
						} else if (h_type == MP4_HDLR_SOUN) {
							is_audio = true;
						}
					} else if (type == MP4_BOX_MDHD) {
						uint8 ver = ReadUint8(*m_storage, cur + hdr_len);
						if (ver == 0) {
							t_timescale = ReadUint32BE(*m_storage, cur + hdr_len + 12);
							t_duration = ReadUint32BE(*m_storage, cur + hdr_len + 16);
						} else if (ver == 1) {
							t_timescale = ReadUint32BE(*m_storage, cur + hdr_len + 20);
							t_duration = ReadUint64BE(*m_storage, cur + hdr_len + 24);
						}
					}

					cur += box_len;
				}

				if (is_video && m_sample_count == 0) {
					m_width = t_width;
					m_height = t_height;
					m_timescale = t_timescale ? t_timescale : 1000;
					m_duration = t_duration;

					cur = mdia_pos;
					while (cur + 8 <= mdia_end) {
						uint32 size32 = ReadUint32BE(*m_storage, cur);
						uint32 type = ReadUint32BE(*m_storage, cur + 4);
						uint64 box_len = (size32 == 1) ? ReadUint64BE(*m_storage, cur + 8) : size32;
						uint64 hdr_len = (size32 == 1) ? 16 : 8;
						if (size32 == 0) box_len = mdia_end - cur;
						if (box_len < hdr_len || cur + box_len > mdia_end) break;

						if (type == MP4_BOX_MINF) {
							ParseMinf(cur + hdr_len, cur + box_len, false);
						}

						cur += box_len;
					}
				} else if (is_audio && m_audio_sample_count == 0) {
					m_audio_timescale = t_timescale ? t_timescale : 44100;
					m_audio_duration = t_duration;

					cur = mdia_pos;
					while (cur + 8 <= mdia_end) {
						uint32 size32 = ReadUint32BE(*m_storage, cur);
						uint32 type = ReadUint32BE(*m_storage, cur + 4);
						uint64 box_len = (size32 == 1) ? ReadUint64BE(*m_storage, cur + 8) : size32;
						uint64 hdr_len = (size32 == 1) ? 16 : 8;
						if (size32 == 0) box_len = mdia_end - cur;
						if (box_len < hdr_len || cur + box_len > mdia_end) break;

						if (type == MP4_BOX_MINF) {
							ParseMinf(cur + hdr_len, cur + box_len, true);
						}

						cur += box_len;
					}
				}
			}
		}

		void ParseMinf(uint64 start, uint64 end, bool is_audio) {
			uint64 cur = start;
			while (cur + 8 <= end) {
				uint32 size32 = ReadUint32BE(*m_storage, cur);
				uint32 type = ReadUint32BE(*m_storage, cur + 4);
				uint64 box_len = (size32 == 1) ? ReadUint64BE(*m_storage, cur + 8) : size32;
				uint64 hdr_len = (size32 == 1) ? 16 : 8;
				if (size32 == 0) box_len = end - cur;
				if (box_len < hdr_len || cur + box_len > end) break;

				if (type == MP4_BOX_STBL) {
					ParseStbl(cur + hdr_len, cur + box_len, is_audio);
				}

				cur += box_len;
			}
		}

		void ParseStbl(uint64 start, uint64 end, bool is_audio) {
			uint64 cur = start;

			uint64 stsd_pos = 0, stsd_end = 0;
			uint64 stts_pos = 0, stts_end = 0;
			uint64 stss_pos = 0, stss_end = 0;
			uint64 stsc_pos = 0, stsc_end = 0;
			uint64 stsz_pos = 0, stsz_end = 0;
			uint64 stco_pos = 0, stco_end = 0;
			uint64 co64_pos = 0, co64_end = 0;

			while (cur + 8 <= end) {
				uint32 size32 = ReadUint32BE(*m_storage, cur);
				uint32 type = ReadUint32BE(*m_storage, cur + 4);
				uint64 box_len = (size32 == 1) ? ReadUint64BE(*m_storage, cur + 8) : size32;
				uint64 hdr_len = (size32 == 1) ? 16 : 8;
				if (size32 == 0) box_len = end - cur;
				if (box_len < hdr_len || cur + box_len > end) break;

				if (type == MP4_BOX_STSD) { stsd_pos = cur + hdr_len; stsd_end = cur + box_len; }
				else if (type == MP4_BOX_STTS) { stts_pos = cur + hdr_len; stts_end = cur + box_len; }
				else if (type == MP4_BOX_STSS) { stss_pos = cur + hdr_len; stss_end = cur + box_len; }
				else if (type == MP4_BOX_STSC) { stsc_pos = cur + hdr_len; stsc_end = cur + box_len; }
				else if (type == MP4_BOX_STSZ) { stsz_pos = cur + hdr_len; stsz_end = cur + box_len; }
				else if (type == MP4_BOX_STCO) { stco_pos = cur + hdr_len; stco_end = cur + box_len; }
				else if (type == MP4_BOX_CO64) { co64_pos = cur + hdr_len; co64_end = cur + box_len; }

				cur += box_len;
			}

			// 1. Parse STSD
			if (stsd_pos > 0 && stsd_pos + 8 <= stsd_end) {
				uint32 entry_count = ReadUint32BE(*m_storage, stsd_pos + 4);
				uint64 e_cur = stsd_pos + 8;
				for (uint32 i = 0; i < entry_count && e_cur + 8 <= stsd_end; ++i) {
					uint32 e_size = ReadUint32BE(*m_storage, e_cur);
					uint32 e_type = ReadUint32BE(*m_storage, e_cur + 4);
					if (e_size < 8 || e_cur + e_size > stsd_end) break;

					if (!is_audio) {
						m_video_codec_fourcc = e_type;
						if (e_type == MP4_CODEC_AVC1 || e_type == MP4_CODEC_AVC3 || e_type == MP4_CODEC_MP4V) {
							if (e_cur + 36 <= stsd_end) {
								// VisualSampleEntry: width/height at +32/+34 (audio uses +24/+26).
								uint16 w = ReadUint16BE(*m_storage, e_cur + 32);
								uint16 h = ReadUint16BE(*m_storage, e_cur + 34);
								if (w > 0) m_width = w;
								if (h > 0) m_height = h;
							}

							// VisualSampleEntry: 8 byte box header plus 78 bytes of fixed fields
							// (reserved, data_reference_index, pre_defined, width, height,
							// resolutions, frame_count, compressorname, depth, pre_defined).
							// The configuration boxes (avcC / esds) start only after them; a
							// byte-wise scan over the fixed fields can run into a byte pattern
							// that looks like a box and then jump right over the real avcC.
							uint64 sub_box = e_cur + 8 + 78;
							uint64 sub_end = e_cur + e_size;
							while (sub_box + 8 <= sub_end) {
								uint32 sb_size = ReadUint32BE(*m_storage, sub_box);
								uint32 sb_type = ReadUint32BE(*m_storage, sub_box + 4);
								if (sb_size < 8 || sub_box + sb_size > sub_end) break;

								if (sb_type == MP4_CODEC_AVCC) {
									size_t avcc_len = (size_t)(sb_size - 8);
									if (avcc_len > 0) {
										m_avcc_data = (byte*)malloc(avcc_len);
										if (m_avcc_data) {
											StorageReadExact(*m_storage, sub_box + 8, m_avcc_data, avcc_len);
											m_avcc_len = avcc_len;
											if (avcc_len >= 5) m_nal_len_size = (int)(m_avcc_data[4] & 0x03) + 1;
										}
									}
									break;
								}
								sub_box += sb_size;
							}
						}
					} else {
						m_audio_codec_fourcc = e_type;
						if (e_cur + 36 <= stsd_end) {
							uint16 ch = ReadUint16BE(*m_storage, e_cur + 24);
							uint16 bps = ReadUint16BE(*m_storage, e_cur + 26);
							uint32 srate = ReadUint32BE(*m_storage, e_cur + 32) >> 16;
							if (ch > 0) m_audio_channels = ch;
							if (bps > 0) m_audio_bits_per_sample = bps;
							if (srate > 0) m_audio_sample_rate = srate;
						}
						if (m_audio_channels == 0) m_audio_channels = 2;
						if (m_audio_sample_rate == 0) m_audio_sample_rate = (m_audio_timescale ? m_audio_timescale : 44100);
						if (m_audio_bits_per_sample == 0) m_audio_bits_per_sample = 16;
					}
					e_cur += e_size;
				}
			}

			// 2. Parse STSZ (Sample Sizes)
			uint32 default_sample_size = 0;
			uint32 sample_count = 0;
			uint32* sample_sizes = nullptr;

			if (stsz_pos > 0 && stsz_pos + 12 <= stsz_end) {
				default_sample_size = ReadUint32BE(*m_storage, stsz_pos + 4);
				sample_count = ReadUint32BE(*m_storage, stsz_pos + 8);

				if (sample_count > 0) {
					sample_sizes = (uint32*)malloc(sample_count * sizeof(uint32));
					if (sample_sizes) {
						if (default_sample_size > 0) {
							for (uint32 i = 0; i < sample_count; ++i) {
								sample_sizes[i] = default_sample_size;
							}
						} else {
							for (uint32 i = 0; i < sample_count && stsz_pos + 12 + (i + 1) * 4 <= stsz_end; ++i) {
								sample_sizes[i] = ReadUint32BE(*m_storage, stsz_pos + 12 + i * 4);
							}
						}
					}
				}
			}

			if (sample_count == 0 || !sample_sizes) {
				if (sample_sizes) free(sample_sizes);
				return;
			}

			// 3. Parse STCO / CO64 (Chunk Offsets)
			uint32 chunk_count = 0;
			uint64* chunk_offsets = nullptr;

			if (stco_pos > 0 && stco_pos + 8 <= stco_end) {
				chunk_count = ReadUint32BE(*m_storage, stco_pos + 4);
				if (chunk_count > 0) {
					chunk_offsets = (uint64*)malloc(chunk_count * sizeof(uint64));
					if (chunk_offsets) {
						for (uint32 i = 0; i < chunk_count && stco_pos + 8 + (i + 1) * 4 <= stco_end; ++i) {
							chunk_offsets[i] = ReadUint32BE(*m_storage, stco_pos + 8 + i * 4);
						}
					}
				}
			} else if (co64_pos > 0 && co64_pos + 8 <= co64_end) {
				chunk_count = ReadUint32BE(*m_storage, co64_pos + 4);
				if (chunk_count > 0) {
					chunk_offsets = (uint64*)malloc(chunk_count * sizeof(uint64));
					if (chunk_offsets) {
						for (uint32 i = 0; i < chunk_count && co64_pos + 8 + (i + 1) * 8 <= co64_end; ++i) {
							chunk_offsets[i] = ReadUint64BE(*m_storage, co64_pos + 8 + i * 8);
						}
					}
				}
			}

			// 4. Parse STSC (Sample to Chunk)
			uint32 stsc_count = 0;
			StscEntry* stsc_entries = nullptr;

			if (stsc_pos > 0 && stsc_pos + 8 <= stsc_end) {
				stsc_count = ReadUint32BE(*m_storage, stsc_pos + 4);
				if (stsc_count > 0) {
					stsc_entries = (StscEntry*)malloc(stsc_count * sizeof(StscEntry));
					if (stsc_entries) {
						for (uint32 i = 0; i < stsc_count && stsc_pos + 8 + (i + 1) * 12 <= stsc_end; ++i) {
							uint64 off = stsc_pos + 8 + i * 12;
							stsc_entries[i].first_chunk = ReadUint32BE(*m_storage, off);
							stsc_entries[i].samples_per_chunk = ReadUint32BE(*m_storage, off + 4);
							stsc_entries[i].sample_description_index = ReadUint32BE(*m_storage, off + 8);
						}
					}
				}
			}

			// 5. Build Sample Table
			if (chunk_offsets && stsc_entries && chunk_count > 0 && stsc_count > 0) {
				MP4Sample* smp_arr = (MP4Sample*)malloc(sample_count * sizeof(MP4Sample));
				if (smp_arr) {
					MemSet(smp_arr, 0, sample_count * sizeof(MP4Sample));
					uint32 smp_idx = 0;
					uint32 stsc_idx = 0;

					for (uint32 c = 0; c < chunk_count && smp_idx < sample_count; ++c) {
						uint32 chunk_1based = c + 1;
						if (stsc_idx + 1 < stsc_count && chunk_1based >= stsc_entries[stsc_idx + 1].first_chunk) {
							stsc_idx++;
						}

						uint32 smp_per_chunk = stsc_entries[stsc_idx].samples_per_chunk;
						uint64 cur_chunk_offset = chunk_offsets[c];

						for (uint32 k = 0; k < smp_per_chunk && smp_idx < sample_count; ++k) {
							smp_arr[smp_idx].offset = cur_chunk_offset;
							smp_arr[smp_idx].size = sample_sizes[smp_idx];
							smp_arr[smp_idx].is_keyframe = true;
							cur_chunk_offset += sample_sizes[smp_idx];
							smp_idx++;
						}
					}

					if (!is_audio) {
						m_samples = smp_arr;
						m_sample_count = smp_idx;
					} else {
						m_audio_samples = smp_arr;
						m_audio_sample_count = smp_idx;
					}
				}
			}

			MP4Sample* target_samples = is_audio ? m_audio_samples : m_samples;
			uint32 target_count = is_audio ? m_audio_sample_count : m_sample_count;
			uint32 cur_timescale = is_audio ? (m_audio_timescale ? m_audio_timescale : 44100) : (m_timescale ? m_timescale : 1000);

			// 6. Parse STSS (Sync Samples) for Video
			if (!is_audio && target_samples && stss_pos > 0 && stss_pos + 8 <= stss_end) {
				uint32 stss_count = ReadUint32BE(*m_storage, stss_pos + 4);
				if (stss_count > 0) {
					for (uint32 i = 0; i < target_count; ++i) {
						target_samples[i].is_keyframe = false;
					}
					for (uint32 i = 0; i < stss_count && stss_pos + 8 + (i + 1) * 4 <= stss_end; ++i) {
						uint32 key_smp_1based = ReadUint32BE(*m_storage, stss_pos + 8 + i * 4);
						if (key_smp_1based > 0 && key_smp_1based <= target_count) {
							target_samples[key_smp_1based - 1].is_keyframe = true;
						}
					}
				}
			}

			// 7. Parse STTS (Time-to-Sample for PTS & Framerate)
			if (target_samples && stts_pos > 0 && stts_pos + 8 <= stts_end) {
				uint32 stts_count = ReadUint32BE(*m_storage, stts_pos + 4);
				if (stts_count > 0) {
					uint64 current_pts = 0;
					uint32 smp_idx = 0;
					uint64 total_delta_sum = 0;
					uint64 total_delta_samples = 0;

					for (uint32 i = 0; i < stts_count && stts_pos + 8 + (i + 1) * 8 <= stts_end; ++i) {
						uint32 s_count = ReadUint32BE(*m_storage, stts_pos + 8 + i * 8);
						uint32 s_delta = ReadUint32BE(*m_storage, stts_pos + 8 + i * 8 + 4);

						for (uint32 k = 0; k < s_count && smp_idx < target_count; ++k) {
							target_samples[smp_idx].pts_ms = (uint32)((current_pts * 1000ULL) / cur_timescale);
							current_pts += s_delta;
							total_delta_sum += s_delta;
							total_delta_samples++;
							smp_idx++;
						}
					}

					if (!is_audio && total_delta_samples > 0 && total_delta_sum > 0) {
						uint64 avg_delta = total_delta_sum / total_delta_samples;
						if (avg_delta > 0 && m_timescale > 0) {
							m_frame_rate_num = m_timescale;
							m_frame_rate_den = (uint32)avg_delta;
						}
					}
				}
			}

			// Cleanup temporaries
			if (sample_sizes) free(sample_sizes);
			if (chunk_offsets) free(chunk_offsets);
			if (stsc_entries) free(stsc_entries);
		}
	};

} // namespace

namespace uni {

	const char* MP4Codec::GetName() const {
		return "MP4 / ISOBMFF Video";
	}

	VideoContainerFormat MP4Codec::GetFormat() const {
		return VideoContainerFormat::MP4;
	}

	const char* const* MP4Codec::GetExtensions() const {
		return MP4_EXTENSIONS;
	}

	VideoResult MP4Codec::Probe(StorageTrait& storage, bool& matched) const {
		matched = false;
		uint64 file_size = storage.getUnits() * (storage.Block_Size ? storage.Block_Size : 4096);
		if (file_size < 8) return VideoResult::IoError;

		uint32 size32 = ReadUint32BE(storage, 0);
		uint32 type = ReadUint32BE(storage, 4);

		if (type == MP4_BOX_FTYP || type == MP4_BOX_MOOV || type == MP4_BOX_MDAT ||
			type == MP4_BOX_FREE || type == MP4_BOX_SKIP || type == MP4_BOX_WIDE) {
			matched = true;
			return VideoResult::OK;
		}

		return VideoResult::OK;
	}

	VideoResult MP4Codec::ReadInfo(StorageTrait& storage, VideoInfo& outInfo) const {
		MP4Stream stream(&storage, nullptr);
		if (!stream.Init()) {
			return VideoResult::InvalidFormat;
		}
		return stream.GetInfo(outInfo);
	}

	VideoResult MP4Codec::OpenStream(
		StorageTrait& storage,
		IVideoStream*& outStream,
		trait::Malloc& allocator
	) const {
		MP4Stream* stream = new MP4Stream(&storage, &allocator);
		if (!stream) {
			outStream = nullptr;
			return VideoResult::OutOfMemory;
		}
		if (!stream->Init()) {
			delete stream;
			outStream = nullptr;
			return VideoResult::InvalidFormat;
		}
		outStream = stream;
		return VideoResult::OK;
	}

} // namespace uni

#endif // _INC_CPP
