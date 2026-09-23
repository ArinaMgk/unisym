// ASCII C/C++ TAB4 CRLF
// Docutitle: MPEG-4 Part 2 (Visual) Video Decoder - Simple / Advanced Simple Profile
// Attribute: Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0; Dosconio Mecocoa, BSD 3-Clause License
//
// Scope
// -----
// Rectangular video object, I-VOP and P-VOP, 16x16 / 4MV inter modes, half-pel motion
// compensation, intra DC and AC coefficient prediction, H.263-style inverse quantization
// (vol_quant_type == 0), MPEG-4 video packet (resync marker) headers.
// B-VOP, sprite / GMC, data partitioning, RVLC, interlacing, quarter-pel and
// MPEG-style quantization (vol_quant_type == 1) are *detected* and refused: the frame is
// dropped instead of being decoded into garbage.
//
// Bitstream notes (the parts that are easy to get wrong)
// -----------------------------------------------------
//  * The VOP header always carries the 3-bit intra_dc_vlc_thr - for I *and* P VOPs
//    (ISO/IEC 14496-2, video_object_plane()). Omitting it for P-VOPs misaligns the whole
//    macroblock layer by three bits, which is what makes the picture turn into noise.
//  * CBPY is transmitted inverted for non-intra macroblocks: cbpy = vlc ^ 0x0F; intra
//    macroblocks use the plain value.
//  * Intra DC is coded with the DC VLC tables (B-12 / B-13) and is *differential*; the
//    predictor comes from the left / top neighbour (gradient rule) and is 1024 for
//    neighbours outside the VOP or in a non-intra macroblock.
//  * AC prediction uses the neighbour's first column (prediction from the left) or first
//    row (from the top), rescaled when the neighbour's quantiser differs. The scan order
//    changes accordingly (alternate vertical / alternate horizontal).
//  * TCOEF has two separate tables (intra: B-16, non-intra: H.263 table 14) and three
//    escape modes; escape mode 3 is last(1) run(6) marker(1) level(12) marker(1).
//
// The VLC tables below are generated from, and validated against, the tables of
// ISO/IEC 14496-2 Annex B and Xvid (src/bitstream/vlc_codes.*). 
// Never hand - edit them : a single wrong code or length silently destroys the whole picture.

#include "../../../../inc/c/format/video/MPEG4.h"
#include "../../../../inc/c/algorithm/dct.h"
#include "../../../../inc/c/ustring.h"
#include <stdlib.h>

#if defined(MPEG4_DIAGNOSTICS)
#include <stdio.h>
#define MPEG4_TRACE(...) do { if (s_trace_enabled) { printf(__VA_ARGS__); fflush(stdout); } } while (0)
static int s_trace_enabled = 0; // enabled by the test harness through MPEG4Decoder_SetTrace()
#else
#define MPEG4_TRACE(...) do { } while (0)
static const int s_trace_enabled = 0;
#endif

#if defined(_INC_CPP)

namespace {

	//=====================================================================================
	// Small helpers
	//=====================================================================================

	static inline int ClampByte(int val) {
		if (val < 0) return 0;
		if (val > 255) return 255;
		return val;
	}

	static inline int Median3(int a, int b, int c) {
		if (a > b) {
			if (b > c) return b;
			return (a > c) ? c : a;
		} else {
			if (a > c) return a;
			return (b > c) ? c : b;
		}
	}

	static inline int AbsInt(int v) {
		return v < 0 ? -v : v;
	}

	// two's complement sign extension of the low `bits` bits of val
	static inline int SignExtend(int val, int bits) {
		const int shift = 32 - bits;
		return (int)(((uint32)val << shift)) >> shift;
	}

	// division rounded to nearest (ISO/IEC 14496-2 "DIV with rounding")
	static inline int RoundedDiv(int a, int b) {
		return (a >= 0) ? ((a + (b >> 1)) / b) : ((a - (b >> 1)) / b);
	}

	//=====================================================================================
	// VLC tables (ISO/IEC 14496-2 Annex B)
	//=====================================================================================

	// ---- MCBPC for I-VOPs (Table B-3); symbol = cbpc | (dquant << 2) | (stuffing << 3)
	static const uint8 s_mcbpc_intra_code[9] = { 1, 1, 2, 3, 1, 1, 2, 3, 1 };
	static const uint8 s_mcbpc_intra_bits[9] = { 1, 3, 3, 3, 4, 6, 6, 6, 9 };

	// ---- MCBPC for P-VOPs (Table B-4); bits == 0 marks an unused symbol;
	// symbol bits: 0-1 = cbpc, 2 = intra, 3 = dquant, 4 = 4 motion vectors, 20 = stuffing
	static const uint16 s_mcbpc_inter_code[28] = {
		 1,  3,  2,  5,  3,  4,  3,
		 3,  3,  7,  6,  5,  4,  4,
		 3,  2,  2,  5,  4,  5,  1,
		 0,  0,  0,  2, 12, 14, 15,
	};
	static const uint8 s_mcbpc_inter_bits[28] = {
		 1,  4,  4,  6,  5,  8,  8,
		 7,  3,  7,  7,  9,  6,  9,
		 9,  9,  3,  7,  7,  8,  9,
		 0,  0,  0, 11, 13, 13, 13,
	};

	// ---- CBPY (Table B-5); index = coded block pattern, value = (code, length)
	static const uint16 s_cbpy_tab[16][2] = {
		{0x3,4}, {0x5,5}, {0x4,5}, {0x9,4},
		{0x3,5}, {0x7,4}, {0x2,6}, {0xB,4},
		{0x2,5}, {0x3,6}, {0x5,4}, {0xA,4},
		{0x4,4}, {0x8,4}, {0x6,4}, {0x3,2},
	};

	// ---- Motion vector difference VLC (Table B-10); index = magnitude code
	static const uint16 s_mv_tab[33][2] = {
		{0x1,1}, {0x1,2}, {0x1,3}, {0x1,4}, {0x3,6}, {0x5,7},
		{0x4,7}, {0x3,7}, {0xB,9}, {0xA,9}, {0x9,9}, {0x11,10},
		{0x10,10}, {0xF,10}, {0xE,10}, {0xD,10}, {0xC,10}, {0xB,10},
		{0xA,10}, {0x9,10}, {0x8,10}, {0x7,10}, {0x6,10}, {0x5,10},
		{0x4,10}, {0x7,11}, {0x6,11}, {0x5,11}, {0x4,11}, {0x3,11},
		{0x2,11}, {0x3,12}, {0x2,12},
	};

	// ---- DC coefficient size VLC (Tables B-12 / B-13); index = dc_size
	static const uint8 s_dc_lum_tab[13][2] = {
		{0x3,3}, {0x3,2}, {0x2,2}, {0x2,3}, {0x1,3},
		{0x1,4}, {0x1,5}, {0x1,6}, {0x1,7}, {0x1,8},
		{0x1,9}, {0x1,10}, {0x1,11},
	};
	static const uint8 s_dc_chrom_tab[13][2] = {
		{0x3,2}, {0x2,2}, {0x1,2}, {0x1,3}, {0x1,4},
		{0x1,5}, {0x1,6}, {0x1,7}, {0x1,8}, {0x1,9},
		{0x1,10}, {0x1,11}, {0x1,12},
	};

	// ---- TCOEF for intra blocks (Table B-16): 102 (last,run,level) entries + escape code
	static const uint16 s_tcoef_intra_tab[103][2] = {
		{0x2,2}, {0x6,3}, {0xF,4}, {0xD,5}, {0xC,5},
		{0x15,6}, {0x13,6}, {0x12,6}, {0x17,7}, {0x1F,8},
		{0x1E,8}, {0x1D,8}, {0x25,9}, {0x24,9}, {0x23,9},
		{0x21,9}, {0x21,10}, {0x20,10}, {0xF,10}, {0xE,10},
		{0x7,11}, {0x6,11}, {0x20,11}, {0x21,11}, {0x50,12},
		{0x51,12}, {0x52,12}, {0xE,4}, {0x14,6}, {0x16,7},
		{0x1C,8}, {0x20,9}, {0x1F,9}, {0xD,10}, {0x22,11},
		{0x53,12}, {0x55,12}, {0xB,5}, {0x15,7}, {0x1E,9},
		{0xC,10}, {0x56,12}, {0x11,6}, {0x1B,8}, {0x1D,9},
		{0xB,10}, {0x10,6}, {0x22,9}, {0xA,10}, {0xD,6},
		{0x1C,9}, {0x8,10}, {0x12,7}, {0x1B,9}, {0x54,12},
		{0x14,7}, {0x1A,9}, {0x57,12}, {0x19,8}, {0x9,10},
		{0x18,8}, {0x23,11}, {0x17,8}, {0x19,9}, {0x18,9},
		{0x7,10}, {0x58,12}, {0x7,4}, {0xC,6}, {0x16,8},
		{0x17,9}, {0x6,10}, {0x5,11}, {0x4,11}, {0x59,12},
		{0xF,6}, {0x16,9}, {0x5,10}, {0xE,6}, {0x4,10},
		{0x11,7}, {0x24,11}, {0x10,7}, {0x25,11}, {0x13,7},
		{0x5A,12}, {0x15,8}, {0x5B,12}, {0x14,8}, {0x13,8},
		{0x1A,8}, {0x15,9}, {0x14,9}, {0x13,9}, {0x12,9},
		{0x11,9}, {0x26,11}, {0x27,11}, {0x5C,12}, {0x5D,12},
		{0x5E,12}, {0x5F,12}, {0x3,7},
	};
	static const int8 s_tcoef_intra_run[102] = {
		  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   1,   1,   1,   1,
		  1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   3,   3,   3,   3,   4,   4,
		  4,   5,   5,   5,   6,   6,   6,   7,   7,   7,   8,   8,   9,   9,  10,  11,
		 12,  13,  14,   0,   0,   0,   0,   0,   0,   0,   0,   1,   1,   1,   2,   2,
		  3,   3,   4,   4,   5,   5,   6,   6,   7,   8,   9,  10,  11,  12,  13,  14,
		 15,  16,  17,  18,  19,  20,
	};
	static const int8 s_tcoef_intra_level[102] = {
	  1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,  16,
	 17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,   1,   2,   3,   4,   5,
	  6,   7,   8,   9,  10,   1,   2,   3,   4,   5,   1,   2,   3,   4,   1,   2,
	  3,   1,   2,   3,   1,   2,   3,   1,   2,   3,   1,   2,   1,   2,   1,   1,
	  1,   1,   1,   1,   2,   3,   4,   5,   6,   7,   8,   1,   2,   3,   1,   2,
	  1,   2,   1,   2,   1,   2,   1,   2,   1,   1,   1,   1,   1,   1,   1,   1,
	  1,   1,   1,   1,   1,   1,
};

