// ASCII C/C++ TAB4 CRLF
// Docutitle: MPEG-1 / MPEG-2 Video & Program Stream Codec Implementation
// Attribute: Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0; Dosconio Mecocoa, BSD 3-Clause License

#include "../../../../inc/c/format/video/MPEG.h"
#include "../../../../inc/c/ustring.h"
#include <stdlib.h>

#if defined(_INC_CPP)

namespace {

	static const char* const MPEG_EXTENSIONS[] = { ".mpg", ".mpeg", ".m1v", ".m2v", ".vob", ".dat", nullptr };

	static const struct FrameRateEntry {
		uint32 num;
		uint32 den;
	} s_mpeg_frame_rates[16] = {
		{ 0, 1 },
		{ 24000, 1001 }, // 1: 23.976 fps
		{ 24, 1 },       // 2: 24.0 fps
		{ 25, 1 },       // 3: 25.0 fps
		{ 30000, 1001 }, // 4: 29.97 fps
		{ 30, 1 },       // 5: 30.0 fps
		{ 50, 1 },       // 6: 50.0 fps
		{ 60000, 1001 }, // 7: 59.94 fps
		{ 60, 1 },       // 8: 60.0 fps
		{ 0, 1 }, { 0, 1 }, { 0, 1 }, { 0, 1 }, { 0, 1 }, { 0, 1 }, { 0, 1 }
	};

	static const uint8 s_default_intra_quant[64] = {
		 8, 16, 19, 22, 26, 27, 29, 34,
		16, 16, 22, 24, 27, 29, 34, 37,
		19, 22, 26, 27, 29, 34, 34, 38,
		22, 22, 26, 27, 29, 34, 37, 40,
		22, 26, 27, 29, 32, 35, 40, 48,
		26, 27, 29, 32, 35, 40, 48, 58,
		26, 27, 29, 34, 38, 46, 56, 69,
		27, 29, 35, 38, 46, 56, 69, 83
	};

	static const uint8 s_zigzag_scan[64] = {
		 0,  1,  8, 16,  9,  2,  3, 10,
		17, 24, 32, 25, 18, 11,  4,  5,
		12, 19, 26, 33, 40, 48, 41, 34,
		27, 20, 13,  6,  7, 14, 21, 28,
		35, 42, 49, 56, 57, 50, 43, 36,
		29, 22, 15, 23, 30, 37, 44, 51,
		58, 59, 52, 45, 38, 31, 39, 46,
		53, 60, 61, 54, 47, 55, 62, 63
	};

	static inline int Clamp255(int v) {
		if (v < 0) return 0;
		if (v > 255) return 255;
		return v;
	}

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

	static DefaultHeapAllocator s_mpeg_allocator;

	class MemoryStorageDevice : public uni::StorageTrait {
	private:
		const byte* m_data;
		stduint     m_size;

	public:
		MemoryStorageDevice(const byte* data, stduint size)
			: m_data(data), m_size(size) {
			Block_Size = 4096;
			readable = true;
			writable = false;
		}

		virtual bool Read(stduint BlockIden, void* Dest, stduint Times = 1) override {
			if (BlockIden + Times > getUnits()) return false;
			stduint off = BlockIden * Block_Size;
			stduint len = Times * Block_Size;
			if (off + len > m_size) len = m_size - off;
			MemCopyN(Dest, m_data + off, len);
			return true;
		}

		virtual bool Write(stduint BlockIden, const void* Sors, stduint Times = 1) override {
			return false;
		}

		virtual stduint getUnits() override {
			return (m_size + Block_Size - 1) / Block_Size;
		}

		virtual int operator[](uint64 bytid) override {
			if (bytid >= m_size) return -1;
			return m_data[bytid];
		}
	};

	// 2D Inverse Discrete Cosine Transform (Fixed-point IEEE-1180 precision)
	static void Compute2D_IDCT(int* block) {
		static const int cos_tab[8][8] = {
			{  724,  1004,   946,   851,   724,   569,   392,   200 },
			{  724,   851,   392,  -200,  -724, -1004,  -946,  -569 },
			{  724,   569,  -392, -1004,  -724,   200,   946,   851 },
			{  724,   200,  -946,  -569,   724,   851,  -392, -1004 },
			{  724,  -200,  -946,   569,   724,  -851,  -392,  1004 },
			{  724,  -569,  -392,  1004,  -724,  -200,   946,  -851 },
			{  724,  -851,   392,   200,  -724,  1004,  -946,   569 },
			{  724, -1004,   946,  -851,   724,  -569,   392,  -200 }
		};

		int temp[64];
		// Row 1D-IDCT
		for (int i = 0; i < 8; ++i) {
			const int* in = block + i * 8;
			int* out = temp + i * 8;
			for (int j = 0; j < 8; ++j) {
				int sum = 0;
				for (int k = 0; k < 8; ++k) {
					sum += in[k] * cos_tab[j][k];
				}
				out[j] = (sum + 1024) >> 11;
			}
		}

		// Col 1D-IDCT
		for (int j = 0; j < 8; ++j) {
			for (int i = 0; i < 8; ++i) {
				int sum = 0;
				for (int k = 0; k < 8; ++k) {
					sum += temp[k * 8 + j] * cos_tab[i][k];
				}
				block[i * 8 + j] = (sum + 1024) >> 11;
			}
		}
	}

	// Bitstream Buffer Helper
	class MPEGBitReader {
	private:
		const byte* data;
		size_t      size;
		size_t      byte_pos;
		uint64      bit_buf;
		int         bits_left;

	public:
		MPEGBitReader(const byte* src, size_t len)
			: data(src), size(len), byte_pos(0), bit_buf(0), bits_left(0) {
		}

		inline bool HasData() const {
			return byte_pos < size || bits_left > 0;
		}

		inline int GetBitsLeft() const {
			return bits_left;
		}

		inline size_t GetBytePos() const {
			return (bits_left >= 8) ? (byte_pos - (bits_left / 8)) : byte_pos;
		}

		inline void FillBits() {
			while (bits_left <= 56 && byte_pos < size) {
				bit_buf = (bit_buf << 8) | (uint64)data[byte_pos++];
				bits_left += 8;
			}
		}

		inline void ByteAlign() {
			int discard = bits_left % 8;
			if (discard > 0) {
				SkipBits(discard);
			}
		}

		inline uint32 ShowBits(int n) {
			FillBits();
			uint64 mask = (n >= 64) ? ~0ULL : ((1ULL << n) - 1ULL);
			if (bits_left < n) {
				return (uint32)((bit_buf << (n - bits_left)) & mask);
			}
			return (uint32)((bit_buf >> (bits_left - n)) & mask);
		}

		inline void SkipBits(int n) {
			if (bits_left < n) FillBits();
			if (bits_left >= n) {
				bits_left -= n;
				if (bits_left < 64) {
					bit_buf &= ((1ULL << bits_left) - 1ULL);
				}
			} else {
				bits_left = 0;
				bit_buf = 0;
			}
		}

		inline uint32 ReadBits(int n) {
			uint32 val = ShowBits(n);
			SkipBits(n);
			return val;
		}

		inline uint32 ReadBit() {
			return ReadBits(1);
		}

		bool NextStartCode(uint32& outCode) {
			ByteAlign();
			while (bits_left >= 8) {
				byte_pos--;
				bits_left -= 8;
			}
			bit_buf = 0;
			bits_left = 0;
			while (byte_pos + 3 < size) {
				if (data[byte_pos] == 0x00 && data[byte_pos + 1] == 0x00 && data[byte_pos + 2] == 0x01) {
					outCode = ((uint32)data[byte_pos] << 24) |
							  ((uint32)data[byte_pos + 1] << 16) |
							  ((uint32)data[byte_pos + 2] << 8) |
							  ((uint32)data[byte_pos + 3]);
					byte_pos += 4;
					return true;
				}
				byte_pos++;
			}
			return false;
		}
	};

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

	// -------------------------------------------------------------
	// MPEG-1 Video VLC Decoders
	// ISO/IEC 11172-2 Table B.1/B.2/B.9/B.10/B.12/B.13/B.14
	// -------------------------------------------------------------

	// VLC entry: value meaning depends on the table (see the comment above each one)
	struct MPEGVLC { uint16 code; uint8 len; uint8 value; };

