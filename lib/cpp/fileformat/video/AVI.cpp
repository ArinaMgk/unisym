// ASCII C/C++ TAB4 CRLF
// Docutitle: RIFF AVI Codec Implementation
// Attribute: Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0; Dosconio Mecocoa, BSD 3-Clause License

#include "../../../../inc/c/format/video/AVI.h"
#include "../../../../inc/c/format/picture/JPEG.h"
#include "../../../../inc/c/ustring.h"
#include <stdlib.h>

#if defined(_INC_CPP)

namespace {

	static const char* const AVI_EXTENSIONS[] = { ".avi", ".divx", nullptr };

	static bool StorageReadExact(uni::StorageTrait& storage, uint64 offset, void* dst, size_t len) {
		if (len == 0) return true;
		byte* p = (byte*)dst;
		stduint b_size = storage.Block_Size ? storage.Block_Size : 4096;

		uint64 cur = offset;
		size_t remaining = len;

		while (remaining > 0) {
			stduint blk = (stduint)(cur / b_size);
			stduint off = (stduint)(cur % b_size);
			stduint avail_in_blk = b_size - off;
			stduint to_read = (avail_in_blk < remaining) ? avail_in_blk : (stduint)remaining;

			// Ensure this block is cached in storage
			int b0 = storage[cur];
			if (b0 < 0) return false;

			if (storage.Block_buffer) {
				MemCopyN(p, (const byte*)storage.Block_buffer + off, to_read);
			} else {
				p[0] = (byte)b0;
				for (size_t i = 1; i < to_read; ++i) {
					int b = storage[cur + i];
					if (b < 0) return false;
					p[i] = (byte)b;
				}
			}

			p += to_read;
			cur += to_read;
			remaining -= to_read;
		}
		return true;
	}

	static uint32 ReadUint32LE(uni::StorageTrait& storage, uint64 offset) {
		uint32 val = 0;
		if (!StorageReadExact(storage, offset, &val, sizeof(val))) return 0;
		return val;
	}

	static uint16 ReadUint16LE(uni::StorageTrait& storage, uint64 offset) {
		uint16 val = 0;
		if (!StorageReadExact(storage, offset, &val, sizeof(val))) return 0;
		return val;
	}

	struct AVIChunkRef {
		uint32 offset;
		uint32 length;
		uint32 flags;
	};

	class AVIStream : public uni::IVideoStream {
	private:
		uni::StorageTrait*  storage;
		uni::VideoInfo      info;
		uni::trait::Malloc* allocator;

		// Video stream info
		uint32 v_stream_idx;
		uint32 v_fourcc;
		uint32 v_scale;
		uint32 v_rate;
		uint32 v_width;
		uint32 v_height;
		uint32 v_bits_per_pixel;

		// Audio stream info
		uint32 a_stream_idx;
		uint32 a_format_tag;
		uint32 a_channels;
		uint32 a_sample_rate;
		uint32 a_bits_per_sample;
		uint32 a_block_align;

		// Frame and chunk index table
		AVIChunkRef* v_frames;
		uint32       v_frame_count;
		uint32       v_frame_cap;
		uint32       curr_vframe;

		AVIChunkRef* a_chunks;
		uint32       a_chunk_count;
		uint32       a_chunk_cap;
		uint32       curr_achunk;
		uint32       curr_achunk_offset;

		bool AddVideoFrame(uint32 offset, uint32 length, uint32 flags) {
			if (v_frame_count >= v_frame_cap) {
				uint32 new_cap = v_frame_cap ? v_frame_cap * 2 : 128;
				AVIChunkRef* new_buf = (AVIChunkRef*)malloc(new_cap * sizeof(AVIChunkRef));
				if (!new_buf) return false;
				if (v_frames && v_frame_count > 0) {
					MemCopyN(new_buf, v_frames, v_frame_count * sizeof(AVIChunkRef));
					free(v_frames);
				}
				v_frames = new_buf;
				v_frame_cap = new_cap;
			}
			v_frames[v_frame_count].offset = offset;
			v_frames[v_frame_count].length = length;
			v_frames[v_frame_count].flags = flags;
			v_frame_count++;
			return true;
		}