	// ---- TCOEF for non-intra blocks (H.263 table 14 / MPEG-4 B-17): 102 entries + escape
	static const uint16 s_tcoef_inter_tab[103][2] = {
		{0x2,2}, {0xF,4}, {0x15,6}, {0x17,7}, {0x1F,8},
		{0x25,9}, {0x24,9}, {0x21,10}, {0x20,10}, {0x7,11},
		{0x6,11}, {0x20,11}, {0x6,3}, {0x14,6}, {0x1E,8},
		{0xF,10}, {0x21,11}, {0x50,12}, {0xE,4}, {0x1D,8},
		{0xE,10}, {0x51,12}, {0xD,5}, {0x23,9}, {0xD,10},
		{0xC,5}, {0x22,9}, {0x52,12}, {0xB,5}, {0xC,10},
		{0x53,12}, {0x13,6}, {0xB,10}, {0x54,12}, {0x12,6},
		{0xA,10}, {0x11,6}, {0x9,10}, {0x10,6}, {0x8,10},
		{0x16,7}, {0x55,12}, {0x15,7}, {0x14,7}, {0x1C,8},
		{0x1B,8}, {0x21,9}, {0x20,9}, {0x1F,9}, {0x1E,9},
		{0x1D,9}, {0x1C,9}, {0x1B,9}, {0x1A,9}, {0x22,11},
		{0x23,11}, {0x56,12}, {0x57,12}, {0x7,4}, {0x19,9},
		{0x5,11}, {0xF,6}, {0x4,11}, {0xE,6}, {0xD,6},
		{0xC,6}, {0x13,7}, {0x12,7}, {0x11,7}, {0x10,7},
		{0x1A,8}, {0x19,8}, {0x18,8}, {0x17,8}, {0x16,8},
		{0x15,8}, {0x14,8}, {0x13,8}, {0x18,9}, {0x17,9},
		{0x16,9}, {0x15,9}, {0x14,9}, {0x13,9}, {0x12,9},
		{0x11,9}, {0x7,10}, {0x6,10}, {0x5,10}, {0x4,10},
		{0x24,11}, {0x25,11}, {0x26,11}, {0x27,11}, {0x58,12},
		{0x59,12}, {0x5A,12}, {0x5B,12}, {0x5C,12}, {0x5D,12},
		{0x5E,12}, {0x5F,12}, {0x3,7},
	};
	static const int8 s_tcoef_inter_run[102] = {
		  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   1,   1,   1,
		  1,   1,   2,   2,   2,   2,   3,   3,   3,   4,   4,   4,   5,   5,   5,   6,
		  6,   6,   7,   7,   8,   8,   9,   9,  10,  10,  11,  12,  13,  14,  15,  16,
		 17,  18,  19,  20,  21,  22,  23,  24,  25,  26,   0,   0,   0,   1,   1,   2,
		  3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,  16,  17,  18,
		 19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,  33,  34,
		 35,  36,  37,  38,  39,  40,
	};
	static const int8 s_tcoef_inter_level[102] = {
		  1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,   1,   2,   3,   4,
		  5,   6,   1,   2,   3,   4,   1,   2,   3,   1,   2,   3,   1,   2,   3,   1,
		  2,   3,   1,   2,   1,   2,   1,   2,   1,   2,   1,   1,   1,   1,   1,   1,
		  1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   3,   1,   2,   1,
		  1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
		  1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
		  1,   1,   1,   1,   1,   1,
	};

	// index of the first entry whose `last` flag is 1
	#define MPEG4_TCOEF_INTRA_LAST_SPLIT 67
	#define MPEG4_TCOEF_INTER_LAST_SPLIT 58
	#define MPEG4_TCOEF_ENTRIES          102
	#define MPEG4_TCOEF_ESCAPE_INDEX     102

	// ---- Scan orders
	static const uint8 s_zigzag_scan[64] = {
		  0,   1,   8,  16,   9,   2,   3,  10,  17,  24,  32,  25,  18,  11,   4,   5,
		 12,  19,  26,  33,  40,  48,  41,  34,  27,  20,  13,   6,   7,  14,  21,  28,
		 35,  42,  49,  56,  57,  50,  43,  36,  29,  22,  15,  23,  30,  37,  44,  51,
		 58,  59,  52,  45,  38,  31,  39,  46,  53,  60,  61,  54,  47,  55,  62,  63,
	};
	static const uint8 s_alt_h_scan[64] = {
		  0,   1,   2,   3,   8,   9,  16,  17,  10,  11,   4,   5,   6,   7,  15,  14,
		 13,  12,  19,  18,  24,  25,  32,  33,  26,  27,  20,  21,  22,  23,  28,  29,
		 30,  31,  34,  35,  40,  41,  48,  49,  42,  43,  36,  37,  38,  39,  44,  45,
		 46,  47,  50,  51,  56,  57,  58,  59,  52,  53,  54,  55,  60,  61,  62,  63,
	};
	static const uint8 s_alt_v_scan[64] = {
		  0,   8,  16,  24,   1,   9,   2,  10,  17,  25,  32,  40,  48,  56,  57,  49,
		 41,  33,  26,  18,   3,  11,   4,  12,  19,  27,  34,  42,  50,  58,  35,  43,
		 51,  59,  20,  28,   5,  13,   6,  14,  21,  29,  36,  44,  52,  60,  37,  45,
		 53,  61,  22,  30,   7,  15,  23,  31,  38,  46,  54,  62,  39,  47,  55,  63,
	};

	// ---- DC scaler tables (Table B-12 / B-13 equivalent, see 14496-2 7.4.3)
	static const uint8 s_y_dc_scale[32] = {
		0, 8, 8, 8, 8,10,12,14,16,17,18,19,20,21,22,23,
		24,25,26,27,28,29,30,31,32,34,36,38,40,42,44,46
	};
	static const uint8 s_c_dc_scale[32] = {
		0, 8, 8, 8, 8, 9, 9,10,10,11,11,12,12,13,13,14,
		14,15,15,16,16,17,17,18,18,19,20,21,22,23,24,25
	};
	// intra_dc_vlc_thr -> threshold: intra DC uses the DC VLC while qscale < threshold
	static const uint8 s_dc_threshold[8] = { 99, 13, 15, 17, 19, 21, 23, 0 };
	// dquant code -> delta
	static const int8 s_quant_tab[4] = { -1, -2, 1, 2 };
	// rounding table used when the chroma vector of a 4MV macroblock is derived
	static const uint8 s_chroma_roundtab[16] = {
		0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1
	};

	// Derived limits of an RL table, needed by escape modes 1 and 2.
	struct MPEG4RLLimits {
		int8 max_level[41]; // by run
		int8 max_run[28];   // by level
	};

	static void BuildRLLimits(const int8* run, const int8* level, int last_split, MPEG4RLLimits out[2]) {
		for (int L = 0; L < 2; ++L) {
			for (int i = 0; i < 41; ++i) out[L].max_level[i] = 0;
			for (int i = 0; i < 28; ++i) out[L].max_run[i] = 0;
			const int begin = (L == 0) ? 0 : last_split;
			const int end = (L == 0) ? last_split : MPEG4_TCOEF_ENTRIES;
			for (int i = begin; i < end; ++i) {
				const int r = run[i];
				const int lv = level[i];
				if (r >= 0 && r < 41 && lv > out[L].max_level[r]) out[L].max_level[r] = (int8)lv;
				if (lv >= 0 && lv < 28 && r > out[L].max_run[lv]) out[L].max_run[lv] = (int8)r;
			}
		}
	}

	//=====================================================================================
	// Bit reader (MSB first, zero padded past the end of the buffer)
	//=====================================================================================

	class MPEG4BitReader {
	private:
		const byte* m_data;
		size_t      m_size;
		uint64      m_pos; // bit position

	public:
		MPEG4BitReader(const byte* data, size_t size)
			: m_data(data), m_size(size), m_pos(0) {
		}

		inline uint32 PeekBits(int n) {
			if (n <= 0) return 0;
			uint32 v = 0;
			size_t byte_i = (size_t)(m_pos >> 3);
			int bit_off = (int)(m_pos & 7);
			int got = 0;
			while (got < n) {
				const uint32 byte_v = (byte_i < m_size) ? (uint32)m_data[byte_i] : 0u;
				const int avail = 8 - bit_off;
				int take = n - got;
				if (take > avail) take = avail;
				const uint32 chunk = (byte_v >> (avail - take)) & ((1u << take) - 1u);
				v = (v << take) | chunk;
				got += take;
				bit_off = 0;
				++byte_i;
			}
			return v;
		}

		inline void SkipBits(int n) {
			if (n > 0) m_pos += (uint64)n;
		}

		inline uint32 ReadBits(int n) {
			const uint32 val = PeekBits(n);
			SkipBits(n);
			return val;
		}

		inline bool ReadBool() {
			return ReadBits(1) != 0;
		}

		inline void ByteAlign() {
			m_pos = (m_pos + 7) & ~(uint64)7;
		}

		inline uint64 BitPos() const { return m_pos; }
		inline void SetBitPos(uint64 pos) { m_pos = pos; }
		inline bool HasData() const { return (m_pos >> 3) < m_size; }
		inline uint64 BitsLeft() const {
			const uint64 total = (uint64)m_size * 8;
			return (total > m_pos) ? (total - m_pos) : 0;
		}
	};

	//=====================================================================================
	// Generic VLC decoding over (code, length) pairs
	//=====================================================================================

	// returns the symbol index, or -1 when no code matches
	static int DecodeVLC(MPEG4BitReader& r, const uint16 tab[][2], int count) {
		for (int i = 0; i < count; ++i) {
			const int len = tab[i][1];
			if (len <= 0) continue;
			if (r.PeekBits(len) == tab[i][0]) {
				r.SkipBits(len);
				return i;
			}
		}
		return -1;
	}