	// Table B.1: value = skipped macroblocks (code 1..5 bits)
	static const MPEGVLC s_mba_5[] = {
		{ 0x02,	5,	6 }, { 0x03,	5,	5 }, { 0x02,	4,	4 }, { 0x02,	4,	4 }, { 0x03,	4,	3 }, { 0x03,	4,	3 },
		{ 0x02,	3,	2 }, { 0x02,	3,	2 }, { 0x02,	3,	2 }, { 0x02,	3,	2 }, { 0x03,	3,	1 }, { 0x03,	3,	1 },
		{ 0x03,	3,	1 }, { 0x03,	3,	1 }, { 0x01,	1,	0 }, { 0x01,	1,	0 }, { 0x01,	1,	0 }, { 0x01,	1,	0 },
		{ 0x01,	1,	0 }, { 0x01,	1,	0 }, { 0x01,	1,	0 }, { 0x01,	1,	0 }, { 0x01,	1,	0 }, { 0x01,	1,	0 },
		{ 0x01,	1,	0 }, { 0x01,	1,	0 }, { 0x01,	1,	0 }, { 0x01,	1,	0 }, { 0x01,	1,	0 }, { 0x01,	1,	0 },
	};
	// Table B.1 continued: value = skipped macroblocks
	static const MPEGVLC s_mba_11[] = {
		{ 0x0018,	11,	32 }, { 0x0019,	11,	31 }, { 0x001A,	11,	30 }, { 0x001B,	11,	29 },
		{ 0x001C,	11,	28 }, { 0x001D,	11,	27 }, { 0x001E,	11,	26 }, { 0x001F,	11,	25 },
		{ 0x0020,	11,	24 }, { 0x0021,	11,	23 }, { 0x0022,	11,	22 }, { 0x0023,	11,	21 },
		{ 0x0012,	10,	20 }, { 0x0013,	10,	19 }, { 0x0014,	10,	18 }, { 0x0015,	10,	17 },
		{ 0x0016,	10,	16 }, { 0x0017,	10,	15 }, { 0x0006,	8,	14 }, { 0x0007,	8,	13 },
		{ 0x0008,	8,	12 }, { 0x0009,	8,	11 }, { 0x000A,	8,	10 }, { 0x000B,	8,	9 },
		{ 0x0006,	7,	8 }, { 0x0007,	7,	7 },
	};
	// Table B.2a/b/c: value = 0x01 intra, 0x02 pattern, 0x04 forward, 0x08 backward, 0x10 quant
	// Table B.2a: I-picture macroblock_type
	static const MPEGVLC s_mb_i[] = {
		{ 0x01,	2,	0x11 }, { 0x01,	1,	0x01 },
	};
	// Table B.2b: P-picture macroblock_type
	static const MPEGVLC s_mb_p[] = {
		{ 0x01,	6,	0x11 }, { 0x01,	5,	0x12 }, { 0x02,	5,	0x16 }, { 0x03,	5,	0x01 },
		{ 0x01,	3,	0x04 }, { 0x01,	2,	0x02 }, { 0x01,	1,	0x06 },
	};
	// Table B.2c: B-picture macroblock_type
	static const MPEGVLC s_mb_b[] = {
		{ 0x00,	6,	0x00 }, { 0x01,	6,	0x11 }, { 0x02,	6,	0x1A }, { 0x03,	6,	0x16 },
		{ 0x02,	5,	0x1E }, { 0x03,	5,	0x01 }, { 0x02,	4,	0x04 }, { 0x03,	4,	0x06 },
		{ 0x02,	3,	0x08 }, { 0x03,	3,	0x0A }, { 0x02,	2,	0x0C }, { 0x03,	2,	0x0E },
	};
	// Table B.10: motion vectors, indexed by the leading bits just like the reference
	// decoder. delta is the motion code magnitude minus one, len its code length; the
	// sign bit and the f_code residual bits follow the code.
	struct MPEGMV { uint8 delta; uint8 len; };
	static const MPEGMV s_mv_4[] = {
		{ 3,	6 }, { 2,	4 }, { 1,	3 }, { 1,	3 },
		{ 0,	2 }, { 0,	2 }, { 0,	2 }, { 0,	2 }
	};
	static const MPEGMV s_mv_10[] = {
		{ 0,	10 }, { 0,	10 }, { 0,	10 }, { 0,	10 }, { 0,	10 }, { 0,	10 },
		{ 0,	10 }, { 0,	10 }, { 0,	10 }, { 0,	10 }, { 0,	10 }, { 0,	10 },
		{ 15,	10 }, { 14,	10 }, { 13,	10 }, { 12,	10 }, { 11,	10 }, { 10,	10 },
		{ 9,	9 }, { 9,	9 }, { 8,	9 }, { 8,	9 }, { 7,	9 }, { 7,	9 },
		{ 6,	7 }, { 6,	7 }, { 6,	7 }, { 6,	7 }, { 6,	7 }, { 6,	7 },
		{ 6,	7 }, { 6,	7 }, { 5,	7 }, { 5,	7 }, { 5,	7 }, { 5,	7 },
		{ 5,	7 }, { 5,	7 }, { 5,	7 }, { 5,	7 }, { 4,	7 }, { 4,	7 },
		{ 4,	7 }, { 4,	7 }, { 4,	7 }, { 4,	7 }, { 4,	7 }, { 4,	7 }
	};
	// Table B.9: value = coded block pattern (bit0 Y0 .. bit5 Cr)
	static const MPEGVLC s_cbp[] = {
		{ 0x00,	9,	0x00 }, { 0x01,	9,	0x00 }, { 0x02,	9,	0x39 }, { 0x03,	9,	0x36 }, { 0x04,	9,	0x37 }, { 0x05,	9,	0x3B },
		{ 0x06,	9,	0x3D }, { 0x07,	9,	0x3E }, { 0x04,	8,	0x17 }, { 0x05,	8,	0x1B }, { 0x06,	8,	0x1D }, { 0x07,	8,	0x1E },
		{ 0x08,	8,	0x27 }, { 0x09,	8,	0x2B }, { 0x0A,	8,	0x2D }, { 0x0B,	8,	0x2E }, { 0x0C,	8,	0x19 }, { 0x0D,	8,	0x16 },
		{ 0x0E,	8,	0x29 }, { 0x0F,	8,	0x26 }, { 0x10,	8,	0x35 }, { 0x11,	8,	0x3A }, { 0x12,	8,	0x33 }, { 0x13,	8,	0x3C },
		{ 0x14,	8,	0x15 }, { 0x15,	8,	0x1A }, { 0x16,	8,	0x13 }, { 0x17,	8,	0x1C }, { 0x18,	8,	0x25 }, { 0x19,	8,	0x2A },
		{ 0x1A,	8,	0x23 }, { 0x1B,	8,	0x2C }, { 0x1C,	8,	0x31 }, { 0x1D,	8,	0x32 }, { 0x1E,	8,	0x34 }, { 0x1F,	8,	0x38 },
		{ 0x10,	7,	0x11 }, { 0x11,	7,	0x12 }, { 0x12,	7,	0x14 }, { 0x13,	7,	0x18 }, { 0x14,	7,	0x21 }, { 0x15,	7,	0x22 },
		{ 0x16,	7,	0x24 }, { 0x17,	7,	0x28 }, { 0x0C,	6,	0x3F }, { 0x0D,	6,	0x30 }, { 0x0E,	6,	0x09 }, { 0x0F,	6,	0x06 },
		{ 0x08,	5,	0x1F }, { 0x09,	5,	0x10 }, { 0x0A,	5,	0x2F }, { 0x0B,	5,	0x20 }, { 0x0C,	5,	0x07 }, { 0x0D,	5,	0x0B },
		{ 0x0E,	5,	0x0D }, { 0x0F,	5,	0x0E }, { 0x10,	5,	0x05 }, { 0x11,	5,	0x0A }, { 0x12,	5,	0x03 }, { 0x13,	5,	0x0C },
		{ 0x0A,	4,	0x01 }, { 0x0B,	4,	0x02 }, { 0x0C,	4,	0x04 }, { 0x0D,	4,	0x08 }, { 0x07,	3,	0x0F },
	};
	// Table B.12: value = dct_dc_size_luminance
	static const MPEGVLC s_dc_lum[] = {
		{ 0x000,	2,	1 }, { 0x001,	2,	2 }, { 0x004,	3,	0 }, { 0x005,	3,	3 },
		{ 0x006,	3,	4 }, { 0x00E,	4,	5 }, { 0x01E,	5,	6 }, { 0x03E,	6,	7 },
		{ 0x07E,	7,	8 }, { 0x0FE,	8,	9 }, { 0x1FE,	9,	10 }, { 0x1FF,	9,	11 },
	};
	// Table B.13: value = dct_dc_size_chrominance
	static const MPEGVLC s_dc_chrom[] = {
		{ 0x000,	2,	0 }, { 0x001,	2,	1 }, { 0x002,	2,	2 }, { 0x006,	3,	3 },
		{ 0x00E,	4,	4 }, { 0x01E,	5,	5 }, { 0x03E,	6,	6 }, { 0x07E,	7,	7 },
		{ 0x0FE,	8,	8 }, { 0x1FE,	9,	9 }, { 0x3FE,	10,	10 }, { 0x3FF,	10,	11 },
	};
	// Table B.14: DCT coefficients; run = zero coefficients before it,
	// eob != 0 means end of block, else a sign bit follows the code
	struct MPEGDCT { uint16 code; uint8 len; uint8 eob; uint8 run; int8 level; };
	// next AC coefficient of a block
	static const MPEGDCT s_dct_next[] = {
		{ 0x0000,	16,	1,	0 ,	  0 }, { 0x0001,	16,	1,	0 ,	  0 }, { 0x0002,	16,	1,	0 ,	  0 },
		{ 0x0003,	16,	1,	0 ,	  0 }, { 0x0004,	16,	1,	0 ,	  0 }, { 0x0005,	16,	1,	0 ,	  0 },
		{ 0x0006,	16,	1,	0 ,	  0 }, { 0x0007,	16,	1,	0 ,	  0 }, { 0x0008,	16,	1,	0 ,	  0 },
		{ 0x0009,	16,	1,	0 ,	  0 }, { 0x000A,	16,	1,	0 ,	  0 }, { 0x000B,	16,	1,	0 ,	  0 },
		{ 0x000C,	16,	1,	0 ,	  0 }, { 0x000D,	16,	1,	0 ,	  0 }, { 0x000E,	16,	1,	0 ,	  0 },
		{ 0x000F,	16,	1,	0 ,	  0 }, { 0x0010,	16,	0,	1 ,	 18 }, { 0x0011,	16,	0,	1 ,	 17 },
		{ 0x0012,	16,	0,	1 ,	 16 }, { 0x0013,	16,	0,	1 ,	 15 }, { 0x0014,	16,	0,	6 ,	  3 },
		{ 0x0015,	16,	0,	16,	  2 }, { 0x0016,	16,	0,	15,	  2 }, { 0x0017,	16,	0,	14,	  2 },
		{ 0x0018,	16,	0,	13,	  2 }, { 0x0019,	16,	0,	12,	  2 }, { 0x001A,	16,	0,	11,	  2 },
		{ 0x001B,	16,	0,	31,	  1 }, { 0x001C,	16,	0,	30,	  1 }, { 0x001D,	16,	0,	29,	  1 },
		{ 0x001E,	16,	0,	28,	  1 }, { 0x001F,	16,	0,	27,	  1 }, { 0x0010,	15,	0,	0 ,	 40 },
		{ 0x0011,	15,	0,	0 ,	 39 }, { 0x0012,	15,	0,	0 ,	 38 }, { 0x0013,	15,	0,	0 ,	 37 },
		{ 0x0014,	15,	0,	0 ,	 36 }, { 0x0015,	15,	0,	0 ,	 35 }, { 0x0016,	15,	0,	0 ,	 34 },
		{ 0x0017,	15,	0,	0 ,	 33 }, { 0x0018,	15,	0,	0 ,	 32 }, { 0x0019,	15,	0,	1 ,	 14 },
		{ 0x001A,	15,	0,	1 ,	 13 }, { 0x001B,	15,	0,	1 ,	 12 }, { 0x001C,	15,	0,	1 ,	 11 },
		{ 0x001D,	15,	0,	1 ,	 10 }, { 0x001E,	15,	0,	1 ,	  9 }, { 0x001F,	15,	0,	1 ,	  8 },
		{ 0x0010,	14,	0,	0 ,	 31 }, { 0x0011,	14,	0,	0 ,	 30 }, { 0x0012,	14,	0,	0 ,	 29 },
		{ 0x0013,	14,	0,	0 ,	 28 }, { 0x0014,	14,	0,	0 ,	 27 }, { 0x0015,	14,	0,	0 ,	 26 },
		{ 0x0016,	14,	0,	0 ,	 25 }, { 0x0017,	14,	0,	0 ,	 24 }, { 0x0018,	14,	0,	0 ,	 23 },
		{ 0x0019,	14,	0,	0 ,	 22 }, { 0x001A,	14,	0,	0 ,	 21 }, { 0x001B,	14,	0,	0 ,	 20 },
		{ 0x001C,	14,	0,	0 ,	 19 }, { 0x001D,	14,	0,	0 ,	 18 }, { 0x001E,	14,	0,	0 ,	 17 },
		{ 0x001F,	14,	0,	0 ,	 16 }, { 0x0010,	13,	0,	10,	  2 }, { 0x0011,	13,	0,	9 ,	  2 },
		{ 0x0012,	13,	0,	5 ,	  3 }, { 0x0013,	13,	0,	3 ,	  4 }, { 0x0014,	13,	0,	2 ,	  5 },
		{ 0x0015,	13,	0,	1 ,	  7 }, { 0x0016,	13,	0,	1 ,	  6 }, { 0x0017,	13,	0,	0 ,	 15 },
		{ 0x0018,	13,	0,	0 ,	 14 }, { 0x0019,	13,	0,	0 ,	 13 }, { 0x001A,	13,	0,	0 ,	 12 },
		{ 0x001B,	13,	0,	26,	  1 }, { 0x001C,	13,	0,	25,	  1 }, { 0x001D,	13,	0,	24,	  1 },
		{ 0x001E,	13,	0,	23,	  1 }, { 0x001F,	13,	0,	22,	  1 }, { 0x0010,	12,	0,	0 ,	 11 },
		{ 0x0011,	12,	0,	8 ,	  2 }, { 0x0012,	12,	0,	4 ,	  3 }, { 0x0013,	12,	0,	0 ,	 10 },
		{ 0x0014,	12,	0,	2 ,	  4 }, { 0x0015,	12,	0,	7 ,	  2 }, { 0x0016,	12,	0,	21,	  1 },
		{ 0x0017,	12,	0,	20,	  1 }, { 0x0018,	12,	0,	0 ,	  9 }, { 0x0019,	12,	0,	19,	  1 },
		{ 0x001A,	12,	0,	18,	  1 }, { 0x001B,	12,	0,	1 ,	  5 }, { 0x001C,	12,	0,	3 ,	  3 },
		{ 0x001D,	12,	0,	0 ,	  8 }, { 0x001E,	12,	0,	6 ,	  2 }, { 0x001F,	12,	0,	17,	  1 },
		{ 0x0008,	10,	0,	16,	  1 }, { 0x0009,	10,	0,	5 ,	  2 }, { 0x000A,	10,	0,	0 ,	  7 },
		{ 0x000B,	10,	0,	2 ,	  3 }, { 0x000C,	10,	0,	1 ,	  4 }, { 0x000D,	10,	0,	15,	  1 },
		{ 0x000E,	10,	0,	14,	  1 }, { 0x000F,	10,	0,	4 ,	  2 }, { 0x0020,	8 ,	0,	13,	  1 },
		{ 0x0021,	8 ,	0,	0 ,	  6 }, { 0x0022,	8 ,	0,	12,	  1 }, { 0x0023,	8 ,	0,	11,	  1 },
		{ 0x0024,	8 ,	0,	3 ,	  2 }, { 0x0025,	8 ,	0,	1 ,	  3 }, { 0x0026,	8 ,	0,	0 ,	  5 },
		{ 0x0027,	8 ,	0,	10,	  1 }, { 0x0004,	7 ,	0,	2 ,	  2 }, { 0x0005,	7 ,	0,	9 ,	  1 },
		{ 0x0006,	7 ,	0,	0 ,	  4 }, { 0x0007,	7 ,	0,	8 ,	  1 }, { 0x0004,	6 ,	0,	7 ,	  1 },
		{ 0x0005,	6 ,	0,	6 ,	  1 }, { 0x0006,	6 ,	0,	1 ,	  2 }, { 0x0007,	6 ,	0,	5 ,	  1 },
		{ 0x0005,	5 ,	0,	0 ,	  3 }, { 0x0006,	5 ,	0,	4 ,	  1 }, { 0x0007,	5 ,	0,	3 ,	  1 },
		{ 0x0004,	4 ,	0,	0 ,	  2 }, { 0x0005,	4 ,	0,	2 ,	  1 }, { 0x0003,	3 ,	0,	1 ,	  1 },
		{ 0x0002,	2 ,	1,	0 ,	  0 }, { 0x0003,	2 ,	0,	0 ,	  1 },
	};
	// first coefficient of a non-intra block
	static const MPEGDCT s_dct_first[] = {
		{ 0x0000,	16,	1,	0 ,	  0 }, { 0x0001,	16,	1,	0 ,	  0 }, { 0x0002,	16,	1,	0 ,	  0 },
		{ 0x0003,	16,	1,	0 ,	  0 }, { 0x0004,	16,	1,	0 ,	  0 }, { 0x0005,	16,	1,	0 ,	  0 },
		{ 0x0006,	16,	1,	0 ,	  0 }, { 0x0007,	16,	1,	0 ,	  0 }, { 0x0008,	16,	1,	0 ,	  0 },
		{ 0x0009,	16,	1,	0 ,	  0 }, { 0x000A,	16,	1,	0 ,	  0 }, { 0x000B,	16,	1,	0 ,	  0 },
		{ 0x000C,	16,	1,	0 ,	  0 }, { 0x000D,	16,	1,	0 ,	  0 }, { 0x000E,	16,	1,	0 ,	  0 },
		{ 0x000F,	16,	1,	0 ,	  0 }, { 0x0010,	16,	0,	1 ,	 18 }, { 0x0011,	16,	0,	1 ,	 17 },
		{ 0x0012,	16,	0,	1 ,	 16 }, { 0x0013,	16,	0,	1 ,	 15 }, { 0x0014,	16,	0,	6 ,	  3 },
		{ 0x0015,	16,	0,	16,	  2 }, { 0x0016,	16,	0,	15,	  2 }, { 0x0017,	16,	0,	14,	  2 },
		{ 0x0018,	16,	0,	13,	  2 }, { 0x0019,	16,	0,	12,	  2 }, { 0x001A,	16,	0,	11,	  2 },
		{ 0x001B,	16,	0,	31,	  1 }, { 0x001C,	16,	0,	30,	  1 }, { 0x001D,	16,	0,	29,	  1 },
		{ 0x001E,	16,	0,	28,	  1 }, { 0x001F,	16,	0,	27,	  1 }, { 0x0010,	15,	0,	0 ,	 40 },
		{ 0x0011,	15,	0,	0 ,	 39 }, { 0x0012,	15,	0,	0 ,	 38 }, { 0x0013,	15,	0,	0 ,	 37 },
		{ 0x0014,	15,	0,	0 ,	 36 }, { 0x0015,	15,	0,	0 ,	 35 }, { 0x0016,	15,	0,	0 ,	 34 },
		{ 0x0017,	15,	0,	0 ,	 33 }, { 0x0018,	15,	0,	0 ,	 32 }, { 0x0019,	15,	0,	1 ,	 14 },
		{ 0x001A,	15,	0,	1 ,	 13 }, { 0x001B,	15,	0,	1 ,	 12 }, { 0x001C,	15,	0,	1 ,	 11 },
		{ 0x001D,	15,	0,	1 ,	 10 }, { 0x001E,	15,	0,	1 ,	  9 }, { 0x001F,	15,	0,	1 ,	  8 },
		{ 0x0010,	14,	0,	0 ,	 31 }, { 0x0011,	14,	0,	0 ,	 30 }, { 0x0012,	14,	0,	0 ,	 29 },
		{ 0x0013,	14,	0,	0 ,	 28 }, { 0x0014,	14,	0,	0 ,	 27 }, { 0x0015,	14,	0,	0 ,	 26 },
		{ 0x0016,	14,	0,	0 ,	 25 }, { 0x0017,	14,	0,	0 ,	 24 }, { 0x0018,	14,	0,	0 ,	 23 },
		{ 0x0019,	14,	0,	0 ,	 22 }, { 0x001A,	14,	0,	0 ,	 21 }, { 0x001B,	14,	0,	0 ,	 20 },
		{ 0x001C,	14,	0,	0 ,	 19 }, { 0x001D,	14,	0,	0 ,	 18 }, { 0x001E,	14,	0,	0 ,	 17 },
		{ 0x001F,	14,	0,	0 ,	 16 }, { 0x0010,	13,	0,	10,	  2 }, { 0x0011,	13,	0,	9 ,	  2 },
		{ 0x0012,	13,	0,	5 ,	  3 }, { 0x0013,	13,	0,	3 ,	  4 }, { 0x0014,	13,	0,	2 ,	  5 },
		{ 0x0015,	13,	0,	1 ,	  7 }, { 0x0016,	13,	0,	1 ,	  6 }, { 0x0017,	13,	0,	0 ,	 15 },
		{ 0x0018,	13,	0,	0 ,	 14 }, { 0x0019,	13,	0,	0 ,	 13 }, { 0x001A,	13,	0,	0 ,	 12 },
		{ 0x001B,	13,	0,	26,	  1 }, { 0x001C,	13,	0,	25,	  1 }, { 0x001D,	13,	0,	24,	  1 },
		{ 0x001E,	13,	0,	23,	  1 }, { 0x001F,	13,	0,	22,	  1 }, { 0x0010,	12,	0,	0 ,	 11 },
		{ 0x0011,	12,	0,	8 ,	  2 }, { 0x0012,	12,	0,	4 ,	  3 }, { 0x0013,	12,	0,	0 ,	 10 },
		{ 0x0014,	12,	0,	2 ,	  4 }, { 0x0015,	12,	0,	7 ,	  2 }, { 0x0016,	12,	0,	21,	  1 },
		{ 0x0017,	12,	0,	20,	  1 }, { 0x0018,	12,	0,	0 ,	  9 }, { 0x0019,	12,	0,	19,	  1 },
		{ 0x001A,	12,	0,	18,	  1 }, { 0x001B,	12,	0,	1 ,	  5 }, { 0x001C,	12,	0,	3 ,	  3 },
		{ 0x001D,	12,	0,	0 ,	  8 }, { 0x001E,	12,	0,	6 ,	  2 }, { 0x001F,	12,	0,	17,	  1 },
		{ 0x0008,	10,	0,	16,	  1 }, { 0x0009,	10,	0,	5 ,	  2 }, { 0x000A,	10,	0,	0 ,	  7 },
		{ 0x000B,	10,	0,	2 ,	  3 }, { 0x000C,	10,	0,	1 ,	  4 }, { 0x000D,	10,	0,	15,	  1 },
		{ 0x000E,	10,	0,	14,	  1 }, { 0x000F,	10,	0,	4 ,	  2 }, { 0x0020,	8 ,	0,	13,	  1 },
		{ 0x0021,	8 ,	0,	0 ,	  6 }, { 0x0022,	8 ,	0,	12,	  1 }, { 0x0023,	8 ,	0,	11,	  1 },
		{ 0x0024,	8 ,	0,	3 ,	  2 }, { 0x0025,	8 ,	0,	1 ,	  3 }, { 0x0026,	8 ,	0,	0 ,	  5 },
		{ 0x0027,	8 ,	0,	10,	  1 }, { 0x0004,	7 ,	0,	2 ,	  2 }, { 0x0005,	7 ,	0,	9 ,	  1 },
		{ 0x0006,	7 ,	0,	0 ,	  4 }, { 0x0007,	7 ,	0,	8 ,	  1 }, { 0x0004,	6 ,	0,	7 ,	  1 },
		{ 0x0005,	6 ,	0,	6 ,	  1 }, { 0x0006,	6 ,	0,	1 ,	  2 }, { 0x0007,	6 ,	0,	5 ,	  1 },
		{ 0x0005,	5 ,	0,	0 ,	  3 }, { 0x0006,	5 ,	0,	4 ,	  1 }, { 0x0007,	5 ,	0,	3 ,	  1 },
		{ 0x0004,	4 ,	0,	0 ,	  2 }, { 0x0005,	4 ,	0,	2 ,	  1 }, { 0x0003,	3 ,	0,	1 ,	  1 },
		{ 0x0001,	1 ,	0,	0 ,	  1 },
	};

