// ASCII C/C++ TAB4 CRLF
// Docutitle: RIFF WAVE Codec Implementation
// Attribute: Env-Freestanding Non-Dependence
// Copyright: UNISYM

#include "../../../../inc/c/format/audio/WAV.h"
#include "../../../../inc/c/ustring.h"
#include <stdlib.h>

#if defined(_INC_CPP)

namespace {

	static const int16 MS_ADPCM_DEFAULT_COEF[7][2] = {
		{ 256, 0 },
		{ 512, -256 },
		{ 0, 0 },
		{ 192, 64 },
		{ 240, 0 },
		{ 460, -208 },
		{ 392, -232 }
	};

	static const int16 MS_ADPCM_ADAPT_TABLE[16] = {
		230, 230, 230, 230, 307, 409, 512, 614,
		768, 614, 512, 409, 307, 230, 230, 230
	};

	static const int8 IMA_INDEX_TABLE[16] = {
		-1, -1, -1, -1, 2, 4, 6, 8,
		-1, -1, -1, -1, 2, 4, 6, 8
	};

	static const int16 IMA_STEP_TABLE[89] = {
		7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
		19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
		50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
		130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
		337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
		876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
		2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
		5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
		15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
	};

	static inline int16 DecodeAlawSample(uint8 a_val) {
		a_val ^= 0x55;
		int32 t = (a_val & 0x0F) << 4;
		uint8 seg = (a_val & 0x70) >> 4;
		switch (seg) {
		case 0:
			t += 8;
			break;
		case 1:
			t += 0x108;
			break;
		default:
			t += 0x108;
			t <<= (seg - 1);
			break;
		}
		return (a_val & 0x80) ? (int16)t : (int16)(-t);
	}

	static inline int16 DecodeMulawSample(uint8 u_val) {
		u_val = ~u_val;
		int32 t = ((u_val & 0x0F) << 3) + 0x84;
		t <<= ((u_val & 0x70) >> 4);
		t -= 0x84;
		return (u_val & 0x80) ? (int16)(-t) : (int16)t;
	}

	struct WAVFormatDetails {
		uint16 audio_format;
		uint16 channels;
		uint32 sample_rate;
		uint32 byte_rate;
		uint16 block_align;
		uint16 bits_per_sample;
		uint16 samples_per_block;
		uint16 num_coef;
		int16 coef[32][2];
		uint32 data_offset;
		uint32 data_size;
	};

	class WAVStream : public uni::IAudioStream {
	private:
		uni::StorageTrait* storage;
		uni::AudioInfo     info;
		WAVFormatDetails   details;
		uint32             dataByteOffset;
		uint32             dataByteLength;
		uint32             currentByteOffset;
		uni::trait::Malloc* allocator;
		byte*              sector_buf;
		stduint            cached_block;

		// ADPCM buffer state
		byte*              adpcm_raw_buf;
		int16*             adpcm_samples_buf;
		uint32             adpcm_sample_cap;
		uint32             adpcm_sample_pos;
		uint32             adpcm_samples_valid;

		uint32 ReadRawBytes(uint32 file_offset, byte* dest, uint32 count) {
			if (!dest || !count) return 0;
			if (file_offset >= dataByteOffset + dataByteLength) return 0;
			uint32 remain = (dataByteOffset + dataByteLength) - file_offset;
			uint32 to_read = count > remain ? remain : count;
			stduint block_size = storage->Block_Size ? storage->Block_Size : 512;
			uint32 done = 0;

			while (done < to_read) {
				stduint cur_offset = file_offset + done;
				stduint block_idx = cur_offset / block_size;
				stduint block_off = cur_offset % block_size;

				if (block_off == 0 && (to_read - done) >= block_size) {
					stduint num_blocks = (to_read - done) / block_size;
					if (!storage->Read(block_idx, dest + done, num_blocks)) {
						break;
					}
					done += (uint32)(num_blocks * block_size);
					continue;
				}

				if (cached_block != block_idx) {
					if (!sector_buf) break;
					if (!storage->Read(block_idx, sector_buf, 1)) {
						break;
					}
					cached_block = block_idx;
				}

				stduint take = block_size - block_off;
				if (take > (to_read - done)) take = to_read - done;
				MemCopyN(dest + done, sector_buf + block_off, take);
				done += (uint32)take;
			}
			return done;
		}