	static int DecodeMCBPCIntra(MPEG4BitReader& r) {
		for (int i = 0; i < 9; ++i) {
			const int len = s_mcbpc_intra_bits[i];
			if (r.PeekBits(len) == s_mcbpc_intra_code[i]) {
				r.SkipBits(len);
				return i;
			}
		}
		return -1;
	}

	static int DecodeMCBPCInter(MPEG4BitReader& r) {
		for (int i = 0; i < 28; ++i) {
			const int len = s_mcbpc_inter_bits[i];
			if (len <= 0) continue;
			if (r.PeekBits(len) == s_mcbpc_inter_code[i]) {
				r.SkipBits(len);
				return i;
			}
		}
		return -1;
	}

	//=====================================================================================
	// Decoder
	//=====================================================================================

	struct MPEG4MotionVector {
		int x;
		int y;
	};

	static const int MPEG4_MOTION_OFFSET[4] = { 2, 1, 1, -1 };

	class MPEG4DecoderImpl {
	public:
		// --- configuration ---
		uint32 default_w;
		uint32 default_h;

		// --- geometry ---
		uint32 width;
		uint32 height;
		uint32 mb_width;
		uint32 mb_height;
		uint32 luma_stride;
		uint32 chroma_stride;

		// --- pictures ---
		uint8* cur_y;
		uint8* cur_u;
		uint8* cur_v;
		uint8* ref_y;
		uint8* ref_u;
		uint8* ref_v;

		// --- prediction state, indexed on the 8x8 block grid of the current VOP ---
		uint32 dc_stride;   // luma, in blocks: 2*mb_width + 2 (1 block border on the left)
		uint32 chr_stride;  // chroma, in blocks: mb_width + 2
		uint32 ac_stride;   // luma, in int16: (2*mb_width + 2) * 16
		uint32 ac_chr_stride;
		uint32 mv_stride;   // luma motion vector grid, in blocks
		int16* dc_y;
		int16* dc_cb;
		int16* dc_cr;
		int16* ac_y;
		int16* ac_cb;
		int16* ac_cr;
		int16* mv_x;
		int16* mv_y;
		uint8* mb_qp;       // per macroblock quantiser of the current VOP

		MPEG4RLLimits rl_intra[2];
		MPEG4RLLimits rl_inter[2];

		// --- headers ---
		MPEG4VOLHeader vol;
		MPEG4VOPHeader vop;
		bool has_vol;
		bool vol_unsupported;

		// --- VOL derived flags ---
		bool progressive;
		bool alternate_scan;
		bool quarter_sample;
		bool resync_enabled;
		bool data_partitioned;
		bool mpeg_quant;    // vol_quant_type == 1
		uint8 picture_type;

		// --- per VOP state ---
		int  cur_qp;
		bool use_intra_dc_vlc;
		bool ac_pred;
		int  dc_dir[6];
		int  mb_x;
		int  mb_y;
		// slice (video packet) state: prediction does not cross a video packet boundary
		int  resync_mb_x;
		int  resync_mb_y;
		bool first_slice_line;

		// --- diagnostics ---
		uint64 diag_bits_used;
		int    diag_errors;
		int    diag_mb_count;
		int    diag_residual_mb; // macroblocks that carried coefficients
		int    diag_mv_mb;       // macroblocks that carried a non zero motion vector
		bool   trace_now;

		MPEG4DecoderImpl(int dw = 0, int dh = 0)
			: default_w((uint32)(dw > 0 ? dw : 0)),
			  default_h((uint32)(dh > 0 ? dh : 0)),
			  width(0), height(0), mb_width(0), mb_height(0),
			  luma_stride(0), chroma_stride(0),
			  cur_y(nullptr), cur_u(nullptr), cur_v(nullptr),
			  ref_y(nullptr), ref_u(nullptr), ref_v(nullptr),
			  dc_stride(0), chr_stride(0), ac_stride(0), ac_chr_stride(0), mv_stride(0),
			  dc_y(nullptr), dc_cb(nullptr), dc_cr(nullptr),
			  ac_y(nullptr), ac_cb(nullptr), ac_cr(nullptr),
			  mv_x(nullptr), mv_y(nullptr), mb_qp(nullptr),
			  has_vol(false), vol_unsupported(false),
			  progressive(true), alternate_scan(false), quarter_sample(false),
			  resync_enabled(false), data_partitioned(false), mpeg_quant(false),
			  picture_type(MPEG4_VOP_TYPE_I),
			  cur_qp(1), use_intra_dc_vlc(true), ac_pred(false),
			  mb_x(0), mb_y(0),
			  resync_mb_x(0), resync_mb_y(0), first_slice_line(true),
			  diag_bits_used(0), diag_errors(0), diag_mb_count(0),
			  diag_residual_mb(0), diag_mv_mb(0), trace_now(false) {
			MemSet(&vol, 0, sizeof(vol));
			MemSet(&vop, 0, sizeof(vop));
			MemSet(dc_dir, 0, sizeof(dc_dir));
			BuildRLLimits(s_tcoef_intra_run, s_tcoef_intra_level, MPEG4_TCOEF_INTRA_LAST_SPLIT, rl_intra);
			BuildRLLimits(s_tcoef_inter_run, s_tcoef_inter_level, MPEG4_TCOEF_INTER_LAST_SPLIT, rl_inter);
			if (default_w > 0 && default_h > 0) {
				AllocateBuffers(default_w, default_h);
				vol.video_object_layer_width = (uint16)default_w;
				vol.video_object_layer_height = (uint16)default_h;
				vol.vop_time_increment_resolution = 30000;
			}
		}

		~MPEG4DecoderImpl() {
			FreeBuffers();
		}

		void FreeBuffers() {
			if (cur_y) { free(cur_y); cur_y = nullptr; }
			if (cur_u) { free(cur_u); cur_u = nullptr; }
			if (cur_v) { free(cur_v); cur_v = nullptr; }
			if (ref_y) { free(ref_y); ref_y = nullptr; }
			if (ref_u) { free(ref_u); ref_u = nullptr; }
			if (ref_v) { free(ref_v); ref_v = nullptr; }
			if (dc_y) { free(dc_y); dc_y = nullptr; }
			if (dc_cb) { free(dc_cb); dc_cb = nullptr; }
			if (dc_cr) { free(dc_cr); dc_cr = nullptr; }
			if (ac_y) { free(ac_y); ac_y = nullptr; }
			if (ac_cb) { free(ac_cb); ac_cb = nullptr; }
			if (ac_cr) { free(ac_cr); ac_cr = nullptr; }
			if (mv_x) { free(mv_x); mv_x = nullptr; }
			if (mv_y) { free(mv_y); mv_y = nullptr; }
			if (mb_qp) { free(mb_qp); mb_qp = nullptr; }
		}

		bool AllocateBuffers(uint32 w, uint32 h) {
			if (width == w && height == h && cur_y != nullptr) return true;
			FreeBuffers();

			width = w;
			height = h;
			mb_width = (w + 15) / 16;
			mb_height = (h + 15) / 16;
			if (mb_width == 0 || mb_height == 0) return false;

			luma_stride = mb_width * 16;
			chroma_stride = mb_width * 8;

			dc_stride = mb_width * 2 + 2;
			chr_stride = mb_width + 2;
			ac_stride = dc_stride * 16;
			ac_chr_stride = chr_stride * 16;
			mv_stride = mb_width * 2 + 2;

			const size_t luma_size = (size_t)luma_stride * mb_height * 16;
			const size_t chroma_size = (size_t)chroma_stride * mb_height * 8;
			const size_t dc_y_size = (size_t)dc_stride * (mb_height * 2 + 2);
			const size_t dc_c_size = (size_t)chr_stride * (mb_height + 2);
			const size_t ac_y_size = (size_t)ac_stride * (mb_height * 2 + 2);
			const size_t ac_c_size = (size_t)ac_chr_stride * (mb_height + 2);
			const size_t mv_size = (size_t)mv_stride * (mb_height * 2 + 2);

			cur_y = (uint8*)malloc(luma_size);
			cur_u = (uint8*)malloc(chroma_size);
			cur_v = (uint8*)malloc(chroma_size);
			ref_y = (uint8*)malloc(luma_size);
			ref_u = (uint8*)malloc(chroma_size);
			ref_v = (uint8*)malloc(chroma_size);
			dc_y = (int16*)malloc(dc_y_size * sizeof(int16));
			dc_cb = (int16*)malloc(dc_c_size * sizeof(int16));
			dc_cr = (int16*)malloc(dc_c_size * sizeof(int16));
			ac_y = (int16*)malloc(ac_y_size * sizeof(int16));
			ac_cb = (int16*)malloc(ac_c_size * sizeof(int16));
			ac_cr = (int16*)malloc(ac_c_size * sizeof(int16));
			mv_x = (int16*)malloc(mv_size * sizeof(int16));
			mv_y = (int16*)malloc(mv_size * sizeof(int16));
			mb_qp = (uint8*)malloc((size_t)mb_width * mb_height);

			if (!cur_y || !cur_u || !cur_v || !ref_y || !ref_u || !ref_v ||
				!dc_y || !dc_cb || !dc_cr || !ac_y || !ac_cb || !ac_cr ||
				!mv_x || !mv_y || !mb_qp) {
				FreeBuffers();
				return false;
			}

			MemSet(cur_y, 0, luma_size);
			MemSet(cur_u, 128, chroma_size);
			MemSet(cur_v, 128, chroma_size);
			MemSet(ref_y, 0, luma_size);
			MemSet(ref_u, 128, chroma_size);
			MemSet(ref_v, 128, chroma_size);
			ResetPredictionState();
			return true;
		}