	// Shortest code first: the VLC tables are prefix free, so a stored long code can
	// otherwise swallow the first bits of a shorter code plus whatever follows it.
	static int VLCSearch(MPEGBitReader& reader, const MPEGVLC* table, stduint count) {
		for (int len = 1; len <= 16; ++len) {
			for (stduint i = 0; i < count; ++i) {
				if (table[i].len == len && reader.ShowBits(len) == table[i].code) {
					return (int)i;
				}
			}
		}
		return -1;
	}

	// The coefficient codes are prefix free, so the shortest matching code is the
	// only correct one. Scanning the table in stored order would let a long 16 bit
	// entry match the first bits of a shorter code plus the data that follows it.
	static int DCTSearch(MPEGBitReader& reader, const MPEGDCT* table, stduint count) {
		for (int len = 1; len <= 16; ++len) {
			for (stduint i = 0; i < count; ++i) {
				if (table[i].len == len && reader.ShowBits(len) == table[i].code) {
					return (int)i;
				}
			}
		}
		return -1;
	}

	// Table B.1: return the macroblock address increment, 0 with end_of_slice set
	static int DecodeMBAI(MPEGBitReader& reader, bool& end_of_slice) {
		end_of_slice = false;
		int mbai = 0;
		while (true) {
			int i;
			if (reader.ShowBits(32) >= 0x10000000) {
				i = VLCSearch(reader, s_mba_5, sizeof(s_mba_5) / sizeof(MPEGVLC));
				if (i < 0) break;
				reader.SkipBits(s_mba_5[i].len);
				return mbai + s_mba_5[i].value + 1;
			} else if (reader.ShowBits(32) >= 0x03000000) {
				i = VLCSearch(reader, s_mba_11, sizeof(s_mba_11) / sizeof(MPEGVLC));
				if (i < 0) break;
				reader.SkipBits(s_mba_11[i].len);
				return mbai + s_mba_11[i].value + 1;
			} else if (reader.ShowBits(11) == 0x008) { // macroblock_escape
				mbai += 33;
				reader.SkipBits(11);
			} else if (reader.ShowBits(11) == 0x00F) { // macroblock_stuffing
				reader.SkipBits(11);
			} else {
				end_of_slice = true;
				return 0;
			}
		}
		end_of_slice = true;
		return 0;
	}