		void DecodeMsAdpcmBlock(const byte* raw_block, uint32 raw_bytes, int16* out_samples, uint32 max_samples, uint32& decoded_samples) {
			decoded_samples = 0;
			if (!raw_block || !out_samples || raw_bytes < details.block_align) return;

			if (details.channels == 1) {
				if (raw_bytes < 7) return;
				uint8 bPredictor = raw_block[0];
				int16 iDelta = (int16)(raw_block[1] | (raw_block[2] << 8));
				int16 iSamp1 = (int16)(raw_block[3] | (raw_block[4] << 8));
				int16 iSamp2 = (int16)(raw_block[5] | (raw_block[6] << 8));
				if (bPredictor >= details.num_coef) bPredictor = 0;
				int32 coef1 = details.coef[bPredictor][0];
				int32 coef2 = details.coef[bPredictor][1];

				if (max_samples >= 1) out_samples[decoded_samples++] = iSamp2;
				if (max_samples >= 2) out_samples[decoded_samples++] = iSamp1;

				for (uint32 i = 7; i < raw_bytes && decoded_samples < max_samples; ++i) {
					uint8 b = raw_block[i];
					uint8 nibbles[2] = { uint8((b >> 4) & 0x0F), uint8(b & 0x0F) };
					for (int n = 0; n < 2 && decoded_samples < max_samples; ++n) {
						uint8 raw_nib = nibbles[n];
						int8 nib = (raw_nib & 0x08) ? (int8)(raw_nib - 16) : (int8)raw_nib;
						int32 pred = ((int32)iSamp1 * coef1 + (int32)iSamp2 * coef2) / 256;
						pred += (int32)nib * iDelta;
						if (pred > 32767) pred = 32767;
						else if (pred < -32768) pred = -32768;
						iSamp2 = iSamp1;
						iSamp1 = (int16)pred;
						out_samples[decoded_samples++] = (int16)pred;
						iDelta = (int16)(((int32)iDelta * MS_ADPCM_ADAPT_TABLE[raw_nib]) / 256);
						if (iDelta < 16) iDelta = 16;
					}
				}
			} else if (details.channels == 2) {
				if (raw_bytes < 14) return;
				uint8 bPred_L = raw_block[0];
				uint8 bPred_R = raw_block[1];
				int16 iDelta_L = (int16)(raw_block[2] | (raw_block[3] << 8));
				int16 iDelta_R = (int16)(raw_block[4] | (raw_block[5] << 8));
				int16 iSamp1_L = (int16)(raw_block[6] | (raw_block[7] << 8));
				int16 iSamp1_R = (int16)(raw_block[8] | (raw_block[9] << 8));
				int16 iSamp2_L = (int16)(raw_block[10] | (raw_block[11] << 8));
				int16 iSamp2_R = (int16)(raw_block[12] | (raw_block[13] << 8));

				if (bPred_L >= details.num_coef) bPred_L = 0;
				if (bPred_R >= details.num_coef) bPred_R = 0;
				int32 coef1_L = details.coef[bPred_L][0]; int32 coef2_L = details.coef[bPred_L][1];
				int32 coef1_R = details.coef[bPred_R][0]; int32 coef2_R = details.coef[bPred_R][1];

				if (decoded_samples + 1 < max_samples) {
					out_samples[decoded_samples++] = iSamp2_L;
					out_samples[decoded_samples++] = iSamp2_R;
				}
				if (decoded_samples + 1 < max_samples) {
					out_samples[decoded_samples++] = iSamp1_L;
					out_samples[decoded_samples++] = iSamp1_R;
				}

				for (uint32 i = 14; i < raw_bytes && decoded_samples + 1 < max_samples; ++i) {
					uint8 b = raw_block[i];
					uint8 raw_L = (b >> 4) & 0x0F;
					uint8 raw_R = b & 0x0F;

					int8 nib_L = (raw_L & 0x08) ? (int8)(raw_L - 16) : (int8)raw_L;
					int32 pred_L = ((int32)iSamp1_L * coef1_L + (int32)iSamp2_L * coef2_L) / 256;
					pred_L += (int32)nib_L * iDelta_L;
					if (pred_L > 32767) pred_L = 32767;
					else if (pred_L < -32768) pred_L = -32768;
					iSamp2_L = iSamp1_L;
					iSamp1_L = (int16)pred_L;
					out_samples[decoded_samples++] = (int16)pred_L;
					iDelta_L = (int16)(((int32)iDelta_L * MS_ADPCM_ADAPT_TABLE[raw_L]) / 256);
					if (iDelta_L < 16) iDelta_L = 16;

					int8 nib_R = (raw_R & 0x08) ? (int8)(raw_R - 16) : (int8)raw_R;
					int32 pred_R = ((int32)iSamp1_R * coef1_R + (int32)iSamp2_R * coef2_R) / 256;
					pred_R += (int32)nib_R * iDelta_R;
					if (pred_R > 32767) pred_R = 32767;
					else if (pred_R < -32768) pred_R = -32768;
					iSamp2_R = iSamp1_R;
					iSamp1_R = (int16)pred_R;
					out_samples[decoded_samples++] = (int16)pred_R;
					iDelta_R = (int16)(((int32)iDelta_R * MS_ADPCM_ADAPT_TABLE[raw_R]) / 256);
					if (iDelta_R < 16) iDelta_R = 16;
				}
			}
		}