		// All predictors are 1024 (neutral DC) / 0 (no AC, no motion) outside the VOP and
		// for non-intra macroblocks, so the frame border is simply initialised that way.
		void ResetPredictionState() {
			const size_t dc_y_size = (size_t)dc_stride * (mb_height * 2 + 2);
			const size_t dc_c_size = (size_t)chr_stride * (mb_height + 2);
			const size_t ac_y_size = (size_t)ac_stride * (mb_height * 2 + 2);
			const size_t ac_c_size = (size_t)ac_chr_stride * (mb_height + 2);
			const size_t mv_size = (size_t)mv_stride * (mb_height * 2 + 2);
			for (size_t i = 0; i < dc_y_size; ++i) dc_y[i] = 1024;
			for (size_t i = 0; i < dc_c_size; ++i) { dc_cb[i] = 1024; dc_cr[i] = 1024; }
			MemSet(ac_y, 0, ac_y_size * sizeof(int16));
			MemSet(ac_cb, 0, ac_c_size * sizeof(int16));
			MemSet(ac_cr, 0, ac_c_size * sizeof(int16));
			MemSet(mv_x, 0, mv_size * sizeof(int16));
			MemSet(mv_y, 0, mv_size * sizeof(int16));
			MemSet(mb_qp, 0, (size_t)mb_width * mb_height);
		}

		//=================================================================================
		// Block grid accessors
		//=================================================================================

		inline uint32 LumaDcIndex(int blk_x, int blk_y) const {
			return (uint32)(blk_y + 1) * dc_stride + (uint32)(blk_x + 1);
		}
		inline uint32 ChromaDcIndex() const {
			return (uint32)(mb_y + 1) * chr_stride + (uint32)(mb_x + 1);
		}
		inline int16* LumaAcPtr(int blk_x, int blk_y) const {
			return ac_y + (size_t)LumaDcIndex(blk_x, blk_y) * 16;
		}
		inline int16* ChromaAcPtr(bool cr) const {
			const uint32 idx = ChromaDcIndex();
			return (cr ? ac_cr : ac_cb) + (size_t)idx * 16;
		}

		//=================================================================================
		// VOL / VOP headers
		//=================================================================================

		bool ParseVOL(MPEG4BitReader& reader, uint32 sc) {
			vol.start_code = sc;
			vol.random_accessible_vol = (uint8)reader.ReadBits(1);
			vol.video_object_type_indication = (uint8)reader.ReadBits(8);
			vol.is_object_layer_identifier = (uint8)reader.ReadBits(1);
			int verid = 1;
			if (vol.is_object_layer_identifier) {
				vol.video_object_layer_verid = (uint8)reader.ReadBits(4);
				verid = vol.video_object_layer_verid;
				vol.video_object_layer_priority = (uint8)reader.ReadBits(3);
			}
			vol.aspect_ratio_info = (uint8)reader.ReadBits(4);
			if (vol.aspect_ratio_info == MPEG4_ASPECT_RATIO_EXTENDED) {
				vol.par_width = (uint16)reader.ReadBits(8);
				vol.par_height = (uint16)reader.ReadBits(8);
			}
			vol.vol_control_parameters = (uint8)reader.ReadBits(1);
			if (vol.vol_control_parameters) {
				reader.ReadBits(2); // chroma_format
				reader.ReadBits(1); // low_delay
				if (reader.ReadBool()) { // vbv_parameters
					reader.ReadBits(15); reader.ReadBits(1);
					reader.ReadBits(15); reader.ReadBits(1);
					reader.ReadBits(15); reader.ReadBits(1);
					reader.ReadBits(3);
					reader.ReadBits(11); reader.ReadBits(1);
					reader.ReadBits(15); reader.ReadBits(1);
				}
			}
			vol.video_object_layer_shape = (uint8)reader.ReadBits(2);
			if (vol.video_object_layer_shape == MPEG4_SHAPE_GRAYSCALE && verid != 1) {
				reader.ReadBits(4);
			}
			reader.ReadBits(1); // marker bit
			vol.vop_time_increment_resolution = (uint16)reader.ReadBits(16);
			reader.ReadBits(1); // marker bit
			vol.fixed_vop_rate = (uint8)reader.ReadBits(1);
			if (vol.fixed_vop_rate) {
				uint32 res = vol.vop_time_increment_resolution ? (vol.vop_time_increment_resolution - 1) : 0;
				int time_inc_bits = 0;
				while (res > 0) { res >>= 1; time_inc_bits++; }
				if (time_inc_bits == 0) time_inc_bits = 1;
				vol.fixed_vop_time_increment = (uint16)reader.ReadBits(time_inc_bits);
			}

			progressive = true;
			alternate_scan = false;
			quarter_sample = false;
			resync_enabled = false;
			data_partitioned = false;
			mpeg_quant = false;
			vol_unsupported = false;

			if (vol.video_object_layer_shape != MPEG4_SHAPE_RECTANGULAR) {
				vol_unsupported = true; // only rectangular objects are supported
			}

			if (vol.video_object_layer_shape != MPEG4_SHAPE_BINARY_ONLY) {
				if (vol.video_object_layer_shape == MPEG4_SHAPE_RECTANGULAR) {
					reader.ReadBits(1); // marker
					vol.video_object_layer_width = (uint16)reader.ReadBits(13);
					reader.ReadBits(1); // marker
					vol.video_object_layer_height = (uint16)reader.ReadBits(13);
					reader.ReadBits(1); // marker
				}
				vol.interlaced = (uint8)reader.ReadBits(1);
				progressive = !vol.interlaced;
				vol.obmc_disable = (uint8)reader.ReadBits(1);
				vol.sprite_enable = (uint8)reader.ReadBits(verid == 1 ? 1 : 2);
				if (vol.sprite_enable == 1 || vol.sprite_enable == 2) {
					if (vol.sprite_enable == 1) { // static sprite: sprite size / position
						reader.ReadBits(13); reader.ReadBits(1);
						reader.ReadBits(13); reader.ReadBits(1);
						reader.ReadBits(13); reader.ReadBits(1);
						reader.ReadBits(13); reader.ReadBits(1);
					}
					reader.ReadBits(6); // num_sprite_warping_points
					reader.ReadBits(2); // sprite_warping_accuracy
					reader.ReadBits(1); // sprite_brightness_change
					if (vol.sprite_enable == 1) reader.ReadBits(1); // low_latency_sprite
				}
				vol.not_8_bit = (uint8)reader.ReadBits(1);
				if (vol.not_8_bit) {
					const uint32 quant_precision = reader.ReadBits(4);
					reader.ReadBits(4); // bits_per_pixel
					if (quant_precision != 5) vol_unsupported = true;
				}
				vol.quant_type = (uint8)reader.ReadBits(1);
				if (vol.quant_type) {
					mpeg_quant = true;
					vol.load_intra_quant_mat = (uint8)reader.ReadBits(1);
					if (vol.load_intra_quant_mat) {
						int last = 0;
						int i = 0;
						for (; i < 64; ++i) {
							const int v = (int)reader.ReadBits(8);
							if (v == 0) break;
							last = v;
							vol.intra_quant_mat[s_zigzag_scan[i]] = (uint8)v;
						}
						for (; i < 64; ++i) vol.intra_quant_mat[s_zigzag_scan[i]] = (uint8)last;
					}
					vol.load_nonintra_quant_mat = (uint8)reader.ReadBits(1);
					if (vol.load_nonintra_quant_mat) {
						int last = 0;
						int i = 0;
						for (; i < 64; ++i) {
							const int v = (int)reader.ReadBits(8);
							if (v == 0) break;
							last = v;
							vol.nonintra_quant_mat[s_zigzag_scan[i]] = (uint8)v;
						}
						for (; i < 64; ++i) vol.nonintra_quant_mat[s_zigzag_scan[i]] = (uint8)last;
					}
				}
				if (verid != 1) {
					quarter_sample = reader.ReadBits(1) != 0;
					if (quarter_sample) vol_unsupported = true;
				}
				vol.complexity_estimation_disable = (uint8)reader.ReadBits(1);
				if (!vol.complexity_estimation_disable) {
					// variable length complexity estimation header: skip it conservatively
					vol_unsupported = true;
				}
				vol.resync_marker_disable = (uint8)reader.ReadBits(1);
				resync_enabled = (vol.resync_marker_disable == 0);
				vol.data_partitioned = (uint8)reader.ReadBits(1);
				data_partitioned = (vol.data_partitioned != 0);
				if (vol.data_partitioned) {
					vol.reversible_vlc = (uint8)reader.ReadBits(1);
					vol_unsupported = true; // data partitioning is not implemented
				}
				if (verid != 1) {
					vol.newpred_enable = (uint8)reader.ReadBits(1);
					if (vol.newpred_enable) {
						reader.ReadBits(2);
						reader.ReadBits(1);
						vol_unsupported = true;
					}
					vol.reduced_resolution_vop_enable = (uint8)reader.ReadBits(1);
					if (vol.reduced_resolution_vop_enable) vol_unsupported = true;
				}
				vol.scalability = (uint8)reader.ReadBits(1);
				if (vol.scalability) vol_unsupported = true;
			}

			has_vol = true;
			if (vol_unsupported) return false;

			if (vol.video_object_layer_width > 0 && vol.video_object_layer_height > 0) {
				return AllocateBuffers(vol.video_object_layer_width, vol.video_object_layer_height);
			}
			return true;
		}

		// returns true when the VOP is coded and can be decoded
		bool ParseVOPHeader(MPEG4BitReader& reader, uint32 sc) {
			vop.start_code = sc;
			vop.modulo_time_base = 0;
			vop.vop_coding_type = (uint8)reader.ReadBits(2);
			while (reader.ReadBits(1) != 0) {
				vop.modulo_time_base++;
				if (vop.modulo_time_base > 64) return false;
			}
			reader.ReadBits(1); // marker
			int time_bits = 1;
			if (vol.vop_time_increment_resolution > 0) {
				uint32 res = vol.vop_time_increment_resolution - 1;
				time_bits = 0;
				while (res > 0) { res >>= 1; time_bits++; }
				if (time_bits == 0) time_bits = 1;
			}
			vop.vop_time_increment = reader.ReadBits(time_bits);
			reader.ReadBits(1); // marker
			vop.vop_coded = (uint8)reader.ReadBits(1);
			if (!vop.vop_coded) return false;

			vop.vop_rounding_type = 0;
			if (vop.vop_coding_type == MPEG4_VOP_TYPE_P || vop.vop_coding_type == MPEG4_VOP_TYPE_S) {
				vop.vop_rounding_type = (uint8)reader.ReadBits(1);
			}
			if (vol.video_object_layer_shape != MPEG4_SHAPE_BINARY_ONLY) {
				// this field is present for I, P *and* B VOPs
				vop.intra_dc_vlc_thr = (uint8)reader.ReadBits(3);
				if (!progressive) {
					reader.ReadBits(1); // top_field_first
					alternate_scan = reader.ReadBits(1) != 0;
				}
			}
			if (vop.vop_coding_type == MPEG4_VOP_TYPE_S && (vol.sprite_enable == 1 || vol.sprite_enable == 2)) {
				return false; // sprite trajectory not implemented
			}
			vop.vop_quant = (uint8)reader.ReadBits(5);
			if (vop.vop_quant == 0) return false;
			if (vop.vop_coding_type != MPEG4_VOP_TYPE_I) {
				vop.vop_fcode_forward = (uint8)reader.ReadBits(3);
				if (vop.vop_fcode_forward == 0) return false;
			}
			if (vop.vop_coding_type == MPEG4_VOP_TYPE_B) {
				vop.vop_fcode_backward = (uint8)reader.ReadBits(3);
				if (vop.vop_fcode_backward == 0) return false;
			}
			return true;
		}