		bool AddAudioChunk(uint32 offset, uint32 length, uint32 flags) {
			if (a_chunk_count >= a_chunk_cap) {
				uint32 new_cap = a_chunk_cap ? a_chunk_cap * 2 : 128;
				AVIChunkRef* new_buf = (AVIChunkRef*)malloc(new_cap * sizeof(AVIChunkRef));
				if (!new_buf) return false;
				if (a_chunks && a_chunk_count > 0) {
					MemCopyN(new_buf, a_chunks, a_chunk_count * sizeof(AVIChunkRef));
					free(a_chunks);
				}
				a_chunks = new_buf;
				a_chunk_cap = new_cap;
			}
			a_chunks[a_chunk_count].offset = offset;
			a_chunks[a_chunk_count].length = length;
			a_chunks[a_chunk_count].flags = flags;
			a_chunk_count++;
			return true;
		}

	public:
		AVIStream(uni::StorageTrait* st, uni::trait::Malloc* alloc)
			: storage(st), allocator(alloc),
			  v_stream_idx(0), v_fourcc(0), v_scale(1), v_rate(30),
			  v_width(0), v_height(0), v_bits_per_pixel(24),
			  a_stream_idx((uint32)-1), a_format_tag(0), a_channels(0),
			  a_sample_rate(0), a_bits_per_sample(0), a_block_align(0),
			  v_frames(nullptr), v_frame_count(0), v_frame_cap(0), curr_vframe(0),
			  a_chunks(nullptr), a_chunk_count(0), a_chunk_cap(0), curr_achunk(0),
			  curr_achunk_offset(0) {
		}

		virtual ~AVIStream() {
			if (v_frames) {
				free(v_frames);
				v_frames = nullptr;
			}
			if (a_chunks) {
				free(a_chunks);
				a_chunks = nullptr;
			}
		}

		virtual void Release() override {
			delete this;
		}

