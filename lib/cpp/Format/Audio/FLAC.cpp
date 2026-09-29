// ASCII C/C++ TAB4 CRLF
// Docutitle: Free Lossless Audio Codec (FLAC) Codec Implementation
// Attribute: Env-Freestanding Non-Dependence
// Copyright: UNISYM

#include "../../../../inc/c/format/audio/FLAC.h"
#include "../../../../inc/c/ustring.h"
#include <stdlib.h>

#if defined(_INC_CPP)

namespace {

	#define FLAC_MAX_CHANNELS           8
	#define FLAC_MAX_BLOCK_SIZE         65535
	#define FLAC_MAX_LPC_ORDER          32

	struct BitReader {
		const uint8* buf;
		uint32 pos;
		uint32 limit;
	};

	struct FLACFrameHeader {
		uint32 block_size;
		uint32 sample_rate;
		uint8  channels;
		uint8  channel_assignment; // 0..7: independent 1..8ch, 8: left/side, 9: right/side, 10: mid/side
		uint8  bits_per_sample;
		uint8  blocking_strategy;  // 0: fixed, 1: variable
		uint64 sample_or_frame_number;
		uint8  crc8;
	};

	struct FLACSubframeHeader {
		uint8  subframe_type; // 0: constant, 1: verbatim, 8..12: fixed (order 0..4), 32..63: lpc (order 1..32)
		uint8  wasted_bits;
		uint8  order;
		int32  warmup[FLAC_MAX_LPC_ORDER];
		int32  qlp_coeff[FLAC_MAX_LPC_ORDER];
		int32  qlp_shift;
	};

} // anonymous namespace

namespace flac {

	namespace Bitstream {

		static inline void InitializeBitReader(BitReader* bs, const uint8* data, uint32 bytes) {
			bs->buf = data;
			bs->pos = 0;
			bs->limit = bytes * 8;
		}

		static inline uint32 getBits(BitReader* bs, uint32 n) {
			if (!n) return 0;
			uint32 next, cache = 0, s = bs->pos & 7;
			int32 shl = (int32)n + (int32)s;
			const uint8* p = bs->buf + (bs->pos >> 3);
			if ((bs->pos += n) > bs->limit) return 0;
			next = *p++ & (255 >> s);
			while ((shl -= 8) > 0) {
				cache |= next << shl;
				next = *p++;
			}
			return cache | (next >> -shl);
		}

		static inline uint64 getBits64(BitReader* bs, uint32 n) {
			if (n <= 32) return (uint64)getBits(bs, n);
			uint64 hi = (uint64)getBits(bs, n - 32);
			uint64 lo = (uint64)getBits(bs, 32);
			return (hi << 32) | lo;
		}

		static inline int32 getSignedBits(BitReader* bs, uint32 n) {
			if (!n) return 0;
			uint32 u = getBits(bs, n);
			if (u & (1u << (n - 1))) {
				u |= ~((1u << n) - 1);
			}
			return (int32)u;
		}

		static inline uint32 getUnary(BitReader* bs) {
			uint32 count = 0;
			while (bs->pos < bs->limit) {
				uint32 bit = getBits(bs, 1);
				if (bit) break;
				count++;
			}
			return count;
		}

		static inline uint64 decodeUtf8Int(BitReader* bs) {
			uint32 v = getBits(bs, 8);
			if (!(v & 0x80)) {
				return v;
			}
			int extra_bytes = 0;
			uint64 result = 0;
			if ((v & 0xE0) == 0xC0) {
				result = v & 0x1F;
				extra_bytes = 1;
			} else if ((v & 0xF0) == 0xE0) {
				result = v & 0x0F;
				extra_bytes = 2;
			} else if ((v & 0xF8) == 0xF0) {
				result = v & 0x07;
				extra_bytes = 3;
			} else if ((v & 0xFC) == 0xF8) {
				result = v & 0x03;
				extra_bytes = 4;
			} else if ((v & 0xFE) == 0xFC) {
				result = v & 0x01;
				extra_bytes = 5;
			} else if (v == 0xFE) {
				result = 0;
				extra_bytes = 6;
			} else {
				return 0;
			}

			for (int i = 0; i < extra_bytes; i++) {
				uint32 b = getBits(bs, 8);
				if ((b & 0xC0) != 0x80) return 0;
				result = (result << 6) | (b & 0x3F);
			}
			return result;
		}