		//=================================================================================
		// Intra DC / AC prediction
		//=================================================================================

		int PredictDC(int n, int& dir) const {
			int a, b, c;
			if (n < 4) {
				const int blk_x = mb_x * 2 + (n & 1);
				const int blk_y = mb_y * 2 + (n >> 1);
				const uint32 idx = LumaDcIndex(blk_x, blk_y);
				a = dc_y[idx - 1];
				b = dc_y[idx - 1 - dc_stride];
				c = dc_y[idx - dc_stride];
			} else {
				const int16* g = (n == 4) ? dc_cb : dc_cr;
				const uint32 idx = ChromaDcIndex();
				a = g[idx - 1];
				b = g[idx - 1 - chr_stride];
				c = g[idx - chr_stride];
			}
			// A X predicts from the left, C X from the top.
			// At the beginning of a video packet the neighbours above / left are treated as
			// neutral (1024) exactly like the reference decoder does.
			if (first_slice_line && n != 3) {
				if (n != 2) b = c = 1024;
				if (n != 1 && mb_x == resync_mb_x) b = a = 1024;
			}
			if (mb_x == resync_mb_x && mb_y == resync_mb_y + 1) {
				if (n == 0 || n == 4 || n == 5) b = 1024;
			}
			if (AbsInt(a - b) < AbsInt(b - c)) {
				dir = 1; // top
				return c;
			}
			dir = 0; // left
			return a;
		}

		void StoreDC(int n, int value) {
			if (n < 4) {
				const int blk_x = mb_x * 2 + (n & 1);
				const int blk_y = mb_y * 2 + (n >> 1);
				dc_y[LumaDcIndex(blk_x, blk_y)] = (int16)value;
			} else {
				int16* g = (n == 4) ? dc_cb : dc_cr;
				g[ChromaDcIndex()] = (int16)value;
			}
		}

		static inline int ClipDCValue(int v) {
			if (v < 0) return 0;
			if (v > 2047) return 2047;
			return v;
		}

		// adds the AC prediction and stores this block's first column / row for later blocks
		void ApplyACPrediction(int16* blk, int n, int dir) {
			int16* cur;
			int16* left;
			int16* top;
			if (n < 4) {
				const int blk_x = mb_x * 2 + (n & 1);
				const int blk_y = mb_y * 2 + (n >> 1);
				cur = LumaAcPtr(blk_x, blk_y);
				left = cur - 16;
				top = cur - (int)(16 * (mb_width * 2));
			} else {
				cur = ChromaAcPtr(n == 5);
				left = cur - 16;
				top = cur - (int)(16 * mb_width);
			}

			if (ac_pred) {
				const int neighbour_qp = NeighbourMBQuant(n, dir == 0);
				const bool same_scale = (neighbour_qp <= 0) || (neighbour_qp == cur_qp) ||
					(dir == 0 ? (n == 1 || n == 3) : (n == 2 || n == 3));
				if (dir == 0) {
					for (int i = 1; i < 8; ++i) {
						const int v = left[i];
						blk[i << 3] += (int16)(same_scale ? v : RoundedDiv(v * neighbour_qp, cur_qp));
					}
				} else {
					for (int i = 1; i < 8; ++i) {
						const int v = top[8 + i];
						blk[i] += (int16)(same_scale ? v : RoundedDiv(v * neighbour_qp, cur_qp));
					}
				}
			}

			for (int i = 1; i < 8; ++i) cur[i] = blk[i << 3];      // first column
			for (int i = 1; i < 8; ++i) cur[8 + i] = blk[i];       // first row
		}

		// quantiser of the macroblock the prediction is taken from; 0 == none
		int NeighbourMBQuant(int n, bool from_left) const {
			if (from_left) {
				if (mb_x == 0) return 0;
				if (n == 1 || n == 3) return cur_qp; // same macroblock
				return mb_qp[(size_t)mb_y * mb_width + (mb_x - 1)];
			}
			if (mb_y == 0) return 0;
			if (n == 2 || n == 3) return cur_qp;     // same macroblock
			return mb_qp[(size_t)(mb_y - 1) * mb_width + mb_x];
		}

		//=================================================================================
		// Coefficient decoding
		//=================================================================================

		int DecodeDCVlc(MPEG4BitReader& reader, bool luma) {
			const uint8 (*tab)[2] = luma ? s_dc_lum_tab : s_dc_chrom_tab;
			for (int i = 0; i < 13; ++i) {
				const int len = tab[i][1];
				if (reader.PeekBits(len) == tab[i][0]) {
					reader.SkipBits(len);
					return i; // dc_size
				}
			}
			return -1;
		}

		// reads one (run, level, last) triple from the TCOEF tables; returns
		// 0 = ok, 1 = escape, -1 = invalid
		int DecodeTCOEF(MPEG4BitReader& reader, bool intra, int& run, int& level, int& last) {
			const uint16 (*tab)[2] = intra ? s_tcoef_intra_tab : s_tcoef_inter_tab;
			const int8* runs = intra ? s_tcoef_intra_run : s_tcoef_inter_run;
			const int8* levels = intra ? s_tcoef_intra_level : s_tcoef_inter_level;
			const int last_split = intra ? MPEG4_TCOEF_INTRA_LAST_SPLIT : MPEG4_TCOEF_INTER_LAST_SPLIT;

			for (int i = 0; i <= MPEG4_TCOEF_ESCAPE_INDEX; ++i) {
				const int len = tab[i][1];
				if (len <= 0) continue;
				if (reader.PeekBits(len) == tab[i][0]) {
					reader.SkipBits(len);
					if (i == MPEG4_TCOEF_ESCAPE_INDEX) return 1; // escape
					last = (i >= last_split) ? 1 : 0;
					run = runs[i];
					level = levels[i];
					return 0;
				}
			}
			return -1;
		}

		// escape modes: '0' = mode 1, '10' = mode 2, '11' = mode 3
		bool DecodeTCOEFEscape(MPEG4BitReader& reader, bool intra, int& run, int& level, int& last) {
			const MPEG4RLLimits* limits = intra ? rl_intra : rl_inter;
			if (reader.ReadBits(1) == 0) {
				// escape mode 1: the level continues above the largest table level
				int run2 = 0, level2 = 0, last2 = 0;
				if (DecodeTCOEF(reader, intra, run2, level2, last2) != 0) return false;
				if (run2 < 0 || run2 > 40) return false;
				run = run2;
				last = last2;
				level = level2 + limits[last2].max_level[run2];
				if (reader.ReadBits(1)) level = -level;
				return true;
			}
			if (reader.ReadBits(1) == 0) {
				// escape mode 2: the run continues after the largest run of that level
				int run2 = 0, level2 = 0, last2 = 0;
				if (DecodeTCOEF(reader, intra, run2, level2, last2) != 0) return false;
				if (level2 < 0 || level2 > 27) return false;
				run = run2 + limits[last2].max_run[level2] + 1;
				last = last2;
				level = level2;
				if (trace_now) MPEG4_TRACE("     esc-mode2: inner(run=%d,level=%d,last=%d) max_run=%d -> run=%d\n",
					run2, level2, last2, (int)limits[last2].max_run[level2], run);
				if (reader.ReadBits(1)) level = -level;
				return true;
			}
			// escape mode 3: fixed length
			last = (int)reader.ReadBits(1);
			run = (int)reader.ReadBits(6);
			reader.ReadBits(1); // marker
			int raw = (int)reader.ReadBits(12);
			reader.ReadBits(1); // marker
			if (raw >= 2048) raw -= 4096;
			level = raw;
			if (level > 2047) level = 2047;
			if (level < -2048) level = -2048;
			return true;
		}

		//=================================================================================
		// Block decoding: fills quantised coefficients, then dequantises into out32
		//=================================================================================