		bool Init() {
			if (!storage) return false;

			// Check RIFF header
			AVIRIFFHEADER riff{};
			if (!StorageReadExact(*storage, 0, &riff, sizeof(riff))) return false;
			if (riff.riff_tag != AVI_FOURCC_RIFF ||
				(riff.avi_tag != AVI_FOURCC_AVI && riff.avi_tag != AVI_FOURCC_AVIX)) {
				return false;
			}

			uint64 file_size = (uint64)riff.riff_size + 8;
			uint64 cur_pos = 12; // Skip 'RIFF' + size + 'AVI '
			uint64 movi_pos = 0;
			uint64 movi_size = 0;
			uint64 idx1_pos = 0;
			uint64 idx1_size = 0;

			uint32 stream_count = 0;
			uint32 cur_stream_type = 0;
			AVISTREAMHEADER cur_strh{};
			bool has_strh = false;

			// Parse top-level chunks and lists
			while (cur_pos + 8 <= file_size) {
				uint32 tag = ReadUint32LE(*storage, cur_pos);
				uint32 size = ReadUint32LE(*storage, cur_pos + 4);

				if (tag == AVI_FOURCC_LIST) {
					if (cur_pos + 12 > file_size) break;
					uint32 list_type = ReadUint32LE(*storage, cur_pos + 8);
					uint64 list_end = cur_pos + 8 + ((size + 1) & ~1ULL);

					if (list_type == AVI_FOURCC_HDRL) {
						// Traverse hdrl contents
						uint64 hdrl_pos = cur_pos + 12;
						while (hdrl_pos + 8 <= list_end) {
							uint32 h_tag = ReadUint32LE(*storage, hdrl_pos);
							uint32 h_size = ReadUint32LE(*storage, hdrl_pos + 4);
							uint64 h_next = hdrl_pos + 8 + ((h_size + 1) & ~1ULL);

							if (h_tag == AVI_FOURCC_AVIH && h_size >= sizeof(AVIMAINHEADER)) {
								AVIMAINHEADER avih{};
								StorageReadExact(*storage, hdrl_pos + 8, &avih, sizeof(avih));
								v_width = avih.dwWidth;
								v_height = avih.dwHeight;
								if (avih.dwMicroSecPerFrame > 0) {
									v_rate = 1000000;
									v_scale = avih.dwMicroSecPerFrame;
								}
							} else if (h_tag == AVI_FOURCC_LIST) {
								uint32 sub_list_type = ReadUint32LE(*storage, hdrl_pos + 8);
								if (sub_list_type == AVI_FOURCC_STRL) {
									// Stream list
									uint64 strl_pos = hdrl_pos + 12;
									has_strh = false;

									while (strl_pos + 8 <= h_next) {
										uint32 s_tag = ReadUint32LE(*storage, strl_pos);
										uint32 s_size = ReadUint32LE(*storage, strl_pos + 4);
										uint64 s_next = strl_pos + 8 + ((s_size + 1) & ~1ULL);

										if (s_tag == AVI_FOURCC_STRH && s_size >= sizeof(AVISTREAMHEADER)) {
											StorageReadExact(*storage, strl_pos + 8, &cur_strh, sizeof(cur_strh));
											cur_stream_type = cur_strh.fccType;
											has_strh = true;

											if (cur_stream_type == AVI_STREAM_TYPE_VIDS) {
												v_stream_idx = stream_count;
												v_fourcc = cur_strh.fccHandler;
												if (cur_strh.dwRate > 0 && cur_strh.dwScale > 0) {
													v_rate = cur_strh.dwRate;
													v_scale = cur_strh.dwScale;
												}
											} else if (cur_stream_type == AVI_STREAM_TYPE_AUDS) {
												a_stream_idx = stream_count;
											}
										} else if (s_tag == AVI_FOURCC_STRF && has_strh) {
											if (cur_stream_type == AVI_STREAM_TYPE_VIDS && s_size >= sizeof(BITMAPINFOHEADER)) {
												BITMAPINFOHEADER bmi{};
												StorageReadExact(*storage, strl_pos + 8, &bmi, sizeof(bmi));
												if (bmi.biWidth > 0) v_width = (uint32)bmi.biWidth;
												if (bmi.biHeight > 0) v_height = (uint32)bmi.biHeight;
												if (bmi.biBitCount > 0) v_bits_per_pixel = bmi.biBitCount;
												if (bmi.biCompression != 0) {
													v_fourcc = bmi.biCompression;
												}
											} else if (cur_stream_type == AVI_STREAM_TYPE_AUDS && s_size >= sizeof(WAVFMTPCM)) {
												WAVFORMATEX wfx{};
												uint32 read_len = s_size < sizeof(wfx) ? s_size : sizeof(wfx);
												StorageReadExact(*storage, strl_pos + 8, &wfx, read_len);
												a_format_tag = wfx.audio_format;
												a_channels = wfx.channel_count;
												a_sample_rate = wfx.sample_rate;
												a_bits_per_sample = wfx.bits_per_sample;
												a_block_align = wfx.block_align;
											}
										}
										strl_pos = s_next;
									}
									stream_count++;
								}
							}
							hdrl_pos = h_next;
						}
					} else if (list_type == AVI_FOURCC_MOVI) {
						movi_pos = cur_pos + 12;
						movi_size = (size > 4) ? (size - 4) : 0;
					}
					cur_pos = list_end;
				} else if (tag == AVI_FOURCC_IDX1) {
					idx1_pos = cur_pos + 8;
					idx1_size = size;
					cur_pos += 8 + ((size + 1) & ~1ULL);
				} else {
					cur_pos += 8 + ((size + 1) & ~1ULL);
				}
			}

			// Build indices from idx1 if available
			bool index_built = false;
			if (idx1_pos > 0 && idx1_size >= sizeof(AVIINDEXENTRY)) {
				uint32 entry_count = idx1_size / sizeof(AVIINDEXENTRY);
				AVIINDEXENTRY* entries = (AVIINDEXENTRY*)malloc(idx1_size);
				if (entries) {
					if (StorageReadExact(*storage, idx1_pos, entries, idx1_size)) {
						uint64 base_offset = 0;
						if (entry_count > 0 && entries[0].dwChunkOffset < movi_pos && movi_pos > 4) {
							base_offset = movi_pos - 4;
						}

						// Pre-allocate frame buffer capacity
						if (entry_count > v_frame_cap) {
							v_frames = (AVIChunkRef*)malloc(entry_count * sizeof(AVIChunkRef));
							if (v_frames) v_frame_cap = entry_count;
						}

						for (uint32 i = 0; i < entry_count; ++i) {
							const AVIINDEXENTRY& entry = entries[i];
							uint64 data_offset = base_offset + entry.dwChunkOffset + 8;

							uint8 stream_num = (uint8)(entry.ckid & 0xFF) - '0';
							uint8 stream_tens = (uint8)((entry.ckid >> 8) & 0xFF) - '0';
							uint32 str_idx = (stream_tens < 10 && stream_num < 10) ? (stream_tens * 10 + stream_num) : (uint8)(entry.ckid & 0xFF) - '0';
							uint16 type_code = (uint16)(entry.ckid >> 16);

							// 'dc' (0x6364) = compressed video, 'db' (0x6264) = uncompressed bitmap, 'wb' (0x6277) = wave audio
							if (type_code == 0x6364 || type_code == 0x6264 || str_idx == v_stream_idx) {
								AddVideoFrame((uint32)data_offset, entry.dwChunkLength, entry.dwFlags);
							} else if (type_code == 0x6277 || str_idx == a_stream_idx) {
								AddAudioChunk((uint32)data_offset, entry.dwChunkLength, entry.dwFlags);
							}
						}

						if (v_frame_count > 0) {
							index_built = true;
						}
					}
					free(entries);
				}
			}

			// Fallback: Scan movi chunk if idx1 was missing or empty
			if (!index_built && movi_pos > 0 && movi_size > 0) {
				uint64 m_pos = movi_pos;
				uint64 m_end = movi_pos + movi_size;

				while (m_pos + 8 <= m_end) {
					uint32 tag = ReadUint32LE(*storage, m_pos);
					uint32 size = ReadUint32LE(*storage, m_pos + 4);
					uint64 next_pos = m_pos + 8 + ((size + 1) & ~1ULL);

					if (tag == AVI_FOURCC_LIST) {
						// Sub-LIST within movi (e.g. 'rec ')
						m_pos += 12;
						continue;
					}

					uint8 s0 = (uint8)(tag & 0xFF);
					uint8 s1 = (uint8)((tag >> 8) & 0xFF);
					uint16 type_code = (uint16)(tag >> 16);

					if (s0 >= '0' && s0 <= '9' && s1 >= '0' && s1 <= '9') {
						uint32 str_idx = (s0 - '0') * 10 + (s1 - '0');
						if (type_code == 0x6364 || type_code == 0x6264 || str_idx == v_stream_idx) {
							AddVideoFrame((uint32)(m_pos + 8), size, AVIIF_KEYFRAME);
						} else if (type_code == 0x6277 || str_idx == a_stream_idx) {
							AddAudioChunk((uint32)(m_pos + 8), size, 0);
						}
					}
					m_pos = next_pos;
				}
			}

			// Populate VideoInfo
			info.containerFormat = uni::VideoContainerFormat::AVI;
			info.videoFormat.width = v_width;
			info.videoFormat.height = v_height;
			info.videoFormat.frame_rate_num = v_rate ? v_rate : 30;
			info.videoFormat.frame_rate_den = v_scale ? v_scale : 1;
			info.totalFrames = v_frame_count;

			if (v_fourcc == AVI_FOURCC_MJPG || v_fourcc == AVI_FOURCC_mjpg ||
				v_fourcc == AVI_FOURCC_JPEG || v_fourcc == AVI_FOURCC_jpeg) {
				info.videoFormat.codec_type = uni::VideoCodecType::MJPEG;
			} else if (v_fourcc == AVI_FOURCC_DIB || v_fourcc == AVI_FOURCC_RGB ||
					   v_fourcc == AVI_FOURCC_RAW || v_fourcc == 0) {
				info.videoFormat.codec_type = uni::VideoCodecType::RGB;
			} else if (v_fourcc == AVI_FOURCC_MPG1) {
				info.videoFormat.codec_type = uni::VideoCodecType::MPEG1;
			} else if (v_fourcc == AVI_FOURCC_MPG2) {
				info.videoFormat.codec_type = uni::VideoCodecType::MPEG2;
			} else if (v_fourcc == AVI_FOURCC_H264 || v_fourcc == AVI_FOURCC_h264) {
				info.videoFormat.codec_type = uni::VideoCodecType::H264;
			} else {
				info.videoFormat.codec_type = uni::VideoCodecType::MJPEG; // default guess
			}

			if (v_rate > 0 && v_scale > 0 && v_frame_count > 0) {
				info.durationMs = (uint32)(((uint64)v_frame_count * 1000ULL * v_scale) / v_rate);
			}

			if (a_channels > 0 && a_sample_rate > 0) {
				info.hasAudio = true;
				info.audioInfo.format.channels = (uint16)a_channels;
				info.audioInfo.format.sample_rate = a_sample_rate;
				info.audioInfo.format.sample_format = (a_bits_per_sample == 8) ? uni::AudioSampleFormat::U8 : uni::AudioSampleFormat::S16LE;
				info.audioInfo.bitsPerSample = a_bits_per_sample ? a_bits_per_sample : 16;
				info.audioInfo.containerFormat = uni::AudioContainerFormat::WAV;
				info.audioInfo.durationMs = info.durationMs;
			}

			return v_frame_count > 0;
		}