		static inline uint8 computeCrc8(const uint8* data, uint32 len) {
			uint8 crc = 0;
			for (uint32 i = 0; i < len; i++) {
				crc ^= data[i];
				for (int b = 0; b < 8; b++) {
					if (crc & 0x80) {
						crc = (uint8)((crc << 1) ^ 0x07);
					} else {
						crc = (uint8)(crc << 1);
					}
				}
			}
			return crc;
		}

		static bool parseStreamInfo(const uint8* data, uint32 len, FLAC_STREAMINFO& outInfo) {
			if (!data || len < 34) return false;
			BitReader bs;
			InitializeBitReader(&bs, data, len);

			outInfo.min_blocksize = (uint16)getBits(&bs, 16);
			outInfo.max_blocksize = (uint16)getBits(&bs, 16);
			outInfo.min_framesize = getBits(&bs, 24);
			outInfo.max_framesize = getBits(&bs, 24);
			outInfo.sample_rate = getBits(&bs, 20);
			outInfo.channels = (uint8)(getBits(&bs, 3) + 1);
			outInfo.bits_per_sample = (uint8)(getBits(&bs, 5) + 1);
			outInfo.total_samples = getBits64(&bs, 36);
			for (int i = 0; i < 16; i++) {
				outInfo.md5[i] = (uint8)getBits(&bs, 8);
			}
			return outInfo.sample_rate > 0 && outInfo.channels > 0 && outInfo.bits_per_sample > 0;
		}

		static bool parseFrameHeader(BitReader* bs, const FLAC_STREAMINFO& stream_info, FLACFrameHeader& outHdr) {
			uint32 start_pos = bs->pos;
			uint32 sync = getBits(bs, 14);
			if (sync != 0x3FFE) return false;

			uint32 reserved = getBits(bs, 1);
			if (reserved != 0) return false;

			outHdr.blocking_strategy = (uint8)getBits(bs, 1);
			uint32 bs_code = getBits(bs, 4);
			uint32 sr_code = getBits(bs, 4);
			uint32 ch_code = getBits(bs, 4);
			uint32 bps_code = getBits(bs, 3);
			uint32 reserved2 = getBits(bs, 1);
			if (reserved2 != 0) return false;

			outHdr.sample_or_frame_number = decodeUtf8Int(bs);

			// Block size
			if (bs_code == 0) return false;
			else if (bs_code == 1) outHdr.block_size = 192;
			else if (bs_code >= 2 && bs_code <= 5) outHdr.block_size = 576 << (bs_code - 2);
			else if (bs_code == 6) outHdr.block_size = 0; // 8-bit at end
			else if (bs_code == 7) outHdr.block_size = 0; // 16-bit at end
			else if (bs_code >= 8 && bs_code <= 15) outHdr.block_size = 256 << (bs_code - 8);

			// Sample rate
			if (sr_code == 0) outHdr.sample_rate = stream_info.sample_rate;
			else if (sr_code == 1) outHdr.sample_rate = 88200;
			else if (sr_code == 2) outHdr.sample_rate = 176400;
			else if (sr_code == 3) outHdr.sample_rate = 192000;
			else if (sr_code == 4) outHdr.sample_rate = 8000;
			else if (sr_code == 5) outHdr.sample_rate = 16000;
			else if (sr_code == 6) outHdr.sample_rate = 22050;
			else if (sr_code == 7) outHdr.sample_rate = 24000;
			else if (sr_code == 8) outHdr.sample_rate = 32000;
			else if (sr_code == 9) outHdr.sample_rate = 44100;
			else if (sr_code == 10) outHdr.sample_rate = 48000;
			else if (sr_code == 11) outHdr.sample_rate = 96000;
			else if (sr_code >= 12 && sr_code <= 14) outHdr.sample_rate = 0; // custom at end
			else return false;

			// Channel assignment
			outHdr.channel_assignment = (uint8)ch_code;
			if (ch_code <= 7) outHdr.channels = (uint8)(ch_code + 1);
			else if (ch_code >= 8 && ch_code <= 10) outHdr.channels = 2;
			else return false;

			// Bits per sample
			if (bps_code == 0) outHdr.bits_per_sample = stream_info.bits_per_sample;
			else if (bps_code == 1) outHdr.bits_per_sample = 8;
			else if (bps_code == 2) outHdr.bits_per_sample = 12;
			else if (bps_code == 4) outHdr.bits_per_sample = 16;
			else if (bps_code == 5) outHdr.bits_per_sample = 20;
			else if (bps_code == 6) outHdr.bits_per_sample = 24;
			else if (bps_code == 7) outHdr.bits_per_sample = 32;
			else return false;

			// Check custom block size
			if (bs_code == 6) outHdr.block_size = getBits(bs, 8) + 1;
			else if (bs_code == 7) outHdr.block_size = getBits(bs, 16) + 1;

			// Check custom sample rate
			if (sr_code == 12) outHdr.sample_rate = getBits(bs, 8) * 1000;
			else if (sr_code == 13) outHdr.sample_rate = getBits(bs, 16);
			else if (sr_code == 14) outHdr.sample_rate = getBits(bs, 16) * 10;

			outHdr.crc8 = (uint8)getBits(bs, 8);

			uint32 end_pos = bs->pos - 8;
			uint32 header_bytes = (end_pos - start_pos) / 8;
			const uint8* header_data = bs->buf + (start_pos / 8);
			uint8 actual_crc = computeCrc8(header_data, header_bytes);
			return actual_crc == outHdr.crc8;
		}

	} // namespace Bitstream