	// Table B.2a/b/c: Macroblock Type
	struct MBTypeDec {
		uint8 intra;
		uint8 pattern;
		uint8 backward;
		uint8 forward;
		uint8 quant;
	};

	static MBTypeDec DecodeMBType(MPEGBitReader& reader, uint8 pic_type) {
		MBTypeDec res = { 0, 0, 0, 0, 0 };
		const MPEGVLC* table = s_mb_p;
		stduint count = sizeof(s_mb_p) / sizeof(MPEGVLC);
		if (pic_type == MPEG_PICTURE_TYPE_I) {
			table = s_mb_i;
			count = sizeof(s_mb_i) / sizeof(MPEGVLC);
		} else if (pic_type != MPEG_PICTURE_TYPE_P) {
			table = s_mb_b;
			count = sizeof(s_mb_b) / sizeof(MPEGVLC);
		}
		int i = VLCSearch(reader, table, count);
		if (i < 0) return res;
		reader.SkipBits(table[i].len);
		uint8 modes = table[i].value;
		res.intra = (modes & 0x01) ? 1 : 0;
		res.pattern = (modes & 0x02) ? 1 : 0;
		res.forward = res.intra ? 0 : ((modes & 0x04) ? 1 : 0);
		res.backward = res.intra ? 0 : ((modes & 0x08) ? 1 : 0);
		res.quant = (modes & 0x10) ? 1 : 0;
		return res;
	}

	// Table B.9: Coded Block Pattern
	static int DecodeCBP(MPEGBitReader& reader) {
		int i = VLCSearch(reader, s_cbp, sizeof(s_cbp) / sizeof(MPEGVLC));
		if (i < 0) {
			reader.SkipBits(1);
			return 0;
		}
		reader.SkipBits(s_cbp[i].len);
		return s_cbp[i].value;
	}

	// Table B.10: motion_code. The reference decoder indexes the table with the
	// leading zero still in the buffer, so the lookup has to peek that bit instead
	// of consuming it: consuming it shifts the index by one and lands past the end
	// of the four bit table.
	static int DecodeMotionCode(MPEGBitReader& reader, int r_size) {
		if (reader.ShowBits(1)) {		// motion_code 0 is the single bit "1"
			reader.SkipBits(1);
			return 0;
		}
		const MPEGMV* entry;
		if (reader.ShowBits(32) >= 0x0C000000) {
			entry = &s_mv_4[reader.ShowBits(4)];
		} else {
			entry = &s_mv_10[reader.ShowBits(10)];
		}
		reader.SkipBits(entry->len);
		bool negative = reader.ReadBit() != 0;
		int residual = r_size > 0 ? (int)reader.ReadBits(r_size) : 0;
		int value = ((int)entry->delta << r_size) + 1 + residual;
		return negative ? -value : value;
	}
	static void DecodeMotionVector(MPEGBitReader& reader, int& pmv, int f_code) {
		pmv += DecodeMotionCode(reader, f_code > 0 ? f_code - 1 : 0);
		int range = 1 << (f_code + 4);
		if (pmv < -range) pmv += 2 * range;
		if (pmv > range - 1) pmv -= 2 * range;
	}

	// Table B.12/B.13: DC size
	static int DecodeDCSize(MPEGBitReader& reader, bool is_luma) {
		const MPEGVLC* table = is_luma ? s_dc_lum : s_dc_chrom;
		stduint count = is_luma ? (sizeof(s_dc_lum) / sizeof(MPEGVLC)) : (sizeof(s_dc_chrom) / sizeof(MPEGVLC));
		int i = VLCSearch(reader, table, count);
		if (i < 0) {
			reader.SkipBits(1);
			return 0;
		}
		reader.SkipBits(table[i].len);
		return table[i].value;
	}

	// Table B.14: DCT coefficient; false at end of block. run counts the zero
	// coefficients preceding this one. The sign bit only exists while the coefficient
	// still fits into the block, so the caller reads it.
	static bool DecodeDCTCoeff(MPEGBitReader& reader, int& run, int& level, bool is_first, bool& esc, bool& error) {
		esc = false;
		const MPEGDCT* table = is_first ? s_dct_first : s_dct_next;
		stduint count = is_first ? (sizeof(s_dct_first) / sizeof(MPEGDCT)) : (sizeof(s_dct_next) / sizeof(MPEGDCT));
		int i = DCTSearch(reader, table, count);
		if (i >= 0) {
			reader.SkipBits(table[i].len);
			if (table[i].eob) return false;
			run = table[i].run;
			level = table[i].level;
			return true;
		}
		if (reader.ShowBits(6) == 0x01) { // escape: run(6) + level(8, extended when 0/128)
			reader.SkipBits(6);
			run = (int)reader.ReadBits(6);
			int raw = (int)reader.ReadBits(8);
			if (raw == 0) raw = (int)reader.ReadBits(8);
			else if (raw == 128) raw = (int)reader.ReadBits(8) - 256;
			else if (raw > 128) raw -= 256;
			level = raw;
			esc = true;
			return true;
		}
		// No code, no escape: the slice is broken here. Report it so the caller can
		// conceal the rest of the row instead of writing garbage.
		error = true;
		reader.SkipBits(1);
		return false;
	}

	static inline int SampleHalfPel(const uint8* src, int w, int h, int stride, int x2, int y2) {
		int int_x = x2 >> 1;
		int int_y = y2 >> 1;
		int frac_x = x2 & 1;
		int frac_y = y2 & 1;

		int x0 = int_x; if (x0 < 0) x0 = 0; if (x0 >= w) x0 = w - 1;
		int x1 = int_x + 1; if (x1 < 0) x1 = 0; if (x1 >= w) x1 = w - 1;
		int y0 = int_y; if (y0 < 0) y0 = 0; if (y0 >= h) y0 = h - 1;
		int y1 = int_y + 1; if (y1 < 0) y1 = 0; if (y1 >= h) y1 = h - 1;

		int p00 = src[y0 * stride + x0];
		if (!frac_x && !frac_y) return p00;
		if (frac_x && !frac_y) {
			int p01 = src[y0 * stride + x1];
			return (p00 + p01 + 1) >> 1;
		}
		if (!frac_x && frac_y) {
			int p10 = src[y1 * stride + x0];
			return (p00 + p10 + 1) >> 1;
		}
		int p01 = src[y0 * stride + x1];
		int p10 = src[y1 * stride + x0];
		int p11 = src[y1 * stride + x1];
		return (p00 + p01 + p10 + p11 + 2) >> 2;
	}