		void DecodeImaAdpcmBlock(const byte* raw_block, uint32 raw_bytes, int16* out_samples, uint32 max_samples, uint32& decoded_samples) {
			decoded_samples = 0;
			if (!raw_block || !out_samples || raw_bytes < details.block_align) return;

			if (details.channels == 1) {
				if (raw_bytes < 4) return;
				int16 predictor = (int16)(raw_block[0] | (raw_block[1] << 8));
				int8 index = (int8)raw_block[2];
				if (index < 0) index = 0; else if (index > 88) index = 88;

				if (max_samples >= 1) out_samples[decoded_samples++] = predictor;

				for (uint32 i = 4; i < raw_bytes && decoded_samples < max_samples; ++i) {
					uint8 b = raw_block[i];
					uint8 nibbles[2] = { uint8(b & 0x0F), uint8((b >> 4) & 0x0F) };
					for (int n = 0; n < 2 && decoded_samples < max_samples; ++n) {
						uint8 nib = nibbles[n];
						int16 step = IMA_STEP_TABLE[index];
						int32 diff = step >> 3;
						if (nib & 4) diff += step;
						if (nib & 2) diff += step >> 1;
						if (nib & 1) diff += step >> 2;
						if (nib & 8) predictor -= diff; else predictor += diff;
						if (predictor > 32767) predictor = 32767;
						else if (predictor < -32768) predictor = -32768;
						index += IMA_INDEX_TABLE[nib];
						if (index < 0) index = 0; else if (index > 88) index = 88;
						out_samples[decoded_samples++] = predictor;
					}
				}
			} else if (details.channels == 2) {
				if (raw_bytes < 8) return;
				int16 pred_L = (int16)(raw_block[0] | (raw_block[1] << 8));
				int8 idx_L = (int8)raw_block[2];
				if (idx_L < 0) idx_L = 0; else if (idx_L > 88) idx_L = 88;

				int16 pred_R = (int16)(raw_block[4] | (raw_block[5] << 8));
				int8 idx_R = (int8)raw_block[6];
				if (idx_R < 0) idx_R = 0; else if (idx_R > 88) idx_R = 88;

				if (decoded_samples + 1 < max_samples) {
					out_samples[decoded_samples++] = pred_L;
					out_samples[decoded_samples++] = pred_R;
				}

				const uint32 block_rem = (raw_bytes > 8) ? (raw_bytes - 8) : 0;
				const uint32 num_chunks = block_rem / 8;
				const byte* ptr = raw_block + 8;
				int16 l_samp[8];
				int16 r_samp[8];

				for (uint32 c = 0; c < num_chunks && decoded_samples + 15 < max_samples; ++c) {
					// Left 4 bytes (8 samples)
					for (int b = 0; b < 4; ++b) {
						uint8 byte_val = ptr[b];
						uint8 nib0 = byte_val & 0x0F;
						int16 step0 = IMA_STEP_TABLE[idx_L];
						int32 diff0 = step0 >> 3;
						if (nib0 & 4) diff0 += step0;
						if (nib0 & 2) diff0 += step0 >> 1;
						if (nib0 & 1) diff0 += step0 >> 2;
						if (nib0 & 8) pred_L -= diff0; else pred_L += diff0;
						if (pred_L > 32767) pred_L = 32767; else if (pred_L < -32768) pred_L = -32768;
						idx_L += IMA_INDEX_TABLE[nib0];
						if (idx_L < 0) idx_L = 0; else if (idx_L > 88) idx_L = 88;
						l_samp[b * 2] = pred_L;

						uint8 nib1 = (byte_val >> 4) & 0x0F;
						int16 step1 = IMA_STEP_TABLE[idx_L];
						int32 diff1 = step1 >> 3;
						if (nib1 & 4) diff1 += step1;
						if (nib1 & 2) diff1 += step1 >> 1;
						if (nib1 & 1) diff1 += step1 >> 2;
						if (nib1 & 8) pred_L -= diff1; else pred_L += diff1;
						if (pred_L > 32767) pred_L = 32767; else if (pred_L < -32768) pred_L = -32768;
						idx_L += IMA_INDEX_TABLE[nib1];
						if (idx_L < 0) idx_L = 0; else if (idx_L > 88) idx_L = 88;
						l_samp[b * 2 + 1] = pred_L;
					}

					// Right 4 bytes (8 samples)
					for (int b = 0; b < 4; ++b) {
						uint8 byte_val = ptr[4 + b];
						uint8 nib0 = byte_val & 0x0F;
						int16 step0 = IMA_STEP_TABLE[idx_R];
						int32 diff0 = step0 >> 3;
						if (nib0 & 4) diff0 += step0;
						if (nib0 & 2) diff0 += step0 >> 1;
						if (nib0 & 1) diff0 += step0 >> 2;
						if (nib0 & 8) pred_R -= diff0; else pred_R += diff0;
						if (pred_R > 32767) pred_R = 32767; else if (pred_R < -32768) pred_R = -32768;
						idx_R += IMA_INDEX_TABLE[nib0];
						if (idx_R < 0) idx_R = 0; else if (idx_R > 88) idx_R = 88;
						r_samp[b * 2] = pred_R;

						uint8 nib1 = (byte_val >> 4) & 0x0F;
						int16 step1 = IMA_STEP_TABLE[idx_R];
						int32 diff1 = step1 >> 3;
						if (nib1 & 4) diff1 += step1;
						if (nib1 & 2) diff1 += step1 >> 1;
						if (nib1 & 1) diff1 += step1 >> 2;
						if (nib1 & 8) pred_R -= diff1; else pred_R += diff1;
						if (pred_R > 32767) pred_R = 32767; else if (pred_R < -32768) pred_R = -32768;
						idx_R += IMA_INDEX_TABLE[nib1];
						if (idx_R < 0) idx_R = 0; else if (idx_R > 88) idx_R = 88;
						r_samp[b * 2 + 1] = pred_R;
					}

					for (int s = 0; s < 8; ++s) {
						if (decoded_samples + 1 < max_samples) {
							out_samples[decoded_samples++] = l_samp[s];
							out_samples[decoded_samples++] = r_samp[s];
						}
					}
					ptr += 8;
				}
			}
		}

	public:
		WAVStream(uni::StorageTrait& stg, const uni::AudioInfo& inf, const WAVFormatDetails& fmt_det, uni::trait::Malloc& alloc)
			: storage(&stg), info(inf), details(fmt_det), dataByteOffset(inf.dataByteOffset),
			  dataByteLength(inf.dataByteLength), currentByteOffset(inf.dataByteOffset),
			  allocator(&alloc), sector_buf(nullptr), cached_block((stduint)~0),
			  adpcm_raw_buf(nullptr), adpcm_samples_buf(nullptr), adpcm_sample_cap(0),
			  adpcm_sample_pos(0), adpcm_samples_valid(0) {
			stduint block_size = storage->Block_Size ? storage->Block_Size : 512;
			sector_buf = (byte*)allocator->allocate(block_size, 3);

			if (details.audio_format == WAV_FORMAT_MS_ADPCM || details.audio_format == WAV_FORMAT_IMA_ADPCM) {
				uint32 blk_align = details.block_align ? details.block_align : 512;
				adpcm_raw_buf = (byte*)allocator->allocate(blk_align, 3);
				uint32 max_smp = details.samples_per_block ? details.samples_per_block : (blk_align * 2);
				adpcm_sample_cap = max_smp * (details.channels ? details.channels : 1);
				if (adpcm_sample_cap < 512) adpcm_sample_cap = 512;
				adpcm_samples_buf = (int16*)allocator->allocate(adpcm_sample_cap * sizeof(int16), 3);
			}
		}