		bool DecodeBlock(MPEG4BitReader& reader, int16* blk, stdsint* out32,
			int n, bool coded, bool intra, int qp) {
			MemSet(blk, 0, 64 * sizeof(int16));

			const int dc_scale = (n < 4) ? s_y_dc_scale[qp] : s_c_dc_scale[qp];
			int dir = 0;
			int pred_quant = 0;
			int first_coeff = 0;

			if (intra) {
				if (use_intra_dc_vlc) {
					const int dc_size = DecodeDCVlc(reader, n < 4);
					if (dc_size < 0) {
						MPEG4_TRACE("  [dc vlc fail] mb=(%d,%d) n=%d pos=%llu next=%06X\n",
							mb_x, mb_y, n, (unsigned long long)reader.BitPos(), reader.PeekBits(24));
						return false;
					}
					int diff = 0;
					if (dc_size > 0) {
						const int v = (int)reader.ReadBits(dc_size);
						if (v & (1 << (dc_size - 1))) diff = v;
						else diff = v - (1 << dc_size) + 1;
						if (dc_size > 8) reader.ReadBits(1); // marker
					}
					const int pred = PredictDC(n, dir);
					pred_quant = (pred + (dc_scale >> 1)) / dc_scale;
					const int dcq = diff + pred_quant;
					if (trace_now) MPEG4_TRACE("   mb=(%d,%d) n=%d dc_size=%d diff=%d pred=%d pred_q=%d dcq=%d dir=%d pos=%llu\n",
						mb_x, mb_y, n, dc_size, diff, pred, pred_quant, dcq, dir, (unsigned long long)reader.BitPos());
					blk[0] = (int16)dcq;
					StoreDC(n, ClipDCValue(dcq * dc_scale));
					first_coeff = 1;
				} else {
					const int pred = PredictDC(n, dir);
					pred_quant = (pred + (dc_scale >> 1)) / dc_scale;
					first_coeff = 0;
				}
			}

			if (coded) {
				const uint8* scan = s_zigzag_scan;
				if (intra && ac_pred) {
					scan = (dir == 0) ? s_alt_v_scan : (alternate_scan ? s_alt_v_scan : s_alt_h_scan);
				}
				int k = first_coeff;
				while (k < 64) {
					int run = 0, level = 0, last = 0;
					const int res = DecodeTCOEF(reader, intra, run, level, last);
					if (res < 0) {
						MPEG4_TRACE("  [tcoef fail] mb=(%d,%d) n=%d k=%d pos=%llu next=%06X\n",
							mb_x, mb_y, n, k, (unsigned long long)reader.BitPos(), reader.PeekBits(24));
						return false;
					}
					if (res == 1) {
						if (!DecodeTCOEFEscape(reader, intra, run, level, last)) {
							MPEG4_TRACE("  [escape fail] mb=(%d,%d) n=%d k=%d pos=%llu\n",
								mb_x, mb_y, n, k, (unsigned long long)reader.BitPos());
							return false;
						}
					} else if (reader.ReadBits(1)) {
						// the sign bit follows the VLC code (escape modes carry their own)
						level = -level;
					}
					k += run;
					if (trace_now) MPEG4_TRACE("   mb=(%d,%d) n=%d coef k=%d run=%d level=%d last=%d pos=%llu\n",
						mb_x, mb_y, n, k, run, level, last, (unsigned long long)reader.BitPos());
					if (k > 63) {
						MPEG4_TRACE("  [run overflow] mb=(%d,%d) n=%d run=%d pos=%llu\n",
							mb_x, mb_y, n, run, (unsigned long long)reader.BitPos());
						return false;
					}
					blk[scan[k]] = (int16)level;
					if (last) break;
					++k;
				}
			}

			if (intra) {
				if (!use_intra_dc_vlc) {
					const int dcq = pred_quant + blk[0];
					blk[0] = (int16)dcq;
					StoreDC(n, ClipDCValue(dcq * dc_scale));
				}
				ApplyACPrediction(blk, n, dir);
				out32[0] = (stdsint)blk[0] * dc_scale;
				// H.263 style inverse quantisation: |REC| = |LEVEL| * 2 * QP + ((QP - 1) | 1)
				const int qadd = (qp - 1) | 1;
				const stdsint qmul = (stdsint)(qp * 2);
				for (int i = 1; i < 64; ++i) {
					const int v = blk[i];
					out32[i] = v ? (stdsint)(v * qmul + (v > 0 ? qadd : -qadd)) : 0;
				}
			} else {
				const int qadd = (qp - 1) | 1;
				const int qmul = qp * 2;
				for (int i = 0; i < 64; ++i) {
					const int v = blk[i];
					out32[i] = v ? (stdsint)(v * qmul + (v > 0 ? qadd : -qadd)) : 0;
				}
			}
			return true;
		}

		//=================================================================================
		// Motion compensation
		//=================================================================================

		static void MotionCompPlane(
			uint8* dst, uint32 dst_stride,
			const uint8* src, uint32 src_stride,
			int src_x, int src_y,
			int frac_x, int frac_y,
			int blk_w, int blk_h,
			int plane_w, int plane_h,
			int rounding) {
			for (int r = 0; r < blk_h; ++r) {
				const int sy0 = src_y + r;
				const int y0 = sy0 < 0 ? 0 : (sy0 >= plane_h ? plane_h - 1 : sy0);
				const int sy1 = sy0 + 1;
				const int y1 = sy1 < 0 ? 0 : (sy1 >= plane_h ? plane_h - 1 : sy1);
				const uint8* row0 = src + (size_t)y0 * src_stride;
				const uint8* row1 = src + (size_t)y1 * src_stride;
				for (int c = 0; c < blk_w; ++c) {
					const int sx0 = src_x + c;
					const int x0 = sx0 < 0 ? 0 : (sx0 >= plane_w ? plane_w - 1 : sx0);
					const int sx1 = sx0 + 1;
					const int x1 = sx1 < 0 ? 0 : (sx1 >= plane_w ? plane_w - 1 : sx1);
					const int p00 = row0[x0];
					int val;
					if (frac_x == 0 && frac_y == 0) {
						val = p00;
					} else if (frac_y == 0) {
						val = (p00 + (int)row0[x1] + 1 - rounding) >> 1;
					} else if (frac_x == 0) {
						val = (p00 + (int)row1[x0] + 1 - rounding) >> 1;
					} else {
						val = (p00 + (int)row0[x1] + (int)row1[x0] + (int)row1[x1] + 2 - rounding) >> 2;
					}
					dst[(size_t)r * dst_stride + c] = (uint8)val;
				}
			}
		}

		// luma motion compensation for one block of `size` x `size` pixels
		void MotionCompLuma(int dst_x, int dst_y, int size, int mv_x, int mv_y, int rounding) {
			const int src_x = dst_x + (mv_x >> 1);
			const int src_y = dst_y + (mv_y >> 1);
			// the planes are allocated macroblock aligned, so the padding columns / rows hold
			// decoded pixels that a motion vector may legitimately point at
			MotionCompPlane(
				cur_y + (size_t)dst_y * luma_stride + dst_x, luma_stride,
				ref_y, luma_stride,
				src_x, src_y, mv_x & 1, mv_y & 1,
				size, size, (int)(mb_width * 16), (int)(mb_height * 16), rounding);
		}

		// chroma motion compensation of the whole 8x8 chroma block of the macroblock
		void MotionCompChroma(int mv_x, int mv_y, int rounding) {
			const int dst_x = mb_x * 8;
			const int dst_y = mb_y * 8;
			const int src_x = (mb_x * 16 + (mv_x >> 1)) >> 1;
			const int src_y = (mb_y * 16 + (mv_y >> 1)) >> 1;
			const int frac_x = (mv_x & 1) | ((mv_x >> 1) & 1);
			const int frac_y = (mv_y & 1) | ((mv_y >> 1) & 1);
			const int pw = (int)(mb_width * 8);
			const int ph = (int)(mb_height * 8);
			MotionCompPlane(
				cur_u + (size_t)dst_y * chroma_stride + dst_x, chroma_stride,
				ref_u, chroma_stride,
				src_x, src_y, frac_x, frac_y, 8, 8, pw, ph, rounding);
			MotionCompPlane(
				cur_v + (size_t)dst_y * chroma_stride + dst_x, chroma_stride,
				ref_v, chroma_stride,
				src_x, src_y, frac_x, frac_y, 8, 8, pw, ph, rounding);
		}

		void MotionCompSkip(int rounding) {
			MotionCompLuma(mb_x * 16, mb_y * 16, 16, 0, 0, rounding);
			MotionCompChroma(0, 0, rounding);
		}

		//=================================================================================
		// Motion vector prediction (H.263 / MPEG-4 median prediction)
		//=================================================================================

		MPEG4MotionVector PredictMV(int block) {
			const int blk_x = mb_x * 2 + (block & 1);
			const int blk_y = mb_y * 2 + (block >> 1);
			const uint32 idx = (uint32)(blk_y + 1) * mv_stride + (uint32)(blk_x + 1);
			const int ax = mv_x[idx - 1];
			const int ay = mv_y[idx - 1];
			MPEG4MotionVector pred;
			pred.x = ax;
			pred.y = ay;
			if (first_slice_line && block < 3) {
				// First line of the *slice* (a video packet restarts mid frame): the blocks
				// above are outside the slice, so blocks 0 and 1 predict from the left
				// neighbour only, and block 0 has no left neighbour at the slice's left edge.
				if (block == 0) {
					if (mb_x == resync_mb_x) { pred.x = 0; pred.y = 0; return pred; }
					return pred;
				}
				if (block == 1) return pred;
			}
			const int bx = mv_x[idx - mv_stride];
			const int by = mv_y[idx - mv_stride];
			const int cx = mv_x[idx + MPEG4_MOTION_OFFSET[block] - (int)mv_stride];
			const int cy = mv_y[idx + MPEG4_MOTION_OFFSET[block] - (int)mv_stride];
			pred.x = Median3(ax, bx, cx);
			pred.y = Median3(ay, by, cy);
			return pred;
		}

		void StoreMV(int blk_x, int blk_y, int mvx, int mvy) {
			const uint32 idx = (uint32)(blk_y + 1) * mv_stride + (uint32)(blk_x + 1);
			mv_x[idx] = (int16)mvx;
			mv_y[idx] = (int16)mvy;
		}

		int DecodeMVD(MPEG4BitReader& reader, int pred, int f_code) {
			const int code = DecodeVLC(reader, s_mv_tab, 33);
			if (code < 0) return pred;
			if (code == 0) return pred;
			const int sign = (int)reader.ReadBits(1);
			const int shift = f_code - 1;
			int val = code;
			if (shift > 0) {
				val = ((val - 1) << shift) | (int)reader.ReadBits(shift);
				val++;
			}
			if (sign) val = -val;
			val += pred;
			// modulo decoding into the range representable with f_code
			return SignExtend(val, 5 + f_code);
		}

		//=================================================================================
		// Macroblock
		//=================================================================================

		void CleanIntraTableEntries() {
			// DC of a non-intra macroblock is 1024 (neutral) and its AC is 0, so that
			// neighbouring intra blocks predict from a defined value.
			for (int n = 0; n < 6; ++n) {
				if (n < 4) {
					const int blk_x = mb_x * 2 + (n & 1);
					const int blk_y = mb_y * 2 + (n >> 1);
					dc_y[LumaDcIndex(blk_x, blk_y)] = 1024;
					int16* ac = LumaAcPtr(blk_x, blk_y);
					MemSet(ac, 0, 16 * sizeof(int16));
				} else {
					int16* dc = (n == 4) ? dc_cb : dc_cr;
					dc[ChromaDcIndex()] = 1024;
					int16* ac = ChromaAcPtr(n == 5);
					MemSet(ac, 0, 16 * sizeof(int16));
				}
			}
		}