		virtual uni::VideoResult GetInfo(uni::VideoInfo& outInfo) const override {
			outInfo = info;
			return uni::VideoResult::OK;
		}

		virtual uni::VideoResult ReadVideoFrame(uni::VideoFrame& outFrame, uni::trait::Malloc& alloc) override {
			if (curr_vframe >= v_frame_count) {
				return uni::VideoResult::EndOfStream;
			}

			const AVIChunkRef& chunk = v_frames[curr_vframe];
			if (chunk.length == 0) {
				curr_vframe++;
				return uni::VideoResult::Failed;
			}

			byte* chunk_data = (byte*)malloc(chunk.length);
			if (!chunk_data) {
				return uni::VideoResult::OutOfMemory;
			}

			if (!StorageReadExact(*storage, chunk.offset, chunk_data, chunk.length)) {
				free(chunk_data);
				return uni::VideoResult::IoError;
			}

			int out_w = 0;
			int out_h = 0;
			uni::Color* pixels = nullptr;

			if (info.videoFormat.codec_type == uni::VideoCodecType::MJPEG) {
				pixels = DecodeJPEG(chunk_data, chunk.length, &out_w, &out_h);
			} else if (info.videoFormat.codec_type == uni::VideoCodecType::RGB) {
				// Raw BMP/DIB frame
				out_w = (int)v_width;
				out_h = (int)v_height;
				if (out_w > 0 && out_h > 0) {
					size_t pixel_cnt = (size_t)out_w * (size_t)out_h;
					pixels = (uni::Color*)malloc(pixel_cnt * sizeof(uni::Color));
					if (pixels) {
						uint32 bpp = v_bits_per_pixel ? v_bits_per_pixel : 24;
						uint32 row_bytes = ((out_w * (bpp / 8) + 3) & ~3u);
						for (int y = 0; y < out_h; ++y) {
							// DIB is bottom-up
							const byte* src_row = chunk_data + (size_t)(out_h - 1 - y) * row_bytes;
							uni::Color* dst_row = pixels + (size_t)y * out_w;
							if (bpp == 24) {
								for (int x = 0; x < out_w; ++x) {
									dst_row[x].b = src_row[x * 3 + 0];
									dst_row[x].g = src_row[x * 3 + 1];
									dst_row[x].r = src_row[x * 3 + 2];
									dst_row[x].a = 0xFF;
								}
							} else if (bpp == 32) {
								for (int x = 0; x < out_w; ++x) {
									dst_row[x].b = src_row[x * 4 + 0];
									dst_row[x].g = src_row[x * 4 + 1];
									dst_row[x].r = src_row[x * 4 + 2];
									dst_row[x].a = src_row[x * 4 + 3];
								}
							}
						}
					}
				}
			}

			free(chunk_data);

			if (!pixels) {
				curr_vframe++;
				return uni::VideoResult::Failed;
			}

			uni::VideoFrameClear(outFrame);
			outFrame.width = (uint32)out_w;
			outFrame.height = (uint32)out_h;
			outFrame.timestampMs = (uint32)(((uint64)curr_vframe * 1000ULL * (v_scale ? v_scale : 1)) / (v_rate ? v_rate : 30));
			outFrame.frameIndex = curr_vframe;
			outFrame.isKeyFrame = (chunk.flags & AVIIF_KEYFRAME) != 0;

			outFrame.image.width = (uint32)out_w;
			outFrame.image.height = (uint32)out_h;
			outFrame.image.stride = (uint32)out_w * sizeof(uni::Color);
			outFrame.image.format = uni::PixelFormat::BGRA8888;
			outFrame.image.colorSpace = uni::ColorSpace::SRGB;
			outFrame.image.pixels = pixels;
			outFrame.image.size = (size_t)out_w * (size_t)out_h * sizeof(uni::Color);
			outFrame.image.allocator = nullptr; // managed / freed via free() or VideoFrameFree

			curr_vframe++;
			return uni::VideoResult::OK;
		}