	namespace Entropy {
		using namespace Bitstream;

		static bool decodeRiceResiduals(
			BitReader* bs,
			int32* out_samples,
			uint32 count,
			uint32 predictor_order,
			bool is_rice2
		) {
			uint32 coding_method = getBits(bs, 2);
			if (coding_method > 1) return false; // 00: 4-bit rice param, 01: 5-bit rice param

			uint32 partition_order = getBits(bs, 4);
			uint32 num_partitions = 1u << partition_order;
			uint32 rice_param_bits = (coding_method == 0) ? 4 : 5;
			uint32 escape_param = (coding_method == 0) ? 0x0F : 0x1F;

			uint32 sample_idx = predictor_order;

			for (uint32 p = 0; p < num_partitions; p++) {
				uint32 param = getBits(bs, rice_param_bits);
				uint32 part_samples;
				if (partition_order == 0) {
					part_samples = count - predictor_order;
				} else if (p == 0) {
					part_samples = (count >> partition_order) - predictor_order;
				} else {
					part_samples = count >> partition_order;
				}

				if (param < escape_param) {
					for (uint32 i = 0; i < part_samples; i++, sample_idx++) {
						uint32 msbs = getUnary(bs);
						uint32 lsbs = getBits(bs, param);
						uint32 uval = (msbs << param) | lsbs;
						// Zigzag decoding
						int32 sval = (uval & 1) ? -(int32)(uval >> 1) - 1 : (int32)(uval >> 1);
						out_samples[sample_idx] = sval;
					}
				} else {
					uint32 unencoded_bits = getBits(bs, 5);
					for (uint32 i = 0; i < part_samples; i++, sample_idx++) {
						out_samples[sample_idx] = getSignedBits(bs, unencoded_bits);
					}
				}
			}
			return true;
		}

	} // namespace Entropy

	namespace Predictor {

		static void restoreFixedLPC(int32* samples, uint32 count, uint32 order) {
			if (order == 0) {
				// No change
			} else if (order == 1) {
				for (uint32 i = 1; i < count; i++) {
					samples[i] += samples[i - 1];
				}
			} else if (order == 2) {
				for (uint32 i = 2; i < count; i++) {
					samples[i] += 2 * samples[i - 1] - samples[i - 2];
				}
			} else if (order == 3) {
				for (uint32 i = 3; i < count; i++) {
					samples[i] += 3 * samples[i - 1] - 3 * samples[i - 2] + samples[i - 3];
				}
			} else if (order == 4) {
				for (uint32 i = 4; i < count; i++) {
					samples[i] += 4 * samples[i - 1] - 6 * samples[i - 2] + 4 * samples[i - 3] - samples[i - 4];
				}
			}
		}