		virtual ~WAVStream() {
			if (sector_buf && allocator) {
				allocator->deallocate(sector_buf);
				sector_buf = nullptr;
			}
			if (adpcm_raw_buf && allocator) {
				allocator->deallocate(adpcm_raw_buf);
				adpcm_raw_buf = nullptr;
			}
			if (adpcm_samples_buf && allocator) {
				allocator->deallocate(adpcm_samples_buf);
				adpcm_samples_buf = nullptr;
			}
		}

		bool IsValid() const {
			if (!sector_buf) return false;
			if ((details.audio_format == WAV_FORMAT_MS_ADPCM || details.audio_format == WAV_FORMAT_IMA_ADPCM) &&
				(!adpcm_raw_buf || !adpcm_samples_buf)) {
				return false;
			}
			return true;
		}

		virtual void Release() override {
			uni::trait::Malloc* alloc = allocator;
			this->~WAVStream();
			if (alloc) {
				alloc->deallocate(this);
			}
		}

		virtual uni::AudioResult GetInfo(uni::AudioInfo& outInfo) const override {
			outInfo = info;
			return uni::AudioResult::OK;
		}

		virtual uni::AudioResult ReadSamples(void* destBuffer, uint32 maxBytes, uint32& bytesRead) override {
			bytesRead = 0;
			if (!destBuffer || !maxBytes) return uni::AudioResult::InvalidArgument;
			if (!storage) return uni::AudioResult::IoError;
			if (currentByteOffset >= dataByteOffset + dataByteLength &&
				adpcm_sample_pos >= adpcm_samples_valid) {
				return uni::AudioResult::EndOfStream;
			}

			// 1. Uncompressed 8-bit / 16-bit PCM: direct sector read
			if (details.audio_format == WAV_FORMAT_PCM && (details.bits_per_sample == 8 || details.bits_per_sample == 16)) {
				uint32 done = ReadRawBytes(currentByteOffset, (byte*)destBuffer, maxBytes);
				currentByteOffset += done;
				bytesRead = done;
				if (done == 0 && currentByteOffset >= dataByteOffset + dataByteLength) {
					return uni::AudioResult::EndOfStream;
				}
				return uni::AudioResult::OK;
			}

			// 2. G.711 A-law Decode (8-bit -> 16-bit PCM)
			if (details.audio_format == WAV_FORMAT_ALAW) {
				byte raw_chunk[256];
				int16* out_ptr = (int16*)destBuffer;
				uint32 max_frames = maxBytes / (sizeof(int16) * details.channels);
				uint32 frames_done = 0;

				while (frames_done < max_frames && currentByteOffset < dataByteOffset + dataByteLength) {
					uint32 needed_bytes = (max_frames - frames_done) * details.channels;
					if (needed_bytes > sizeof(raw_chunk)) needed_bytes = sizeof(raw_chunk);
					uint32 read_bytes = ReadRawBytes(currentByteOffset, raw_chunk, needed_bytes);
					if (read_bytes == 0) break;
					currentByteOffset += read_bytes;

					for (uint32 i = 0; i < read_bytes; ++i) {
						*out_ptr++ = DecodeAlawSample(raw_chunk[i]);
					}
					frames_done += read_bytes / details.channels;
				}
				bytesRead = frames_done * details.channels * sizeof(int16);
				if (bytesRead == 0 && currentByteOffset >= dataByteOffset + dataByteLength) {
					return uni::AudioResult::EndOfStream;
				}
				return uni::AudioResult::OK;
			}

			// 3. G.711 mu-law Decode (8-bit -> 16-bit PCM)
			if (details.audio_format == WAV_FORMAT_MULAW) {
				byte raw_chunk[256];
				int16* out_ptr = (int16*)destBuffer;
				uint32 max_frames = maxBytes / (sizeof(int16) * details.channels);
				uint32 frames_done = 0;

				while (frames_done < max_frames && currentByteOffset < dataByteOffset + dataByteLength) {
					uint32 needed_bytes = (max_frames - frames_done) * details.channels;
					if (needed_bytes > sizeof(raw_chunk)) needed_bytes = sizeof(raw_chunk);
					uint32 read_bytes = ReadRawBytes(currentByteOffset, raw_chunk, needed_bytes);
					if (read_bytes == 0) break;
					currentByteOffset += read_bytes;

					for (uint32 i = 0; i < read_bytes; ++i) {
						*out_ptr++ = DecodeMulawSample(raw_chunk[i]);
					}
					frames_done += read_bytes / details.channels;
				}
				bytesRead = frames_done * details.channels * sizeof(int16);
				if (bytesRead == 0 && currentByteOffset >= dataByteOffset + dataByteLength) {
					return uni::AudioResult::EndOfStream;
				}
				return uni::AudioResult::OK;
			}

			// 4. IEEE 754 32-bit Float Decode (float -> 16-bit PCM)
			if (details.audio_format == WAV_FORMAT_IEEE_FLOAT && details.bits_per_sample == 32) {
				float raw_chunk[64];
				int16* out_ptr = (int16*)destBuffer;
				uint32 max_samples = maxBytes / sizeof(int16);
				uint32 samples_done = 0;

				while (samples_done < max_samples && currentByteOffset < dataByteOffset + dataByteLength) {
					uint32 needed_samples = max_samples - samples_done;
					if (needed_samples > sizeof(raw_chunk) / sizeof(float)) {
						needed_samples = sizeof(raw_chunk) / sizeof(float);
					}
					uint32 read_bytes = ReadRawBytes(currentByteOffset, (byte*)raw_chunk, needed_samples * sizeof(float));
					if (read_bytes < sizeof(float)) break;
					uint32 num_floats = read_bytes / sizeof(float);
					currentByteOffset += num_floats * sizeof(float);

					for (uint32 i = 0; i < num_floats; ++i) {
						float f = raw_chunk[i];
						if (f > 1.0f) f = 1.0f;
						else if (f < -1.0f) f = -1.0f;
						*out_ptr++ = (int16)(f * 32767.0f);
					}
					samples_done += num_floats;
				}
				bytesRead = samples_done * sizeof(int16);
				if (bytesRead == 0 && currentByteOffset >= dataByteOffset + dataByteLength) {
					return uni::AudioResult::EndOfStream;
				}
				return uni::AudioResult::OK;
			}

			// 5. 24-bit / 32-bit Integer PCM Decode (-> 16-bit PCM)
			if (details.audio_format == WAV_FORMAT_PCM && (details.bits_per_sample == 24 || details.bits_per_sample == 32)) {
				byte raw_chunk[256];
				int16* out_ptr = (int16*)destBuffer;
				uint32 max_samples = maxBytes / sizeof(int16);
				uint32 samples_done = 0;
				uint32 bytes_per_sample = details.bits_per_sample / 8;

				while (samples_done < max_samples && currentByteOffset < dataByteOffset + dataByteLength) {
					uint32 needed_samples = max_samples - samples_done;
					uint32 needed_bytes = needed_samples * bytes_per_sample;
					if (needed_bytes > sizeof(raw_chunk)) needed_bytes = sizeof(raw_chunk) - (sizeof(raw_chunk) % bytes_per_sample);
					uint32 read_bytes = ReadRawBytes(currentByteOffset, raw_chunk, needed_bytes);
					if (read_bytes < bytes_per_sample) break;
					uint32 num_smp = read_bytes / bytes_per_sample;
					currentByteOffset += num_smp * bytes_per_sample;

					for (uint32 i = 0; i < num_smp; ++i) {
						const byte* p = raw_chunk + i * bytes_per_sample;
						if (details.bits_per_sample == 24) {
							*out_ptr++ = (int16)(p[1] | (p[2] << 8));
						} else {
							*out_ptr++ = (int16)(p[2] | (p[3] << 8));
						}
					}
					samples_done += num_smp;
				}
				bytesRead = samples_done * sizeof(int16);
				if (bytesRead == 0 && currentByteOffset >= dataByteOffset + dataByteLength) {
					return uni::AudioResult::EndOfStream;
				}
				return uni::AudioResult::OK;
			}

			// 6. MS-ADPCM / IMA-ADPCM Block Decode
			if (details.audio_format == WAV_FORMAT_MS_ADPCM || details.audio_format == WAV_FORMAT_IMA_ADPCM) {
				int16* out_ptr = (int16*)destBuffer;
				uint32 max_samples = maxBytes / sizeof(int16);
				uint32 samples_done = 0;

				while (samples_done < max_samples) {
					// Flush buffered samples first
					if (adpcm_sample_pos < adpcm_samples_valid) {
						uint32 avail = adpcm_samples_valid - adpcm_sample_pos;
						uint32 take = (max_samples - samples_done < avail) ? (max_samples - samples_done) : avail;
						MemCopyN(out_ptr + samples_done, adpcm_samples_buf + adpcm_sample_pos, take * sizeof(int16));
						adpcm_sample_pos += take;
						samples_done += take;
						if (samples_done >= max_samples) break;
					}

					// Read and decode next block
					if (currentByteOffset >= dataByteOffset + dataByteLength) break;
					uint32 blk_align = details.block_align ? details.block_align : 512;
					uint32 raw_read = ReadRawBytes(currentByteOffset, adpcm_raw_buf, blk_align);
					if (raw_read < blk_align && raw_read == 0) break;
					currentByteOffset += raw_read;

					adpcm_sample_pos = 0;
					adpcm_samples_valid = 0;
					if (details.audio_format == WAV_FORMAT_MS_ADPCM) {
						DecodeMsAdpcmBlock(adpcm_raw_buf, raw_read, adpcm_samples_buf, adpcm_sample_cap, adpcm_samples_valid);
					} else {
						DecodeImaAdpcmBlock(adpcm_raw_buf, raw_read, adpcm_samples_buf, adpcm_sample_cap, adpcm_samples_valid);
					}
					if (adpcm_samples_valid == 0) break;
				}

				bytesRead = samples_done * sizeof(int16);
				if (bytesRead == 0 && currentByteOffset >= dataByteOffset + dataByteLength) {
					return uni::AudioResult::EndOfStream;
				}
				return uni::AudioResult::OK;
			}

			return uni::AudioResult::Unsupported;
		}