		bool DecodeMacroblock(MPEG4BitReader& reader) {
			const int rounding = vop.vop_rounding_type;
			bool is_intra = false;
			bool not_coded = false;
			bool four_mv = false;
			bool dquant = false;
			int sym = 0;
			int cbpc = 0;
			MPEG4MotionVector mv[4];

			ac_pred = false;

			if (vop.vop_coding_type == MPEG4_VOP_TYPE_P) {
				if (reader.ReadBool()) {
					not_coded = true;
				} else {
					for (;;) {
						sym = DecodeMCBPCInter(reader);
						if (sym < 0) return false;
						if (sym != 20) break; // 20 = stuffing
					}
					dquant = (sym & 8) != 0;
					is_intra = (sym & 4) != 0;
					four_mv = (sym & 16) != 0;
					cbpc = sym & 3;
					if (is_intra) {
						// handled by the intra path below
					} else {
						const int cbpy = DecodeVLC(reader, s_cbpy_tab, 16);
						if (cbpy < 0) return false;
						cbpc |= ((cbpy ^ 0x0F) << 2);
						if (dquant) {
							cur_qp += s_quant_tab[reader.ReadBits(2)];
							if (cur_qp < 1) cur_qp = 1;
							if (cur_qp > 31) cur_qp = 31;
						}
						if (!progressive && (cbpc != 0)) {
							return false; // field DCT not supported
						}
						if (!four_mv) {
							const MPEG4MotionVector pred = PredictMV(0);
							mv[0].x = DecodeMVD(reader, pred.x, vop.vop_fcode_forward ? vop.vop_fcode_forward : 1);
							mv[0].y = DecodeMVD(reader, pred.y, vop.vop_fcode_forward ? vop.vop_fcode_forward : 1);
							for (int i = 1; i < 4; ++i) { mv[i] = mv[0]; }
						} else {
							for (int i = 0; i < 4; ++i) {
								const MPEG4MotionVector pred = PredictMV(i);
								mv[i].x = DecodeMVD(reader, pred.x, vop.vop_fcode_forward ? vop.vop_fcode_forward : 1);
								mv[i].y = DecodeMVD(reader, pred.y, vop.vop_fcode_forward ? vop.vop_fcode_forward : 1);
								// The predictor of blocks 1..3 reads the vectors of the blocks
								// already decoded inside this macroblock, so they have to be
								// stored here and not after the whole macroblock is parsed.
								StoreMV(mb_x * 2 + (i & 1), mb_y * 2 + (i >> 1), mv[i].x, mv[i].y);
							}
						}
					}
				}
			} else if (vop.vop_coding_type == MPEG4_VOP_TYPE_I) {
				for (;;) {
					sym = DecodeMCBPCIntra(reader);
					if (sym < 0) return false;
					if (sym != 8) break; // 8 = stuffing
				}
				dquant = (sym & 4) != 0;
				is_intra = true;
				cbpc = sym & 3;
			} else {
				return false; // B / S VOPs are not supported
			}

			if (is_intra) {
				ac_pred = reader.ReadBool();
				const int cbpy = DecodeVLC(reader, s_cbpy_tab, 16);
				if (cbpy < 0) return false;
				cbpc |= (cbpy << 2);
				use_intra_dc_vlc = (cur_qp < s_dc_threshold[vop.intra_dc_vlc_thr & 7]);
				if (dquant) {
					cur_qp += s_quant_tab[reader.ReadBits(2)];
					if (cur_qp < 1) cur_qp = 1;
					if (cur_qp > 31) cur_qp = 31;
				}
				if (!progressive) {
					return false; // field DCT not supported
				}
			}

			mb_qp[(size_t)mb_y * mb_width + mb_x] = (uint8)cur_qp;
			if (trace_now) MPEG4_TRACE("  MB(%d,%d) sym=%d intra=%d nocode=%d 4mv=%d dquant=%d cbpc=%d qp=%d acpred=%d useidcvlc=%d pos=%llu\n",
				mb_x, mb_y, sym, (int)is_intra, (int)not_coded, (int)four_mv, (int)dquant, cbpc, cur_qp,
				(int)ac_pred, (int)use_intra_dc_vlc, (unsigned long long)reader.BitPos());

			if (not_coded) {
				for (int i = 0; i < 4; ++i) {
					StoreMV(mb_x * 2 + (i & 1), mb_y * 2 + (i >> 1), 0, 0);
				}
				MotionCompSkip(rounding);
				CleanIntraTableEntries();
				return true;
			}

			// --- motion compensation ---
			if (!is_intra) {
				if (trace_now) MPEG4_TRACE("   MC mb=(%d,%d) mv=(%d,%d) 4mv=%d cbp=%02X\n",
					mb_x, mb_y, mv[0].x, mv[0].y, (int)four_mv, cbpc);
				const int mb_px = mb_x * 16;
				const int mb_py = mb_y * 16;
				int chroma_x = 0, chroma_y = 0;
				if (!four_mv) {
					MotionCompLuma(mb_px, mb_py, 16, mv[0].x, mv[0].y, rounding);
					for (int i = 0; i < 4; ++i) {
						StoreMV(mb_x * 2 + (i & 1), mb_y * 2 + (i >> 1), mv[0].x, mv[0].y);
					}
					MotionCompChroma(mv[0].x, mv[0].y, rounding);
				} else {
					for (int i = 0; i < 4; ++i) {
						MotionCompLuma(mb_px + (i & 1) * 8, mb_py + (i >> 1) * 8, 8, mv[i].x, mv[i].y, rounding);
						StoreMV(mb_x * 2 + (i & 1), mb_y * 2 + (i >> 1), mv[i].x, mv[i].y);
					}
					// a single chroma vector derived from the sum of the four vectors
					const int sum_x = mv[0].x + mv[1].x + mv[2].x + mv[3].x;
					const int sum_y = mv[0].y + mv[1].y + mv[2].y + mv[3].y;
					chroma_x = (sum_x >> 3) + s_chroma_roundtab[sum_x & 0xF];
					chroma_y = (sum_y >> 3) + s_chroma_roundtab[sum_y & 0xF];
					// chroma_* is in half chroma pel units here
					MotionCompChromaHalf(chroma_x, chroma_y, rounding);
				}
			} else {
				// An intra macroblock has a neutral motion vector, so the median prediction of
				// the blocks to its right and below must see zeros and not the vectors that
				// the same slots held in the previous picture.
				for (int i = 0; i < 4; ++i) {
					StoreMV(mb_x * 2 + (i & 1), mb_y * 2 + (i >> 1), 0, 0);
				}
			}

			// --- coefficients ---
			const uint32 luma_stride_local = luma_stride;
			const uint32 chroma_stride_local = chroma_stride;
			stdsint out32[64];
			int16 qblk[64];

			for (int blk_idx = 0; blk_idx < 6; ++blk_idx) {
				const bool coded = (cbpc & (1 << (5 - blk_idx))) != 0;
				if (!DecodeBlock(reader, qblk, out32, blk_idx, coded, is_intra, cur_qp)) {
					diag_errors++;
					return false;
				}
				uni::IDCT::Transform(out32, 8);

				if (blk_idx < 4) {
					const int bx = (blk_idx & 1) * 8;
					const int by = (blk_idx >> 1) * 8;
					uint8* dst = cur_y + (size_t)(mb_y * 16 + by) * luma_stride_local + (mb_x * 16 + bx);
					for (int r = 0; r < 8; ++r) {
						for (int c = 0; c < 8; ++c) {
							const int val = is_intra ? (int)out32[r * 8 + c]
								: ((int)dst[(size_t)r * luma_stride_local + c] + (int)out32[r * 8 + c]);
							dst[(size_t)r * luma_stride_local + c] = (uint8)ClampByte(val);
						}
					}
				} else {
					uint8* plane = (blk_idx == 4) ? cur_u : cur_v;
					uint8* dst = plane + (size_t)(mb_y * 8) * chroma_stride_local + (mb_x * 8);
					for (int r = 0; r < 8; ++r) {
						for (int c = 0; c < 8; ++c) {
							const int val = is_intra ? (int)out32[r * 8 + c]
								: ((int)dst[(size_t)r * chroma_stride_local + c] + (int)out32[r * 8 + c]);
							dst[(size_t)r * chroma_stride_local + c] = (uint8)ClampByte(val);
						}
					}
				}
			}

			if (cbpc) ++diag_residual_mb;
			if (!is_intra && (mv[0].x || mv[0].y)) ++diag_mv_mb;
			if (!is_intra) CleanIntraTableEntries();
			++diag_mb_count;
			return true;
		}

		// chroma motion compensation from a vector already expressed in half chroma pel units
		void MotionCompChromaHalf(int cmv_x, int cmv_y, int rounding) {
			const int dst_x = mb_x * 8;
			const int dst_y = mb_y * 8;
			const int src_x = dst_x + (cmv_x >> 1);
			const int src_y = dst_y + (cmv_y >> 1);
			const int frac_x = cmv_x & 1;
			const int frac_y = cmv_y & 1;
			const int pw = (int)width / 2;
			const int ph = (int)height / 2;
			MotionCompPlane(
				cur_u + (size_t)dst_y * chroma_stride + dst_x, chroma_stride,
				ref_u, chroma_stride,
				src_x, src_y, frac_x, frac_y, 8, 8, pw, ph, rounding);
			MotionCompPlane(
				cur_v + (size_t)dst_y * chroma_stride + dst_x, chroma_stride,
				ref_v, chroma_stride,
				src_x, src_y, frac_x, frac_y, 8, 8, pw, ph, rounding);
		}

		//=================================================================================
		// Resync (video packet) markers
		//=================================================================================

		int VideoPacketPrefixLength() const {
			const int f_code = vop.vop_fcode_forward ? vop.vop_fcode_forward : 1;
			if (vop.vop_coding_type == MPEG4_VOP_TYPE_I) return 16;
			if (vop.vop_coding_type == MPEG4_VOP_TYPE_P) return 15 + f_code;
			return 0;
		}