		static void restoreLinearLPC(
			int32* samples,
			uint32 count,
			uint32 order,
			const int32* qlp_coeff,
			int32 qlp_shift
		) {
			for (uint32 i = order; i < count; i++) {
				int64 sum = 0;
				for (uint32 j = 0; j < order; j++) {
					sum += (int64)qlp_coeff[j] * (int64)samples[i - 1 - j];
				}
				samples[i] += (int32)(sum >> qlp_shift);
			}
		}

	} // namespace Predictor

	namespace Stereo {

		static void decodeStereoChannels(
			int32* left,
			int32* right,
			uint32 count,
			uint8 channel_assignment
		) {
			if (channel_assignment == 8) { // Left / Side
				for (uint32 i = 0; i < count; i++) {
					right[i] = left[i] - right[i];
				}
			} else if (channel_assignment == 9) { // Right / Side
				for (uint32 i = 0; i < count; i++) {
					left[i] = left[i] + right[i];
				}
			} else if (channel_assignment == 10) { // Mid / Side
				for (uint32 i = 0; i < count; i++) {
					int32 mid = left[i];
					int32 side = right[i];
					mid = (mid * 2) | (side & 1);
					left[i] = (mid + side) >> 1;
					right[i] = (mid - side) >> 1;
				}
			}
		}

	} // namespace Stereo

	namespace Engine {
		using namespace Bitstream;
		using namespace Entropy;
		using namespace Predictor;
		using namespace Stereo;

		static bool decodeSubframe(
			BitReader* bs,
			int32* samples,
			uint32 block_size,
			uint8 subframe_bps
		) {
			uint32 pad = getBits(bs, 1);
			if (pad != 0) return false;

			uint32 type = getBits(bs, 6);
			uint32 wasted_flag = getBits(bs, 1);
			uint32 wasted_bits = 0;
			if (wasted_flag) {
				wasted_bits = getUnary(bs) + 1;
				subframe_bps -= (uint8)wasted_bits;
			}

			if (type == 0) { // Constant
				int32 val = getSignedBits(bs, subframe_bps);
				for (uint32 i = 0; i < block_size; i++) {
					samples[i] = val;
				}
			} else if (type == 1) { // Verbatim
				for (uint32 i = 0; i < block_size; i++) {
					samples[i] = getSignedBits(bs, subframe_bps);
				}
			} else if (type >= 8 && type <= 12) { // Fixed LPC (order 0..4)
				uint32 order = type - 8;
				for (uint32 i = 0; i < order; i++) {
					samples[i] = getSignedBits(bs, subframe_bps);
				}
				if (!decodeRiceResiduals(bs, samples, block_size, order, false)) {
					return false;
				}
				restoreFixedLPC(samples, block_size, order);
			} else if (type >= 32 && type <= 63) { // LPC (order 1..32)
				uint32 order = type - 31;
				for (uint32 i = 0; i < order; i++) {
					samples[i] = getSignedBits(bs, subframe_bps);
				}
				uint32 qlp_prec = getBits(bs, 4) + 1;
				int32 qlp_shift = getSignedBits(bs, 5);
				int32 qlp_coeff[FLAC_MAX_LPC_ORDER];
				for (uint32 i = 0; i < order; i++) {
					qlp_coeff[i] = getSignedBits(bs, qlp_prec);
				}
				if (!decodeRiceResiduals(bs, samples, block_size, order, false)) {
					return false;
				}
				restoreLinearLPC(samples, block_size, order, qlp_coeff, qlp_shift);
			} else {
				return false;
			}

			if (wasted_bits > 0) {
				for (uint32 i = 0; i < block_size; i++) {
					samples[i] <<= wasted_bits;
				}
			}
			return true;
		}