	static void MotionCompensateMB(
		uint8* cur_y, uint8* cur_u, uint8* cur_v,
		const uint8* ref_y, const uint8* ref_u, const uint8* ref_v,
		int width, int height, int mb_width,
		int mb_x, int mb_y, int pmv_x, int pmv_y
	) {
		int stride_y = mb_width * 16;
		int stride_uv = mb_width * 8;

		// Luma 16x16
		for (int y = 0; y < 16; ++y) {
			for (int x = 0; x < 16; ++x) {
				int x2 = (mb_x * 16 + x) * 2 + pmv_x;
				int y2 = (mb_y * 16 + y) * 2 + pmv_y;
				cur_y[(mb_y * 16 + y) * stride_y + (mb_x * 16 + x)] = (uint8)SampleHalfPel(ref_y, width, height, stride_y, x2, y2);
			}
		}

		// Chroma 8x8
		int pmv_uv_x = pmv_x / 2;
		int pmv_uv_y = pmv_y / 2;
		for (int y = 0; y < 8; ++y) {
			for (int x = 0; x < 8; ++x) {
				int x2 = (mb_x * 8 + x) * 2 + pmv_uv_x;
				int y2 = (mb_y * 8 + y) * 2 + pmv_uv_y;
				cur_u[(mb_y * 8 + y) * stride_uv + (mb_x * 8 + x)] = (uint8)SampleHalfPel(ref_u, width / 2, height / 2, stride_uv, x2, y2);
				cur_v[(mb_y * 8 + y) * stride_uv + (mb_x * 8 + x)] = (uint8)SampleHalfPel(ref_v, width / 2, height / 2, stride_uv, x2, y2);
			}
		}
	}

	static void MotionCompensateInterpMB(
		uint8* cur_y, uint8* cur_u, uint8* cur_v,
		const uint8* fwd_y, const uint8* fwd_u, const uint8* fwd_v,
		const uint8* bwd_y, const uint8* bwd_u, const uint8* bwd_v,
		int width, int height, int mb_width,
		int mb_x, int mb_y,
		int pmv_fwd_x, int pmv_fwd_y,
		int pmv_bwd_x, int pmv_bwd_y
	) {
		int stride_y = mb_width * 16;
		int stride_uv = mb_width * 8;

		// Luma 16x16
		for (int y = 0; y < 16; ++y) {
			for (int x = 0; x < 16; ++x) {
				int x2_fwd = (mb_x * 16 + x) * 2 + pmv_fwd_x;
				int y2_fwd = (mb_y * 16 + y) * 2 + pmv_fwd_y;
				int v_fwd = SampleHalfPel(fwd_y, width, height, stride_y, x2_fwd, y2_fwd);

				int x2_bwd = (mb_x * 16 + x) * 2 + pmv_bwd_x;
				int y2_bwd = (mb_y * 16 + y) * 2 + pmv_bwd_y;
				int v_bwd = SampleHalfPel(bwd_y, width, height, stride_y, x2_bwd, y2_bwd);

				cur_y[(mb_y * 16 + y) * stride_y + (mb_x * 16 + x)] = (uint8)((v_fwd + v_bwd + 1) >> 1);
			}
		}

		// Chroma 8x8
		int pmv_fwd_uv_x = pmv_fwd_x / 2;
		int pmv_fwd_uv_y = pmv_fwd_y / 2;
		int pmv_bwd_uv_x = pmv_bwd_x / 2;
		int pmv_bwd_uv_y = pmv_bwd_y / 2;

		for (int y = 0; y < 8; ++y) {
			for (int x = 0; x < 8; ++x) {
				int x2_fwd = (mb_x * 8 + x) * 2 + pmv_fwd_uv_x;
				int y2_fwd = (mb_y * 8 + y) * 2 + pmv_fwd_uv_y;
				int u_fwd = SampleHalfPel(fwd_u, width / 2, height / 2, stride_uv, x2_fwd, y2_fwd);
				int v_fwd = SampleHalfPel(fwd_v, width / 2, height / 2, stride_uv, x2_fwd, y2_fwd);

				int x2_bwd = (mb_x * 8 + x) * 2 + pmv_bwd_uv_x;
				int y2_bwd = (mb_y * 8 + y) * 2 + pmv_bwd_uv_y;
				int u_bwd = SampleHalfPel(bwd_u, width / 2, height / 2, stride_uv, x2_bwd, y2_bwd);
				int v_bwd = SampleHalfPel(bwd_v, width / 2, height / 2, stride_uv, x2_bwd, y2_bwd);

				cur_u[(mb_y * 8 + y) * stride_uv + (mb_x * 8 + x)] = (uint8)((u_fwd + u_bwd + 1) >> 1);
				cur_v[(mb_y * 8 + y) * stride_uv + (mb_x * 8 + x)] = (uint8)((v_fwd + v_bwd + 1) >> 1);
			}
		}
	}

	// -------------------------------------------------------------
	// MPEG Program Stream Demuxer & Player
	// -------------------------------------------------------------

	struct MPEGFrameRef {
		uint32 es_offset;
		uint32 es_length;
		uint32 pts_ms;
		uint8  type; // 1=I, 2=P, 3=B
	};

	class MPEGStream : public uni::IVideoStream {
	private:
		uni::StorageTrait* storage;
		uni::VideoInfo     info;

		// Pure Elementary Stream Buffers
		byte*  video_es;
		size_t video_es_len;
		byte*  audio_es;
		size_t audio_es_len;

		// Audio Decoder integration
		MemoryStorageDevice* audio_storage;
		uni::IAudioStream*   audio_stream;

		// Video properties
		uint32 width;
		uint32 height;
		uint32 mb_width;
		uint32 mb_height;
		uint32 frame_rate_num;
		uint32 frame_rate_den;
		uint8  intra_quant[64];
		uint8  non_intra_quant[64];

		// Planes (Y, U, V)
		uint8* fwd_y;
		uint8* fwd_u;
		uint8* fwd_v;
		uint8* bwd_y;
		uint8* bwd_u;
		uint8* bwd_v;
		uint8* cur_y;
		uint8* cur_u;
		uint8* cur_v;

		// Frame index mapping
		MPEGFrameRef* v_frames;
		uint32        v_frame_count;
		uint32        curr_vframe;

		int dc_dct_pred[3];
		int recon_block[64];

	public:
		MPEGStream(uni::StorageTrait* st)
			: storage(st), video_es(nullptr), video_es_len(0), audio_es(nullptr), audio_es_len(0),
			  audio_storage(nullptr), audio_stream(nullptr),
			  width(0), height(0), mb_width(0), mb_height(0),
			  frame_rate_num(30), frame_rate_den(1),
			  fwd_y(nullptr), fwd_u(nullptr), fwd_v(nullptr),
			  bwd_y(nullptr), bwd_u(nullptr), bwd_v(nullptr),
			  cur_y(nullptr), cur_u(nullptr), cur_v(nullptr),
			  v_frames(nullptr), v_frame_count(0), curr_vframe(0) {
			MemCopyN(intra_quant, s_default_intra_quant, 64);
			MemSet(non_intra_quant, 16, 64);
		}

		virtual ~MPEGStream() {
			Release();
		}

		virtual void Release() override {
			if (audio_stream) {
				audio_stream->Release();
				audio_stream = nullptr;
			}
			if (audio_storage) {
				delete audio_storage;
				audio_storage = nullptr;
			}
			if (v_frames) { free(v_frames); v_frames = nullptr; }
			if (video_es) { free(video_es); video_es = nullptr; }
			if (audio_es) { free(audio_es); audio_es = nullptr; }
			if (fwd_y) { free(fwd_y); fwd_y = nullptr; }
			if (fwd_u) { free(fwd_u); fwd_u = nullptr; }
			if (fwd_v) { free(fwd_v); fwd_v = nullptr; }
			if (bwd_y) { free(bwd_y); bwd_y = nullptr; }
			if (bwd_u) { free(bwd_u); bwd_u = nullptr; }
			if (bwd_v) { free(bwd_v); bwd_v = nullptr; }
			if (cur_y) { free(cur_y); cur_y = nullptr; }
			if (cur_u) { free(cur_u); cur_u = nullptr; }
			if (cur_v) { free(cur_v); cur_v = nullptr; }
		}

		bool Init() {
			if (!storage) return false;

			stduint total_size = storage->getUnits() * (storage->Block_Size ? storage->Block_Size : 4096);
			if (total_size < 32) return false;

			// Demux Video ES and Audio ES from container
			if (!DemuxStream(total_size)) {
				return false;
			}

			// Parse Sequence Header from Video ES
			MPEGBitReader reader(video_es, video_es_len);
			uint32 sc = 0;
			bool found_seq = false;

			while (reader.NextStartCode(sc)) {
				if (sc == MPEG_SEQUENCE_HEADER_CODE) {
					width = reader.ReadBits(12);
					height = reader.ReadBits(12);
					reader.ReadBits(4); // aspect ratio
					uint8 fr_code = (uint8)reader.ReadBits(4);

					if (fr_code > 0 && fr_code <= 8) {
						frame_rate_num = s_mpeg_frame_rates[fr_code].num;
						frame_rate_den = s_mpeg_frame_rates[fr_code].den;
					}

					reader.ReadBits(18); // bit_rate
					reader.ReadBit();    // marker
					reader.ReadBits(10); // vbv
					reader.ReadBit();    // constrained

					if (reader.ReadBit()) { // load intra quant
						for (int i = 0; i < 64; ++i) {
							intra_quant[s_zigzag_scan[i]] = (uint8)reader.ReadBits(8);
						}
					}
					if (reader.ReadBit()) { // load non-intra quant
						for (int i = 0; i < 64; ++i) {
							non_intra_quant[s_zigzag_scan[i]] = (uint8)reader.ReadBits(8);
						}
					}

					found_seq = true;
					break;
				}
			}

			if (!found_seq || width == 0 || height == 0) {
				return false;
			}

			mb_width = (width + 15) / 16;
			mb_height = (height + 15) / 16;

			// Allocate planes
			size_t y_size = (size_t)mb_width * 16 * (size_t)mb_height * 16;
			size_t uv_size = (size_t)mb_width * 8 * (size_t)mb_height * 8;

			fwd_y = (uint8*)malloc(y_size);
			fwd_u = (uint8*)malloc(uv_size);
			fwd_v = (uint8*)malloc(uv_size);
			bwd_y = (uint8*)malloc(y_size);
			bwd_u = (uint8*)malloc(uv_size);
			bwd_v = (uint8*)malloc(uv_size);
			cur_y = (uint8*)malloc(y_size);
			cur_u = (uint8*)malloc(uv_size);
			cur_v = (uint8*)malloc(uv_size);

			if (!fwd_y || !fwd_u || !fwd_v || !bwd_y || !bwd_u || !bwd_v || !cur_y || !cur_u || !cur_v) {
				return false;
			}

			MemSet(fwd_y, 0, y_size);
			MemSet(fwd_u, 128, uv_size);
			MemSet(fwd_v, 128, uv_size);
			MemSet(bwd_y, 0, y_size);
			MemSet(bwd_u, 128, uv_size);
			MemSet(bwd_v, 128, uv_size);

			// Build Video Frame Indices
			BuildVideoIndices();

			// Connect MP3 / MP2 audio stream decoder
			if (audio_es_len >= 4) {
				audio_storage = new MemoryStorageDevice(audio_es, audio_es_len);
				if (audio_storage) {
					uni::MP3Codec mp3_codec;
					bool matched = false;
					if (mp3_codec.Probe(*audio_storage, matched) == uni::AudioResult::OK && matched) {
						if (mp3_codec.OpenStream(*audio_storage, audio_stream, s_mpeg_allocator) == uni::AudioResult::OK && audio_stream) {
							uni::AudioInfo a_info{};
							if (audio_stream->GetInfo(a_info) == uni::AudioResult::OK) {
								info.hasAudio = true;
								info.audioInfo = a_info;
							}
						}
					}
					if (!info.hasAudio) {
						delete audio_storage;
						audio_storage = nullptr;
					}
				}
			}

			info.videoFormat.width = width;
			info.videoFormat.height = height;
			info.videoFormat.frame_rate_num = frame_rate_num;
			info.videoFormat.frame_rate_den = frame_rate_den;
			info.videoFormat.codec_type = uni::VideoCodecType::MPEG1;
			info.videoFormat.pixel_format = uni::PixelFormat::BGRA8888;
			info.containerFormat = uni::VideoContainerFormat::MPEG;
			info.totalFrames = v_frame_count;
			info.durationMs = (v_frame_count > 0 && frame_rate_num > 0)
				? (uint32)(((uint64)v_frame_count * 1000ULL * frame_rate_den) / frame_rate_num)
				: 0;

			return true;
		}