		virtual uni::VideoResult ReadAudioSamples(void* destBuffer, uint32 maxBytes, uint32& bytesRead) override {
			bytesRead = 0;
			if (!destBuffer || maxBytes == 0 || !info.hasAudio) {
				return uni::VideoResult::InvalidArgument;
			}

			byte* out_ptr = (byte*)destBuffer;
			while (maxBytes > 0 && curr_achunk < a_chunk_count) {
				const AVIChunkRef& chunk = a_chunks[curr_achunk];
				uint32 avail = (chunk.length > curr_achunk_offset) ? (chunk.length - curr_achunk_offset) : 0;

				if (avail == 0) {
					curr_achunk++;
					curr_achunk_offset = 0;
					continue;
				}

				uint32 to_read = (maxBytes < avail) ? maxBytes : avail;
				if (!StorageReadExact(*storage, chunk.offset + curr_achunk_offset, out_ptr, to_read)) {
					return bytesRead > 0 ? uni::VideoResult::OK : uni::VideoResult::IoError;
				}

				out_ptr += to_read;
				bytesRead += to_read;
				maxBytes -= to_read;
				curr_achunk_offset += to_read;

				if (curr_achunk_offset >= chunk.length) {
					curr_achunk++;
					curr_achunk_offset = 0;
				}
			}

			if (bytesRead == 0 && curr_achunk >= a_chunk_count) {
				return uni::VideoResult::EndOfStream;
			}
			return uni::VideoResult::OK;
		}