		virtual uni::AudioResult Seek(uint32 sampleIndex) override {
			if (details.audio_format == WAV_FORMAT_MS_ADPCM || details.audio_format == WAV_FORMAT_IMA_ADPCM) {
				uint32 smp_per_blk = details.samples_per_block ? details.samples_per_block : 1;
				uint32 target_block = sampleIndex / smp_per_blk;
				uint32 smp_in_blk = sampleIndex % smp_per_blk;
				uint32 blk_align = details.block_align ? details.block_align : 512;
				uint32 target_offset = dataByteOffset + target_block * blk_align;
				if (target_offset > dataByteOffset + dataByteLength) {
					return uni::AudioResult::InvalidArgument;
				}
				currentByteOffset = target_offset;
				adpcm_sample_pos = 0;
				adpcm_samples_valid = 0;

				if (smp_in_blk > 0 && currentByteOffset < dataByteOffset + dataByteLength) {
					uint32 raw_read = ReadRawBytes(currentByteOffset, adpcm_raw_buf, blk_align);
					currentByteOffset += raw_read;
					if (details.audio_format == WAV_FORMAT_MS_ADPCM) {
						DecodeMsAdpcmBlock(adpcm_raw_buf, raw_read, adpcm_samples_buf, adpcm_sample_cap, adpcm_samples_valid);
					} else {
						DecodeImaAdpcmBlock(adpcm_raw_buf, raw_read, adpcm_samples_buf, adpcm_sample_cap, adpcm_samples_valid);
					}
					uint32 offset_in_samples = smp_in_blk * details.channels;
					adpcm_sample_pos = (offset_in_samples < adpcm_samples_valid) ? offset_in_samples : adpcm_samples_valid;
				}
				return uni::AudioResult::OK;
			}

			uint32 bytes_per_sample = 0;
			if (details.audio_format == WAV_FORMAT_ALAW || details.audio_format == WAV_FORMAT_MULAW) {
				bytes_per_sample = details.channels;
			} else if (details.audio_format == WAV_FORMAT_IEEE_FLOAT) {
				bytes_per_sample = sizeof(float) * details.channels;
			} else {
				bytes_per_sample = (details.bits_per_sample / 8) * details.channels;
			}
			if (!bytes_per_sample) bytes_per_sample = 1;

			uint32 target_offset = dataByteOffset + sampleIndex * bytes_per_sample;
			if (target_offset > dataByteOffset + dataByteLength) {
				return uni::AudioResult::InvalidArgument;
			}
			currentByteOffset = target_offset;
			adpcm_sample_pos = 0;
			adpcm_samples_valid = 0;
			return uni::AudioResult::OK;
		}
	};

}