		bool DemuxStream(stduint total_size) {
			size_t v_cap = (total_size > 1024 * 1024) ? total_size : 1024 * 1024;
			size_t a_cap = (total_size > 1024 * 1024) ? (total_size / 4) : 262144;

			video_es = (byte*)malloc(v_cap);
			audio_es = (byte*)malloc(a_cap);
			if (!video_es || !audio_es) return false;
			video_es_len = 0;
			audio_es_len = 0;

			stduint block_size = storage->Block_Size ? storage->Block_Size : 4096;
			stduint total_units = storage->getUnits();

			// Allocate a 256KB streaming window buffer
			size_t win_cap = 256 * 1024;
			if (win_cap < block_size * 2) win_cap = block_size * 2;
			byte* win_buf = (byte*)malloc(win_cap);
			if (!win_buf) return false;

			stduint cur_unit = 0;
			size_t win_len = 0;
			size_t win_pos = 0;
			uint64 stream_pos = 0;

			auto ensure_bytes = [&](size_t needed) -> bool {
				if (needed > win_cap) {
					size_t new_cap = (needed + 65535) & ~65535;
					byte* new_buf = (byte*)realloc(win_buf, new_cap);
					if (!new_buf) return false;
					win_buf = new_buf;
					win_cap = new_cap;
				}
				while (win_len - win_pos < needed && cur_unit < total_units) {
					if (win_cap - win_len < block_size || win_pos >= 65536) {
						if (win_pos > 0) {
							size_t rem = win_len - win_pos;
							if (rem > 0) MemCopyN(win_buf, win_buf + win_pos, rem);
							win_len = rem;
							win_pos = 0;
						}
					}
					stduint space = win_cap - win_len;
					stduint max_units = space / block_size;
					if (max_units == 0) max_units = 1;
					stduint units_to_read = total_units - cur_unit;
					if (units_to_read > max_units) units_to_read = max_units;
					if (units_to_read == 0) break;

					if (!storage->Read(cur_unit, win_buf + win_len, units_to_read)) {
						break;
					}
					stduint bytes_added = units_to_read * block_size;
					if ((cur_unit + units_to_read) * block_size > total_size) {
						bytes_added = (stduint)(total_size - cur_unit * block_size);
					}
					cur_unit += units_to_read;
					win_len += bytes_added;
				}
				return (win_len - win_pos) >= needed;
			};

			// Check first 4 bytes for pure ES
			if (ensure_bytes(4)) {
				if (win_buf[win_pos] == 0x00 && win_buf[win_pos+1] == 0x00 && win_buf[win_pos+2] == 0x01 && win_buf[win_pos+3] == 0xB3) {
					video_es = (byte*)realloc(video_es, (size_t)total_size);
					size_t copied = 0;
					while (copied < total_size) {
						size_t avail = win_len - win_pos;
						if (avail == 0) {
							if (!ensure_bytes(1)) break;
							avail = win_len - win_pos;
						}
						size_t to_copy = (copied + avail > total_size) ? (size_t)(total_size - copied) : avail;
						MemCopyN(video_es + copied, win_buf + win_pos, to_copy);
						copied += to_copy;
						win_pos += to_copy;
					}
					video_es_len = copied;
					free(win_buf);
					return video_es_len > 0;
				}
			}

			// Demux Program Stream
			while (ensure_bytes(6)) {
				const byte* h = win_buf + win_pos;
				if (h[0] == 0x00 && h[1] == 0x00 && h[2] == 0x01) {
					uint8 code = h[3];
					if (code >= 0xE0 && code <= 0xEF) { // Video PES
						size_t pkt_len = ((size_t)h[4] << 8) | h[5];
						size_t payload_len = pkt_len ? pkt_len : (size_t)(total_size - (stream_pos + 6));
						win_pos += 6;
						stream_pos += 6;

						if (ensure_bytes(payload_len)) {
							const byte* pes = win_buf + win_pos;
							size_t off = 0;
							while (off < payload_len && pes[off] == 0xFF) off++;
							if (off < payload_len && (pes[off] & 0xC0) == 0x40) off += 2;
							if (off < payload_len) {
								if ((pes[off] & 0xF0) == 0x20) off += 5;
								else if ((pes[off] & 0xF0) == 0x30) off += 10;
								else if (pes[off] == 0x0F) off += 1;
								else if ((pes[off] & 0xC0) == 0x80) off += 3 + pes[off + 2];
							}
							if (off < payload_len) {
								size_t es_bytes = payload_len - off;
								if (video_es_len + es_bytes > v_cap) {
									v_cap = (video_es_len + es_bytes) * 2;
									video_es = (byte*)realloc(video_es, v_cap);
								}
								MemCopyN(video_es + video_es_len, pes + off, es_bytes);
								video_es_len += es_bytes;
							}
							win_pos += payload_len;
							stream_pos += payload_len;
						}
					} else if (code >= 0xC0 && code <= 0xDF) { // Audio PES
						size_t pkt_len = ((size_t)h[4] << 8) | h[5];
						size_t payload_len = pkt_len ? pkt_len : (size_t)(total_size - (stream_pos + 6));
						win_pos += 6;
						stream_pos += 6;

						if (ensure_bytes(payload_len)) {
							const byte* pes = win_buf + win_pos;
							size_t off = 0;
							while (off < payload_len && pes[off] == 0xFF) off++;
							if (off < payload_len && (pes[off] & 0xC0) == 0x40) off += 2;
							if (off < payload_len) {
								if ((pes[off] & 0xF0) == 0x20) off += 5;
								else if ((pes[off] & 0xF0) == 0x30) off += 10;
								else if (pes[off] == 0x0F) off += 1;
								else if ((pes[off] & 0xC0) == 0x80) off += 3 + pes[off + 2];
							}
							if (off < payload_len) {
								size_t es_bytes = payload_len - off;
								if (audio_es_len + es_bytes > a_cap) {
									a_cap = (audio_es_len + es_bytes) * 2;
									audio_es = (byte*)realloc(audio_es, a_cap);
								}
								MemCopyN(audio_es + audio_es_len, pes + off, es_bytes);
								audio_es_len += es_bytes;
							}
							win_pos += payload_len;
							stream_pos += payload_len;
						}
					} else if (code == 0xBA) { // Pack Header
						if ((h[4] & 0xF0) == 0x20) {
							win_pos += 12; stream_pos += 12;
						} else if ((h[4] & 0xC0) == 0x40) {
							uint8 stuffing = (h[13] & 0x07);
							win_pos += 14 + stuffing; stream_pos += 14 + stuffing;
						} else {
							win_pos += 12; stream_pos += 12;
						}
					} else if (code == 0xBB) { // System Header
						size_t sys_len = ((size_t)h[4] << 8) | h[5];
						win_pos += 6 + sys_len; stream_pos += 6 + sys_len;
					} else if (code == 0xB9) {
						win_pos += 4; stream_pos += 4;
					} else {
						win_pos += 1; stream_pos += 1;
					}
				} else {
					win_pos += 1; stream_pos += 1;
				}
			}

			free(win_buf);
			return video_es_len > 0;
		}

		void BuildVideoIndices() {
			uint32 v_frame_cap = 256;
			v_frame_count = 0;
			v_frames = (MPEGFrameRef*)malloc(sizeof(MPEGFrameRef) * v_frame_cap);

			uint32 last_pic_offset = 0;
			uint8  last_pic_type = 1;
			bool   has_pending_pic = false;

			size_t p = 0;
			while (p + 4 <= video_es_len) {
				if (video_es[p] == 0x00 && video_es[p + 1] == 0x00 && video_es[p + 2] == 0x01) {
					uint8 code = video_es[p + 3];
					if (code == 0x00) { // Picture Header
						if (has_pending_pic) {
							if (v_frame_count >= v_frame_cap) {
								v_frame_cap *= 2;
								v_frames = (MPEGFrameRef*)realloc(v_frames, sizeof(MPEGFrameRef) * v_frame_cap);
							}
							MPEGFrameRef& fr = v_frames[v_frame_count++];
							fr.es_offset = last_pic_offset;
							fr.es_length = (uint32)(p - last_pic_offset);
							fr.type = last_pic_type;
							fr.pts_ms = (uint32)(((uint64)(v_frame_count - 1) * 1000ULL * frame_rate_den) / frame_rate_num);
						}
						last_pic_offset = (uint32)p;
						if (p + 5 < video_es_len) {
							last_pic_type = (video_es[p + 5] >> 3) & 0x07;
							if (last_pic_type == 0 || last_pic_type > 3) last_pic_type = 1;
						} else {
							last_pic_type = 1;
						}
						has_pending_pic = true;
						p += 4;
					} else if (code == 0xB9 || code == 0xB7) { // Sequence / Program End
						if (has_pending_pic) {
							if (v_frame_count >= v_frame_cap) {
								v_frame_cap *= 2;
								v_frames = (MPEGFrameRef*)realloc(v_frames, sizeof(MPEGFrameRef) * v_frame_cap);
							}
							MPEGFrameRef& fr = v_frames[v_frame_count++];
							fr.es_offset = last_pic_offset;
							fr.es_length = (uint32)(p - last_pic_offset);
							fr.type = last_pic_type;
							fr.pts_ms = (uint32)(((uint64)(v_frame_count - 1) * 1000ULL * frame_rate_den) / frame_rate_num);
							has_pending_pic = false;
						}
						p += 4;
					} else {
						p += 4;
					}
				} else {
					p++;
				}
			}

			if (has_pending_pic && last_pic_offset < video_es_len) {
				if (v_frame_count >= v_frame_cap) {
					v_frame_cap += 16;
					v_frames = (MPEGFrameRef*)realloc(v_frames, sizeof(MPEGFrameRef) * v_frame_cap);
				}
				MPEGFrameRef& fr = v_frames[v_frame_count++];
				fr.es_offset = last_pic_offset;
				fr.es_length = (uint32)(video_es_len - last_pic_offset);
				fr.type = last_pic_type;
				fr.pts_ms = (uint32)(((uint64)(v_frame_count - 1) * 1000ULL * frame_rate_den) / frame_rate_num);
			}
		}