		virtual uni::VideoResult SeekFrame(uint32 frameIndex) override {
			if (frameIndex >= v_frame_count) {
				curr_vframe = v_frame_count;
				return uni::VideoResult::EndOfStream;
			}
			curr_vframe = frameIndex;

			// Approximate audio position
			if (info.hasAudio && v_frame_count > 0 && a_chunk_count > 0) {
				curr_achunk = (uint32)(((uint64)frameIndex * a_chunk_count) / v_frame_count);
				curr_achunk_offset = 0;
			}
			return uni::VideoResult::OK;
		}

		virtual uni::VideoResult SeekTime(uint32 timestampMs) override {
			if (info.durationMs == 0 || v_frame_count == 0) return uni::VideoResult::Failed;
			uint32 target_frame = (uint32)(((uint64)timestampMs * (v_rate ? v_rate : 30)) / ((v_scale ? v_scale : 1) * 1000ULL));
			return SeekFrame(target_frame);
		}
	};

} // namespace

namespace uni {

	const char* AVICodec::GetName() const {
		return "AVI";
	}

	VideoContainerFormat AVICodec::GetFormat() const {
		return VideoContainerFormat::AVI;
	}

	const char* const* AVICodec::GetExtensions() const {
		return AVI_EXTENSIONS;
	}

	VideoResult AVICodec::Probe(StorageTrait& storage, bool& matched) const {
		matched = false;
		AVIRIFFHEADER riff{};
		if (!StorageReadExact(storage, 0, &riff, sizeof(riff))) {
			return VideoResult::IoError;
		}
		if (riff.riff_tag == AVI_FOURCC_RIFF &&
			(riff.avi_tag == AVI_FOURCC_AVI || riff.avi_tag == AVI_FOURCC_AVIX)) {
			matched = true;
			return VideoResult::OK;
		}
		return VideoResult::OK;
	}

	VideoResult AVICodec::ReadInfo(StorageTrait& storage, VideoInfo& outInfo) const {
		AVIStream stream(&storage, nullptr);
		if (!stream.Init()) {
			return VideoResult::InvalidFormat;
		}
		return stream.GetInfo(outInfo);
	}

	VideoResult AVICodec::OpenStream(
		StorageTrait& storage,
		IVideoStream*& outStream,
		trait::Malloc& allocator
	) const {
		outStream = nullptr;
		AVIStream* s = new AVIStream(&storage, &allocator);
		if (!s) return VideoResult::OutOfMemory;
		if (!s->Init()) {
			delete s;
			return VideoResult::InvalidFormat;
		}
		outStream = s;
		return VideoResult::OK;
	}

} // namespace uni

#endif // _INC_CPP