		// Checks for a resync marker at the current position; on success the video packet
		// header is consumed and the reader is positioned on the first macroblock of the
		// new slice. Returns the macroblock number of that slice, or -1.
		// The macroblock layer is padded with stuffing before a video packet header, see
		// ff_mpeg4_stuffing(): a leading 0 followed by ones up to the next byte boundary.
		int TryConsumeResyncMarker(MPEG4BitReader& reader, int expected_mb) {
			if (!resync_enabled) return -1;
			const int prefix_len = VideoPacketPrefixLength();
			if (prefix_len <= 0) return -1;
			if (reader.BitsLeft() < (uint64)(prefix_len + 1 + 20)) return -1;

			const uint64 save = reader.BitPos();
			{
				const int rem = (int)(8 - (reader.BitPos() & 7)); // 1..8
				if (rem >= 1 && rem <= 8 && reader.PeekBits(rem) == (uint32)((1u << (rem - 1)) - 1u)) {
					reader.SkipBits(rem);
				}
			}
			if (reader.PeekBits(prefix_len + 1) != 1) {
				reader.SetBitPos(save);
				return -1;
			}

			reader.SkipBits(prefix_len + 1);
			int mb_num_bits = 1;
			{
				uint32 total = mb_width * mb_height;
				uint32 res = total > 0 ? total - 1 : 0;
				mb_num_bits = 0;
				while (res > 0) { res >>= 1; ++mb_num_bits; }
				if (mb_num_bits < 1) mb_num_bits = 1;
			}
			const int mb_num = (int)reader.ReadBits(mb_num_bits);
			const int qscale = (int)reader.ReadBits(5);
			const int header_extension = (int)reader.ReadBits(1);
			if (mb_num != expected_mb || header_extension != 0) {
				reader.SetBitPos(save);
				return -1;
			}
			if (qscale > 0) cur_qp = qscale;
			// a new slice starts here: the DC / AC / motion predictors of the neighbouring
			// slice are not used (see PredictDC and PredictMV)
			resync_mb_x = mb_num % (int)mb_width;
			resync_mb_y = mb_num / (int)mb_width;
			first_slice_line = true;
			MPEG4_TRACE("  [resync] mb_num=%d (row %d) qscale=%d next_bit=%llu\n",
				mb_num, resync_mb_y, qscale, (unsigned long long)reader.BitPos());
			return mb_num;
		}

		//=================================================================================
		// Frame
		//=================================================================================

		uni::Color* DecodeFrame(const byte* data, size_t size, int* out_w, int* out_h, int* out_vop_type) {
			if (!data || size < 4) return nullptr;
			MPEG4BitReader reader(data, size);

			bool found_vop = false;
			while (reader.HasData()) {
				if (reader.PeekBits(24) == 0x000001) {
					const uint32 sc = reader.ReadBits(32);
					if (sc >= MPEG4_START_CODE_VOL_MIN && sc <= MPEG4_START_CODE_VOL_MAX) {
						if (!ParseVOL(reader, sc)) return nullptr;
						reader.ByteAlign();
					} else if (sc == MPEG4_START_CODE_VOP) {
						if (ParseVOPHeader(reader, sc)) {
							found_vop = true;
							break;
						}
						reader.ByteAlign();
					} else {
						// VOS / VO / user data / GOV / visual object: resynchronise on the
						// next start code with a byte aligned scan
						reader.ByteAlign();
					}
				} else {
					reader.SkipBits(8);
				}
			}

			if (!found_vop) return nullptr;
			MPEG4_TRACE("vop type=%d quant=%d thr=%d fcode=%d rounding=%d hdr_end_bit=%llu\n",
				(int)vop.vop_coding_type, (int)vop.vop_quant, (int)vop.intra_dc_vlc_thr,
				(int)vop.vop_fcode_forward, (int)vop.vop_rounding_type,
				(unsigned long long)reader.BitPos());
			if (!cur_y) {
				const uint32 w = (default_w > 0) ? default_w : 320;
				const uint32 h = (default_h > 0) ? default_h : 240;
				if (!AllocateBuffers(w, h)) return nullptr;
			}
			if (quarter_sample || data_partitioned || mpeg_quant) return nullptr;
			if (vop.vop_coding_type == MPEG4_VOP_TYPE_B || vop.vop_coding_type == MPEG4_VOP_TYPE_S) return nullptr;

			if (out_w) *out_w = (int)width;
			if (out_h) *out_h = (int)height;
			if (out_vop_type) *out_vop_type = (int)vop.vop_coding_type;

			if (vop.vop_coding_type == MPEG4_VOP_TYPE_I) {
				// an I-VOP does not need the previous picture and fully defines the
				// predictors of its first row / column
				ResetPredictionState();
			}

			cur_qp = vop.vop_quant ? vop.vop_quant : 1;
			use_intra_dc_vlc = true;
			diag_bits_used = 0;
			diag_errors = 0;
			diag_mb_count = 0;
			diag_residual_mb = 0;
			diag_mv_mb = 0;

			resync_mb_x = 0;
			resync_mb_y = 0;
			first_slice_line = true;

			bool ok = true;
			for (mb_y = 0; mb_y < (int)mb_height && ok; ++mb_y) {
				for (mb_x = 0; mb_x < (int)mb_width; ++mb_x) {
					const int mb_index = mb_y * (int)mb_width + mb_x;
					// level 2 adds per-macroblock detail (see MPEG4Decoder_SetTrace)
					trace_now = (s_trace_enabled >= 2);
					if (mb_index > 0) TryConsumeResyncMarker(reader, mb_index);
					first_slice_line = (mb_y == resync_mb_y);
					if (!DecodeMacroblock(reader)) {
						ok = false;
						break;
					}
				}
			}
			diag_bits_used = reader.BitPos();

			if (!ok) return nullptr;

			// --- YUV420 -> BGRA ---
			const size_t pixel_cnt = (size_t)width * (size_t)height;
			uni::Color* pixels = (uni::Color*)malloc(pixel_cnt * sizeof(uni::Color));
			if (!pixels) return nullptr;

			for (uint32 y = 0; y < height; ++y) {
				const uint8* row_y = cur_y + (size_t)y * luma_stride;
				const uint8* row_u = cur_u + (size_t)(y / 2) * chroma_stride;
				const uint8* row_v = cur_v + (size_t)(y / 2) * chroma_stride;
				uni::Color* dst_row = pixels + (size_t)y * width;
				for (uint32 x = 0; x < width; ++x) {
					// MPEG-4 Part 2 has no range signalling: yuv420p content is video range
					// exactly like in H.263/MPEG-2, so expand BT.601 limited range here.
					dst_row[x] = uni::Color::FromYCbCr(row_y[x], row_u[x / 2], row_v[x / 2],
						uni::YCbCrMatrix::BT601, uni::YCbCrRange::Limited);
				}
			}

			// --- the reconstructed frame becomes the reference of the next P-VOP ---
			const size_t luma_size = (size_t)luma_stride * mb_height * 16;
			const size_t chroma_size = (size_t)chroma_stride * mb_height * 8;
			MemCopyN(ref_y, cur_y, luma_size);
			MemCopyN(ref_u, cur_u, chroma_size);
			MemCopyN(ref_v, cur_v, chroma_size);

			return pixels;
		}
	};

} // namespace

extern "C" {

void* MPEG4Decoder_Create(int default_w, int default_h) {
	return new MPEG4DecoderImpl(default_w, default_h);
}

void MPEG4Decoder_Destroy(void* ctx) {
	if (ctx) {
		delete (MPEG4DecoderImpl*)ctx;
	}
}

uni::Color* MPEG4Decoder_DecodeFrame(
	void* ctx,
	const byte* data,
	size_t size,
	int* out_w,
	int* out_h,
	int* out_vop_type
) {
	if (!ctx) return nullptr;
	return ((MPEG4DecoderImpl*)ctx)->DecodeFrame(data, size, out_w, out_h, out_vop_type);
}

#if defined(MPEG4_DIAGNOSTICS)
// Test hook: reports how many bits of the last frame chunk were consumed and whether the
// macroblock layer stayed in sync. Used by the host side test harness only.
void MPEG4Decoder_SetTrace(int enabled) {
	s_trace_enabled = enabled;
}

void MPEG4Decoder_GetFrameStats(void* ctx, int* coded_mb, int* residual_mb, int* mv_mb) {
	if (!ctx) return;
	MPEG4DecoderImpl* impl = (MPEG4DecoderImpl*)ctx;
	if (coded_mb) *coded_mb = impl->diag_mb_count;
	if (residual_mb) *residual_mb = impl->diag_residual_mb;
	if (mv_mb) *mv_mb = impl->diag_mv_mb;
}

void MPEG4Decoder_GetDiagnostics(void* ctx, uint64* bits_used, int* errors, int* mb_count) {
	if (!ctx) return;
	MPEG4DecoderImpl* impl = (MPEG4DecoderImpl*)ctx;
	if (bits_used) *bits_used = impl->diag_bits_used;
	if (errors) *errors = impl->diag_errors;
	if (mb_count) *mb_count = impl->diag_mb_count;
}
#endif

} // extern "C"

namespace uni {

	const char* MPEG4Codec::GetName() const {
		return "MPEG-4 Part 2 Video";
	}

	VideoContainerFormat MPEG4Codec::GetFormat() const {
		return VideoContainerFormat::MP4;
	}

	const char* const* MPEG4Codec::GetExtensions() const {
		static const char* const extensions[] = { ".m4v", ".mp4v", nullptr };
		return extensions;
	}

	VideoResult MPEG4Codec::Probe(StorageTrait& storage, bool& matched) const {
		matched = false;
		byte head[4] = { 0 };
		for (int i = 0; i < 4; ++i) {
			int b = storage[i];
			if (b < 0) return VideoResult::IoError;
			head[i] = (byte)b;
		}
		if (head[0] == 0x00 && head[1] == 0x00 && head[2] == 0x01) {
			if (head[3] == 0xB0 || (head[3] >= 0x20 && head[3] <= 0x2F) || head[3] == 0xB6) {
				matched = true;
				return VideoResult::OK;
			}
		}
		return VideoResult::OK;
	}

	VideoResult MPEG4Codec::ReadInfo(StorageTrait& storage, VideoInfo& outInfo) const {
		outInfo = {};
		outInfo.containerFormat = VideoContainerFormat::MP4;
		outInfo.videoFormat.codec_type = VideoCodecType::MPEG4;
		return VideoResult::OK;
	}

	VideoResult MPEG4Codec::OpenStream(
		StorageTrait& storage,
		IVideoStream*& outStream,
		trait::Malloc& allocator
	) const {
		outStream = nullptr;
		return VideoResult::Unsupported;
	}

} // namespace uni

#endif // _INC_CPP