		static int decodeFrame(
			const uint8* flac_data,
			uint32 flac_bytes,
			const FLAC_STREAMINFO& stream_info,
			int32 channel_buffers[FLAC_MAX_CHANNELS][FLAC_MAX_BLOCK_SIZE],
			FLACFrameHeader& outHdr
		) {
			if (flac_bytes < 10) return 0;
			BitReader bs;
			InitializeBitReader(&bs, flac_data, flac_bytes);

			if (!parseFrameHeader(&bs, stream_info, outHdr)) {
				return 0;
			}

			if (outHdr.block_size > FLAC_MAX_BLOCK_SIZE || outHdr.channels > FLAC_MAX_CHANNELS) {
				return 0;
			}

			for (uint8 ch = 0; ch < outHdr.channels; ch++) {
				uint8 subframe_bps = outHdr.bits_per_sample;
				if (outHdr.channel_assignment == 8 && ch == 1) subframe_bps++;      // Side channel
				else if (outHdr.channel_assignment == 9 && ch == 0) subframe_bps++; // Side channel
				else if (outHdr.channel_assignment == 10 && ch == 1) subframe_bps++;// Side channel

				if (!decodeSubframe(&bs, channel_buffers[ch], outHdr.block_size, subframe_bps)) {
					return 0;
				}
			}

			// Zero padding to byte boundary
			if (bs.pos & 7) {
				getBits(&bs, 8 - (bs.pos & 7));
			}

			// CRC-16 footer
			getBits(&bs, 16);

			if (outHdr.channels == 2 && outHdr.channel_assignment >= 8) {
				decodeStereoChannels(channel_buffers[0], channel_buffers[1], outHdr.block_size, outHdr.channel_assignment);
			}

			return (int)((bs.pos + 7) / 8);
		}

	} // namespace Engine

} // namespace flac

namespace {

	using namespace flac::Bitstream;
	using namespace flac::Engine;

	struct FLACFormatDetails {
		FLAC_STREAMINFO stream_info;
		uint32          data_offset;
		uint32          data_size;
		uint32          duration_ms;
		uint32          total_samples;
	};

	class FLACStream : public uni::IAudioStream {
	private:
		uni::StorageTrait*  storage;
		uni::AudioInfo      info;
		FLACFormatDetails   details;
		uint32              dataByteOffset;
		uint32              dataByteLength;
		uint32              currentByteOffset;
		uni::trait::Malloc* allocator;

		byte*               input_buf;
		uint32              input_buf_size;
		uint32              input_buf_valid;
		uint32              input_buf_pos;

		int16*              pcm_buf;
		uint32              pcm_samples_valid;
		uint32              pcm_sample_pos;

		byte*               sector_buf;
		stduint             cached_block;

		int32*              channel_buffers[FLAC_MAX_CHANNELS];