const char* uni::WAVCodec::GetName() const {
	return "WAV";
}

uni::AudioContainerFormat uni::WAVCodec::GetFormat() const {
	return uni::AudioContainerFormat::WAV;
}

const char* const* uni::WAVCodec::GetExtensions() const {
	static const char* const extensions[] = { "wav", "wave", nullptr };
	return extensions;
}

uni::AudioResult uni::WAVCodec::Probe(StorageTrait& storage, bool& matched) const {
	matched = false;

	WAVRIFFHEADER riff;
	stduint block_size = storage.Block_Size ? storage.Block_Size : 512;
	byte* block_buf = (byte*)malloc(block_size);
	if (!block_buf) {
		return uni::AudioResult::OutOfMemory;
	}

	stduint read_bytes = storage.Read(0, &riff, sizeof(riff), block_buf);
	free(block_buf);

	if (read_bytes == sizeof(riff) &&
		riff.riff_tag == WAV_FOURCC_RIFF &&
		riff.wave_tag == WAV_FOURCC_WAVE) {
		matched = true;
	}

	return uni::AudioResult::OK;
}

uni::AudioResult uni::WAVCodec::ReadInfo(StorageTrait& storage, AudioInfo& outInfo) const {
	bool matched = false;
	uni::AudioResult res = Probe(storage, matched);
	if (res != uni::AudioResult::OK) return res;
	if (!matched) return uni::AudioResult::InvalidFormat;

	stduint block_size = storage.Block_Size ? storage.Block_Size : 512;
	byte* block_buf = (byte*)malloc(block_size);
	if (!block_buf) {
		return uni::AudioResult::OutOfMemory;
	}

	stduint total_storage_size = storage.getUnits() * storage.Block_Size;
	stduint offset = sizeof(WAVRIFFHEADER);

	bool has_fmt = false;
	bool has_data = false;
	WAVFMTPCM fmt{};
	WAVFormatDetails details{};
	uint32 data_offset = 0;
	uint32 data_size = 0;

	while (offset + sizeof(WAVCHUNKHEADER) <= total_storage_size) {
		WAVCHUNKHEADER chunk;
		stduint rd = storage.Read(offset, &chunk, sizeof(chunk), block_buf);
		if (rd != sizeof(chunk)) break;

		uint32 chunk_data_offset = (uint32)(offset + sizeof(WAVCHUNKHEADER));
		uint32 padded_size = chunk.chunk_size + (chunk.chunk_size & 1u);

		if (chunk.chunk_tag == WAV_FOURCC_FMT) {
			if (chunk.chunk_size >= sizeof(WAVFMTPCM)) {
				rd = storage.Read(chunk_data_offset, &fmt, sizeof(fmt), block_buf);
				if (rd == sizeof(fmt)) {
					has_fmt = true;
					details.audio_format = fmt.audio_format;
					details.channels = fmt.channel_count;
					details.sample_rate = fmt.sample_rate;
					details.byte_rate = fmt.byte_rate;
					details.block_align = fmt.block_align;
					details.bits_per_sample = fmt.bits_per_sample;
					details.samples_per_block = 0;
					details.num_coef = 0;

					// Extensible format check
					if (fmt.audio_format == WAV_FORMAT_EXTENSIBLE && chunk.chunk_size >= sizeof(WAVFORMATEXTENSIBLE)) {
						WAVFORMATEXTENSIBLE ext{};
						if (storage.Read(chunk_data_offset, &ext, sizeof(ext), block_buf) == sizeof(ext)) {
							uint16 sub_code = (uint16)(ext.sub_format[0] | (ext.sub_format[1] << 8));
							details.audio_format = sub_code;
							details.samples_per_block = ext.samples.samples_per_block;
						}
					}
					// MS-ADPCM format extra data
					else if (fmt.audio_format == WAV_FORMAT_MS_ADPCM && chunk.chunk_size >= 22) {
						byte extra[64];
						uint32 extra_len = chunk.chunk_size - sizeof(WAVFMTPCM);
						if (extra_len > sizeof(extra)) extra_len = sizeof(extra);
						if (storage.Read(chunk_data_offset + sizeof(WAVFMTPCM), extra, extra_len, block_buf) == extra_len) {
							if (extra_len >= 4) {
								details.samples_per_block = (uint16)(extra[2] | (extra[3] << 8));
								uint16 num_c = (uint16)(extra[4] | (extra[5] << 8));
								if (num_c > 32) num_c = 32;
								details.num_coef = num_c;
								for (uint16 i = 0; i < num_c && (6 + i * 4 + 3) < extra_len; ++i) {
									details.coef[i][0] = (int16)(extra[6 + i * 4] | (extra[6 + i * 4 + 1] << 8));
									details.coef[i][1] = (int16)(extra[6 + i * 4 + 2] | (extra[6 + i * 4 + 3] << 8));
								}
							}
						}
					}
					// IMA-ADPCM format extra data
					else if (fmt.audio_format == WAV_FORMAT_IMA_ADPCM && chunk.chunk_size >= 20) {
						byte extra[8];
						if (storage.Read(chunk_data_offset + sizeof(WAVFMTPCM), extra, 4, block_buf) == 4) {
							details.samples_per_block = (uint16)(extra[2] | (extra[3] << 8));
						}
					}
				}
			}
		}
		else if (chunk.chunk_tag == WAV_FOURCC_DATA) {
			data_offset = chunk_data_offset;
			data_size = chunk.chunk_size;
			has_data = true;
			if (has_fmt) break;
		}

		offset = chunk_data_offset + padded_size;
	}

	free(block_buf);

	if (!has_fmt || !has_data) {
		return uni::AudioResult::InvalidFormat;
	}

	if (details.num_coef == 0 && details.audio_format == WAV_FORMAT_MS_ADPCM) {
		details.num_coef = 7;
		for (int i = 0; i < 7; ++i) {
			details.coef[i][0] = MS_ADPCM_DEFAULT_COEF[i][0];
			details.coef[i][1] = MS_ADPCM_DEFAULT_COEF[i][1];
		}
	}

	if (details.samples_per_block == 0) {
		if (details.audio_format == WAV_FORMAT_MS_ADPCM && details.block_align) {
			uint32 hdr_bytes = (details.channels == 2) ? 14 : 7;
			if (details.block_align > hdr_bytes) {
				details.samples_per_block = uint16(2 + ((details.block_align - hdr_bytes) * 2) / details.channels);
			}
		} else if (details.audio_format == WAV_FORMAT_IMA_ADPCM && details.block_align) {
			uint32 hdr_bytes = (details.channels == 2) ? 8 : 4;
			if (details.block_align > hdr_bytes) {
				details.samples_per_block = uint16(1 + ((details.block_align - hdr_bytes) * 2) / details.channels);
			}
		}
	}

	// Output format normalization
	if (details.audio_format == WAV_FORMAT_PCM && details.bits_per_sample == 8) {
		outInfo.format.sample_format = uni::AudioSampleFormat::U8;
		outInfo.bitsPerSample = 8;
	} else {
		outInfo.format.sample_format = uni::AudioSampleFormat::S16LE;
		outInfo.bitsPerSample = 16;
	}

	outInfo.format.channels = details.channels;
	outInfo.format.sample_rate = details.sample_rate;
	outInfo.containerFormat = uni::AudioContainerFormat::WAV;
	outInfo.dataByteOffset = data_offset;
	outInfo.dataByteLength = data_size;

	if (details.audio_format == WAV_FORMAT_MS_ADPCM || details.audio_format == WAV_FORMAT_IMA_ADPCM) {
		uint32 blk = details.block_align ? details.block_align : 512;
		uint32 num_blocks = data_size / blk;
		outInfo.totalSamples = num_blocks * details.samples_per_block;
	} else if (details.audio_format == WAV_FORMAT_ALAW || details.audio_format == WAV_FORMAT_MULAW) {
		outInfo.totalSamples = details.channels ? (data_size / details.channels) : 0;
	} else if (details.audio_format == WAV_FORMAT_IEEE_FLOAT) {
		outInfo.totalSamples = (details.channels && details.bits_per_sample) ? (data_size / ((details.bits_per_sample / 8) * details.channels)) : 0;
	} else {
		uint32 bytes_per_src_sample = (details.bits_per_sample / 8) * details.channels;
		outInfo.totalSamples = bytes_per_src_sample ? (data_size / bytes_per_src_sample) : 0;
	}

	outInfo.durationMs = details.sample_rate ?
		(uint32)(((uint64)outInfo.totalSamples * 1000) / details.sample_rate) : 0;

	return uni::AudioResult::OK;
}