		virtual uni::VideoResult GetInfo(uni::VideoInfo& outInfo) const override {
			outInfo = info;
			return uni::VideoResult::OK;
		}

		virtual uni::VideoResult ReadVideoFrame(uni::VideoFrame& outFrame, uni::trait::Malloc& allocator) override {
			if (curr_vframe >= v_frame_count) {
				return uni::VideoResult::EndOfStream;
			}

			const MPEGFrameRef& ref = v_frames[curr_vframe];
			if (ref.es_length == 0 || ref.es_offset + ref.es_length > video_es_len) {
				curr_vframe++;
				return uni::VideoResult::Failed;
			}

			MPEGBitReader reader(video_es + ref.es_offset, ref.es_length);
			uint32 sc = 0;

			// Parse Picture Header
			if (reader.NextStartCode(sc) && sc == MPEG_PICTURE_START_CODE) {
				reader.ReadBits(10); // temporal_reference
				uint8 pic_type = (uint8)reader.ReadBits(3);
				reader.ReadBits(16); // vbv_delay

				int f_code_fwd = 1;
				int f_code_bwd = 1;

				if (pic_type == MPEG_PICTURE_TYPE_P || pic_type == MPEG_PICTURE_TYPE_B) {
					reader.ReadBit(); // full_pel_fwd
					f_code_fwd = (int)reader.ReadBits(3);
					if (f_code_fwd == 0) f_code_fwd = 1;
				}
				if (pic_type == MPEG_PICTURE_TYPE_B) {
					reader.ReadBit(); // full_pel_bwd
					f_code_bwd = (int)reader.ReadBits(3);
					if (f_code_bwd == 0) f_code_bwd = 1;
				}

				size_t y_sz = (size_t)mb_width * 16 * (size_t)mb_height * 16;
				size_t uv_sz = (size_t)mb_width * 8 * (size_t)mb_height * 8;

				if (pic_type != MPEG_PICTURE_TYPE_I) {
					// B pictures always code every macroblock; start from the anchor
					MemCopyN(cur_y, bwd_y, y_sz);
					MemCopyN(cur_u, bwd_u, uv_sz);
					MemCopyN(cur_v, bwd_v, uv_sz);
				}
				if (pic_type == MPEG_PICTURE_TYPE_I) {
					// Start from the previous anchor rather than neutral values: if a slice of
					// this I picture cannot be decoded the area keeps the previous picture
					// instead of turning into a block of black or grey.
					MemCopyN(cur_y, bwd_y, y_sz);
					MemCopyN(cur_u, bwd_u, uv_sz);
					MemCopyN(cur_v, bwd_v, uv_sz);
				}

				// Reset Predictors
				dc_dct_pred[0] = 128;
				dc_dct_pred[1] = 128;
				dc_dct_pred[2] = 128;

				int pmv_fwd[2] = { 0, 0 };
				int pmv_bwd[2] = { 0, 0 };

				// Parse Slices
				while (reader.NextStartCode(sc)) {
					if (sc >= MPEG_SLICE_START_CODE_MIN && sc <= MPEG_SLICE_START_CODE_MAX) {
						int slice_vert = (int)(sc & 0xFF);
						int quant_scale = (int)reader.ReadBits(5);
						if (quant_scale == 0) quant_scale = 1;

						while (reader.ReadBit() == 1) {
							reader.ReadBits(8);
						}

						// Reset predictors at slice boundary
						dc_dct_pred[0] = 128;
						dc_dct_pred[1] = 128;
						dc_dct_pred[2] = 128;
						pmv_fwd[0] = pmv_fwd[1] = 0;
						pmv_bwd[0] = pmv_bwd[1] = 0;

						int mb_address = (slice_vert - 1) * (int)mb_width - 1;
						int last_mb_address = mb_address;

						while (reader.HasData()) {
							int rem = reader.GetBitsLeft() % 8;
							if (rem > 0) {
								if (reader.ShowBits(rem + 24) == 1) break;
							} else {
								if (reader.ShowBits(24) == 1) break;
							}

							bool slice_end = false;
							int mbai = DecodeMBAI(reader, slice_end);
							if (slice_end) break;
							mb_address += mbai;
							if (mb_address >= (int)(mb_width * mb_height)) break;

							// Handle skipped macroblocks
							for (int skip_mb = last_mb_address + 1; skip_mb < mb_address; ++skip_mb) {
								int sx = skip_mb % (int)mb_width;
								int sy = skip_mb / (int)mb_width;
								if (pic_type != MPEG_PICTURE_TYPE_B) {
									pmv_fwd[0] = pmv_fwd[1] = 0;
									MotionCompensateMB(cur_y, cur_u, cur_v, bwd_y, bwd_u, bwd_v, width, height, mb_width, sx, sy, 0, 0);
								}
								dc_dct_pred[0] = dc_dct_pred[1] = dc_dct_pred[2] = 128;
							}

							int mb_x = mb_address % (int)mb_width;
							int mb_y = mb_address / (int)mb_width;

							MBTypeDec mb_type = DecodeMBType(reader, pic_type);
							if (mb_type.quant) {
								quant_scale = (int)reader.ReadBits(5);
								if (quant_scale == 0) quant_scale = 1;
							}
							if (mb_type.intra) {
								pmv_fwd[0] = pmv_fwd[1] = 0;
								pmv_bwd[0] = pmv_bwd[1] = 0;
							} else {
								if (mb_type.forward) {
									DecodeMotionVector(reader, pmv_fwd[0], f_code_fwd);
									DecodeMotionVector(reader, pmv_fwd[1], f_code_fwd);
								} else if (pic_type == MPEG_PICTURE_TYPE_P) {
									pmv_fwd[0] = pmv_fwd[1] = 0;
								}
								if (mb_type.backward) {
									DecodeMotionVector(reader, pmv_bwd[0], f_code_bwd);
									DecodeMotionVector(reader, pmv_bwd[1], f_code_bwd);
								} else if (pic_type == MPEG_PICTURE_TYPE_B) {
									pmv_bwd[0] = pmv_bwd[1] = 0;
								}
							}

							int cbp = 0;
							if (mb_type.intra) {
								cbp = 63;
							} else if (mb_type.pattern) {
								cbp = DecodeCBP(reader);
							}

							// Motion Compensation Prediction for non-intra
							if (!mb_type.intra) {
								if (mb_type.forward && mb_type.backward) {
									MotionCompensateInterpMB(cur_y, cur_u, cur_v, fwd_y, fwd_u, fwd_v, bwd_y, bwd_u, bwd_v, width, height, mb_width, mb_x, mb_y, pmv_fwd[0], pmv_fwd[1], pmv_bwd[0], pmv_bwd[1]);
								} else if (mb_type.backward) {
									MotionCompensateMB(cur_y, cur_u, cur_v, bwd_y, bwd_u, bwd_v, width, height, mb_width, mb_x, mb_y, pmv_bwd[0], pmv_bwd[1]);
								} else if (pic_type == MPEG_PICTURE_TYPE_B) {
									// forward prediction of a B-picture refers to the past anchor
									MotionCompensateMB(cur_y, cur_u, cur_v, fwd_y, fwd_u, fwd_v, width, height, mb_width, mb_x, mb_y, pmv_fwd[0], pmv_fwd[1]);
								} else {
									// forward prediction of a P-picture refers to the last anchor
									MotionCompensateMB(cur_y, cur_u, cur_v, bwd_y, bwd_u, bwd_v, width, height, mb_width, mb_x, mb_y, pmv_fwd[0], pmv_fwd[1]);
								}
								// Non-intra resets DC predictors
								dc_dct_pred[0] = dc_dct_pred[1] = dc_dct_pred[2] = 128;
							}

							// Decode Blocks
							if (!DecodeBlocks(reader, mb_type.intra, cbp, quant_scale, mb_x, mb_y)) {
								// Broken slice: conceal the rest of the row from the previous
								// anchor, then resync at the next slice start code.
								ConcealSliceTail(mb_x, mb_y);
								break;
							}
							last_mb_address = mb_address;
						}

						// A slice has to cover whole macroblock rows. If the data ran out
						// early the remaining macroblocks of this row were never decoded,
						// so conceal them instead of leaving neutral blocks on screen.
						{
							// Slices in this stream can span two macroblock rows, so conceal up to
							// the end of the picture; the following slices overwrite their own
							// macroblocks anyway.
							int row_end = (int)(mb_width * mb_height);
							for (int miss = last_mb_address + 1; miss < row_end; ) {
								int miss_y = miss / (int)mb_width;
								ConcealSliceTail(miss % (int)mb_width, miss_y);
								miss = (miss_y + 1) * (int)mb_width;
							}
						}
					} else {
						break;
					}
				}

				// Update anchor frames for subsequent P/B pictures
				if (pic_type == MPEG_PICTURE_TYPE_I || pic_type == MPEG_PICTURE_TYPE_P) {
					// the previous anchor becomes the forward (past) reference, the
					// picture just decoded becomes the backward (future) reference
					MemCopyN(fwd_y, bwd_y, y_sz);
					MemCopyN(fwd_u, bwd_u, uv_sz);
					MemCopyN(fwd_v, bwd_v, uv_sz);
					MemCopyN(bwd_y, cur_y, y_sz);
					MemCopyN(bwd_u, cur_u, uv_sz);
					MemCopyN(bwd_v, cur_v, uv_sz);
				}
			}

			// Render current YUV420 planes to BGRA8888 Color canvas
			size_t canvas_size = (size_t)width * (size_t)height * sizeof(uni::Color);
			uni::Color* pixels = (uni::Color*)malloc(canvas_size);
			if (!pixels) {
				curr_vframe++;
				return uni::VideoResult::OutOfMemory;
			}

			uint32 stride_y = mb_width * 16;
			uint32 stride_uv = mb_width * 8;

			for (uint32 y = 0; y < height; ++y) {
				const uint8* py = cur_y + y * stride_y;
				const uint8* pu = cur_u + (y / 2) * stride_uv;
				const uint8* pv = cur_v + (y / 2) * stride_uv;
				uni::Color* dst_row = pixels + y * width;

				for (uint32 x = 0; x < width; ++x) {
					// MPEG-1 codes studio range video: luma 16..235, chroma 16..240. The
					// matrix has to expand that onto 0..255, otherwise black stays at 16
					// and white stops at 235 and the whole picture looks washed out.
					int Y = (((int)py[x] - 16) * 1192) >> 10;
					int U = pu[x / 2] - 128;
					int V = pv[x / 2] - 128;

					int r = Y + ((V * 1634) >> 10);
					int g = Y - ((U * 401 + V * 833) >> 10);
					int b = Y + ((U * 2066) >> 10);

					dst_row[x].r = (uint8)Clamp255(r);
					dst_row[x].g = (uint8)Clamp255(g);
					dst_row[x].b = (uint8)Clamp255(b);
					dst_row[x].a = 0xFF;
				}
			}

			uni::VideoFrameClear(outFrame);
			outFrame.width = width;
			outFrame.height = height;
			outFrame.timestampMs = ref.pts_ms;
			outFrame.frameIndex = curr_vframe;
			outFrame.isKeyFrame = (ref.type == MPEG_PICTURE_TYPE_I);

			outFrame.image.width = width;
			outFrame.image.height = height;
			outFrame.image.stride = width * sizeof(uni::Color);
			outFrame.image.format = uni::PixelFormat::BGRA8888;
			outFrame.image.colorSpace = uni::ColorSpace::SRGB;
			outFrame.image.pixels = pixels;
			outFrame.image.size = canvas_size;
			outFrame.image.allocator = nullptr;

			curr_vframe++;
			return uni::VideoResult::OK;
		}