		uint32 ReadRawBytes(uint32 file_offset, byte* dest, uint32 count) {
			if (!storage || !dest || !count) return 0;
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

		bool RefillInputBuffer() {
			if (input_buf_pos > 0) {
				if (input_buf_valid > input_buf_pos) {
					uint32 remain = input_buf_valid - input_buf_pos;
					MemCopyN(input_buf, input_buf + input_buf_pos, remain);
					input_buf_valid = remain;
				} else {
					input_buf_valid = 0;
				}
				input_buf_pos = 0;
			}

			uint32 space_left = input_buf_size - input_buf_valid;
			if (space_left > 0 && currentByteOffset < dataByteOffset + dataByteLength) {
				uint32 bytes_to_read = space_left;
				if (currentByteOffset + bytes_to_read > dataByteOffset + dataByteLength) {
					bytes_to_read = dataByteOffset + dataByteLength - currentByteOffset;
				}
				uint32 nread = ReadRawBytes(currentByteOffset, input_buf + input_buf_valid, bytes_to_read);
				if (nread > 0) {
					input_buf_valid += nread;
					currentByteOffset += nread;
				}
			}
			return input_buf_valid > 0;
		}

		bool DecodeNextFrame() {
			pcm_sample_pos = 0;
			pcm_samples_valid = 0;

			while (true) {
				RefillInputBuffer();
				if (input_buf_valid <= input_buf_pos) {
					return false;
				}

				// Look for frame sync
				const uint8* ptr = input_buf + input_buf_pos;
				uint32 remain = input_buf_valid - input_buf_pos;
				if (remain < 10) return false;

				FLACFrameHeader hdr{};
				int32 (*ch_ptrs)[FLAC_MAX_BLOCK_SIZE] = (int32 (*)[FLAC_MAX_BLOCK_SIZE])channel_buffers[0];
				int consumed = decodeFrame(ptr, remain, details.stream_info, ch_ptrs, hdr);

				if (consumed > 0 && hdr.block_size > 0) {
					input_buf_pos += (uint32)consumed;

					// Interleave channels to 16-bit PCM buffer
					uint32 channels = hdr.channels;
					uint32 block_size = hdr.block_size;
					int shift = (int)hdr.bits_per_sample - 16;

					uint32 out_idx = 0;
					for (uint32 i = 0; i < block_size; i++) {
						for (uint32 ch = 0; ch < channels; ch++) {
							int32 sample = channel_buffers[ch][i];
							if (shift > 0) sample >>= shift;
							else if (shift < 0) sample <<= -shift;

							if (sample > 32767) sample = 32767;
							else if (sample < -32768) sample = -32768;

							pcm_buf[out_idx++] = (int16)sample;
						}
					}

					pcm_samples_valid = block_size * channels;
					return true;
				}

				// Advance by 1 byte to find sync
				input_buf_pos++;
			}
		}

	public:
		FLACStream(
			uni::StorageTrait& stor,
			const uni::AudioInfo& inf,
			const FLACFormatDetails& det,
			uni::trait::Malloc& alloc
		) : storage(&stor), info(inf), details(det),
		    dataByteOffset(det.data_offset), dataByteLength(det.data_size),
		    currentByteOffset(det.data_offset), allocator(&alloc),
		    input_buf(nullptr), input_buf_size(32768), input_buf_valid(0), input_buf_pos(0),
		    pcm_buf(nullptr), pcm_samples_valid(0), pcm_sample_pos(0),
		    sector_buf(nullptr), cached_block((stduint)~0)
		{
			stduint block_size = storage->Block_Size ? storage->Block_Size : 512;
			sector_buf = (byte*)allocator->allocate(block_size, 3);
			input_buf = (byte*)allocator->allocate(input_buf_size, 3);
			pcm_buf = (int16*)allocator->allocate(FLAC_MAX_BLOCK_SIZE * FLAC_MAX_CHANNELS * sizeof(int16), 3);

			// Allocate contiguous channel buffers
			channel_buffers[0] = (int32*)allocator->allocate(FLAC_MAX_CHANNELS * FLAC_MAX_BLOCK_SIZE * sizeof(int32), 3);
			for (int ch = 1; ch < FLAC_MAX_CHANNELS; ch++) {
				channel_buffers[ch] = channel_buffers[0] + ch * FLAC_MAX_BLOCK_SIZE;
			}
		}

		virtual ~FLACStream() {
			if (channel_buffers[0] && allocator) {
				allocator->deallocate(channel_buffers[0]);
				channel_buffers[0] = nullptr;
			}
			if (pcm_buf && allocator) {
				allocator->deallocate(pcm_buf);
				pcm_buf = nullptr;
			}
			if (sector_buf && allocator) {
				allocator->deallocate(sector_buf);
				sector_buf = nullptr;
			}
			if (input_buf && allocator) {
				allocator->deallocate(input_buf);
				input_buf = nullptr;
			}
		}

		bool isValid() const {
			return sector_buf != nullptr && input_buf != nullptr && pcm_buf != nullptr && channel_buffers[0] != nullptr;
		}

		virtual void Release() override {
			uni::trait::Malloc* alloc = allocator;
			this->~FLACStream();
			if (alloc) alloc->deallocate(this);
		}

		virtual uni::AudioResult GetInfo(uni::AudioInfo& outInfo) const override {
			outInfo = info;
			return uni::AudioResult::OK;
		}

		virtual uni::AudioResult ReadSamples(void* destBuffer, uint32 maxBytes, uint32& bytesRead) override {
			bytesRead = 0;
			if (!destBuffer || !maxBytes) return uni::AudioResult::InvalidArgument;

			byte* out_ptr = (byte*)destBuffer;
			uint32 bytes_remaining = maxBytes;

			while (bytes_remaining > 0) {
				if (pcm_sample_pos >= pcm_samples_valid) {
					if (!DecodeNextFrame()) {
						break;
					}
				}

				uint32 avail_samples = pcm_samples_valid - pcm_sample_pos;
				uint32 avail_bytes = avail_samples * sizeof(int16);
				uint32 bytes_to_copy = (avail_bytes < bytes_remaining) ? avail_bytes : bytes_remaining;

				MemCopyN(out_ptr, (const byte*)(pcm_buf + pcm_sample_pos), bytes_to_copy);
				uint32 samples_copied = bytes_to_copy / sizeof(int16);
				pcm_sample_pos += samples_copied;
				out_ptr += bytes_to_copy;
				bytesRead += bytes_to_copy;
				bytes_remaining -= bytes_to_copy;
			}

			return (bytesRead > 0) ? uni::AudioResult::OK : uni::AudioResult::EndOfStream;
		}

		virtual uni::AudioResult Seek(uint32 sampleIndex) override {
			if (details.total_samples > 0 && sampleIndex > details.total_samples) {
				return uni::AudioResult::InvalidArgument;
			}

			uint32 target_offset = details.data_offset;
			if (details.total_samples > 0) {
				uint64 byte_pos = ((uint64)sampleIndex * details.data_size) / details.total_samples;
				target_offset += (uint32)byte_pos;
			}

			if (target_offset > details.data_offset + details.data_size) {
				target_offset = details.data_offset + details.data_size;
			}

			currentByteOffset = target_offset;
			input_buf_valid = 0;
			input_buf_pos = 0;
			pcm_samples_valid = 0;
			pcm_sample_pos = 0;
			cached_block = (stduint)~0;

			return uni::AudioResult::OK;
		}
	};