uni::AudioResult uni::WAVCodec::OpenStream(
	StorageTrait& storage,
	IAudioStream*& outStream,
	trait::Malloc& allocator
) const {
	bool matched = false;
	uni::AudioResult res = Probe(storage, matched);
	if (res != uni::AudioResult::OK) return res;
	if (!matched) return uni::AudioResult::InvalidFormat;

	stduint block_size = storage.Block_Size ? storage.Block_Size : 512;
	byte* block_buf = (byte*)malloc(block_size);
	if (!block_buf) {
		return uni::AudioResult::OutOfMemory;
	}

	stduint total_storage_size = storage.getUnits() * storage.Block_Size;
	stduint offset = sizeof(WAVRIFFHEADER);

	bool has_fmt = false;
	bool has_data = false;
	WAVFMTPCM fmt{};
	WAVFormatDetails details{};
	uint32 data_offset = 0;
	uint32 data_size = 0;

	while (offset + sizeof(WAVCHUNKHEADER) <= total_storage_size) {
		WAVCHUNKHEADER chunk;
		stduint rd = storage.Read(offset, &chunk, sizeof(chunk), block_buf);
		if (rd != sizeof(chunk)) break;

		uint32 chunk_data_offset = (uint32)(offset + sizeof(WAVCHUNKHEADER));
		uint32 padded_size = chunk.chunk_size + (chunk.chunk_size & 1u);

		if (chunk.chunk_tag == WAV_FOURCC_FMT) {
			if (chunk.chunk_size >= sizeof(WAVFMTPCM)) {
				rd = storage.Read(chunk_data_offset, &fmt, sizeof(fmt), block_buf);
				if (rd == sizeof(fmt)) {
					has_fmt = true;
					details.audio_format = fmt.audio_format;
					details.channels = fmt.channel_count;
					details.sample_rate = fmt.sample_rate;
					details.byte_rate = fmt.byte_rate;
					details.block_align = fmt.block_align;
					details.bits_per_sample = fmt.bits_per_sample;
					details.samples_per_block = 0;
					details.num_coef = 0;

					if (fmt.audio_format == WAV_FORMAT_EXTENSIBLE && chunk.chunk_size >= sizeof(WAVFORMATEXTENSIBLE)) {
						WAVFORMATEXTENSIBLE ext{};
						if (storage.Read(chunk_data_offset, &ext, sizeof(ext), block_buf) == sizeof(ext)) {
							uint16 sub_code = (uint16)(ext.sub_format[0] | (ext.sub_format[1] << 8));
							details.audio_format = sub_code;
							details.samples_per_block = ext.samples.samples_per_block;
						}
					}
					else if (fmt.audio_format == WAV_FORMAT_MS_ADPCM && chunk.chunk_size >= 22) {
						byte extra[64];
						uint32 extra_len = chunk.chunk_size - sizeof(WAVFMTPCM);
						if (extra_len > sizeof(extra)) extra_len = sizeof(extra);
						if (storage.Read(chunk_data_offset + sizeof(WAVFMTPCM), extra, extra_len, block_buf) == extra_len) {
							if (extra_len >= 4) {
								details.samples_per_block = (uint16)(extra[2] | (extra[3] << 8));
								uint16 num_c = (uint16)(extra[4] | (extra[5] << 8));
								if (num_c > 32) num_c = 32;
								details.num_coef = num_c;
								for (uint16 i = 0; i < num_c && (6 + i * 4 + 3) < extra_len; ++i) {
									details.coef[i][0] = (int16)(extra[6 + i * 4] | (extra[6 + i * 4 + 1] << 8));
									details.coef[i][1] = (int16)(extra[6 + i * 4 + 2] | (extra[6 + i * 4 + 3] << 8));
								}
							}
						}
					}
					else if (fmt.audio_format == WAV_FORMAT_IMA_ADPCM && chunk.chunk_size >= 20) {
						byte extra[8];
						if (storage.Read(chunk_data_offset + sizeof(WAVFMTPCM), extra, 4, block_buf) == 4) {
							details.samples_per_block = (uint16)(extra[2] | (extra[3] << 8));
						}
					}
				}
			}
		}
		else if (chunk.chunk_tag == WAV_FOURCC_DATA) {
			data_offset = chunk_data_offset;
			data_size = chunk.chunk_size;
			has_data = true;
			if (has_fmt) break;
		}

		offset = chunk_data_offset + padded_size;
	}

	free(block_buf);

	if (!has_fmt || !has_data) {
		return uni::AudioResult::InvalidFormat;
	}

	if (details.num_coef == 0 && details.audio_format == WAV_FORMAT_MS_ADPCM) {
		details.num_coef = 7;
		for (int i = 0; i < 7; ++i) {
			details.coef[i][0] = MS_ADPCM_DEFAULT_COEF[i][0];
			details.coef[i][1] = MS_ADPCM_DEFAULT_COEF[i][1];
		}
	}

	if (details.samples_per_block == 0) {
		if (details.audio_format == WAV_FORMAT_MS_ADPCM && details.block_align) {
			uint32 hdr_bytes = (details.channels == 2) ? 14 : 7;
			if (details.block_align > hdr_bytes) {
				details.samples_per_block = uint16(2 + ((details.block_align - hdr_bytes) * 2) / details.channels);
			}
		} else if (details.audio_format == WAV_FORMAT_IMA_ADPCM && details.block_align) {
			uint32 hdr_bytes = (details.channels == 2) ? 8 : 4;
			if (details.block_align > hdr_bytes) {
				details.samples_per_block = uint16(1 + ((details.block_align - hdr_bytes) * 2) / details.channels);
			}
		}
	}

	details.data_offset = data_offset;
	details.data_size = data_size;

	AudioInfo info{};
	if (details.audio_format == WAV_FORMAT_PCM && details.bits_per_sample == 8) {
		info.format.sample_format = uni::AudioSampleFormat::U8;
		info.bitsPerSample = 8;
	} else {
		info.format.sample_format = uni::AudioSampleFormat::S16LE;
		info.bitsPerSample = 16;
	}

	info.format.channels = details.channels;
	info.format.sample_rate = details.sample_rate;
	info.containerFormat = uni::AudioContainerFormat::WAV;
	info.dataByteOffset = data_offset;
	info.dataByteLength = data_size;

	if (details.audio_format == WAV_FORMAT_MS_ADPCM || details.audio_format == WAV_FORMAT_IMA_ADPCM) {
		uint32 blk = details.block_align ? details.block_align : 512;
		uint32 num_blocks = data_size / blk;
		info.totalSamples = num_blocks * details.samples_per_block;
	} else if (details.audio_format == WAV_FORMAT_ALAW || details.audio_format == WAV_FORMAT_MULAW) {
		info.totalSamples = details.channels ? (data_size / details.channels) : 0;
	} else if (details.audio_format == WAV_FORMAT_IEEE_FLOAT) {
		info.totalSamples = (details.channels && details.bits_per_sample) ? (data_size / ((details.bits_per_sample / 8) * details.channels)) : 0;
	} else {
		uint32 bytes_per_src_sample = (details.bits_per_sample / 8) * details.channels;
		info.totalSamples = bytes_per_src_sample ? (data_size / bytes_per_src_sample) : 0;
	}

	info.durationMs = details.sample_rate ?
		(uint32)(((uint64)info.totalSamples * 1000) / details.sample_rate) : 0;

	void* mem = allocator.allocate(sizeof(WAVStream), 3);// 3 => 2^3 = 8 字节对齐: WAVStream 含 vptr, 未对齐对象会 UNDEFINSTR(trait::Malloc& 默认实参 0 对 mempool 意味着仅 1 字节对齐)
	if (!mem) return uni::AudioResult::OutOfMemory;

	WAVStream* stream = new (mem) WAVStream(storage, info, details, allocator);
	if (!stream->IsValid()) {
		stream->Release();
		return uni::AudioResult::OutOfMemory;
	}
	outStream = stream;
	return uni::AudioResult::OK;
}

#endif