		// A broken slice leaves the rest of its macroblock row undecoded. Copy the
		// previous anchor over it so the picture degrades to "unchanged" instead of
		// showing neutral or random blocks.
		void ConcealSliceTail(int mb_x, int mb_y) {
			uint32 stride_y = mb_width * 16;
			uint32 stride_uv = mb_width * 8;

			for (int x = mb_x; x < (int)mb_width; ++x) {
				for (int r = 0; r < 16; ++r) {
					MemCopyN(cur_y + (mb_y * 16 + r) * stride_y + x * 16,
							 bwd_y + (mb_y * 16 + r) * stride_y + x * 16, 16);
				}
				for (int r = 0; r < 8; ++r) {
					MemCopyN(cur_u + (mb_y * 8 + r) * stride_uv + x * 8,
							 bwd_u + (mb_y * 8 + r) * stride_uv + x * 8, 8);
					MemCopyN(cur_v + (mb_y * 8 + r) * stride_uv + x * 8,
							 bwd_v + (mb_y * 8 + r) * stride_uv + x * 8, 8);
				}
			}
		}

		bool DecodeBlocks(MPEGBitReader& reader, bool is_intra, int cbp, int quant_scale, int mb_x, int mb_y) {
			uint32 stride_y = mb_width * 16;
			uint32 stride_uv = mb_width * 8;

			for (int block_idx = 0; block_idx < 6; ++block_idx) {
				if (!(cbp & (1 << block_idx))) {
					continue;
				}

				MemSet(recon_block, 0, sizeof(recon_block));
				int comp = (block_idx < 4) ? 0 : (block_idx == 4 ? 1 : 2);
				int coeff_idx = 0;

				if (is_intra) {
					int dc_size = DecodeDCSize(reader, comp == 0);
					int dc_diff = 0;
					if (dc_size > 0) {
						dc_diff = reader.ReadBits(dc_size);
						if (!(dc_diff & (1 << (dc_size - 1)))) {
							dc_diff = dc_diff - (1 << dc_size) + 1;
						}
					}
					dc_dct_pred[comp] += dc_diff;
					recon_block[0] = dc_dct_pred[comp] * 8;
					coeff_idx = 1;
				}

				// Decode AC Coefficients
				int run = 0, level = 0;
				bool full_block = false;					// last coefficient landed on position 63
				int slice_bits = (int)mb_width * 16 * 0;	// unused placeholder, see reader guard
				while (coeff_idx < 64) {
					bool is_first = (coeff_idx == 0 && !is_intra);
					bool esc = false;
					bool error = false;
					if (reader.GetBitsLeft() <= 0 && !reader.HasData()) { error = true; }
					if (!DecodeDCTCoeff(reader, run, level, is_first, esc, error)) {
						if (error) return false;	// broken slice: let the caller conceal
						break;						// end of block
					}
					coeff_idx += run;
					if (coeff_idx >= 64) break;	// past the block: end of block, no sign bit
					if (!esc) {
						level = reader.ReadBit() ? -level : level;
					}
					int scan_pos = s_zigzag_scan[coeff_idx];
					// The reference decoder folds the quantiser scale into its matrix as
					// 2 * quant_scale * weight and then shifts by 4 (intra) or 5 (non intra),
					// so the reconstruction divides by 8 and 16. Dividing by twice that
					// halves every AC amplitude and washes the picture out.
					if (is_intra) {
						int q_val = (level * (int)intra_quant[scan_pos] * quant_scale) / 8;
						recon_block[scan_pos] = q_val;
					} else {
						int sign = (level < 0) ? -1 : 1;
						int q_val = ((2 * level + sign) * (int)non_intra_quant[scan_pos] * quant_scale) / 16;
						recon_block[scan_pos] = q_val;
					}
					coeff_idx++;
					full_block = (coeff_idx >= 64);
				}
				// A block that ends on the very last coefficient is still terminated by an
				// end of block code. Leaving it unread makes the next block start one code
				// early and take those two bits for a level 1 coefficient of its own.
				if (full_block) {
					bool esc = false;
					bool error = false;
					if (!DecodeDCTCoeff(reader, run, level, false, esc, error) && error) {
						return false;	// broken slice: let the caller conceal
					}
				}

				// 2D IDCT
				Compute2D_IDCT(recon_block);

				// Put block into plane
				if (block_idx < 4) {
					int bx = (block_idx & 1) * 8;
					int by = (block_idx >> 1) * 8;
					uint8* dst = cur_y + (mb_y * 16 + by) * stride_y + (mb_x * 16 + bx);
					for (int r = 0; r < 8; ++r) {
						for (int c = 0; c < 8; ++c) {
							int val = is_intra ? recon_block[r * 8 + c] : (int)dst[r * stride_y + c] + recon_block[r * 8 + c];
							dst[r * stride_y + c] = (uint8)Clamp255(val);
						}
					}
				} else if (block_idx == 4) { // U (Cb)
					uint8* dst = cur_u + (mb_y * 8) * stride_uv + (mb_x * 8);
					for (int r = 0; r < 8; ++r) {
						for (int c = 0; c < 8; ++c) {
							int val = is_intra ? recon_block[r * 8 + c] : (int)dst[r * stride_uv + c] + recon_block[r * 8 + c];
							dst[r * stride_uv + c] = (uint8)Clamp255(val);
						}
					}
				} else { // V (Cr)
					uint8* dst = cur_v + (mb_y * 8) * stride_uv + (mb_x * 8);
					for (int r = 0; r < 8; ++r) {
						for (int c = 0; c < 8; ++c) {
							int val = is_intra ? recon_block[r * 8 + c] : (int)dst[r * stride_uv + c] + recon_block[r * 8 + c];
							dst[r * stride_uv + c] = (uint8)Clamp255(val);
						}
					}
				}
			}

			return true;
		}

		virtual uni::VideoResult ReadAudioSamples(void* destBuffer, uint32 maxBytes, uint32& bytesRead) override {
			bytesRead = 0;
			if (!destBuffer || maxBytes == 0 || !info.hasAudio || !audio_stream) {
				return uni::VideoResult::EndOfStream;
			}
			uni::AudioResult ares = audio_stream->ReadSamples(destBuffer, maxBytes, bytesRead);
			if (ares == uni::AudioResult::OK) return uni::VideoResult::OK;
			if (ares == uni::AudioResult::EndOfStream) return uni::VideoResult::EndOfStream;
			return uni::VideoResult::Failed;
		}

		virtual uni::VideoResult SeekFrame(uint32 frameIndex) override {
			if (frameIndex >= v_frame_count) {
				curr_vframe = v_frame_count;
				return uni::VideoResult::EndOfStream;
			}
			uint32 keyframe = frameIndex;
			while (keyframe > 0 && v_frames[keyframe].type != MPEG_PICTURE_TYPE_I) {
				keyframe--;
			}
			curr_vframe = keyframe;
			return uni::VideoResult::OK;
		}

		virtual uni::VideoResult SeekTime(uint32 timestampMs) override {
			if (info.durationMs == 0 || v_frame_count == 0) return uni::VideoResult::Failed;
			uint32 target_frame = (uint32)(((uint64)timestampMs * frame_rate_num) / (1000ULL * frame_rate_den));
			if (audio_stream && info.hasAudio && info.audioInfo.format.sample_rate > 0) {
				uint32 sample_idx = (uint32)(((uint64)timestampMs * info.audioInfo.format.sample_rate) / 1000ULL);
				audio_stream->Seek(sample_idx);
			}
			return SeekFrame(target_frame);
		}
	};

} // namespace

namespace uni {

	const char* MPEGCodec::GetName() const {
		return "MPEG-1/2";
	}

	VideoContainerFormat MPEGCodec::GetFormat() const {
		return VideoContainerFormat::MPEG;
	}

	const char* const* MPEGCodec::GetExtensions() const {
		return MPEG_EXTENSIONS;
	}

	VideoResult MPEGCodec::Probe(StorageTrait& storage, bool& matched) const {
		matched = false;
		byte header[4] = { 0 };
		if (!StorageReadExact(storage, 0, header, sizeof(header))) {
			return VideoResult::IoError;
		}
		if (header[0] == 0x00 && header[1] == 0x00 && header[2] == 0x01) {
			uint8 code = header[3];
			if (code == 0xBA || code == 0xB3 || (code >= 0xE0 && code <= 0xEF)) {
				matched = true;
				return VideoResult::OK;
			}
		}
		return VideoResult::OK;
	}

	VideoResult MPEGCodec::ReadInfo(StorageTrait& storage, VideoInfo& outInfo) const {
		MPEGStream stream(&storage);
		if (!stream.Init()) {
			return VideoResult::InvalidFormat;
		}
		return stream.GetInfo(outInfo);
	}

	VideoResult MPEGCodec::OpenStream(
		StorageTrait& storage,
		IVideoStream*& outStream,
		trait::Malloc& allocator
	) const {
		(void)allocator;
		outStream = nullptr;
		MPEGStream* stream = new MPEGStream(&storage);
		if (!stream) {
			return VideoResult::OutOfMemory;
		}
		if (!stream->Init()) {
			delete stream;
			return VideoResult::InvalidFormat;
		}
		outStream = stream;
		return VideoResult::OK;
	}

} // namespace uni

#endif // _INC_CPP