	static bool ScanFLACMetadata(
		uni::StorageTrait& storage,
		FLAC_STREAMINFO& outStreamInfo,
		uint32& outDataOffset
	) {
		stduint block_size = storage.Block_Size ? storage.Block_Size : 512;
		stduint total_storage_size = storage.getUnits() * block_size;
		if (total_storage_size < 42) return false;

		constexpr uint32 header_scan_size = 16384;
		byte* scan_buf = (byte*)malloc(header_scan_size);
		if (!scan_buf) return false;

		byte* blk = (byte*)malloc(block_size);
		if (!blk) {
			free(scan_buf);
			return false;
		}

		uint32 bytes_to_read = (total_storage_size < header_scan_size) ? (uint32)total_storage_size : header_scan_size;
		uint32 done = 0;
		while (done < bytes_to_read) {
			stduint b_idx = done / block_size;
			stduint b_off = done % block_size;
			stduint chunk = block_size - b_off;
			if (chunk > (bytes_to_read - done)) chunk = bytes_to_read - done;

			if (!storage.Read(b_idx, blk, 1)) break;
			MemCopyN(scan_buf + done, blk + b_off, chunk);
			done += (uint32)chunk;
		}

		free(blk);

		if (done < 42) {
			free(scan_buf);
			return false;
		}

		// Check 'fLaC' magic
		if (scan_buf[0] != 'f' || scan_buf[1] != 'L' || scan_buf[2] != 'a' || scan_buf[3] != 'C') {
			free(scan_buf);
			return false;
		}

		uint32 offset = 4;
		bool found_streaminfo = false;

		while (offset + 4 <= done) {
			uint8 block_header = scan_buf[offset];
			bool is_last = (block_header & 0x80) != 0;
			uint8 block_type = block_header & 0x7F;
			uint32 block_len = ((uint32)scan_buf[offset + 1] << 16) |
			                   ((uint32)scan_buf[offset + 2] << 8)  |
			                   ((uint32)scan_buf[offset + 3]);

			offset += 4;
			if (block_type == 0 && block_len >= 34 && offset + 34 <= done) { // STREAMINFO
				if (parseStreamInfo(scan_buf + offset, block_len, outStreamInfo)) {
					found_streaminfo = true;
				}
			}

			offset += block_len;
			if (is_last) break;
		}

		outDataOffset = offset;
		free(scan_buf);
		return found_streaminfo;
	}

} // anonymous namespace

const char* uni::FLACCodec::GetName() const {
	return "FLAC";
}

uni::AudioContainerFormat uni::FLACCodec::GetFormat() const {
	return uni::AudioContainerFormat::FLAC;
}

const char* const* uni::FLACCodec::GetExtensions() const {
	static const char* const extensions[] = { "flac", "fla", nullptr };
	return extensions;
}

uni::AudioResult uni::FLACCodec::Probe(StorageTrait& storage, bool& matched) const {
	matched = false;
	FLAC_STREAMINFO stream_info{};
	uint32 data_offset = 0;
	if (ScanFLACMetadata(storage, stream_info, data_offset)) {
		matched = true;
	}
	return uni::AudioResult::OK;
}

uni::AudioResult uni::FLACCodec::ReadInfo(StorageTrait& storage, AudioInfo& outInfo) const {
	FLAC_STREAMINFO stream_info{};
	uint32 data_offset = 0;
	if (!ScanFLACMetadata(storage, stream_info, data_offset)) {
		return uni::AudioResult::InvalidFormat;
	}

	stduint block_size = storage.Block_Size ? storage.Block_Size : 512;
	stduint total_storage_size = storage.getUnits() * block_size;
	uint32 data_size = (total_storage_size > data_offset) ? (uint32)(total_storage_size - data_offset) : 0;

	outInfo.format.sample_format = uni::AudioSampleFormat::S16LE;
	outInfo.format.channels = (uint16)stream_info.channels;
	outInfo.format.sample_rate = stream_info.sample_rate;
	outInfo.containerFormat = uni::AudioContainerFormat::FLAC;
	outInfo.bitsPerSample = 16;
	outInfo.dataByteOffset = data_offset;
	outInfo.dataByteLength = data_size;

	if (stream_info.total_samples > 0 && stream_info.sample_rate > 0) {
		outInfo.totalSamples = (uint32)stream_info.total_samples;
		outInfo.durationMs = (uint32)((stream_info.total_samples * 1000) / stream_info.sample_rate);
	} else {
		outInfo.totalSamples = 0;
		outInfo.durationMs = 0;
	}

	return uni::AudioResult::OK;
}

uni::AudioResult uni::FLACCodec::OpenStream(
	StorageTrait& storage,
	IAudioStream*& outStream,
	trait::Malloc& allocator
) const {
	FLAC_STREAMINFO stream_info{};
	uint32 data_offset = 0;
	if (!ScanFLACMetadata(storage, stream_info, data_offset)) {
		return uni::AudioResult::InvalidFormat;
	}

	stduint block_size = storage.Block_Size ? storage.Block_Size : 512;
	stduint total_storage_size = storage.getUnits() * block_size;
	uint32 data_size = (total_storage_size > data_offset) ? (uint32)(total_storage_size - data_offset) : 0;

	FLACFormatDetails details{};
	details.stream_info = stream_info;
	details.data_offset = data_offset;
	details.data_size = data_size;
	if (stream_info.total_samples > 0 && stream_info.sample_rate > 0) {
		details.total_samples = (uint32)stream_info.total_samples;
		details.duration_ms = (uint32)((stream_info.total_samples * 1000) / stream_info.sample_rate);
	}

	AudioInfo info{};
	info.format.sample_format = uni::AudioSampleFormat::S16LE;
	info.format.channels = (uint16)stream_info.channels;
	info.format.sample_rate = stream_info.sample_rate;
	info.containerFormat = uni::AudioContainerFormat::FLAC;
	info.bitsPerSample = 16;
	info.dataByteOffset = data_offset;
	info.dataByteLength = data_size;
	info.durationMs = details.duration_ms;
	info.totalSamples = details.total_samples;

	void* mem = allocator.allocate(sizeof(FLACStream), 3);
	if (!mem) return uni::AudioResult::OutOfMemory;

	FLACStream* stream = new (mem) FLACStream(storage, info, details, allocator);
	if (!stream->isValid()) {
		stream->Release();
		return uni::AudioResult::OutOfMemory;
	}

	outStream = stream;
	return uni::AudioResult::OK;
}

#endif
