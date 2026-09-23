// ASCII C/C++ TAB4 CRLF
// Docutitle: [Format.Video] Advanced Video Coding (H.264 / AVC / MPEG-4 Part 10)
// Attribute: Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0; Dosconio Mecocoa, BSD 3-Clause License
// Copyright only. H.264 is covered by third-party patents -- commercial use needs its own patent consideration.

#include "../../../../inc/c/format/video/H264.h"
#include "../../../../inc/c/algorithm/dct.h"
#include "../../../../inc/c/ustring.h"
#include <stdlib.h>

#if defined(_INC_CPP)

namespace {

	static const char* const H264_EXTENSIONS[] = { ".264", ".h264", ".avc", nullptr };

	// ITU-T H.264 Table 9-5: coeff_token mapping for 2x2 Chroma DC blocks (nC = -1)
	// Mathematical derivation & indexing:
	//   TotalCoeff in [0, 4], TrailingOnes (T1) in [0, 3] (T1 <= TotalCoeff)
	//   Index formula: idx = TotalCoeff * 4 + TrailingOnes (Total 20 entries)
	//   Generated via Canonical Huffman prefix tree from conditional probability P(TotalCoeff, T1 | ChromaDC).
	static const uint8 chroma_dc_coeff_token_len[4 * 5] = {
		2, 0, 0, 0,
		6, 1, 0, 0,
		6, 6, 3, 0,
		6, 7, 7, 6,
		6, 8, 8, 7,
	};

	static const uint8 chroma_dc_coeff_token_bits[4 * 5] = {
		1, 0, 0, 0,
		7, 1, 0, 0,
		4, 6, 1, 0,
		3, 3, 2, 5,
		2, 3, 2, 0,
	};

	// ITU-T H.264 Table 9-5: coeff_token mapping for 4x4 Luma and Chroma AC/interleaved blocks
	// Mathematical derivation & indexing:
	//   1. Context variable nC calculated from upper neighbor nB and left neighbor nA:
	//        if (hasA && hasB) nC = (nA + nB + 1) >> 1
	//        else if (hasA)    nC = nA
	//        else if (hasB)    nC = nB
	//        else              nC = 0
	//   2. Table column selection by nC:
	//        Table[0]: 0 <= nC < 2  -> Num-VLC0 (Canonical Huffman prefix code)
	//        Table[1]: 2 <= nC < 4  -> Num-VLC1 (Canonical Huffman prefix code)
	//        Table[2]: 4 <= nC < 8  -> Num-VLC2 (Canonical Huffman prefix code)
	//        Table[3]: nC >= 8      -> fixed 6-bit codes, but NOT (TotalCoeff << 2) | TrailingOnes:
	//                   e.g. TotalCoeff=0/TrailingOnes=0 is 000011 and TotalCoeff=1/TrailingOnes=0
	//                   is 000000.  Always look the code up in the table below.
	//   3. Entry lookup index formula:
	//        idx = TotalCoeff * 4 + TrailingOnes, where TotalCoeff in [0, 16], TrailingOnes in [0, 3].
	//        Table size: 4 sub-tables * (17 * 4) = 4 * 68 = 272 entries.
	static const uint8 coeff_token_len[4][4 * 17] = {
		{
			1, 0, 0, 0,
			6, 2, 0, 0,     8, 6, 3, 0,     9, 8, 7, 5,    10, 9, 8, 6,
			11,10, 9, 7,    13,11,10, 8,    13,13,11, 9,    13,13,13,10,
			14,14,13,11,    14,14,14,13,    15,15,14,14,    15,15,15,14,
			16,15,15,15,    16,16,16,15,    16,16,16,16,    16,16,16,16,
		},
		{
			2, 0, 0, 0,
			6, 2, 0, 0,     6, 5, 3, 0,     7, 6, 6, 4,     8, 6, 6, 4,
			8, 7, 7, 5,     9, 8, 8, 6,    11, 9, 9, 6,    11,11,11, 7,
			12,11,11, 9,    12,12,12,11,    12,12,12,11,    13,13,13,12,
			13,13,13,13,    13,14,13,13,    14,14,14,13,    14,14,14,14,
		},
		{
			4, 0, 0, 0,
			6, 4, 0, 0,     6, 5, 4, 0,     6, 5, 5, 4,     7, 5, 5, 4,
			7, 5, 5, 4,     7, 6, 6, 4,     7, 6, 6, 4,     8, 7, 7, 5,
			8, 8, 7, 6,     9, 8, 8, 7,     9, 9, 8, 8,     9, 9, 9, 8,
			10, 9, 9, 9,    10,10,10,10,    10,10,10,10,    10,10,10,10,
		},
		{
			6, 0, 0, 0,
			6, 6, 0, 0,     6, 6, 6, 0,     6, 6, 6, 6,     6, 6, 6, 6,
			6, 6, 6, 6,     6, 6, 6, 6,     6, 6, 6, 6,     6, 6, 6, 6,
			6, 6, 6, 6,     6, 6, 6, 6,     6, 6, 6, 6,     6, 6, 6, 6,
			6, 6, 6, 6,     6, 6, 6, 6,     6, 6, 6, 6,     6, 6, 6, 6,
		}
	};

	static const uint8 coeff_token_bits[4][4 * 17] = {
		{
			1, 0, 0, 0,
			5, 1, 0, 0,     7, 4, 1, 0,     7, 6, 5, 3,     7, 6, 5, 3,
			7, 6, 5, 4,    15, 6, 5, 4,    11,14, 5, 4,     8,10,13, 4,
			15,14, 9, 4,    11,10,13,12,    15,14, 9,12,    11,10,13, 8,
			15, 1, 9,12,    11,14,13, 8,     7,10, 9,12,     4, 6, 5, 8,
		},
		{
			3, 0, 0, 0,
			11, 2, 0, 0,     7, 7, 3, 0,     7,10, 9, 5,     7, 6, 5, 4,
			4, 6, 5, 6,     7, 6, 5, 8,    15, 6, 5, 4,    11,14,13, 4,
			15,10, 9, 4,    11,14,13,12,     8,10, 9, 8,    15,14,13,12,
			11,10, 9,12,     7,11, 6, 8,     9, 8,10, 1,     7, 6, 5, 4,
		},
		{
			15, 0, 0, 0,
			15,14, 0, 0,    11,15,13, 0,     8,12,14,12,    15,10,11,11,
			11, 8, 9,10,     9,14,13, 9,     8,10, 9, 8,    15,14,13,13,
			11,14,10,12,    15,10,13,12,    11,14, 9,12,     8,10,13, 8,
			13, 7, 9,12,     9,12,11,10,     5, 8, 7, 6,     1, 4, 3, 2,
		},
		{
			3, 0, 0, 0,
			0, 1, 0, 0,     4, 5, 6, 0,     8, 9,10,11,    12,13,14,15,
			16,17,18,19,    20,21,22,23,    24,25,26,27,    28,29,30,31,
			32,33,34,35,    36,37,38,39,    40,41,42,43,    44,45,46,47,
			48,49,50,51,    52,53,54,55,    56,57,58,59,    60,61,62,63,
		}
	};

	// ITU-T H.264 Table 9-7: total_zeros tables for 4x4 blocks
	// Indexed by (TotalCoeff - 1) in 0..14 and total_zeros in 0..(16 - TotalCoeff).
	static const uint8 total_zeros_len[15][16] = {
		{ 1, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 9 },
		{ 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 6, 6, 6, 6, 0 },
		{ 4, 3, 3, 3, 4, 4, 3, 3, 4, 5, 5, 6, 5, 6, 0, 0 },
		{ 5, 3, 4, 4, 3, 3, 3, 4, 3, 4, 5, 5, 5, 0, 0, 0 },
		{ 4, 4, 4, 3, 3, 3, 3, 3, 4, 5, 4, 5, 0, 0, 0, 0 },
		{ 6, 5, 3, 3, 3, 3, 3, 3, 4, 3, 6, 0, 0, 0, 0, 0 },
		{ 6, 5, 3, 3, 3, 2, 3, 4, 3, 6, 0, 0, 0, 0, 0, 0 },
		{ 6, 4, 5, 3, 2, 2, 3, 3, 6, 0, 0, 0, 0, 0, 0, 0 },
		{ 6, 6, 4, 2, 2, 3, 2, 5, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 5, 5, 3, 2, 2, 2, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 4, 4, 3, 3, 1, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 4, 4, 2, 1, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 3, 3, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	};

	static const uint8 total_zeros_bits[15][16] = {
		{ 1, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 1 },
		{ 7, 6, 5, 4, 3, 5, 4, 3, 2, 3, 2, 3, 2, 1, 0, 0 },
		{ 5, 7, 6, 5, 4, 3, 4, 3, 2, 3, 2, 1, 1, 0, 0, 0 },
		{ 3, 7, 5, 4, 6, 5, 4, 3, 3, 2, 2, 1, 0, 0, 0, 0 },
		{ 5, 4, 3, 7, 6, 5, 4, 3, 2, 1, 1, 0, 0, 0, 0, 0 },
		{ 1, 1, 7, 6, 5, 4, 3, 2, 1, 1, 0, 0, 0, 0, 0, 0 },
		{ 1, 1, 5, 4, 3, 3, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0 },
		{ 1, 1, 1, 3, 3, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 1, 0, 1, 3, 2, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 1, 0, 1, 3, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 1, 1, 2, 1, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	};

	// ITU-T H.264 Table 9-8: total_zeros tables for 2x2 Chroma DC blocks
	// Indexed by (TotalCoeff - 1) in 0..2 and total_zeros in 0..(4 - TotalCoeff).
	static const uint8 chroma_dc_total_zeros_len[3][4] = {
		{ 1, 2, 3, 3 },
		{ 1, 2, 2, 0 },
		{ 1, 1, 0, 0 },
	};

	static const uint8 chroma_dc_total_zeros_bits[3][4] = {
		{ 1, 1, 1, 0 },
		{ 1, 1, 0, 0 },
		{ 1, 0, 0, 0 },
	};

	// ITU-T H.264 Table 9-10: run_before tables
	// Indexed by zerosLeft (1..6 mapped to 0..5, and >6 mapped to 6).
	static const uint8 run_len[7][16] = {
		{ 1, 1 },
		{ 1, 2, 2 },
		{ 2, 2, 2, 2 },
		{ 2, 2, 2, 3, 3 },
		{ 2, 2, 3, 3, 3, 3 },
		{ 2, 3, 3, 3, 3, 3, 3 },
		{ 3, 3, 3, 3, 3, 3, 3, 4, 5, 6, 7, 8, 9, 10, 11 },
	};

	static const uint8 run_bits[7][16] = {
		{ 1, 0 },
		{ 1, 1, 0 },
		{ 3, 2, 1, 0 },
		{ 3, 2, 1, 1, 0 },
		{ 3, 2, 3, 2, 1, 0 },
		{ 3, 0, 1, 3, 2, 5, 4 },
		{ 7, 6, 5, 4, 3, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
	};

	// ITU-T H.264 Figure 8-6: 4x4 Zig-Zag Scan Matrix
	static const uint8 zigzag_scan_4x4[16] = {
		0,  1,  4,  8,  5,  2,  3,  6,
		9, 12, 13, 10,  7, 11, 14, 15
	};

	// ITU-T H.264 Figure 8-7: 4x4 AC Scan Order (skipping DC at index 0)
	static const uint8 zigzag_scan_4x4_ac[15] = {
		1,  4,  8,  5,  2,  3,  6,  9,
		12, 13, 10,  7, 11, 14, 15
	};

	// ITU-T H.264 2x2 Chroma DC Raster Scan
	static const uint8 raster_scan_2x2[4] = {
		0, 1, 2, 3
	};

	// ITU-T H.264 Table 8-14: 4x4 Dequantization Multiplier Matrix V(qp_rem, i, j)
	// For a 4x4 block, positions (i,j) are grouped into 3 equivalence classes:
	//   Class 0: (even, even) -> (0,0), (0,2), (2,0), (2,2)
	//   Class 1: (odd, odd)   -> (1,1), (1,3), (3,1), (3,3)
	//   Class 2: remaining    -> mixed (even, odd) or (odd, even)
	// Multiplier = V[qp % 6][class] << (qp / 6).
	static const int32 dequant_v[6][3] = {
		{ 10, 16, 13 },
		{ 11, 18, 14 },
		{ 13, 20, 16 },
		{ 14, 23, 18 },
		{ 16, 25, 20 },
		{ 18, 29, 23 }
	};

	// ITU-T H.264 Table 8-15: Chroma QP Mapping Table
	// Maps index qPI = Clip3(0, 51, QPY + chroma_qp_index_offset) to actual chroma QP (qPC).
	// For qPI < 30: qPC = qPI (identity).
	// For qPI >= 30: non-linear compression to preserve bit budget for high luma detail.
	static const uint8 chroma_qp_table[52] = {
		0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
		10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
		20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
		29, 30, 31, 32, 32, 33, 34, 34, 35, 35,
		36, 36, 37, 37, 37, 38, 38, 38, 39, 39,
		39, 39
	};

	static inline void RefillBits(H264BitReader* reader) {
		while (reader->bits_left <= 56 && reader->byte_pos < reader->size) {
			reader->bit_buf = (reader->bit_buf << 8) | (uint64)reader->data[reader->byte_pos++];
			reader->bits_left += 8;
		}
	}

	static inline int32 DecodeVLC(H264BitReader* reader, const uint8* table_len, const uint8* table_bits, int num_entries) {
		for (int i = 0; i < num_entries; ++i) {
			int len = (int)table_len[i];
			if (len > 0) {
				uint32 bits = (uint32)table_bits[i];
				if (H264_PeekBits(reader, len) == bits) {
					H264_SkipBits(reader, len);
					return i;
				}
			}
		}
		return -1;
	}

	static void ParseScalingList(H264BitReader& reader, uint8* scaling_list, int size, uint8& use_default_scaling_matrix_flag) {
		int last_scale = 8;
		int next_scale = 8;
		use_default_scaling_matrix_flag = 0;

		for (int j = 0; j < size; ++j) {
			if (next_scale != 0) {
				int32 delta_scale = H264_ReadSE(&reader);
				next_scale = (last_scale + delta_scale + 256) % 256;
				use_default_scaling_matrix_flag = (j == 0 && next_scale == 0) ? 1 : 0;
			}
			scaling_list[j] = (next_scale == 0) ? (uint8)last_scale : (uint8)next_scale;
			last_scale = scaling_list[j];
		}
	}

} // namespace

extern "C" {

void H264_InitBitReader(H264BitReader* reader, const byte* data, size_t size) {
	if (!reader) return;
	reader->data = data;
	reader->size = size;
	reader->byte_pos = 0;
	reader->bit_buf = 0;
	reader->bits_left = 0;
	RefillBits(reader);
}

uint32 H264_PeekBits(H264BitReader* reader, int n) {
	if (!reader || n <= 0) return 0;
	if (reader->bits_left < n) RefillBits(reader);
	if (reader->bits_left < n) {
		return (uint32)((reader->bit_buf << (n - reader->bits_left)) & ((1ULL << n) - 1ULL));
	}
	return (uint32)((reader->bit_buf >> (reader->bits_left - n)) & ((1ULL << n) - 1ULL));
}

void H264_SkipBits(H264BitReader* reader, int n) {
	if (!reader || n <= 0) return;
	if (reader->bits_left < n) {
		n -= reader->bits_left;
		reader->bit_buf = 0;
		reader->bits_left = 0;
		size_t bytes_to_skip = (size_t)(n / 8);
		if (reader->byte_pos + bytes_to_skip <= reader->size) {
			reader->byte_pos += bytes_to_skip;
		} else {
			reader->byte_pos = reader->size;
		}
		n %= 8;
		RefillBits(reader);
	}
	reader->bits_left -= n;
}

uint32 H264_ReadBits(H264BitReader* reader, int n) {
	uint32 val = H264_PeekBits(reader, n);
	H264_SkipBits(reader, n);
	return val;
}

bool H264_ReadBool(H264BitReader* reader) {
	return H264_ReadBits(reader, 1) != 0;
}

uint32 H264_ReadUE(H264BitReader* reader) {
	int leading_zeros = 0;
	while (H264_PeekBits(reader, 1) == 0 && H264_HasData(reader) && leading_zeros < 32) {
		H264_SkipBits(reader, 1);
		leading_zeros++;
	}
	if (leading_zeros == 0) {
		H264_SkipBits(reader, 1);
		return 0;
	}
	H264_SkipBits(reader, 1); // skip the '1' bit
	uint32 info = H264_ReadBits(reader, leading_zeros);
	return (1u << leading_zeros) - 1u + info;
}

int32 H264_ReadSE(H264BitReader* reader) {
	uint32 k = H264_ReadUE(reader);
	if (k == 0) return 0;
	if (k & 1) {
		return (int32)((k + 1) >> 1);
	} else {
		return -(int32)(k >> 1);
	}
}

uint32 H264_ReadTE(H264BitReader* reader, uint32 max_val) {
	if (max_val == 1) {
		return H264_ReadBool(reader) ? 0 : 1;
	}
	return H264_ReadUE(reader);
}

void H264_ByteAlign(H264BitReader* reader) {
	if (!reader) return;
	int rem = reader->bits_left % 8;
	if (rem > 0) {
		H264_SkipBits(reader, rem);
	}
}

bool H264_HasData(const H264BitReader* reader) {
	if (!reader) return false;
	return reader->byte_pos < reader->size || reader->bits_left > 0;
}

int H264_GetBitsLeft(const H264BitReader* reader) {
	if (!reader) return 0;
	return (int)((reader->size - reader->byte_pos) * 8 + reader->bits_left);
}

size_t H264_UnescapeRBSP(const byte* src, size_t src_len, byte* dst, size_t dst_len) {
	if (!src || !dst || src_len == 0 || dst_len == 0) return 0;

	size_t src_i = 0;
	size_t dst_i = 0;

	while (src_i < src_len && dst_i < dst_len) {
		if (src_i + 2 < src_len && src[src_i] == 0x00 && src[src_i + 1] == 0x00 && src[src_i + 2] == 0x03) {
			// Emulation prevention byte detected: write 0x00, 0x00 and skip 0x03
			dst[dst_i++] = 0x00;
			if (dst_i >= dst_len) break;
			dst[dst_i++] = 0x00;
			src_i += 3;
		} else {
			dst[dst_i++] = src[src_i++];
		}
	}
	return dst_i;
}

bool H264_ParseSPS(const byte* rbsp_data, size_t rbsp_len, H264SPS* out_sps) {
	if (!rbsp_data || rbsp_len == 0 || !out_sps) return false;

	MemSet(out_sps, 0, sizeof(H264SPS));
	H264BitReader reader;
	H264_InitBitReader(&reader, rbsp_data, rbsp_len);

	out_sps->profile_idc = (uint8)H264_ReadBits(&reader, 8);
	out_sps->constraint_set0_flag = (uint8)H264_ReadBits(&reader, 1);
	out_sps->constraint_set1_flag = (uint8)H264_ReadBits(&reader, 1);
	out_sps->constraint_set2_flag = (uint8)H264_ReadBits(&reader, 1);
	out_sps->constraint_set3_flag = (uint8)H264_ReadBits(&reader, 1);
	out_sps->constraint_set4_flag = (uint8)H264_ReadBits(&reader, 1);
	out_sps->constraint_set5_flag = (uint8)H264_ReadBits(&reader, 1);
	H264_ReadBits(&reader, 2); // reserved_zero_2bits
	out_sps->level_idc = (uint8)H264_ReadBits(&reader, 8);
	out_sps->seq_parameter_set_id = (uint8)H264_ReadUE(&reader);

	out_sps->chroma_format_idc = 1; // default 4:2:0

	if (out_sps->profile_idc == 100 || out_sps->profile_idc == 110 ||
		out_sps->profile_idc == 122 || out_sps->profile_idc == 244 ||
		out_sps->profile_idc == 44  || out_sps->profile_idc == 83  ||
		out_sps->profile_idc == 86  || out_sps->profile_idc == 118 ||
		out_sps->profile_idc == 128 || out_sps->profile_idc == 138 ||
		out_sps->profile_idc == 139 || out_sps->profile_idc == 134 ||
		out_sps->profile_idc == 73) {

		out_sps->chroma_format_idc = (uint8)H264_ReadUE(&reader);
		if (out_sps->chroma_format_idc == 3) {
			out_sps->separate_colour_plane_flag = (uint8)H264_ReadBits(&reader, 1);
		}
		out_sps->bit_depth_luma_minus8 = (uint8)H264_ReadUE(&reader);
		out_sps->bit_depth_chroma_minus8 = (uint8)H264_ReadUE(&reader);
		out_sps->qpprime_y_zero_transform_bypass_flag = (uint8)H264_ReadBits(&reader, 1);
		out_sps->seq_scaling_matrix_present_flag = (uint8)H264_ReadBits(&reader, 1);

		if (out_sps->seq_scaling_matrix_present_flag) {
			int num_scaling_lists = (out_sps->chroma_format_idc != 3) ? 8 : 12;
			for (int i = 0; i < num_scaling_lists; ++i) {
				out_sps->seq_scaling_list_present_flag[i] = (uint8)H264_ReadBits(&reader, 1);
				if (out_sps->seq_scaling_list_present_flag[i]) {
					uint8 def_mat = 0;
					if (i < 6) {
						ParseScalingList(reader, out_sps->scaling_list_4x4[i], 16, def_mat);
					} else {
						ParseScalingList(reader, out_sps->scaling_list_8x8[i - 6], 64, def_mat);
					}
				}
			}
		}
	}

	out_sps->log2_max_frame_num_minus4 = (uint8)H264_ReadUE(&reader);
	out_sps->pic_order_cnt_type = (uint8)H264_ReadUE(&reader);

	if (out_sps->pic_order_cnt_type == 0) {
		out_sps->log2_max_pic_order_cnt_lsb_minus4 = (uint8)H264_ReadUE(&reader);
	} else if (out_sps->pic_order_cnt_type == 1) {
		out_sps->delta_pic_order_always_zero_flag = (uint8)H264_ReadBits(&reader, 1);
		out_sps->offset_for_non_ref_pic = H264_ReadSE(&reader);
		out_sps->offset_for_top_to_bottom_field = H264_ReadSE(&reader);
		out_sps->num_ref_frames_in_pic_order_cnt_cycle = (uint8)H264_ReadUE(&reader);
		for (uint32 i = 0; i < out_sps->num_ref_frames_in_pic_order_cnt_cycle; ++i) {
			out_sps->offset_for_ref_frame[i] = H264_ReadSE(&reader);
		}
	}

	out_sps->max_num_ref_frames = (uint8)H264_ReadUE(&reader);
	out_sps->gaps_in_frame_num_value_allowed_flag = (uint8)H264_ReadBits(&reader, 1);
	out_sps->pic_width_in_mbs_minus1 = H264_ReadUE(&reader);
	out_sps->pic_height_in_map_units_minus1 = H264_ReadUE(&reader);
	out_sps->frame_mbs_only_flag = (uint8)H264_ReadBits(&reader, 1);

	if (!out_sps->frame_mbs_only_flag) {
		out_sps->mb_adaptive_frame_field_flag = (uint8)H264_ReadBits(&reader, 1);
	}
	out_sps->direct_8x8_inference_flag = (uint8)H264_ReadBits(&reader, 1);

	out_sps->frame_cropping_flag = (uint8)H264_ReadBits(&reader, 1);
	if (out_sps->frame_cropping_flag) {
		out_sps->frame_crop_left_offset = H264_ReadUE(&reader);
		out_sps->frame_crop_right_offset = H264_ReadUE(&reader);
		out_sps->frame_crop_top_offset = H264_ReadUE(&reader);
		out_sps->frame_crop_bottom_offset = H264_ReadUE(&reader);
	}

	out_sps->vui_parameters_present_flag = (uint8)H264_ReadBits(&reader, 1);
	if (out_sps->vui_parameters_present_flag) {
		out_sps->aspect_ratio_info_present_flag = (uint8)H264_ReadBits(&reader, 1);
		if (out_sps->aspect_ratio_info_present_flag) {
			out_sps->aspect_ratio_idc = (uint8)H264_ReadBits(&reader, 8);
			if (out_sps->aspect_ratio_idc == 255) { // Extended_SAR
				out_sps->sar_width = (uint16)H264_ReadBits(&reader, 16);
				out_sps->sar_height = (uint16)H264_ReadBits(&reader, 16);
			}
		}

		if (H264_ReadBool(&reader)) { // overscan_info_present_flag
			H264_ReadBits(&reader, 1);  // overscan_appropriate_flag
		}

		if (H264_ReadBool(&reader)) { // video_signal_type_present_flag
			out_sps->video_signal_type_present_flag = 1;
			H264_ReadBits(&reader, 3);  // video_format
			out_sps->video_full_range_flag = (uint8)H264_ReadBits(&reader, 1);
			if (H264_ReadBool(&reader)) { // colour_description_present_flag
				out_sps->colour_description_present_flag = 1;
				H264_ReadBits(&reader, 8);  // colour_primaries
				H264_ReadBits(&reader, 8);  // transfer_characteristics
				out_sps->matrix_coefficients = (uint8)H264_ReadBits(&reader, 8);
			}
		}

		if (H264_ReadBool(&reader)) { // chroma_loc_info_present_flag
			H264_ReadUE(&reader);     // chroma_sample_loc_type_top_field
			H264_ReadUE(&reader);     // chroma_sample_loc_type_bottom_field
		}

		out_sps->timing_info_present_flag = (uint8)H264_ReadBits(&reader, 1);
		if (out_sps->timing_info_present_flag) {
			out_sps->num_units_in_tick = H264_ReadBits(&reader, 32);
			out_sps->time_scale = H264_ReadBits(&reader, 32);
			out_sps->fixed_frame_rate_flag = (uint8)H264_ReadBits(&reader, 1);
		}
	}

	// Calculate geometry helpers
	out_sps->mb_width = out_sps->pic_width_in_mbs_minus1 + 1;
	out_sps->mb_height = (2 - out_sps->frame_mbs_only_flag) * (out_sps->pic_height_in_map_units_minus1 + 1);

	uint32 crop_unit_x = (out_sps->chroma_format_idc == 0) ? 1 : 2;
	uint32 crop_unit_y = (2 - out_sps->frame_mbs_only_flag) * ((out_sps->chroma_format_idc == 0) ? 1 : 2);

	out_sps->width = (out_sps->mb_width * 16) -
		(out_sps->frame_crop_left_offset + out_sps->frame_crop_right_offset) * crop_unit_x;
	out_sps->height = (out_sps->mb_height * 16) -
		(out_sps->frame_crop_top_offset + out_sps->frame_crop_bottom_offset) * crop_unit_y;

	if (out_sps->timing_info_present_flag && out_sps->num_units_in_tick > 0) {
		out_sps->fps_num = out_sps->time_scale;
		out_sps->fps_den = out_sps->num_units_in_tick * 2;
	} else {
		out_sps->fps_num = 30;
		out_sps->fps_den = 1;
	}

	out_sps->is_valid = (out_sps->width > 0 && out_sps->height > 0);
	return out_sps->is_valid;
}

bool H264_ParsePPS(const byte* rbsp_data, size_t rbsp_len, const H264SPS* sps_list, H264PPS* out_pps) {
	if (!rbsp_data || rbsp_len == 0 || !out_pps) return false;

	MemSet(out_pps, 0, sizeof(H264PPS));
	H264BitReader reader;
	H264_InitBitReader(&reader, rbsp_data, rbsp_len);

	out_pps->pic_parameter_set_id = (uint8)H264_ReadUE(&reader);
	out_pps->seq_parameter_set_id = (uint8)H264_ReadUE(&reader);
	out_pps->entropy_coding_mode_flag = (uint8)H264_ReadBits(&reader, 1);
	out_pps->bottom_field_pic_order_in_frame_present_flag = (uint8)H264_ReadBits(&reader, 1);
	out_pps->num_slice_groups_minus1 = (uint8)H264_ReadUE(&reader);

	if (out_pps->num_slice_groups_minus1 > 0) {
		out_pps->slice_group_map_type = (uint8)H264_ReadUE(&reader);
		// Skip slice group map specifics for baseline
	}

	out_pps->num_ref_idx_l0_default_active_minus1 = (uint8)H264_ReadUE(&reader);
	out_pps->num_ref_idx_l1_default_active_minus1 = (uint8)H264_ReadUE(&reader);
	out_pps->weighted_pred_flag = (uint8)H264_ReadBits(&reader, 1);
	out_pps->weighted_bipred_idc = (uint8)H264_ReadBits(&reader, 2);
	out_pps->pic_init_qp_minus26 = (int8)H264_ReadSE(&reader);
	out_pps->pic_init_qs_minus26 = (int8)H264_ReadSE(&reader);
	out_pps->chroma_qp_index_offset = (int8)H264_ReadSE(&reader);

	out_pps->deblocking_filter_control_present_flag = (uint8)H264_ReadBits(&reader, 1);
	out_pps->constrained_intra_pred_flag = (uint8)H264_ReadBits(&reader, 1);
	out_pps->redundant_pic_cnt_present_flag = (uint8)H264_ReadBits(&reader, 1);

	// H.264 7.3.2.2 / 7.4.2.2: transform_8x8_mode_flag and everything after it are present only
	// "if (more_rbsp_data())".  For the baseline / main / extended profiles that announce a
	// constraint set there is no extension, and the RBSP ends right after  redundant_pic_cnt_present_flag.  Testing only bits_left > 0 is wrong there - the remaining
	// bits are the rbsp_stop_one_bit plus alignment zeros, so the stop bit was consumed as
	// transform_8x8_mode_flag = 1 and the slice decoder then waited for a transform_size_8x8_flag
	// bin the encoder had never written.  Measured on a main-profile (77) re-encode of wind.mp4:
	// every stream desynchronised at the first picture and the library returned an almost black
	// frame (Y mean 12.9 against the reference decode's 201.0).
	// With the extension absent, second_chroma_qp_index_offset defaults to chroma_qp_index_offset.
	out_pps->second_chroma_qp_index_offset = out_pps->chroma_qp_index_offset;
	const uint8 pps_profile = sps_list ? sps_list->profile_idc : 0;
	const bool pps_constraint_sets = (sps_list != nullptr) &&
		(sps_list->constraint_set0_flag || sps_list->constraint_set1_flag || sps_list->constraint_set2_flag);
	const bool pps_more_rbsp = !((pps_profile == 66 || pps_profile == 77 || pps_profile == 88) &&
								 pps_constraint_sets);
	if (pps_more_rbsp && H264_HasData(&reader) && H264_GetBitsLeft(&reader) > 0) {
		out_pps->transform_8x8_mode_flag = (uint8)H264_ReadBits(&reader, 1);
		out_pps->pic_scaling_matrix_present_flag = (uint8)H264_ReadBits(&reader, 1);
		if (out_pps->pic_scaling_matrix_present_flag) {
			int num_scaling_lists = 6 + ((sps_list && sps_list->chroma_format_idc == 3) ? 6 : 2) * out_pps->transform_8x8_mode_flag;
			for (int i = 0; i < num_scaling_lists; ++i) {
				if (H264_ReadBool(&reader)) {
					uint8 dummy[64];
					uint8 def_mat = 0;
					ParseScalingList(reader, dummy, (i < 6) ? 16 : 64, def_mat);
				}
			}
		}
		out_pps->second_chroma_qp_index_offset = (int8)H264_ReadSE(&reader);
	}

	out_pps->is_valid = true;
	return true;
}

// Phase 2: CAVLC Entropy Decoder, Dequantization & Transforms

bool H264_DecodeCAVLC_Block(
	H264BitReader* reader,
	int32 nC,
	int32 max_num_coeff,
	int16* out_coeffs,
	int32* out_total_coeff
) {
	if (!reader || !out_coeffs || !out_total_coeff || max_num_coeff <= 0) return false;

	for (int i = 0; i < max_num_coeff; ++i) {
		out_coeffs[i] = 0;
	}
	*out_total_coeff = 0;

	int32 total_coeff = 0;
	int32 trailing_ones = 0;

	if (nC < 0) { // Chroma DC (2x2)
		int32 idx = DecodeVLC(reader, chroma_dc_coeff_token_len, chroma_dc_coeff_token_bits, 20);
		if (idx < 0) return false;
		total_coeff = idx / 4;
		trailing_ones = idx % 4;
	} else if (nC < 2) {
		int32 idx = DecodeVLC(reader, coeff_token_len[0], coeff_token_bits[0], 68);
		if (idx < 0) return false;
		total_coeff = idx / 4;
		trailing_ones = idx % 4;
	} else if (nC < 4) {
		int32 idx = DecodeVLC(reader, coeff_token_len[1], coeff_token_bits[1], 68);
		if (idx < 0) return false;
		total_coeff = idx / 4;
		trailing_ones = idx % 4;
	} else if (nC < 8) {
		int32 idx = DecodeVLC(reader, coeff_token_len[2], coeff_token_bits[2], 68);
		if (idx < 0) return false;
		total_coeff = idx / 4;
		trailing_ones = idx % 4;
	} else { // nC >= 8: ITU-T H.264 Table 9-5, 8 <= nC column (6-bit codes, NOT 4*TC+TE)
		int32 idx = DecodeVLC(reader, coeff_token_len[3], coeff_token_bits[3], 68);
		if (idx < 0) return false;
		total_coeff = idx / 4;
		trailing_ones = idx % 4;
	}

	if (total_coeff == 0) {
		*out_total_coeff = 0;
		return true;
	}
	if (total_coeff > max_num_coeff) {
		return false;
	}

	int32 level[16] = { 0 };

	// 1. Trailing Ones Signs
	for (int i = 0; i < trailing_ones; ++i) {
		level[i] = H264_ReadBits(reader, 1) ? -1 : 1;
	}

	// 2. Remaining Levels
	int32 suffix_length = (total_coeff > 10 && trailing_ones < 3) ? 1 : 0;
	for (int i = trailing_ones; i < total_coeff; ++i) {
		int32 level_prefix = 0;
		while (H264_PeekBits(reader, 1) == 0 && H264_HasData(reader) && level_prefix < 32) {
			H264_SkipBits(reader, 1);
			level_prefix++;
		}
		H264_SkipBits(reader, 1); // consume the '1'

		// H.264 9.2.2: levelSuffixSize is suffixLength, except 4 when level_prefix == 14 and
		// suffixLength == 0, and level_prefix - 3 when level_prefix >= 15.
		int32 level_suffix_size = suffix_length;
		if (level_prefix == 14 && suffix_length == 0) level_suffix_size = 4;
		else if (level_prefix >= 15) level_suffix_size = level_prefix - 3;
		if (level_suffix_size > 24) return false;
		uint32 level_suffix = (level_suffix_size > 0) ? H264_ReadBits(reader, level_suffix_size) : 0;

		uint32 prefix_capped = ((uint32)level_prefix > 15u) ? 15u : (uint32)level_prefix;
		uint32 level_code = (prefix_capped << suffix_length) + level_suffix;
		if (level_prefix >= 15 && suffix_length == 0) level_code += 15;
		if (level_prefix >= 16) level_code += (1u << (level_prefix - 3)) - 4096u;

		if (i == trailing_ones && trailing_ones < 3) {
			level_code += 2;
		}

		if ((level_code & 1) == 0) {
			level[i] = (int32)((level_code + 2) >> 1);
		} else {
			level[i] = -(int32)((level_code + 1) >> 1);
		}

		if (suffix_length == 0) {
			suffix_length = 1;
		}
		int32 abs_lvl = level[i] < 0 ? -level[i] : level[i];
		if (abs_lvl > (3 << (suffix_length - 1)) && suffix_length < 6) {
			suffix_length++;
		}
	}

	// 3. Total Zeros
	int32 total_zeros = 0;
	if (total_coeff < max_num_coeff) {
		if (max_num_coeff == 4) { // Chroma DC
			if (total_coeff >= 1 && total_coeff <= 3) {
				int32 tz_idx = DecodeVLC(reader, chroma_dc_total_zeros_len[total_coeff - 1],
										 chroma_dc_total_zeros_bits[total_coeff - 1], 4);
				if (tz_idx < 0) return false;
				total_zeros = tz_idx;
			}
		} else {
			if (total_coeff >= 1 && total_coeff <= 15) {
				int32 max_tz_entries = 16 - total_coeff + 1;
				int32 tz_idx = DecodeVLC(reader, total_zeros_len[total_coeff - 1],
										 total_zeros_bits[total_coeff - 1], max_tz_entries);
				if (tz_idx < 0) return false;
				total_zeros = tz_idx;
			}
		}
	}

	// 4. Run Before
	int32 zeros_left = total_zeros;
	int32 run_before[16] = { 0 };

	// H.264 9.2.3: the runs are read for i = 0, 1, ... TotalCoeff-2 and the leftover
	// zerosLeft becomes run[TotalCoeff-1]; 9.2.4 then walks i downwards from TotalCoeff-1.
	for (int i = 0; i < total_coeff - 1; ++i) {
		if (zeros_left > 0) {
			int col = (zeros_left >= 7) ? 6 : (zeros_left - 1);
			int max_entries = (zeros_left >= 7) ? 15 : (zeros_left + 1);
			int32 run = DecodeVLC(reader, run_len[col], run_bits[col], max_entries);
			if (run < 0 || run > zeros_left) return false;
			run_before[i] = run;
			zeros_left -= run;
		} else {
			run_before[i] = 0;
		}
	}
	run_before[total_coeff - 1] = zeros_left;

	// 5. Place coefficients into output block
	const uint8* scan = (max_num_coeff == 4) ? raster_scan_2x2 :
						((max_num_coeff == 15) ? zigzag_scan_4x4_ac : zigzag_scan_4x4);

	int32 coeff_idx = -1;
	for (int i = total_coeff - 1; i >= 0; --i) {
		coeff_idx += run_before[i] + 1;
		if (coeff_idx < max_num_coeff) {
			out_coeffs[scan[coeff_idx]] = (int16)level[i];
		}
	}

	*out_total_coeff = total_coeff;
	return true;
}

void H264_IDCT4x4(const int32 in_scaled[16], int32 out_residual[16]) {
	if (!in_scaled || !out_residual) return;
	stdsint in_s[16], out_s[16];
	for (int i = 0; i < 16; ++i) in_s[i] = (stdsint)in_scaled[i];
	IDCT_H264_4x4(in_s, out_s);
	for (int i = 0; i < 16; ++i) out_residual[i] = (int32)out_s[i];
}

void H264_Hadamard4x4(const int16 in_dc[16], int32 qp, int32 out_scaled_dc[16]) {
	if (!in_dc || !out_scaled_dc) return;
	stdsint in_s[16], out_s[16];
	for (int i = 0; i < 16; ++i) in_s[i] = (stdsint)in_dc[i];
	Hadamard_H264_4x4(in_s, qp, out_s);
	for (int i = 0; i < 16; ++i) out_scaled_dc[i] = (int32)out_s[i];
}

void H264_Hadamard2x2(const int16 in_dc[4], int32 qp, int32 out_scaled_dc[4]) {
	if (!in_dc || !out_scaled_dc) return;
	stdsint in_s[4], out_s[4];
	for (int i = 0; i < 4; ++i) in_s[i] = (stdsint)in_dc[i];
	Hadamard_H264_2x2(in_s, qp, out_s);
	for (int i = 0; i < 4; ++i) out_scaled_dc[i] = (int32)out_s[i];
}

void H264_Dequant4x4(const int16 in_coeffs[16], int32 qp, bool is_dc_present, int32 out_scaled[16]) {
	if (!in_coeffs || !out_scaled) return;
	stdsint out_s[16];
	Dequant_H264_4x4(in_coeffs, qp, is_dc_present, out_s);
	for (int i = 0; i < 16; ++i) out_scaled[i] = (int32)out_s[i];
}

uint8 H264_GetChromaQP(int32 qp_luma, int8 chroma_qp_index_offset) {
	int32 qp_i = qp_luma + (int32)chroma_qp_index_offset;
	if (qp_i < 0) qp_i = 0;
	else if (qp_i > 51) qp_i = 51;
	return chroma_qp_table[qp_i];
}

// Phase 3: Intra Macroblock Prediction & I-Slice Decoding

static inline uint8 Clip8(int val) {
	if (val < 0) return 0;
	if (val > 255) return 255;
	return (uint8)val;
}

static inline int32 Clip3(int32 min_val, int32 max_val, int32 val) {
	if (val < min_val) return min_val;
	if (val > max_val) return max_val;
	return val;
}

static const uint8 golomb_to_intra4x4_cbp[48] = {
	47, 31, 15,  0, 23, 27, 29, 30,  7, 11, 13, 14, 39, 43, 45, 46,
	16,  3,  5, 10, 12, 19, 21, 26, 28, 35, 37, 42, 44,  1,  2,  4,
	 8, 17, 18, 20, 24,  6,  9, 22, 25, 32, 33, 34, 36, 40, 38, 41
};

bool H264_ParseSliceHeader(
	H264BitReader* reader,
	const H264SPS* sps,
	const H264PPS* pps,
	uint8 nal_unit_type,
	uint8 nal_ref_idc,
	H264SliceHeader* out_sh
) {
	if (!reader || !sps || !pps || !out_sh) return false;

	MemSet(out_sh, 0, sizeof(H264SliceHeader));

	out_sh->first_mb_in_slice = H264_ReadUE(reader);
	out_sh->slice_type = (uint8)H264_ReadUE(reader);
	out_sh->pic_parameter_set_id = (uint8)H264_ReadUE(reader);

	int frame_num_len = (int)sps->log2_max_frame_num_minus4 + 4;
	out_sh->frame_num = H264_ReadBits(reader, frame_num_len);

	if (!sps->frame_mbs_only_flag) {
		out_sh->field_pic_flag = (uint8)H264_ReadBits(reader, 1);
		if (out_sh->field_pic_flag) {
			out_sh->bottom_field_flag = (uint8)H264_ReadBits(reader, 1);
		}
	}

	if (nal_unit_type == H264_NAL_SLICE_IDR) {
		out_sh->idr_pic_id = (uint16)H264_ReadUE(reader);
	}

	if (sps->pic_order_cnt_type == 0) {
		int poc_len = (int)sps->log2_max_pic_order_cnt_lsb_minus4 + 4;
		out_sh->pic_order_cnt_lsb = H264_ReadBits(reader, poc_len);
		if (pps->bottom_field_pic_order_in_frame_present_flag && !out_sh->field_pic_flag) {
			out_sh->delta_pic_order_cnt_bottom = H264_ReadSE(reader);
		}
	} else if (sps->pic_order_cnt_type == 1 && !sps->delta_pic_order_always_zero_flag) {
		out_sh->delta_pic_order_cnt[0] = H264_ReadSE(reader);
		if (pps->bottom_field_pic_order_in_frame_present_flag && !out_sh->field_pic_flag) {
			out_sh->delta_pic_order_cnt[1] = H264_ReadSE(reader);
		}
	}

	if (pps->redundant_pic_cnt_present_flag) {
		out_sh->redundant_pic_cnt = (uint8)H264_ReadUE(reader);
	}

	uint8 actual_type = out_sh->slice_type % 5;
	if (actual_type == H264_SLICE_B) {
		out_sh->direct_spatial_mv_pred_flag = (uint8)H264_ReadBits(reader, 1);
	}

	if (actual_type == H264_SLICE_P || actual_type == H264_SLICE_SP || actual_type == H264_SLICE_B) {
		out_sh->num_ref_idx_active_override_flag = (uint8)H264_ReadBits(reader, 1);
		if (out_sh->num_ref_idx_active_override_flag) {
			out_sh->num_ref_idx_l0_active_minus1 = (uint8)H264_ReadUE(reader);
			if (actual_type == H264_SLICE_B) {
				out_sh->num_ref_idx_l1_active_minus1 = (uint8)H264_ReadUE(reader);
			}
		} else {
			out_sh->num_ref_idx_l0_active_minus1 = pps->num_ref_idx_l0_default_active_minus1;
			out_sh->num_ref_idx_l1_active_minus1 = pps->num_ref_idx_l1_default_active_minus1;
		}
	}

	// Ref pic list reordering
	out_sh->n_reorder_l0 = 0;
	out_sh->n_reorder_l1 = 0;
	if (actual_type != H264_SLICE_I && actual_type != H264_SLICE_SI) {
		if (H264_ReadBool(reader)) {
			uint32 reorder_idc;
			do {
				reorder_idc = H264_ReadUE(reader);
				uint32 reorder_arg = 0;
				if (reorder_idc == 0 || reorder_idc == 1 || reorder_idc == 2) {
					reorder_arg = H264_ReadUE(reader);
				} else {
					break;
				}
				if (out_sh->n_reorder_l0 < 32) {
					out_sh->reorder_l0[out_sh->n_reorder_l0++] = (reorder_idc << 16) | (reorder_arg & 0xFFFFu);
				}
			} while (H264_HasData(reader));
		}
		if (actual_type == H264_SLICE_B) {
			if (H264_ReadBool(reader)) {
				uint32 reorder_idc;
				do {
					reorder_idc = H264_ReadUE(reader);
					uint32 reorder_arg = 0;
					if (reorder_idc == 0 || reorder_idc == 1 || reorder_idc == 2) {
						reorder_arg = H264_ReadUE(reader);
					} else {
						break;
					}
					if (out_sh->n_reorder_l1 < 32) {
						out_sh->reorder_l1[out_sh->n_reorder_l1++] = (reorder_idc << 16) | (reorder_arg & 0xFFFFu);
					}
				} while (H264_HasData(reader));
			}
		}
	}

	// Weighted prediction table (H.264 7.3.3.2). The bits were always consumed here, but the values
	// were thrown away, so every partition that predicts from a reference which the encoder gave an
	// explicit weight and offset was reconstructed from the unweighted prediction. This stream does
	// use them (P slices carry luma_weight_l0[1] = 1 with luma_offset_l0[1] = -1), and a constant
	// offset on a subset of the pixels is exactly the kind of error that survives into the
	// reference pictures and accumulates. A cleared flag keeps the default weight, which is stored
	// as the effective value so that the reconstruction needs no further condition.
	if ((pps->weighted_pred_flag && (actual_type == H264_SLICE_P || actual_type == H264_SLICE_SP)) ||
		(pps->weighted_bipred_idc == 1 && actual_type == H264_SLICE_B)) {
		out_sh->weighted_pred_present = 1;
		uint32 luma_log2_weight_denom = H264_ReadUE(reader);
		uint32 chroma_log2_weight_denom = luma_log2_weight_denom;
		if (sps->chroma_format_idc != 0) {
			chroma_log2_weight_denom = H264_ReadUE(reader);
		}
		out_sh->luma_log2_weight_denom = (uint8)luma_log2_weight_denom;
		out_sh->chroma_log2_weight_denom = (uint8)chroma_log2_weight_denom;
		const int luma_def_w = 1 << (luma_log2_weight_denom & 7);
		const int chroma_def_w = 1 << (chroma_log2_weight_denom & 7);
		uint32 num_ref_l0 = out_sh->num_ref_idx_l0_active_minus1 + 1;
		if (num_ref_l0 > 32) num_ref_l0 = 32;
		for (uint32 i = 0; i < num_ref_l0 && H264_HasData(reader); ++i) {
			out_sh->luma_weight_l0[i] = (int16)luma_def_w;
			out_sh->luma_offset_l0[i] = 0;
			out_sh->chroma_weight_l0[i][0] = (int16)chroma_def_w;
			out_sh->chroma_weight_l0[i][1] = (int16)chroma_def_w;
			out_sh->chroma_offset_l0[i][0] = 0;
			out_sh->chroma_offset_l0[i][1] = 0;
			uint32 luma_weight_l0_flag = H264_ReadBits(reader, 1);
			if (luma_weight_l0_flag) {
				out_sh->luma_weight_l0[i] = (int16)H264_ReadSE(reader);
				out_sh->luma_offset_l0[i] = (int16)H264_ReadSE(reader);
			}
			if (sps->chroma_format_idc != 0) {
				uint32 chroma_weight_l0_flag = H264_ReadBits(reader, 1);
				if (chroma_weight_l0_flag) {
					for (int j = 0; j < 2; ++j) {
						out_sh->chroma_weight_l0[i][j] = (int16)H264_ReadSE(reader);
						out_sh->chroma_offset_l0[i][j] = (int16)H264_ReadSE(reader);
					}
				}
			}
		}
		if (actual_type == H264_SLICE_B) {
			uint32 num_ref_l1 = out_sh->num_ref_idx_l1_active_minus1 + 1;
			if (num_ref_l1 > 32) num_ref_l1 = 32;
			for (uint32 i = 0; i < num_ref_l1 && H264_HasData(reader); ++i) {
				out_sh->luma_weight_l1[i] = (int16)luma_def_w;
				out_sh->luma_offset_l1[i] = 0;
				out_sh->chroma_weight_l1[i][0] = (int16)chroma_def_w;
				out_sh->chroma_weight_l1[i][1] = (int16)chroma_def_w;
				out_sh->chroma_offset_l1[i][0] = 0;
				out_sh->chroma_offset_l1[i][1] = 0;
				uint32 luma_weight_l1_flag = H264_ReadBits(reader, 1);
				if (luma_weight_l1_flag) {
					out_sh->luma_weight_l1[i] = (int16)H264_ReadSE(reader);
					out_sh->luma_offset_l1[i] = (int16)H264_ReadSE(reader);
				}
				if (sps->chroma_format_idc != 0) {
					uint32 chroma_weight_l1_flag = H264_ReadBits(reader, 1);
					if (chroma_weight_l1_flag) {
						for (int j = 0; j < 2; ++j) {
							out_sh->chroma_weight_l1[i][j] = (int16)H264_ReadSE(reader);
							out_sh->chroma_offset_l1[i][j] = (int16)H264_ReadSE(reader);
						}
					}
				}
			}
		}
	}

	// Decoded ref pic marking
	out_sh->n_mmco = 0;
	out_sh->adaptive_ref_pic_marking = 0;
	if (nal_ref_idc != 0) {
		if (nal_unit_type == H264_NAL_SLICE_IDR) {
			H264_ReadBits(reader, 1); // no_output_of_prior_pics_flag
			H264_ReadBits(reader, 1); // long_term_reference_flag
		} else {
			out_sh->adaptive_ref_pic_marking = H264_ReadBool(reader) ? 1 : 0;
			if (out_sh->adaptive_ref_pic_marking) {
				uint32 mmco;
				do {
					mmco = H264_ReadUE(reader);
					// Every memory_management_control_operation that carries an argument reads
					// it here so that the bit reader lands on the next operation; the argument
					// itself is kept for the operations the decoded picture buffer can apply.
					uint32 arg = 0;
					if (mmco == 1) arg = H264_ReadUE(reader);
					if (mmco == 2) arg = H264_ReadUE(reader);
					if (mmco == 3) { arg = H264_ReadUE(reader); H264_ReadUE(reader); }
					if (mmco == 4) arg = H264_ReadUE(reader);
					if (mmco == 6) arg = H264_ReadUE(reader);
					if (mmco != 0 && out_sh->n_mmco < 16) {
						out_sh->mmco[out_sh->n_mmco++] = (mmco << 16) | (arg & 0xFFFF);
					}
				} while (mmco != 0 && H264_HasData(reader));
			}
		}
	}

	if (pps->entropy_coding_mode_flag && actual_type != H264_SLICE_I && actual_type != H264_SLICE_SI) {
		out_sh->cabac_init_idc = (uint8)H264_ReadUE(reader);
	}

	out_sh->slice_qp_delta = H264_ReadSE(reader);
	out_sh->slice_qp = 26 + (int32)pps->pic_init_qp_minus26 + out_sh->slice_qp_delta;

	// 7.3.2.1: SP/SI slices carry these after slice_qp_delta, before the deblocking parameters.
	if (actual_type == H264_SLICE_SP || actual_type == H264_SLICE_SI) {
		if (actual_type == H264_SLICE_SP) out_sh->sp_for_switch_flag = (uint8)H264_ReadBits(reader, 1);
		out_sh->slice_qs_delta = H264_ReadSE(reader);
	}

	if (pps->deblocking_filter_control_present_flag) {
		out_sh->disable_deblocking_filter_idc = (uint8)H264_ReadUE(reader);
		if (out_sh->disable_deblocking_filter_idc != 1) {
			out_sh->slice_alpha_c_offset_div2 = (int8)H264_ReadSE(reader);
			out_sh->slice_beta_offset_div2 = (int8)H264_ReadSE(reader);
		}
	}

	return true;
}

void H264_PredIntra4x4(
	uint8* dst,
	int stride,
	const uint8* top,
	const uint8* left,
	uint8 top_left,
	const uint8* top_right,
	bool has_top,
	bool has_left,
	bool has_top_left,
	bool has_top_right,
	int mode
) {
	if (!dst || stride <= 0) return;

	int A = has_top ? top[0] : 128;
	int B = has_top ? top[1] : 128;
	int C = has_top ? top[2] : 128;
	int D = has_top ? top[3] : 128;

	int I = has_left ? left[0] : 128;
	int J = has_left ? left[1] : 128;
	int K = has_left ? left[2] : 128;
	int L = has_left ? left[3] : 128;

	int M = has_top_left ? top_left : 128;

	int E = (has_top_right && top_right) ? top_right[0] : D;
	int F = (has_top_right && top_right) ? top_right[1] : D;
	int G = (has_top_right && top_right) ? top_right[2] : D;
	int H = (has_top_right && top_right) ? top_right[3] : D;

	switch (mode) {
		case 0: // Vertical
			if (!has_top) mode = 2; // Fallback to DC
			else {
				for (int y = 0; y < 4; ++y) {
					dst[y * stride + 0] = (uint8)A;
					dst[y * stride + 1] = (uint8)B;
					dst[y * stride + 2] = (uint8)C;
					dst[y * stride + 3] = (uint8)D;
				}
				return;
			}
			break;

		case 1: // Horizontal
			if (!has_left) mode = 2; // Fallback to DC
			else {
				for (int y = 0; y < 4; ++y) {
					uint8 val = left[y];
					dst[y * stride + 0] = val;
					dst[y * stride + 1] = val;
					dst[y * stride + 2] = val;
					dst[y * stride + 3] = val;
				}
				return;
			}
			break;

		case 2: // DC
			break;

		case 3: // Diagonal Down-Left
			if (!has_top) mode = 2;
			else {
				dst[0 * stride + 0] = (uint8)((A + 2 * B + C + 2) >> 2);
				dst[0 * stride + 1] = dst[1 * stride + 0] = (uint8)((B + 2 * C + D + 2) >> 2);
				dst[0 * stride + 2] = dst[1 * stride + 1] = dst[2 * stride + 0] = (uint8)((C + 2 * D + E + 2) >> 2);
				dst[0 * stride + 3] = dst[1 * stride + 2] = dst[2 * stride + 1] = dst[3 * stride + 0] = (uint8)((D + 2 * E + F + 2) >> 2);
				dst[1 * stride + 3] = dst[2 * stride + 2] = dst[3 * stride + 1] = (uint8)((E + 2 * F + G + 2) >> 2);
				dst[2 * stride + 3] = dst[3 * stride + 2] = (uint8)((F + 2 * G + H + 2) >> 2);
				dst[3 * stride + 3] = (uint8)((G + 3 * H + 2) >> 2);
				return;
			}
			break;

		case 4: // Diagonal Down-Right
			if (!has_top || !has_left || !has_top_left) mode = 2;
			else {
				dst[3 * stride + 0] = (uint8)((L + 2 * K + J + 2) >> 2);
				dst[2 * stride + 0] = dst[3 * stride + 1] = (uint8)((K + 2 * J + I + 2) >> 2);
				dst[1 * stride + 0] = dst[2 * stride + 1] = dst[3 * stride + 2] = (uint8)((J + 2 * I + M + 2) >> 2);
				dst[0 * stride + 0] = dst[1 * stride + 1] = dst[2 * stride + 2] = dst[3 * stride + 3] = (uint8)((I + 2 * M + A + 2) >> 2);
				dst[0 * stride + 1] = dst[1 * stride + 2] = dst[2 * stride + 3] = (uint8)((M + 2 * A + B + 2) >> 2);
				dst[0 * stride + 2] = dst[1 * stride + 3] = (uint8)((A + 2 * B + C + 2) >> 2);
				dst[0 * stride + 3] = (uint8)((B + 2 * C + D + 2) >> 2);
				return;
			}
			break;

		case 5: // Vertical-Right
			if (!has_top || !has_left || !has_top_left) mode = 2;
			else {
				dst[0 * stride + 0] = dst[2 * stride + 1] = (uint8)((M + A + 1) >> 1);
				dst[0 * stride + 1] = dst[2 * stride + 2] = (uint8)((A + B + 1) >> 1);
				dst[0 * stride + 2] = dst[2 * stride + 3] = (uint8)((B + C + 1) >> 1);
				dst[0 * stride + 3] = (uint8)((C + D + 1) >> 1);
				dst[1 * stride + 0] = dst[3 * stride + 1] = (uint8)((I + 2 * M + A + 2) >> 2);
				dst[1 * stride + 1] = dst[3 * stride + 2] = (uint8)((M + 2 * A + B + 2) >> 2);
				dst[1 * stride + 2] = dst[3 * stride + 3] = (uint8)((A + 2 * B + C + 2) >> 2);
				dst[1 * stride + 3] = (uint8)((B + 2 * C + D + 2) >> 2);
				dst[2 * stride + 0] = (uint8)((M + 2 * I + J + 2) >> 2);
				dst[3 * stride + 0] = (uint8)((I + 2 * J + K + 2) >> 2);
				return;
			}
			break;

		case 6: // Horizontal-Down
			if (!has_top || !has_left || !has_top_left) mode = 2;
			else {
				dst[0 * stride + 0] = dst[1 * stride + 2] = (uint8)((M + I + 1) >> 1);
				dst[1 * stride + 0] = dst[2 * stride + 2] = (uint8)((I + J + 1) >> 1);
				dst[2 * stride + 0] = dst[3 * stride + 2] = (uint8)((J + K + 1) >> 1);
				dst[3 * stride + 0] = (uint8)((K + L + 1) >> 1);
				dst[0 * stride + 1] = dst[1 * stride + 3] = (uint8)((I + 2 * M + A + 2) >> 2);
				dst[1 * stride + 1] = dst[2 * stride + 3] = (uint8)((M + 2 * I + J + 2) >> 2);
				dst[2 * stride + 1] = dst[3 * stride + 3] = (uint8)((I + 2 * J + K + 2) >> 2);
				dst[3 * stride + 1] = (uint8)((J + 2 * K + L + 2) >> 2);
				dst[0 * stride + 2] = (uint8)((M + 2 * A + B + 2) >> 2);
				dst[0 * stride + 3] = (uint8)((A + 2 * B + C + 2) >> 2);
				return;
			}
			break;

		case 7: // Vertical-Left
			if (!has_top) mode = 2;
			else {
				dst[0 * stride + 0] = (uint8)((A + B + 1) >> 1);
				dst[0 * stride + 1] = dst[2 * stride + 0] = (uint8)((B + C + 1) >> 1);
				dst[0 * stride + 2] = dst[2 * stride + 1] = (uint8)((C + D + 1) >> 1);
				dst[0 * stride + 3] = dst[2 * stride + 2] = (uint8)((D + E + 1) >> 1);
				dst[2 * stride + 3] = (uint8)((E + F + 1) >> 1);
				dst[1 * stride + 0] = (uint8)((A + 2 * B + C + 2) >> 2);
				dst[1 * stride + 1] = dst[3 * stride + 0] = (uint8)((B + 2 * C + D + 2) >> 2);
				dst[1 * stride + 2] = dst[3 * stride + 1] = (uint8)((C + 2 * D + E + 2) >> 2);
				dst[1 * stride + 3] = dst[3 * stride + 2] = (uint8)((D + 2 * E + F + 2) >> 2);
				dst[3 * stride + 3] = (uint8)((E + 2 * F + G + 2) >> 2);
				return;
			}
			break;

		case 8: // Horizontal-Up
			if (!has_left) mode = 2;
			else {
				dst[0 * stride + 0] = (uint8)((I + J + 1) >> 1);
				dst[0 * stride + 1] = (uint8)((I + 2 * J + K + 2) >> 2);
				dst[1 * stride + 0] = dst[0 * stride + 2] = (uint8)((J + K + 1) >> 1);
				dst[1 * stride + 1] = dst[0 * stride + 3] = (uint8)((J + 2 * K + L + 2) >> 2);
				dst[2 * stride + 0] = dst[1 * stride + 2] = (uint8)((K + L + 1) >> 1);
				dst[2 * stride + 1] = dst[1 * stride + 3] = (uint8)((K + 2 * L + L + 2) >> 2);
				dst[3 * stride + 0] = dst[2 * stride + 2] = dst[2 * stride + 3] =
				dst[3 * stride + 1] = dst[3 * stride + 2] = dst[3 * stride + 3] = (uint8)L;
				return;
			}
			break;

		default:
			mode = 2;
			break;
	}

	// Mode 2: DC
	int dc_val = 128;
	if (has_top && has_left) {
		dc_val = (A + B + C + D + I + J + K + L + 4) >> 3;
	} else if (has_top) {
		dc_val = (A + B + C + D + 2) >> 2;
	} else if (has_left) {
		dc_val = (I + J + K + L + 2) >> 2;
	}

	for (int y = 0; y < 4; ++y) {
		dst[y * stride + 0] = (uint8)dc_val;
		dst[y * stride + 1] = (uint8)dc_val;
		dst[y * stride + 2] = (uint8)dc_val;
		dst[y * stride + 3] = (uint8)dc_val;
	}
}

void H264_PredIntra16x16(
	uint8* dst,
	int stride,
	const uint8* top,
	const uint8* left,
	uint8 top_left,
	bool has_top,
	bool has_left,
	bool has_top_left,
	int mode
) {
	if (!dst || stride <= 0) return;

	switch (mode) {
		case 0: // Vertical
			if (!has_top) mode = 2;
			else {
				for (int y = 0; y < 16; ++y) {
					for (int x = 0; x < 16; ++x) {
						dst[y * stride + x] = top[x];
					}
				}
				return;
			}
			break;

		case 1: // Horizontal
			if (!has_left) mode = 2;
			else {
				for (int y = 0; y < 16; ++y) {
					uint8 val = left[y];
					for (int x = 0; x < 16; ++x) {
						dst[y * stride + x] = val;
					}
				}
				return;
			}
			break;

		case 3: // Plane
			if (!has_top || !has_left || !has_top_left) mode = 2;
			else {
				int H = 0, V = 0;
				// The last difference of the spec sums is p[6-x', -1] with x' = 7, i.e. the
				// top-left neighbour p[-1, -1], NOT an out-of-range index of the top row.
				for (int x = 0; x < 8; ++x) {
					H += (x + 1) * ((int)top[8 + x] - (int)(x == 7 ? top_left : top[6 - x]));
				}
				for (int y = 0; y < 8; ++y) {
					V += (y + 1) * ((int)left[8 + y] - (int)(y == 7 ? top_left : left[6 - y]));
				}
				int a = 16 * ((int)left[15] + (int)top[15]);
				int b = (5 * H + 32) >> 6;
				int c = (5 * V + 32) >> 6;

				for (int y = 0; y < 16; ++y) {
					for (int x = 0; x < 16; ++x) {
						int val = (a + b * (x - 7) + c * (y - 7) + 16) >> 5;
						dst[y * stride + x] = Clip8(val);
					}
				}
				return;
			}
			break;

		default:
			mode = 2;
			break;
	}

	// Mode 2: DC
	int dc_val = 128;
	if (has_top && has_left) {
		int sum = 0;
		for (int i = 0; i < 16; ++i) sum += (int)top[i] + (int)left[i];
		dc_val = (sum + 16) >> 5;
	} else if (has_top) {
		int sum = 0;
		for (int i = 0; i < 16; ++i) sum += (int)top[i];
		dc_val = (sum + 8) >> 4;
	} else if (has_left) {
		int sum = 0;
		for (int i = 0; i < 16; ++i) sum += (int)left[i];
		dc_val = (sum + 8) >> 4;
	}

	for (int y = 0; y < 16; ++y) {
		for (int x = 0; x < 16; ++x) {
			dst[y * stride + x] = (uint8)dc_val;
		}
	}
}

void H264_PredIntraChroma8x8(
	uint8* dst,
	int stride,
	const uint8* top,
	const uint8* left,
	uint8 top_left,
	bool has_top,
	bool has_left,
	bool has_top_left,
	int mode
) {
	if (!dst || stride <= 0) return;

	switch (mode) {
		case 1: // Horizontal
			if (!has_left) mode = 0;
			else {
				for (int y = 0; y < 8; ++y) {
					uint8 val = left[y];
					for (int x = 0; x < 8; ++x) {
						dst[y * stride + x] = val;
					}
				}
				return;
			}
			break;

		case 2: // Vertical
			if (!has_top) mode = 0;
			else {
				for (int y = 0; y < 8; ++y) {
					for (int x = 0; x < 8; ++x) {
						dst[y * stride + x] = top[x];
					}
				}
				return;
			}
			break;

		case 3: // Plane
			if (!has_top || !has_left || !has_top_left) mode = 0;
			else {
				int H = 0, V = 0;
				// The last difference of the spec sums is p[2-x', -1] with x' = 3, i.e. the
				// top-left neighbour p[-1, -1], NOT an out-of-range index of the top row.
				for (int x = 0; x < 4; ++x) {
					H += (x + 1) * ((int)top[4 + x] - (int)(x == 3 ? top_left : top[2 - x]));
				}
				for (int y = 0; y < 4; ++y) {
					V += (y + 1) * ((int)left[4 + y] - (int)(y == 3 ? top_left : left[2 - y]));
				}
				int a = 16 * ((int)left[7] + (int)top[7]);
				int b = (17 * H + 16) >> 5;
				int c = (17 * V + 16) >> 5;

				for (int y = 0; y < 8; ++y) {
					for (int x = 0; x < 8; ++x) {
						int val = (a + b * (x - 3) + c * (y - 3) + 16) >> 5;
						dst[y * stride + x] = Clip8(val);
					}
				}
				return;
			}
			break;

		default:
			mode = 0;
			break;
	}

	// Mode 0: DC (4 sub-blocks)
	int dc[4] = { 128, 128, 128, 128 };
	int top_sum0 = has_top ? (top[0] + top[1] + top[2] + top[3]) : 0;
	int top_sum1 = has_top ? (top[4] + top[5] + top[6] + top[7]) : 0;
	int left_sum0 = has_left ? (left[0] + left[1] + left[2] + left[3]) : 0;
	int left_sum1 = has_left ? (left[4] + left[5] + left[6] + left[7]) : 0;

	if (has_top && has_left) {
		dc[0] = (top_sum0 + left_sum0 + 4) >> 3;
		dc[1] = (top_sum1 + 2) >> 2;
		dc[2] = (left_sum1 + 2) >> 2;
		dc[3] = (top_sum1 + left_sum1 + 4) >> 3;
	} else if (has_top) {
		dc[0] = (top_sum0 + 2) >> 2;
		dc[1] = (top_sum1 + 2) >> 2;
		dc[2] = (top_sum0 + 2) >> 2;
		dc[3] = (top_sum1 + 2) >> 2;
	} else if (has_left) {
		dc[0] = (left_sum0 + 2) >> 2;
		dc[1] = (left_sum0 + 2) >> 2;
		dc[2] = (left_sum1 + 2) >> 2;
		dc[3] = (left_sum1 + 2) >> 2;
	}

	for (int y = 0; y < 4; ++y) {
		for (int x = 0; x < 4; ++x) dst[(0 + y) * stride + (0 + x)] = (uint8)dc[0];
		for (int x = 0; x < 4; ++x) dst[(0 + y) * stride + (4 + x)] = (uint8)dc[1];
		for (int x = 0; x < 4; ++x) dst[(4 + y) * stride + (0 + x)] = (uint8)dc[2];
		for (int x = 0; x < 4; ++x) dst[(4 + y) * stride + (4 + x)] = (uint8)dc[3];
	}
}

bool H264_DecodeSlice_I(
	const byte* rbsp_data,
	size_t rbsp_len,
	const H264SPS* sps,
	const H264PPS* pps,
	uint8 nal_unit_type,
	uint8 nal_ref_idc,
	uint8* y_plane,
	uint8* u_plane,
	uint8* v_plane,
	int y_stride,
	int uv_stride
) {
	if (!rbsp_data || rbsp_len == 0 || !sps || !pps || !y_plane || !u_plane || !v_plane) return false;

	H264BitReader reader;
	H264_InitBitReader(&reader, rbsp_data, rbsp_len);

	H264SliceHeader sh;
	if (!H264_ParseSliceHeader(&reader, sps, pps, nal_unit_type, nal_ref_idc, &sh)) {
		return false;
	}

	uint32 mb_width = sps->mb_width;
	uint32 mb_height = sps->mb_height;
	uint32 num_mbs = mb_width * mb_height;

	if (sh.first_mb_in_slice >= num_mbs) return false;

	// Allocate temporary context tracking tables
	size_t nnz_size = (size_t)num_mbs * 24; // 16 luma + 4 u + 4 v
	size_t mode_size = (size_t)num_mbs * 16;
	uint8* nnz_table = (uint8*)malloc(nnz_size);
	int8* mode_table = (int8*)malloc(mode_size);
	if (!nnz_table || !mode_table) {
		if (nnz_table) free(nnz_table);
		if (mode_table) free(mode_table);
		return false;
	}
	MemSet(nnz_table, 0, nnz_size);
	MemSet(mode_table, -1, mode_size);

	int32 current_qp = sh.slice_qp;

	for (uint32 mb_idx = sh.first_mb_in_slice; mb_idx < num_mbs && H264_HasData(&reader); ++mb_idx) {
		uint32 mb_x = mb_idx % mb_width;
		uint32 mb_y = mb_idx / mb_width;

		uint8* mb_y_dst = y_plane + mb_y * 16 * y_stride + mb_x * 16;
		uint8* mb_u_dst = u_plane + mb_y * 8 * uv_stride + mb_x * 8;
		uint8* mb_v_dst = v_plane + mb_y * 8 * uv_stride + mb_x * 8;

		uint32 mb_type = H264_ReadUE(&reader);

		if (mb_type == 0) { // I_NxN (Intra_4x4)
			// 1. Read Intra_4x4 prediction modes
			int8 sub_modes[16];
			for (int k = 0; k < 16; ++k) {
				int bx = ((k & 1) ? 1 : 0) + ((k & 4) ? 2 : 0);
				int by = ((k & 2) ? 1 : 0) + ((k & 8) ? 2 : 0);

				int mode_A = 2; // DC default
				if (bx > 0) {
					int kA = (by & 2) * 4 + ((bx - 1) & 2) * 2 + (by & 1) * 2 + ((bx - 1) & 1);
					int8 mA = mode_table[mb_idx * 16 + kA];
					if (mA >= 0) mode_A = (int)mA;
				} else if (mb_x > 0) {
					int kA = (by & 2) * 4 + (3 & 2) * 2 + (by & 1) * 2 + (3 & 1);
					int8 mA = mode_table[(mb_idx - 1) * 16 + kA];
					if (mA >= 0) mode_A = (int)mA;
				}

				int mode_B = 2; // DC default
				if (by > 0) {
					int kB = ((by - 1) & 2) * 4 + (bx & 2) * 2 + ((by - 1) & 1) * 2 + (bx & 1);
					int8 mB = mode_table[mb_idx * 16 + kB];
					if (mB >= 0) mode_B = (int)mB;
				} else if (mb_y > 0) {
					int kB = (3 & 2) * 4 + (bx & 2) * 2 + (3 & 1) * 2 + (bx & 1);
					int8 mB = mode_table[(mb_idx - mb_width) * 16 + kB];
					if (mB >= 0) mode_B = (int)mB;
				}

				if ((bx == 0 && mb_x == 0) || (by == 0 && mb_y == 0)) { mode_A = 2; mode_B = 2; } // 8.3.1.1: 任一侧邻居宏块不可用则 A、B 都按 DC 预测
				int mpm = (mode_A < mode_B) ? mode_A : mode_B;

				bool prev_flag = H264_ReadBool(&reader);
				if (prev_flag) {
					sub_modes[k] = (int8)mpm;
				} else {
					int rem = (int)H264_ReadBits(&reader, 3);
					sub_modes[k] = (int8)((rem < mpm) ? rem : (rem + 1));
				}
				mode_table[mb_idx * 16 + k] = sub_modes[k];
			}

			uint32 intra_chroma_mode = H264_ReadUE(&reader);
			uint32 cbp_code = H264_ReadUE(&reader);
			uint8 cbp = (cbp_code < 48) ? golomb_to_intra4x4_cbp[cbp_code] : 0;
			uint8 cbp_luma = cbp & 15;
			uint8 cbp_chroma = (cbp >> 4) & 3;

			if (cbp > 0) {
				int32 delta = H264_ReadSE(&reader);
				current_qp = (current_qp + delta + 52) % 52;
			}

			// 2. Decode & reconstruct 16 4x4 Luma blocks
			for (int k = 0; k < 16; ++k) {
				int bx = ((k & 1) ? 1 : 0) + ((k & 4) ? 2 : 0);
				int by = ((k & 2) ? 1 : 0) + ((k & 8) ? 2 : 0);

				// Compute nC for CAVLC
				int nA = 0, nB = 0;
				bool has_A = false, has_B = false;

				if (bx > 0) {
					int kA = (by & 2) * 4 + ((bx - 1) & 2) * 2 + (by & 1) * 2 + ((bx - 1) & 1);
					nA = nnz_table[mb_idx * 24 + kA];
					has_A = true;
				} else if (mb_x > 0) {
					int kA = (by & 2) * 4 + (3 & 2) * 2 + (by & 1) * 2 + (3 & 1);
					nA = nnz_table[(mb_idx - 1) * 24 + kA];
					has_A = true;
				}

				if (by > 0) {
					int kB = ((by - 1) & 2) * 4 + (bx & 2) * 2 + ((by - 1) & 1) * 2 + (bx & 1);
					nB = nnz_table[mb_idx * 24 + kB];
					has_B = true;
				} else if (mb_y > 0) {
					int kB = (3 & 2) * 4 + (bx & 2) * 2 + (3 & 1) * 2 + (bx & 1);
					nB = nnz_table[(mb_idx - mb_width) * 24 + kB];
					has_B = true;
				}

				int nC = 0;
				if (has_A && has_B) nC = (nA + nB + 1) >> 1;
				else if (has_A) nC = nA;
				else if (has_B) nC = nB;

				int16 coeffs[16] = { 0 };
				int32 total_coeff = 0;
				if ((cbp_luma & (1 << (k >> 2))) != 0) {
					H264_DecodeCAVLC_Block(&reader, nC, 16, coeffs, &total_coeff);
				}
				nnz_table[mb_idx * 24 + k] = (uint8)total_coeff;

				int32 residual[16] = { 0 };
				if (total_coeff > 0) {
					H264_Dequant4x4(coeffs, current_qp, false, residual);
					H264_IDCT4x4(residual, residual);
				}

				// Neighbor samples for Intra_4x4
				uint8 top_buf[4], left_buf[4], tr_buf[4];
				uint8 tl_val = 128;
				bool blk_has_top = (by > 0) || (mb_y > 0);
				bool blk_has_left = (bx > 0) || (mb_x > 0);
				bool blk_has_tl = false, blk_has_tr = false;

				uint8* blk_dst = mb_y_dst + by * 4 * y_stride + bx * 4;

				if (blk_has_top) {
					const uint8* top_ptr = blk_dst - y_stride;
					top_buf[0] = top_ptr[0]; top_buf[1] = top_ptr[1];
					top_buf[2] = top_ptr[2]; top_buf[3] = top_ptr[3];
				}
				if (blk_has_left) {
					const uint8* left_ptr = blk_dst - 1;
					left_buf[0] = left_ptr[0 * y_stride]; left_buf[1] = left_ptr[1 * y_stride];
					left_buf[2] = left_ptr[2 * y_stride]; left_buf[3] = left_ptr[3 * y_stride];
				}
				if (blk_has_top && blk_has_left) {
					tl_val = *(blk_dst - y_stride - 1);
					blk_has_tl = true;
				}

				bool tr_available = false;
				if (by == 0) {
					if (mb_y > 0 && (bx < 3 || mb_x + 1 < mb_width)) {
						tr_available = true;
					}
				} else {
					if (k == 2 || k == 6 || k == 8 || k == 9 || k == 10 || k == 12 || k == 14) {
						tr_available = true;
					}
				}

				if (tr_available) {
					const uint8* tr_ptr = blk_dst - y_stride + 4;
					tr_buf[0] = tr_ptr[0]; tr_buf[1] = tr_ptr[1];
					tr_buf[2] = tr_ptr[2]; tr_buf[3] = tr_ptr[3];
					blk_has_tr = true;
				}

				uint8 pred_buf[16];
				H264_PredIntra4x4(pred_buf, 4, top_buf, left_buf, tl_val, tr_buf,
								  blk_has_top, blk_has_left, blk_has_tl, blk_has_tr, sub_modes[k]);

				for (int y = 0; y < 4; ++y) {
					for (int x = 0; x < 4; ++x) {
						int recon = (int)pred_buf[y * 4 + x] + residual[y * 4 + x];
						blk_dst[y * y_stride + x] = Clip8(recon);
					}
				}
			}

			// 3. Decode & reconstruct Chroma (Cb & Cr)
			uint8 qp_c = H264_GetChromaQP(current_qp, pps->chroma_qp_index_offset);

			// H.264 7.3.5.3 residual() has two separate for(iCbCr) loops: the chroma DC blocks of
			// BOTH components are decoded before ANY chroma AC block.  Interleaving the two
			// components desynchronises the CAVLC parse from the first macroblock whose
			// CodedBlockPatternChroma is 2.
			stdsint chroma_scaled_dc[2][4];
			MemSet(chroma_scaled_dc, 0, sizeof(chroma_scaled_dc));
			if (cbp_chroma > 0) {
				for (int plane = 0; plane < 2; ++plane) {
					int16 dc_coeffs[4] = { 0 };
					int32 total_dc = 0;
					H264_DecodeCAVLC_Block(&reader, -1, 4, dc_coeffs, &total_dc);
					if (total_dc > 0) {
						stdsint dc_in[4] = { dc_coeffs[0], dc_coeffs[1], dc_coeffs[2], dc_coeffs[3] };
						Hadamard_H264_2x2(dc_in, qp_c, chroma_scaled_dc[plane]);
					}
				}
			}

			for (int plane = 0; plane < 2; ++plane) {
				uint8* ch_dst = (plane == 0) ? mb_u_dst : mb_v_dst;
				int nnz_offset = 16 + plane * 4;

				int32 residuals[4][16];
				MemSet(residuals, 0, sizeof(residuals));

				for (int ck = 0; ck < 4; ++ck) {
					int cbx = ck & 1;
					int cby = (ck >> 1) & 1;

					int16 ac_coeffs[16] = { 0 };
					int32 total_ac = 0;
					if (cbp_chroma == 2) {
						int nA_c = 0, nB_c = 0; bool okA_c = false, okB_c = false;
						int qbx = ck & 1, qby = (ck >> 1) & 1;
						if (qbx > 0) { nA_c = nnz_table[mb_idx * 24 + nnz_offset + ck - 1]; okA_c = true; }
						else if (mb_x > 0) { nA_c = nnz_table[(mb_idx - 1) * 24 + nnz_offset + 2 * qby + 1]; okA_c = true; }
						if (qby > 0) { nB_c = nnz_table[mb_idx * 24 + nnz_offset + ck - 2]; okB_c = true; }
						else if (mb_y > 0) { nB_c = nnz_table[(mb_idx - mb_width) * 24 + nnz_offset + 2 + qbx]; okB_c = true; }
						int nC_c = (okA_c && okB_c) ? ((nA_c + nB_c + 1) >> 1) : (nA_c + nB_c);
						H264_DecodeCAVLC_Block(&reader, nC_c, 15, ac_coeffs, &total_ac);
					}
					nnz_table[mb_idx * 24 + nnz_offset + ck] = (uint8)total_ac;

					stdsint scaled_block[16] = { 0 };
					if (total_ac > 0) {
						Dequant_H264_4x4(ac_coeffs, qp_c, true, scaled_block);
					}
					scaled_block[0] = chroma_scaled_dc[plane][ck];
					stdsint res_s[16];
					IDCT_H264_4x4(scaled_block, res_s);
					for (int i = 0; i < 16; ++i) residuals[ck][i] = (int32)res_s[i];
				}

				uint8 top_c[8] = { 128, 128, 128, 128, 128, 128, 128, 128 }, left_c[8] = { 128, 128, 128, 128, 128, 128, 128, 128 }, tl_c = 128;
				bool ch_has_top = (mb_y > 0);
				bool ch_has_left = (mb_x > 0);
				bool ch_has_tl = (mb_y > 0 && mb_x > 0);

				if (ch_has_top) {
					const uint8* tptr = ch_dst - uv_stride;
					for (int i = 0; i < 8; ++i) top_c[i] = tptr[i];
				}
				if (ch_has_left) {
					const uint8* lptr = ch_dst - 1;
					for (int i = 0; i < 8; ++i) left_c[i] = lptr[i * uv_stride];
				}
				if (ch_has_tl) {
					tl_c = *(ch_dst - uv_stride - 1);
				}

				uint8 pred_c[64];
				H264_PredIntraChroma8x8(pred_c, 8, top_c, left_c, tl_c,
										ch_has_top, ch_has_left, ch_has_tl, (int)intra_chroma_mode);

				for (int ck = 0; ck < 4; ++ck) {
					int cbx = (ck & 1) * 4;
					int cby = ((ck >> 1) & 1) * 4;
					for (int y = 0; y < 4; ++y) {
						for (int x = 0; x < 4; ++x) {
							int recon = (int)pred_c[(cby + y) * 8 + (cbx + x)] + residuals[ck][y * 4 + x];
							ch_dst[(cby + y) * uv_stride + (cbx + x)] = Clip8(recon);
						}
					}
				}
			}

		} else if (mb_type >= 1 && mb_type <= 24) { // I_16x16
			int intra16_mode = (int)(mb_type - 1) % 4;
			int cbp_chroma = (int)((mb_type - 1) / 4) % 3;
			int cbp_luma = (int)((mb_type - 1) / 12) * 15;

			uint32 intra_chroma_mode = H264_ReadUE(&reader);
			int32 delta = H264_ReadSE(&reader);
			current_qp = (current_qp + delta + 52) % 52;

			// Luma DC
			int16 luma_dc[16] = { 0 };
			int32 total_dc = 0;
			{
				int nA_d = 0, nB_d = 0; bool okA_d = false, okB_d = false;
				if (mb_x > 0) { nA_d = nnz_table[(mb_idx - 1) * 24 + 5]; okA_d = true; }
				if (mb_y > 0) { nB_d = nnz_table[(mb_idx - mb_width) * 24 + 10]; okB_d = true; }
				int nC_d = (okA_d && okB_d) ? ((nA_d + nB_d + 1) >> 1) : (nA_d + nB_d);
				H264_DecodeCAVLC_Block(&reader, nC_d, 16, luma_dc, &total_dc);
			}

			stdsint scaled_luma_dc[16] = { 0 };
			if (total_dc > 0) {
				stdsint dc_in[16];
				for (int i = 0; i < 16; ++i) dc_in[i] = (stdsint)luma_dc[i];
				Hadamard_H264_4x4(dc_in, current_qp, scaled_luma_dc);
			}

			// 16 Luma AC blocks
			int32 residuals[16][16];
			MemSet(residuals, 0, sizeof(residuals));

			for (int k = 0; k < 16; ++k) {
				int bx = ((k & 1) ? 1 : 0) + ((k & 4) ? 2 : 0);
				int by = ((k & 2) ? 1 : 0) + ((k & 8) ? 2 : 0);
				int16 ac_coeffs[16] = { 0 };
				int32 total_ac = 0;
				if (cbp_luma > 0) {
					int nA_a = 0, nB_a = 0; bool okA_a = false, okB_a = false;
				if (bx > 0) { nA_a = nnz_table[mb_idx * 24 + (by & 2) * 4 + ((bx - 1) & 2) * 2 + (by & 1) * 2 + ((bx - 1) & 1)]; okA_a = true; }
				else if (mb_x > 0) { nA_a = nnz_table[(mb_idx - 1) * 24 + (by & 2) * 4 + (by & 1) * 2 + 5]; okA_a = true; }
				if (by > 0) { nB_a = nnz_table[mb_idx * 24 + ((by - 1) & 2) * 4 + (bx & 2) * 2 + ((by - 1) & 1) * 2 + (bx & 1)]; okB_a = true; }
				else if (mb_y > 0) { nB_a = nnz_table[(mb_idx - mb_width) * 24 + 10 + (bx & 2) * 2 + (bx & 1)]; okB_a = true; }
				int nC_a = (okA_a && okB_a) ? ((nA_a + nB_a + 1) >> 1) : (nA_a + nB_a);
				H264_DecodeCAVLC_Block(&reader, nC_a, 15, ac_coeffs, &total_ac);
				}
				nnz_table[mb_idx * 24 + k] = (uint8)total_ac;

				stdsint scaled_block[16] = { 0 };
				if (total_ac > 0) {
					Dequant_H264_4x4(ac_coeffs, current_qp, true, scaled_block);
				}
				scaled_block[0] = scaled_luma_dc[(k & 9) | ((k & 2) << 1) | ((k & 4) >> 1)];    // Figure 8-6: dcY_ij belongs to the block at ( col j, row i )
				stdsint res_s[16];
				IDCT_H264_4x4(scaled_block, res_s);
				for (int i = 0; i < 16; ++i) residuals[k][i] = (int32)res_s[i];
			}

			// Intra_16x16 Prediction
			uint8 top_16[16], left_16[16], tl_16 = 128;
			bool has_top = (mb_y > 0);
			bool has_left = (mb_x > 0);
			bool has_tl = (mb_y > 0 && mb_x > 0);

			if (has_top) {
				const uint8* tptr = mb_y_dst - y_stride;
				for (int i = 0; i < 16; ++i) top_16[i] = tptr[i];
			}
			if (has_left) {
				const uint8* lptr = mb_y_dst - 1;
				for (int i = 0; i < 16; ++i) left_16[i] = lptr[i * y_stride];
			}
			if (has_tl) {
				tl_16 = *(mb_y_dst - y_stride - 1);
			}

			uint8 pred_16[256];
			H264_PredIntra16x16(pred_16, 16, top_16, left_16, tl_16, has_top, has_left, has_tl, intra16_mode);

			for (int k = 0; k < 16; ++k) {
				int bx = ((k & 1) ? 1 : 0) + ((k & 4) ? 2 : 0);
				int by = ((k & 2) ? 1 : 0) + ((k & 8) ? 2 : 0);
				for (int y = 0; y < 4; ++y) {
					for (int x = 0; x < 4; ++x) {
						int recon = (int)pred_16[(by * 4 + y) * 16 + (bx * 4 + x)] + residuals[k][y * 4 + x];
						mb_y_dst[(by * 4 + y) * y_stride + (bx * 4 + x)] = Clip8(recon);
					}
				}
			}

			// Chroma for Intra_16x16
			uint8 qp_c = H264_GetChromaQP(current_qp, pps->chroma_qp_index_offset);

			// H.264 7.3.5.3 residual() has two separate for(iCbCr) loops: the chroma DC blocks of
			// BOTH components are decoded before ANY chroma AC block.  Interleaving the two
			// components desynchronises the CAVLC parse from the first macroblock whose
			// CodedBlockPatternChroma is 2.
			stdsint chroma_scaled_dc[2][4];
			MemSet(chroma_scaled_dc, 0, sizeof(chroma_scaled_dc));
			if (cbp_chroma > 0) {
				for (int plane = 0; plane < 2; ++plane) {
					int16 dc_coeffs[4] = { 0 };
					int32 total_dc = 0;
					H264_DecodeCAVLC_Block(&reader, -1, 4, dc_coeffs, &total_dc);
					if (total_dc > 0) {
						stdsint dc_in[4] = { dc_coeffs[0], dc_coeffs[1], dc_coeffs[2], dc_coeffs[3] };
						Hadamard_H264_2x2(dc_in, qp_c, chroma_scaled_dc[plane]);
					}
				}
			}

			for (int plane = 0; plane < 2; ++plane) {
				uint8* ch_dst = (plane == 0) ? mb_u_dst : mb_v_dst;
				int nnz_offset = 16 + plane * 4;

				int32 residuals_c[4][16];
				MemSet(residuals_c, 0, sizeof(residuals_c));

				for (int ck = 0; ck < 4; ++ck) {
					int16 ac_coeffs[16] = { 0 };
					int32 total_ac = 0;
					if (cbp_chroma == 2) {
						int nA_c = 0, nB_c = 0; bool okA_c = false, okB_c = false;
						int qbx = ck & 1, qby = (ck >> 1) & 1;
						if (qbx > 0) { nA_c = nnz_table[mb_idx * 24 + nnz_offset + ck - 1]; okA_c = true; }
						else if (mb_x > 0) { nA_c = nnz_table[(mb_idx - 1) * 24 + nnz_offset + 2 * qby + 1]; okA_c = true; }
						if (qby > 0) { nB_c = nnz_table[mb_idx * 24 + nnz_offset + ck - 2]; okB_c = true; }
						else if (mb_y > 0) { nB_c = nnz_table[(mb_idx - mb_width) * 24 + nnz_offset + 2 + qbx]; okB_c = true; }
						int nC_c = (okA_c && okB_c) ? ((nA_c + nB_c + 1) >> 1) : (nA_c + nB_c);
						H264_DecodeCAVLC_Block(&reader, nC_c, 15, ac_coeffs, &total_ac);
					}
					nnz_table[mb_idx * 24 + nnz_offset + ck] = (uint8)total_ac;

					stdsint scaled_block[16] = { 0 };
					if (total_ac > 0) {
						Dequant_H264_4x4(ac_coeffs, qp_c, true, scaled_block);
					}
					scaled_block[0] = chroma_scaled_dc[plane][ck];
					stdsint res_s[16];
					IDCT_H264_4x4(scaled_block, res_s);
					for (int i = 0; i < 16; ++i) residuals_c[ck][i] = (int32)res_s[i];
				}

				uint8 top_c[8] = { 128, 128, 128, 128, 128, 128, 128, 128 }, left_c[8] = { 128, 128, 128, 128, 128, 128, 128, 128 }, tl_c = 128;
				bool ch_has_top = (mb_y > 0);
				bool ch_has_left = (mb_x > 0);
				bool ch_has_tl = (mb_y > 0 && mb_x > 0);

				if (ch_has_top) {
					const uint8* tptr = ch_dst - uv_stride;
					for (int i = 0; i < 8; ++i) top_c[i] = tptr[i];
				}
				if (ch_has_left) {
					const uint8* lptr = ch_dst - 1;
					for (int i = 0; i < 8; ++i) left_c[i] = lptr[i * uv_stride];
				}
				if (ch_has_tl) {
					tl_c = *(ch_dst - uv_stride - 1);
				}

				uint8 pred_c[64];
				H264_PredIntraChroma8x8(pred_c, 8, top_c, left_c, tl_c,
										ch_has_top, ch_has_left, ch_has_tl, (int)intra_chroma_mode);

				for (int ck = 0; ck < 4; ++ck) {
					int cbx = (ck & 1) * 4;
					int cby = ((ck >> 1) & 1) * 4;
					for (int y = 0; y < 4; ++y) {
						for (int x = 0; x < 4; ++x) {
							int recon = (int)pred_c[(cby + y) * 8 + (cbx + x)] + residuals_c[ck][y * 4 + x];
							ch_dst[(cby + y) * uv_stride + (cbx + x)] = Clip8(recon);
						}
					}
				}
			}
		} else if (mb_type == 25) { // I_PCM: raw samples, no prediction and no transform
			// H.264 7.3.5: pcm_alignment_zero_bit, then 256 luma and 2 * 8 * 8 chroma samples.
			H264_ByteAlign(&reader);
			for (int y = 0; y < 16; ++y) {
				for (int x = 0; x < 16; ++x) {
					mb_y_dst[y * y_stride + x] = (uint8)H264_ReadBits(&reader, 8);
				}
			}
			for (int plane = 0; plane < 2; ++plane) {
				uint8* pcm_dst = (plane == 0) ? mb_u_dst : mb_v_dst;
				for (int y = 0; y < 8; ++y) {
					for (int x = 0; x < 8; ++x) {
						pcm_dst[y * uv_stride + x] = (uint8)H264_ReadBits(&reader, 8);
					}
				}
			}
			// 9.2.1: a neighbouring I_PCM macroblock contributes nN = 16, and its 4x4
			// prediction mode is treated as DC by 8.3.1.1.
			for (int t = 0; t < 24; ++t) nnz_table[mb_idx * 24 + t] = 16;
			mode_table[mb_idx * 16 + 0] = 2;
		}
	}

	free(mode_table);
	free(nnz_table);
	return true;
}

// Phase 4: Inter Prediction, Motion Compensation & P-Slice Decoding

static const uint8 golomb_to_inter_cbp[48] = {
	 0, 16,  1,  2,  4,  8, 32,  3,  5, 10, 12, 15, 47,  7, 11, 13,
	14,  6,  9, 31, 35, 37, 42, 44, 33, 34, 36, 40, 39, 43, 45, 46,
	17, 18, 20, 24, 19, 21, 26, 28, 23, 27, 29, 30, 22, 25, 38, 41
};

static inline uint8 GetPixelClamped(const uint8* ref, int ref_stride, int x, int y, int w, int h) {
	if (x < 0) x = 0; else if (x >= w) x = w - 1;
	if (y < 0) y = 0; else if (y >= h) y = h - 1;
	return ref[y * ref_stride + x];
}

static inline int Filter6TapH(const uint8* ref, int ref_stride, int x, int y, int w, int h) {
	int p0 = GetPixelClamped(ref, ref_stride, x - 2, y, w, h);
	int p1 = GetPixelClamped(ref, ref_stride, x - 1, y, w, h);
	int p2 = GetPixelClamped(ref, ref_stride, x,     y, w, h);
	int p3 = GetPixelClamped(ref, ref_stride, x + 1, y, w, h);
	int p4 = GetPixelClamped(ref, ref_stride, x + 2, y, w, h);
	int p5 = GetPixelClamped(ref, ref_stride, x + 3, y, w, h);
	return p0 - 5 * p1 + 20 * p2 + 20 * p3 - 5 * p4 + p5;
}

static inline int Filter6TapV(const uint8* ref, int ref_stride, int x, int y, int w, int h) {
	int p0 = GetPixelClamped(ref, ref_stride, x, y - 2, w, h);
	int p1 = GetPixelClamped(ref, ref_stride, x, y - 1, w, h);
	int p2 = GetPixelClamped(ref, ref_stride, x, y,     w, h);
	int p3 = GetPixelClamped(ref, ref_stride, x, y + 1, w, h);
	int p4 = GetPixelClamped(ref, ref_stride, x, y + 2, w, h);
	int p5 = GetPixelClamped(ref, ref_stride, x, y + 3, w, h);
	return p0 - 5 * p1 + 20 * p2 + 20 * p3 - 5 * p4 + p5;
}

// Weighted sample prediction (H.264 8.4.2.2.2 for P, 8.4.2.2.3 explicit for B): the motion
// compensated prediction is scaled and offset per reference index, pred = ( w * mc + rnd ) >> denom
// + o.  The slice decoder sets these as soon as a partition picks its reference, so the motion
// compensation itself does not have to carry the table.  Index 0 = luma, 1 = Cb, 2 = Cr; the
// initial weight 1 with denom 0 is the identity, which every slice without a weighted prediction
// table resolves to (a cleared weight flag also keeps the default, so it stores the identity too).
// Each slice decoder assigns all three triples on entry, so no slice can inherit the previous one.
#define H264_WP_IDENTITY() do { g_wp_w[0] = g_wp_w[1] = g_wp_w[2] = 1; \
	g_wp_o[0] = g_wp_o[1] = g_wp_o[2] = 0; \
	g_wp_den[0] = g_wp_den[1] = g_wp_den[2] = 0; } while (0)
static int g_wp_w[3] = { 1, 1, 1 };
static int g_wp_o[3] = { 0, 0, 0 };
static int g_wp_den[3] = { 0, 0, 0 };

void H264_MC_Luma(
	uint8* dst, int dst_stride,
	const uint8* ref, int ref_stride,
	int width, int height,
	int src_x, int src_y,
	int mv_x, int mv_y,
	int pic_width, int pic_height
) {
	if (!dst || !ref || width <= 0 || height <= 0) return;

	int x_frac = (mv_x % 4 + 4) % 4;
	int y_frac = (mv_y % 4 + 4) % 4;
	int full_x = src_x + ((mv_x - x_frac) >> 2);
	int full_y = src_y + ((mv_y - y_frac) >> 2);

	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			int X = full_x + x;
			int Y = full_y + y;

			if (x_frac == 0 && y_frac == 0) {
				dst[y * dst_stride + x] = GetPixelClamped(ref, ref_stride, X, Y, pic_width, pic_height);
			} else if (x_frac == 2 && y_frac == 0) {
				int b = Filter6TapH(ref, ref_stride, X, Y, pic_width, pic_height);
				dst[y * dst_stride + x] = Clip8((b + 16) >> 5);
			} else if (x_frac == 0 && y_frac == 2) {
				int h = Filter6TapV(ref, ref_stride, X, Y, pic_width, pic_height);
				dst[y * dst_stride + x] = Clip8((h + 16) >> 5);
			} else if (x_frac == 2 && y_frac == 2) {
				int cc_m2 = Filter6TapH(ref, ref_stride, X, Y - 2, pic_width, pic_height);
				int cc_m1 = Filter6TapH(ref, ref_stride, X, Y - 1, pic_width, pic_height);
				int cc_0  = Filter6TapH(ref, ref_stride, X, Y,     pic_width, pic_height);
				int cc_1  = Filter6TapH(ref, ref_stride, X, Y + 1, pic_width, pic_height);
				int cc_2  = Filter6TapH(ref, ref_stride, X, Y + 2, pic_width, pic_height);
				int cc_3  = Filter6TapH(ref, ref_stride, X, Y + 3, pic_width, pic_height);
				int j = cc_m2 - 5 * cc_m1 + 20 * cc_0 + 20 * cc_1 - 5 * cc_2 + cc_3;
				dst[y * dst_stride + x] = Clip8((j + 512) >> 10);
			} else {
				// Quarter-pel combinations
				int S_00 = GetPixelClamped(ref, ref_stride, X, Y, pic_width, pic_height);
				int b_00 = Clip8((Filter6TapH(ref, ref_stride, X, Y, pic_width, pic_height) + 16) >> 5);
				int h_00 = Clip8((Filter6TapV(ref, ref_stride, X, Y, pic_width, pic_height) + 16) >> 5);

				if (x_frac == 1 && y_frac == 0) {
					dst[y * dst_stride + x] = (uint8)((S_00 + b_00 + 1) >> 1);
				} else if (x_frac == 3 && y_frac == 0) {
					int S_10 = GetPixelClamped(ref, ref_stride, X + 1, Y, pic_width, pic_height);
					dst[y * dst_stride + x] = (uint8)((S_10 + b_00 + 1) >> 1);
				} else if (x_frac == 0 && y_frac == 1) {
					dst[y * dst_stride + x] = (uint8)((S_00 + h_00 + 1) >> 1);
				} else if (x_frac == 0 && y_frac == 3) {
					int S_01 = GetPixelClamped(ref, ref_stride, X, Y + 1, pic_width, pic_height);
					dst[y * dst_stride + x] = (uint8)((S_01 + h_00 + 1) >> 1);
				} else {
					// 2D quarter-pel
					int cc_m2 = Filter6TapH(ref, ref_stride, X, Y - 2, pic_width, pic_height);
					int cc_m1 = Filter6TapH(ref, ref_stride, X, Y - 1, pic_width, pic_height);
					int cc_0  = Filter6TapH(ref, ref_stride, X, Y,     pic_width, pic_height);
					int cc_1  = Filter6TapH(ref, ref_stride, X, Y + 1, pic_width, pic_height);
					int cc_2  = Filter6TapH(ref, ref_stride, X, Y + 2, pic_width, pic_height);
					int cc_3  = Filter6TapH(ref, ref_stride, X, Y + 3, pic_width, pic_height);
					int j_00 = Clip8((cc_m2 - 5 * cc_m1 + 20 * cc_0 + 20 * cc_1 - 5 * cc_2 + cc_3 + 512) >> 10);

					// The remaining quarter sample positions average the two half sample values that
					// lie next to them.  b is the horizontal half sample at ( X, Y ), h the vertical
					// one at ( X, Y ), m the vertical one at ( X + 1, Y ) and s the horizontal one at
					// ( X, Y + 1 ).  H.264 8.4.2.2.1 derives s1 "the same way as b1", i.e. with the
					// HORIZONTAL 6-tap filter; filtering s vertically put q/p/r on the wrong sample
					// position.  1/4 sample geometry: a=(G+b)/2  c=(H+b)/2  d=(G+h)/2  n=(M+h)/2,
					// f=(b+j)/2  i=(h+j)/2  k=(j+m)/2  q=(j+s)/2, e=(b+h)/2  g=(b+m)/2  p=(h+s)/2
					// r=(m+s)/2.
					if (x_frac == 2 && y_frac == 1) {
						dst[y * dst_stride + x] = (uint8)((b_00 + j_00 + 1) >> 1);      // f = ( b + j + 1 ) >> 1
					} else if (x_frac == 1 && y_frac == 2) {
						dst[y * dst_stride + x] = (uint8)((h_00 + j_00 + 1) >> 1);      // i = ( h + j + 1 ) >> 1
					} else if (x_frac == 3 && y_frac == 1) {
						int m_10 = Clip8((Filter6TapV(ref, ref_stride, X + 1, Y, pic_width, pic_height) + 16) >> 5);
						dst[y * dst_stride + x] = (uint8)((b_00 + m_10 + 1) >> 1);      // g = ( b + m + 1 ) >> 1
					} else if (x_frac == 3 && y_frac == 2) {
						int m_10 = Clip8((Filter6TapV(ref, ref_stride, X + 1, Y, pic_width, pic_height) + 16) >> 5);
						dst[y * dst_stride + x] = (uint8)((m_10 + j_00 + 1) >> 1);      // k = ( j + m + 1 ) >> 1
					} else if (x_frac == 2 && y_frac == 3) {
						int s_01 = Clip8((Filter6TapH(ref, ref_stride, X, Y + 1, pic_width, pic_height) + 16) >> 5);
						dst[y * dst_stride + x] = (uint8)((s_01 + j_00 + 1) >> 1);      // q = ( j + s + 1 ) >> 1
					} else if (x_frac == 1 && y_frac == 1) {
						dst[y * dst_stride + x] = (uint8)((b_00 + h_00 + 1) >> 1);      // e = ( b + h + 1 ) >> 1
					} else if (x_frac == 1 && y_frac == 3) {
						int s_01 = Clip8((Filter6TapH(ref, ref_stride, X, Y + 1, pic_width, pic_height) + 16) >> 5);
						dst[y * dst_stride + x] = (uint8)((h_00 + s_01 + 1) >> 1);      // p = ( h + s + 1 ) >> 1
					} else if (x_frac == 3 && y_frac == 3) {
						int m_10 = Clip8((Filter6TapV(ref, ref_stride, X + 1, Y, pic_width, pic_height) + 16) >> 5);
						int s_01 = Clip8((Filter6TapH(ref, ref_stride, X, Y + 1, pic_width, pic_height) + 16) >> 5);
						dst[y * dst_stride + x] = (uint8)((m_10 + s_01 + 1) >> 1);      // r = ( m + s + 1 ) >> 1
					}
				}
			}
		}
	}

	// Weighted sample prediction (H.264 8.4.2.2.2).  The block was built from one partition's
	// reference, so the weight and offset that partition's reference index selected apply to every
	// sample of it.  Only luma is scaled here; the chroma planes are weighted where they are
	// reconstructed, because the two chroma components can carry different weights.
	if (g_wp_den[0] != 0 || g_wp_w[0] != 1 || g_wp_o[0] != 0) {
		const int rnd = (g_wp_den[0] > 0) ? (1 << (g_wp_den[0] - 1)) : 0;
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				dst[y * dst_stride + x] = Clip8(((g_wp_w[0] * dst[y * dst_stride + x] + rnd) >> g_wp_den[0]) + g_wp_o[0]);
			}
		}
	}
}

void H264_MC_Chroma(
	uint8* dst, int dst_stride,
	const uint8* ref, int ref_stride,
	int width, int height,
	int src_x, int src_y,
	int mv_x, int mv_y,
	int pic_width, int pic_height,
	int wp_plane
) {
	if (!dst || !ref || width <= 0 || height <= 0) return;

	int dx = (mv_x % 8 + 8) % 8;
	int dy = (mv_y % 8 + 8) % 8;
	int full_cx = src_x + ((mv_x - dx) >> 3);
	int full_cy = src_y + ((mv_y - dy) >> 3);

	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			int A = GetPixelClamped(ref, ref_stride, full_cx + x,     full_cy + y,     pic_width, pic_height);
			int B = GetPixelClamped(ref, ref_stride, full_cx + x + 1, full_cy + y,     pic_width, pic_height);
			int C = GetPixelClamped(ref, ref_stride, full_cx + x,     full_cy + y + 1, pic_width, pic_height);
			int D = GetPixelClamped(ref, ref_stride, full_cx + x + 1, full_cy + y + 1, pic_width, pic_height);
			int val = ((8 - dx) * (8 - dy) * A + dx * (8 - dy) * B + (8 - dx) * dy * C + dx * dy * D + 32) >> 6;
			dst[y * dst_stride + x] = (uint8)val;
		}
	}

	// Weighted sample prediction of this chroma component (H.264 8.4.2.2.2).  Cb and Cr carry
	// their own weight and offset, so only the component named by wp_plane is scaled; the luma
	// plane applies its own weights inside H264_MC_Luma.
	if (wp_plane > 0 && (g_wp_den[wp_plane] != 0 || g_wp_w[wp_plane] != 1 || g_wp_o[wp_plane] != 0)) {
		const int rnd = (g_wp_den[wp_plane] > 0) ? (1 << (g_wp_den[wp_plane] - 1)) : 0;
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				dst[y * dst_stride + x] = Clip8(((g_wp_w[wp_plane] * dst[y * dst_stride + x] + rnd) >> g_wp_den[wp_plane]) + g_wp_o[wp_plane]);
			}
		}
	}
}

static inline H264MotionVector MedianMVP(
	H264MotionVector mvA, bool hasA,
	H264MotionVector mvB, bool hasB,
	H264MotionVector mvC, bool hasC,
	H264MotionVector mvD, bool hasD
) {
	if (hasA && !hasB && !hasC) return mvA;
	if (!hasA && hasB && !hasC) return mvB;
	if (!hasA && !hasB && hasC) return mvC;
	if (!hasA && !hasB && !hasC) {
		H264MotionVector zero = { 0, 0 };
		return zero;
	}
	// Only now may D stand in for an unavailable C: the "exactly one available" test above is
	// defined on the availability of A, B and C themselves (H.264 8.4.1.3.1).  Substituting D
	// before that test turns "only the above-left neighbour exists" into "only C exists" and
	// returns D's motion vector where the median of {0, 0, mvD} - i.e. zero - is required, which
	// shifts the predictor of every partition at the top-left corner of a macroblock.
	if (!hasC) {
		mvC = mvD;
		hasC = hasD;
	}
	if (!hasA) mvA = { 0, 0 };
	if (!hasB) mvB = { 0, 0 };
	if (!hasC) mvC = { 0, 0 };

	H264MotionVector res;
	res.x = mvA.x + mvB.x + mvC.x -
			((mvA.x < mvB.x ? mvA.x : mvB.x) < mvC.x ? (mvA.x < mvB.x ? mvA.x : mvB.x) : mvC.x) -
			((mvA.x > mvB.x ? mvA.x : mvB.x) > mvC.x ? (mvA.x > mvB.x ? mvA.x : mvB.x) : mvC.x);
	res.y = mvA.y + mvB.y + mvC.y -
			((mvA.y < mvB.y ? mvA.y : mvB.y) < mvC.y ? (mvA.y < mvB.y ? mvA.y : mvB.y) : mvC.y) -
			((mvA.y > mvB.y ? mvA.y : mvB.y) > mvC.y ? (mvA.y > mvB.y ? mvA.y : mvB.y) : mvC.y);
	return res;
}

// Neighbour of a partition: its position, its reference index (-1 when the partition is
// unavailable: outside the picture, intra coded, or not using the list at all - the tables mark
// that last case with 255) and its motion vector.
struct H264MvpNeighbour {
	H264MotionVector mv;
	int ref;
};

static inline void H264_LoadNeighbour(const H264MotionVector* mv_table, const uint8* ref_table,
									  int idx, int cur_ref, bool single_list,
									  H264MvpNeighbour* out) {
	out->mv = H264MotionVector{ 0, 0 };
	out->ref = -1;
	if (idx < 0) return;
	if (single_list) {
		// No reference table: the slice activates a single reference picture, so every
		// neighbouring partition is available and uses it.
		out->mv = mv_table[idx];
		out->ref = cur_ref;
		return;
	}
	// H.264 8.4.1.3.2: a partition that does not use the list contributes a zero motion vector
	// and reference index -1; one that uses a *different* reference still contributes its real
	// motion vector (the reference index is only compared later, in 8.4.1.3.1).
	if (ref_table[idx] == 255) return;
	// H.264 8.4.1.3.2: a neighbouring partition that uses the list but a *different* reference
	// index is still available and contributes its real motion vector and reference index.  A
	// mismatching reference index only disqualifies that neighbour from the directional predictor
	// and from the "exactly one neighbour matches" rule of 8.4.1.3.1; the median of (8-165) still
	// takes the real vector - JM's MVPRED_MEDIAN reads the neighbour's mv whenever the partition
	// is available, whatever its reference index is.  Reporting a zero vector here made the
	// predictor collapse to 0 whenever the only available neighbour used another reference, which
	// left whole partitions with the coded difference alone as their motion vector.
	out->mv = mv_table[idx];
	out->ref = (int)ref_table[idx];
}

// H.264 8.4.1.3 derivation of the luma motion vector predictor.
//
// The neighbours are A = (xN - 1, yN), B = (xN, yN - 1), C = (xN + w4, yN - 1) and
// D = (xN - 1, yN - 1); an unavailable C is replaced by D (8.4.1.3.2).  The predictor is
//  - the directional predictor of 8.4.1.3 when the partition is 16x8 or 8x16 and the neighbour
//    the rule names uses the same reference index,
//  - the motion vector of the single neighbour whose reference index matches when exactly one
//    does,
//  - otherwise the component-wise median of A, B and C.
// A null table means the slice has a single reference, so every neighbour qualifies.
static H264MotionVector H264_GetMVPForPartition(
	const H264MotionVector* mv_table,
	uint32 mb_idx,
	uint32 mb_width,
	uint32 mb_x,
	uint32 mb_y,
	int bx4,
	int by4,
	int bw4,
	int bh4,
	const uint8* ref_table = nullptr,
	int cur_ref = 0
) {
	const bool single_list = (ref_table == nullptr);
	const uint32 cur_base = mb_idx * 16;
	H264MvpNeighbour nA, nB, nC, nD;

	// A: the 4x4 block to the left of the partition.
	int ia = (bx4 > 0) ? (int)cur_base + by4 * 4 + bx4 - 1
					   : ((mb_x > 0) ? (int)(mb_idx - 1) * 16 + by4 * 4 + 3 : -1);
	// B: the 4x4 block above it.
	int ib = (by4 > 0) ? (int)cur_base + (by4 - 1) * 4 + bx4
					   : ((mb_y > 0) ? (int)(mb_idx - mb_width) * 16 + 12 + bx4 : -1);
	// C: the block above the partition's top-right corner (8.4.1.3.2 step 2, which folds xN + w4).
	// When that position is not available the predictor falls back to the block above-left (D).
	// A position whose y4 - 1 stays inside the current macroblock row (the 16x8 bottom half, or
	// the lower sub-partitions of a B_8x8) lies in the macroblock to the *right*, which raster
	// order has not decoded yet - it is not available there, so C must fall back to D.  Only
	// the positions one row up (y4 == 0) can reach a decoded macroblock, and those are the ones
	// handled here.
	int cx4 = bx4 + bw4;
	int cy4 = by4 - 1;
	int ic = -1;
	// The current macroblock's own 4x4 blocks (2, 0) and (2, 2) - the top-left block of
	// sub-macroblocks 1 and 3 - hold no motion vector yet and stay unavailable until the decode
	// reaches the sub-macroblock they belong to.  Until then the diagonal of a partition that
	// reaches into them does not exist and C falls back to D.  Reading them as available let C
	// enter the median with a zero vector and bit 0..1 of match_count, which changed the
	// predicted vector of exactly those partitions - measured on display frame 30, mb 297
	// (P_8x8, subtype 8x4 in sub-macroblock 2): C was (0,0) instead of D (-7,2), so the predictor
	// was (-7,0) instead of (-7,2) and the reconstructed block came out up to 95 levels away from
	// the reference decode.
	int cur_sub = (by4 >> 1) * 2 + (bx4 >> 1);
	bool c_open = true;
	if (cy4 >= 0 && cx4 < 4 && (cx4 & 1) == 0 && (cy4 & 1) == 0)
		c_open = (((cy4 >> 1) * 2 + (cx4 >> 1)) <= cur_sub);
	if (c_open) {
		if (cy4 >= 0 && cx4 < 4) {
			ic = (int)cur_base + cy4 * 4 + cx4;
		} else if (cy4 < 0 && cx4 < 4 && mb_y > 0) {
			ic = (int)(mb_idx - mb_width) * 16 + 12 + cx4;
		} else if (cy4 < 0 && cx4 >= 4 && mb_y > 0 && mb_x + 1 < mb_width) {
			ic = (int)(mb_idx - mb_width + 1) * 16 + 12;
		}
	}
	// D: the block above-left of the partition, the stand-in for an unavailable C.  (x-1, y-1) is
	// only in the macroblock above-left when the partition sits at the macroblock's top-left corner
	// as well; with bx4 == 0 but by4 > 0 it is still in the PREVIOUS macroblock of the same row.
	// Sending it to the macroblock above-left there made the predictor read an unrelated motion
	// vector: measured on display frame 2, mb 349's bottom 16x8 half took its predictor from mb
	// 294's block (3,1) - mv (-11,0) - where the correct neighbour (mb 348's block (3,1)) does not
	// use list 0 at all, so the predictor should have been the median of three unavailable
	// neighbours, i.e. zero.  That single wrong vector then propagated through the whole moving
	// object of the picture.
	int id = -1;
	if (bx4 > 0 && by4 > 0) {
		id = (int)cur_base + (by4 - 1) * 4 + bx4 - 1;
	} else if (bx4 == 0 && by4 > 0 && mb_x > 0) {
		id = (int)(mb_idx - 1) * 16 + (by4 - 1) * 4 + 3;
	} else if (bx4 > 0 && by4 == 0 && mb_y > 0) {
		id = (int)(mb_idx - mb_width) * 16 + 12 + bx4 - 1;
	} else if (bx4 == 0 && by4 == 0 && mb_x > 0 && mb_y > 0) {
		id = (int)(mb_idx - mb_width - 1) * 16 + 15;
	}

	H264_LoadNeighbour(mv_table, ref_table, ia, cur_ref, single_list, &nA);
	H264_LoadNeighbour(mv_table, ref_table, ib, cur_ref, single_list, &nB);
	H264_LoadNeighbour(mv_table, ref_table, ic, cur_ref, single_list, &nC);
	H264_LoadNeighbour(mv_table, ref_table, id, cur_ref, single_list, &nD);

	// H.264 8.4.1.3.2 step 3: an unavailable C takes D's place entirely.
	// H.264 8.4.1.3.2 step 3: a C partition that does not exist is replaced by D.  A C that
	// exists but does not use the list is NOT replaced - it stays an unavailable neighbour
	// with a zero vector and reference index -1 (8.4.1.3.2).
	bool availA = (ia >= 0), availB = (ib >= 0), availC = (ic >= 0);
	if (!availC) { nC = nD; availC = (id >= 0); }

	// H.264 8.4.1.3.1 (8-160)..(8-163): when B and C are not available and A is, the two
	// take A's value.
	if (!availB && !availC && availA) { nB = nA; nC = nA; }

	H264MotionVector zero = { 0, 0 };

	// H.264 8.4.1.3 directional prediction for the elongated partitions.
	if (bw4 == 4 && bh4 == 2) { // 16x8: top half from B, bottom half from A
		int part = (by4 >= 2) ? 1 : 0;
		if (part == 0 && nB.ref == cur_ref) return nB.mv;
		if (part == 1 && nA.ref == cur_ref) return nA.mv;
	} else if (bw4 == 2 && bh4 == 4) { // 8x16: left half from A, right half from C
		int part = (bx4 >= 2) ? 1 : 0;
		if (part == 0 && nA.ref == cur_ref) return nA.mv;
		if (part == 1 && nC.ref == cur_ref) return nC.mv;
	}

	// H.264 8.4.1.3.1: exactly one neighbouring partition with the same reference index.
	int matches = ((nA.ref == cur_ref) ? 1 : 0) + ((nB.ref == cur_ref) ? 1 : 0) +
				  ((nC.ref == cur_ref) ? 1 : 0);
	if (matches == 1) {
		if (nA.ref == cur_ref) return nA.mv;
		if (nB.ref == cur_ref) return nB.mv;
		return nC.mv;
	}

	if (nA.ref < 0) nA.mv = zero;
	if (nB.ref < 0) nB.mv = zero;
	if (nC.ref < 0) nC.mv = zero;
	return MedianMVP(nA.mv, true, nB.mv, true, nC.mv, true, zero, false);
}

bool H264_DecodeSlice_P(
	const byte* rbsp_data,
	size_t rbsp_len,
	const H264SPS* sps,
	const H264PPS* pps,
	uint8 nal_unit_type,
	uint8 nal_ref_idc,
	const uint8* ref_y,
	const uint8* ref_u,
	const uint8* ref_v,
	int ref_y_stride,
	int ref_uv_stride,
	uint8* dst_y,
	uint8* dst_u,
	uint8* dst_v,
	int dst_y_stride,
	int dst_uv_stride
) {
	if (!rbsp_data || rbsp_len == 0 || !sps || !pps || !dst_y || !dst_u || !dst_v || !ref_y || !ref_u || !ref_v) return false;

	H264BitReader reader;
	H264_InitBitReader(&reader, rbsp_data, rbsp_len);

	H264SliceHeader sh;
	if (!H264_ParseSliceHeader(&reader, sps, pps, nal_unit_type, nal_ref_idc, &sh)) {
		return false;
	}

	uint32 mb_width = sps->mb_width;
	uint32 mb_height = sps->mb_height;
	uint32 num_mbs = mb_width * mb_height;

	if (sh.first_mb_in_slice >= num_mbs) return false;

	size_t nnz_size = (size_t)num_mbs * 24;
	size_t mv_size = (size_t)num_mbs * 16 * sizeof(H264MotionVector);
	uint8* nnz_table = (uint8*)malloc(nnz_size);
	H264MotionVector* mv_table = (H264MotionVector*)malloc(mv_size);
	if (!nnz_table || !mv_table) {
		if (nnz_table) free(nnz_table);
		if (mv_table) free(mv_table);
		return false;
	}
	MemSet(nnz_table, 0, nnz_size);
	MemSet(mv_table, 0, mv_size);

	int32 current_qp = sh.slice_qp;
	uint32 mb_idx = sh.first_mb_in_slice;

	int pic_w = (int)sps->width;
	int pic_h = (int)sps->height;
	int pic_w_c = pic_w / 2;
	int pic_h_c = pic_h / 2;

	while (mb_idx < num_mbs && H264_HasData(&reader)) {
		uint32 mb_skip_run = H264_ReadUE(&reader);

		// Process skipped macroblocks
		while (mb_skip_run > 0 && mb_idx < num_mbs) {
			uint32 mb_x = mb_idx % mb_width;
			uint32 mb_y = mb_idx / mb_width;

			int src_x = (int)mb_x * 16;
			int src_y = (int)mb_y * 16;
			uint8* mb_y_dst = dst_y + src_y * dst_y_stride + src_x;
			uint8* mb_u_dst = dst_u + (src_y / 2) * dst_uv_stride + (src_x / 2);
			uint8* mb_v_dst = dst_v + (src_y / 2) * dst_uv_stride + (src_x / 2);

			H264MotionVector mvA = { 0, 0 }, mvB = { 0, 0 }, mvC = { 0, 0 }, mvD = { 0, 0 };
			bool hasA = (mb_x > 0);
			bool hasB = (mb_y > 0);
			bool hasC = (mb_y > 0 && mb_x + 1 < mb_width);
			bool hasD = (mb_y > 0 && mb_x > 0);

			if (hasA) mvA = mv_table[(mb_idx - 1) * 16 + 5];
			if (hasB) mvB = mv_table[(mb_idx - mb_width) * 16 + 10];
			if (hasC) mvC = mv_table[(mb_idx - mb_width + 1) * 16 + 10];
			if (hasD) mvD = mv_table[(mb_idx - mb_width - 1) * 16 + 15];

			H264MotionVector skip_mv = { 0, 0 };
			if (hasA && hasB && (mvA.x != 0 || mvA.y != 0) && (mvB.x != 0 || mvB.y != 0)) {
				skip_mv = MedianMVP(mvA, hasA, mvB, hasB, mvC, hasC, mvD, hasD);
			}

			for (int k = 0; k < 16; ++k) mv_table[mb_idx * 16 + k] = skip_mv;
			for (int k = 0; k < 24; ++k) nnz_table[mb_idx * 24 + k] = 0;

			H264_MC_Luma(mb_y_dst, dst_y_stride, ref_y, ref_y_stride, 16, 16,
						 src_x, src_y, skip_mv.x, skip_mv.y, pic_w, pic_h);
			H264_MC_Chroma(mb_u_dst, dst_uv_stride, ref_u, ref_uv_stride, 8, 8,
						   src_x / 2, src_y / 2, skip_mv.x, skip_mv.y, pic_w_c, pic_h_c);
			H264_MC_Chroma(mb_v_dst, dst_uv_stride, ref_v, ref_uv_stride, 8, 8,
						   src_x / 2, src_y / 2, skip_mv.x, skip_mv.y, pic_w_c, pic_h_c);

			mb_skip_run--;
			mb_idx++;
		}

		if (mb_idx >= num_mbs || !H264_HasData(&reader)) break;

		uint32 mb_x = mb_idx % mb_width;
		uint32 mb_y = mb_idx / mb_width;

		int src_x = (int)mb_x * 16;
		int src_y = (int)mb_y * 16;
		uint8* mb_y_dst = dst_y + src_y * dst_y_stride + src_x;
		uint8* mb_u_dst = dst_u + (src_y / 2) * dst_uv_stride + (src_x / 2);
		uint8* mb_v_dst = dst_v + (src_y / 2) * dst_uv_stride + (src_x / 2);

		uint32 mb_type = H264_ReadUE(&reader);

		if (mb_type == 0) { // P_L0_16x16
			if (sh.num_ref_idx_l0_active_minus1 > 0) {
				H264_ReadTE(&reader, sh.num_ref_idx_l0_active_minus1);
			}

			int32 mvd_x = H264_ReadSE(&reader);
			int32 mvd_y = H264_ReadSE(&reader);

			H264MotionVector mvA = { 0, 0 }, mvB = { 0, 0 }, mvC = { 0, 0 }, mvD = { 0, 0 };
			bool hasA = (mb_x > 0);
			bool hasB = (mb_y > 0);
			bool hasC = (mb_y > 0 && mb_x + 1 < mb_width);
			bool hasD = (mb_y > 0 && mb_x > 0);

			if (hasA) mvA = mv_table[(mb_idx - 1) * 16 + 5];
			if (hasB) mvB = mv_table[(mb_idx - mb_width) * 16 + 10];
			if (hasC) mvC = mv_table[(mb_idx - mb_width + 1) * 16 + 10];
			if (hasD) mvD = mv_table[(mb_idx - mb_width - 1) * 16 + 15];

			H264MotionVector mvp = MedianMVP(mvA, hasA, mvB, hasB, mvC, hasC, mvD, hasD);
			H264MotionVector mv = { (int16)(mvp.x + mvd_x), (int16)(mvp.y + mvd_y) };

			for (int k = 0; k < 16; ++k) mv_table[mb_idx * 16 + k] = mv;

			uint8 pred_y[256], pred_u[64], pred_v[64];
			H264_MC_Luma(pred_y, 16, ref_y, ref_y_stride, 16, 16,
						 src_x, src_y, mv.x, mv.y, pic_w, pic_h);
			H264_MC_Chroma(pred_u, 8, ref_u, ref_uv_stride, 8, 8,
						   src_x / 2, src_y / 2, mv.x, mv.y, pic_w_c, pic_h_c);
			H264_MC_Chroma(pred_v, 8, ref_v, ref_uv_stride, 8, 8,
						   src_x / 2, src_y / 2, mv.x, mv.y, pic_w_c, pic_h_c);

			uint32 cbp_code = H264_ReadUE(&reader);
			uint8 cbp = (cbp_code < 48) ? golomb_to_inter_cbp[cbp_code] : 0;
			uint8 cbp_luma = cbp & 15;
			uint8 cbp_chroma = (cbp >> 4) & 3;

			if (cbp > 0) {
				int32 delta = H264_ReadSE(&reader);
				current_qp = (current_qp + delta + 52) % 52;
			}

			// Residuals
			for (int k = 0; k < 16; ++k) {
				int bx = ((k & 1) ? 1 : 0) + ((k & 4) ? 2 : 0);
				int by = ((k & 2) ? 1 : 0) + ((k & 8) ? 2 : 0);

				int16 coeffs[16] = { 0 };
				int32 total_coeff = 0;
				if ((cbp_luma & (1 << (k >> 2))) != 0) {
					H264_DecodeCAVLC_Block(&reader, 0, 16, coeffs, &total_coeff);
				}
				nnz_table[mb_idx * 24 + k] = (uint8)total_coeff;

				int32 residual[16] = { 0 };
				if (total_coeff > 0) {
					H264_Dequant4x4(coeffs, current_qp, false, residual);
					H264_IDCT4x4(residual, residual);
				}

				for (int y = 0; y < 4; ++y) {
					for (int x = 0; x < 4; ++x) {
						int recon = (int)pred_y[(by * 4 + y) * 16 + (bx * 4 + x)] + residual[y * 4 + x];
						mb_y_dst[(by * 4 + y) * dst_y_stride + (bx * 4 + x)] = Clip8(recon);
					}
				}
			}

			uint8 qp_c = H264_GetChromaQP(current_qp, pps->chroma_qp_index_offset);

			for (int plane = 0; plane < 2; ++plane) {
				uint8* ch_dst = (plane == 0) ? mb_u_dst : mb_v_dst;
				uint8* ch_pred = (plane == 0) ? pred_u : pred_v;
				int nnz_offset = 16 + plane * 4;

				int16 dc_coeffs[4] = { 0 };
				int32 total_dc = 0;
				stdsint scaled_dc[4] = { 0 };

				if (cbp_chroma > 0) {
					H264_DecodeCAVLC_Block(&reader, -1, 4, dc_coeffs, &total_dc);
					if (total_dc > 0) {
						stdsint dc_in[4] = { dc_coeffs[0], dc_coeffs[1], dc_coeffs[2], dc_coeffs[3] };
						Hadamard_H264_2x2(dc_in, qp_c, scaled_dc);
					}
				}

				int32 residuals_c[4][16];
				MemSet(residuals_c, 0, sizeof(residuals_c));

				for (int ck = 0; ck < 4; ++ck) {
					int16 ac_coeffs[16] = { 0 };
					int32 total_ac = 0;
					if (cbp_chroma == 2) {
						H264_DecodeCAVLC_Block(&reader, 0, 15, ac_coeffs, &total_ac);
					}
					nnz_table[mb_idx * 24 + nnz_offset + ck] = (uint8)(total_ac + (total_dc > 0 ? 1 : 0));

					stdsint scaled_block[16] = { 0 };
					if (total_ac > 0) {
						Dequant_H264_4x4(ac_coeffs, qp_c, true, scaled_block);
					}
					scaled_block[0] = scaled_dc[ck];
					stdsint res_s[16];
					IDCT_H264_4x4(scaled_block, res_s);
					for (int i = 0; i < 16; ++i) residuals_c[ck][i] = (int32)res_s[i];
				}

				for (int ck = 0; ck < 4; ++ck) {
					int cbx = (ck & 1) * 4;
					int cby = ((ck >> 1) & 1) * 4;
					for (int y = 0; y < 4; ++y) {
						for (int x = 0; x < 4; ++x) {
							int recon = (int)ch_pred[(cby + y) * 8 + (cbx + x)] + residuals_c[ck][y * 4 + x];
							ch_dst[(cby + y) * dst_uv_stride + (cbx + x)] = Clip8(recon);
						}
					}
				}
			}

		} else if (mb_type >= 5) {
			// Intra MB inside P-slice: pass to Intra_4x4 logic
			uint32 intra_chroma_mode = H264_ReadUE(&reader);
			uint32 cbp_code = H264_ReadUE(&reader);
			uint8 cbp = (cbp_code < 48) ? golomb_to_intra4x4_cbp[cbp_code] : 0;
			uint8 cbp_luma = cbp & 15;
			uint8 cbp_chroma = (cbp >> 4) & 3;

			if (cbp > 0) {
				int32 delta = H264_ReadSE(&reader);
				current_qp = (current_qp + delta + 52) % 52;
			}

			for (int k = 0; k < 16; ++k) {
				int bx = ((k & 1) ? 1 : 0) + ((k & 4) ? 2 : 0);
				int by = ((k & 2) ? 1 : 0) + ((k & 8) ? 2 : 0);

				int16 coeffs[16] = { 0 };
				int32 total_coeff = 0;
				if ((cbp_luma & (1 << (k >> 2))) != 0) {
					H264_DecodeCAVLC_Block(&reader, 0, 16, coeffs, &total_coeff);
				}
				nnz_table[mb_idx * 24 + k] = (uint8)total_coeff;

				int32 residual[16] = { 0 };
				if (total_coeff > 0) {
					H264_Dequant4x4(coeffs, current_qp, false, residual);
					H264_IDCT4x4(residual, residual);
				}

				uint8 top_buf[4], left_buf[4], tr_buf[4], tl_val = 128;
				bool blk_has_top = (by > 0) || (mb_y > 0);
				bool blk_has_left = (bx > 0) || (mb_x > 0);
				bool blk_has_tl = false, blk_has_tr = false;

				uint8* blk_dst = mb_y_dst + by * 4 * dst_y_stride + bx * 4;

				if (blk_has_top) {
					const uint8* top_ptr = blk_dst - dst_y_stride;
					top_buf[0] = top_ptr[0]; top_buf[1] = top_ptr[1];
					top_buf[2] = top_ptr[2]; top_buf[3] = top_ptr[3];
				}
				if (blk_has_left) {
					const uint8* left_ptr = blk_dst - 1;
					left_buf[0] = left_ptr[0 * dst_y_stride]; left_buf[1] = left_ptr[1 * dst_y_stride];
					left_buf[2] = left_ptr[2 * dst_y_stride]; left_buf[3] = left_ptr[3 * dst_y_stride];
				}
				if (blk_has_top && blk_has_left) {
					tl_val = *(blk_dst - dst_y_stride - 1);
					blk_has_tl = true;
				}

				uint8 pred_buf[16];
				H264_PredIntra4x4(pred_buf, 4, top_buf, left_buf, tl_val, tr_buf,
								  blk_has_top, blk_has_left, blk_has_tl, blk_has_tr, 2 /* DC */);

				for (int y = 0; y < 4; ++y) {
					for (int x = 0; x < 4; ++x) {
						int recon = (int)pred_buf[y * 4 + x] + residual[y * 4 + x];
						blk_dst[y * dst_y_stride + x] = Clip8(recon);
					}
				}
			}

			// Chroma DC & AC for Intra
			uint8 qp_c = H264_GetChromaQP(current_qp, pps->chroma_qp_index_offset);

			for (int plane = 0; plane < 2; ++plane) {
				uint8* ch_dst = (plane == 0) ? mb_u_dst : mb_v_dst;
				int nnz_offset = 16 + plane * 4;

				int16 dc_coeffs[4] = { 0 };
				int32 total_dc = 0;
				stdsint scaled_dc[4] = { 0 };

				if (cbp_chroma > 0) {
					H264_DecodeCAVLC_Block(&reader, -1, 4, dc_coeffs, &total_dc);
					if (total_dc > 0) {
						stdsint dc_in[4] = { dc_coeffs[0], dc_coeffs[1], dc_coeffs[2], dc_coeffs[3] };
						Hadamard_H264_2x2(dc_in, qp_c, scaled_dc);
					}
				}

				int32 residuals_c[4][16];
				MemSet(residuals_c, 0, sizeof(residuals_c));

				for (int ck = 0; ck < 4; ++ck) {
					int16 ac_coeffs[16] = { 0 };
					int32 total_ac = 0;
					if (cbp_chroma == 2) {
						H264_DecodeCAVLC_Block(&reader, 0, 15, ac_coeffs, &total_ac);
					}
					nnz_table[mb_idx * 24 + nnz_offset + ck] = (uint8)(total_ac + (total_dc > 0 ? 1 : 0));

					stdsint scaled_block[16] = { 0 };
					if (total_ac > 0) {
						Dequant_H264_4x4(ac_coeffs, qp_c, true, scaled_block);
					}
					scaled_block[0] = scaled_dc[ck];
					stdsint res_s[16];
					IDCT_H264_4x4(scaled_block, res_s);
					for (int i = 0; i < 16; ++i) residuals_c[ck][i] = (int32)res_s[i];
				}

				uint8 top_c[8] = { 128, 128, 128, 128, 128, 128, 128, 128 }, left_c[8] = { 128, 128, 128, 128, 128, 128, 128, 128 }, tl_c = 128;
				bool ch_has_top = (mb_y > 0);
				bool ch_has_left = (mb_x > 0);
				bool ch_has_tl = (mb_y > 0 && mb_x > 0);

				if (ch_has_top) {
					const uint8* tptr = ch_dst - dst_uv_stride;
					for (int i = 0; i < 8; ++i) top_c[i] = tptr[i];
				}
				if (ch_has_left) {
					const uint8* lptr = ch_dst - 1;
					for (int i = 0; i < 8; ++i) left_c[i] = lptr[i * dst_uv_stride];
				}
				if (ch_has_tl) {
					tl_c = *(ch_dst - dst_uv_stride - 1);
				}

				uint8 pred_c[64];
				H264_PredIntraChroma8x8(pred_c, 8, top_c, left_c, tl_c,
										ch_has_top, ch_has_left, ch_has_tl, (int)intra_chroma_mode);

				for (int ck = 0; ck < 4; ++ck) {
					int cbx = (ck & 1) * 4;
					int cby = ((ck >> 1) & 1) * 4;
					for (int y = 0; y < 4; ++y) {
						for (int x = 0; x < 4; ++x) {
							int recon = (int)pred_c[(cby + y) * 8 + (cbx + x)] + residuals_c[ck][y * 4 + x];
							ch_dst[(cby + y) * dst_uv_stride + (cbx + x)] = Clip8(recon);
						}
					}
				}
			}
		}

		mb_idx++;
	}

	free(mv_table);
	free(nnz_table);
	return true;
}



// Phase 4.5: High Profile CABAC (Context-based Adaptive Binary Arithmetic Coding) & 8x8 Transform Engine

// ITU-T H.264 Table 9-44: Specification of rangeTabLPS[pStateIdx][qCodIRangeIdx]
// Range interval subdividing for the Least Probable Symbol (LPS).
// State index pStateIdx (0..63) and quantized range qCodIRangeIdx = (codIRange >> 6) & 3.
// Derived from the 64-state Markov probability transition model: pLPS = 0.5 * (62/63)^state.

static const uint8 rangeTabLPS[64][4] = {
	{ 128, 176, 208, 240 }, { 128, 167, 197, 227 }, { 128, 158, 187, 216 }, { 123, 150, 178, 205 },
	{ 116, 142, 169, 195 }, { 111, 135, 160, 185 }, { 105, 128, 152, 175 }, { 100, 122, 144, 166 },
	{  95, 116, 137, 158 }, {  90, 110, 130, 150 }, {  85, 104, 123, 142 }, {  81,  99, 117, 135 },
	{  77,  94, 111, 128 }, {  73,  89, 105, 122 }, {  69,  85, 100, 116 }, {  66,  80,  95, 110 },
	{  62,  76,  90, 104 }, {  59,  72,  86,  99 }, {  56,  69,  81,  94 }, {  53,  65,  77,  89 },
	{  51,  62,  73,  85 }, {  48,  59,  69,  80 }, {  46,  56,  66,  76 }, {  43,  53,  63,  72 },
	{  41,  50,  59,  69 }, {  39,  48,  56,  65 }, {  37,  45,  54,  62 }, {  35,  43,  51,  59 },
	{  33,  41,  48,  56 }, {  32,  39,  46,  53 }, {  30,  37,  43,  50 }, {  29,  35,  41,  48 },
	{  27,  33,  39,  45 }, {  26,  31,  37,  43 }, {  24,  30,  35,  41 }, {  23,  28,  33,  39 },
	{  22,  27,  32,  37 }, {  21,  26,  30,  35 }, {  20,  24,  29,  33 }, {  19,  23,  27,  31 },
	{  18,  22,  26,  30 }, {  17,  21,  25,  28 }, {  16,  20,  23,  27 }, {  15,  19,  22,  25 },
	{  14,  18,  21,  24 }, {  14,  17,  20,  23 }, {  13,  16,  19,  22 }, {  12,  15,  18,  21 },
	{  12,  14,  17,  20 }, {  11,  14,  16,  19 }, {  11,  13,  15,  18 }, {  10,  12,  15,  17 },
	{  10,  12,  14,  16 }, {   9,  11,  13,  15 }, {   9,  11,  12,  14 }, {   8,  10,  12,  14 },
	{   8,   9,  11,  13 }, {   7,   9,  11,  12 }, {   7,   9,  10,  12 }, {   7,   8,  10,  11 },
	{   6,   8,   9,  11 }, {   6,   7,   9,  10 }, {   6,   7,   8,   9 }, {   2,   2,   2,   2 }
};

static const uint8 transIdxMPS[64] = {
	 1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16,
	17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
	33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
	49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 62, 63
};

static const uint8 transIdxLPS[64] = {
	 0,  0,  1,  2,  2,  4,  4,  5,  6,  7,  8,  9,  9, 11, 11, 12,
	13, 13, 15, 15, 16, 16, 18, 18, 19, 19, 21, 21, 22, 22, 23, 24,
	24, 25, 26, 26, 27, 27, 28, 29, 29, 30, 30, 30, 31, 32, 32, 33,
	33, 33, 34, 34, 35, 35, 35, 36, 36, 36, 37, 37, 37, 38, 38, 63
};

// ITU-T H.264 Clause 9.3.1.1: Context Variable Initialization Tables for I and SI Slices
// Mathematical initialization formula per context ctxIdx (0..1023):
//   1. preCtxState = Clip3(1, 126, (((m * Clip3(0, 51, SliceQP)) >> 4) + n))
//   2. if (preCtxState <= 63) {
//          pStateIdx = 63 - preCtxState;
//          valMPS = 0;
//      } else {
//          pStateIdx = preCtxState - 64;
//          valMPS = 1;
//      }
// Pairs format: { m, n } where m is slope factor, n is constant offset.
static const int8 cabac_context_init_I[1024][2] =
{
    /* 0 - 10 */
    { 20, -15 }, {  2, 54 },  {  3,  74 }, { 20, -15 },
    {  2,  54 }, {  3, 74 },  { -28,127 }, { -23, 104 },
    { -6,  53 }, { -1, 54 },  {  7,  51 },

    /* 11 - 23 unused for I */
    { 0, 0 },    { 0, 0 },    { 0, 0 },      { 0, 0 },
    { 0, 0 },    { 0, 0 },    { 0, 0 },      { 0, 0 },
    { 0, 0 },    { 0, 0 },    { 0, 0 },      { 0, 0 },
    { 0, 0 },

    /* 24- 39 */
    { 0, 0 },    { 0, 0 },    { 0, 0 },      { 0, 0 },
    { 0, 0 },    { 0, 0 },    { 0, 0 },      { 0, 0 },
    { 0, 0 },    { 0, 0 },    { 0, 0 },      { 0, 0 },
    { 0, 0 },    { 0, 0 },    { 0, 0 },      { 0, 0 },

    /* 40 - 53 */
    { 0, 0 },    { 0, 0 },    { 0, 0 },      { 0, 0 },
    { 0, 0 },    { 0, 0 },    { 0, 0 },      { 0, 0 },
    { 0, 0 },    { 0, 0 },    { 0, 0 },      { 0, 0 },
    { 0, 0 },    { 0, 0 },

    /* 54 - 59 */
    { 0, 0 },    { 0, 0 },    { 0, 0 },      { 0, 0 },
    { 0, 0 },    { 0, 0 },

    /* 60 - 69 */
    { 0, 41 },   { 0, 63 },   { 0, 63 },     { 0, 63 },
    { -9, 83 },  { 4, 86 },   { 0, 97 },     { -7, 72 },
    { 13, 41 },  { 3, 62 },

    /* 70 -> 87 */
    { 0, 11 },   { 1, 55 },   { 0, 69 },     { -17, 127 },
    { -13, 102 },{ 0, 82 },   { -7, 74 },    { -21, 107 },
    { -27, 127 },{ -31, 127 },{ -24, 127 },  { -18, 95 },
    { -27, 127 },{ -21, 114 },{ -30, 127 },  { -17, 123 },
    { -12, 115 },{ -16, 122 },

    /* 88 -> 104 */
    { -11, 115 },{ -12, 63 }, { -2, 68 },    { -15, 84 },
    { -13, 104 },{ -3, 70 },  { -8, 93 },    { -10, 90 },
    { -30, 127 },{ -1, 74 },  { -6, 97 },    { -7, 91 },
    { -20, 127 },{ -4, 56 },  { -5, 82 },    { -7, 76 },
    { -22, 125 },

    /* 105 -> 135 */
    { -7, 93 },  { -11, 87 }, { -3, 77 },    { -5, 71 },
    { -4, 63 },  { -4, 68 },  { -12, 84 },   { -7, 62 },
    { -7, 65 },  { 8, 61 },   { 5, 56 },     { -2, 66 },
    { 1, 64 },   { 0, 61 },   { -2, 78 },    { 1, 50 },
    { 7, 52 },   { 10, 35 },  { 0, 44 },     { 11, 38 },
    { 1, 45 },   { 0, 46 },   { 5, 44 },     { 31, 17 },
    { 1, 51 },   { 7, 50 },   { 28, 19 },    { 16, 33 },
    { 14, 62 },  { -13, 108 },{ -15, 100 },

    /* 136 -> 165 */
    { -13, 101 },{ -13, 91 }, { -12, 94 },   { -10, 88 },
    { -16, 84 }, { -10, 86 }, { -7, 83 },    { -13, 87 },
    { -19, 94 }, { 1, 70 },   { 0, 72 },     { -5, 74 },
    { 18, 59 },  { -8, 102 }, { -15, 100 },  { 0, 95 },
    { -4, 75 },  { 2, 72 },   { -11, 75 },   { -3, 71 },
    { 15, 46 },  { -13, 69 }, { 0, 62 },     { 0, 65 },
    { 21, 37 },  { -15, 72 }, { 9, 57 },     { 16, 54 },
    { 0, 62 },   { 12, 72 },

    /* 166 -> 196 */
    { 24, 0 },   { 15, 9 },   { 8, 25 },     { 13, 18 },
    { 15, 9 },   { 13, 19 },  { 10, 37 },    { 12, 18 },
    { 6, 29 },   { 20, 33 },  { 15, 30 },    { 4, 45 },
    { 1, 58 },   { 0, 62 },   { 7, 61 },     { 12, 38 },
    { 11, 45 },  { 15, 39 },  { 11, 42 },    { 13, 44 },
    { 16, 45 },  { 12, 41 },  { 10, 49 },    { 30, 34 },
    { 18, 42 },  { 10, 55 },  { 17, 51 },    { 17, 46 },
    { 0, 89 },   { 26, -19 }, { 22, -17 },

    /* 197 -> 226 */
    { 26, -17 }, { 30, -25 }, { 28, -20 },   { 33, -23 },
    { 37, -27 }, { 33, -23 }, { 40, -28 },   { 38, -17 },
    { 33, -11 }, { 40, -15 }, { 41, -6 },    { 38, 1 },
    { 41, 17 },  { 30, -6 },  { 27, 3 },     { 26, 22 },
    { 37, -16 }, { 35, -4 },  { 38, -8 },    { 38, -3 },
    { 37, 3 },   { 38, 5 },   { 42, 0 },     { 35, 16 },
    { 39, 22 },  { 14, 48 },  { 27, 37 },    { 21, 60 },
    { 12, 68 },  { 2, 97 },

    /* 227 -> 251 */
    { -3, 71 },  { -6, 42 },  { -5, 50 },    { -3, 54 },
    { -2, 62 },  { 0, 58 },   { 1, 63 },     { -2, 72 },
    { -1, 74 },  { -9, 91 },  { -5, 67 },    { -5, 27 },
    { -3, 39 },  { -2, 44 },  { 0, 46 },     { -16, 64 },
    { -8, 68 },  { -10, 78 }, { -6, 77 },    { -10, 86 },
    { -12, 92 }, { -15, 55 }, { -10, 60 },   { -6, 62 },
    { -4, 65 },

    /* 252 -> 275 */
    { -12, 73 }, { -8, 76 },  { -7, 80 },    { -9, 88 },
    { -17, 110 },{ -11, 97 }, { -20, 84 },   { -11, 79 },
    { -6, 73 },  { -4, 74 },  { -13, 86 },   { -13, 96 },
    { -11, 97 }, { -19, 117 },{ -8, 78 },    { -5, 33 },
    { -4, 48 },  { -2, 53 },  { -3, 62 },    { -13, 71 },
    { -10, 79 }, { -12, 86 }, { -13, 90 },   { -14, 97 },

    /* 276 a bit special (not used, bypass is used instead) */
    { 0, 0 },

    /* 277 -> 307 */
    { -6, 93 },  { -6, 84 },  { -8, 79 },    { 0, 66 },
    { -1, 71 },  { 0, 62 },   { -2, 60 },    { -2, 59 },
    { -5, 75 },  { -3, 62 },  { -4, 58 },    { -9, 66 },
    { -1, 79 },  { 0, 71 },   { 3, 68 },     { 10, 44 },
    { -7, 62 },  { 15, 36 },  { 14, 40 },    { 16, 27 },
    { 12, 29 },  { 1, 44 },   { 20, 36 },    { 18, 32 },
    { 5, 42 },   { 1, 48 },   { 10, 62 },    { 17, 46 },
    { 9, 64 },   { -12, 104 },{ -11, 97 },

    /* 308 -> 337 */
    { -16, 96 }, { -7, 88 },  { -8, 85 },    { -7, 85 },
    { -9, 85 },  { -13, 88 }, { 4, 66 },     { -3, 77 },
    { -3, 76 },  { -6, 76 },  { 10, 58 },    { -1, 76 },
    { -1, 83 },  { -7, 99 },  { -14, 95 },   { 2, 95 },
    { 0, 76 },   { -5, 74 },  { 0, 70 },     { -11, 75 },
    { 1, 68 },   { 0, 65 },   { -14, 73 },   { 3, 62 },
    { 4, 62 },   { -1, 68 },  { -13, 75 },   { 11, 55 },
    { 5, 64 },   { 12, 70 },

    /* 338 -> 368 */
    { 15, 6 },   { 6, 19 },   { 7, 16 },     { 12, 14 },
    { 18, 13 },  { 13, 11 },  { 13, 15 },    { 15, 16 },
    { 12, 23 },  { 13, 23 },  { 15, 20 },    { 14, 26 },
    { 14, 44 },  { 17, 40 },  { 17, 47 },    { 24, 17 },
    { 21, 21 },  { 25, 22 },  { 31, 27 },    { 22, 29 },
    { 19, 35 },  { 14, 50 },  { 10, 57 },    { 7, 63 },
    { -2, 77 },  { -4, 82 },  { -3, 94 },    { 9, 69 },
    { -12, 109 },{ 36, -35 }, { 36, -34 },

    /* 369 -> 398 */
    { 32, -26 }, { 37, -30 }, { 44, -32 },   { 34, -18 },
    { 34, -15 }, { 40, -15 }, { 33, -7 },    { 35, -5 },
    { 33, 0 },   { 38, 2 },   { 33, 13 },    { 23, 35 },
    { 13, 58 },  { 29, -3 },  { 26, 0 },     { 22, 30 },
    { 31, -7 },  { 35, -15 }, { 34, -3 },    { 34, 3 },
    { 36, -1 },  { 34, 5 },   { 32, 11 },    { 35, 5 },
    { 34, 12 },  { 39, 11 },  { 30, 29 },    { 34, 26 },
    { 29, 39 },  { 19, 66 },

    /* 399 -> 435 */
    {  31,  21 }, {  31,  31 }, {  25,  50 },
    { -17, 120 }, { -20, 112 }, { -18, 114 }, { -11,  85 },
    { -15,  92 }, { -14,  89 }, { -26,  71 }, { -15,  81 },
    { -14,  80 }, {   0,  68 }, { -14,  70 }, { -24,  56 },
    { -23,  68 }, { -24,  50 }, { -11,  74 }, {  23, -13 },
    {  26, -13 }, {  40, -15 }, {  49, -14 }, {  44,   3 },
    {  45,   6 }, {  44,  34 }, {  33,  54 }, {  19,  82 },
    {  -3,  75 }, {  -1,  23 }, {   1,  34 }, {   1,  43 },
    {   0,  54 }, {  -2,  55 }, {   0,  61 }, {   1,  64 },
    {   0,  68 }, {  -9,  92 },

    /* 436 -> 459 */
    { -14, 106 }, { -13,  97 }, { -15,  90 }, { -12,  90 },
    { -18,  88 }, { -10,  73 }, {  -9,  79 }, { -14,  86 },
    { -10,  73 }, { -10,  70 }, { -10,  69 }, {  -5,  66 },
    {  -9,  64 }, {  -5,  58 }, {   2,  59 }, {  21, -10 },
    {  24, -11 }, {  28,  -8 }, {  28,  -1 }, {  29,   3 },
    {  29,   9 }, {  35,  20 }, {  29,  36 }, {  14,  67 },

    /* 460 -> 1024 */
    { -17, 123 }, { -12, 115 }, { -16, 122 }, { -11, 115 },
    { -12,  63 }, {  -2,  68 }, { -15,  84 }, { -13, 104 },
    {  -3,  70 }, {  -8,  93 }, { -10,  90 }, { -30, 127 },
    { -17, 123 }, { -12, 115 }, { -16, 122 }, { -11, 115 },
    { -12,  63 }, {  -2,  68 }, { -15,  84 }, { -13, 104 },
    {  -3,  70 }, {  -8,  93 }, { -10,  90 }, { -30, 127 },
    {  -7,  93 }, { -11,  87 }, {  -3,  77 }, {  -5,  71 },
    {  -4,  63 }, {  -4,  68 }, { -12,  84 }, {  -7,  62 },
    {  -7,  65 }, {   8,  61 }, {   5,  56 }, {  -2,  66 },
    {   1,  64 }, {   0,  61 }, {  -2,  78 }, {   1,  50 },
    {   7,  52 }, {  10,  35 }, {   0,  44 }, {  11,  38 },
    {   1,  45 }, {   0,  46 }, {   5,  44 }, {  31,  17 },
    {   1,  51 }, {   7,  50 }, {  28,  19 }, {  16,  33 },
    {  14,  62 }, { -13, 108 }, { -15, 100 }, { -13, 101 },
    { -13,  91 }, { -12,  94 }, { -10,  88 }, { -16,  84 },
    { -10,  86 }, {  -7,  83 }, { -13,  87 }, { -19,  94 },
    {   1,  70 }, {   0,  72 }, {  -5,  74 }, {  18,  59 },
    {  -7,  93 }, { -11,  87 }, {  -3,  77 }, {  -5,  71 },
    {  -4,  63 }, {  -4,  68 }, { -12,  84 }, {  -7,  62 },
    {  -7,  65 }, {   8,  61 }, {   5,  56 }, {  -2,  66 },
    {   1,  64 }, {   0,  61 }, {  -2,  78 }, {   1,  50 },
    {   7,  52 }, {  10,  35 }, {   0,  44 }, {  11,  38 },
    {   1,  45 }, {   0,  46 }, {   5,  44 }, {  31,  17 },
    {   1,  51 }, {   7,  50 }, {  28,  19 }, {  16,  33 },
    {  14,  62 }, { -13, 108 }, { -15, 100 }, { -13, 101 },
    { -13,  91 }, { -12,  94 }, { -10,  88 }, { -16,  84 },
    { -10,  86 }, {  -7,  83 }, { -13,  87 }, { -19,  94 },
    {   1,  70 }, {   0,  72 }, {  -5,  74 }, {  18,  59 },
    {  24,   0 }, {  15,   9 }, {   8,  25 }, {  13,  18 },
    {  15,   9 }, {  13,  19 }, {  10,  37 }, {  12,  18 },
    {   6,  29 }, {  20,  33 }, {  15,  30 }, {   4,  45 },
    {   1,  58 }, {   0,  62 }, {   7,  61 }, {  12,  38 },
    {  11,  45 }, {  15,  39 }, {  11,  42 }, {  13,  44 },
    {  16,  45 }, {  12,  41 }, {  10,  49 }, {  30,  34 },
    {  18,  42 }, {  10,  55 }, {  17,  51 }, {  17,  46 },
    {   0,  89 }, {  26, -19 }, {  22, -17 }, {  26, -17 },
    {  30, -25 }, {  28, -20 }, {  33, -23 }, {  37, -27 },
    {  33, -23 }, {  40, -28 }, {  38, -17 }, {  33, -11 },
    {  40, -15 }, {  41,  -6 }, {  38,   1 }, {  41,  17 },
    {  24,   0 }, {  15,   9 }, {   8,  25 }, {  13,  18 },
    {  15,   9 }, {  13,  19 }, {  10,  37 }, {  12,  18 },
    {   6,  29 }, {  20,  33 }, {  15,  30 }, {   4,  45 },
    {   1,  58 }, {   0,  62 }, {   7,  61 }, {  12,  38 },
    {  11,  45 }, {  15,  39 }, {  11,  42 }, {  13,  44 },
    {  16,  45 }, {  12,  41 }, {  10,  49 }, {  30,  34 },
    {  18,  42 }, {  10,  55 }, {  17,  51 }, {  17,  46 },
    {   0,  89 }, {  26, -19 }, {  22, -17 }, {  26, -17 },
    {  30, -25 }, {  28, -20 }, {  33, -23 }, {  37, -27 },
    {  33, -23 }, {  40, -28 }, {  38, -17 }, {  33, -11 },
    {  40, -15 }, {  41,  -6 }, {  38,   1 }, {  41,  17 },
    { -17, 120 }, { -20, 112 }, { -18, 114 }, { -11,  85 },
    { -15,  92 }, { -14,  89 }, { -26,  71 }, { -15,  81 },
    { -14,  80 }, {   0,  68 }, { -14,  70 }, { -24,  56 },
    { -23,  68 }, { -24,  50 }, { -11,  74 }, { -14, 106 },
    { -13,  97 }, { -15,  90 }, { -12,  90 }, { -18,  88 },
    { -10,  73 }, {  -9,  79 }, { -14,  86 }, { -10,  73 },
    { -10,  70 }, { -10,  69 }, {  -5,  66 }, {  -9,  64 },
    {  -5,  58 }, {   2,  59 }, {  23, -13 }, {  26, -13 },
    {  40, -15 }, {  49, -14 }, {  44,   3 }, {  45,   6 },
    {  44,  34 }, {  33,  54 }, {  19,  82 }, {  21, -10 },
    {  24, -11 }, {  28,  -8 }, {  28,  -1 }, {  29,   3 },
    {  29,   9 }, {  35,  20 }, {  29,  36 }, {  14,  67 },
    {  -3,  75 }, {  -1,  23 }, {   1,  34 }, {   1,  43 },
    {   0,  54 }, {  -2,  55 }, {   0,  61 }, {   1,  64 },
    {   0,  68 }, {  -9,  92 }, { -17, 120 }, { -20, 112 },
    { -18, 114 }, { -11,  85 }, { -15,  92 }, { -14,  89 },
    { -26,  71 }, { -15,  81 }, { -14,  80 }, {   0,  68 },
    { -14,  70 }, { -24,  56 }, { -23,  68 }, { -24,  50 },
    { -11,  74 }, { -14, 106 }, { -13,  97 }, { -15,  90 },
    { -12,  90 }, { -18,  88 }, { -10,  73 }, {  -9,  79 },
    { -14,  86 }, { -10,  73 }, { -10,  70 }, { -10,  69 },
    {  -5,  66 }, {  -9,  64 }, {  -5,  58 }, {   2,  59 },
    {  23, -13 }, {  26, -13 }, {  40, -15 }, {  49, -14 },
    {  44,   3 }, {  45,   6 }, {  44,  34 }, {  33,  54 },
    {  19,  82 }, {  21, -10 }, {  24, -11 }, {  28,  -8 },
    {  28,  -1 }, {  29,   3 }, {  29,   9 }, {  35,  20 },
    {  29,  36 }, {  14,  67 }, {  -3,  75 }, {  -1,  23 },
    {   1,  34 }, {   1,  43 }, {   0,  54 }, {  -2,  55 },
    {   0,  61 }, {   1,  64 }, {   0,  68 }, {  -9,  92 },
    {  -6,  93 }, {  -6,  84 }, {  -8,  79 }, {   0,  66 },
    {  -1,  71 }, {   0,  62 }, {  -2,  60 }, {  -2,  59 },
    {  -5,  75 }, {  -3,  62 }, {  -4,  58 }, {  -9,  66 },
    {  -1,  79 }, {   0,  71 }, {   3,  68 }, {  10,  44 },
    {  -7,  62 }, {  15,  36 }, {  14,  40 }, {  16,  27 },
    {  12,  29 }, {   1,  44 }, {  20,  36 }, {  18,  32 },
    {   5,  42 }, {   1,  48 }, {  10,  62 }, {  17,  46 },
    {   9,  64 }, { -12, 104 }, { -11,  97 }, { -16,  96 },
    {  -7,  88 }, {  -8,  85 }, {  -7,  85 }, {  -9,  85 },
    { -13,  88 }, {   4,  66 }, {  -3,  77 }, {  -3,  76 },
    {  -6,  76 }, {  10,  58 }, {  -1,  76 }, {  -1,  83 },
    {  -6,  93 }, {  -6,  84 }, {  -8,  79 }, {   0,  66 },
    {  -1,  71 }, {   0,  62 }, {  -2,  60 }, {  -2,  59 },
    {  -5,  75 }, {  -3,  62 }, {  -4,  58 }, {  -9,  66 },
    {  -1,  79 }, {   0,  71 }, {   3,  68 }, {  10,  44 },
    {  -7,  62 }, {  15,  36 }, {  14,  40 }, {  16,  27 },
    {  12,  29 }, {   1,  44 }, {  20,  36 }, {  18,  32 },
    {   5,  42 }, {   1,  48 }, {  10,  62 }, {  17,  46 },
    {   9,  64 }, { -12, 104 }, { -11,  97 }, { -16,  96 },
    {  -7,  88 }, {  -8,  85 }, {  -7,  85 }, {  -9,  85 },
    { -13,  88 }, {   4,  66 }, {  -3,  77 }, {  -3,  76 },
    {  -6,  76 }, {  10,  58 }, {  -1,  76 }, {  -1,  83 },
    {  15,   6 }, {   6,  19 }, {   7,  16 }, {  12,  14 },
    {  18,  13 }, {  13,  11 }, {  13,  15 }, {  15,  16 },
    {  12,  23 }, {  13,  23 }, {  15,  20 }, {  14,  26 },
    {  14,  44 }, {  17,  40 }, {  17,  47 }, {  24,  17 },
    {  21,  21 }, {  25,  22 }, {  31,  27 }, {  22,  29 },
    {  19,  35 }, {  14,  50 }, {  10,  57 }, {   7,  63 },
    {  -2,  77 }, {  -4,  82 }, {  -3,  94 }, {   9,  69 },
    { -12, 109 }, {  36, -35 }, {  36, -34 }, {  32, -26 },
    {  37, -30 }, {  44, -32 }, {  34, -18 }, {  34, -15 },
    {  40, -15 }, {  33,  -7 }, {  35,  -5 }, {  33,   0 },
    {  38,   2 }, {  33,  13 }, {  23,  35 }, {  13,  58 },
    {  15,   6 }, {   6,  19 }, {   7,  16 }, {  12,  14 },
    {  18,  13 }, {  13,  11 }, {  13,  15 }, {  15,  16 },
    {  12,  23 }, {  13,  23 }, {  15,  20 }, {  14,  26 },
    {  14,  44 }, {  17,  40 }, {  17,  47 }, {  24,  17 },
    {  21,  21 }, {  25,  22 }, {  31,  27 }, {  22,  29 },
    {  19,  35 }, {  14,  50 }, {  10,  57 }, {   7,  63 },
    {  -2,  77 }, {  -4,  82 }, {  -3,  94 }, {   9,  69 },
    { -12, 109 }, {  36, -35 }, {  36, -34 }, {  32, -26 },
    {  37, -30 }, {  44, -32 }, {  34, -18 }, {  34, -15 },
    {  40, -15 }, {  33,  -7 }, {  35,  -5 }, {  33,   0 },
    {  38,   2 }, {  33,  13 }, {  23,  35 }, {  13,  58 },
    {  -3,  71 }, {  -6,  42 }, {  -5,  50 }, {  -3,  54 },
    {  -2,  62 }, {   0,  58 }, {   1,  63 }, {  -2,  72 },
    {  -1,  74 }, {  -9,  91 }, {  -5,  67 }, {  -5,  27 },
    {  -3,  39 }, {  -2,  44 }, {   0,  46 }, { -16,  64 },
    {  -8,  68 }, { -10,  78 }, {  -6,  77 }, { -10,  86 },
    { -12,  92 }, { -15,  55 }, { -10,  60 }, {  -6,  62 },
    {  -4,  65 }, { -12,  73 }, {  -8,  76 }, {  -7,  80 },
    {  -9,  88 }, { -17, 110 }, {  -3,  71 }, {  -6,  42 },
    {  -5,  50 }, {  -3,  54 }, {  -2,  62 }, {   0,  58 },
    {   1,  63 }, {  -2,  72 }, {  -1,  74 }, {  -9,  91 },
    {  -5,  67 }, {  -5,  27 }, {  -3,  39 }, {  -2,  44 },
    {   0,  46 }, { -16,  64 }, {  -8,  68 }, { -10,  78 },
    {  -6,  77 }, { -10,  86 }, { -12,  92 }, { -15,  55 },
    { -10,  60 }, {  -6,  62 }, {  -4,  65 }, { -12,  73 },
    {  -8,  76 }, {  -7,  80 }, {  -9,  88 }, { -17, 110 },
    {  -3,  70 }, {  -8,  93 }, { -10,  90 }, { -30, 127 },
    {  -3,  70 }, {  -8,  93 }, { -10,  90 }, { -30, 127 },
    {  -3,  70 }, {  -8,  93 }, { -10,  90 }, { -30, 127 }
};

// ITU-T H.264 Clause 9.3.1.1: Context Variable Initialization Tables for P, SP, and B Slices
// Mathematical initialization formula per context ctxIdx (0..1023) and cabac_init_idc (0..2):
//   1. preCtxState = Clip3(1, 126, (((m * Clip3(0, 51, SliceQP)) >> 4) + n))
//   2. if (preCtxState <= 63) {
//          pStateIdx = 63 - preCtxState;
//          valMPS = 0;
//      } else {
//          pStateIdx = preCtxState - 64;
//          valMPS = 1;
//      }
// Dimensions: [cabac_init_idc: 0..2][ctxIdx: 0..1023][pair: {m, n}]
static const int8 cabac_context_init_PB[3][1024][2] =
{
    /* i_cabac_init_idc == 0 */
    {
        /* 0 - 10 */
        {  20, -15 }, {   2,  54 }, {   3,  74 }, {  20, -15 },
        {   2,  54 }, {   3,  74 }, { -28, 127 }, { -23, 104 },
        {  -6,  53 }, {  -1,  54 }, {   7,  51 },

        /* 11 - 23 */
        {  23,  33 }, {  23,   2 }, {  21,   0 }, {   1,   9 },
        {   0,  49 }, { -37, 118 }, {   5,  57 }, { -13,  78 },
        { -11,  65 }, {   1,  62 }, {  12,  49 }, {  -4,  73 },
        {  17,  50 },

        /* 24 - 39 */
        {  18,  64 }, {   9,  43 }, {  29,   0 }, {  26,  67 },
        {  16,  90 }, {   9, 104 }, { -46, 127 }, { -20, 104 },
        {   1,  67 }, { -13,  78 }, { -11,  65 }, {   1,  62 },
        {  -6,  86 }, { -17,  95 }, {  -6,  61 }, {   9,  45 },

        /* 40 - 53 */
        {  -3,  69 }, {  -6,  81 }, { -11,  96 }, {   6,  55 },
        {   7,  67 }, {  -5,  86 }, {   2,  88 }, {   0,  58 },
        {  -3,  76 }, { -10,  94 }, {   5,  54 }, {   4,  69 },
        {  -3,  81 }, {   0,  88 },

        /* 54 - 59 */
        {  -7,  67 }, {  -5,  74 }, {  -4,  74 }, {  -5,  80 },
        {  -7,  72 }, {   1,  58 },

        /* 60 - 69 */
        {   0,  41 }, {   0,  63 }, {   0,  63 }, { 0, 63 },
        {  -9,  83 }, {   4,  86 }, {   0,  97 }, { -7, 72 },
        {  13,  41 }, {   3,  62 },

        /* 70 - 87 */
        {   0,  45 }, {  -4,  78 }, {  -3,  96 }, { -27,  126 },
        { -28,  98 }, { -25, 101 }, { -23,  67 }, { -28,  82 },
        { -20,  94 }, { -16,  83 }, { -22, 110 }, { -21,  91 },
        { -18, 102 }, { -13,  93 }, { -29, 127 }, {  -7,  92 },
        {  -5,  89 }, {  -7,  96 }, { -13, 108 }, {  -3,  46 },
        {  -1,  65 }, {  -1,  57 }, {  -9,  93 }, {  -3,  74 },
        {  -9,  92 }, {  -8,  87 }, { -23, 126 }, {   5,  54 },
        {   6,  60 }, {   6,  59 }, {   6,  69 }, {  -1,  48 },
        {   0,  68 }, {  -4,  69 }, {  -8,  88 },

        /* 105 -> 165 */
        {  -2,  85 }, {  -6,  78 }, {  -1,  75 }, {  -7,  77 },
        {   2,  54 }, {   5,  50 }, {  -3,  68 }, {   1,  50 },
        {   6,  42 }, {  -4,  81 }, {   1,  63 }, {  -4,  70 },
        {   0,  67 }, {   2,  57 }, {  -2,  76 }, {  11,  35 },
        {   4,  64 }, {   1,  61 }, {  11,  35 }, {  18,  25 },
        {  12,  24 }, {  13,  29 }, {  13,  36 }, { -10,  93 },
        {  -7,  73 }, {  -2,  73 }, {  13,  46 }, {   9,  49 },
        {  -7, 100 }, {   9,  53 }, {   2,  53 }, {   5,  53 },
        {  -2,  61 }, {   0,  56 }, {   0,  56 }, { -13,  63 },
        {  -5,  60 }, {  -1,  62 }, {   4,  57 }, {  -6,  69 },
        {   4,  57 }, {  14,  39 }, {   4,  51 }, {  13,  68 },
        {   3,  64 }, {   1,  61 }, {   9,  63 }, {   7,  50 },
        {  16,  39 }, {   5,  44 }, {   4,  52 }, {  11,  48 },
        {  -5,  60 }, {  -1,  59 }, {   0,  59 }, {  22,  33 },
        {   5,  44 }, {  14,  43 }, {  -1,  78 }, {   0,  60 },
        {   9,  69 },

        /* 166 - 226 */
        {  11,  28 }, {   2,  40 }, {   3,  44 }, {   0,  49 },
        {   0,  46 }, {   2,  44 }, {   2,  51 }, {   0,  47 },
        {   4,  39 }, {   2,  62 }, {   6,  46 }, {   0,  54 },
        {   3,  54 }, {   2,  58 }, {   4,  63 }, {   6,  51 },
        {   6,  57 }, {   7,  53 }, {   6,  52 }, {   6,  55 },
        {  11,  45 }, {  14,  36 }, {   8,  53 }, {  -1,  82 },
        {   7,  55 }, {  -3,  78 }, {  15,  46 }, {  22,  31 },
        {  -1,  84 }, {  25,   7 }, {  30,  -7 }, {  28,   3 },
        {  28,   4 }, {  32,   0 }, {  34,  -1 }, {  30,   6 },
        {  30,   6 }, {  32,   9 }, {  31,  19 }, {  26,  27 },
        {  26,  30 }, {  37,  20 }, {  28,  34 }, {  17,  70 },
        {   1,  67 }, {   5,  59 }, {   9,  67 }, {  16,  30 },
        {  18,  32 }, {  18,  35 }, {  22,  29 }, {  24,  31 },
        {  23,  38 }, {  18,  43 }, {  20,  41 }, {  11,  63 },
        {   9,  59 }, {   9,  64 }, {  -1,  94 }, {  -2,  89 },
        {  -9, 108 },

        /* 227 - 275 */
        {  -6,  76 }, {  -2,  44 }, {   0,  45 }, {   0,  52 },
        {  -3,  64 }, {  -2,  59 }, {  -4,  70 }, {  -4,  75 },
        {  -8,  82 }, { -17, 102 }, {  -9,  77 }, {   3,  24 },
        {   0,  42 }, {   0,  48 }, {   0,  55 }, {  -6,  59 },
        {  -7,  71 }, { -12,  83 }, { -11,  87 }, { -30, 119 },
        {   1,  58 }, {  -3,  29 }, {  -1,  36 }, {   1,  38 },
        {   2,  43 }, {  -6,  55 }, {   0,  58 }, {   0,  64 },
        {  -3,  74 }, { -10,  90 }, {   0,  70 }, {  -4,  29 },
        {   5,  31 }, {   7,  42 }, {   1,  59 }, {  -2,  58 },
        {  -3,  72 }, {  -3,  81 }, { -11,  97 }, {   0,  58 },
        {   8,   5 }, {  10,  14 }, {  14,  18 }, {  13,  27 },
        {   2,  40 }, {   0,  58 }, {  -3,  70 }, {  -6,  79 },
        {  -8,  85 },

        /* 276 a bit special (not used, bypass is used instead) */
        { 0, 0 },

        /* 277 - 337 */
        { -13, 106 }, { -16, 106 }, { -10,  87 }, { -21, 114 },
        { -18, 110 }, { -14,  98 }, { -22, 110 }, { -21, 106 },
        { -18, 103 }, { -21, 107 }, { -23, 108 }, { -26, 112 },
        { -10,  96 }, { -12,  95 }, {  -5,  91 }, {  -9,  93 },
        { -22,  94 }, {  -5,  86 }, {   9,  67 }, {  -4,  80 },
        { -10,  85 }, {  -1,  70 }, {   7,  60 }, {   9,  58 },
        {   5,  61 }, {  12,  50 }, {  15,  50 }, {  18,  49 },
        {  17,  54 }, {  10,  41 }, {   7,  46 }, {  -1,  51 },
        {   7,  49 }, {   8,  52 }, {   9,  41 }, {   6,  47 },
        {   2,  55 }, {  13,  41 }, {  10,  44 }, {   6,  50 },
        {   5,  53 }, {  13,  49 }, {   4,  63 }, {   6,  64 },
        {  -2,  69 }, {  -2,  59 }, {   6,  70 }, {  10,  44 },
        {   9,  31 }, {  12,  43 }, {   3,  53 }, {  14,  34 },
        {  10,  38 }, {  -3,  52 }, {  13,  40 }, {  17,  32 },
        {   7,  44 }, {   7,  38 }, {  13,  50 }, {  10,  57 },
        {  26,  43 },

        /* 338 - 398 */
        {  14,  11 }, {  11,  14 }, {   9,  11 }, {  18,  11 },
        {  21,   9 }, {  23,  -2 }, {  32, -15 }, {  32, -15 },
        {  34, -21 }, {  39, -23 }, {  42, -33 }, {  41, -31 },
        {  46, -28 }, {  38, -12 }, {  21,  29 }, {  45, -24 },
        {  53, -45 }, {  48, -26 }, {  65, -43 }, {  43, -19 },
        {  39, -10 }, {  30,   9 }, {  18,  26 }, {  20,  27 },
        {   0,  57 }, { -14,  82 }, {  -5,  75 }, { -19,  97 },
        { -35, 125 }, {  27,   0 }, {  28,   0 }, {  31,  -4 },
        {  27,   6 }, {  34,   8 }, {  30,  10 }, {  24,  22 },
        {  33,  19 }, {  22,  32 }, {  26,  31 }, {  21,  41 },
        {  26,  44 }, {  23,  47 }, {  16,  65 }, {  14,  71 },
        {   8,  60 }, {   6,  63 }, {  17,  65 }, {  21,  24 },
        {  23,  20 }, {  26,  23 }, {  27,  32 }, {  28,  23 },
        {  28,  24 }, {  23,  40 }, {  24,  32 }, {  28,  29 },
        {  23,  42 }, {  19,  57 }, {  22,  53 }, {  22,  61 },
        {  11,  86 },

        /* 399 - 435 */
        {  12,  40 }, {  11,  51 }, {  14,  59 },
        {  -4,  79 }, {  -7,  71 }, {  -5,  69 }, {  -9,  70 },
        {  -8,  66 }, { -10,  68 }, { -19,  73 }, { -12,  69 },
        { -16,  70 }, { -15,  67 }, { -20,  62 }, { -19,  70 },
        { -16,  66 }, { -22,  65 }, { -20,  63 }, {   9,  -2 },
        {  26,  -9 }, {  33,  -9 }, {  39,  -7 }, {  41,  -2 },
        {  45,   3 }, {  49,   9 }, {  45,  27 }, {  36,  59 },
        {  -6,  66 }, {  -7,  35 }, {  -7,  42 }, {  -8,  45 },
        {  -5,  48 }, { -12,  56 }, {  -6,  60 }, {  -5,  62 },
        {  -8,  66 }, {  -8,  76 },

        /* 436 - 459 */
        {  -5,  85 }, {  -6,  81 }, { -10,  77 }, {  -7,  81 },
        { -17,  80 }, { -18,  73 }, {  -4,  74 }, { -10,  83 },
        {  -9,  71 }, {  -9,  67 }, {  -1,  61 }, {  -8,  66 },
        { -14,  66 }, {   0,  59 }, {   2,  59 }, {  21, -13 },
        {  33, -14 }, {  39,  -7 }, {  46,  -2 }, {  51,   2 },
        {  60,   6 }, {  61,  17 }, {  55,  34 }, {  42,  62 },

        /* 460 - 1024 */
        {  -7,  92 }, {  -5,  89 }, {  -7,  96 }, { -13, 108 },
        {  -3,  46 }, {  -1,  65 }, {  -1,  57 }, {  -9,  93 },
        {  -3,  74 }, {  -9,  92 }, {  -8,  87 }, { -23, 126 },
        {  -7,  92 }, {  -5,  89 }, {  -7,  96 }, { -13, 108 },
        {  -3,  46 }, {  -1,  65 }, {  -1,  57 }, {  -9,  93 },
        {  -3,  74 }, {  -9,  92 }, {  -8,  87 }, { -23, 126 },
        {  -2,  85 }, {  -6,  78 }, {  -1,  75 }, {  -7,  77 },
        {   2,  54 }, {   5,  50 }, {  -3,  68 }, {   1,  50 },
        {   6,  42 }, {  -4,  81 }, {   1,  63 }, {  -4,  70 },
        {   0,  67 }, {   2,  57 }, {  -2,  76 }, {  11,  35 },
        {   4,  64 }, {   1,  61 }, {  11,  35 }, {  18,  25 },
        {  12,  24 }, {  13,  29 }, {  13,  36 }, { -10,  93 },
        {  -7,  73 }, {  -2,  73 }, {  13,  46 }, {   9,  49 },
        {  -7, 100 }, {   9,  53 }, {   2,  53 }, {   5,  53 },
        {  -2,  61 }, {   0,  56 }, {   0,  56 }, { -13,  63 },
        {  -5,  60 }, {  -1,  62 }, {   4,  57 }, {  -6,  69 },
        {   4,  57 }, {  14,  39 }, {   4,  51 }, {  13,  68 },
        {  -2,  85 }, {  -6,  78 }, {  -1,  75 }, {  -7,  77 },
        {   2,  54 }, {   5,  50 }, {  -3,  68 }, {   1,  50 },
        {   6,  42 }, {  -4,  81 }, {   1,  63 }, {  -4,  70 },
        {   0,  67 }, {   2,  57 }, {  -2,  76 }, {  11,  35 },
        {   4,  64 }, {   1,  61 }, {  11,  35 }, {  18,  25 },
        {  12,  24 }, {  13,  29 }, {  13,  36 }, { -10,  93 },
        {  -7,  73 }, {  -2,  73 }, {  13,  46 }, {   9,  49 },
        {  -7, 100 }, {   9,  53 }, {   2,  53 }, {   5,  53 },
        {  -2,  61 }, {   0,  56 }, {   0,  56 }, { -13,  63 },
        {  -5,  60 }, {  -1,  62 }, {   4,  57 }, {  -6,  69 },
        {   4,  57 }, {  14,  39 }, {   4,  51 }, {  13,  68 },
        {  11,  28 }, {   2,  40 }, {   3,  44 }, {   0,  49 },
        {   0,  46 }, {   2,  44 }, {   2,  51 }, {   0,  47 },
        {   4,  39 }, {   2,  62 }, {   6,  46 }, {   0,  54 },
        {   3,  54 }, {   2,  58 }, {   4,  63 }, {   6,  51 },
        {   6,  57 }, {   7,  53 }, {   6,  52 }, {   6,  55 },
        {  11,  45 }, {  14,  36 }, {   8,  53 }, {  -1,  82 },
        {   7,  55 }, {  -3,  78 }, {  15,  46 }, {  22,  31 },
        {  -1,  84 }, {  25,   7 }, {  30,  -7 }, {  28,   3 },
        {  28,   4 }, {  32,   0 }, {  34,  -1 }, {  30,   6 },
        {  30,   6 }, {  32,   9 }, {  31,  19 }, {  26,  27 },
        {  26,  30 }, {  37,  20 }, {  28,  34 }, {  17,  70 },
        {  11,  28 }, {   2,  40 }, {   3,  44 }, {   0,  49 },
        {   0,  46 }, {   2,  44 }, {   2,  51 }, {   0,  47 },
        {   4,  39 }, {   2,  62 }, {   6,  46 }, {   0,  54 },
        {   3,  54 }, {   2,  58 }, {   4,  63 }, {   6,  51 },
        {   6,  57 }, {   7,  53 }, {   6,  52 }, {   6,  55 },
        {  11,  45 }, {  14,  36 }, {   8,  53 }, {  -1,  82 },
        {   7,  55 }, {  -3,  78 }, {  15,  46 }, {  22,  31 },
        {  -1,  84 }, {  25,   7 }, {  30,  -7 }, {  28,   3 },
        {  28,   4 }, {  32,   0 }, {  34,  -1 }, {  30,   6 },
        {  30,   6 }, {  32,   9 }, {  31,  19 }, {  26,  27 },
        {  26,  30 }, {  37,  20 }, {  28,  34 }, {  17,  70 },
        {  -4,  79 }, {  -7,  71 }, {  -5,  69 }, {  -9,  70 },
        {  -8,  66 }, { -10,  68 }, { -19,  73 }, { -12,  69 },
        { -16,  70 }, { -15,  67 }, { -20,  62 }, { -19,  70 },
        { -16,  66 }, { -22,  65 }, { -20,  63 }, {  -5,  85 },
        {  -6,  81 }, { -10,  77 }, {  -7,  81 }, { -17,  80 },
        { -18,  73 }, {  -4,  74 }, { -10,  83 }, {  -9,  71 },
        {  -9,  67 }, {  -1,  61 }, {  -8,  66 }, { -14,  66 },
        {   0,  59 }, {   2,  59 }, {   9,  -2 }, {  26,  -9 },
        {  33,  -9 }, {  39,  -7 }, {  41,  -2 }, {  45,   3 },
        {  49,   9 }, {  45,  27 }, {  36,  59 }, {  21, -13 },
        {  33, -14 }, {  39,  -7 }, {  46,  -2 }, {  51,   2 },
        {  60,   6 }, {  61,  17 }, {  55,  34 }, {  42,  62 },
        {  -6,  66 }, {  -7,  35 }, {  -7,  42 }, {  -8,  45 },
        {  -5,  48 }, { -12,  56 }, {  -6,  60 }, {  -5,  62 },
        {  -8,  66 }, {  -8,  76 }, {  -4,  79 }, {  -7,  71 },
        {  -5,  69 }, {  -9,  70 }, {  -8,  66 }, { -10,  68 },
        { -19,  73 }, { -12,  69 }, { -16,  70 }, { -15,  67 },
        { -20,  62 }, { -19,  70 }, { -16,  66 }, { -22,  65 },
        { -20,  63 }, {  -5,  85 }, {  -6,  81 }, { -10,  77 },
        {  -7,  81 }, { -17,  80 }, { -18,  73 }, {  -4,  74 },
        { -10,  83 }, {  -9,  71 }, {  -9,  67 }, {  -1,  61 },
        {  -8,  66 }, { -14,  66 }, {   0,  59 }, {   2,  59 },
        {   9,  -2 }, {  26,  -9 }, {  33,  -9 }, {  39,  -7 },
        {  41,  -2 }, {  45,   3 }, {  49,   9 }, {  45,  27 },
        {  36,  59 }, {  21, -13 }, {  33, -14 }, {  39,  -7 },
        {  46,  -2 }, {  51,   2 }, {  60,   6 }, {  61,  17 },
        {  55,  34 }, {  42,  62 }, {  -6,  66 }, {  -7,  35 },
        {  -7,  42 }, {  -8,  45 }, {  -5,  48 }, { -12,  56 },
        {  -6,  60 }, {  -5,  62 }, {  -8,  66 }, {  -8,  76 },
        { -13, 106 }, { -16, 106 }, { -10,  87 }, { -21, 114 },
        { -18, 110 }, { -14,  98 }, { -22, 110 }, { -21, 106 },
        { -18, 103 }, { -21, 107 }, { -23, 108 }, { -26, 112 },
        { -10,  96 }, { -12,  95 }, {  -5,  91 }, {  -9,  93 },
        { -22,  94 }, {  -5,  86 }, {   9,  67 }, {  -4,  80 },
        { -10,  85 }, {  -1,  70 }, {   7,  60 }, {   9,  58 },
        {   5,  61 }, {  12,  50 }, {  15,  50 }, {  18,  49 },
        {  17,  54 }, {  10,  41 }, {   7,  46 }, {  -1,  51 },
        {   7,  49 }, {   8,  52 }, {   9,  41 }, {   6,  47 },
        {   2,  55 }, {  13,  41 }, {  10,  44 }, {   6,  50 },
        {   5,  53 }, {  13,  49 }, {   4,  63 }, {   6,  64 },
        { -13, 106 }, { -16, 106 }, { -10,  87 }, { -21, 114 },
        { -18, 110 }, { -14,  98 }, { -22, 110 }, { -21, 106 },
        { -18, 103 }, { -21, 107 }, { -23, 108 }, { -26, 112 },
        { -10,  96 }, { -12,  95 }, {  -5,  91 }, {  -9,  93 },
        { -22,  94 }, {  -5,  86 }, {   9,  67 }, {  -4,  80 },
        { -10,  85 }, {  -1,  70 }, {   7,  60 }, {   9,  58 },
        {   5,  61 }, {  12,  50 }, {  15,  50 }, {  18,  49 },
        {  17,  54 }, {  10,  41 }, {   7,  46 }, {  -1,  51 },
        {   7,  49 }, {   8,  52 }, {   9,  41 }, {   6,  47 },
        {   2,  55 }, {  13,  41 }, {  10,  44 }, {   6,  50 },
        {   5,  53 }, {  13,  49 }, {   4,  63 }, {   6,  64 },
        {  14,  11 }, {  11,  14 }, {   9,  11 }, {  18,  11 },
        {  21,   9 }, {  23,  -2 }, {  32, -15 }, {  32, -15 },
        {  34, -21 }, {  39, -23 }, {  42, -33 }, {  41, -31 },
        {  46, -28 }, {  38, -12 }, {  21,  29 }, {  45, -24 },
        {  53, -45 }, {  48, -26 }, {  65, -43 }, {  43, -19 },
        {  39, -10 }, {  30,   9 }, {  18,  26 }, {  20,  27 },
        {   0,  57 }, { -14,  82 }, {  -5,  75 }, { -19,  97 },
        { -35, 125 }, {  27,   0 }, {  28,   0 }, {  31,  -4 },
        {  27,   6 }, {  34,   8 }, {  30,  10 }, {  24,  22 },
        {  33,  19 }, {  22,  32 }, {  26,  31 }, {  21,  41 },
        {  26,  44 }, {  23,  47 }, {  16,  65 }, {  14,  71 },
        {  14,  11 }, {  11,  14 }, {   9,  11 }, {  18,  11 },
        {  21,   9 }, {  23,  -2 }, {  32, -15 }, {  32, -15 },
        {  34, -21 }, {  39, -23 }, {  42, -33 }, {  41, -31 },
        {  46, -28 }, {  38, -12 }, {  21,  29 }, {  45, -24 },
        {  53, -45 }, {  48, -26 }, {  65, -43 }, {  43, -19 },
        {  39, -10 }, {  30,   9 }, {  18,  26 }, {  20,  27 },
        {   0,  57 }, { -14,  82 }, {  -5,  75 }, { -19,  97 },
        { -35, 125 }, {  27,   0 }, {  28,   0 }, {  31,  -4 },
        {  27,   6 }, {  34,   8 }, {  30,  10 }, {  24,  22 },
        {  33,  19 }, {  22,  32 }, {  26,  31 }, {  21,  41 },
        {  26,  44 }, {  23,  47 }, {  16,  65 }, {  14,  71 },
        {  -6,  76 }, {  -2,  44 }, {   0,  45 }, {   0,  52 },
        {  -3,  64 }, {  -2,  59 }, {  -4,  70 }, {  -4,  75 },
        {  -8,  82 }, { -17, 102 }, {  -9,  77 }, {   3,  24 },
        {   0,  42 }, {   0,  48 }, {   0,  55 }, {  -6,  59 },
        {  -7,  71 }, { -12,  83 }, { -11,  87 }, { -30, 119 },
        {   1,  58 }, {  -3,  29 }, {  -1,  36 }, {   1,  38 },
        {   2,  43 }, {  -6,  55 }, {   0,  58 }, {   0,  64 },
        {  -3,  74 }, { -10,  90 }, {  -6,  76 }, {  -2,  44 },
        {   0,  45 }, {   0,  52 }, {  -3,  64 }, {  -2,  59 },
        {  -4,  70 }, {  -4,  75 }, {  -8,  82 }, { -17, 102 },
        {  -9,  77 }, {   3,  24 }, {   0,  42 }, {   0,  48 },
        {   0,  55 }, {  -6,  59 }, {  -7,  71 }, { -12,  83 },
        { -11,  87 }, { -30, 119 }, {   1,  58 }, {  -3,  29 },
        {  -1,  36 }, {   1,  38 }, {   2,  43 }, {  -6,  55 },
        {   0,  58 }, {   0,  64 }, {  -3,  74 }, { -10,  90 },
        {  -3,  74 }, {  -9,  92 }, {  -8,  87 }, { -23, 126 },
        {  -3,  74 }, {  -9,  92 }, {  -8,  87 }, { -23, 126 },
        {  -3,  74 }, {  -9,  92 }, {  -8,  87 }, { -23, 126 }
    },

    /* i_cabac_init_idc == 1 */
    {
        /* 0 - 10 */
        {  20, -15 }, {   2,  54 }, {   3,  74 }, {  20, -15 },
        {   2,  54 }, {   3,  74 }, { -28, 127 }, { -23, 104 },
        {  -6,  53 }, {  -1,  54 }, {   7,  51 },

        /* 11 - 23 */
        {  22,  25 }, {  34,   0 }, {  16,   0 }, {  -2,   9 },
        {   4,  41 }, { -29, 118 }, {   2,  65 }, {  -6,  71 },
        { -13,  79 }, {   5,  52 }, {   9,  50 }, {  -3,  70 },
        {  10,  54 },

        /* 24 - 39 */
        {  26,  34 }, {  19,  22 }, {  40,   0 }, {  57,   2 },
        {  41,  36 }, {  26,  69 }, { -45, 127 }, { -15, 101 },
        {  -4,  76 }, {  -6,  71 }, { -13,  79 }, {   5,  52 },
        {   6,  69 }, { -13,  90 }, {   0,  52 }, {   8,  43 },

        /* 40 - 53 */
        {  -2,  69 },{  -5,  82 },{ -10,  96 },{   2,  59 },
        {   2,  75 },{  -3,  87 },{  -3,  100 },{   1,  56 },
        {  -3,  74 },{  -6,  85 },{   0,  59 },{  -3,  81 },
        {  -7,  86 },{  -5,  95 },

        /* 54 - 59 */
        {  -1,  66 },{  -1,  77 },{   1,  70 },{  -2,  86 },
        {  -5,  72 },{   0,  61 },

        /* 60 - 69 */
        { 0, 41 },   { 0, 63 },   { 0, 63 },     { 0, 63 },
        { -9, 83 },  { 4, 86 },   { 0, 97 },     { -7, 72 },
        { 13, 41 },  { 3, 62 },

        /* 70 - 104 */
        {  13,  15 }, {   7,  51 }, {   2,  80 }, { -39, 127 },
        { -18,  91 }, { -17,  96 }, { -26,  81 }, { -35,  98 },
        { -24, 102 }, { -23,  97 }, { -27, 119 }, { -24,  99 },
        { -21, 110 }, { -18, 102 }, { -36, 127 }, {   0,  80 },
        {  -5,  89 }, {  -7,  94 }, {  -4,  92 }, {   0,  39 },
        {   0,  65 }, { -15,  84 }, { -35, 127 }, {  -2,  73 },
        { -12, 104 }, {  -9,  91 }, { -31, 127 }, {   3,  55 },
        {   7,  56 }, {   7,  55 }, {   8,  61 }, {  -3,  53 },
        {   0,  68 }, {  -7,  74 }, {  -9,  88 },

        /* 105 -> 165 */
        { -13, 103 }, { -13,  91 }, {  -9,  89 }, { -14,  92 },
        {  -8,  76 }, { -12,  87 }, { -23, 110 }, { -24, 105 },
        { -10,  78 }, { -20, 112 }, { -17,  99 }, { -78, 127 },
        { -70, 127 }, { -50, 127 }, { -46, 127 }, {  -4,  66 },
        {  -5,  78 }, {  -4,  71 }, {  -8,  72 }, {   2,  59 },
        {  -1,  55 }, {  -7,  70 }, {  -6,  75 }, {  -8,  89 },
        { -34, 119 }, {  -3,  75 }, {  32,  20 }, {  30,  22 },
        { -44, 127 }, {   0,  54 }, {  -5,  61 }, {   0,  58 },
        {  -1,  60 }, {  -3,  61 }, {  -8,  67 }, { -25,  84 },
        { -14,  74 }, {  -5,  65 }, {   5,  52 }, {   2,  57 },
        {   0,  61 }, {  -9,  69 }, { -11,  70 }, {  18,  55 },
        {  -4,  71 }, {   0,  58 }, {   7,  61 }, {   9,  41 },
        {  18,  25 }, {   9,  32 }, {   5,  43 }, {   9,  47 },
        {   0,  44 }, {   0,  51 }, {   2,  46 }, {  19,  38 },
        {  -4,  66 }, {  15,  38 }, {  12,  42 }, {   9,  34 },
        {   0,  89 },

        /* 166 - 226 */
        {   4,  45 }, {  10,  28 }, {  10,  31 }, {  33, -11 },
        {  52, -43 }, {  18,  15 }, {  28,   0 }, {  35, -22 },
        {  38, -25 }, {  34,   0 }, {  39, -18 }, {  32, -12 },
        { 102, -94 }, {   0,   0 }, {  56, -15 }, {  33,  -4 },
        {  29,  10 }, {  37,  -5 }, {  51, -29 }, {  39,  -9 },
        {  52, -34 }, {  69, -58 }, {  67, -63 }, {  44,  -5 },
        {  32,   7 }, {  55, -29 }, {  32,   1 }, {   0,   0 },
        {  27,  36 }, {  33, -25 }, {  34, -30 }, {  36, -28 },
        {  38, -28 }, {  38, -27 }, {  34, -18 }, {  35, -16 },
        {  34, -14 }, {  32,  -8 }, {  37,  -6 }, {  35,   0 },
        {  30,  10 }, {  28,  18 }, {  26,  25 }, {  29,  41 },
        {   0,  75 }, {   2,  72 }, {   8,  77 }, {  14,  35 },
        {  18,  31 }, {  17,  35 }, {  21,  30 }, {  17,  45 },
        {  20,  42 }, {  18,  45 }, {  27,  26 }, {  16,  54 },
        {   7,  66 }, {  16,  56 }, {  11,  73 }, {  10,  67 },
        { -10, 116 },

        /* 227 - 275 */
        { -23, 112 }, { -15,  71 }, {  -7,  61 }, {   0,  53 },
        {  -5,  66 }, { -11,  77 }, {  -9,  80 }, {  -9,  84 },
        { -10,  87 }, { -34, 127 }, { -21, 101 }, {  -3,  39 },
        {  -5,  53 }, {  -7,  61 }, { -11,  75 }, { -15,  77 },
        { -17,  91 }, { -25, 107 }, { -25, 111 }, { -28, 122 },
        { -11,  76 }, { -10,  44 }, { -10,  52 }, { -10,  57 },
        {  -9,  58 }, { -16,  72 }, {  -7,  69 }, {  -4,  69 },
        {  -5,  74 }, {  -9,  86 }, {   2,  66 }, {  -9,  34 },
        {   1,  32 }, {  11,  31 }, {   5,  52 }, {  -2,  55 },
        {  -2,  67 }, {   0,  73 }, {  -8,  89 }, {   3,  52 },
        {   7,   4 }, {  10,   8 }, {  17,   8 }, {  16,  19 },
        {   3,  37 }, {  -1,  61 }, {  -5,  73 }, {  -1,  70 },
        {  -4,  78 },

        /* 276 a bit special (not used, bypass is used instead) */
        { 0, 0 },

        /* 277 - 337 */
        { -21, 126 }, { -23, 124 }, { -20, 110 }, { -26, 126 },
        { -25, 124 }, { -17, 105 }, { -27, 121 }, { -27, 117 },
        { -17, 102 }, { -26, 117 }, { -27, 116 }, { -33, 122 },
        { -10,  95 }, { -14, 100 }, {  -8,  95 }, { -17, 111 },
        { -28, 114 }, {  -6,  89 }, {  -2,  80 }, {  -4,  82 },
        {  -9,  85 }, {  -8,  81 }, {  -1,  72 }, {   5,  64 },
        {   1,  67 }, {   9,  56 }, {   0,  69 }, {   1,  69 },
        {   7,  69 }, {  -7,  69 }, {  -6,  67 }, { -16,  77 },
        {  -2,  64 }, {   2,  61 }, {  -6,  67 }, {  -3,  64 },
        {   2,  57 }, {  -3,  65 }, {  -3,  66 }, {   0,  62 },
        {   9,  51 }, {  -1,  66 }, {  -2,  71 }, {  -2,  75 },
        {  -1,  70 }, {  -9,  72 }, {  14,  60 }, {  16,  37 },
        {   0,  47 }, {  18,  35 }, {  11,  37 }, {  12,  41 },
        {  10,  41 }, {   2,  48 }, {  12,  41 }, {  13,  41 },
        {   0,  59 }, {   3,  50 }, {  19,  40 }, {   3,  66 },
        {  18,  50 },

        /* 338 - 398 */
        {  19,  -6 }, {  18,  -6 }, {  14,   0 }, {  26, -12 },
        {  31, -16 }, {  33, -25 }, {  33, -22 }, {  37, -28 },
        {  39, -30 }, {  42, -30 }, {  47, -42 }, {  45, -36 },
        {  49, -34 }, {  41, -17 }, {  32,   9 }, {  69, -71 },
        {  63, -63 }, {  66, -64 }, {  77, -74 }, {  54, -39 },
        {  52, -35 }, {  41, -10 }, {  36,   0 }, {  40,  -1 },
        {  30,  14 }, {  28,  26 }, {  23,  37 }, {  12,  55 },
        {  11,  65 }, {  37, -33 }, {  39, -36 }, {  40, -37 },
        {  38, -30 }, {  46, -33 }, {  42, -30 }, {  40, -24 },
        {  49, -29 }, {  38, -12 }, {  40, -10 }, {  38,  -3 },
        {  46,  -5 }, {  31,  20 }, {  29,  30 }, {  25,  44 },
        {  12,  48 }, {  11,  49 }, {  26,  45 }, {  22,  22 },
        {  23,  22 }, {  27,  21 }, {  33,  20 }, {  26,  28 },
        {  30,  24 }, {  27,  34 }, {  18,  42 }, {  25,  39 },
        {  18,  50 }, {  12,  70 }, {  21,  54 }, {  14,  71 },
        {  11,  83 },

        /* 399 - 435 */
        {  25,  32 }, {  21,  49 }, {  21,  54 },
        {  -5,  85 }, {  -6,  81 }, { -10,  77 }, {  -7,  81 },
        { -17,  80 }, { -18,  73 }, {  -4,  74 }, { -10,  83 },
        {  -9,  71 }, {  -9,  67 }, {  -1,  61 }, {  -8,  66 },
        { -14,  66 }, {   0,  59 }, {   2,  59 }, {  17, -10 },
        {  32, -13 }, {  42,  -9 }, {  49,  -5 }, {  53,   0 },
        {  64,   3 }, {  68,  10 }, {  66,  27 }, {  47,  57 },
        {  -5,  71 }, {   0,  24 }, {  -1,  36 }, {  -2,  42 },
        {  -2,  52 }, {  -9,  57 }, {  -6,  63 }, {  -4,  65 },
        {  -4,  67 }, {  -7,  82 },

        /* 436 - 459 */
        {  -3,  81 }, {  -3,  76 }, {  -7,  72 }, {  -6,  78 },
        { -12,  72 }, { -14,  68 }, {  -3,  70 }, {  -6,  76 },
        {  -5,  66 }, {  -5,  62 }, {   0,  57 }, {  -4,  61 },
        {  -9,  60 }, {   1,  54 }, {   2,  58 }, {  17, -10 },
        {  32, -13 }, {  42,  -9 }, {  49,  -5 }, {  53,   0 },
        {  64,   3 }, {  68,  10 }, {  66,  27 }, {  47,  57 },

        /* 460 - 1024 */
        {   0,  80 }, {  -5,  89 }, {  -7,  94 }, {  -4,  92 },
        {   0,  39 }, {   0,  65 }, { -15,  84 }, { -35, 127 },
        {  -2,  73 }, { -12, 104 }, {  -9,  91 }, { -31, 127 },
        {   0,  80 }, {  -5,  89 }, {  -7,  94 }, {  -4,  92 },
        {   0,  39 }, {   0,  65 }, { -15,  84 }, { -35, 127 },
        {  -2,  73 }, { -12, 104 }, {  -9,  91 }, { -31, 127 },
        { -13, 103 }, { -13,  91 }, {  -9,  89 }, { -14,  92 },
        {  -8,  76 }, { -12,  87 }, { -23, 110 }, { -24, 105 },
        { -10,  78 }, { -20, 112 }, { -17,  99 }, { -78, 127 },
        { -70, 127 }, { -50, 127 }, { -46, 127 }, {  -4,  66 },
        {  -5,  78 }, {  -4,  71 }, {  -8,  72 }, {   2,  59 },
        {  -1,  55 }, {  -7,  70 }, {  -6,  75 }, {  -8,  89 },
        { -34, 119 }, {  -3,  75 }, {  32,  20 }, {  30,  22 },
        { -44, 127 }, {   0,  54 }, {  -5,  61 }, {   0,  58 },
        {  -1,  60 }, {  -3,  61 }, {  -8,  67 }, { -25,  84 },
        { -14,  74 }, {  -5,  65 }, {   5,  52 }, {   2,  57 },
        {   0,  61 }, {  -9,  69 }, { -11,  70 }, {  18,  55 },
        { -13, 103 }, { -13,  91 }, {  -9,  89 }, { -14,  92 },
        {  -8,  76 }, { -12,  87 }, { -23, 110 }, { -24, 105 },
        { -10,  78 }, { -20, 112 }, { -17,  99 }, { -78, 127 },
        { -70, 127 }, { -50, 127 }, { -46, 127 }, {  -4,  66 },
        {  -5,  78 }, {  -4,  71 }, {  -8,  72 }, {   2,  59 },
        {  -1,  55 }, {  -7,  70 }, {  -6,  75 }, {  -8,  89 },
        { -34, 119 }, {  -3,  75 }, {  32,  20 }, {  30,  22 },
        { -44, 127 }, {   0,  54 }, {  -5,  61 }, {   0,  58 },
        {  -1,  60 }, {  -3,  61 }, {  -8,  67 }, { -25,  84 },
        { -14,  74 }, {  -5,  65 }, {   5,  52 }, {   2,  57 },
        {   0,  61 }, {  -9,  69 }, { -11,  70 }, {  18,  55 },
        {   4,  45 }, {  10,  28 }, {  10,  31 }, {  33, -11 },
        {  52, -43 }, {  18,  15 }, {  28,   0 }, {  35, -22 },
        {  38, -25 }, {  34,   0 }, {  39, -18 }, {  32, -12 },
        { 102, -94 }, {   0,   0 }, {  56, -15 }, {  33,  -4 },
        {  29,  10 }, {  37,  -5 }, {  51, -29 }, {  39,  -9 },
        {  52, -34 }, {  69, -58 }, {  67, -63 }, {  44,  -5 },
        {  32,   7 }, {  55, -29 }, {  32,   1 }, {   0,   0 },
        {  27,  36 }, {  33, -25 }, {  34, -30 }, {  36, -28 },
        {  38, -28 }, {  38, -27 }, {  34, -18 }, {  35, -16 },
        {  34, -14 }, {  32,  -8 }, {  37,  -6 }, {  35,   0 },
        {  30,  10 }, {  28,  18 }, {  26,  25 }, {  29,  41 },
        {   4,  45 }, {  10,  28 }, {  10,  31 }, {  33, -11 },
        {  52, -43 }, {  18,  15 }, {  28,   0 }, {  35, -22 },
        {  38, -25 }, {  34,   0 }, {  39, -18 }, {  32, -12 },
        { 102, -94 }, {   0,   0 }, {  56, -15 }, {  33,  -4 },
        {  29,  10 }, {  37,  -5 }, {  51, -29 }, {  39,  -9 },
        {  52, -34 }, {  69, -58 }, {  67, -63 }, {  44,  -5 },
        {  32,   7 }, {  55, -29 }, {  32,   1 }, {   0,   0 },
        {  27,  36 }, {  33, -25 }, {  34, -30 }, {  36, -28 },
        {  38, -28 }, {  38, -27 }, {  34, -18 }, {  35, -16 },
        {  34, -14 }, {  32,  -8 }, {  37,  -6 }, {  35,   0 },
        {  30,  10 }, {  28,  18 }, {  26,  25 }, {  29,  41 },
        {  -5,  85 }, {  -6,  81 }, { -10,  77 }, {  -7,  81 },
        { -17,  80 }, { -18,  73 }, {  -4,  74 }, { -10,  83 },
        {  -9,  71 }, {  -9,  67 }, {  -1,  61 }, {  -8,  66 },
        { -14,  66 }, {   0,  59 }, {   2,  59 }, {  -3,  81 },
        {  -3,  76 }, {  -7,  72 }, {  -6,  78 }, { -12,  72 },
        { -14,  68 }, {  -3,  70 }, {  -6,  76 }, {  -5,  66 },
        {  -5,  62 }, {   0,  57 }, {  -4,  61 }, {  -9,  60 },
        {   1,  54 }, {   2,  58 }, {  17, -10 }, {  32, -13 },
        {  42,  -9 }, {  49,  -5 }, {  53,   0 }, {  64,   3 },
        {  68,  10 }, {  66,  27 }, {  47,  57 }, {  17, -10 },
        {  32, -13 }, {  42,  -9 }, {  49,  -5 }, {  53,   0 },
        {  64,   3 }, {  68,  10 }, {  66,  27 }, {  47,  57 },
        {  -5,  71 }, {   0,  24 }, {  -1,  36 }, {  -2,  42 },
        {  -2,  52 }, {  -9,  57 }, {  -6,  63 }, {  -4,  65 },
        {  -4,  67 }, {  -7,  82 }, {  -5,  85 }, {  -6,  81 },
        { -10,  77 }, {  -7,  81 }, { -17,  80 }, { -18,  73 },
        {  -4,  74 }, { -10,  83 }, {  -9,  71 }, {  -9,  67 },
        {  -1,  61 }, {  -8,  66 }, { -14,  66 }, {   0,  59 },
        {   2,  59 }, {  -3,  81 }, {  -3,  76 }, {  -7,  72 },
        {  -6,  78 }, { -12,  72 }, { -14,  68 }, {  -3,  70 },
        {  -6,  76 }, {  -5,  66 }, {  -5,  62 }, {   0,  57 },
        {  -4,  61 }, {  -9,  60 }, {   1,  54 }, {   2,  58 },
        {  17, -10 }, {  32, -13 }, {  42,  -9 }, {  49,  -5 },
        {  53,   0 }, {  64,   3 }, {  68,  10 }, {  66,  27 },
        {  47,  57 }, {  17, -10 }, {  32, -13 }, {  42,  -9 },
        {  49,  -5 }, {  53,   0 }, {  64,   3 }, {  68,  10 },
        {  66,  27 }, {  47,  57 }, {  -5,  71 }, {   0,  24 },
        {  -1,  36 }, {  -2,  42 }, {  -2,  52 }, {  -9,  57 },
        {  -6,  63 }, {  -4,  65 }, {  -4,  67 }, {  -7,  82 },
        { -21, 126 }, { -23, 124 }, { -20, 110 }, { -26, 126 },
        { -25, 124 }, { -17, 105 }, { -27, 121 }, { -27, 117 },
        { -17, 102 }, { -26, 117 }, { -27, 116 }, { -33, 122 },
        { -10,  95 }, { -14, 100 }, {  -8,  95 }, { -17, 111 },
        { -28, 114 }, {  -6,  89 }, {  -2,  80 }, {  -4,  82 },
        {  -9,  85 }, {  -8,  81 }, {  -1,  72 }, {   5,  64 },
        {   1,  67 }, {   9,  56 }, {   0,  69 }, {   1,  69 },
        {   7,  69 }, {  -7,  69 }, {  -6,  67 }, { -16,  77 },
        {  -2,  64 }, {   2,  61 }, {  -6,  67 }, {  -3,  64 },
        {   2,  57 }, {  -3,  65 }, {  -3,  66 }, {   0,  62 },
        {   9,  51 }, {  -1,  66 }, {  -2,  71 }, {  -2,  75 },
        { -21, 126 }, { -23, 124 }, { -20, 110 }, { -26, 126 },
        { -25, 124 }, { -17, 105 }, { -27, 121 }, { -27, 117 },
        { -17, 102 }, { -26, 117 }, { -27, 116 }, { -33, 122 },
        { -10,  95 }, { -14, 100 }, {  -8,  95 }, { -17, 111 },
        { -28, 114 }, {  -6,  89 }, {  -2,  80 }, {  -4,  82 },
        {  -9,  85 }, {  -8,  81 }, {  -1,  72 }, {   5,  64 },
        {   1,  67 }, {   9,  56 }, {   0,  69 }, {   1,  69 },
        {   7,  69 }, {  -7,  69 }, {  -6,  67 }, { -16,  77 },
        {  -2,  64 }, {   2,  61 }, {  -6,  67 }, {  -3,  64 },
        {   2,  57 }, {  -3,  65 }, {  -3,  66 }, {   0,  62 },
        {   9,  51 }, {  -1,  66 }, {  -2,  71 }, {  -2,  75 },
        {  19,  -6 }, {  18,  -6 }, {  14,   0 }, {  26, -12 },
        {  31, -16 }, {  33, -25 }, {  33, -22 }, {  37, -28 },
        {  39, -30 }, {  42, -30 }, {  47, -42 }, {  45, -36 },
        {  49, -34 }, {  41, -17 }, {  32,   9 }, {  69, -71 },
        {  63, -63 }, {  66, -64 }, {  77, -74 }, {  54, -39 },
        {  52, -35 }, {  41, -10 }, {  36,   0 }, {  40,  -1 },
        {  30,  14 }, {  28,  26 }, {  23,  37 }, {  12,  55 },
        {  11,  65 }, {  37, -33 }, {  39, -36 }, {  40, -37 },
        {  38, -30 }, {  46, -33 }, {  42, -30 }, {  40, -24 },
        {  49, -29 }, {  38, -12 }, {  40, -10 }, {  38,  -3 },
        {  46,  -5 }, {  31,  20 }, {  29,  30 }, {  25,  44 },
        {  19,  -6 }, {  18,  -6 }, {  14,   0 }, {  26, -12 },
        {  31, -16 }, {  33, -25 }, {  33, -22 }, {  37, -28 },
        {  39, -30 }, {  42, -30 }, {  47, -42 }, {  45, -36 },
        {  49, -34 }, {  41, -17 }, {  32,   9 }, {  69, -71 },
        {  63, -63 }, {  66, -64 }, {  77, -74 }, {  54, -39 },
        {  52, -35 }, {  41, -10 }, {  36,   0 }, {  40,  -1 },
        {  30,  14 }, {  28,  26 }, {  23,  37 }, {  12,  55 },
        {  11,  65 }, {  37, -33 }, {  39, -36 }, {  40, -37 },
        {  38, -30 }, {  46, -33 }, {  42, -30 }, {  40, -24 },
        {  49, -29 }, {  38, -12 }, {  40, -10 }, {  38,  -3 },
        {  46,  -5 }, {  31,  20 }, {  29,  30 }, {  25,  44 },
        { -23, 112 }, { -15,  71 }, {  -7,  61 }, {   0,  53 },
        {  -5,  66 }, { -11,  77 }, {  -9,  80 }, {  -9,  84 },
        { -10,  87 }, { -34, 127 }, { -21, 101 }, {  -3,  39 },
        {  -5,  53 }, {  -7,  61 }, { -11,  75 }, { -15,  77 },
        { -17,  91 }, { -25, 107 }, { -25, 111 }, { -28, 122 },
        { -11,  76 }, { -10,  44 }, { -10,  52 }, { -10,  57 },
        {  -9,  58 }, { -16,  72 }, {  -7,  69 }, {  -4,  69 },
        {  -5,  74 }, {  -9,  86 }, { -23, 112 }, { -15,  71 },
        {  -7,  61 }, {   0,  53 }, {  -5,  66 }, { -11,  77 },
        {  -9,  80 }, {  -9,  84 }, { -10,  87 }, { -34, 127 },
        { -21, 101 }, {  -3,  39 }, {  -5,  53 }, {  -7,  61 },
        { -11,  75 }, { -15,  77 }, { -17,  91 }, { -25, 107 },
        { -25, 111 }, { -28, 122 }, { -11,  76 }, { -10,  44 },
        { -10,  52 }, { -10,  57 }, {  -9,  58 }, { -16,  72 },
        {  -7,  69 }, {  -4,  69 }, {  -5,  74 }, {  -9,  86 },
        {  -2,  73 }, { -12, 104 }, {  -9,  91 }, { -31, 127 },
        {  -2,  73 }, { -12, 104 }, {  -9,  91 }, { -31, 127 },
        {  -2,  73 }, { -12, 104 }, {  -9,  91 }, { -31, 127 }
    },

    /* i_cabac_init_idc == 2 */
    {
        /* 0 - 10 */
        {  20, -15 }, {   2,  54 }, {   3,  74 }, {  20, -15 },
        {   2,  54 }, {   3,  74 }, { -28, 127 }, { -23, 104 },
        {  -6,  53 }, {  -1,  54 }, {   7,  51 },

        /* 11 - 23 */
        {  29,  16 }, {  25,   0 }, {  14,   0 }, { -10,  51 },
        {  -3,  62 }, { -27,  99 }, {  26,  16 }, {  -4,  85 },
        { -24, 102 }, {   5,  57 }, {   6,  57 }, { -17,  73 },
        {  14,  57 },

        /* 24 - 39 */
        {  20,  40 }, {  20,  10 }, {  29,   0 }, {  54,   0 },
        {  37,  42 }, {  12,  97 }, { -32, 127 }, { -22, 117 },
        {  -2,  74 }, {  -4,  85 }, { -24, 102 }, {   5,  57 },
        {  -6,  93 }, { -14,  88 }, {  -6,  44 }, {   4,  55 },

        /* 40 - 53 */
        { -11,  89 },{ -15,  103 },{ -21,  116 },{  19,  57 },
        {  20,  58 },{   4,  84 },{   6,  96 },{   1,  63 },
        {  -5,  85 },{ -13,  106 },{   5,  63 },{   6,  75 },
        {  -3,  90 },{  -1,  101 },

        /* 54 - 59 */
        {   3,  55 },{  -4,  79 },{  -2,  75 },{ -12,  97 },
        {  -7,  50 },{   1,  60 },

        /* 60 - 69 */
        { 0, 41 },   { 0, 63 },   { 0, 63 },     { 0, 63 },
        { -9, 83 },  { 4, 86 },   { 0, 97 },     { -7, 72 },
        { 13, 41 },  { 3, 62 },

        /* 70 - 104 */
        {   7,  34 }, {  -9,  88 }, { -20, 127 }, { -36, 127 },
        { -17,  91 }, { -14,  95 }, { -25,  84 }, { -25,  86 },
        { -12,  89 }, { -17,  91 }, { -31, 127 }, { -14,  76 },
        { -18, 103 }, { -13,  90 }, { -37, 127 }, {  11,  80 },
        {   5,  76 }, {   2,  84 }, {   5,  78 }, {  -6,  55 },
        {   4,  61 }, { -14,  83 }, { -37, 127 }, {  -5,  79 },
        { -11, 104 }, { -11,  91 }, { -30, 127 }, {   0,  65 },
        {  -2,  79 }, {   0,  72 }, {  -4,  92 }, {  -6,  56 },
        {   3,  68 }, {  -8,  71 }, { -13,  98 },

        /* 105 -> 165 */
        {  -4,  86 }, { -12,  88 }, {  -5,  82 }, {  -3,  72 },
        {  -4,  67 }, {  -8,  72 }, { -16,  89 }, {  -9,  69 },
        {  -1,  59 }, {   5,  66 }, {   4,  57 }, {  -4,  71 },
        {  -2,  71 }, {   2,  58 }, {  -1,  74 }, {  -4,  44 },
        {  -1,  69 }, {   0,  62 }, {  -7,  51 }, {  -4,  47 },
        {  -6,  42 }, {  -3,  41 }, {  -6,  53 }, {   8,  76 },
        {  -9,  78 }, { -11,  83 }, {   9,  52 }, {   0,  67 },
        {  -5,  90 }, {   1,  67 }, { -15,  72 }, {  -5,  75 },
        {  -8,  80 }, { -21,  83 }, { -21,  64 }, { -13,  31 },
        { -25,  64 }, { -29,  94 }, {   9,  75 }, {  17,  63 },
        {  -8,  74 }, {  -5,  35 }, {  -2,  27 }, {  13,  91 },
        {   3,  65 }, {  -7,  69 }, {   8,  77 }, { -10,  66 },
        {   3,  62 }, {  -3,  68 }, { -20,  81 }, {   0,  30 },
        {   1,   7 }, {  -3,  23 }, { -21,  74 }, {  16,  66 },
        { -23, 124 }, {  17,  37 }, {  44, -18 }, {  50, -34 },
        { -22, 127 },

        /* 166 - 226 */
        {   4,  39 }, {   0,  42 }, {   7,  34 }, {  11,  29 },
        {   8,  31 }, {   6,  37 }, {   7,  42 }, {   3,  40 },
        {   8,  33 }, {  13,  43 }, {  13,  36 }, {   4,  47 },
        {   3,  55 }, {   2,  58 }, {   6,  60 }, {   8,  44 },
        {  11,  44 }, {  14,  42 }, {   7,  48 }, {   4,  56 },
        {   4,  52 }, {  13,  37 }, {   9,  49 }, {  19,  58 },
        {  10,  48 }, {  12,  45 }, {   0,  69 }, {  20,  33 },
        {   8,  63 }, {  35, -18 }, {  33, -25 }, {  28,  -3 },
        {  24,  10 }, {  27,   0 }, {  34, -14 }, {  52, -44 },
        {  39, -24 }, {  19,  17 }, {  31,  25 }, {  36,  29 },
        {  24,  33 }, {  34,  15 }, {  30,  20 }, {  22,  73 },
        {  20,  34 }, {  19,  31 }, {  27,  44 }, {  19,  16 },
        {  15,  36 }, {  15,  36 }, {  21,  28 }, {  25,  21 },
        {  30,  20 }, {  31,  12 }, {  27,  16 }, {  24,  42 },
        {   0,  93 }, {  14,  56 }, {  15,  57 }, {  26,  38 },
        { -24, 127 },

        /* 227 - 275 */
        { -24, 115 }, { -22,  82 }, {  -9,  62 }, {   0,  53 },
        {   0,  59 }, { -14,  85 }, { -13,  89 }, { -13,  94 },
        { -11,  92 }, { -29, 127 }, { -21, 100 }, { -14,  57 },
        { -12,  67 }, { -11,  71 }, { -10,  77 }, { -21,  85 },
        { -16,  88 }, { -23, 104 }, { -15,  98 }, { -37, 127 },
        { -10,  82 }, {  -8,  48 }, {  -8,  61 }, {  -8,  66 },
        {  -7,  70 }, { -14,  75 }, { -10,  79 }, {  -9,  83 },
        { -12,  92 }, { -18, 108 }, {  -4,  79 }, { -22,  69 },
        { -16,  75 }, {  -2,  58 }, {   1,  58 }, { -13,  78 },
        {  -9,  83 }, {  -4,  81 }, { -13,  99 }, { -13,  81 },
        {  -6,  38 }, { -13,  62 }, {  -6,  58 }, {  -2,  59 },
        { -16,  73 }, { -10,  76 }, { -13,  86 }, {  -9,  83 },
        { -10,  87 },

        /* 276 a bit special (not used, bypass is used instead) */
        { 0, 0 },

        /* 277 - 337 */
        { -22, 127 }, { -25, 127 }, { -25, 120 }, { -27, 127 },
        { -19, 114 }, { -23, 117 }, { -25, 118 }, { -26, 117 },
        { -24, 113 }, { -28, 118 }, { -31, 120 }, { -37, 124 },
        { -10,  94 }, { -15, 102 }, { -10,  99 }, { -13, 106 },
        { -50, 127 }, {  -5,  92 }, {  17,  57 }, {  -5,  86 },
        { -13,  94 }, { -12,  91 }, {  -2,  77 }, {   0,  71 },
        {  -1,  73 }, {   4,  64 }, {  -7,  81 }, {   5,  64 },
        {  15,  57 }, {   1,  67 }, {   0,  68 }, { -10,  67 },
        {   1,  68 }, {   0,  77 }, {   2,  64 }, {   0,  68 },
        {  -5,  78 }, {   7,  55 }, {   5,  59 }, {   2,  65 },
        {  14,  54 }, {  15,  44 }, {   5,  60 }, {   2,  70 },
        {  -2,  76 }, { -18,  86 }, {  12,  70 }, {   5,  64 },
        { -12,  70 }, {  11,  55 }, {   5,  56 }, {   0,  69 },
        {   2,  65 }, {  -6,  74 }, {   5,  54 }, {   7,  54 },
        {  -6,  76 }, { -11,  82 }, {  -2,  77 }, {  -2,  77 },
        {  25,  42 },

        /* 338 - 398 */
        {  17, -13 }, {  16,  -9 }, {  17, -12 }, {  27, -21 },
        {  37, -30 }, {  41, -40 }, {  42, -41 }, {  48, -47 },
        {  39, -32 }, {  46, -40 }, {  52, -51 }, {  46, -41 },
        {  52, -39 }, {  43, -19 }, {  32,  11 }, {  61, -55 },
        {  56, -46 }, {  62, -50 }, {  81, -67 }, {  45, -20 },
        {  35,  -2 }, {  28,  15 }, {  34,   1 }, {  39,   1 },
        {  30,  17 }, {  20,  38 }, {  18,  45 }, {  15,  54 },
        {   0,  79 }, {  36, -16 }, {  37, -14 }, {  37, -17 },
        {  32,   1 }, {  34,  15 }, {  29,  15 }, {  24,  25 },
        {  34,  22 }, {  31,  16 }, {  35,  18 }, {  31,  28 },
        {  33,  41 }, {  36,  28 }, {  27,  47 }, {  21,  62 },
        {  18,  31 }, {  19,  26 }, {  36,  24 }, {  24,  23 },
        {  27,  16 }, {  24,  30 }, {  31,  29 }, {  22,  41 },
        {  22,  42 }, {  16,  60 }, {  15,  52 }, {  14,  60 },
        {   3,  78 }, { -16, 123 }, {  21,  53 }, {  22,  56 },
        {  25,  61 },

        /* 399 - 435 */
        {  21,  33 }, {  19,  50 }, {  17,  61 },
        {  -3,  78 }, {  -8,  74 }, {  -9,  72 }, { -10,  72 },
        { -18,  75 }, { -12,  71 }, { -11,  63 }, {  -5,  70 },
        { -17,  75 }, { -14,  72 }, { -16,  67 }, {  -8,  53 },
        { -14,  59 }, {  -9,  52 }, { -11,  68 }, {   9,  -2 },
        {  30, -10 }, {  31,  -4 }, {  33,  -1 }, {  33,   7 },
        {  31,  12 }, {  37,  23 }, {  31,  38 }, {  20,  64 },
        {  -9,  71 }, {  -7,  37 }, {  -8,  44 }, { -11,  49 },
        { -10,  56 }, { -12,  59 }, {  -8,  63 }, {  -9,  67 },
        {  -6,  68 }, { -10,  79 },

        /* 436 - 459 */
        {  -3,  78 }, {  -8,  74 }, {  -9,  72 }, { -10,  72 },
        { -18,  75 }, { -12,  71 }, { -11,  63 }, {  -5,  70 },
        { -17,  75 }, { -14,  72 }, { -16,  67 }, {  -8,  53 },
        { -14,  59 }, {  -9,  52 }, { -11,  68 }, {   9,  -2 },
        {  30, -10 }, {  31,  -4 }, {  33,  -1 }, {  33,   7 },
        {  31,  12 }, {  37,  23 }, {  31,  38 }, {  20,  64 },

        /* 460 - 1024 */
        {  11,  80 }, {   5,  76 }, {   2,  84 }, {   5,  78 },
        {  -6,  55 }, {   4,  61 }, { -14,  83 }, { -37, 127 },
        {  -5,  79 }, { -11, 104 }, { -11,  91 }, { -30, 127 },
        {  11,  80 }, {   5,  76 }, {   2,  84 }, {   5,  78 },
        {  -6,  55 }, {   4,  61 }, { -14,  83 }, { -37, 127 },
        {  -5,  79 }, { -11, 104 }, { -11,  91 }, { -30, 127 },
        {  -4,  86 }, { -12,  88 }, {  -5,  82 }, {  -3,  72 },
        {  -4,  67 }, {  -8,  72 }, { -16,  89 }, {  -9,  69 },
        {  -1,  59 }, {   5,  66 }, {   4,  57 }, {  -4,  71 },
        {  -2,  71 }, {   2,  58 }, {  -1,  74 }, {  -4,  44 },
        {  -1,  69 }, {   0,  62 }, {  -7,  51 }, {  -4,  47 },
        {  -6,  42 }, {  -3,  41 }, {  -6,  53 }, {   8,  76 },
        {  -9,  78 }, { -11,  83 }, {   9,  52 }, {   0,  67 },
        {  -5,  90 }, {   1,  67 }, { -15,  72 }, {  -5,  75 },
        {  -8,  80 }, { -21,  83 }, { -21,  64 }, { -13,  31 },
        { -25,  64 }, { -29,  94 }, {   9,  75 }, {  17,  63 },
        {  -8,  74 }, {  -5,  35 }, {  -2,  27 }, {  13,  91 },
        {  -4,  86 }, { -12,  88 }, {  -5,  82 }, {  -3,  72 },
        {  -4,  67 }, {  -8,  72 }, { -16,  89 }, {  -9,  69 },
        {  -1,  59 }, {   5,  66 }, {   4,  57 }, {  -4,  71 },
        {  -2,  71 }, {   2,  58 }, {  -1,  74 }, {  -4,  44 },
        {  -1,  69 }, {   0,  62 }, {  -7,  51 }, {  -4,  47 },
        {  -6,  42 }, {  -3,  41 }, {  -6,  53 }, {   8,  76 },
        {  -9,  78 }, { -11,  83 }, {   9,  52 }, {   0,  67 },
        {  -5,  90 }, {   1,  67 }, { -15,  72 }, {  -5,  75 },
        {  -8,  80 }, { -21,  83 }, { -21,  64 }, { -13,  31 },
        { -25,  64 }, { -29,  94 }, {   9,  75 }, {  17,  63 },
        {  -8,  74 }, {  -5,  35 }, {  -2,  27 }, {  13,  91 },
        {   4,  39 }, {   0,  42 }, {   7,  34 }, {  11,  29 },
        {   8,  31 }, {   6,  37 }, {   7,  42 }, {   3,  40 },
        {   8,  33 }, {  13,  43 }, {  13,  36 }, {   4,  47 },
        {   3,  55 }, {   2,  58 }, {   6,  60 }, {   8,  44 },
        {  11,  44 }, {  14,  42 }, {   7,  48 }, {   4,  56 },
        {   4,  52 }, {  13,  37 }, {   9,  49 }, {  19,  58 },
        {  10,  48 }, {  12,  45 }, {   0,  69 }, {  20,  33 },
        {   8,  63 }, {  35, -18 }, {  33, -25 }, {  28,  -3 },
        {  24,  10 }, {  27,   0 }, {  34, -14 }, {  52, -44 },
        {  39, -24 }, {  19,  17 }, {  31,  25 }, {  36,  29 },
        {  24,  33 }, {  34,  15 }, {  30,  20 }, {  22,  73 },
        {   4,  39 }, {   0,  42 }, {   7,  34 }, {  11,  29 },
        {   8,  31 }, {   6,  37 }, {   7,  42 }, {   3,  40 },
        {   8,  33 }, {  13,  43 }, {  13,  36 }, {   4,  47 },
        {   3,  55 }, {   2,  58 }, {   6,  60 }, {   8,  44 },
        {  11,  44 }, {  14,  42 }, {   7,  48 }, {   4,  56 },
        {   4,  52 }, {  13,  37 }, {   9,  49 }, {  19,  58 },
        {  10,  48 }, {  12,  45 }, {   0,  69 }, {  20,  33 },
        {   8,  63 }, {  35, -18 }, {  33, -25 }, {  28,  -3 },
        {  24,  10 }, {  27,   0 }, {  34, -14 }, {  52, -44 },
        {  39, -24 }, {  19,  17 }, {  31,  25 }, {  36,  29 },
        {  24,  33 }, {  34,  15 }, {  30,  20 }, {  22,  73 },
        {  -3,  78 }, {  -8,  74 }, {  -9,  72 }, { -10,  72 },
        { -18,  75 }, { -12,  71 }, { -11,  63 }, {  -5,  70 },
        { -17,  75 }, { -14,  72 }, { -16,  67 }, {  -8,  53 },
        { -14,  59 }, {  -9,  52 }, { -11,  68 }, {  -3,  78 },
        {  -8,  74 }, {  -9,  72 }, { -10,  72 }, { -18,  75 },
        { -12,  71 }, { -11,  63 }, {  -5,  70 }, { -17,  75 },
        { -14,  72 }, { -16,  67 }, {  -8,  53 }, { -14,  59 },
        {  -9,  52 }, { -11,  68 }, {   9,  -2 }, {  30, -10 },
        {  31,  -4 }, {  33,  -1 }, {  33,   7 }, {  31,  12 },
        {  37,  23 }, {  31,  38 }, {  20,  64 }, {   9,  -2 },
        {  30, -10 }, {  31,  -4 }, {  33,  -1 }, {  33,   7 },
        {  31,  12 }, {  37,  23 }, {  31,  38 }, {  20,  64 },
        {  -9,  71 }, {  -7,  37 }, {  -8,  44 }, { -11,  49 },
        { -10,  56 }, { -12,  59 }, {  -8,  63 }, {  -9,  67 },
        {  -6,  68 }, { -10,  79 }, {  -3,  78 }, {  -8,  74 },
        {  -9,  72 }, { -10,  72 }, { -18,  75 }, { -12,  71 },
        { -11,  63 }, {  -5,  70 }, { -17,  75 }, { -14,  72 },
        { -16,  67 }, {  -8,  53 }, { -14,  59 }, {  -9,  52 },
        { -11,  68 }, {  -3,  78 }, {  -8,  74 }, {  -9,  72 },
        { -10,  72 }, { -18,  75 }, { -12,  71 }, { -11,  63 },
        {  -5,  70 }, { -17,  75 }, { -14,  72 }, { -16,  67 },
        {  -8,  53 }, { -14,  59 }, {  -9,  52 }, { -11,  68 },
        {   9,  -2 }, {  30, -10 }, {  31,  -4 }, {  33,  -1 },
        {  33,   7 }, {  31,  12 }, {  37,  23 }, {  31,  38 },
        {  20,  64 }, {   9,  -2 }, {  30, -10 }, {  31,  -4 },
        {  33,  -1 }, {  33,   7 }, {  31,  12 }, {  37,  23 },
        {  31,  38 }, {  20,  64 }, {  -9,  71 }, {  -7,  37 },
        {  -8,  44 }, { -11,  49 }, { -10,  56 }, { -12,  59 },
        {  -8,  63 }, {  -9,  67 }, {  -6,  68 }, { -10,  79 },
        { -22, 127 }, { -25, 127 }, { -25, 120 }, { -27, 127 },
        { -19, 114 }, { -23, 117 }, { -25, 118 }, { -26, 117 },
        { -24, 113 }, { -28, 118 }, { -31, 120 }, { -37, 124 },
        { -10,  94 }, { -15, 102 }, { -10,  99 }, { -13, 106 },
        { -50, 127 }, {  -5,  92 }, {  17,  57 }, {  -5,  86 },
        { -13,  94 }, { -12,  91 }, {  -2,  77 }, {   0,  71 },
        {  -1,  73 }, {   4,  64 }, {  -7,  81 }, {   5,  64 },
        {  15,  57 }, {   1,  67 }, {   0,  68 }, { -10,  67 },
        {   1,  68 }, {   0,  77 }, {   2,  64 }, {   0,  68 },
        {  -5,  78 }, {   7,  55 }, {   5,  59 }, {   2,  65 },
        {  14,  54 }, {  15,  44 }, {   5,  60 }, {   2,  70 },
        { -22, 127 }, { -25, 127 }, { -25, 120 }, { -27, 127 },
        { -19, 114 }, { -23, 117 }, { -25, 118 }, { -26, 117 },
        { -24, 113 }, { -28, 118 }, { -31, 120 }, { -37, 124 },
        { -10,  94 }, { -15, 102 }, { -10,  99 }, { -13, 106 },
        { -50, 127 }, {  -5,  92 }, {  17,  57 }, {  -5,  86 },
        { -13,  94 }, { -12,  91 }, {  -2,  77 }, {   0,  71 },
        {  -1,  73 }, {   4,  64 }, {  -7,  81 }, {   5,  64 },
        {  15,  57 }, {   1,  67 }, {   0,  68 }, { -10,  67 },
        {   1,  68 }, {   0,  77 }, {   2,  64 }, {   0,  68 },
        {  -5,  78 }, {   7,  55 }, {   5,  59 }, {   2,  65 },
        {  14,  54 }, {  15,  44 }, {   5,  60 }, {   2,  70 },
        {  17, -13 }, {  16,  -9 }, {  17, -12 }, {  27, -21 },
        {  37, -30 }, {  41, -40 }, {  42, -41 }, {  48, -47 },
        {  39, -32 }, {  46, -40 }, {  52, -51 }, {  46, -41 },
        {  52, -39 }, {  43, -19 }, {  32,  11 }, {  61, -55 },
        {  56, -46 }, {  62, -50 }, {  81, -67 }, {  45, -20 },
        {  35,  -2 }, {  28,  15 }, {  34,   1 }, {  39,   1 },
        {  30,  17 }, {  20,  38 }, {  18,  45 }, {  15,  54 },
        {   0,  79 }, {  36, -16 }, {  37, -14 }, {  37, -17 },
        {  32,   1 }, {  34,  15 }, {  29,  15 }, {  24,  25 },
        {  34,  22 }, {  31,  16 }, {  35,  18 }, {  31,  28 },
        {  33,  41 }, {  36,  28 }, {  27,  47 }, {  21,  62 },
        {  17, -13 }, {  16,  -9 }, {  17, -12 }, {  27, -21 },
        {  37, -30 }, {  41, -40 }, {  42, -41 }, {  48, -47 },
        {  39, -32 }, {  46, -40 }, {  52, -51 }, {  46, -41 },
        {  52, -39 }, {  43, -19 }, {  32,  11 }, {  61, -55 },
        {  56, -46 }, {  62, -50 }, {  81, -67 }, {  45, -20 },
        {  35,  -2 }, {  28,  15 }, {  34,   1 }, {  39,   1 },
        {  30,  17 }, {  20,  38 }, {  18,  45 }, {  15,  54 },
        {   0,  79 }, {  36, -16 }, {  37, -14 }, {  37, -17 },
        {  32,   1 }, {  34,  15 }, {  29,  15 }, {  24,  25 },
        {  34,  22 }, {  31,  16 }, {  35,  18 }, {  31,  28 },
        {  33,  41 }, {  36,  28 }, {  27,  47 }, {  21,  62 },
        { -24, 115 }, { -22,  82 }, {  -9,  62 }, {   0,  53 },
        {   0,  59 }, { -14,  85 }, { -13,  89 }, { -13,  94 },
        { -11,  92 }, { -29, 127 }, { -21, 100 }, { -14,  57 },
        { -12,  67 }, { -11,  71 }, { -10,  77 }, { -21,  85 },
        { -16,  88 }, { -23, 104 }, { -15,  98 }, { -37, 127 },
        { -10,  82 }, {  -8,  48 }, {  -8,  61 }, {  -8,  66 },
        {  -7,  70 }, { -14,  75 }, { -10,  79 }, {  -9,  83 },
        { -12,  92 }, { -18, 108 }, { -24, 115 }, { -22,  82 },
        {  -9,  62 }, {   0,  53 }, {   0,  59 }, { -14,  85 },
        { -13,  89 }, { -13,  94 }, { -11,  92 }, { -29, 127 },
        { -21, 100 }, { -14,  57 }, { -12,  67 }, { -11,  71 },
        { -10,  77 }, { -21,  85 }, { -16,  88 }, { -23, 104 },
        { -15,  98 }, { -37, 127 }, { -10,  82 }, {  -8,  48 },
        {  -8,  61 }, {  -8,  66 }, {  -7,  70 }, { -14,  75 },
        { -10,  79 }, {  -9,  83 }, { -12,  92 }, { -18, 108 },
        {  -5,  79 }, { -11, 104 }, { -11,  91 }, { -30, 127 },
        {  -5,  79 }, { -11, 104 }, { -11,  91 }, { -30, 127 },
        {  -5,  79 }, { -11, 104 }, { -11,  91 }, { -30, 127 }
    }
};

// ITU-T H.264 Figure 8-8: 8x8 Zig-Zag Scan Order
static const uint8 zigzag_scan_8x8[64] = {
	 0,  1,  8, 16,  9,  2,  3, 10,
	17, 24, 32, 25, 18, 11,  4,  5,
	12, 19, 26, 33, 40, 48, 41, 34,
	27, 20, 13,  6,  7, 14, 21, 28,
	35, 42, 49, 56, 57, 50, 43, 36,
	29, 22, 15, 23, 30, 37, 44, 51,
	58, 59, 52, 45, 38, 31, 39, 46,
	53, 60, 61, 54, 47, 55, 62, 63
};

// ITU-T H.264 Table 9-41: Context index increments for significant_coeff_flag in 8x8 transform blocks
static const uint8 significant_coeff_flag_offset_8x8[63] = {
	0, 1, 2, 3, 4, 5, 5, 4, 4, 3, 3, 4, 4, 4, 5, 5,
	4, 4, 4, 4, 3, 3, 6, 7, 7, 7, 8, 9,10, 9, 8, 7,
	7, 6,11,12,13,11, 6, 7, 8, 9,14,10, 9, 8, 6,11,
   12,13,11, 6, 9,14,10, 9,11,12,13,11,14,10,12
};

// ITU-T H.264 Table 9-42: Context index increments for last_significant_coeff_flag in 8x8 transform blocks
static const uint8 last_coeff_flag_offset_8x8[63] = {
	0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4,
	5, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8
};

// ITU-T H.264 Table 8-15: 8x8 Dequantization Matrix Multipliers V(qp_rem, i, j)
// Multiplier = V[qp % 6][pos_to_v[y][x]] << (qp / 6).
static const int32 dequant_v_8x8[6][6] = {
	{ 20, 18, 32, 19, 25, 24 },
	{ 22, 19, 35, 21, 28, 26 },
	{ 26, 23, 42, 24, 33, 31 },
	{ 28, 25, 45, 26, 35, 33 },
	{ 32, 28, 51, 30, 40, 38 },
	{ 36, 32, 58, 34, 46, 43 }
};

struct CABACContext {
	uint8 state;
	uint8 mps;
};

struct CABACDecoder {
	const uint8* data;
	size_t size;
	size_t bit_pos;
	uint32 codIRange;
	uint32 codIOffset;
	int    overrun; // the slice data ran out: the parse is desynchronised
	CABACContext ctx[1024];
};

#ifdef H264_PROBE_STATS
struct H264ProbeStats {
	int kind;
	int ok;
	int last_mb;
	int mb_count;
	int terminate;
	int overrun;
	int fail_reason;
	size_t bit_pos;
	size_t bit_limit;
	int event_count;
	int event_mb[2048];
	int event_type[2048];
	int event_cbp[2048];
	size_t event_bit0[2048];
	size_t event_bit1[2048];
};

H264ProbeStats g_h264_probe_stats;

static void H264Probe_Reset(int kind, size_t bit_limit) {
	g_h264_probe_stats.kind = kind;
	g_h264_probe_stats.ok = 0;
	g_h264_probe_stats.last_mb = -1;
	g_h264_probe_stats.mb_count = 0;
	g_h264_probe_stats.terminate = 0;
	g_h264_probe_stats.overrun = 0;
	g_h264_probe_stats.fail_reason = 0;
	g_h264_probe_stats.bit_pos = 0;
	g_h264_probe_stats.bit_limit = bit_limit;
	g_h264_probe_stats.event_count = 0;
}

static void H264Probe_MB(uint32 mb_idx) {
	g_h264_probe_stats.last_mb = (int)mb_idx;
	g_h264_probe_stats.mb_count++;
}

static void H264Probe_Event(uint32 mb_idx, size_t bit0, size_t bit1, int mb_type, int cbp) {
	int slot = g_h264_probe_stats.event_count & 2047;
	g_h264_probe_stats.event_mb[slot] = (int)mb_idx;
	g_h264_probe_stats.event_type[slot] = mb_type;
	g_h264_probe_stats.event_cbp[slot] = cbp;
	g_h264_probe_stats.event_bit0[slot] = bit0;
	g_h264_probe_stats.event_bit1[slot] = bit1;
	g_h264_probe_stats.event_count++;
}

static void H264Probe_End(int ok, const CABACDecoder* cabac, int terminate, int fail_reason) {
	g_h264_probe_stats.ok = ok;
	g_h264_probe_stats.terminate = terminate;
	g_h264_probe_stats.fail_reason = fail_reason;
	if (cabac) {
		g_h264_probe_stats.overrun = cabac->overrun;
		g_h264_probe_stats.bit_pos = cabac->bit_pos;
	}
}
#else
static inline void H264Probe_Reset(int, size_t) {}
static inline void H264Probe_MB(uint32) {}
static inline void H264Probe_Event(uint32, size_t, size_t, int, int) {}
static inline void H264Probe_End(int, const CABACDecoder*, int, int) {}
#endif

static inline int CABAC_GetBit(CABACDecoder* cabac) {
	size_t byte_idx = cabac->bit_pos / 8;
	int bit_idx = 7 - (int)(cabac->bit_pos % 8);
	int bit = 0;
	if (byte_idx < cabac->size) {
		bit = (cabac->data[byte_idx] >> bit_idx) & 1;
	} else {
		cabac->overrun = 1;
	}
	cabac->bit_pos++;
	return bit;
}

static inline void CABAC_Init(CABACDecoder* cabac, const uint8* data, size_t size, size_t start_bit_pos, int slice_type, int cabac_init_idc, int slice_qp) {
	cabac->data = data;
	cabac->size = size;
	cabac->bit_pos = start_bit_pos;
	cabac->overrun = 0;

	while (cabac->bit_pos % 8 != 0) {
		cabac->bit_pos++;
	}

	const int8 (*init_table)[2] = nullptr;
	if (slice_type == 2 || slice_type == 7) {
		init_table = cabac_context_init_I;
	} else {
		int idc = cabac_init_idc;
		if (idc < 0 || idc > 2) idc = 0;
		init_table = cabac_context_init_PB[idc];
	}

	for (int i = 0; i < 1024; ++i) {
		int m = init_table[i][0];
		int n = init_table[i][1];
		int preCtxState = ((m * slice_qp) >> 4) + n;
		if (preCtxState < 1) preCtxState = 1;
		if (preCtxState > 126) preCtxState = 126;
		if (preCtxState <= 63) {
			cabac->ctx[i].state = (uint8)(63 - preCtxState);
			cabac->ctx[i].mps = 0;
		} else {
			cabac->ctx[i].state = (uint8)(preCtxState - 64);
			cabac->ctx[i].mps = 1;
		}
	}

	cabac->codIRange = 510;
	cabac->codIOffset = 0;
	for (int i = 0; i < 9; ++i) {
		cabac->codIOffset = (cabac->codIOffset << 1) | CABAC_GetBit(cabac);
	}
}

static inline int CABAC_DecodeBin(CABACDecoder* cabac, int ctxIdx) {
	CABACContext* ctx = &cabac->ctx[ctxIdx];
	uint8 pStateIdx = ctx->state;
	uint8 valMPS = ctx->mps;

	uint32 qCodIRangeIdx = (cabac->codIRange >> 6) & 3;
	uint32 codILPS = rangeTabLPS[pStateIdx][qCodIRangeIdx];
	cabac->codIRange -= codILPS;

	int bin = 0;
	if (cabac->codIOffset >= cabac->codIRange) {
		bin = 1 - valMPS;
		cabac->codIOffset -= cabac->codIRange;
		cabac->codIRange = codILPS;
		if (pStateIdx == 0) {
			ctx->mps = 1 - valMPS;
		}
		ctx->state = transIdxLPS[pStateIdx];
	} else {
		bin = valMPS;
		ctx->state = transIdxMPS[pStateIdx];
	}

	int renorm_guard = 0;

	while (cabac->codIRange < 256) {
		// A conforming stream needs at most a few renormalisation steps; a corrupted one can
		// drive codIRange to zero, where shifting would never reach 256 again (dead lock).
		if (++renorm_guard > 32 || cabac->codIRange == 0) break;
		cabac->codIRange <<= 1;
		cabac->codIOffset = (cabac->codIOffset << 1) | CABAC_GetBit(cabac);
	}
	return bin;
}

static inline int CABAC_DecodeBypass(CABACDecoder* cabac) {
	cabac->codIOffset = (cabac->codIOffset << 1) | CABAC_GetBit(cabac);
	int bin = 0;
	if (cabac->codIOffset >= cabac->codIRange) {
		bin = 1;
		cabac->codIOffset -= cabac->codIRange;
	} else {
		bin = 0;
	}
	return bin;
}

static inline int CABAC_DecodeTerminate(CABACDecoder* cabac) {
	cabac->codIRange -= 2;
	if (cabac->codIOffset >= cabac->codIRange) {
		return 1;
	}
	int renorm_guard = 0;
	while (cabac->codIRange < 256) {
		if (++renorm_guard > 32 || cabac->codIRange == 0) break;
		cabac->codIRange <<= 1;
		cabac->codIOffset = (cabac->codIOffset << 1) | CABAC_GetBit(cabac);
	}
	return 0;
}

// H.264 9.3.2.3.1: the context base is 54 and the
// first context comes from whether the left / top neighbouring partitions use a
// reference index greater than zero; afterwards the context walks 4, 5, 5, 5, ...
static inline int CABAC_DecodeRefIdx(CABACDecoder* cabac, int refa, int refb, int num_ref) {
	// H.264 9.3.2.3.1: ref_idx_lX is a unary code that runs on while the decoded bin is 1 and
	// stops on the first 0, so the value is bounded by the active reference count that the
	// encoder used.  Truncating the loop at num_ref - 1 was tried and is wrong - the encoder
	// still writes the terminating zero bin, so cutting the loop short left that bin unread
	// and shifted every following syntax element (measured: 11.34 vs 6.75 mean error).
	(void)num_ref;
	int ref = 0;
	int ctx = (refa > 0 ? 1 : 0) + (refb > 0 ? 2 : 0);
	while (CABAC_DecodeBin(cabac, 54 + ctx)) {
		ref++;
		ctx = (ctx >> 2) + 4;
		if (ref >= 32) break;
	}
	return ref;
}

// H.264 9.3.2.3.2: the first bin's context depends on
// amvd (the sum of the neighbouring |mvd| of the same component) and the unary
// remainder increments its context while mvd < 4.
static inline int CABAC_DecodeMvdComp(CABACDecoder* cabac, int ctxbase, int amvd, int* mvda) {
	// ctxIdxOffset for mvd_l0 is 40 (x) / 47 (y) and the first bin selects one of three
	// contexts from the neighbour magnitude sum: ctxIdxOffset + (amvd > 2) + (amvd > 32).
	// That is the same test as ((amvd-3)>>31) + ((amvd-33)>>31) + 2.
	// The unary remainder then uses ctxIdxOffset + 3 .. + 6.
	int ctx = ctxbase + (amvd > 2 ? 1 : 0) + (amvd > 32 ? 1 : 0);
	if (CABAC_DecodeBin(cabac, ctx) == 0) {
		*mvda = 0;
		return 0;
	}
	int mvd = 1;
	int ctxb = ctxbase + 3;
	while (mvd < 9 && CABAC_DecodeBin(cabac, ctxb)) {
		if (mvd < 4) ctxb++;
		mvd++;
	}
	if (mvd >= 9) {
		int k = 3;
		while (k <= 24 && CABAC_DecodeBypass(cabac)) {
			mvd += 1 << k;
			k++;
		}
		while (k-- > 0) mvd += CABAC_DecodeBypass(cabac) << k;
		*mvda = mvd < 70 ? mvd : 70;
	} else {
		*mvda = mvd;
	}
	int sign = CABAC_DecodeBypass(cabac);
	return sign ? -mvd : mvd;
}

// Reference index cache: one uint8 per 4x4 block. The context of ref_idx depends on whether
// the left / top neighbouring partitions use a reference index greater than zero, so the
// decoded values have to be kept and looked up the same way as the mvd cache below.
static inline int CABAC_RefNeighbour(const uint8* refs, uint32 mb_idx, uint32 mb_width,
									 int bx4, int by4, bool hasA) {
	// H.264 9.3.3.1.1.6: the context increment counts a neighbour only when it is available,
	// is not skipped, is not intra coded, uses the same prediction list and has a reference
	// index greater than zero.  An entry of 255 marks an intra neighbour, so it must contribute
	// 0; returning the raw marker made (refa > 0) true and picked the wrong context, which
	// desynchronised every slice that codes ref_idx (num_ref_idx_lX_active > 1).
	// CABAC_RefNeighbourDir below already folded the marker away, these two did not.
	int r;
	if (bx4 > 0) r = refs[mb_idx * 16 + by4 * 4 + bx4 - 1];
	else if (hasA) r = refs[(mb_idx - 1) * 16 + by4 * 4 + 3];
	else return 0;
	return (r == 255) ? 0 : r;
}
static inline int CABAC_RefNeighbourTop(const uint8* refs, uint32 mb_idx, uint32 mb_width,
										int bx4, int by4, bool hasB) {
	int r;
	if (by4 > 0) r = refs[mb_idx * 16 + (by4 - 1) * 4 + bx4];
	else if (hasB) r = refs[(mb_idx - mb_width) * 16 + 3 * 4 + bx4];
	else return 0;
	return (r == 255) ? 0 : r;
}
static inline void CABAC_StoreRef(uint8* refs, uint32 mb_idx, int bx4, int by4, int w4, int h4, int ref) {
	for (int yy = 0; yy < h4; ++yy) {
		for (int xx = 0; xx < w4; ++xx) {
			refs[mb_idx * 16 + (by4 + yy) * 4 + bx4 + xx] = (uint8)ref;
		}
	}
}

// A neighbouring partition coded in direct mode does not contribute to the ref_idx context of a
// B slice even when its derived reference index is not zero, so such a neighbour is reported as
// index 0.
static inline int CABAC_RefNeighbourDir(const uint8* refs, const uint8* nv_direct4,
										uint32 mb_idx, uint32 mb_width,
										int bx4, int by4, bool hasA) {
	int idx;
	if (bx4 > 0) idx = (int)(mb_idx * 16) + by4 * 4 + bx4 - 1;
	else if (hasA) idx = (int)(mb_idx - 1) * 16 + by4 * 4 + 3;
	else return 0;
	if (nv_direct4 && nv_direct4[idx]) return 0;
	return (refs[idx] == 255) ? 0 : (int)refs[idx];
}

static inline int CABAC_RefNeighbourTopDir(const uint8* refs, const uint8* nv_direct4,
										   uint32 mb_idx, uint32 mb_width,
										   int bx4, int by4, bool hasB) {
	int idx;
	if (by4 > 0) idx = (int)(mb_idx * 16) + (by4 - 1) * 4 + bx4;
	else if (hasB) idx = (int)(mb_idx - mb_width) * 16 + 12 + bx4;
	else return 0;
	if (nv_direct4 && nv_direct4[idx]) return 0;
	return (refs[idx] == 255) ? 0 : (int)refs[idx];
}

// mvd cache: two int16 components per 4x4 block, so amvd can be taken from the left
// (bx4-1) and top (bx4, by4-1) neighbours, the same way the mvd cache is read below.
static inline int CABAC_MvdNeighbour(const int16* mvd, uint32 mb_idx, uint32 mb_width,
									 int bx4, int by4, bool hasA, bool hasB, int comp) {
	if (bx4 > 0) return mvd[(mb_idx * 16 + by4 * 4 + bx4 - 1) * 2 + comp];
	if (hasA) return mvd[((mb_idx - 1) * 16 + by4 * 4 + 3) * 2 + comp];
	return 0;
}
static inline int CABAC_MvdNeighbourTop(const int16* mvd, uint32 mb_idx, uint32 mb_width,
										int bx4, int by4, bool hasA, bool hasB, int comp) {
	if (by4 > 0) return mvd[(mb_idx * 16 + (by4 - 1) * 4 + bx4) * 2 + comp];
	if (hasB) return mvd[((mb_idx - mb_width) * 16 + 3 * 4 + bx4) * 2 + comp];
	return 0;
}

// Returns the signed motion vector difference; *abs_out receives the decoded
// magnitude, which is what has to be cached: amvd is the sum of the neighbouring
// |mvd| values, and it is the magnitude, not the signed result, that the cache stores.
static inline H264MotionVector CABAC_DecodeMVDAt(CABACDecoder* cabac, const int16* mvd,
												 uint32 mb_idx, uint32 mb_width,
												 bool hasA, bool hasB, int bx4, int by4,
												 H264MotionVector* abs_out) {
	int amvd0 = CABAC_MvdNeighbour(mvd, mb_idx, mb_width, bx4, by4, hasA, hasB, 0)
			  + CABAC_MvdNeighbourTop(mvd, mb_idx, mb_width, bx4, by4, hasA, hasB, 0);
	int amvd1 = CABAC_MvdNeighbour(mvd, mb_idx, mb_width, bx4, by4, hasA, hasB, 1)
			  + CABAC_MvdNeighbourTop(mvd, mb_idx, mb_width, bx4, by4, hasA, hasB, 1);
	H264MotionVector r;
	int da = 0, db = 0;
	r.x = (int16)CABAC_DecodeMvdComp(cabac, 40, amvd0, &da);
	r.y = (int16)CABAC_DecodeMvdComp(cabac, 47, amvd1, &db);
	if (abs_out) {
		abs_out->x = (int16)da;
		abs_out->y = (int16)db;
	}
	return r;
}

static inline void CABAC_StoreMVD(int16* mvd, uint32 mb_idx, int bx4, int by4, int w4, int h4, H264MotionVector v) {
	for (int yy = 0; yy < h4; ++yy)
		for (int xx = 0; xx < w4; ++xx) {
			int slot = (mb_idx * 16 + (by4 + yy) * 4 + bx4 + xx) * 2;
			mvd[slot + 0] = v.x;
			mvd[slot + 1] = v.y;
		}
}

static bool CABAC_DecodeResidual(
	CABACDecoder* cabac,
	int cat,
	int num_coeffs,
	int16* coeffs
) {
	static const int sig_ctx_base[6] = { 105, 120, 134, 149, 152, 402 };
	static const int last_ctx_base[6] = { 166, 181, 195, 210, 213, 417 };

	const uint8* scan = (cat == 5) ? zigzag_scan_8x8 :
						((cat == 3) ? raster_scan_2x2 :
						((cat == 1 || cat == 4) ? zigzag_scan_4x4_ac : zigzag_scan_4x4));

	int sig_positions[64];
	int num_sig = 0;
	bool hit_last = false;

	for (int c = 0; c < num_coeffs - 1; ++c) {
		int ctx = (cat == 5) ? (sig_ctx_base[cat] + significant_coeff_flag_offset_8x8[c]) : (sig_ctx_base[cat] + c);
		if (CABAC_DecodeBin(cabac, ctx)) {
			sig_positions[num_sig++] = c;
			int last_ctx = (cat == 5) ? (last_ctx_base[cat] + last_coeff_flag_offset_8x8[c]) : (last_ctx_base[cat] + c);
			if (CABAC_DecodeBin(cabac, last_ctx)) {
				hit_last = true;
				break;
			}
		}
	}
	if (!hit_last) {
		sig_positions[num_sig++] = num_coeffs - 1;
	}

	static const int abs_ctx_base[6] = { 227, 237, 247, 257, 266, 426 };

	int node_ctx = 0;
	static const uint8 coeff_abs_level1_ctx[8] = { 1, 2, 3, 4, 0, 0, 0, 0 };
	static const uint8 coeff_abs_levelgt1_ctx[8] = { 5, 5, 5, 5, 6, 7, 8, 9 };
	static const uint8 coeff_abs_level_transition[2][8] = {
		{ 1, 2, 3, 3, 4, 5, 6, 7 },
		{ 4, 4, 4, 4, 5, 6, 7, 7 }
	};

	for (int i = num_sig - 1; i >= 0; --i) {
		int pos = sig_positions[i];
		int ctx0 = abs_ctx_base[cat] + coeff_abs_level1_ctx[node_ctx];
		int bin0 = CABAC_DecodeBin(cabac, ctx0);
		int level = 1;
		if (bin0 == 0) {
			level = 1;
			node_ctx = coeff_abs_level_transition[0][node_ctx];
		} else {
			int ctx_gt1 = abs_ctx_base[cat] + coeff_abs_levelgt1_ctx[node_ctx];
			node_ctx = coeff_abs_level_transition[1][node_ctx];
			level = 2;
			while (level < 15 && CABAC_DecodeBin(cabac, ctx_gt1)) {
				level++;
			}
			if (level >= 15) {
				int j = 0;
				while (CABAC_DecodeBypass(cabac) == 1 && j < 23) {
					j++;
				}
				int coeff_abs = 1;
				for (int b = 0; b < j; ++b) {
					coeff_abs = (coeff_abs << 1) | CABAC_DecodeBypass(cabac);
				}
				level = coeff_abs + 14;
			}
		}
		int sign = CABAC_DecodeBypass(cabac);
		coeffs[scan[pos]] = sign ? (int16)-level : (int16)level;
	}
	return true;
}

static void H264_IDCT8x8(const int32 in[64], int32 out[64]) {
	// H.264 8.5.13.2 inverse 8x8 transform.  The odd part is the point of this butterfly: the
	// specification mixes the four odd inputs with the -1, -1/2 and
	// -1/4 terms below.  A symmetric "even/odd" decomposition is NOT equivalent to it, and
	// using one silently corrupts every 8x8 residual - which is invisible in flat areas and so
	// survives as a small blocky band in the only textured macroblocks of a picture.
	//
	// Rounding: the "+32" of the final ">>6" of 8.5.13.2 is folded into coefficient 0 before the
	// first pass, with a single shift in the second pass, which reproduces it exactly.
	int32 m[64];
	int32 in0 = in[0] + 32;
	for (int i = 0; i < 8; ++i) {
		int32 s0 = (i == 0) ? in0 : in[i * 8 + 0];
		int32 s1 = in[i * 8 + 1], s2 = in[i * 8 + 2], s3 = in[i * 8 + 3];
		int32 s4 = in[i * 8 + 4], s5 = in[i * 8 + 5], s6 = in[i * 8 + 6], s7 = in[i * 8 + 7];

		int32 a0 = s0 + s4;
		int32 a2 = s0 - s4;
		int32 a4 = (s2 >> 1) - s6;
		int32 a6 = (s6 >> 1) + s2;
		int32 b0 = a0 + a6;
		int32 b2 = a2 + a4;
		int32 b4 = a2 - a4;
		int32 b6 = a0 - a6;

		int32 a1 = -s3 + s5 - s7 - (s7 >> 1);
		int32 a3 = s1 + s7 - s3 - (s3 >> 1);
		int32 a5 = -s1 + s7 + s5 + (s5 >> 1);
		int32 a7 = s3 + s5 + s1 + (s1 >> 1);
		int32 b1 = (a7 >> 2) + a1;
		int32 b3 = a3 + (a5 >> 2);
		int32 b5 = (a3 >> 2) - a5;
		int32 b7 = a7 - (a1 >> 2);

		m[i * 8 + 0] = b0 + b7;
		m[i * 8 + 1] = b2 + b5;
		m[i * 8 + 2] = b4 + b3;
		m[i * 8 + 3] = b6 + b1;
		m[i * 8 + 4] = b6 - b1;
		m[i * 8 + 5] = b4 - b3;
		m[i * 8 + 6] = b2 - b5;
		m[i * 8 + 7] = b0 - b7;
	}
	for (int i = 0; i < 8; ++i) {
		int32 s0 = m[0 * 8 + i], s1 = m[1 * 8 + i], s2 = m[2 * 8 + i], s3 = m[3 * 8 + i];
		int32 s4 = m[4 * 8 + i], s5 = m[5 * 8 + i], s6 = m[6 * 8 + i], s7 = m[7 * 8 + i];

		int32 a0 = s0 + s4;
		int32 a2 = s0 - s4;
		int32 a4 = (s2 >> 1) - s6;
		int32 a6 = (s6 >> 1) + s2;
		int32 b0 = a0 + a6;
		int32 b2 = a2 + a4;
		int32 b4 = a2 - a4;
		int32 b6 = a0 - a6;

		int32 a1 = -s3 + s5 - s7 - (s7 >> 1);
		int32 a3 = s1 + s7 - s3 - (s3 >> 1);
		int32 a5 = -s1 + s7 + s5 + (s5 >> 1);
		int32 a7 = s3 + s5 + s1 + (s1 >> 1);
		int32 b1 = (a7 >> 2) + a1;
		int32 b3 = a3 + (a5 >> 2);
		int32 b5 = (a3 >> 2) - a5;
		int32 b7 = a7 - (a1 >> 2);

		out[0 * 8 + i] = (b0 + b7) >> 6;
		out[1 * 8 + i] = (b2 + b5) >> 6;
		out[2 * 8 + i] = (b4 + b3) >> 6;
		out[3 * 8 + i] = (b6 + b1) >> 6;
		out[4 * 8 + i] = (b6 - b1) >> 6;
		out[5 * 8 + i] = (b4 - b3) >> 6;
		out[6 * 8 + i] = (b2 - b5) >> 6;
		out[7 * 8 + i] = (b0 - b7) >> 6;
	}
}

static void H264_Dequant8x8(const int16 in[64], int32 qp, int32 out[64]) {
	int32 qp_rem = qp % 6;
	int32 qp_per = qp / 6;
	static const uint8 pos_to_v[8][8] = {
		{ 0, 3, 4, 3, 0, 3, 4, 3 },
		{ 3, 1, 5, 1, 3, 1, 5, 1 },
		{ 4, 5, 2, 5, 4, 5, 2, 5 },
		{ 3, 1, 5, 1, 3, 1, 5, 1 },
		{ 0, 3, 4, 3, 0, 3, 4, 3 },
		{ 3, 1, 5, 1, 3, 1, 5, 1 },
		{ 4, 5, 2, 5, 4, 5, 2, 5 },
		{ 3, 1, 5, 1, 3, 1, 5, 1 }
	};
	for (int y = 0; y < 8; ++y) {
		for (int x = 0; x < 8; ++x) {
			int v_idx = pos_to_v[y][x];
			// The 8x8 dequantiser needs the same normalisation as the 4x4 luminance DC path:
			// a shift of qp/6 - 6 for both, against qp/6 - 4 for the 4x4 AC blocks, over the
			// same tables and the same "(x + 32) >> 6" inverse transform, so with the 4x4 AC
			// shift of 0 in dct.c the net shift here must be 2. Handing the coefficients over
			// unshifted left every luminance 8x8 residual 4x too large - visible as the blocky
			// error band of the IDR, whose only textured macroblocks are coded with
			// transform_size_8x8_flag set.
			int32 scaled = (int32)in[y * 8 + x] * dequant_v_8x8[qp_rem][v_idx];
			if (qp_per >= 2) {
				out[y * 8 + x] = scaled << (qp_per - 2);
			} else {
				// ( scaled << qp_per ) >> 2 == scaled >> ( 2 - qp_per ), and that shift ROUNDS:
				// truncating it leaves every odd coefficient of an 8x8 block one too large in
				// magnitude.  Measured on the seed macroblock of the IDR (mb 97, QP 10, the first
				// sample that differed from the reference): coefficient (2,2) is -153, truncation
				// gives -77 where the correct value is -76, which puts two residual samples of
				// that block one too high.  A sweep of both rounding decisions over that block
				// (fold "+32" into coefficient 0, and this shift) reproduced the reference
				// residual exactly only with both of them rounding.
				int sh = 2 - qp_per;
				out[y * 8 + x] = (scaled + (1 << (sh - 1))) >> sh;
			}
		}
	}
}

static void H264_PredIntra8x8(uint8* dst, int stride, const uint8* top, const uint8* left, uint8 top_left, const uint8* top_right,
							  bool has_top, bool has_left, bool has_top_left, bool has_top_right, int mode) {
	uint8 p_top[17], p_left[17];
	for (int x = 0; x < 8; ++x) p_top[x] = has_top ? top[x] : 128;
	for (int x = 8; x < 16; ++x) p_top[x] = (has_top_right && top_right) ? top_right[x - 8] : (has_top ? top[7] : 128);
	for (int y = 0; y < 8; ++y) p_left[y] = has_left ? left[y] : 128;

	uint8 f_top[17], f_left[9], f_tl = top_left;
	// Unavailable reference samples are 128 (H.264 8.3.2.2.1).  The two entries past the
	// end of the 8x8 block's neighbours are filled in below; initialising them here keeps a
	// mode that reads them well defined even when the neighbours are missing.
	for (int i = 0; i < 17; ++i) f_top[i] = 128;
	for (int i = 0; i < 9; ++i) f_left[i] = 128;
	if (has_top) {
		f_top[0] = ((has_top_left ? top_left : top[0]) + 2 * top[0] + top[1] + 2) >> 2;
		for (int x = 1; x < 15; ++x) f_top[x] = (p_top[x - 1] + 2 * p_top[x] + p_top[x + 1] + 2) >> 2;
		f_top[15] = (p_top[14] + 3 * p_top[15] + 2) >> 2;
	}
	if (has_left) {
		f_left[0] = ((has_top_left ? top_left : left[0]) + 2 * left[0] + left[1] + 2) >> 2;
		for (int y = 1; y < 7; ++y) f_left[y] = (p_left[y - 1] + 2 * p_left[y] + p_left[y + 1] + 2) >> 2;
		f_left[7] = (p_left[6] + 3 * p_left[7] + 2) >> 2;
	}
	if (has_top_left) {
		f_tl = ((has_left ? left[0] : 128) + 2 * top_left + (has_top ? top[0] : 128) + 2) >> 2;
	}

	// The reference samples beyond the end of the block's neighbours repeat the last available
	// one, exactly as L is reused in the 4x4 modes (H.264 8.3.2.2).  Mode 8 reads
	// f_left[y + (x >> 1) + 2], which reaches index 8 for the three pixels with x + 2y == 13;
	// leaving that entry undefined makes the prediction depend on stack garbage.
	f_top[16] = f_top[15];
	f_left[8] = f_left[7];

	#define P8_SAMPLE(x, y) ((y) == -1 ? ((x) == -1 ? f_tl : f_top[x]) : f_left[y])

	switch (mode) {
		case 0: // Vertical
			if (!has_top) mode = 2;
			else {
				for (int y = 0; y < 8; ++y) {
					for (int x = 0; x < 8; ++x) dst[y * stride + x] = f_top[x];
				}
				return;
			}
			break;
		case 1: // Horizontal
			if (!has_left) mode = 2;
			else {
				for (int y = 0; y < 8; ++y) {
					for (int x = 0; x < 8; ++x) dst[y * stride + x] = f_left[y];
				}
				return;
			}
			break;
		case 3: // Diagonal Down-Left
			if (!has_top) mode = 2;
			else {
				for (int y = 0; y < 8; ++y) {
					for (int x = 0; x < 8; ++x) {
						if (x == 7 && y == 7) {
							dst[y * stride + x] = (uint8)((f_top[14] + 3 * f_top[15] + 2) >> 2);
						} else {
							dst[y * stride + x] = (uint8)((f_top[x + y] + 2 * f_top[x + y + 1] + f_top[x + y + 2] + 2) >> 2);
						}
					}
				}
				return;
			}
			break;
		case 4: // Diagonal Down-Right
			if (!has_top || !has_left || !has_top_left) mode = 2;
			else {
				for (int y = 0; y < 8; ++y) {
					for (int x = 0; x < 8; ++x) {
						if (x > y) {
							dst[y * stride + x] = (uint8)((P8_SAMPLE(x - y - 2, -1) + 2 * P8_SAMPLE(x - y - 1, -1) + P8_SAMPLE(x - y, -1) + 2) >> 2);
						} else if (x < y) {
							dst[y * stride + x] = (uint8)((P8_SAMPLE(-1, y - x - 2) + 2 * P8_SAMPLE(-1, y - x - 1) + P8_SAMPLE(-1, y - x) + 2) >> 2);
						} else {
							dst[y * stride + x] = (uint8)((P8_SAMPLE(0, -1) + 2 * P8_SAMPLE(-1, -1) + P8_SAMPLE(-1, 0) + 2) >> 2);
						}
					}
				}
				return;
			}
			break;
		case 5: // Vertical-Right
			if (!has_top || !has_left || !has_top_left) mode = 2;
			else {
				for (int y = 0; y < 8; ++y) {
					for (int x = 0; x < 8; ++x) {
						int zVR = 2 * x - y;
						if (zVR >= 0) {
							if (zVR % 2 == 0) {
								dst[y * stride + x] = (uint8)((P8_SAMPLE(x - (y >> 1) - 1, -1) + P8_SAMPLE(x - (y >> 1), -1) + 1) >> 1);
							} else {
								dst[y * stride + x] = (uint8)((P8_SAMPLE(x - (y >> 1) - 2, -1) + 2 * P8_SAMPLE(x - (y >> 1) - 1, -1) + P8_SAMPLE(x - (y >> 1), -1) + 2) >> 2);
							}
						} else if (zVR == -1) {
							dst[y * stride + x] = (uint8)((P8_SAMPLE(-1, 0) + 2 * P8_SAMPLE(-1, -1) + P8_SAMPLE(0, -1) + 2) >> 2);
						} else {
							dst[y * stride + x] = (uint8)((P8_SAMPLE(-1, y - 2 * x - 1) + 2 * P8_SAMPLE(-1, y - 2 * x - 2) + P8_SAMPLE(-1, y - 2 * x - 3) + 2) >> 2);
						}
					}
				}
				return;
			}
			break;
		case 6: // Horizontal-Down
			if (!has_top || !has_left || !has_top_left) mode = 2;
			else {
				for (int y = 0; y < 8; ++y) {
					for (int x = 0; x < 8; ++x) {
						int zHD = 2 * y - x;
						if (zHD >= 0) {
							if (zHD % 2 == 0) {
								dst[y * stride + x] = (uint8)((P8_SAMPLE(-1, y - (x >> 1) - 1) + P8_SAMPLE(-1, y - (x >> 1)) + 1) >> 1);
							} else {
								dst[y * stride + x] = (uint8)((P8_SAMPLE(-1, y - (x >> 1) - 2) + 2 * P8_SAMPLE(-1, y - (x >> 1) - 1) + P8_SAMPLE(-1, y - (x >> 1)) + 2) >> 2);
							}
						} else if (zHD == -1) {
							dst[y * stride + x] = (uint8)((P8_SAMPLE(-1, 0) + 2 * P8_SAMPLE(-1, -1) + P8_SAMPLE(0, -1) + 2) >> 2);
						} else {
							dst[y * stride + x] = (uint8)((P8_SAMPLE(x - 2 * y - 1, -1) + 2 * P8_SAMPLE(x - 2 * y - 2, -1) + P8_SAMPLE(x - 2 * y - 3, -1) + 2) >> 2);
						}
					}
				}
				return;
			}
			break;
		case 7: // Vertical-Left
			if (!has_top) mode = 2;
			else {
				for (int y = 0; y < 8; ++y) {
					for (int x = 0; x < 8; ++x) {
						if (y % 2 == 0) {
							dst[y * stride + x] = (uint8)((f_top[x + (y >> 1)] + f_top[x + (y >> 1) + 1] + 1) >> 1);
						} else {
							dst[y * stride + x] = (uint8)((f_top[x + (y >> 1)] + 2 * f_top[x + (y >> 1) + 1] + f_top[x + (y >> 1) + 2] + 2) >> 2);
						}
					}
				}
				return;
			}
			break;
		case 8: // Horizontal-Up
			if (!has_left) mode = 2;
			else {
				for (int y = 0; y < 8; ++y) {
					for (int x = 0; x < 8; ++x) {
						int zHU = x + 2 * y;
						if (zHU <= 13) {
							if (zHU % 2 == 0) {
								dst[y * stride + x] = (uint8)((f_left[y + (x >> 1)] + f_left[y + (x >> 1) + 1] + 1) >> 1);
							} else {
								dst[y * stride + x] = (uint8)((f_left[y + (x >> 1)] + 2 * f_left[y + (x >> 1) + 1] + f_left[y + (x >> 1) + 2] + 2) >> 2);
							}
						} else if (zHU == 14) {
							dst[y * stride + x] = (uint8)((f_left[7] + 3 * f_left[7] + 2) >> 2);
						} else {
							dst[y * stride + x] = (uint8)f_left[7];
						}
					}
				}
				return;
			}
			break;
		default: mode = 2; break;
	}

	#undef P8_SAMPLE

	// Mode 2: DC mode.  H.264 8.3.2.2.1 (8-91)..(8-93) sums the FILTERED reference samples pF, not
	// the raw neighbours: summing the raw ones biases the constant prediction by one grey level
	// over the whole 8x8 block.  Measured on the IDR: it was the first sample of the picture that
	// differed from the reference, and every macroblock that predicted from a corrected one
	// inherited the offset - 28,476 samples one too bright.  With pF the IDR is bit exact.
	int dc = 128;
	if (has_top && has_left) {
		int sum = 0;
		for (int i = 0; i < 8; ++i) sum += (int)f_top[i] + (int)f_left[i];
		dc = (sum + 8) >> 4;
	} else if (has_top) {
		int sum = 0;
		for (int i = 0; i < 8; ++i) sum += (int)f_top[i];
		dc = (sum + 4) >> 3;
	} else if (has_left) {
		int sum = 0;
		for (int i = 0; i < 8; ++i) sum += (int)f_left[i];
		dc = (sum + 4) >> 3;
	}
	for (int y = 0; y < 8; ++y) {
		for (int x = 0; x < 8; ++x) dst[y * stride + x] = (uint8)dc;
	}
}

// The non_zero_count cache border of an unavailable neighbour holds 0x80, which the context
// derivation then masks with "0x7f + (b_intra << 7)": 0xff for an intra
// macroblock, so 0x80 survives and !!i_nza is 1, and 0x7f for an inter one, where
// 0x80 & 0x7f == 0 and !!i_nza is 0.  An unavailable neighbour therefore contributes
// condTermFlag == 1 only for an *intra* macroblock; for an inter macroblock it counts as an
// empty block.  The helpers below default to the intra value and the inter paths of the P
// and B slice decoders pass b_intra = 0 explicitly.

static inline int GetLuma4x4CondA(const uint8* nnz, int mb_idx, int mb_w, int k, bool hasA, int b_intra = 1) {
	int bx = ((k & 1) ? 1 : 0) + ((k & 4) ? 2 : 0);
	int by = ((k & 2) ? 1 : 0) + ((k & 8) ? 2 : 0);
	if (bx > 0) {
		int bxA = bx - 1;
		int kA = ((by & 2) << 2) + ((bxA & 2) << 1) + ((by & 1) << 1) + (bxA & 1);
		return nnz[mb_idx * 24 + kA] > 0 ? 1 : 0;
	} else if (hasA) {
		int kA = ((by & 2) << 2) + (2 << 1) + ((by & 1) << 1) + 1;
		return nnz[(mb_idx - 1) * 24 + kA] > 0 ? 1 : 0;
	}
	return b_intra ? 1 : 0;
}

static inline int GetLuma4x4CondB(const uint8* nnz, int mb_idx, int mb_w, int k, bool hasB, int b_intra = 1) {
	int bx = ((k & 1) ? 1 : 0) + ((k & 4) ? 2 : 0);
	int by = ((k & 2) ? 1 : 0) + ((k & 8) ? 2 : 0);
	if (by > 0) {
		int byB = by - 1;
		int kB = ((byB & 2) << 2) + ((bx & 2) << 1) + ((byB & 1) << 1) + (bx & 1);
		return nnz[mb_idx * 24 + kB] > 0 ? 1 : 0;
	} else if (hasB) {
		int kB = (2 << 2) + ((bx & 2) << 1) + (1 << 1) + (bx & 1);
		return nnz[(mb_idx - mb_w) * 24 + kB] > 0 ? 1 : 0;
	}
	return b_intra ? 1 : 0;
}

static inline int GetChromaACCondA(const uint8* nnz, int mb_idx, int mb_w, int plane, int ck, bool hasA, int b_intra = 1) {
	int offset = 16 + plane * 4;
	int cby = (ck >> 1) & 1;
	int cbx = ck & 1;
	if (cbx > 0) {
		return nnz[mb_idx * 24 + offset + cby * 2 + 0] > 0 ? 1 : 0;
	} else if (hasA) {
		return nnz[(mb_idx - 1) * 24 + offset + cby * 2 + 1] > 0 ? 1 : 0;
	}
	return b_intra ? 1 : 0;
}

static inline int GetChromaACCondB(const uint8* nnz, int mb_idx, int mb_w, int plane, int ck, bool hasB, int b_intra = 1) {
	int offset = 16 + plane * 4;
	int cbx = ck & 1;
	int cby = (ck >> 1) & 1;
	if (cby > 0) {
		return nnz[mb_idx * 24 + offset + 0 * 2 + cbx] > 0 ? 1 : 0;
	} else if (hasB) {
		return nnz[(mb_idx - mb_w) * 24 + offset + 1 * 2 + cbx] > 0 ? 1 : 0;
	}
	return b_intra ? 1 : 0;
}

static int CABAC_DecodeIntraMbTypeP(CABACDecoder* cabac, int* cbp_luma, int* cbp_chroma, int* intra16_mode) {
	if (CABAC_DecodeBin(cabac, 17) == 0) {
		if (cbp_luma) *cbp_luma = 0;
		if (cbp_chroma) *cbp_chroma = 0;
		if (intra16_mode) *intra16_mode = 0;
		return 0;
	}
	if (CABAC_DecodeTerminate(cabac)) return 25;
	int luma = CABAC_DecodeBin(cabac, 18) ? 15 : 0;
	int chroma = 0;
	if (CABAC_DecodeBin(cabac, 19)) {
		chroma = CABAC_DecodeBin(cabac, 19) ? 2 : 1;
	}
	// The two mode bins share one context and are MSB first; the operand order of `(a << 1) | b`
	// is unspecified in C, so the reads must be sequenced explicitly or the two bins can be
	// assigned the wrong way round.
	int mode_msb = CABAC_DecodeBin(cabac, 20);
	int mode_lsb = CABAC_DecodeBin(cabac, 20);
	int mode = (mode_msb << 1) | mode_lsb;
	if (cbp_luma) *cbp_luma = luma;
	if (cbp_chroma) *cbp_chroma = chroma;
	if (intra16_mode) *intra16_mode = mode;
	return 1 + mode + 4 * chroma + 12 * (luma ? 1 : 0);
}

// H.264 7.3.5.3 intra prediction modes of an I_NxN macroblock (prev_intra4x4_pred_mode_flag and
// rem_intra4x4_pred_mode, ctxIdxOffset 68/69). Used when an intra macroblock appears inside a
// slice whose blocks are not reconstructed, so that the CABAC stream stays synchronised.
static void CABAC_ConsumeIntraPredModes(
	CABACDecoder* cabac,
	uint32 mb_idx,
	uint32 mb_width,
	bool hasA,
	bool hasB,
	bool is_8x8,
	int8* mode_table
) {
	if (is_8x8) {
		for (int k8 = 0; k8 < 4; ++k8) {
			int mode_A = 2;
			if (k8 == 0) {
				if (hasA) {
					int8 mA = mode_table[(mb_idx - 1) * 16 + 1 * 4 + 1];
					if (mA >= 0) mode_A = (int)mA;
				}
			} else if (k8 == 1) {
				int8 mA = mode_table[mb_idx * 16 + 0 * 4 + 1];
				if (mA >= 0) mode_A = (int)mA;
			} else if (k8 == 2) {
				if (hasA) {
					int8 mA = mode_table[(mb_idx - 1) * 16 + 3 * 4 + 1];
					if (mA >= 0) mode_A = (int)mA;
				}
			} else {
				int8 mA = mode_table[mb_idx * 16 + 2 * 4 + 1];
				if (mA >= 0) mode_A = (int)mA;
			}

			int mode_B = 2;
			if (k8 == 0) {
				if (hasB) {
					int8 mB = mode_table[(mb_idx - mb_width) * 16 + 2 * 4 + 2];
					if (mB >= 0) mode_B = (int)mB;
				}
			} else if (k8 == 1) {
				if (hasB) {
					int8 mB = mode_table[(mb_idx - mb_width) * 16 + 3 * 4 + 2];
					if (mB >= 0) mode_B = (int)mB;
				}
			} else if (k8 == 2) {
				int8 mB = mode_table[mb_idx * 16 + 0 * 4 + 2];
				if (mB >= 0) mode_B = (int)mB;
			} else {
				int8 mB = mode_table[mb_idx * 16 + 1 * 4 + 2];
				if (mB >= 0) mode_B = (int)mB;
			}

			if (((k8 == 0 || k8 == 2) && !hasA) || ((k8 == 0 || k8 == 1) && !hasB)) { mode_A = 2; mode_B = 2; } // 8.3.1.1: 任一侧邻居宏块不可用则 A、B 都按 DC 预测
			int mpm = (mode_A < mode_B) ? mode_A : mode_B;
			int m = mpm;
			if (!CABAC_DecodeBin(cabac, 68)) {
				int rem = 0;
				rem += 1 * CABAC_DecodeBin(cabac, 69);
				rem += 2 * CABAC_DecodeBin(cabac, 69);
				rem += 4 * CABAC_DecodeBin(cabac, 69);
				m = rem + (rem >= mpm);
			}
			for (int sub = 0; sub < 4; ++sub) mode_table[mb_idx * 16 + k8 * 4 + sub] = (int8)m;
		}
		return;
	}

	for (int k = 0; k < 16; ++k) {
		int bx = ((k & 1) ? 1 : 0) + ((k & 4) ? 2 : 0);
		int by = ((k & 2) ? 1 : 0) + ((k & 8) ? 2 : 0);

		int mode_A = 2;
		if (bx > 0) {
			int kA = (by & 2) * 4 + ((bx - 1) & 2) * 2 + (by & 1) * 2 + ((bx - 1) & 1);
			int8 mA = mode_table[mb_idx * 16 + kA];
			if (mA >= 0) mode_A = (int)mA;
		} else if (hasA) {
			int kA = (by & 2) * 4 + (3 & 2) * 2 + (by & 1) * 2 + (3 & 1);
			int8 mA = mode_table[(mb_idx - 1) * 16 + kA];
			if (mA >= 0) mode_A = (int)mA;
		}

		int mode_B = 2;
		if (by > 0) {
			int kB = ((by - 1) & 2) * 4 + (bx & 2) * 2 + ((by - 1) & 1) * 2 + (bx & 1);
			int8 mB = mode_table[mb_idx * 16 + kB];
			if (mB >= 0) mode_B = (int)mB;
		} else if (hasB) {
			int kB = (3 & 2) * 4 + (bx & 2) * 2 + (3 & 1) * 2 + (bx & 1);
			int8 mB = mode_table[(mb_idx - mb_width) * 16 + kB];
			if (mB >= 0) mode_B = (int)mB;
		}

		if ((bx == 0 && !hasA) || (by == 0 && !hasB)) { mode_A = 2; mode_B = 2; } // 8.3.1.1: 任一侧邻居宏块不可用则 A、B 都按 DC 预测
		int mpm = (mode_A < mode_B) ? mode_A : mode_B;
		int m = mpm;
		if (!CABAC_DecodeBin(cabac, 68)) {
			int rem = 0;
			rem += 1 * CABAC_DecodeBin(cabac, 69);
			rem += 2 * CABAC_DecodeBin(cabac, 69);
			rem += 4 * CABAC_DecodeBin(cabac, 69);
			m = rem + (rem >= mpm);
		}
		mode_table[mb_idx * 16 + k] = (int8)m;
	}
}

// ---------------------------------------------------------------------------------------
// Intra macroblock decoder: syntax plus reconstruction.
//
// An intra macroblock can appear inside a B slice (scene cuts and intra refresh use them) and
// it has to be predicted from its reconstructed neighbours with its residual added, exactly as
// the I and P slice decoders do. Filling such a macroblock from the reference picture instead
// leaves a hole that every later picture predicts from.
//
// A coded block pattern is carried inside mb_type for I_16x16 and read here for I_NxN, so the
// caller has to say which of the two it is and hand over the values that already arrived.
// ---------------------------------------------------------------------------------------
struct H264IntraMBArgs {
	CABACDecoder* cabac;
	const H264PPS* pps;
	uint32 mb_idx;
	uint32 mb_width;
	uint32 mb_x;
	bool hasA;
	bool hasB;
	int8* mode_table;
	uint8* transform_8x8_table;
	uint8* intra_chroma_table;
	uint8* cbp_table;
	uint8* cbp_chroma_table;
	uint8* nnz_table;
	uint8* nz_dc_table;
	uint8* mb_qp_out;
	uint8* dst_y;
	uint8* dst_u;
	uint8* dst_v;
	int dst_y_stride;
	int dst_uv_stride;
	bool is_intra16;
	int cbp_luma_in;
	int cbp_chroma_in;
	int intra16_mode_in;
	int* qp;
	bool* prev_mb_has_qp_delta;
};

static void CABAC_DecodeIntraMB(H264IntraMBArgs& a) {
	CABACDecoder* cabac = a.cabac;
	const H264PPS* pps = a.pps;
	const uint32 mb_idx = a.mb_idx;
	const uint32 mb_width = a.mb_width;
	const uint32 mb_x = a.mb_x;
	const bool hasA = a.hasA;
	const bool hasB = a.hasB;
	int8* mode_table = a.mode_table;
	uint8* transform_8x8_table = a.transform_8x8_table;
	uint8* intra_chroma_table = a.intra_chroma_table;
	uint8* cbp_table = a.cbp_table;
	uint8* cbp_chroma_table = a.cbp_chroma_table;
	uint8* nnz_table = a.nnz_table;
	uint8* nz_dc_table = a.nz_dc_table;
	uint8* mb_qp_out = a.mb_qp_out;
	uint8* mb_y_dst = a.dst_y;
	uint8* mb_u_dst = a.dst_u;
	uint8* mb_v_dst = a.dst_v;
	const int dst_y_stride = a.dst_y_stride;
	const int dst_uv_stride = a.dst_uv_stride;
	int current_qp = *a.qp;
	bool prev_mb_has_qp_delta = *a.prev_mb_has_qp_delta;

	int cbp_luma = 0;
	int cbp_chroma = 0;
	int intra_chroma_mode = 0;
	int intra16_mode = 0;
	bool is_8x8_dct = false;

	if (!a.is_intra16) {
		// I_NxN: transform_size_8x8_flag, then the prediction modes, the chroma prediction mode
		// and only then the coded block pattern (H.264 7.3.5).
		if (pps->transform_8x8_mode_flag) {
			int ctx_8x8 = 399 + (hasA && transform_8x8_table[mb_idx - 1] ? 1 : 0)
							   + (hasB && transform_8x8_table[mb_idx - mb_width] ? 1 : 0);
			is_8x8_dct = (CABAC_DecodeBin(cabac, ctx_8x8) != 0);
		}
		transform_8x8_table[mb_idx] = is_8x8_dct ? 1 : 0;

		CABAC_ConsumeIntraPredModes(cabac, mb_idx, mb_width, hasA, hasB, is_8x8_dct, mode_table);

		int ctx_c = 64 + (hasA && intra_chroma_table[mb_idx - 1] > 0 ? 1 : 0)
						 + (hasB && intra_chroma_table[mb_idx - mb_width] > 0 ? 1 : 0);
		if (CABAC_DecodeBin(cabac, ctx_c) == 0) {
			intra_chroma_mode = 0;
		} else {
			intra_chroma_mode = CABAC_DecodeBin(cabac, 67) ? (CABAC_DecodeBin(cabac, 67) ? 3 : 2) : 1;
		}
		intra_chroma_table[mb_idx] = (uint8)intra_chroma_mode;

		// coded_block_pattern: the neighbour test of the four luma contexts is inverted, so a
		// neighbour whose corresponding bit is CLEAR raises the context.
		int condA_0 = (hasA && ((cbp_table[mb_idx - 1] & 2) == 0)) ? 1 : 0;
		int condB_0 = (hasB && ((cbp_table[mb_idx - mb_width] & 4) == 0)) ? 1 : 0;
		if (CABAC_DecodeBin(cabac, 73 + condA_0 + 2 * condB_0)) cbp_luma |= 1;

		int condA_1 = ((cbp_luma & 1) == 0) ? 1 : 0;
		int condB_1 = (hasB && ((cbp_table[mb_idx - mb_width] & 8) == 0)) ? 1 : 0;
		if (CABAC_DecodeBin(cabac, 73 + condA_1 + 2 * condB_1)) cbp_luma |= 2;

		int condA_2 = (hasA && ((cbp_table[mb_idx - 1] & 8) == 0)) ? 1 : 0;
		int condB_2 = ((cbp_luma & 1) == 0) ? 1 : 0;
		if (CABAC_DecodeBin(cabac, 73 + condA_2 + 2 * condB_2)) cbp_luma |= 4;

		int condA_3 = ((cbp_luma & 4) == 0) ? 1 : 0;
		int condB_3 = ((cbp_luma & 2) == 0) ? 1 : 0;
		if (CABAC_DecodeBin(cabac, 73 + condA_3 + 2 * condB_3)) cbp_luma |= 8;

		int ca = hasA ? cbp_chroma_table[mb_idx - 1] : 0;
		int cbc = hasB ? cbp_chroma_table[mb_idx - mb_width] : 0;
		int c0 = (ca > 0 ? 1 : 0) + (cbc > 0 ? 2 : 0);
		if (CABAC_DecodeBin(cabac, 77 + c0) == 0) {
			cbp_chroma = 0;
		} else {
			int c1 = 4 + (ca == 2 ? 1 : 0) + (cbc == 2 ? 2 : 0);
			cbp_chroma = 1 + CABAC_DecodeBin(cabac, 77 + c1);
		}

		cbp_table[mb_idx] = (uint8)cbp_luma;
		cbp_chroma_table[mb_idx] = (uint8)cbp_chroma;

		if ((cbp_luma | (cbp_chroma << 4)) > 0) {
			int ctx_qp = 60 + (prev_mb_has_qp_delta ? 1 : 0);
			if (CABAC_DecodeBin(cabac, ctx_qp)) {
				int val = 1;
				while (val < 64 && CABAC_DecodeBin(cabac, (val == 1 ? 62 : 63))) val++;
				int delta = (val & 1) ? ((val + 1) >> 1) : -((val + 1) >> 1);
				current_qp = (current_qp + delta + 52) % 52;
				prev_mb_has_qp_delta = true;
			} else {
				prev_mb_has_qp_delta = false;
			}
		} else {
			prev_mb_has_qp_delta = false;
		}
	} else {
		// I_16x16: the coded block pattern and the prediction mode arrived with mb_type.
		cbp_luma = a.cbp_luma_in;
		cbp_chroma = a.cbp_chroma_in;
		// The two bins of the shared Intra16x16PredMode context carry the prediction mode
		// itself, MSB first (i_pred >> 1, then i_pred & 1). The mode is NOT permuted: decoded
		// value 0 is Intra_16x16_Vertical and 2 is Intra_16x16_DC. Permuting it swaps Vertical
		// and DC and corrupts every I_16x16 macroblock of a P or B slice whose neighbourhood is
		// not flat - measured on display 16 (a P picture): R-MAE 2.608 -> 0.027 once removed.
		intra16_mode = a.intra16_mode_in & 3;
		// A following I_NxN or inter macroblock still reads this neighbour's coded block
		// pattern, so the table has to be filled even though mb_type carried the pattern.
		cbp_table[mb_idx] = (uint8)cbp_luma;
		cbp_chroma_table[mb_idx] = (uint8)cbp_chroma;

		int ctx_c = 64 + (hasA && intra_chroma_table[mb_idx - 1] > 0 ? 1 : 0)
						 + (hasB && intra_chroma_table[mb_idx - mb_width] > 0 ? 1 : 0);
		if (CABAC_DecodeBin(cabac, ctx_c) == 0) {
			intra_chroma_mode = 0;
		} else {
			intra_chroma_mode = CABAC_DecodeBin(cabac, 67) ? (CABAC_DecodeBin(cabac, 67) ? 3 : 2) : 1;
		}
		intra_chroma_table[mb_idx] = (uint8)intra_chroma_mode;

		// mb_qp_delta is present for I_16x16 even with a zero coded block pattern.
		int ctx_qp = 60 + (prev_mb_has_qp_delta ? 1 : 0);
		if (CABAC_DecodeBin(cabac, ctx_qp)) {
			int val = 1;
			while (val < 64 && CABAC_DecodeBin(cabac, (val == 1 ? 62 : 63))) val++;
			int delta = (val & 1) ? ((val + 1) >> 1) : -((val + 1) >> 1);
			current_qp = (current_qp + delta + 52) % 52;
			prev_mb_has_qp_delta = true;
		} else {
			prev_mb_has_qp_delta = false;
		}
	}

	if (a.is_intra16) {
		// ---- I_16x16: one luma DC block, sixteen luma AC blocks -------------------------
		int16 luma_dc[16] = { 0 };
		stdsint scaled_luma_dc[16] = { 0 };
		// The luma DC coded_block_flag of an I_16x16 macroblock is always present, even when
		// CodedBlockPatternLuma is zero.
		int condA_dc = hasA ? (nz_dc_table[(mb_idx - 1) * 3 + 0] ? 1 : 0) : 1;
		int condB_dc = hasB ? (nz_dc_table[(mb_idx - mb_width) * 3 + 0] ? 1 : 0) : 1;
		if (CABAC_DecodeBin(cabac, 85 + condA_dc + 2 * condB_dc)) {
			CABAC_DecodeResidual(cabac, 0, 16, luma_dc);
			nz_dc_table[mb_idx * 3 + 0] = 1;
		}
		stdsint dc_in[16];
		for (int i = 0; i < 16; ++i) dc_in[i] = (stdsint)luma_dc[i];
		Hadamard_H264_4x4(dc_in, current_qp, scaled_luma_dc);

		int32 residuals[16][16];
		MemSet(residuals, 0, sizeof(residuals));
		for (int k = 0; k < 16; ++k) {
			int16 ac_coeffs[16] = { 0 };
			if (cbp_luma > 0) {
				int condA_ac = GetLuma4x4CondA(nnz_table, mb_idx, mb_width, k, hasA);
				int condB_ac = GetLuma4x4CondB(nnz_table, mb_idx, mb_width, k, hasB);
				// I_16x16 luma AC uses context base 89; base 85 belongs to the DC block.
				if (CABAC_DecodeBin(cabac, 89 + condA_ac + 2 * condB_ac)) {
					CABAC_DecodeResidual(cabac, 1, 15, ac_coeffs);
					nnz_table[mb_idx * 24 + k] = 1;
				}
			}
			stdsint scaled_block[16] = { 0 };
			Dequant_H264_4x4(ac_coeffs, current_qp, true, scaled_block);
			scaled_block[0] = scaled_luma_dc[(k & 9) | ((k & 2) << 1) | ((k & 4) >> 1)];    // Figure 8-6: dcY_ij belongs to the block at ( col j, row i )
			stdsint res_s[16];
			IDCT_H264_4x4(scaled_block, res_s);
			for (int i = 0; i < 16; ++i) residuals[k][i] = (int32)res_s[i];
		}

		uint8 top_16[16], left_16[16], tl_16 = 128;
		for (int i = 0; i < 16; ++i) { top_16[i] = 128; left_16[i] = 128; }
		if (hasB) {
			const uint8* tptr = mb_y_dst - dst_y_stride;
			for (int x = 0; x < 16; ++x) top_16[x] = tptr[x];
		}
		if (hasA) {
			const uint8* lptr = mb_y_dst - 1;
			for (int y = 0; y < 16; ++y) left_16[y] = lptr[y * dst_y_stride];
		}
		if (hasA && hasB) tl_16 = *(mb_y_dst - dst_y_stride - 1);

		uint8 pred_16[256];
		H264_PredIntra16x16(pred_16, 16, top_16, left_16, tl_16, hasB, hasA, hasA && hasB, intra16_mode);

		for (int k = 0; k < 16; ++k) {
			int bx = ((k & 1) ? 1 : 0) + ((k & 4) ? 2 : 0);
			int by = ((k & 2) ? 1 : 0) + ((k & 8) ? 2 : 0);
			for (int y = 0; y < 4; ++y) {
				for (int x = 0; x < 4; ++x) {
					int recon = (int)pred_16[(by * 4 + y) * 16 + (bx * 4 + x)] + residuals[k][y * 4 + x];
					mb_y_dst[(by * 4 + y) * dst_y_stride + (bx * 4 + x)] = Clip8(recon);
				}
			}
		}
	} else if (is_8x8_dct) {
		// ---- I_NxN with transform_size_8x8_flag: four 8x8 luma blocks (ctxBlockCat 5) -----
		for (int k8 = 0; k8 < 4; ++k8) {
			int bx8 = (k8 & 1) * 8;
			int by8 = ((k8 >> 1) & 1) * 8;
			int16 coeffs_8x8[64] = { 0 };
			int32 scaled_8x8[64] = { 0 };
			int32 res_8x8[64] = { 0 };

			if (cbp_luma & (1 << k8)) {
				// In 4:2:0 there is no coded_block_flag for the 8x8 blocks: the coefficients
				// follow the transform_size_8x8_flag directly.
				CABAC_DecodeResidual(cabac, 5, 64, coeffs_8x8);
				for (int sub = 0; sub < 4; ++sub) nnz_table[mb_idx * 24 + k8 * 4 + sub] = 1;
				H264_Dequant8x8(coeffs_8x8, current_qp, scaled_8x8);
				H264_IDCT8x8(scaled_8x8, res_8x8);
			}

			uint8* blk_dst = mb_y_dst + by8 * dst_y_stride + bx8;
			uint8 top_8[8] = { 0 }, left_8[8] = { 0 }, tr_8[8] = { 0 };
			uint8 tl_8 = 128;
			bool b_has_top = (by8 > 0) || hasB;
			bool b_has_left = (bx8 > 0) || hasA;
			bool b_has_tl = b_has_top && b_has_left;
			bool b_has_tr = false;

			if (b_has_top) {
				const uint8* tptr = blk_dst - dst_y_stride;
				for (int x = 0; x < 8; ++x) top_8[x] = tptr[x];
			}
			if (b_has_left) {
				const uint8* lptr = blk_dst - 1;
				for (int y = 0; y < 8; ++y) left_8[y] = lptr[y * dst_y_stride];
			}
			if (b_has_tl) tl_8 = *(blk_dst - dst_y_stride - 1);

			if (by8 == 0) {
				if (bx8 == 0 && hasB) {
					const uint8* tptr = blk_dst - dst_y_stride + 8;
					for (int x = 0; x < 8; ++x) tr_8[x] = tptr[x];
					b_has_tr = true;
				} else if (bx8 == 8 && hasB && mb_x + 1 < mb_width) {
					const uint8* tptr = blk_dst - dst_y_stride + 8;
					for (int x = 0; x < 8; ++x) tr_8[x] = tptr[x];
					b_has_tr = true;
				}
			} else if (bx8 == 0) {
				const uint8* tptr = blk_dst - dst_y_stride + 8;
				for (int x = 0; x < 8; ++x) tr_8[x] = tptr[x];
				b_has_tr = true;
			}

			uint8 pred_8[64];
			H264_PredIntra8x8(pred_8, 8, top_8, left_8, tl_8, tr_8, b_has_top, b_has_left, b_has_tl, b_has_tr,
							  mode_table[mb_idx * 16 + k8 * 4]);

			for (int y = 0; y < 8; ++y) {
				for (int x = 0; x < 8; ++x) {
					int recon = (int)pred_8[y * 8 + x] + res_8x8[y * 8 + x];
					blk_dst[y * dst_y_stride + x] = Clip8(recon);
				}
			}
		}
	} else {
		// ---- I_NxN with a 4x4 transform: sixteen luma blocks (ctxBlockCat 2) -------------
		for (int k = 0; k < 16; ++k) {
			int bx = ((k & 1) ? 1 : 0) + ((k & 4) ? 2 : 0);
			int by = ((k & 2) ? 1 : 0) + ((k & 8) ? 2 : 0);
			int blk_8x8 = (by / 2) * 2 + (bx / 2);

			int16 coeffs[16] = { 0 };
			int32 scaled[16] = { 0 };
			int32 residual[16] = { 0 };

			if (cbp_luma & (1 << blk_8x8)) {
				int condA_f = GetLuma4x4CondA(nnz_table, mb_idx, mb_width, k, hasA);
				int condB_f = GetLuma4x4CondB(nnz_table, mb_idx, mb_width, k, hasB);
				// Luma 4x4 (ctxBlockCat 2) uses coded_block_flag context base 93.
				if (CABAC_DecodeBin(cabac, 93 + condA_f + 2 * condB_f)) {
					CABAC_DecodeResidual(cabac, 2, 16, coeffs);
					nnz_table[mb_idx * 24 + k] = 1;
					H264_Dequant4x4(coeffs, current_qp, false, scaled);
					H264_IDCT4x4(scaled, residual);
				}
			}

			uint8* blk_dst = mb_y_dst + by * 4 * dst_y_stride + bx * 4;
			uint8 top_buf[4], left_buf[4], tr_buf[4] = { 0 };
			uint8 tl_val = 128;
			bool blk_has_top = (by > 0) || hasB;
			bool blk_has_left = (bx > 0) || hasA;
			bool blk_has_tl = false;
			bool blk_has_tr = false;

			if (blk_has_top) {
				const uint8* tptr = blk_dst - dst_y_stride;
				for (int x = 0; x < 4; ++x) top_buf[x] = tptr[x];
			}
			if (blk_has_left) {
				const uint8* lptr = blk_dst - 1;
				for (int y = 0; y < 4; ++y) left_buf[y] = lptr[y * dst_y_stride];
			}
			if (by > 0 && bx > 0) {
				tl_val = *(blk_dst - dst_y_stride - 1);
				blk_has_tl = true;
			} else if (by > 0 && bx == 0 && hasA) {
				tl_val = *(blk_dst - dst_y_stride - 1);
				blk_has_tl = true;
			} else if (by == 0 && bx > 0 && hasB) {
				tl_val = *(blk_dst - dst_y_stride - 1);
				blk_has_tl = true;
			} else if (by == 0 && bx == 0 && hasA && hasB) {
				tl_val = *(blk_dst - dst_y_stride - 1);
				blk_has_tl = true;
			}

			bool tr_avail = false;
			if (by == 0) {
				if (hasB && (bx < 3 || mb_x + 1 < mb_width)) tr_avail = true;
			} else {
				if (k == 2 || k == 6 || k == 8 || k == 9 || k == 10 || k == 12 || k == 14) tr_avail = true;
			}
			if (tr_avail) {
				const uint8* tr_ptr = blk_dst - dst_y_stride + 4;
				for (int x = 0; x < 4; ++x) tr_buf[x] = tr_ptr[x];
				blk_has_tr = true;
			}

			uint8 pred_buf[16];
			H264_PredIntra4x4(pred_buf, 4, top_buf, left_buf, tl_val, tr_buf,
							  blk_has_top, blk_has_left, blk_has_tl, blk_has_tr, mode_table[mb_idx * 16 + k]);

			for (int y = 0; y < 4; ++y) {
				for (int x = 0; x < 4; ++x) {
					int recon = (int)pred_buf[y * 4 + x] + residual[y * 4 + x];
					blk_dst[y * dst_y_stride + x] = Clip8(recon);
				}
			}
		}
	}

	// ---- chroma: both DC blocks first, then both planes' AC blocks ----------------------
	uint8 qp_c = H264_GetChromaQP(current_qp, pps->chroma_qp_index_offset);
	int32 scaled_dc_plane[2][4];
	int32 residuals_c_plane[2][4][16];
	MemSet(scaled_dc_plane, 0, sizeof(scaled_dc_plane));
	MemSet(residuals_c_plane, 0, sizeof(residuals_c_plane));

	if (cbp_chroma > 0) {
		for (int plane = 0; plane < 2; ++plane) {
			int16 dc_coeffs[4] = { 0 };
			int condA_dc = hasA ? (nz_dc_table[(mb_idx - 1) * 3 + 1 + plane] ? 1 : 0) : 1;
			int condB_dc = hasB ? (nz_dc_table[(mb_idx - mb_width) * 3 + 1 + plane] ? 1 : 0) : 1;
			if (CABAC_DecodeBin(cabac, 97 + condA_dc + 2 * condB_dc)) {
				CABAC_DecodeResidual(cabac, 3, 4, dc_coeffs);
				nz_dc_table[mb_idx * 3 + 1 + plane] = 1;
			}
			stdsint dc_in[4] = { dc_coeffs[0], dc_coeffs[1], dc_coeffs[2], dc_coeffs[3] };
			stdsint dc_out[4] = { 0 };
			Hadamard_H264_2x2(dc_in, qp_c, dc_out);
			for (int i = 0; i < 4; ++i) scaled_dc_plane[plane][i] = (int32)dc_out[i];
		}
	}
	if (cbp_chroma == 2) {
		for (int plane = 0; plane < 2; ++plane) {
			int nnz_offset = 16 + plane * 4;
			for (int ck = 0; ck < 4; ++ck) {
				int16 ac_coeffs[16] = { 0 };
				int condA_ac = GetChromaACCondA(nnz_table, mb_idx, mb_width, plane, ck, hasA);
				int condB_ac = GetChromaACCondB(nnz_table, mb_idx, mb_width, plane, ck, hasB);
				if (CABAC_DecodeBin(cabac, 101 + condA_ac + 2 * condB_ac)) {
					CABAC_DecodeResidual(cabac, 4, 15, ac_coeffs);
					nnz_table[mb_idx * 24 + nnz_offset + ck] = 1;
				}
				stdsint scaled_block[16] = { 0 };
				Dequant_H264_4x4(ac_coeffs, qp_c, true, scaled_block);
				scaled_block[0] = scaled_dc_plane[plane][ck];
				stdsint res_s[16];
				IDCT_H264_4x4(scaled_block, res_s);
				for (int i = 0; i < 16; ++i) residuals_c_plane[plane][ck][i] = (int32)res_s[i];
			}
		}
	} else if (cbp_chroma == 1) {
		for (int plane = 0; plane < 2; ++plane) {
			for (int ck = 0; ck < 4; ++ck) {
				stdsint scaled_block[16] = { 0 };
				scaled_block[0] = scaled_dc_plane[plane][ck];
				stdsint res_s[16];
				IDCT_H264_4x4(scaled_block, res_s);
				for (int i = 0; i < 16; ++i) residuals_c_plane[plane][ck][i] = (int32)res_s[i];
			}
		}
	}

	for (int plane = 0; plane < 2; ++plane) {
		uint8* ch_dst = (plane == 0) ? mb_u_dst : mb_v_dst;
		uint8 top_c[8] = { 128, 128, 128, 128, 128, 128, 128, 128 }, left_c[8] = { 128, 128, 128, 128, 128, 128, 128, 128 }, tl_c = 128;
		if (hasB) {
			const uint8* tptr = ch_dst - dst_uv_stride;
			for (int x = 0; x < 8; ++x) top_c[x] = tptr[x];
		}
		if (hasA) {
			const uint8* lptr = ch_dst - 1;
			for (int y = 0; y < 8; ++y) left_c[y] = lptr[y * dst_uv_stride];
		}
		if (hasA && hasB) tl_c = *(ch_dst - dst_uv_stride - 1);

		uint8 pred_c[64];
		H264_PredIntraChroma8x8(pred_c, 8, top_c, left_c, tl_c, hasB, hasA, hasA && hasB, intra_chroma_mode);

		for (int ck = 0; ck < 4; ++ck) {
			int cbx = (ck & 1) * 4;
			int cby = ((ck >> 1) & 1) * 4;
			for (int y = 0; y < 4; ++y) {
				for (int x = 0; x < 4; ++x) {
					int recon = (int)pred_c[(cby + y) * 8 + (cbx + x)] + residuals_c_plane[plane][ck][y * 4 + x];
					ch_dst[(cby + y) * dst_uv_stride + (cbx + x)] = Clip8(recon);
				}
			}
		}
	}

	if (mb_qp_out) mb_qp_out[mb_idx] = (uint8)current_qp;
	*a.qp = current_qp;
	*a.prev_mb_has_qp_delta = prev_mb_has_qp_delta;
}

bool H264_DecodeSlice_CABAC_I(
	const byte* rbsp_data,
	size_t rbsp_len,
	const H264SPS* sps,
	const H264PPS* pps,
	uint8 nal_unit_type,
	uint8 nal_ref_idc,
	uint8* dst_y,
	uint8* dst_u,
	uint8* dst_v,
	int dst_y_stride,
	int dst_uv_stride,
	uint8* mb_qp_out,
	// Deblocking export (H.264 8.7.2.1): every macroblock of an I slice is intra, so only the
	// intra flag and transform_size_8x8_flag are exported; the motion data is left untouched.
	uint8* db_flags_out
) {
	if (!rbsp_data || rbsp_len == 0 || !sps || !pps || !dst_y || !dst_u || !dst_v) return false;
	// A slice that has no weighted prediction of its own must not inherit the weights the previous
	// slice selected, and a B slice never scales its two predictions here: it uses the implicit
	// weights computed below instead.
	H264_WP_IDENTITY();

	H264BitReader br;
	H264_InitBitReader(&br, rbsp_data, rbsp_len);

	H264SliceHeader sh;
	if (!H264_ParseSliceHeader(&br, sps, pps, nal_unit_type, nal_ref_idc, &sh)) {
		return false;
	}

	uint32 mb_width = sps->mb_width;
	uint32 mb_height = sps->mb_height;
	uint32 num_mbs = mb_width * mb_height;
	if (sh.first_mb_in_slice >= num_mbs) return false;

	uint8* cbp_table = (uint8*)malloc(num_mbs);
	uint8* cbp_chroma_table = (uint8*)malloc(num_mbs);
	int8* mode_table = (int8*)malloc(num_mbs * 16);
	uint8* intra_chroma_table = (uint8*)malloc(num_mbs);
	uint8* mb_type_table = (uint8*)malloc(num_mbs);
	uint8* nnz_table = (uint8*)malloc(num_mbs * 24);
	uint8* transform_8x8_table = (uint8*)malloc(num_mbs);
	// Per-macroblock coded_block_flag of the DC blocks: 0 = luma DC, 1 = Cb DC, 2 = Cr DC.
	// Required for the coded_block_flag contexts of the luma (base 85) and chroma (base 97)
	// DC blocks, which are read before the DC coefficients themselves.
	uint8* nz_dc_table = (uint8*)malloc(num_mbs * 3);

	if (!cbp_table || !cbp_chroma_table || !mode_table || !intra_chroma_table ||
		!mb_type_table || !nnz_table || !transform_8x8_table || !nz_dc_table) {
		if (cbp_table) free(cbp_table);
		if (cbp_chroma_table) free(cbp_chroma_table);
		if (mode_table) free(mode_table);
		if (intra_chroma_table) free(intra_chroma_table);
		if (mb_type_table) free(mb_type_table);
		if (nnz_table) free(nnz_table);
		if (transform_8x8_table) free(transform_8x8_table);
		if (nz_dc_table) free(nz_dc_table);
		return false;
	}

	MemSet(cbp_table, 0, num_mbs);
	MemSet(cbp_chroma_table, 0, num_mbs);
	MemSet(mode_table, -1, num_mbs * 16);
	MemSet(intra_chroma_table, 0, num_mbs);
	MemSet(mb_type_table, 0, num_mbs);
	MemSet(nnz_table, 0, num_mbs * 24);
	MemSet(transform_8x8_table, 0, num_mbs);
	MemSet(nz_dc_table, 0, num_mbs * 3);

	size_t bit_pos = (((br.byte_pos * 8) - br.bits_left) + 7) & ~(size_t)7;
	CABACDecoder cabac;
	CABAC_Init(&cabac, rbsp_data, rbsp_len, bit_pos, 2, sh.cabac_init_idc, sh.slice_qp);
	H264Probe_Reset((int)H264_SLICE_I, rbsp_len * 8);

	int current_qp = sh.slice_qp;
	bool prev_mb_has_qp_delta = false;
	int terminated = 0;

	for (uint32 mb_idx = sh.first_mb_in_slice; mb_idx < num_mbs; ++mb_idx) {
		if (cabac.overrun) break; // the slice data ended early: stop instead of reading garbage
		H264Probe_MB(mb_idx);
		size_t mb_bit_before = cabac.bit_pos;
		uint32 mb_x = mb_idx % mb_width;
		uint32 mb_y = mb_idx / mb_width;
		bool hasA = (mb_x > 0);
		bool hasB = (mb_y > 0);

		uint8* mb_y_dst = dst_y + mb_y * 16 * dst_y_stride + mb_x * 16;
		uint8* mb_u_dst = dst_u + mb_y * 8 * dst_uv_stride + mb_x * 8;
		uint8* mb_v_dst = dst_v + mb_y * 8 * dst_uv_stride + mb_x * 8;

		int ctx_type = 3 + (hasA && mb_type_table[mb_idx - 1] != 0 ? 1 : 0) + (hasB && mb_type_table[mb_idx - mb_width] != 0 ? 1 : 0);
		int bin0 = CABAC_DecodeBin(&cabac, ctx_type);

		int mb_type = 0;
		int cbp_luma = 0;
		int cbp_chroma = 0;
		int intra_chroma_mode = 0;
		int intra16_mode = 0;
		bool is_8x8_dct = false;

		if (bin0 == 0) {
			// I_NxN: bin 0 selects I_4x4 / I_8x8, and bin 1 selects I_16x16 or I_PCM.
			mb_type = 0;
			mb_type_table[mb_idx] = 0;

			if (pps->transform_8x8_mode_flag) {
				int ctx_8x8 = 399 + (hasA && transform_8x8_table[mb_idx - 1] ? 1 : 0) + (hasB && transform_8x8_table[mb_idx - mb_width] ? 1 : 0);
				is_8x8_dct = (CABAC_DecodeBin(&cabac, ctx_8x8) != 0);
				transform_8x8_table[mb_idx] = is_8x8_dct ? 1 : 0;
			}

			int8 sub_modes[16];
			if (is_8x8_dct) {
				for (int k8 = 0; k8 < 4; ++k8) {
					int mode_A = 2;
					if (k8 == 0) {
						if (hasA) {
							int8 mA = mode_table[(mb_idx - 1) * 16 + 1 * 4 + 1];
							if (mA >= 0) mode_A = (int)mA;
						}
					} else if (k8 == 1) {
						int8 mA = mode_table[mb_idx * 16 + 0 * 4 + 1];
						if (mA >= 0) mode_A = (int)mA;
					} else if (k8 == 2) {
						if (hasA) {
							int8 mA = mode_table[(mb_idx - 1) * 16 + 3 * 4 + 1];
							if (mA >= 0) mode_A = (int)mA;
						}
					} else if (k8 == 3) {
						int8 mA = mode_table[mb_idx * 16 + 2 * 4 + 1];
						if (mA >= 0) mode_A = (int)mA;
					}

					int mode_B = 2;
					if (k8 == 0) {
						if (hasB) {
							int8 mB = mode_table[(mb_idx - mb_width) * 16 + 2 * 4 + 2];
							if (mB >= 0) mode_B = (int)mB;
						}
					} else if (k8 == 1) {
						if (hasB) {
							int8 mB = mode_table[(mb_idx - mb_width) * 16 + 3 * 4 + 2];
							if (mB >= 0) mode_B = (int)mB;
						}
					} else if (k8 == 2) {
						int8 mB = mode_table[mb_idx * 16 + 0 * 4 + 2];
						if (mB >= 0) mode_B = (int)mB;
					} else if (k8 == 3) {
						int8 mB = mode_table[mb_idx * 16 + 1 * 4 + 2];
						if (mB >= 0) mode_B = (int)mB;
					}

					if (((k8 == 0 || k8 == 2) && !hasA) || ((k8 == 0 || k8 == 1) && !hasB)) { mode_A = 2; mode_B = 2; } // 8.3.1.1: 任一侧邻居宏块不可用则 A、B 都按 DC 预测
					int mpm = (mode_A < mode_B) ? mode_A : mode_B;
					int prev_flag = CABAC_DecodeBin(&cabac, 68);
					int m = mpm;
					if (!prev_flag) {
						int rem = 0;
						rem += 1 * CABAC_DecodeBin(&cabac, 69);
						rem += 2 * CABAC_DecodeBin(&cabac, 69);
						rem += 4 * CABAC_DecodeBin(&cabac, 69);
						m = rem + (rem >= mpm);
					}
					for (int sub = 0; sub < 4; ++sub) {
						sub_modes[k8 * 4 + sub] = (int8)m;
						mode_table[mb_idx * 16 + k8 * 4 + sub] = (int8)m;
					}
				}
			} else {
				for (int k = 0; k < 16; ++k) {
					int bx = ((k & 1) ? 1 : 0) + ((k & 4) ? 2 : 0);
					int by = ((k & 2) ? 1 : 0) + ((k & 8) ? 2 : 0);

					int mode_A = 2;
					if (bx > 0) {
						int kA = (by & 2) * 4 + ((bx - 1) & 2) * 2 + (by & 1) * 2 + ((bx - 1) & 1);
						int8 mA = mode_table[mb_idx * 16 + kA];
						if (mA >= 0) mode_A = (int)mA;
					} else if (hasA) {
						int kA = (by & 2) * 4 + (3 & 2) * 2 + (by & 1) * 2 + (3 & 1);
						int8 mA = mode_table[(mb_idx - 1) * 16 + kA];
						if (mA >= 0) mode_A = (int)mA;
					}

					int mode_B = 2;
					if (by > 0) {
						int kB = ((by - 1) & 2) * 4 + (bx & 2) * 2 + ((by - 1) & 1) * 2 + (bx & 1);
						int8 mB = mode_table[mb_idx * 16 + kB];
						if (mB >= 0) mode_B = (int)mB;
					} else if (hasB) {
						int kB = (3 & 2) * 4 + (bx & 2) * 2 + (3 & 1) * 2 + (bx & 1);
						int8 mB = mode_table[(mb_idx - mb_width) * 16 + kB];
						if (mB >= 0) mode_B = (int)mB;
					}

					if ((bx == 0 && !hasA) || (by == 0 && !hasB)) { mode_A = 2; mode_B = 2; } // 8.3.1.1: 任一侧邻居宏块不可用则 A、B 都按 DC 预测
					int mpm = (mode_A < mode_B) ? mode_A : mode_B;
					int prev_flag = CABAC_DecodeBin(&cabac, 68);
					int m = mpm;
					if (!prev_flag) {
						int rem = 0;
						rem += 1 * CABAC_DecodeBin(&cabac, 69);
						rem += 2 * CABAC_DecodeBin(&cabac, 69);
						rem += 4 * CABAC_DecodeBin(&cabac, 69);
						m = rem + (rem >= mpm);
					}
					sub_modes[k] = (int8)m;
					mode_table[mb_idx * 16 + k] = (int8)m;
				}
			}

			// intra_chroma_pred_mode
			int ctx_c = (hasA && intra_chroma_table[mb_idx - 1] > 0 ? 1 : 0) + (hasB && intra_chroma_table[mb_idx - mb_width] > 0 ? 1 : 0);
			if (CABAC_DecodeBin(&cabac, 64 + ctx_c) == 0) {
				intra_chroma_mode = 0;
			} else {
				if (CABAC_DecodeBin(&cabac, 67) == 0) {
					intra_chroma_mode = 1;
				} else {
					intra_chroma_mode = CABAC_DecodeBin(&cabac, 67) ? 3 : 2;
				}
			}
			intra_chroma_table[mb_idx] = (uint8)intra_chroma_mode;

			// coded_block_pattern
			// The first bin's context (73 .. 76) has the neighbouring bit INVERTED: condTermFlagN
			// is 1 when the neighbour is available AND the corresponding bit of its
			// coded_block_pattern is CLEAR (0 when the neighbour is unavailable). The internal
			// bits are offset by one.
			cbp_luma = 0;
			int condA_0 = (hasA && ((cbp_table[mb_idx - 1] & 2) == 0)) ? 1 : 0;
			int condB_0 = (hasB && ((cbp_table[mb_idx - mb_width] & 4) == 0)) ? 1 : 0;
			if (CABAC_DecodeBin(&cabac, 73 + condA_0 + 2 * condB_0)) cbp_luma |= 1;

			int condA_1 = ((cbp_luma & 1) == 0) ? 1 : 0;
			int condB_1 = (hasB && ((cbp_table[mb_idx - mb_width] & 8) == 0)) ? 1 : 0;
			if (CABAC_DecodeBin(&cabac, 73 + condA_1 + 2 * condB_1)) cbp_luma |= 2;

			int condA_2 = (hasA && ((cbp_table[mb_idx - 1] & 8) == 0)) ? 1 : 0;
			int condB_2 = ((cbp_luma & 1) == 0) ? 1 : 0;
			if (CABAC_DecodeBin(&cabac, 73 + condA_2 + 2 * condB_2)) cbp_luma |= 4;

			int condA_3 = ((cbp_luma & 4) == 0) ? 1 : 0;
			int condB_3 = ((cbp_luma & 2) == 0) ? 1 : 0;
			if (CABAC_DecodeBin(&cabac, 73 + condA_3 + 2 * condB_3)) cbp_luma |= 8;

			int cbp_ca = hasA ? cbp_chroma_table[mb_idx - 1] : 0;
			int cbp_cb = hasB ? cbp_chroma_table[mb_idx - mb_width] : 0;
			int ctx_c0 = (cbp_ca > 0 ? 1 : 0) + (cbp_cb > 0 ? 2 : 0);
			if (CABAC_DecodeBin(&cabac, 77 + ctx_c0) == 0) {
				cbp_chroma = 0;
			} else {
				int ctx_c1 = 4 + (cbp_ca == 2 ? 1 : 0) + (cbp_cb == 2 ? 2 : 0);
				cbp_chroma = 1 + CABAC_DecodeBin(&cabac, 77 + ctx_c1);
			}
			cbp_table[mb_idx] = (uint8)cbp_luma;
			cbp_chroma_table[mb_idx] = (uint8)cbp_chroma;

			uint8 cbp = (uint8)(cbp_luma | (cbp_chroma << 4));
			if (cbp > 0) {
				int ctx_qp = 60 + (prev_mb_has_qp_delta ? 1 : 0);
				if (CABAC_DecodeBin(&cabac, ctx_qp)) {
					int val = 1;
					while (val < 64 && CABAC_DecodeBin(&cabac, (val == 1 ? 62 : 63))) val++;
					int delta = (val & 1) ? ((val + 1) >> 1) : -((val + 1) >> 1);
					current_qp = (current_qp + delta + 52) % 52;
					prev_mb_has_qp_delta = true;
				} else {
					prev_mb_has_qp_delta = false;
				}
			} else {
				prev_mb_has_qp_delta = false;
			}

			if (is_8x8_dct) {
				for (int k8 = 0; k8 < 4; ++k8) {
					int bx8 = (k8 & 1) * 8;
					int by8 = ((k8 >> 1) & 1) * 8;
					int16 coeffs_8x8[64] = { 0 };
					int32 scaled_8x8[64] = { 0 };
					int32 res_8x8[64] = { 0 };

					if (cbp_luma & (1 << k8)) {
						int condA_f = 0, condB_f = 0;
						if (k8 == 0) {
							condA_f = (hasA && (nnz_table[(mb_idx - 1) * 24 + 1 * 4 + 1] > 0 || nnz_table[(mb_idx - 1) * 24 + 1 * 4 + 3] > 0)) ? 1 : 0;
							condB_f = (hasB && (nnz_table[(mb_idx - mb_width) * 24 + 2 * 4 + 2] > 0 || nnz_table[(mb_idx - mb_width) * 24 + 2 * 4 + 3] > 0)) ? 1 : 0;
						} else if (k8 == 1) {
							condA_f = (nnz_table[mb_idx * 24 + 0 * 4 + 1] > 0 || nnz_table[mb_idx * 24 + 0 * 4 + 3] > 0) ? 1 : 0;
							condB_f = (hasB && (nnz_table[(mb_idx - mb_width) * 24 + 3 * 4 + 2] > 0 || nnz_table[(mb_idx - mb_width) * 24 + 3 * 4 + 3] > 0)) ? 1 : 0;
						} else if (k8 == 2) {
							condA_f = (hasA && (nnz_table[(mb_idx - 1) * 24 + 3 * 4 + 1] > 0 || nnz_table[(mb_idx - 1) * 24 + 3 * 4 + 3] > 0)) ? 1 : 0;
							condB_f = (nnz_table[mb_idx * 24 + 0 * 4 + 2] > 0 || nnz_table[mb_idx * 24 + 0 * 4 + 3] > 0) ? 1 : 0;
						} else if (k8 == 3) {
							condA_f = (nnz_table[mb_idx * 24 + 2 * 4 + 1] > 0 || nnz_table[mb_idx * 24 + 2 * 4 + 3] > 0) ? 1 : 0;
							condB_f = (nnz_table[mb_idx * 24 + 1 * 4 + 2] > 0 || nnz_table[mb_idx * 24 + 1 * 4 + 3] > 0) ? 1 : 0;
						}

						int ctx_f = 1012 + condA_f + 2 * condB_f;
						(void)ctx_f;
						// In 4:2:0 there is no coded_block_flag for the 8x8 blocks: the coefficients
						// follow the transform_size_8x8_flag directly. Reading a bin here would
						// consume one that belongs to the next syntax element and desynchronise
						// the whole parse.
						{
							CABAC_DecodeResidual(&cabac, 5, 64, coeffs_8x8);
							for (int sub = 0; sub < 4; ++sub) {
								nnz_table[mb_idx * 24 + k8 * 4 + sub] = 1;
							}
							H264_Dequant8x8(coeffs_8x8, current_qp, scaled_8x8);
							H264_IDCT8x8(scaled_8x8, res_8x8);
						}
					}

					uint8* blk_dst = mb_y_dst + by8 * dst_y_stride + bx8;
					uint8 top_8[8] = { 0 }, left_8[8] = { 0 }, tr_8[8] = { 0 };
					uint8 tl_8 = 128;
					bool b_has_top = (by8 > 0) || hasB;
					bool b_has_left = (bx8 > 0) || hasA;
					bool b_has_tl = b_has_top && b_has_left;
					bool b_has_tr = false;

					if (b_has_top) {
						const uint8* tptr = blk_dst - dst_y_stride;
						for (int x = 0; x < 8; ++x) top_8[x] = tptr[x];
					}
					if (b_has_left) {
						const uint8* lptr = blk_dst - 1;
						for (int y = 0; y < 8; ++y) left_8[y] = lptr[y * dst_y_stride];
					}
					if (b_has_tl) {
						tl_8 = *(blk_dst - dst_y_stride - 1);
					}

					if (by8 == 0) {
						if (bx8 == 0 && hasB) {
							const uint8* tptr = blk_dst - dst_y_stride + 8;
							for (int x = 0; x < 8; ++x) tr_8[x] = tptr[x];
							b_has_tr = true;
						} else if (bx8 == 8 && hasB && mb_x + 1 < mb_width) {
							const uint8* tptr = blk_dst - dst_y_stride + 8;
							for (int x = 0; x < 8; ++x) tr_8[x] = tptr[x];
							b_has_tr = true;
						}
					} else {
						if (bx8 == 0) {
							const uint8* tptr = blk_dst - dst_y_stride + 8;
							for (int x = 0; x < 8; ++x) tr_8[x] = tptr[x];
							b_has_tr = true;
						}
					}

					uint8 pred_8[64];
					H264_PredIntra8x8(pred_8, 8, top_8, left_8, tl_8, tr_8, b_has_top, b_has_left, b_has_tl, b_has_tr, sub_modes[k8 * 4]);

					for (int y = 0; y < 8; ++y) {
						for (int x = 0; x < 8; ++x) {
							int recon = (int)pred_8[y * 8 + x] + res_8x8[y * 8 + x];
							blk_dst[y * dst_y_stride + x] = Clip8(recon);
						}
					}
				}
			} else {
				for (int k = 0; k < 16; ++k) {
					int bx = ((k & 1) ? 1 : 0) + ((k & 4) ? 2 : 0);
					int by = ((k & 2) ? 1 : 0) + ((k & 8) ? 2 : 0);
					int blk_8x8 = (by / 2) * 2 + (bx / 2);

					int16 coeffs[16] = { 0 };
					int32 scaled[16] = { 0 };
					int32 residual[16] = { 0 };

					if (cbp_luma & (1 << blk_8x8)) {
						int condA_f = GetLuma4x4CondA(nnz_table, mb_idx, mb_width, k, hasA);
						int condB_f = GetLuma4x4CondB(nnz_table, mb_idx, mb_width, k, hasB);
						// Luma 4x4 (ctxBlockCat 2) uses coded_block_flag context base 93;
						// base 89 belongs to the I_16x16 luma AC blocks.
						int ctx_f = 93 + condA_f + 2 * condB_f;
						if (CABAC_DecodeBin(&cabac, ctx_f)) {
							CABAC_DecodeResidual(&cabac, 2, 16, coeffs);
							nnz_table[mb_idx * 24 + k] = 1;
							H264_Dequant4x4(coeffs, current_qp, false, scaled);
							H264_IDCT4x4(scaled, residual);
						}
					}

					uint8* blk_dst = mb_y_dst + by * 4 * dst_y_stride + bx * 4;
					uint8 top_buf[4], left_buf[4], tr_buf[4] = { 0 };
					uint8 tl_val = 128;
					bool blk_has_top = (by > 0) || hasB;
					bool blk_has_left = (bx > 0) || hasA;
					bool blk_has_tl = false;
					bool blk_has_tr = false;

					if (blk_has_top) {
						const uint8* tptr = blk_dst - dst_y_stride;
						for (int x = 0; x < 4; ++x) top_buf[x] = tptr[x];
					}
					if (blk_has_left) {
						const uint8* lptr = blk_dst - 1;
						for (int y = 0; y < 4; ++y) left_buf[y] = lptr[y * dst_y_stride];
					}
					if (by > 0 && bx > 0) {
						tl_val = *(blk_dst - dst_y_stride - 1);
						blk_has_tl = true;
					} else if (by > 0 && bx == 0 && hasA) {
						tl_val = *(blk_dst - dst_y_stride - 1);
						blk_has_tl = true;
					} else if (by == 0 && bx > 0 && hasB) {
						tl_val = *(blk_dst - dst_y_stride - 1);
						blk_has_tl = true;
					} else if (by == 0 && bx == 0 && hasA && hasB) {
						tl_val = *(blk_dst - dst_y_stride - 1);
						blk_has_tl = true;
					}

					bool tr_avail = false;
					if (by == 0) {
						if (hasB && (bx < 3 || mb_x + 1 < mb_width)) tr_avail = true;
					} else {
						if (k == 2 || k == 6 || k == 8 || k == 9 || k == 10 || k == 12 || k == 14) tr_avail = true;
					}
					if (tr_avail) {
						const uint8* tr_ptr = blk_dst - dst_y_stride + 4;
						for (int x = 0; x < 4; ++x) tr_buf[x] = tr_ptr[x];
						blk_has_tr = true;
					}

					uint8 pred_buf[16];
					H264_PredIntra4x4(pred_buf, 4, top_buf, left_buf, tl_val, tr_buf,
									  blk_has_top, blk_has_left, blk_has_tl, blk_has_tr, sub_modes[k]);

					for (int y = 0; y < 4; ++y) {
						for (int x = 0; x < 4; ++x) {
							int recon = (int)pred_buf[y * 4 + x] + residual[y * 4 + x];
							blk_dst[y * dst_y_stride + x] = Clip8(recon);
						}
					}
				}
			}

			// Chroma residual: both DC blocks are coded first, then both planes' AC blocks.
			// Reading one whole plane before the other desynchronises the bin stream whenever
			// cbp_chroma == 2.
			uint8 qp_c = H264_GetChromaQP(current_qp, pps->chroma_qp_index_offset);
			int32 scaled_dc_plane[2][4];
			int32 residuals_c_plane[2][4][16];
			MemSet(scaled_dc_plane, 0, sizeof(scaled_dc_plane));
			MemSet(residuals_c_plane, 0, sizeof(residuals_c_plane));

			if (cbp_chroma > 0) {
				for (int plane = 0; plane < 2; ++plane) {
					int16 dc_coeffs[4] = { 0 };
					int condA_dc = hasA ? (nz_dc_table[(mb_idx - 1) * 3 + 1 + plane] ? 1 : 0) : 1;
					int condB_dc = hasB ? (nz_dc_table[(mb_idx - mb_width) * 3 + 1 + plane] ? 1 : 0) : 1;
					if (CABAC_DecodeBin(&cabac, 97 + condA_dc + 2 * condB_dc)) {
						CABAC_DecodeResidual(&cabac, 3, 4, dc_coeffs);
						nz_dc_table[mb_idx * 3 + 1 + plane] = 1;
					}
					stdsint dc_in[4] = { dc_coeffs[0], dc_coeffs[1], dc_coeffs[2], dc_coeffs[3] };
					stdsint dc_out[4] = { 0 };
					Hadamard_H264_2x2(dc_in, qp_c, dc_out);
					for (int i = 0; i < 4; ++i) scaled_dc_plane[plane][i] = (int32)dc_out[i];
				}
			}
			if (cbp_chroma == 2) {
				for (int plane = 0; plane < 2; ++plane) {
					int nnz_offset = 16 + plane * 4;
					for (int ck = 0; ck < 4; ++ck) {
						int16 ac_coeffs[16] = { 0 };
						int condA_ac = GetChromaACCondA(nnz_table, mb_idx, mb_width, plane, ck, hasA);
						int condB_ac = GetChromaACCondB(nnz_table, mb_idx, mb_width, plane, ck, hasB);
						int ctx_ac = 101 + condA_ac + 2 * condB_ac;
						if (CABAC_DecodeBin(&cabac, ctx_ac)) {
							CABAC_DecodeResidual(&cabac, 4, 15, ac_coeffs);
							nnz_table[mb_idx * 24 + nnz_offset + ck] = 1;
						}
						stdsint scaled_block[16] = { 0 };
						Dequant_H264_4x4(ac_coeffs, qp_c, true, scaled_block);
						scaled_block[0] = scaled_dc_plane[plane][ck];
						stdsint res_s[16];
						IDCT_H264_4x4(scaled_block, res_s);
						for (int i = 0; i < 16; ++i) residuals_c_plane[plane][ck][i] = (int32)res_s[i];
					}
				}
			} else if (cbp_chroma == 1) {
				for (int plane = 0; plane < 2; ++plane) {
					for (int ck = 0; ck < 4; ++ck) {
						stdsint scaled_block[16] = { 0 };
						scaled_block[0] = scaled_dc_plane[plane][ck];
						stdsint res_s[16];
						IDCT_H264_4x4(scaled_block, res_s);
						for (int i = 0; i < 16; ++i) residuals_c_plane[plane][ck][i] = (int32)res_s[i];
					}
				}
			}

			for (int plane = 0; plane < 2; ++plane) {
				uint8* ch_dst = (plane == 0) ? mb_u_dst : mb_v_dst;
				uint8 top_c[8] = { 128, 128, 128, 128, 128, 128, 128, 128 }, left_c[8] = { 128, 128, 128, 128, 128, 128, 128, 128 }, tl_c = 128;
				if (hasB) {
					const uint8* tptr = ch_dst - dst_uv_stride;
					for (int x = 0; x < 8; ++x) top_c[x] = tptr[x];
				}
				if (hasA) {
					const uint8* lptr = ch_dst - 1;
					for (int y = 0; y < 8; ++y) left_c[y] = lptr[y * dst_uv_stride];
				}
				if (hasA && hasB) {
					tl_c = *(ch_dst - dst_uv_stride - 1);
				}

				uint8 pred_c[64];
				H264_PredIntraChroma8x8(pred_c, 8, top_c, left_c, tl_c, hasB, hasA, hasA && hasB, intra_chroma_mode);

				for (int ck = 0; ck < 4; ++ck) {
					int cbx = (ck & 1) * 4;
					int cby = ((ck >> 1) & 1) * 4;
					for (int y = 0; y < 4; ++y) {
						for (int x = 0; x < 4; ++x) {
							int recon = (int)pred_c[(cby + y) * 8 + (cbx + x)] + residuals_c_plane[plane][ck][y * 4 + x];
							ch_dst[(cby + y) * dst_uv_stride + (cbx + x)] = Clip8(recon);
						}
					}
				}
			}

		} else {
			// I_16x16
			if (CABAC_DecodeTerminate(&cabac)) {
				mb_type = 25; // I_PCM
			} else {
				int cbp_l = CABAC_DecodeBin(&cabac, 6);
				cbp_luma = cbp_l ? 15 : 0;
				if (CABAC_DecodeBin(&cabac, 7)) {
					cbp_chroma = CABAC_DecodeBin(&cabac, 8) ? 2 : 1;
				} else {
					cbp_chroma = 0;
				}
				int mode_bit1 = CABAC_DecodeBin(&cabac, 9);
				int mode_bit0 = CABAC_DecodeBin(&cabac, 10);
				// The two bins are the Intra16x16PredMode, MSB first (i_pred >> 1, then
				// i_pred & 1). The decoded value is therefore the mode itself, with 0 = Vertical,
				// 1 = Horizontal, 2 = DC and 3 = Plane - see the note in the P-slice I_16x16
				// branch.
				intra16_mode = (mode_bit1 << 1) | mode_bit0;
				mb_type = 1 + intra16_mode + 4 * cbp_chroma + 12 * (cbp_luma ? 1 : 0);
			}
			mb_type_table[mb_idx] = (uint8)mb_type;
			// An I_16x16 macroblock carries its cbp inside mb_type, but the cbp context of a
			// following I_NxN / inter macroblock still reads the neighbour's cbp. Leaving these
			// at 0 makes every cbp context next to an I_16x16 macroblock wrong.
			cbp_table[mb_idx] = (uint8)cbp_luma;
			cbp_chroma_table[mb_idx] = (uint8)cbp_chroma;

			int ctx_c = 64 + (hasA && intra_chroma_table[mb_idx - 1] > 0 ? 1 : 0) + (hasB && intra_chroma_table[mb_idx - mb_width] > 0 ? 1 : 0);
			if (CABAC_DecodeBin(&cabac, ctx_c) == 0) {
				intra_chroma_mode = 0;
			} else {
				if (CABAC_DecodeBin(&cabac, 67) == 0) {
					intra_chroma_mode = 1;
				} else {
					intra_chroma_mode = CABAC_DecodeBin(&cabac, 67) ? 3 : 2;
				}
			}
			intra_chroma_table[mb_idx] = (uint8)intra_chroma_mode;

			int ctx_qp = 60 + (prev_mb_has_qp_delta ? 1 : 0);
			if (CABAC_DecodeBin(&cabac, ctx_qp)) {
				int val = 1;
				// The unary remainder of mb_qp_delta uses a fixed context after its first bin:
				// 60+2 for the second bin and 60+3 for every later one. It does NOT depend on
				// the previous macroblock.
				while (val < 64 && CABAC_DecodeBin(&cabac, (val == 1 ? 62 : 63))) val++;
				int delta = (val & 1) ? ((val + 1) >> 1) : -((val + 1) >> 1);
				current_qp = (current_qp + delta + 52) % 52;
				prev_mb_has_qp_delta = true;
			} else {
				prev_mb_has_qp_delta = false;
			}

			// Luma DC: only present when the luma cbp is not zero, otherwise reading it would
			// consume the bits of the following macroblock and desynchronise the CABAC parse.
			int16 luma_dc[16] = { 0 };
			stdsint scaled_luma_dc[16] = { 0 };
			{
				// The luma DC block of an I_16x16 macroblock ALWAYS carries a coded_block_flag
				// (context base 85), even when CodedBlockPatternLuma is 0, so it is read
				// unconditionally, ahead of the "cbp_luma != 0" guard. Skipping
				// this bin leaves the whole residual parse one bin short and desynchronises the
				// slice from the next coded block onwards.
				int condA_dc = hasA ? (nz_dc_table[(mb_idx - 1) * 3 + 0] ? 1 : 0) : 1;
				int condB_dc = hasB ? (nz_dc_table[(mb_idx - mb_width) * 3 + 0] ? 1 : 0) : 1;
				if (CABAC_DecodeBin(&cabac, 85 + condA_dc + 2 * condB_dc)) {
					CABAC_DecodeResidual(&cabac, 0, 16, luma_dc);
					nz_dc_table[mb_idx * 3 + 0] = 1;
				}
				stdsint dc_in[16];
				for (int i = 0; i < 16; ++i) dc_in[i] = (stdsint)luma_dc[i];
				Hadamard_H264_4x4(dc_in, current_qp, scaled_luma_dc);
			}

			// 16 Luma AC blocks
			int32 residuals[16][16];
			MemSet(residuals, 0, sizeof(residuals));

			for (int k = 0; k < 16; ++k) {
				int16 ac_coeffs[16] = { 0 };
				if (cbp_luma > 0) {
					int condA_ac = GetLuma4x4CondA(nnz_table, mb_idx, mb_width, k, hasA);
					int condB_ac = GetLuma4x4CondB(nnz_table, mb_idx, mb_width, k, hasB);
					// I_16x16 luma AC uses context base 89; base 85 belongs to the DC block.
					int ctx_ac = 89 + condA_ac + 2 * condB_ac;
					if (CABAC_DecodeBin(&cabac, ctx_ac)) {
						CABAC_DecodeResidual(&cabac, 1, 15, ac_coeffs);
						nnz_table[mb_idx * 24 + k] = 1;
					}
				}
				stdsint scaled_block[16] = { 0 };
				Dequant_H264_4x4(ac_coeffs, current_qp, true, scaled_block);
				scaled_block[0] = scaled_luma_dc[(k & 9) | ((k & 2) << 1) | ((k & 4) >> 1)];    // Figure 8-6: dcY_ij belongs to the block at ( col j, row i )
				stdsint res_s[16];
				IDCT_H264_4x4(scaled_block, res_s);
				for (int i = 0; i < 16; ++i) residuals[k][i] = (int32)res_s[i];
			}

			uint8 top_16[16], left_16[16], tl_16 = 128;
			for (int i = 0; i < 16; ++i) { top_16[i] = 128; left_16[i] = 128; }
			if (hasB) {
				const uint8* tptr = mb_y_dst - dst_y_stride;
				for (int x = 0; x < 16; ++x) top_16[x] = tptr[x];
			}
			if (hasA) {
				const uint8* lptr = mb_y_dst - 1;
				for (int y = 0; y < 16; ++y) left_16[y] = lptr[y * dst_y_stride];
			}
			if (hasA && hasB) {
				tl_16 = *(mb_y_dst - dst_y_stride - 1);
			}

			uint8 pred_16[256];
			H264_PredIntra16x16(pred_16, 16, top_16, left_16, tl_16, hasB, hasA, hasA && hasB, intra16_mode);

			for (int k = 0; k < 16; ++k) {
				int bx = ((k & 1) ? 1 : 0) + ((k & 4) ? 2 : 0);
				int by = ((k & 2) ? 1 : 0) + ((k & 8) ? 2 : 0);
				for (int y = 0; y < 4; ++y) {
					for (int x = 0; x < 4; ++x) {
						int recon = (int)pred_16[(by * 4 + y) * 16 + (bx * 4 + x)] + residuals[k][y * 4 + x];
						mb_y_dst[(by * 4 + y) * dst_y_stride + (bx * 4 + x)] = Clip8(recon);
					}
				}
			}

			// Chroma residual: both DC blocks are coded first, then both planes' AC blocks.
			// The chroma DC block has its own coded_block_flag with context base 97, and the
			// chroma AC coded_block_flag uses context base 101 (93 belongs to luma 4x4).
			uint8 qp_c = H264_GetChromaQP(current_qp, pps->chroma_qp_index_offset);
			int32 scaled_dc_plane[2][4];
			int32 residuals_c_plane[2][4][16];
			MemSet(scaled_dc_plane, 0, sizeof(scaled_dc_plane));
			MemSet(residuals_c_plane, 0, sizeof(residuals_c_plane));

			if (cbp_chroma > 0) {
				for (int plane = 0; plane < 2; ++plane) {
					int16 dc_coeffs[4] = { 0 };
					int condA_dc = hasA ? (nz_dc_table[(mb_idx - 1) * 3 + 1 + plane] ? 1 : 0) : 1;
					int condB_dc = hasB ? (nz_dc_table[(mb_idx - mb_width) * 3 + 1 + plane] ? 1 : 0) : 1;
					if (CABAC_DecodeBin(&cabac, 97 + condA_dc + 2 * condB_dc)) {
						CABAC_DecodeResidual(&cabac, 3, 4, dc_coeffs);
						nz_dc_table[mb_idx * 3 + 1 + plane] = 1;
					}
					stdsint dc_in[4] = { dc_coeffs[0], dc_coeffs[1], dc_coeffs[2], dc_coeffs[3] };
					stdsint dc_out[4] = { 0 };
					Hadamard_H264_2x2(dc_in, qp_c, dc_out);
					for (int i = 0; i < 4; ++i) scaled_dc_plane[plane][i] = (int32)dc_out[i];
				}
			}
			if (cbp_chroma == 2) {
				for (int plane = 0; plane < 2; ++plane) {
					int nnz_offset = 16 + plane * 4;
					for (int ck = 0; ck < 4; ++ck) {
						int16 ac_coeffs[16] = { 0 };
						int condA_ac = GetChromaACCondA(nnz_table, mb_idx, mb_width, plane, ck, hasA);
						int condB_ac = GetChromaACCondB(nnz_table, mb_idx, mb_width, plane, ck, hasB);
						int ctx_ac = 101 + condA_ac + 2 * condB_ac;
						if (CABAC_DecodeBin(&cabac, ctx_ac)) {
							CABAC_DecodeResidual(&cabac, 4, 15, ac_coeffs);
							nnz_table[mb_idx * 24 + nnz_offset + ck] = 1;
						}
						stdsint scaled_block[16] = { 0 };
						Dequant_H264_4x4(ac_coeffs, qp_c, true, scaled_block);
						scaled_block[0] = scaled_dc_plane[plane][ck];
						stdsint res_s[16];
						IDCT_H264_4x4(scaled_block, res_s);
						for (int i = 0; i < 16; ++i) residuals_c_plane[plane][ck][i] = (int32)res_s[i];
					}
				}
			} else if (cbp_chroma == 1) {
				for (int plane = 0; plane < 2; ++plane) {
					for (int ck = 0; ck < 4; ++ck) {
						stdsint scaled_block[16] = { 0 };
						scaled_block[0] = scaled_dc_plane[plane][ck];
						stdsint res_s[16];
						IDCT_H264_4x4(scaled_block, res_s);
						for (int i = 0; i < 16; ++i) residuals_c_plane[plane][ck][i] = (int32)res_s[i];
					}
				}
			}

			for (int plane = 0; plane < 2; ++plane) {
				uint8* ch_dst = (plane == 0) ? mb_u_dst : mb_v_dst;
				uint8 top_c[8] = { 128, 128, 128, 128, 128, 128, 128, 128 }, left_c[8] = { 128, 128, 128, 128, 128, 128, 128, 128 }, tl_c = 128;
				if (hasB) {
					const uint8* tptr = ch_dst - dst_uv_stride;
					for (int x = 0; x < 8; ++x) top_c[x] = tptr[x];
				}
				if (hasA) {
					const uint8* lptr = ch_dst - 1;
					for (int y = 0; y < 8; ++y) left_c[y] = lptr[y * dst_uv_stride];
				}
				if (hasA && hasB) {
					tl_c = *(ch_dst - dst_uv_stride - 1);
				}

				uint8 pred_c[64];
				H264_PredIntraChroma8x8(pred_c, 8, top_c, left_c, tl_c, hasB, hasA, hasA && hasB, intra_chroma_mode);

				for (int ck = 0; ck < 4; ++ck) {
					int cbx = (ck & 1) * 4;
					int cby = ((ck >> 1) & 1) * 4;
					for (int y = 0; y < 4; ++y) {
						for (int x = 0; x < 4; ++x) {
							int recon = (int)pred_c[(cby + y) * 8 + (cbx + x)] + residuals_c_plane[plane][ck][y * 4 + x];
							ch_dst[(cby + y) * dst_uv_stride + (cbx + x)] = Clip8(recon);
						}
					}
				}
			}
		}

		H264Probe_Event(mb_idx, mb_bit_before, cabac.bit_pos, mb_type, cbp_luma | (cbp_chroma << 4));

		if (mb_qp_out) mb_qp_out[mb_idx] = (uint8)current_qp;

		if (CABAC_DecodeTerminate(&cabac)) {
			terminated = 1;
			break;
		}
	}

	if (db_flags_out) {
		for (uint32 m = 0; m < num_mbs; ++m) {
			db_flags_out[(size_t)m * 4 + 0] = 1;
			db_flags_out[(size_t)m * 4 + 1] = transform_8x8_table[m];
			db_flags_out[(size_t)m * 4 + 2] = 0;
			db_flags_out[(size_t)m * 4 + 3] = 0;
		}
	}

	free(cbp_table);
	free(cbp_chroma_table);
	free(mode_table);
	free(intra_chroma_table);
	free(mb_type_table);
	free(nnz_table);
	free(transform_8x8_table);
	free(nz_dc_table);
	// the arithmetic coder legitimately reads a few bytes past the slice data (flush and
	// cabac_zero_word padding), so only a gross overrun means the parse is broken
	bool ok = cabac.bit_pos <= rbsp_len * 8 + 512;
	H264Probe_End(ok ? 1 : 0, &cabac, terminated, 0);
	return ok;
}

// The list0 pictures a P slice may predict from, indexed by the reference index that a partition
// codes.  A P slice whose num_ref_idx_l0_active_minus1 is greater than 0 selects its reference
// picture per partition, so motion compensation cannot use a single picture for the whole slice.
struct H264RefPicView {
	const uint8* y;
	const uint8* u;
	const uint8* v;
};

bool H264_DecodeSlice_CABAC_P(
	const byte* rbsp_data,
	size_t rbsp_len,
	const H264SPS* sps,
	const H264PPS* pps,
	uint8 nal_unit_type,
	uint8 nal_ref_idc,
	const uint8* ref_y_def,
	const uint8* ref_u_def,
	const uint8* ref_v_def,
	int ref_y_stride,
	int ref_uv_stride,
	// list0 of this slice in reference index order.  ref_y_def/ref_u_def/ref_v_def above are only
	// the fallback, used when no list is supplied.
	const H264RefPicView* refs0,
	int n_refs0,
	uint8* dst_y,
	uint8* dst_u,
	uint8* dst_v,
	int dst_y_stride,
	int dst_uv_stride,
	uint8* mb_qp_out,
	// Optional export of the per-4x4 motion data of the reconstructed picture. Temporal
	// direct prediction in a later B slice reads the collocated picture's list0 motion
	// vectors and reference indices, so a reference picture has to hand them over.
	H264MotionVector* mv_out,
	uint8* refidx_out,
	// Per-macroblock deblocking export (H.264 8.7.2.1), see H264_DeblockFrame for the layout.
	uint8* db_flags_out,
	int16* db_mv_out,
	int16* db_ref_out
) {
	if (!rbsp_data || rbsp_len == 0 || !sps || !pps || !dst_y || !dst_u || !dst_v || !ref_y_def || !ref_u_def || !ref_v_def) return false;

	// The reference picture the partition being reconstructed predicts from.  Every motion
	// compensation call below reads these three pointers, so a partition only has to select its
	// own reference index before its prediction is built.  Index 0 is the fallback picture.
	const uint8* ref_y = ref_y_def;
	const uint8* ref_u = ref_u_def;
	const uint8* ref_v = ref_v_def;

	H264BitReader br;
	H264_InitBitReader(&br, rbsp_data, rbsp_len);

	H264SliceHeader sh;
	if (!H264_ParseSliceHeader(&br, sps, pps, nal_unit_type, nal_ref_idc, &sh)) {
		return false;
	}

	// Selecting a reference index also selects the weighted prediction parameters that go with it
	// (H.264 8.4.2.2.2), which is why both live in one place.  The parser already stored the
	// effective weight of every entry, so an index whose weight flag was cleared is the identity.
	auto set_ref = [&](int r) {
		if (!sh.weighted_pred_present || r < 0 || r >= 32) {
			H264_WP_IDENTITY();
		} else {
			g_wp_w[0] = sh.luma_weight_l0[r];
			g_wp_o[0] = sh.luma_offset_l0[r];
			g_wp_den[0] = sh.luma_log2_weight_denom;
			g_wp_w[1] = sh.chroma_weight_l0[r][0];
			g_wp_o[1] = sh.chroma_offset_l0[r][0];
			g_wp_den[1] = sh.chroma_log2_weight_denom;
			g_wp_w[2] = sh.chroma_weight_l0[r][1];
			g_wp_o[2] = sh.chroma_offset_l0[r][1];
			g_wp_den[2] = sh.chroma_log2_weight_denom;
		}
		if (!refs0 || r < 0 || r >= n_refs0) return;
		if (!refs0[r].y || !refs0[r].u || !refs0[r].v) return;
		ref_y = refs0[r].y;
		ref_u = refs0[r].u;
		ref_v = refs0[r].v;
	};

	uint32 mb_width = sps->mb_width;
	uint32 mb_height = sps->mb_height;
	uint32 num_mbs = mb_width * mb_height;
	if (sh.first_mb_in_slice >= num_mbs) return false;

	uint8* cbp_table = (uint8*)malloc(num_mbs);
	uint8* cbp_chroma_table = (uint8*)malloc(num_mbs);
	int8* mode_table = (int8*)malloc(num_mbs * 16);
	uint8* intra_chroma_table = (uint8*)malloc(num_mbs);
	uint8* mb_type_table = (uint8*)malloc(num_mbs);
	uint8* nnz_table = (uint8*)malloc(num_mbs * 24);
	uint8* transform_8x8_table = (uint8*)malloc(num_mbs);
	uint8* mb_skip_table = (uint8*)malloc(num_mbs);
	uint8* nz_dc_table = (uint8*)malloc(num_mbs * 3);
	H264MotionVector* mv_table = (H264MotionVector*)malloc(num_mbs * 16 * sizeof(H264MotionVector));
	int16* mvd_table = (int16*)malloc(num_mbs * 16 * 2 * sizeof(int16));
	uint8* ref_table = (uint8*)malloc(num_mbs * 16);

	if (!cbp_table || !cbp_chroma_table || !mode_table || !intra_chroma_table ||
		!mb_type_table || !nnz_table || !transform_8x8_table || !mb_skip_table || !nz_dc_table || !mv_table || !mvd_table) {
		if (cbp_table) free(cbp_table);
		if (cbp_chroma_table) free(cbp_chroma_table);
		if (mode_table) free(mode_table);
		if (intra_chroma_table) free(intra_chroma_table);
		if (mb_type_table) free(mb_type_table);
		if (nnz_table) free(nnz_table);
		if (transform_8x8_table) free(transform_8x8_table);
		if (mb_skip_table) free(mb_skip_table);
		if (nz_dc_table) free(nz_dc_table);
		if (mv_table) free(mv_table);
		return false;
	}

	MemSet(cbp_table, 0, num_mbs);
	MemSet(cbp_chroma_table, 0, num_mbs);
	MemSet(mode_table, -1, num_mbs * 16);
	MemSet(intra_chroma_table, 0, num_mbs);
	MemSet(mb_type_table, 0, num_mbs);
	MemSet(nnz_table, 0, num_mbs * 24);
	MemSet(transform_8x8_table, 0, num_mbs);
	MemSet(mb_skip_table, 0, num_mbs);
	MemSet(nz_dc_table, 0, num_mbs * 3);
	MemSet(mv_table, 0, num_mbs * 16 * sizeof(H264MotionVector));
	MemSet(mvd_table, 0, num_mbs * 16 * 2 * sizeof(int16));
	MemSet(ref_table, 0, num_mbs * 16);

	// The deblocking filter has to know which macroblocks are intra, and neither the coded block
	// pattern nor ref_table can tell it here: this path leaves both at 0 for an intra macroblock.
	uint8* db_intra_tab = nullptr;
	if (db_flags_out) {
		db_intra_tab = (uint8*)malloc(num_mbs);
		if (db_intra_tab) MemSet(db_intra_tab, 0, num_mbs);
		else db_flags_out = nullptr;
	}

	size_t bit_pos = (((br.byte_pos * 8) - br.bits_left) + 7) & ~(size_t)7;
	CABACDecoder cabac;
	CABAC_Init(&cabac, rbsp_data, rbsp_len, bit_pos, 0, sh.cabac_init_idc, sh.slice_qp);
	H264Probe_Reset((int)H264_SLICE_P, rbsp_len * 8);

	int current_qp = sh.slice_qp;
	bool prev_mb_has_qp_delta = false;
	int pic_w = (int)sps->width;
	int pic_h = (int)sps->height;
	int pic_w_c = pic_w / 2;
	int pic_h_c = pic_h / 2;
	bool failed = false;
	int fail_reason = 0;
	int terminated = 0;

	for (uint32 mb_idx = sh.first_mb_in_slice; mb_idx < num_mbs; ++mb_idx) {
		if (cabac.overrun) break; // the slice data ended early: stop instead of reading garbage
		H264Probe_MB(mb_idx);
		uint32 mb_x = mb_idx % mb_width;
		uint32 mb_y = mb_idx / mb_width;
		bool hasA = (mb_x > 0);
		bool hasB = (mb_y > 0);
		bool hasC = (mb_y > 0 && mb_x + 1 < mb_width);
		bool hasD = (mb_y > 0 && mb_x > 0);

		int src_x = (int)mb_x * 16;
		int src_y = (int)mb_y * 16;
		uint8* mb_y_dst = dst_y + src_y * dst_y_stride + src_x;
		uint8* mb_u_dst = dst_u + (src_y / 2) * dst_uv_stride + (src_x / 2);
		uint8* mb_v_dst = dst_v + (src_y / 2) * dst_uv_stride + (src_x / 2);

		// mb_skip_flag (ctx 11..13)
		int ctx_skip = 11 + (hasA && !mb_skip_table[mb_idx - 1] ? 1 : 0) + (hasB && !mb_skip_table[mb_idx - mb_width] ? 1 : 0);
		int mb_skip = CABAC_DecodeBin(&cabac, ctx_skip);

		if (mb_skip) {
			mb_skip_table[mb_idx] = 1;
			// H.264 8.4.1.1: a skipped macroblock of a P slice is treated as a P_L0_16x16 with
			// RefIdxL0 = 0 and mvd_l0 = 0, so it always predicts from list 0 entry 0.  The
			// prediction previously used whichever reference the last coded macroblock had
			// selected, so as soon as a slice activated more than one reference every skipped
			// macroblock after a ref_idx > 0 macroblock predicted from the wrong picture.
			set_ref(0);
			// H.264 8.4.1.1: a skipped macroblock of a P slice is treated as a P_L0_16x16 with
			// RefIdxL0 = 0, and its motion vector is ZERO unless BOTH A and B are available and
			// neither of them uses reference 0 with a zero motion vector.  Only in that last case
			// is the predictor of 8.4.1.3.2/8.4.1.3.1 used.  Taking the median unconditionally
			// gave skipped macroblocks a non-zero motion vector next to a stationary neighbour,
			// which puts a small error into every moving region - and since the result is stored
			// as a reference picture, that error accumulates frame after frame until the next IDR.
			H264MotionVector skip_mv = { 0, 0 };
			{
				const int ia = hasA ? (int)(mb_idx - 1) * 16 + 3 : -1;         // left MB, block (3,0)
				const int ib = hasB ? (int)(mb_idx - mb_width) * 16 + 12 : -1;  // above MB, block (0,3)
				H264MvpNeighbour nA, nB;
				H264_LoadNeighbour(mv_table, ref_table, ia, 0, false, &nA);
				H264_LoadNeighbour(mv_table, ref_table, ib, 0, false, &nB);
				const bool zeroA = (nA.ref == 0 && nA.mv.x == 0 && nA.mv.y == 0);
				const bool zeroB = (nB.ref == 0 && nB.mv.x == 0 && nB.mv.y == 0);
				if (ia >= 0 && ib >= 0 && !zeroA && !zeroB) {
					skip_mv = H264_GetMVPForPartition(mv_table, mb_idx, mb_width,
													  mb_x, mb_y, 0, 0, 4, 4, ref_table, 0);
				}
			}
			for (int k = 0; k < 16; ++k) mv_table[mb_idx * 16 + k] = skip_mv;

			H264_MC_Luma(mb_y_dst, dst_y_stride, ref_y, ref_y_stride, 16, 16,
						 src_x, src_y, skip_mv.x, skip_mv.y, pic_w, pic_h);
			H264_MC_Chroma(mb_u_dst, dst_uv_stride, ref_u, ref_uv_stride, 8, 8,
						   src_x / 2, src_y / 2, skip_mv.x, skip_mv.y, pic_w_c, pic_h_c, 1);
			H264_MC_Chroma(mb_v_dst, dst_uv_stride, ref_v, ref_uv_stride, 8, 8,
						   src_x / 2, src_y / 2, skip_mv.x, skip_mv.y, pic_w_c, pic_h_c, 2);

			prev_mb_has_qp_delta = false;
			// end_of_slice_flag is present after *every* macroblock, skipped ones included.
			// Leaving it unread here shifts the arithmetic decoder by one bin per skipped
			// macroblock, which desynchronises the whole slice.
			if (CABAC_DecodeTerminate(&cabac)) {
				terminated = 1;
				break;
			}
			continue;
		}

		// Decode mb_type in P-slice. CABAC first separates inter from intra, then the inter
		// branch distinguishes 16x16 / 16x8 / 8x16 / 8x8.
		int p_type = 0;
		if (CABAC_DecodeBin(&cabac, 14) == 0) {
			if (CABAC_DecodeBin(&cabac, 15) == 0) {
				p_type = CABAC_DecodeBin(&cabac, 16) ? 3 : 0;
			} else {
				p_type = CABAC_DecodeBin(&cabac, 17) ? 1 : 2;
			}
		} else {
			p_type = 5;
		}
		if (p_type <= 3) {
			// Inter MB (P_L0_16x16 / P_L0_L0_16x8 / P_L0_L0_8x16 / P_8x8).
			// This decoder currently keeps a single list0 reference picture, so multi-ref
			// streams must not continue into MVD parsing without consuming ref_idx bins.
			// The reference index is coded per partition whenever the slice activates
			// more than one list-0 picture.  It must be consumed even though this decoder
			// currently keeps a single reference picture, otherwise every later bin of the
			// macroblock is read from the wrong place.
			const bool ref_idx_coded = (sh.num_ref_idx_l0_active_minus1 > 0);
			// transform_size_8x8_flag is coded for inter macroblocks too, but only when no
			// sub-partition is smaller than 8x8 (H.264 7.3.5).  P_16x16/16x8/8x16
			// always qualify; P_8x8 qualifies only when all four sub types are P_L0_8x8.
			bool dct8x8_allowed = true;
			uint8 pred_y[256], pred_u[64], pred_v[64];
			MemSet(pred_y, 0, sizeof(pred_y));
			MemSet(pred_u, 0, sizeof(pred_u));
			MemSet(pred_v, 0, sizeof(pred_v));

			H264MotionVector mvA = { 0, 0 }, mvB = { 0, 0 }, mvC = { 0, 0 }, mvD = { 0, 0 };
			if (hasA) mvA = mv_table[(mb_idx - 1) * 16 + 5];
			if (hasB) mvB = mv_table[(mb_idx - mb_width) * 16 + 10];
			if (hasC) mvC = mv_table[(mb_idx - mb_width + 1) * 16 + 10];
			if (hasD) mvD = mv_table[(mb_idx - mb_width - 1) * 16 + 15];

			// H.264 8.4.1.3.2: the blocks (2,0) and (2,2) of the current macroblock are the
			// diagonal neighbour C of partitions that sit in its left half, and they are not
			// decoded yet, so they start as "not available" with a zero vector.  A partition of
			// the top-left sub-macroblock reads C from block (2,0): without this marker it picked
			// up whatever the PREVIOUS macroblock had left in the table - a stale reference index
			// that looks like a valid one - and then took that foreign motion vector instead of
			// falling back to D and taking the median.
			ref_table[mb_idx * 16 + 2] = 255;
			mv_table[mb_idx * 16 + 2] = H264MotionVector{ 0, 0 };
			ref_table[mb_idx * 16 + 10] = 255;
			mv_table[mb_idx * 16 + 10] = H264MotionVector{ 0, 0 };

			if (p_type == 0) {
				int ref0 = 0;
				if (ref_idx_coded) {
					ref0 = CABAC_DecodeRefIdx(&cabac,
						CABAC_RefNeighbour(ref_table, mb_idx, mb_width, 0, 0, hasA),
						CABAC_RefNeighbourTop(ref_table, mb_idx, mb_width, 0, 0, hasB), sh.num_ref_idx_l0_active_minus1 + 1);
					CABAC_StoreRef(ref_table, mb_idx, 0, 0, 4, 4, ref0);
				}
				// 8.4.1.3.1: the predictor of a 16x16 partition is the neighbour's motion vector
				// when exactly one of A, B and C uses the same reference index, the median
				// otherwise.  H264_GetMVPForPartition also substitutes D for an unavailable C,
				// which the bare median of A/B/C/D did not model.
				H264MotionVector mvp = H264_GetMVPForPartition(mv_table, mb_idx, mb_width, mb_x, mb_y,
															   0, 0, 4, 4, ref_table, ref0);
				H264MotionVector mvd_abs;
				H264MotionVector mvd = CABAC_DecodeMVDAt(&cabac, mvd_table, mb_idx, mb_width, hasA, hasB, 0, 0, &mvd_abs);
				CABAC_StoreMVD(mvd_table, mb_idx, 0, 0, 4, 4, mvd_abs);
				H264MotionVector mv = { (int16)(mvp.x + mvd.x), (int16)(mvp.y + mvd.y) };
				for (int k = 0; k < 16; ++k) mv_table[mb_idx * 16 + k] = mv;

				set_ref(ref0);
				H264_MC_Luma(pred_y, 16, ref_y, ref_y_stride, 16, 16,
							 src_x, src_y, mv.x, mv.y, pic_w, pic_h);
				H264_MC_Chroma(pred_u, 8, ref_u, ref_uv_stride, 8, 8,
							   src_x / 2, src_y / 2, mv.x, mv.y, pic_w_c, pic_h_c, 1);
				H264_MC_Chroma(pred_v, 8, ref_v, ref_uv_stride, 8, 8,
							   src_x / 2, src_y / 2, mv.x, mv.y, pic_w_c, pic_h_c, 2);
			} else if (p_type == 1) {
				// 7.3.5: a macroblock with more than one partition codes *all* of its reference
				// indices before any motion vector difference (ref_idx_l0[0], ref_idx_l0[1], then
				// mvd_l0[0] and mvd_l0[1]).  Interleaving them desynchronised the CABAC engine as
				// soon as ref_idx was present at all, i.e. for every slice with more than one
				// active list-0 reference.
				int ref0 = 0;
				int ref1 = 0;
				if (ref_idx_coded) {
					ref0 = CABAC_DecodeRefIdx(&cabac,
						CABAC_RefNeighbour(ref_table, mb_idx, mb_width, 0, 0, hasA),
						CABAC_RefNeighbourTop(ref_table, mb_idx, mb_width, 0, 0, hasB), sh.num_ref_idx_l0_active_minus1 + 1);
					CABAC_StoreRef(ref_table, mb_idx, 0, 0, 4, 2, ref0);
					ref1 = CABAC_DecodeRefIdx(&cabac,
						CABAC_RefNeighbour(ref_table, mb_idx, mb_width, 0, 2, hasA),
						CABAC_RefNeighbourTop(ref_table, mb_idx, mb_width, 0, 2, hasB), sh.num_ref_idx_l0_active_minus1 + 1);
					CABAC_StoreRef(ref_table, mb_idx, 0, 2, 4, 2, ref1);
				}
				// 8.4.1.3 (8-200): the top 16x8 partition takes B's motion vector when B uses the
				// same reference index, and the 8.4.1.3.1 median only otherwise.
				H264MotionVector mvp0 = H264_GetMVPForPartition(mv_table, mb_idx, mb_width, mb_x, mb_y,
																0, 0, 4, 2, ref_table, ref0);
				H264MotionVector mvd0_abs;
				H264MotionVector mvd0 = CABAC_DecodeMVDAt(&cabac, mvd_table, mb_idx, mb_width, hasA, hasB, 0, 0, &mvd0_abs);
				CABAC_StoreMVD(mvd_table, mb_idx, 0, 0, 4, 2, mvd0_abs);
				H264MotionVector mv0 = { (int16)(mvp0.x + mvd0.x), (int16)(mvp0.y + mvd0.y) };
				for (int k = 0; k < 16; ++k) {
					const int by4 = k >> 2;   // mv_table 是光栅顺序：slot k 对应 (bx=k&3, by=k>>2)
					if (by4 < 2) mv_table[mb_idx * 16 + k] = mv0;
				}

				// 8.4.1.3 (8-201): the bottom half takes the motion vector of the partition to its
				// left (A) when that one uses the same reference index.  A here is read from
				// mv_table, which already holds the top half written above.
				H264MotionVector mvp1 = H264_GetMVPForPartition(mv_table, mb_idx, mb_width, mb_x, mb_y,
																0, 2, 4, 2, ref_table, ref1);
				H264MotionVector mvd1_abs;
				H264MotionVector mvd1 = CABAC_DecodeMVDAt(&cabac, mvd_table, mb_idx, mb_width, hasA, hasB, 0, 2, &mvd1_abs);
				CABAC_StoreMVD(mvd_table, mb_idx, 0, 2, 4, 2, mvd1_abs);
				H264MotionVector mv1 = { (int16)(mvp1.x + mvd1.x), (int16)(mvp1.y + mvd1.y) };

				for (int k = 0; k < 16; ++k) {
					const int by4 = k >> 2;   // mv_table 是光栅顺序：slot k 对应 (bx=k&3, by=k>>2)
					mv_table[mb_idx * 16 + k] = (by4 < 2) ? mv0 : mv1;
				}
				// The two 16x8 partitions predict from their own reference index, so the top half
				// is built completely before the source picture is switched for the bottom half.
				set_ref(ref0);
				H264_MC_Luma(pred_y, 16, ref_y, ref_y_stride, 16, 8,
							 src_x, src_y, mv0.x, mv0.y, pic_w, pic_h);
				H264_MC_Chroma(pred_u, 8, ref_u, ref_uv_stride, 8, 4,
							   src_x / 2, src_y / 2, mv0.x, mv0.y, pic_w_c, pic_h_c, 1);
				H264_MC_Chroma(pred_v, 8, ref_v, ref_uv_stride, 8, 4,
							   src_x / 2, src_y / 2, mv0.x, mv0.y, pic_w_c, pic_h_c, 2);
				set_ref(ref1);
				H264_MC_Luma(pred_y + 8 * 16, 16, ref_y, ref_y_stride, 16, 8,
							 src_x, src_y + 8, mv1.x, mv1.y, pic_w, pic_h);
				H264_MC_Chroma(pred_u + 4 * 8, 8, ref_u, ref_uv_stride, 8, 4,
							   src_x / 2, src_y / 2 + 4, mv1.x, mv1.y, pic_w_c, pic_h_c, 1);
				H264_MC_Chroma(pred_v + 4 * 8, 8, ref_v, ref_uv_stride, 8, 4,
							   src_x / 2, src_y / 2 + 4, mv1.x, mv1.y, pic_w_c, pic_h_c, 2);
			} else if (p_type == 2) {
				// 7.3.5: both 8x16 reference indices are coded before either motion vector
				// difference, exactly as in the 16x8 case above.
				int ref0 = 0;
				int ref1 = 0;
				if (ref_idx_coded) {
					ref0 = CABAC_DecodeRefIdx(&cabac,
						CABAC_RefNeighbour(ref_table, mb_idx, mb_width, 0, 0, hasA),
						CABAC_RefNeighbourTop(ref_table, mb_idx, mb_width, 0, 0, hasB), sh.num_ref_idx_l0_active_minus1 + 1);
					CABAC_StoreRef(ref_table, mb_idx, 0, 0, 2, 4, ref0);
					ref1 = CABAC_DecodeRefIdx(&cabac,
						CABAC_RefNeighbour(ref_table, mb_idx, mb_width, 2, 0, hasA),
						CABAC_RefNeighbourTop(ref_table, mb_idx, mb_width, 2, 0, hasB), sh.num_ref_idx_l0_active_minus1 + 1);
					CABAC_StoreRef(ref_table, mb_idx, 2, 0, 2, 4, ref1);
				}
				// 8.4.1.3 (8-202): the left 8x16 partition takes A's motion vector when A uses the
				// same reference index, the 8.4.1.3.1 median otherwise.
				H264MotionVector mvp0 = H264_GetMVPForPartition(mv_table, mb_idx, mb_width, mb_x, mb_y,
																0, 0, 2, 4, ref_table, ref0);
				H264MotionVector mvd0_abs;
				H264MotionVector mvd0 = CABAC_DecodeMVDAt(&cabac, mvd_table, mb_idx, mb_width, hasA, hasB, 0, 0, &mvd0_abs);
				CABAC_StoreMVD(mvd_table, mb_idx, 0, 0, 2, 4, mvd0_abs);
				H264MotionVector mv0 = { (int16)(mvp0.x + mvd0.x), (int16)(mvp0.y + mvd0.y) };
				for (int k = 0; k < 16; ++k) {
					const int bx4 = k & 3;   // mv_table 是光栅顺序：slot k 对应 (bx=k&3, by=k>>2)
					if (bx4 < 2) mv_table[mb_idx * 16 + k] = mv0;
				}

				// 8.4.1.3 (8-203): the right half takes C's motion vector when C uses the same
				// reference index.  C is the block above the partition's top-right corner, which
				// lies in the macroblock to the right of the current one at x4 == 4.
				H264MotionVector mvp1 = H264_GetMVPForPartition(mv_table, mb_idx, mb_width, mb_x, mb_y,
																2, 0, 2, 4, ref_table, ref1);
				H264MotionVector mvd1_abs;
				H264MotionVector mvd1 = CABAC_DecodeMVDAt(&cabac, mvd_table, mb_idx, mb_width, hasA, hasB, 2, 0, &mvd1_abs);
				CABAC_StoreMVD(mvd_table, mb_idx, 2, 0, 2, 4, mvd1_abs);
				H264MotionVector mv1 = { (int16)(mvp1.x + mvd1.x), (int16)(mvp1.y + mvd1.y) };

				// mv_table is addressed in raster order inside a macroblock - (by4 * 4 + bx4) -
				// by CABAC_StoreRef, CABAC_StoreMVD, H264_GetMVPForPartition and the P_8x8 path.
				// This loop used to derive bx4 with the scan8 formula while indexing the table
				// with the raw loop counter, so it put mv0 into slots {0,1,2,3,8,9,10,11} where
				// the left half of an 8x16 is {0,1,4,5,8,9,12,13}: slots 2 and 3 (and 10, 11)
				// carried the left partition's vector although they belong to the right one.
				// Every neighbour that read those 4x4 blocks - the MVP of the next macroblock,
				// its mvd context - inherited the wrong vector.
				for (int k = 0; k < 16; ++k) {
					const int bx4 = k & 3;
					mv_table[mb_idx * 16 + k] = (bx4 < 2) ? mv0 : mv1;
				}
				// Same as the 16x8 case: the left 8x16 partition is built before the source
				// picture is switched to the one its own reference index selects.
				set_ref(ref0);
				H264_MC_Luma(pred_y, 16, ref_y, ref_y_stride, 8, 16,
							 src_x, src_y, mv0.x, mv0.y, pic_w, pic_h);
				H264_MC_Chroma(pred_u, 8, ref_u, ref_uv_stride, 4, 8,
							   src_x / 2, src_y / 2, mv0.x, mv0.y, pic_w_c, pic_h_c, 1);
				H264_MC_Chroma(pred_v, 8, ref_v, ref_uv_stride, 4, 8,
							   src_x / 2, src_y / 2, mv0.x, mv0.y, pic_w_c, pic_h_c, 2);
				set_ref(ref1);
				H264_MC_Luma(pred_y + 8, 16, ref_y, ref_y_stride, 8, 16,
							 src_x + 8, src_y, mv1.x, mv1.y, pic_w, pic_h);
				H264_MC_Chroma(pred_u + 4, 8, ref_u, ref_uv_stride, 4, 8,
							   src_x / 2 + 4, src_y / 2, mv1.x, mv1.y, pic_w_c, pic_h_c, 1);
				H264_MC_Chroma(pred_v + 4, 8, ref_v, ref_uv_stride, 4, 8,
							   src_x / 2 + 4, src_y / 2, mv1.x, mv1.y, pic_w_c, pic_h_c, 2);
			} else {
				int sub_type[4];
				for (int sub = 0; sub < 4; ++sub) {
					// The sub-macroblock type of a P_8x8 partition: a set first bin is P_L0_8x8, a
					// clear first bin with a clear second bin is P_L0_8x4, and the third bin then
					// selects P_L0_4x8 (set) against P_L0_4x4 (clear).
					sub_type[sub] = 0; // P_L0_8x8
					if (CABAC_DecodeBin(&cabac, 21) == 0) {
						if (CABAC_DecodeBin(&cabac, 22) == 0) {
							sub_type[sub] = 1; // P_L0_8x4
						} else {
							sub_type[sub] = CABAC_DecodeBin(&cabac, 23) ? 2 : 3; // P_L0_4x8 / P_L0_4x4
						}
					}
					if (sub_type[sub] != 0) dct8x8_allowed = false;
				}

				// Every reference index of a P_8x8 macroblock is written before any motion
				// vector difference: one ref_idx is read per 8x8 sub-macroblock (indices 0, 4,
				// 8 and 12), not one per sub-partition, and all four are emitted ahead of the
				// motion vector differences.
				if (ref_idx_coded) {
					for (int sub = 0; sub < 4; ++sub) {
						int base_bx4 = (sub & 1) * 2;
						int base_by4 = ((sub >> 1) & 1) * 2;
						int ref = CABAC_DecodeRefIdx(&cabac,
							CABAC_RefNeighbour(ref_table, mb_idx, mb_width, base_bx4, base_by4, hasA),
							CABAC_RefNeighbourTop(ref_table, mb_idx, mb_width, base_bx4, base_by4, hasB), sh.num_ref_idx_l0_active_minus1 + 1);
						// Only +1, +8 and +9 are written here: the sub-macroblock's own
						// top-left block stays "not available" until the motion vector loop
						// reaches that sub-macroblock.  Writing the whole 2x2
						// up front made block (2,0) available while the top-left sub-macroblock was
						// still being decoded, which is exactly what the marker above prevents.
						ref_table[mb_idx * 16 + base_by4 * 4 + base_bx4 + 1] = (uint8)ref;
						ref_table[mb_idx * 16 + (base_by4 + 1) * 4 + base_bx4] = (uint8)ref;
						ref_table[mb_idx * 16 + (base_by4 + 1) * 4 + base_bx4 + 1] = (uint8)ref;
					}
				}

				for (int sub = 0; sub < 4; ++sub) {
					int base_bx4 = (sub & 1) * 2;
					int base_by4 = ((sub >> 1) & 1) * 2;
					// The sub-macroblock becomes a neighbour of its own partitions now.
					ref_table[mb_idx * 16 + base_by4 * 4 + base_bx4] =
						ref_table[mb_idx * 16 + base_by4 * 4 + base_bx4 + 1];
					int part_count = (sub_type[sub] == 0) ? 1 : ((sub_type[sub] == 3) ? 4 : 2);
					int part_w4 = (sub_type[sub] == 2 || sub_type[sub] == 3) ? 1 : 2;
					int part_h4 = (sub_type[sub] == 1 || sub_type[sub] == 3) ? 1 : 2;

					for (int part = 0; part < part_count; ++part) {
						int part_bx4 = base_bx4;
						int part_by4 = base_by4;
						if (sub_type[sub] == 1) {
							part_by4 += part;
						} else if (sub_type[sub] == 2) {
							part_bx4 += part;
						} else if (sub_type[sub] == 3) {
							part_bx4 += part & 1;
							part_by4 += (part >> 1) & 1;
						}
						H264MotionVector mvp = H264_GetMVPForPartition(mv_table, mb_idx, mb_width, mb_x, mb_y,
																	   part_bx4, part_by4, part_w4, part_h4,
																	   ref_table,
																	   (int)ref_table[mb_idx * 16 + part_by4 * 4 + part_bx4]);
						H264MotionVector mvd_abs;
						H264MotionVector mvd = CABAC_DecodeMVDAt(&cabac, mvd_table, mb_idx, mb_width, hasA, hasB, part_bx4, part_by4, &mvd_abs);
						CABAC_StoreMVD(mvd_table, mb_idx, part_bx4, part_by4, part_w4, part_h4, mvd_abs);
						H264MotionVector mv = { (int16)(mvp.x + mvd.x), (int16)(mvp.y + mvd.y) };

						for (int yy = 0; yy < part_h4; ++yy) {
							for (int xx = 0; xx < part_w4; ++xx) {
								mv_table[mb_idx * 16 + (part_by4 + yy) * 4 + part_bx4 + xx] = mv;
							}
						}

						int luma_x = part_bx4 * 4;
						int luma_y = part_by4 * 4;
						int luma_w = part_w4 * 4;
						int luma_h = part_h4 * 4;
						// All four sub-macroblock reference indices were read above, so the
						// picture this sub-partition predicts from is known here.
						set_ref((int)ref_table[mb_idx * 16 + base_by4 * 4 + base_bx4]);
						H264_MC_Luma(pred_y + luma_y * 16 + luma_x, 16,
									 ref_y, ref_y_stride, luma_w, luma_h,
									 src_x + luma_x, src_y + luma_y, mv.x, mv.y, pic_w, pic_h);

						int chroma_x = luma_x / 2;
						int chroma_y = luma_y / 2;
						int chroma_w = luma_w / 2;
						int chroma_h = luma_h / 2;
						H264_MC_Chroma(pred_u + chroma_y * 8 + chroma_x, 8,
									   ref_u, ref_uv_stride, chroma_w, chroma_h,
									   src_x / 2 + chroma_x, src_y / 2 + chroma_y, mv.x, mv.y, pic_w_c, pic_h_c, 1);
						H264_MC_Chroma(pred_v + chroma_y * 8 + chroma_x, 8,
									   ref_v, ref_uv_stride, chroma_w, chroma_h,
									   src_x / 2 + chroma_x, src_y / 2 + chroma_y, mv.x, mv.y, pic_w_c, pic_h_c, 2);
					}
				}
			}

			// Decode cbp; coded_block_pattern_luma uses an inverted neighbour bit in the
			// ctxIdxInc: a present neighbour contributes 1 when the selected CBP bit is clear.
			int cbp_luma = 0;
			int condA_0 = (hasA && ((cbp_table[mb_idx - 1] & 2) == 0)) ? 1 : 0;
			int condB_0 = (hasB && ((cbp_table[mb_idx - mb_width] & 4) == 0)) ? 1 : 0;
			if (CABAC_DecodeBin(&cabac, 73 + condA_0 + 2 * condB_0)) cbp_luma |= 1;

			int condA_1 = ((cbp_luma & 1) == 0) ? 1 : 0;
			int condB_1 = (hasB && ((cbp_table[mb_idx - mb_width] & 8) == 0)) ? 1 : 0;
			if (CABAC_DecodeBin(&cabac, 73 + condA_1 + 2 * condB_1)) cbp_luma |= 2;

			int condA_2 = (hasA && ((cbp_table[mb_idx - 1] & 8) == 0)) ? 1 : 0;
			int condB_2 = ((cbp_luma & 1) == 0) ? 1 : 0;
			if (CABAC_DecodeBin(&cabac, 73 + condA_2 + 2 * condB_2)) cbp_luma |= 4;

			int condA_3 = ((cbp_luma & 4) == 0) ? 1 : 0;
			int condB_3 = ((cbp_luma & 2) == 0) ? 1 : 0;
			if (CABAC_DecodeBin(&cabac, 73 + condA_3 + 2 * condB_3)) cbp_luma |= 8;

			int cbp_chroma = 0;
			int cbp_ca = hasA ? cbp_chroma_table[mb_idx - 1] : 0;
			int cbp_cb = hasB ? cbp_chroma_table[mb_idx - mb_width] : 0;
			int ctx_c0 = (cbp_ca > 0 ? 1 : 0) + (cbp_cb > 0 ? 2 : 0);
			if (CABAC_DecodeBin(&cabac, 77 + ctx_c0) == 0) {
				cbp_chroma = 0;
			} else {
				int ctx_c1 = 4 + (cbp_ca == 2 ? 1 : 0) + (cbp_cb == 2 ? 2 : 0);
				cbp_chroma = 1 + CABAC_DecodeBin(&cabac, 77 + ctx_c1);
			}
			cbp_table[mb_idx] = (uint8)cbp_luma;
			cbp_chroma_table[mb_idx] = (uint8)cbp_chroma;

			bool is_8x8_dct = false;
			if (dct8x8_allowed && cbp_luma > 0 && pps->transform_8x8_mode_flag) {
				int ctx_8x8 = 399 + (hasA && transform_8x8_table[mb_idx - 1] ? 1 : 0) + (hasB && transform_8x8_table[mb_idx - mb_width] ? 1 : 0);
				is_8x8_dct = (CABAC_DecodeBin(&cabac, ctx_8x8) != 0);
			}
			transform_8x8_table[mb_idx] = is_8x8_dct ? 1 : 0;

			uint8 cbp = (uint8)(cbp_luma | (cbp_chroma << 4));
			if (cbp > 0) {
				int ctx_qp = 60 + (prev_mb_has_qp_delta ? 1 : 0);
				if (CABAC_DecodeBin(&cabac, ctx_qp)) {
					int val = 1;
					while (val < 64 && CABAC_DecodeBin(&cabac, (val == 1 ? 62 : 63))) val++;
					int delta = (val & 1) ? ((val + 1) >> 1) : -((val + 1) >> 1);
					current_qp = (current_qp + delta + 52) % 52;
					prev_mb_has_qp_delta = true;
				} else {
					prev_mb_has_qp_delta = false;
				}
			} else {
				prev_mb_has_qp_delta = false;
			}

			if (is_8x8_dct) {
				for (int k8 = 0; k8 < 4; ++k8) {
					int bx8 = (k8 & 1) * 8;
					int by8 = ((k8 >> 1) & 1) * 8;
					int16 coeffs_8x8[64] = { 0 };
					int32 scaled_8x8[64] = { 0 };
					int32 res_8x8[64] = { 0 };

					if (cbp_luma & (1 << k8)) {
						int condA_f = 0, condB_f = 0;
						if (k8 == 0) {
							condA_f = (hasA && (nnz_table[(mb_idx - 1) * 24 + 1 * 4 + 1] > 0 || nnz_table[(mb_idx - 1) * 24 + 1 * 4 + 3] > 0)) ? 1 : 0;
							condB_f = (hasB && (nnz_table[(mb_idx - mb_width) * 24 + 2 * 4 + 2] > 0 || nnz_table[(mb_idx - mb_width) * 24 + 2 * 4 + 3] > 0)) ? 1 : 0;
						} else if (k8 == 1) {
							condA_f = (nnz_table[mb_idx * 24 + 0 * 4 + 1] > 0 || nnz_table[mb_idx * 24 + 0 * 4 + 3] > 0) ? 1 : 0;
							condB_f = (hasB && (nnz_table[(mb_idx - mb_width) * 24 + 3 * 4 + 2] > 0 || nnz_table[(mb_idx - mb_width) * 24 + 3 * 4 + 3] > 0)) ? 1 : 0;
						} else if (k8 == 2) {
							condA_f = (hasA && (nnz_table[(mb_idx - 1) * 24 + 3 * 4 + 1] > 0 || nnz_table[(mb_idx - 1) * 24 + 3 * 4 + 3] > 0)) ? 1 : 0;
							condB_f = (nnz_table[mb_idx * 24 + 0 * 4 + 2] > 0 || nnz_table[mb_idx * 24 + 0 * 4 + 3] > 0) ? 1 : 0;
						} else if (k8 == 3) {
							condA_f = (nnz_table[mb_idx * 24 + 2 * 4 + 1] > 0 || nnz_table[mb_idx * 24 + 2 * 4 + 3] > 0) ? 1 : 0;
							condB_f = (nnz_table[mb_idx * 24 + 1 * 4 + 2] > 0 || nnz_table[mb_idx * 24 + 1 * 4 + 3] > 0) ? 1 : 0;
						}

						int ctx_f = 1012 + condA_f + 2 * condB_f;
						(void)ctx_f;
						// In 4:2:0 there is no coded_block_flag for the 8x8 blocks: the coefficients
						// follow the transform_size_8x8_flag directly. Reading a bin here would
						// consume one that belongs to the next syntax element and desynchronise
						// the whole parse.
						{
							CABAC_DecodeResidual(&cabac, 5, 64, coeffs_8x8);
							for (int sub = 0; sub < 4; ++sub) {
								nnz_table[mb_idx * 24 + k8 * 4 + sub] = 1;
							}
							H264_Dequant8x8(coeffs_8x8, current_qp, scaled_8x8);
							H264_IDCT8x8(scaled_8x8, res_8x8);
						}
					}

					uint8* blk_dst = mb_y_dst + by8 * dst_y_stride + bx8;
					for (int y = 0; y < 8; ++y) {
						for (int x = 0; x < 8; ++x) {
							int recon = (int)pred_y[(by8 + y) * 16 + (bx8 + x)] + res_8x8[y * 8 + x];
							blk_dst[y * dst_y_stride + x] = Clip8(recon);
						}
					}
				}
			} else {
				for (int k = 0; k < 16; ++k) {
					int bx = ((k & 1) ? 1 : 0) + ((k & 4) ? 2 : 0);
					int by = ((k & 2) ? 1 : 0) + ((k & 8) ? 2 : 0);
					int blk_8x8 = (by / 2) * 2 + (bx / 2);

					int16 coeffs[16] = { 0 };
					int32 scaled[16] = { 0 };
					int32 residual[16] = { 0 };

					if (cbp_luma & (1 << blk_8x8)) {
						// Inter macroblock: an unavailable neighbour counts as empty (b_intra = 0).
						int condA_f = GetLuma4x4CondA(nnz_table, mb_idx, mb_width, k, hasA, 0);
						int condB_f = GetLuma4x4CondB(nnz_table, mb_idx, mb_width, k, hasB, 0);
						// Luma 4x4 (ctxBlockCat 2) uses coded_block_flag context base 93.
						int ctx_f = 93 + condA_f + 2 * condB_f;
						if (CABAC_DecodeBin(&cabac, ctx_f)) {
							CABAC_DecodeResidual(&cabac, 2, 16, coeffs);
							nnz_table[mb_idx * 24 + k] = 1;
							H264_Dequant4x4(coeffs, current_qp, false, scaled);
							H264_IDCT4x4(scaled, residual);
						}
					}

					uint8* blk_dst = mb_y_dst + by * 4 * dst_y_stride + bx * 4;
					for (int y = 0; y < 4; ++y) {
						for (int x = 0; x < 4; ++x) {
							int recon = (int)pred_y[(by * 4 + y) * 16 + (bx * 4 + x)] + residual[y * 4 + x];
							blk_dst[y * dst_y_stride + x] = Clip8(recon);
						}
					}
				}
			}

			// Chroma reconstruction. CABAC carries coded_block_flag bins for chroma DC
			// (context base 97), and both DC planes are decoded before any chroma AC bins.
			uint8 qp_c = H264_GetChromaQP(current_qp, pps->chroma_qp_index_offset);
			int32 scaled_dc_plane[2][4];
			int32 residuals_c_plane[2][4][16];
			MemSet(scaled_dc_plane, 0, sizeof(scaled_dc_plane));
			MemSet(residuals_c_plane, 0, sizeof(residuals_c_plane));

			if (cbp_chroma > 0) {
				for (int plane = 0; plane < 2; ++plane) {
					int16 dc_coeffs[4] = { 0 };
					int condA_dc = hasA ? (nz_dc_table[(mb_idx - 1) * 3 + 1 + plane] ? 1 : 0) : 0;
					int condB_dc = hasB ? (nz_dc_table[(mb_idx - mb_width) * 3 + 1 + plane] ? 1 : 0) : 0;
					if (CABAC_DecodeBin(&cabac, 97 + condA_dc + 2 * condB_dc)) {
						CABAC_DecodeResidual(&cabac, 3, 4, dc_coeffs);
						nz_dc_table[mb_idx * 3 + 1 + plane] = 1;
					}
					stdsint dc_in[4] = { dc_coeffs[0], dc_coeffs[1], dc_coeffs[2], dc_coeffs[3] };
					stdsint dc_out[4] = { 0 };
					Hadamard_H264_2x2(dc_in, qp_c, dc_out);
					for (int i = 0; i < 4; ++i) scaled_dc_plane[plane][i] = (int32)dc_out[i];
				}
			}

			if (cbp_chroma == 2) {
				for (int plane = 0; plane < 2; ++plane) {
					int nnz_offset = 16 + plane * 4;
					for (int ck = 0; ck < 4; ++ck) {
						int16 ac_coeffs[16] = { 0 };
						// Inter macroblock: an unavailable neighbour counts as empty (b_intra = 0).
						int condA_ac = GetChromaACCondA(nnz_table, mb_idx, mb_width, plane, ck, hasA, 0);
						int condB_ac = GetChromaACCondB(nnz_table, mb_idx, mb_width, plane, ck, hasB, 0);
						int ctx_ac = 101 + condA_ac + 2 * condB_ac;
						if (CABAC_DecodeBin(&cabac, ctx_ac)) {
							CABAC_DecodeResidual(&cabac, 4, 15, ac_coeffs);
							nnz_table[mb_idx * 24 + nnz_offset + ck] = 1;
						}
						stdsint scaled_block[16] = { 0 };
						Dequant_H264_4x4(ac_coeffs, qp_c, true, scaled_block);
						scaled_block[0] = scaled_dc_plane[plane][ck];
						stdsint res_s[16];
						IDCT_H264_4x4(scaled_block, res_s);
						for (int i = 0; i < 16; ++i) residuals_c_plane[plane][ck][i] = (int32)res_s[i];
					}
				}
			} else if (cbp_chroma == 1) {
				for (int plane = 0; plane < 2; ++plane) {
					for (int ck = 0; ck < 4; ++ck) {
						stdsint scaled_block[16] = { 0 };
						scaled_block[0] = scaled_dc_plane[plane][ck];
						stdsint res_s[16];
						IDCT_H264_4x4(scaled_block, res_s);
						for (int i = 0; i < 16; ++i) residuals_c_plane[plane][ck][i] = (int32)res_s[i];
					}
				}
			}

			for (int plane = 0; plane < 2; ++plane) {
				uint8* ch_dst = (plane == 0) ? mb_u_dst : mb_v_dst;
				uint8* ch_pred = (plane == 0) ? pred_u : pred_v;

				for (int ck = 0; ck < 4; ++ck) {
					int cbx = (ck & 1) * 4;
					int cby = ((ck >> 1) & 1) * 4;
					for (int y = 0; y < 4; ++y) {
						for (int x = 0; x < 4; ++x) {
							int recon = (int)ch_pred[(cby + y) * 8 + (cbx + x)] + residuals_c_plane[plane][ck][y * 4 + x];
							ch_dst[(cby + y) * dst_uv_stride + (cbx + x)] = Clip8(recon);
						}
					}
				}
			}

		} else {
			int intra_luma = 0;
			int intra_chroma = 0;
			int intra16_mode = 0;
			int intra_type = CABAC_DecodeIntraMbTypeP(&cabac, &intra_luma, &intra_chroma, &intra16_mode);
			if (db_intra_tab) db_intra_tab[mb_idx] = 1;
			// 8.4.1.3.2: an intra neighbour has refIdxLX = -1, so it contributes no motion
			// vector to the prediction.  ref_table marks that with 255 (see H264_LoadNeighbour);
			// leaving the zero-filled entry behind made every intra macroblock look like an
			// inter one that predicts with a zero vector and reference 0.
			for (int k = 0; k < 16; ++k) ref_table[mb_idx * 16 + k] = 255;
			if (intra_type == 25) {
				failed = true;
				fail_reason = 2;
				break;
			}

			int cbp_luma = 0;
			int cbp_chroma = 0;
			int intra_chroma_mode = 0;
			int mb_type = 0;
			bool is_8x8_dct = false;

			// Both branches below are the I-slice intra decoder: a P slice can carry intra
			// macroblocks (scene changes, intra refresh) and they must be predicted and
			// reconstructed, not copied from the reference picture.
			if (intra_type == 0) {
			// I_NxN: bin 0 selects I_4x4 / I_8x8, and bin 1 selects I_16x16 or I_PCM.
			mb_type = 0;
			mb_type_table[mb_idx] = 0;

			if (pps->transform_8x8_mode_flag) {
				int ctx_8x8 = 399 + (hasA && transform_8x8_table[mb_idx - 1] ? 1 : 0) + (hasB && transform_8x8_table[mb_idx - mb_width] ? 1 : 0);
				is_8x8_dct = (CABAC_DecodeBin(&cabac, ctx_8x8) != 0);
				transform_8x8_table[mb_idx] = is_8x8_dct ? 1 : 0;
			}

			int8 sub_modes[16];
			if (is_8x8_dct) {
				for (int k8 = 0; k8 < 4; ++k8) {
					int mode_A = 2;
					if (k8 == 0) {
						if (hasA) {
							int8 mA = mode_table[(mb_idx - 1) * 16 + 1 * 4 + 1];
							if (mA >= 0) mode_A = (int)mA;
						}
					} else if (k8 == 1) {
						int8 mA = mode_table[mb_idx * 16 + 0 * 4 + 1];
						if (mA >= 0) mode_A = (int)mA;
					} else if (k8 == 2) {
						if (hasA) {
							int8 mA = mode_table[(mb_idx - 1) * 16 + 3 * 4 + 1];
							if (mA >= 0) mode_A = (int)mA;
						}
					} else if (k8 == 3) {
						int8 mA = mode_table[mb_idx * 16 + 2 * 4 + 1];
						if (mA >= 0) mode_A = (int)mA;
					}

					int mode_B = 2;
					if (k8 == 0) {
						if (hasB) {
							int8 mB = mode_table[(mb_idx - mb_width) * 16 + 2 * 4 + 2];
							if (mB >= 0) mode_B = (int)mB;
						}
					} else if (k8 == 1) {
						if (hasB) {
							int8 mB = mode_table[(mb_idx - mb_width) * 16 + 3 * 4 + 2];
							if (mB >= 0) mode_B = (int)mB;
						}
					} else if (k8 == 2) {
						int8 mB = mode_table[mb_idx * 16 + 0 * 4 + 2];
						if (mB >= 0) mode_B = (int)mB;
					} else if (k8 == 3) {
						int8 mB = mode_table[mb_idx * 16 + 1 * 4 + 2];
						if (mB >= 0) mode_B = (int)mB;
					}

					if (((k8 == 0 || k8 == 2) && !hasA) || ((k8 == 0 || k8 == 1) && !hasB)) { mode_A = 2; mode_B = 2; } // 8.3.1.1: 任一侧邻居宏块不可用则 A、B 都按 DC 预测
					int mpm = (mode_A < mode_B) ? mode_A : mode_B;
					int prev_flag = CABAC_DecodeBin(&cabac, 68);
					int m = mpm;
					if (!prev_flag) {
						int rem = 0;
						rem += 1 * CABAC_DecodeBin(&cabac, 69);
						rem += 2 * CABAC_DecodeBin(&cabac, 69);
						rem += 4 * CABAC_DecodeBin(&cabac, 69);
						m = rem + (rem >= mpm);
					}
					for (int sub = 0; sub < 4; ++sub) {
						sub_modes[k8 * 4 + sub] = (int8)m;
						mode_table[mb_idx * 16 + k8 * 4 + sub] = (int8)m;
					}
				}
			} else {
				for (int k = 0; k < 16; ++k) {
					int bx = ((k & 1) ? 1 : 0) + ((k & 4) ? 2 : 0);
					int by = ((k & 2) ? 1 : 0) + ((k & 8) ? 2 : 0);

					int mode_A = 2;
					if (bx > 0) {
						int kA = (by & 2) * 4 + ((bx - 1) & 2) * 2 + (by & 1) * 2 + ((bx - 1) & 1);
						int8 mA = mode_table[mb_idx * 16 + kA];
						if (mA >= 0) mode_A = (int)mA;
					} else if (hasA) {
						int kA = (by & 2) * 4 + (3 & 2) * 2 + (by & 1) * 2 + (3 & 1);
						int8 mA = mode_table[(mb_idx - 1) * 16 + kA];
						if (mA >= 0) mode_A = (int)mA;
					}

					int mode_B = 2;
					if (by > 0) {
						int kB = ((by - 1) & 2) * 4 + (bx & 2) * 2 + ((by - 1) & 1) * 2 + (bx & 1);
						int8 mB = mode_table[mb_idx * 16 + kB];
						if (mB >= 0) mode_B = (int)mB;
					} else if (hasB) {
						int kB = (3 & 2) * 4 + (bx & 2) * 2 + (3 & 1) * 2 + (bx & 1);
						int8 mB = mode_table[(mb_idx - mb_width) * 16 + kB];
						if (mB >= 0) mode_B = (int)mB;
					}

					if ((bx == 0 && !hasA) || (by == 0 && !hasB)) { mode_A = 2; mode_B = 2; } // 8.3.1.1: 任一侧邻居宏块不可用则 A、B 都按 DC 预测
					int mpm = (mode_A < mode_B) ? mode_A : mode_B;
					int prev_flag = CABAC_DecodeBin(&cabac, 68);
					int m = mpm;
					if (!prev_flag) {
						int rem = 0;
						rem += 1 * CABAC_DecodeBin(&cabac, 69);
						rem += 2 * CABAC_DecodeBin(&cabac, 69);
						rem += 4 * CABAC_DecodeBin(&cabac, 69);
						m = rem + (rem >= mpm);
					}
					sub_modes[k] = (int8)m;
					mode_table[mb_idx * 16 + k] = (int8)m;
				}
			}

			// intra_chroma_pred_mode
			int ctx_c = (hasA && intra_chroma_table[mb_idx - 1] > 0 ? 1 : 0) + (hasB && intra_chroma_table[mb_idx - mb_width] > 0 ? 1 : 0);
			if (CABAC_DecodeBin(&cabac, 64 + ctx_c) == 0) {
				intra_chroma_mode = 0;
			} else {
				if (CABAC_DecodeBin(&cabac, 67) == 0) {
					intra_chroma_mode = 1;
				} else {
					intra_chroma_mode = CABAC_DecodeBin(&cabac, 67) ? 3 : 2;
				}
			}
			intra_chroma_table[mb_idx] = (uint8)intra_chroma_mode;

			// coded_block_pattern
			// The first bin's context (73 .. 76) has the neighbouring bit INVERTED: condTermFlagN
			// is 1 when the neighbour is available AND the corresponding bit of its
			// coded_block_pattern is CLEAR (0 when the neighbour is unavailable). The internal
			// bits are offset by one.
			cbp_luma = 0;
			int condA_0 = (hasA && ((cbp_table[mb_idx - 1] & 2) == 0)) ? 1 : 0;
			int condB_0 = (hasB && ((cbp_table[mb_idx - mb_width] & 4) == 0)) ? 1 : 0;
			if (CABAC_DecodeBin(&cabac, 73 + condA_0 + 2 * condB_0)) cbp_luma |= 1;

			int condA_1 = ((cbp_luma & 1) == 0) ? 1 : 0;
			int condB_1 = (hasB && ((cbp_table[mb_idx - mb_width] & 8) == 0)) ? 1 : 0;
			if (CABAC_DecodeBin(&cabac, 73 + condA_1 + 2 * condB_1)) cbp_luma |= 2;

			int condA_2 = (hasA && ((cbp_table[mb_idx - 1] & 8) == 0)) ? 1 : 0;
			int condB_2 = ((cbp_luma & 1) == 0) ? 1 : 0;
			if (CABAC_DecodeBin(&cabac, 73 + condA_2 + 2 * condB_2)) cbp_luma |= 4;

			int condA_3 = ((cbp_luma & 4) == 0) ? 1 : 0;
			int condB_3 = ((cbp_luma & 2) == 0) ? 1 : 0;
			if (CABAC_DecodeBin(&cabac, 73 + condA_3 + 2 * condB_3)) cbp_luma |= 8;

			int cbp_ca = hasA ? cbp_chroma_table[mb_idx - 1] : 0;
			int cbp_cb = hasB ? cbp_chroma_table[mb_idx - mb_width] : 0;
			int ctx_c0 = (cbp_ca > 0 ? 1 : 0) + (cbp_cb > 0 ? 2 : 0);
			if (CABAC_DecodeBin(&cabac, 77 + ctx_c0) == 0) {
				cbp_chroma = 0;
			} else {
				int ctx_c1 = 4 + (cbp_ca == 2 ? 1 : 0) + (cbp_cb == 2 ? 2 : 0);
				cbp_chroma = 1 + CABAC_DecodeBin(&cabac, 77 + ctx_c1);
			}
			cbp_table[mb_idx] = (uint8)cbp_luma;
			cbp_chroma_table[mb_idx] = (uint8)cbp_chroma;

			uint8 cbp = (uint8)(cbp_luma | (cbp_chroma << 4));
			if (cbp > 0) {
				int ctx_qp = 60 + (prev_mb_has_qp_delta ? 1 : 0);
				if (CABAC_DecodeBin(&cabac, ctx_qp)) {
					int val = 1;
					while (val < 64 && CABAC_DecodeBin(&cabac, (val == 1 ? 62 : 63))) val++;
					int delta = (val & 1) ? ((val + 1) >> 1) : -((val + 1) >> 1);
					current_qp = (current_qp + delta + 52) % 52;
					prev_mb_has_qp_delta = true;
				} else {
					prev_mb_has_qp_delta = false;
				}
			} else {
				prev_mb_has_qp_delta = false;
			}

			if (is_8x8_dct) {
				for (int k8 = 0; k8 < 4; ++k8) {
					int bx8 = (k8 & 1) * 8;
					int by8 = ((k8 >> 1) & 1) * 8;
					int16 coeffs_8x8[64] = { 0 };
					int32 scaled_8x8[64] = { 0 };
					int32 res_8x8[64] = { 0 };

					if (cbp_luma & (1 << k8)) {
						int condA_f = 0, condB_f = 0;
						if (k8 == 0) {
							condA_f = (hasA && (nnz_table[(mb_idx - 1) * 24 + 1 * 4 + 1] > 0 || nnz_table[(mb_idx - 1) * 24 + 1 * 4 + 3] > 0)) ? 1 : 0;
							condB_f = (hasB && (nnz_table[(mb_idx - mb_width) * 24 + 2 * 4 + 2] > 0 || nnz_table[(mb_idx - mb_width) * 24 + 2 * 4 + 3] > 0)) ? 1 : 0;
						} else if (k8 == 1) {
							condA_f = (nnz_table[mb_idx * 24 + 0 * 4 + 1] > 0 || nnz_table[mb_idx * 24 + 0 * 4 + 3] > 0) ? 1 : 0;
							condB_f = (hasB && (nnz_table[(mb_idx - mb_width) * 24 + 3 * 4 + 2] > 0 || nnz_table[(mb_idx - mb_width) * 24 + 3 * 4 + 3] > 0)) ? 1 : 0;
						} else if (k8 == 2) {
							condA_f = (hasA && (nnz_table[(mb_idx - 1) * 24 + 3 * 4 + 1] > 0 || nnz_table[(mb_idx - 1) * 24 + 3 * 4 + 3] > 0)) ? 1 : 0;
							condB_f = (nnz_table[mb_idx * 24 + 0 * 4 + 2] > 0 || nnz_table[mb_idx * 24 + 0 * 4 + 3] > 0) ? 1 : 0;
						} else if (k8 == 3) {
							condA_f = (nnz_table[mb_idx * 24 + 2 * 4 + 1] > 0 || nnz_table[mb_idx * 24 + 2 * 4 + 3] > 0) ? 1 : 0;
							condB_f = (nnz_table[mb_idx * 24 + 1 * 4 + 2] > 0 || nnz_table[mb_idx * 24 + 1 * 4 + 3] > 0) ? 1 : 0;
						}

						int ctx_f = 1012 + condA_f + 2 * condB_f;
						(void)ctx_f;
						// In 4:2:0 there is no coded_block_flag for the 8x8 blocks: the coefficients
						// follow the transform_size_8x8_flag directly. Reading a bin here would
						// consume one that belongs to the next syntax element and desynchronise
						// the whole parse.
						{
							CABAC_DecodeResidual(&cabac, 5, 64, coeffs_8x8);
							for (int sub = 0; sub < 4; ++sub) {
								nnz_table[mb_idx * 24 + k8 * 4 + sub] = 1;
							}
							H264_Dequant8x8(coeffs_8x8, current_qp, scaled_8x8);
							H264_IDCT8x8(scaled_8x8, res_8x8);
						}
					}

					uint8* blk_dst = mb_y_dst + by8 * dst_y_stride + bx8;
					uint8 top_8[8] = { 0 }, left_8[8] = { 0 }, tr_8[8] = { 0 };
					uint8 tl_8 = 128;
					bool b_has_top = (by8 > 0) || hasB;
					bool b_has_left = (bx8 > 0) || hasA;
					bool b_has_tl = b_has_top && b_has_left;
					bool b_has_tr = false;

					if (b_has_top) {
						const uint8* tptr = blk_dst - dst_y_stride;
						for (int x = 0; x < 8; ++x) top_8[x] = tptr[x];
					}
					if (b_has_left) {
						const uint8* lptr = blk_dst - 1;
						for (int y = 0; y < 8; ++y) left_8[y] = lptr[y * dst_y_stride];
					}
					if (b_has_tl) {
						tl_8 = *(blk_dst - dst_y_stride - 1);
					}

					if (by8 == 0) {
						if (bx8 == 0 && hasB) {
							const uint8* tptr = blk_dst - dst_y_stride + 8;
							for (int x = 0; x < 8; ++x) tr_8[x] = tptr[x];
							b_has_tr = true;
						} else if (bx8 == 8 && hasB && mb_x + 1 < mb_width) {
							const uint8* tptr = blk_dst - dst_y_stride + 8;
							for (int x = 0; x < 8; ++x) tr_8[x] = tptr[x];
							b_has_tr = true;
						}
					} else {
						if (bx8 == 0) {
							const uint8* tptr = blk_dst - dst_y_stride + 8;
							for (int x = 0; x < 8; ++x) tr_8[x] = tptr[x];
							b_has_tr = true;
						}
					}

					uint8 pred_8[64];
					H264_PredIntra8x8(pred_8, 8, top_8, left_8, tl_8, tr_8, b_has_top, b_has_left, b_has_tl, b_has_tr, sub_modes[k8 * 4]);

					for (int y = 0; y < 8; ++y) {
						for (int x = 0; x < 8; ++x) {
							int recon = (int)pred_8[y * 8 + x] + res_8x8[y * 8 + x];
							blk_dst[y * dst_y_stride + x] = Clip8(recon);
						}
					}
				}
			} else {
				for (int k = 0; k < 16; ++k) {
					int bx = ((k & 1) ? 1 : 0) + ((k & 4) ? 2 : 0);
					int by = ((k & 2) ? 1 : 0) + ((k & 8) ? 2 : 0);
					int blk_8x8 = (by / 2) * 2 + (bx / 2);

					int16 coeffs[16] = { 0 };
					int32 scaled[16] = { 0 };
					int32 residual[16] = { 0 };

					if (cbp_luma & (1 << blk_8x8)) {
						int condA_f = GetLuma4x4CondA(nnz_table, mb_idx, mb_width, k, hasA);
						int condB_f = GetLuma4x4CondB(nnz_table, mb_idx, mb_width, k, hasB);
						// Luma 4x4 (ctxBlockCat 2) uses coded_block_flag context base 93;
						// base 89 belongs to the I_16x16 luma AC blocks.
						int ctx_f = 93 + condA_f + 2 * condB_f;
						if (CABAC_DecodeBin(&cabac, ctx_f)) {
							CABAC_DecodeResidual(&cabac, 2, 16, coeffs);
							nnz_table[mb_idx * 24 + k] = 1;
							H264_Dequant4x4(coeffs, current_qp, false, scaled);
							H264_IDCT4x4(scaled, residual);
						}
					}

					uint8* blk_dst = mb_y_dst + by * 4 * dst_y_stride + bx * 4;
					uint8 top_buf[4], left_buf[4], tr_buf[4] = { 0 };
					uint8 tl_val = 128;
					bool blk_has_top = (by > 0) || hasB;
					bool blk_has_left = (bx > 0) || hasA;
					bool blk_has_tl = false;
					bool blk_has_tr = false;

					if (blk_has_top) {
						const uint8* tptr = blk_dst - dst_y_stride;
						for (int x = 0; x < 4; ++x) top_buf[x] = tptr[x];
					}
					if (blk_has_left) {
						const uint8* lptr = blk_dst - 1;
						for (int y = 0; y < 4; ++y) left_buf[y] = lptr[y * dst_y_stride];
					}
					if (by > 0 && bx > 0) {
						tl_val = *(blk_dst - dst_y_stride - 1);
						blk_has_tl = true;
					} else if (by > 0 && bx == 0 && hasA) {
						tl_val = *(blk_dst - dst_y_stride - 1);
						blk_has_tl = true;
					} else if (by == 0 && bx > 0 && hasB) {
						tl_val = *(blk_dst - dst_y_stride - 1);
						blk_has_tl = true;
					} else if (by == 0 && bx == 0 && hasA && hasB) {
						tl_val = *(blk_dst - dst_y_stride - 1);
						blk_has_tl = true;
					}

					bool tr_avail = false;
					if (by == 0) {
						if (hasB && (bx < 3 || mb_x + 1 < mb_width)) tr_avail = true;
					} else {
						if (k == 2 || k == 6 || k == 8 || k == 9 || k == 10 || k == 12 || k == 14) tr_avail = true;
					}
					if (tr_avail) {
						const uint8* tr_ptr = blk_dst - dst_y_stride + 4;
						for (int x = 0; x < 4; ++x) tr_buf[x] = tr_ptr[x];
						blk_has_tr = true;
					}

					uint8 pred_buf[16];
					H264_PredIntra4x4(pred_buf, 4, top_buf, left_buf, tl_val, tr_buf,
									  blk_has_top, blk_has_left, blk_has_tl, blk_has_tr, sub_modes[k]);

					for (int y = 0; y < 4; ++y) {
						for (int x = 0; x < 4; ++x) {
							int recon = (int)pred_buf[y * 4 + x] + residual[y * 4 + x];
							blk_dst[y * dst_y_stride + x] = Clip8(recon);
						}
					}
				}
			}

			// Chroma residual: both DC blocks are coded first, then both planes' AC blocks.
			// Reading one whole plane before the other desynchronises the bin stream whenever
			// cbp_chroma == 2.
			uint8 qp_c = H264_GetChromaQP(current_qp, pps->chroma_qp_index_offset);
			int32 scaled_dc_plane[2][4];
			int32 residuals_c_plane[2][4][16];
			MemSet(scaled_dc_plane, 0, sizeof(scaled_dc_plane));
			MemSet(residuals_c_plane, 0, sizeof(residuals_c_plane));

			if (cbp_chroma > 0) {
				for (int plane = 0; plane < 2; ++plane) {
					int16 dc_coeffs[4] = { 0 };
					int condA_dc = hasA ? (nz_dc_table[(mb_idx - 1) * 3 + 1 + plane] ? 1 : 0) : 1;
					int condB_dc = hasB ? (nz_dc_table[(mb_idx - mb_width) * 3 + 1 + plane] ? 1 : 0) : 1;
					if (CABAC_DecodeBin(&cabac, 97 + condA_dc + 2 * condB_dc)) {
						CABAC_DecodeResidual(&cabac, 3, 4, dc_coeffs);
						nz_dc_table[mb_idx * 3 + 1 + plane] = 1;
					}
					stdsint dc_in[4] = { dc_coeffs[0], dc_coeffs[1], dc_coeffs[2], dc_coeffs[3] };
					stdsint dc_out[4] = { 0 };
					Hadamard_H264_2x2(dc_in, qp_c, dc_out);
					for (int i = 0; i < 4; ++i) scaled_dc_plane[plane][i] = (int32)dc_out[i];
				}
			}
			if (cbp_chroma == 2) {
				for (int plane = 0; plane < 2; ++plane) {
					int nnz_offset = 16 + plane * 4;
					for (int ck = 0; ck < 4; ++ck) {
						int16 ac_coeffs[16] = { 0 };
						int condA_ac = GetChromaACCondA(nnz_table, mb_idx, mb_width, plane, ck, hasA);
						int condB_ac = GetChromaACCondB(nnz_table, mb_idx, mb_width, plane, ck, hasB);
						int ctx_ac = 101 + condA_ac + 2 * condB_ac;
						if (CABAC_DecodeBin(&cabac, ctx_ac)) {
							CABAC_DecodeResidual(&cabac, 4, 15, ac_coeffs);
							nnz_table[mb_idx * 24 + nnz_offset + ck] = 1;
						}
						stdsint scaled_block[16] = { 0 };
						Dequant_H264_4x4(ac_coeffs, qp_c, true, scaled_block);
						scaled_block[0] = scaled_dc_plane[plane][ck];
						stdsint res_s[16];
						IDCT_H264_4x4(scaled_block, res_s);
						for (int i = 0; i < 16; ++i) residuals_c_plane[plane][ck][i] = (int32)res_s[i];
					}
				}
			} else if (cbp_chroma == 1) {
				for (int plane = 0; plane < 2; ++plane) {
					for (int ck = 0; ck < 4; ++ck) {
						stdsint scaled_block[16] = { 0 };
						scaled_block[0] = scaled_dc_plane[plane][ck];
						stdsint res_s[16];
						IDCT_H264_4x4(scaled_block, res_s);
						for (int i = 0; i < 16; ++i) residuals_c_plane[plane][ck][i] = (int32)res_s[i];
					}
				}
			}

			for (int plane = 0; plane < 2; ++plane) {
				uint8* ch_dst = (plane == 0) ? mb_u_dst : mb_v_dst;
				uint8 top_c[8] = { 128, 128, 128, 128, 128, 128, 128, 128 }, left_c[8] = { 128, 128, 128, 128, 128, 128, 128, 128 }, tl_c = 128;
				if (hasB) {
					const uint8* tptr = ch_dst - dst_uv_stride;
					for (int x = 0; x < 8; ++x) top_c[x] = tptr[x];
				}
				if (hasA) {
					const uint8* lptr = ch_dst - 1;
					for (int y = 0; y < 8; ++y) left_c[y] = lptr[y * dst_uv_stride];
				}
				if (hasA && hasB) {
					tl_c = *(ch_dst - dst_uv_stride - 1);
				}

				uint8 pred_c[64];
				H264_PredIntraChroma8x8(pred_c, 8, top_c, left_c, tl_c, hasB, hasA, hasA && hasB, intra_chroma_mode);

				for (int ck = 0; ck < 4; ++ck) {
					int cbx = (ck & 1) * 4;
					int cby = ((ck >> 1) & 1) * 4;
					for (int y = 0; y < 4; ++y) {
						for (int x = 0; x < 4; ++x) {
							int recon = (int)pred_c[(cby + y) * 8 + (cbx + x)] + residuals_c_plane[plane][ck][y * 4 + x];
							ch_dst[(cby + y) * dst_uv_stride + (cbx + x)] = Clip8(recon);
						}
					}
				}
			}

			} else {
			// I_16x16 inside a P slice: coded block pattern and prediction mode arrived with
			// CABAC_DecodeIntraMbTypeP (contexts 17..20). The transmitted value IS the
			// Intra16x16PredMode (0 Vertical, 1 Horizontal, 2 DC, 3 Plane): the mode's two bits
			// are transmitted MSB first, as i_pred >> 1 and i_pred & 1. Permuting that mapping
			// here would swap Vertical and DC and break every I_16x16 macroblock of a P/B slice
			// whose neighbourhood is not flat.
			cbp_luma = intra_luma;
			cbp_chroma = intra_chroma;
			mb_type = intra_type;
			mb_type_table[mb_idx] = (uint8)mb_type;
			// An I_16x16 macroblock carries its cbp inside mb_type, but the cbp context of a
			// following I_NxN / inter macroblock still reads the neighbour's cbp. Leaving these
			// at 0 makes every cbp context next to an I_16x16 macroblock wrong.
			cbp_table[mb_idx] = (uint8)cbp_luma;
			cbp_chroma_table[mb_idx] = (uint8)cbp_chroma;

			int ctx_c = 64 + (hasA && intra_chroma_table[mb_idx - 1] > 0 ? 1 : 0) + (hasB && intra_chroma_table[mb_idx - mb_width] > 0 ? 1 : 0);
			if (CABAC_DecodeBin(&cabac, ctx_c) == 0) {
				intra_chroma_mode = 0;
			} else {
				if (CABAC_DecodeBin(&cabac, 67) == 0) {
					intra_chroma_mode = 1;
				} else {
					intra_chroma_mode = CABAC_DecodeBin(&cabac, 67) ? 3 : 2;
				}
			}
			intra_chroma_table[mb_idx] = (uint8)intra_chroma_mode;

			int ctx_qp = 60 + (prev_mb_has_qp_delta ? 1 : 0);
			if (CABAC_DecodeBin(&cabac, ctx_qp)) {
				int val = 1;
				// The unary remainder of mb_qp_delta uses a fixed context after its first bin:
				// 60+2 for the second bin and 60+3 for every later one. It does NOT depend on
				// the previous macroblock.
				while (val < 64 && CABAC_DecodeBin(&cabac, (val == 1 ? 62 : 63))) val++;
				int delta = (val & 1) ? ((val + 1) >> 1) : -((val + 1) >> 1);
				current_qp = (current_qp + delta + 52) % 52;
				prev_mb_has_qp_delta = true;
			} else {
				prev_mb_has_qp_delta = false;
			}

			// Luma DC: only present when the luma cbp is not zero, otherwise reading it would
			// consume the bits of the following macroblock and desynchronise the CABAC parse.
			int16 luma_dc[16] = { 0 };
			stdsint scaled_luma_dc[16] = { 0 };
			{
				// The luma DC block of an I_16x16 macroblock ALWAYS carries a coded_block_flag
				// (context base 85), even when CodedBlockPatternLuma is 0, so it is read
				// unconditionally, ahead of the "cbp_luma != 0" guard. Skipping
				// this bin leaves the whole residual parse one bin short and desynchronises the
				// slice from the next coded block onwards.
				int condA_dc = hasA ? (nz_dc_table[(mb_idx - 1) * 3 + 0] ? 1 : 0) : 1;
				int condB_dc = hasB ? (nz_dc_table[(mb_idx - mb_width) * 3 + 0] ? 1 : 0) : 1;
				if (CABAC_DecodeBin(&cabac, 85 + condA_dc + 2 * condB_dc)) {
					CABAC_DecodeResidual(&cabac, 0, 16, luma_dc);
					nz_dc_table[mb_idx * 3 + 0] = 1;
				}
				stdsint dc_in[16];
				for (int i = 0; i < 16; ++i) dc_in[i] = (stdsint)luma_dc[i];
				Hadamard_H264_4x4(dc_in, current_qp, scaled_luma_dc);
			}

			// 16 Luma AC blocks
			int32 residuals[16][16];
			MemSet(residuals, 0, sizeof(residuals));

			for (int k = 0; k < 16; ++k) {
				int16 ac_coeffs[16] = { 0 };
				if (cbp_luma > 0) {
					int condA_ac = GetLuma4x4CondA(nnz_table, mb_idx, mb_width, k, hasA);
					int condB_ac = GetLuma4x4CondB(nnz_table, mb_idx, mb_width, k, hasB);
					// I_16x16 luma AC uses context base 89; base 85 belongs to the DC block.
					int ctx_ac = 89 + condA_ac + 2 * condB_ac;
					if (CABAC_DecodeBin(&cabac, ctx_ac)) {
						CABAC_DecodeResidual(&cabac, 1, 15, ac_coeffs);
						nnz_table[mb_idx * 24 + k] = 1;
					}
				}
				stdsint scaled_block[16] = { 0 };
				Dequant_H264_4x4(ac_coeffs, current_qp, true, scaled_block);
				scaled_block[0] = scaled_luma_dc[(k & 9) | ((k & 2) << 1) | ((k & 4) >> 1)];    // Figure 8-6: dcY_ij belongs to the block at ( col j, row i )
				stdsint res_s[16];
				IDCT_H264_4x4(scaled_block, res_s);
				for (int i = 0; i < 16; ++i) residuals[k][i] = (int32)res_s[i];
			}

			uint8 top_16[16], left_16[16], tl_16 = 128;
			for (int i = 0; i < 16; ++i) { top_16[i] = 128; left_16[i] = 128; }
			if (hasB) {
				const uint8* tptr = mb_y_dst - dst_y_stride;
				for (int x = 0; x < 16; ++x) top_16[x] = tptr[x];
			}
			if (hasA) {
				const uint8* lptr = mb_y_dst - 1;
				for (int y = 0; y < 16; ++y) left_16[y] = lptr[y * dst_y_stride];
			}
			if (hasA && hasB) {
				tl_16 = *(mb_y_dst - dst_y_stride - 1);
			}

			uint8 pred_16[256];
			H264_PredIntra16x16(pred_16, 16, top_16, left_16, tl_16, hasB, hasA, hasA && hasB, intra16_mode);

			for (int k = 0; k < 16; ++k) {
				int bx = ((k & 1) ? 1 : 0) + ((k & 4) ? 2 : 0);
				int by = ((k & 2) ? 1 : 0) + ((k & 8) ? 2 : 0);
				for (int y = 0; y < 4; ++y) {
					for (int x = 0; x < 4; ++x) {
						int recon = (int)pred_16[(by * 4 + y) * 16 + (bx * 4 + x)] + residuals[k][y * 4 + x];
						mb_y_dst[(by * 4 + y) * dst_y_stride + (bx * 4 + x)] = Clip8(recon);
					}
				}
			}

			// Chroma residual: both DC blocks are coded first, then both planes' AC blocks.
			// The chroma DC block has its own coded_block_flag with context base 97, and the
			// chroma AC coded_block_flag uses context base 101 (93 belongs to luma 4x4).
			uint8 qp_c = H264_GetChromaQP(current_qp, pps->chroma_qp_index_offset);
			int32 scaled_dc_plane[2][4];
			int32 residuals_c_plane[2][4][16];
			MemSet(scaled_dc_plane, 0, sizeof(scaled_dc_plane));
			MemSet(residuals_c_plane, 0, sizeof(residuals_c_plane));

			if (cbp_chroma > 0) {
				for (int plane = 0; plane < 2; ++plane) {
					int16 dc_coeffs[4] = { 0 };
					int condA_dc = hasA ? (nz_dc_table[(mb_idx - 1) * 3 + 1 + plane] ? 1 : 0) : 1;
					int condB_dc = hasB ? (nz_dc_table[(mb_idx - mb_width) * 3 + 1 + plane] ? 1 : 0) : 1;
					if (CABAC_DecodeBin(&cabac, 97 + condA_dc + 2 * condB_dc)) {
						CABAC_DecodeResidual(&cabac, 3, 4, dc_coeffs);
						nz_dc_table[mb_idx * 3 + 1 + plane] = 1;
					}
					stdsint dc_in[4] = { dc_coeffs[0], dc_coeffs[1], dc_coeffs[2], dc_coeffs[3] };
					stdsint dc_out[4] = { 0 };
					Hadamard_H264_2x2(dc_in, qp_c, dc_out);
					for (int i = 0; i < 4; ++i) scaled_dc_plane[plane][i] = (int32)dc_out[i];
				}
			}
			if (cbp_chroma == 2) {
				for (int plane = 0; plane < 2; ++plane) {
					int nnz_offset = 16 + plane * 4;
					for (int ck = 0; ck < 4; ++ck) {
						int16 ac_coeffs[16] = { 0 };
						int condA_ac = GetChromaACCondA(nnz_table, mb_idx, mb_width, plane, ck, hasA);
						int condB_ac = GetChromaACCondB(nnz_table, mb_idx, mb_width, plane, ck, hasB);
						int ctx_ac = 101 + condA_ac + 2 * condB_ac;
						if (CABAC_DecodeBin(&cabac, ctx_ac)) {
							CABAC_DecodeResidual(&cabac, 4, 15, ac_coeffs);
							nnz_table[mb_idx * 24 + nnz_offset + ck] = 1;
						}
						stdsint scaled_block[16] = { 0 };
						Dequant_H264_4x4(ac_coeffs, qp_c, true, scaled_block);
						scaled_block[0] = scaled_dc_plane[plane][ck];
						stdsint res_s[16];
						IDCT_H264_4x4(scaled_block, res_s);
						for (int i = 0; i < 16; ++i) residuals_c_plane[plane][ck][i] = (int32)res_s[i];
					}
				}
			} else if (cbp_chroma == 1) {
				for (int plane = 0; plane < 2; ++plane) {
					for (int ck = 0; ck < 4; ++ck) {
						stdsint scaled_block[16] = { 0 };
						scaled_block[0] = scaled_dc_plane[plane][ck];
						stdsint res_s[16];
						IDCT_H264_4x4(scaled_block, res_s);
						for (int i = 0; i < 16; ++i) residuals_c_plane[plane][ck][i] = (int32)res_s[i];
					}
				}
			}

			for (int plane = 0; plane < 2; ++plane) {
				uint8* ch_dst = (plane == 0) ? mb_u_dst : mb_v_dst;
				uint8 top_c[8] = { 128, 128, 128, 128, 128, 128, 128, 128 }, left_c[8] = { 128, 128, 128, 128, 128, 128, 128, 128 }, tl_c = 128;
				if (hasB) {
					const uint8* tptr = ch_dst - dst_uv_stride;
					for (int x = 0; x < 8; ++x) top_c[x] = tptr[x];
				}
				if (hasA) {
					const uint8* lptr = ch_dst - 1;
					for (int y = 0; y < 8; ++y) left_c[y] = lptr[y * dst_uv_stride];
				}
				if (hasA && hasB) {
					tl_c = *(ch_dst - dst_uv_stride - 1);
				}

				uint8 pred_c[64];
				H264_PredIntraChroma8x8(pred_c, 8, top_c, left_c, tl_c, hasB, hasA, hasA && hasB, intra_chroma_mode);

				for (int ck = 0; ck < 4; ++ck) {
					int cbx = (ck & 1) * 4;
					int cby = ((ck >> 1) & 1) * 4;
					for (int y = 0; y < 4; ++y) {
						for (int x = 0; x < 4; ++x) {
							int recon = (int)pred_c[(cby + y) * 8 + (cbx + x)] + residuals_c_plane[plane][ck][y * 4 + x];
							ch_dst[(cby + y) * dst_uv_stride + (cbx + x)] = Clip8(recon);
						}
					}
				}
			}
			}

			for (int k = 0; k < 16; ++k) {
				mv_table[mb_idx * 16 + k].x = 0;
				mv_table[mb_idx * 16 + k].y = 0;
			}
		}

		if (mb_qp_out) mb_qp_out[mb_idx] = (uint8)current_qp;

		if (CABAC_DecodeTerminate(&cabac)) {
			terminated = 1;
			break;
		}
	}

	// The motion data of this picture has to be exported *before* its tables are released:
	// copying it afterwards reads freed memory.
	if (mv_out) MemCopyN(mv_out, mv_table, (size_t)num_mbs * 16 * sizeof(H264MotionVector));
	if (refidx_out) MemCopyN(refidx_out, ref_table, (size_t)num_mbs * 16);

	// Export what the deblocking filter needs: this picture uses list0 only, so list1 stays empty
	// (-1 = "list not used") and its motion vectors stay zero, which is what JM keeps for a list a
	// block does not predict from.
	if (db_flags_out) {
		for (uint32 m = 0; m < num_mbs; ++m) {
			db_flags_out[(size_t)m * 4 + 0] = db_intra_tab ? db_intra_tab[m] : 0;
			db_flags_out[(size_t)m * 4 + 1] = transform_8x8_table[m];
			db_flags_out[(size_t)m * 4 + 2] = cbp_table[m];
			db_flags_out[(size_t)m * 4 + 3] = 0;
		}
	}
	if (db_mv_out) {
		MemSet(db_mv_out, 0, (size_t)num_mbs * 32 * 2 * sizeof(int16));
		for (uint32 m = 0; m < num_mbs; ++m) {
			for (int b = 0; b < 16; ++b) {
				db_mv_out[((size_t)m * 32 + b) * 2 + 0] = (int16)mv_table[m * 16 + b].x;
				db_mv_out[((size_t)m * 32 + b) * 2 + 1] = (int16)mv_table[m * 16 + b].y;
			}
		}
	}
	if (db_ref_out) {
		for (uint32 m = 0; m < num_mbs; ++m) {
			for (int b = 0; b < 16; ++b) {
				const int ri = (int)ref_table[m * 16 + b];
				db_ref_out[(size_t)m * 32 + b] = (int16)((ri == 0xFF) ? -1 : ri);
				db_ref_out[(size_t)m * 32 + 16 + b] = -1;
			}
		}
	}
	if (db_intra_tab) free(db_intra_tab);

	free(cbp_table);
	free(cbp_chroma_table);
	free(mode_table);
	free(intra_chroma_table);
	free(mb_type_table);
	free(nnz_table);
	free(transform_8x8_table);
	free(mb_skip_table);
	free(nz_dc_table);
	free(mvd_table);
	free(ref_table);
	free(mv_table);
	bool ok = !failed && cabac.bit_pos <= rbsp_len * 8 + 512;
	H264Probe_End(ok ? 1 : 0, &cabac, terminated, fail_reason);
	return ok;
}

// Phase 5: In-Loop Deblocking Filter

static const uint8 deblock_alpha_table[52] = {
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  4,  4,  5,  6,
	7,  8,  9, 10, 12, 13, 15, 17, 20, 22,
	25, 28, 32, 36, 40, 45, 50, 56, 63, 71,
	80, 90, 101, 113, 127, 144, 162, 182, 203, 226,
	255, 255
};

static const uint8 deblock_beta_table[52] = {
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  2,  2,  2,  3,
	3,  3,  3,  4,  4,  4,  6,  6,  7,  7,
	8,  8,  9,  9, 10, 10, 11, 11, 12, 12,
	13, 13, 14, 14, 15, 15, 16, 16, 17, 17,
	18, 18
};

static const uint8 deblock_tc0_table[52][3] = {
	{ 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 },
	{ 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 },
	{ 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 },
	{ 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 1 }, { 0, 0, 1 }, { 0, 0, 1 },
	{ 0, 0, 1 }, { 0, 1, 1 }, { 0, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 },
	{ 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 2 }, { 1, 1, 2 }, { 1, 1, 2 },
	{ 1, 1, 2 }, { 1, 2, 3 }, { 1, 2, 3 }, { 2, 2, 3 }, { 2, 2, 4 },
	{ 2, 3, 4 }, { 2, 3, 4 }, { 3, 3, 5 }, { 3, 4, 6 }, { 3, 4, 6 },
	{ 4, 5, 7 }, { 4, 5, 8 }, { 4, 6, 9 }, { 5, 7, 10 }, { 6, 8, 11 },
	{ 6, 8, 13 }, { 7, 10, 14 }, { 8, 11, 16 }, { 9, 12, 18 }, { 10, 13, 20 },
	{ 11, 15, 23 }, { 13, 17, 25 }
};

// =========================================================================================
// B slices
//
// A B picture is never used as a reference picture, so a B slice that cannot be decoded
// cannot corrupt the reference chain: the caller repeats the list0 reference picture.
// =========================================================================================

// A stored reference picture as handed to the motion compensation paths. mv / refidx are the
// list0 motion data of that picture, needed by temporal direct prediction of a B slice.
struct H264RefPicDesc {
	const uint8* y;
	const uint8* u;
	const uint8* v;
	int y_stride;
	int uv_stride;
	const H264MotionVector* mv;
	const uint8* refidx;
	// List1 motion data of the same picture; 255 means "this list is not used by the block".
	// H.264 8.4.1.2.1 takes mvCol and refIdxCol from the collocated partition's list1 entry
	// when that partition does not use list0 at all, so the backward motion field of a
	// reference B picture has to be stored alongside the forward one.
	const H264MotionVector* mv1;
	const uint8* refidx1;
	// List0 of THIS picture, by picture order count: the list its own reference indices index
	// into.  H.264 8.4.1.2.3 MapColToList0() maps refIdxCol to the current list0 entry that
	// names the same picture, which needs this list.
	const int* list0_poc;
	int n_list0;
	int poc;
};

// Upper bound on the reference picture lists handed to the B slice decoder.
#define H264_MAX_REF_LIST 16

// One partition of a B macroblock. The syntax is parsed first and the motion compensation is
// run afterwards, because the standard emits all reference indices before all motion vector
// differences (H.264 7.3.5) while the reconstruction order is free.
struct H264BPredPart {
	int x4, y4, w4, h4;
	bool use0, use1;
	int ref0, ref1;
	H264MotionVector mv0, mv1;
};

static inline int H264_Clip3i(int lo, int v, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

// A B_8x8 macroblock can contribute up to 4 sub-macroblocks * 4 sub-partitions * 2 lists
// partitions before the bi-predictive pairs are merged, so the partition scratch buffer has to
// hold 32 entries; a smaller one overflows the stack.
#define H264_MAX_B_PARTS 32

// Distance proportional scale factor (H.264 8.4.2.3.1): tb expressed over td in 1/256 units.
static inline int H264_DistScaleFactor(int poc0, int poc1, int cur_poc) {
	int td = H264_Clip3i(-128, poc1 - poc0, 127);
	if (td == 0) return 0;
	int tb = H264_Clip3i(-128, cur_poc - poc0, 127);
	int atd = (td < 0) ? -td : td;
	int tx = (16384 + atd / 2) / td;
	return H264_Clip3i(-1024, (tb * tx + 32) >> 6, 1023);
}

// Weights of an implicitly weighted bi-predictive partition (weighted_bipred_idc == 2). w0 and
// w1 always sum to 64 and the prediction is (p0 * w0 + p1 * w1 + 32) >> 6; for a pair of
// equidistant references this degenerates to the plain average (32, 32).
static inline void H264_ImplicitWeights(int idc, int cur_poc, int poc0, int poc1, int* w0, int* w1) {
	if (idc == 2) {
		// H.264 8.4.2.2.3: the scaled weight only applies when the picture order counts produce a
		// DistScaleFactor in the range the derivation assumes; equations 8-276/8-277 then replace
		// it by equal weights, and without that test w0 = 64 - (dsf >> 2) goes NEGATIVE for the
		// picture distances a temporal direct partition can produce.
		const int w = H264_DistScaleFactor(poc0, poc1, cur_poc) >> 2;
		if (poc1 == poc0 || w < -64 || w > 128) {
			*w0 = 32;
			*w1 = 32;
		} else {
			*w1 = w;
			*w0 = 64 - w;
		}
	} else {
		*w0 = 32;
		*w1 = 32;
	}
}

static inline int H264_MinPositive(int a, int b) {
	if (a < 0) return b;
	if (b < 0) return a;
	return (a < b) ? a : b;
}

// Motion vector of a 4x4 neighbour, or zero when it does not use the requested reference.
static inline H264MotionVector H264_NbMV(const H264MotionVector* mvt, const uint8* reft,
										 uint32 base, int idx, int refidx) {
	H264MotionVector zero = { 0, 0 };
	if (!mvt) return zero;
	if (reft && reft[base + idx] != (uint8)refidx) return zero;
	return mvt[base + idx];
}

// H.264 8.4.1.3.1 median prediction restricted to the neighbours that use refidx. Used by
// spatial direct prediction, where the reference index itself comes from the neighbours.
// A neighbour whose reference index differs is NOT a neighbour with a zero motion vector: it is
// unavailable, so that "exactly one of A, B, C uses refIdxLX" picks that one's motion vector
// instead of a median that the zero vectors would drag towards zero.  On a skipped macroblock,
// which carries no residual, the motion vector derived this way is the only thing that decides
// the reconstruction, so the difference is visible immediately.
static H264MotionVector H264_MvpFiltered(
	const H264MotionVector* mvt, const uint8* reft,
	uint32 mb_idx, uint32 mb_width, uint32 mb_x, uint32 mb_y,
	int bx4, int by4, int bw4, int bh4, int refidx
) {
	H264MotionVector zero = { 0, 0 };
	H264MotionVector mvA = zero, mvB = zero, mvC = zero, mvD = zero;
	bool hasA = false, hasB = false, hasC = false, hasD = false;
	const uint32 cur_base = mb_idx * 16;
	// H264_NbMV reports a neighbour of another reference as a zero vector; the availability has
	// to be decided from the same test, otherwise the zero joins the median.
	#define H264_NB_REF(i) (reft ? (int)reft[i] : refidx)

	if (bx4 > 0) {
		mvA = H264_NbMV(mvt, reft, cur_base, by4 * 4 + bx4 - 1, refidx);
		hasA = (H264_NB_REF(cur_base + by4 * 4 + bx4 - 1) == refidx);
	} else if (mb_x > 0) {
		mvA = H264_NbMV(mvt, reft, (mb_idx - 1) * 16, by4 * 4 + 3, refidx);
		hasA = (H264_NB_REF((mb_idx - 1) * 16 + by4 * 4 + 3) == refidx);
	}

	if (by4 > 0) {
		mvB = H264_NbMV(mvt, reft, cur_base, (by4 - 1) * 4 + bx4, refidx);
		hasB = (H264_NB_REF(cur_base + (by4 - 1) * 4 + bx4) == refidx);
	} else if (mb_y > 0) {
		mvB = H264_NbMV(mvt, reft, (mb_idx - mb_width) * 16, 12 + bx4, refidx);
		hasB = (H264_NB_REF((mb_idx - mb_width) * 16 + 12 + bx4) == refidx);
	}

	int cx4 = bx4 + bw4;
	int cy4 = by4 - 1;
	// Same rule as H264_GetMVPForPartition: the current macroblock's own blocks (2, 0) and
	// (2, 2) are PART_NOT_AVAILABLE until the sub-macroblock loop opens them, so a diagonal that
	// reaches into them does not exist and D has to stand in.
	bool c_open = true;
	if (cy4 >= 0 && cx4 < 4 && (cx4 & 1) == 0 && (cy4 & 1) == 0)
		c_open = ((((cy4 >> 1) * 2 + (cx4 >> 1))) <= ((by4 >> 1) * 2 + (bx4 >> 1)));
	if (!c_open) {
		/* leave C unavailable; MedianMVP substitutes D */
	} else if (cy4 >= 0 && cx4 < 4) {
		mvC = H264_NbMV(mvt, reft, cur_base, cy4 * 4 + cx4, refidx);
		hasC = (H264_NB_REF(cur_base + cy4 * 4 + cx4) == refidx);
	} else if (cy4 < 0 && cx4 < 4 && mb_y > 0) {
		mvC = H264_NbMV(mvt, reft, (mb_idx - mb_width) * 16, 12 + cx4, refidx);
		hasC = (H264_NB_REF((mb_idx - mb_width) * 16 + 12 + cx4) == refidx);
	} else if (cy4 < 0 && cx4 >= 4 && mb_y > 0 && mb_x + 1 < mb_width) {
		// see H264_GetMVPForPartition: with by4 > 0 the above-right sample belongs to the
		// macroblock to the right of the same row, which is not decoded yet
		mvC = H264_NbMV(mvt, reft, (mb_idx - mb_width + 1) * 16, 12, refidx);
		hasC = (H264_NB_REF((mb_idx - mb_width + 1) * 16 + 12) == refidx);
	}

	if (bx4 > 0 && by4 > 0) {
		mvD = H264_NbMV(mvt, reft, cur_base, (by4 - 1) * 4 + bx4 - 1, refidx);
		hasD = (H264_NB_REF(cur_base + (by4 - 1) * 4 + bx4 - 1) == refidx);
	} else if (mb_x > 0 && mb_y > 0) {
		int dx4 = (bx4 > 0) ? (bx4 - 1) : 3;
		int dy4 = (by4 > 0) ? (by4 - 1) : 3;
		mvD = H264_NbMV(mvt, reft, (mb_idx - mb_width - 1) * 16, dy4 * 4 + dx4, refidx);
		hasD = (H264_NB_REF((mb_idx - mb_width - 1) * 16 + dy4 * 4 + dx4) == refidx);
	}
	#undef H264_NB_REF

	return MedianMVP(mvA, hasA, mvB, hasB, mvC, hasC, mvD, hasD);
}

// Reference index of a 4x4 neighbour for spatial direct prediction: -1 when the block is
// unavailable or belongs to an intra or direct coded block (H.264 8.4.1.2.2).
static inline int H264_NbRefIdx(const uint8* reft, const uint8* nv_direct4,
								uint32 base, int idx, bool available, uint32 cur_base) {
	if (!available || !reft) return -1;
	if (idx < (int)cur_base - 16 || idx >= (int)cur_base) {
		if (nv_direct4 && nv_direct4[idx]) return -1;
	}
	return (int)reft[idx];
}

// H.264 8.4.1.2.1. The macroblock's mb_type for a B slice, six/seven binary decisions at
// ctxIdxOffset 27. Returns the B mb_type index 0..22, or -1 for an intra macroblock (in which
// case the intra mb_type values are returned through the trailing pointers).
static int CABAC_DecodeBMbType(CABACDecoder* cabac,
							   bool left_coded, bool top_coded,
							   int* intra_cbp_luma, int* intra_cbp_chroma,
							   int* intra16_mode, bool* intra_is_16, bool* is_pcm) {
	*is_pcm = false;
	*intra_is_16 = false;
	// ctxIdxOffset 27 counts the neighbouring macroblocks that are available and were *not*
	// coded as B_Skip or B_Direct_16x16.  A skipped or direct neighbour does not raise it.
	int ctx = 0;
	if (left_coded) ctx++;
	if (top_coded) ctx++;

	if (CABAC_DecodeBin(cabac, 27 + ctx) == 0) return 0;             // B_Direct_16x16
	if (CABAC_DecodeBin(cabac, 27 + 3) == 0) {
		return 1 + CABAC_DecodeBin(cabac, 27 + 5);                   // B_L0_16x16 / B_L1_16x16
	}

	int bits = CABAC_DecodeBin(cabac, 27 + 4) << 3;
	bits += CABAC_DecodeBin(cabac, 27 + 5) << 2;
	bits += CABAC_DecodeBin(cabac, 27 + 5) << 1;
	bits += CABAC_DecodeBin(cabac, 27 + 5);
	if (bits < 8) return bits + 3;                                   // B_Bi_16x16 .. B_L1_L0_16x8
	if (bits == 14) return 11;                                       // B_L1_L0_8x16
	if (bits == 15) return 22;                                       // B_8x8
	if (bits != 13) {
		bits = (bits << 1) + CABAC_DecodeBin(cabac, 27 + 5);
		return bits - 4;                                             // B_L0_Bi_* .. B_Bi_Bi_*
	}

	// Intra macroblock inside a B slice: ctxIdxOffset 32.
	if (CABAC_DecodeBin(cabac, 32) == 0) {
		*intra_cbp_luma = 0;
		*intra_cbp_chroma = 0;
		*intra16_mode = 0;
		return -1;                                                   // I_NxN
	}
	if (CABAC_DecodeTerminate(cabac)) {
		*is_pcm = true;
		return -1;
	}
	int luma = CABAC_DecodeBin(cabac, 33) ? 15 : 0;
	int chroma = 0;
	if (CABAC_DecodeBin(cabac, 34)) chroma = CABAC_DecodeBin(cabac, 34) ? 2 : 1;
	// Both mode bins use the same context and are MSB first; sequence the reads explicitly,
	// because the operand order of `(a << 1) | b` is unspecified.
	int b_mode_msb = CABAC_DecodeBin(cabac, 35);
	int b_mode_lsb = CABAC_DecodeBin(cabac, 35);
	int mode = (b_mode_msb << 1) | b_mode_lsb;
	*intra_cbp_luma = luma;
	*intra_cbp_chroma = chroma;
	*intra16_mode = mode;
	*intra_is_16 = true;
	return -1;
}

// H.264 7.3.5.3 sub_mb_type of a B slice (ctxIdxOffset 36). 0 is B_Direct_8x8, 1..3 are the
// 8x8 modes, 4..6 the 8x4 modes, 7..9 the 4x8 modes and 10..12 the 4x4 modes.
static inline int CABAC_DecodeBSubMbType(CABACDecoder* cabac) {
	if (CABAC_DecodeBin(cabac, 36) == 0) return 0;
	if (CABAC_DecodeBin(cabac, 37) == 0) return 1 + CABAC_DecodeBin(cabac, 39);
	int type = 3;
	if (CABAC_DecodeBin(cabac, 38)) {
		if (CABAC_DecodeBin(cabac, 39)) return 11 + CABAC_DecodeBin(cabac, 39);
		type += 4;
	}
	type += 2 * CABAC_DecodeBin(cabac, 39);
	type += CABAC_DecodeBin(cabac, 39);
	return type;
}

// Geometry and list usage of one sub-partition of a B 8x8 sub-macroblock.
static inline void H264_BSubPartInfo(int st, int part, int* use0, int* use1, int* w4, int* h4) {
	// H.264 Table 7-14.  The three sub-macroblock types of a
	// group share the same partition shape, and the two-partition shapes alternate: an even
	// type is 16x8 (two horizontal halves, stacked vertically) and an odd type is 8x16 (two
	// vertical halves, side by side).
	if (st <= 3) {            // B_Direct_8x8, B_L0/L1/Bi_8x8
		*w4 = 2; *h4 = 2;
	} else if (st <= 9) {     // B_L0/L1/Bi_8x4 and B_L0/L1/Bi_4x8
		if (st & 1) { *w4 = 1; *h4 = 2; }   // 8x16: two 4x8 partitions
		else        { *w4 = 2; *h4 = 1; }   // 16x8: two 8x4 partitions
	} else {                  // B_L0/L1/Bi_4x4
		*w4 = 1; *h4 = 1;
	}
	// List usage: 0 = list0 only, 1 = list1 only, 2 = bi-predictive.  The two-partition
	// types are declared as L0, L0, L1, L1, Bi, Bi, so the remainder modulo three of the
	// sub-macroblock type only describes the 8x8 and 4x4 groups.
	int m;
	if (st <= 3) m = st - 1;
	else if (st <= 9) m = (st <= 5) ? 0 : ((st <= 7) ? 1 : 2);
	else m = st - 10;
	*use0 = (m != 1) ? 1 : 0;
	*use1 = (m != 0) ? 1 : 0;
	(void)part;
}

// List usage bits of a B mb_type: bit 2p = list0 used by partition p, bit 2p+1 = list1.
static const uint8 h264_b_mb_lists[23] = {
	0x0, 0x1, 0x2, 0x3,          // 0 B_Direct_16x16, 1..3 B_L0/L1/Bi_16x16
	0x5, 0x5, 0xA, 0xA,          // 4..7   B_L0_L0_16x8, B_L0_L0_8x16, B_L1_L1_*
	0x9, 0x9, 0x6, 0x6,          // 8..11  B_L0_L1_*, B_L1_L0_*
	0xD, 0xD, 0xE, 0xE,          // 12..15 B_L0_Bi_*, B_L1_Bi_*
	0x7, 0x7, 0xB, 0xB,          // 16..19 B_Bi_L0_*, B_Bi_L1_*
	0xF, 0xF,                    // 20..21 B_Bi_Bi_16x8, B_Bi_Bi_8x16
	0x0                          // 22     B_8x8 (handled through sub_mb_type)
};

// Reference index of a neighbouring 4x4 block, with the "list not used" marker (255) reported as
// the negative value the spatial direct derivation expects.
static inline int H264_RefIdxOrUnused(uint8 stored) {
	return (stored == 255) ? -1 : (int)stored;
}

// H.264 8.4.1.2 direct prediction of a whole macroblock. Spatial direct takes the reference
// indices and predicted motion vectors from the neighbours of the *macroblock* (8.4.1.2.2
// invokes 8.4.1.3.2 with mbPartIdx = 0 and subMbPartIdx = 0, which fixes predPartWidth to 16,
// so the derivation is not done per 8x8 partition and the result is the same for every 4x4 block
// of the picture; direct_8x8_inference_flag only affects subMvCnt).  Temporal direct scales the
// collocated picture's motion vector by the POC distance, per 8x8 partition.
static void H264_BuildDirectMB(
	bool spatial,
	const H264RefPicDesc* colpic,
	int cur_poc, const H264RefPicDesc* pics0, const H264RefPicDesc* pics1,
	uint32 mb_idx, uint32 mb_width, uint32 mb_x, uint32 mb_y,
	bool hasA, bool hasB,
	const H264MotionVector* tmv0, const uint8* tref0,
	const H264MotionVector* tmv1, const uint8* tref1,
	const uint8* nv_direct4,
	int n0_use, int n1_use, int sq,
	H264MotionVector* out_mv0, uint8* out_ref0,
	H264MotionVector* out_mv1, uint8* out_ref1
) {
	const uint32 cur_base = mb_idx * 16;
	H264MotionVector zero = { 0, 0 };

	if (spatial) {
		// Macroblock level neighbours (H.264 Table 6-2 with predPartWidth = 16, resolved through
		// 6.4.8 for positions outside the macroblock): A is the left macroblock's block (3, 0),
		// B the block above (0, 3), C the above-right macroblock's block (0, 3) and D the
		// above-left macroblock's block (3, 3).
		int ia = (mb_x > 0) ? (int)(mb_idx - 1) * 16 + 3 : -1;
		int ib = (mb_y > 0) ? (int)(mb_idx - mb_width) * 16 + 12 : -1;
		int ic = ((mb_y > 0) && (mb_x + 1 < mb_width)) ? (int)(mb_idx - mb_width + 1) * 16 + 12 : -1;
		int id = ((mb_x > 0) && (mb_y > 0)) ? (int)(mb_idx - mb_width - 1) * 16 + 15 : -1;

		H264MvpNeighbour nA, nB, nC, nD;
		H264_LoadNeighbour(tmv0, tref0, ia, 0, false, &nA);
		H264_LoadNeighbour(tmv0, tref0, ib, 0, false, &nB);
		H264_LoadNeighbour(tmv0, tref0, ic, 0, false, &nC);
		H264_LoadNeighbour(tmv0, tref0, id, 0, false, &nD);
		bool availA = (ia >= 0), availB = (ib >= 0), availC = (ic >= 0);
		if (!availC) { nC = nD; availC = (id >= 0); }    // 8.4.1.3.2 step 3
		if (!availB && !availC && availA) { nB = nA; nC = nA; }
		int r0 = H264_MinPositive(nA.ref, H264_MinPositive(nB.ref, nC.ref));

		H264MvpNeighbour oA, oB, oC, oD;
		H264_LoadNeighbour(tmv1, tref1, ia, 0, false, &oA);
		H264_LoadNeighbour(tmv1, tref1, ib, 0, false, &oB);
		H264_LoadNeighbour(tmv1, tref1, ic, 0, false, &oC);
		H264_LoadNeighbour(tmv1, tref1, id, 0, false, &oD);
		bool availA1 = (ia >= 0), availB1 = (ib >= 0), availC1 = (ic >= 0);
		if (!availC1) { oC = oD; availC1 = (id >= 0); }
		if (!availB1 && !availC1 && availA1) { oB = oA; oC = oA; }
		int r1 = H264_MinPositive(oA.ref, H264_MinPositive(oB.ref, oC.ref));

		// H.264 8.4.1.2.1: the direct reference index of each list is the minimum positive one of
		// that list's three neighbours; MinPositive() yields -1 when no neighbouring partition uses
		// the list at all.  A list that no neighbour uses therefore carries no information: the
		// macroblock predicts from the other list alone and the unused list has to be stored as
		// unused (255).  Marking it as "reference index 0 with a zero motion vector" instead made
		// every later partition that reads this macroblock as a neighbour take that zero vector
		// through the 8.4.1.3 predictor - measured on this stream it wrecked whole B pictures
		// (display 3: Y 2.246 -> 0.129).  Only when BOTH lists lack a neighbour is there no
		// information at all, and then both are 0 with zero motion vectors.
		const bool had0 = (r0 >= 0);
		const bool had1 = (r1 >= 0);
		// H.264 8.4.1.2.2 step 5: directZeroPredictionFlag is set only when NEITHER list gets a
		// reference index from the neighbours.  When exactly one list is unused, Table 8-9 still
		// predicts from the other list, and its motion vector is the 8.4.1.3 neighbour predictor -
		// clearing the flag for a single missing list forced both vectors to zero and threw that
		// predictor away (a B picture then kept a 5x8 macroblock cluster around a moving object at
		// errors of 40..70 while its neighbours were exact).
		bool direct_zero = false;
		bool use0 = had0, use1 = had1;
		if (!had0 && !had1) {
			r0 = 0;
			r1 = 0;
			direct_zero = true;
			use0 = use1 = true;
		} else {
			// A list that no neighbour uses keeps the prediction flag 0; its reference index is
			// irrelevant and only has to stay in range.
			if (!had0) r0 = 0;
			if (!had1) r1 = 0;
		}
		if (r0 >= n0_use) r0 = 0;
		if (r1 >= n1_use) r1 = 0;

		// colZeroFlag (H.264 8.4.1.2.2): the collocated partition of RefPicList1[0] uses list0
		// with reference index 0 and a motion vector of at most a quarter sample, so a direct
		// partition that predicts from reference index 0 of a list takes a zero motion vector
		// instead of the neighbouring predictor.  The collocated partition is partition 0 of the
		// collocated macroblock (predPartWidth = 16 when direct_8x8_inference_flag is set) and
		// every reference picture of this stream is short term.
		bool col_zero = false;
		if (colpic && colpic->refidx) {
			const int czc = (int)cur_base;
			const H264MotionVector czmv = colpic->mv ? colpic->mv[czc] : zero;
			(void)czmv;
			// H.264 8.4.1.2.2 takes refIdxCol and mvCol from the process of 8.4.1.2.1, which
			// selects the list the collocated partition actually uses: its list0 entry, or its
			// list1 entry when the partition does not use list0 (list0 is tested first, then
			// list1).  Looking only at list0 left colZeroFlag at 0 whenever the collocated
			// partition was coded L1-only, so a
			// direct partition that should have taken a zero motion vector took the neighbour
			// predictor instead.
			const int czr0 = (int)colpic->refidx[czc];
			const int czr1 = colpic->refidx1 ? (int)colpic->refidx1[czc] : 255;
			const H264MotionVector* czp = nullptr;
			if (czr0 == 0) czp = colpic->mv ? &colpic->mv[czc] : nullptr;
			else if (czr0 < 0 && czr1 == 0) czp = colpic->mv1 ? &colpic->mv1[czc] : nullptr;
			if (czp) col_zero = (czp->x >= -1) && (czp->x <= 1) && (czp->y >= -1) && (czp->y <= 1);
		}

		H264MotionVector m0 = zero, m1 = zero;
		const bool u0 = use0;
		const bool u1 = use1;
		if (u0 && !(direct_zero || (r0 == 0 && col_zero))) {
			m0 = H264_GetMVPForPartition(tmv0, mb_idx, mb_width, mb_x, mb_y, 0, 0, 4, 4, tref0, r0);
		}
		if (u1 && !(direct_zero || (r1 == 0 && col_zero))) {
			m1 = H264_GetMVPForPartition(tmv1, mb_idx, mb_width, mb_x, mb_y, 0, 0, 4, 4, tref1, r1);
		}

		for (int k = 0; k < 16; ++k) {
			out_mv0[cur_base + k] = m0;
			out_mv1[cur_base + k] = m1;
			out_ref0[cur_base + k] = u0 ? (uint8)r0 : (uint8)255;
			out_ref1[cur_base + k] = u1 ? (uint8)r1 : (uint8)255;
		}
		(void)sq;
		return;
	}

	for (int sy = 0; sy < 4; sy += sq) {
		for (int sx = 0; sx < 4; sx += sq) {
			int r0 = 0, r1 = 0;
			bool u0 = true, u1 = true;
			H264MotionVector m0 = zero, m1 = zero;


			// Temporal direct (H.264 8.4.1.2.1, 8.4.1.2.3).  mvCol and refIdxCol are taken from
			// the collocated partition of RefPicList1[0]: its list0 entry when the partition uses
			// list0, its list1 entry when the partition uses list1 only, and a zero vector with
			// refIdxCol = -1 when it is intra coded or uses neither list (both stored entries are
			// 255 then).  The reference index of the chosen list does not have to be 0.
			const int cidx = (int)cur_base + sy * 4 + sx;
			const int cri0 = (colpic && colpic->refidx) ? (int)colpic->refidx[cidx] : 255;
			const int cri1 = (colpic && colpic->refidx1) ? (int)colpic->refidx1[cidx] : 255;
			int col_ref = -1;
			H264MotionVector col_mv = zero;
			if (cri0 != 255) {
				col_ref = cri0;
				col_mv = colpic->mv[cidx];
			} else if (cri1 != 255) {
				col_ref = cri1;
				col_mv = (colpic->mv1) ? colpic->mv1[cidx] : zero;
			}

			// 8.4.1.2.3 for a frame picture: refIdxL0 = MapColToList0(refIdxCol) (below),
			// refIdxL1 = 0, and - Table 8-9, both reference indices >= 0 - the flags predFlagL0
			// and predFlagL1 are both 1: a temporal direct partition always predicts from BOTH
			// lists.  Marking list0 as unused whenever refIdxCol was not 0 dropped half of the
			// prediction of whole regions.
			r0 = 0;
			if (col_ref >= 0) {
				// H.264 8.4.1.2.3 MapColToList0(): refIdxCol indexes the collocated picture's own
				// list0, so the current list0 entry that names the same PICTURE is the one the
				// prediction uses.  Reusing refIdxCol unchanged is only correct while both lists
				// are ordered identically, which they are not between a P picture and the B
				// picture that follows it; taking that wrong picture made previously exact
				// macroblocks wrong.
				const int refpoc = (colpic->list0_poc && col_ref < colpic->n_list0)
								   ? colpic->list0_poc[col_ref] : -1;
				if (refpoc >= 0) {
					for (int q = 0; q < n0_use; ++q) {
						if (pics0[q].poc == refpoc) { r0 = q; break; }
					}
				}
			}
			r1 = 0;
			if (r0 >= n0_use) r0 = 0;
			// Equations 8-192/8-193: no scaling when the list1[0] picture and the list0 picture
			// the collocated reference index selects are the same picture.
			const int poc0 = pics0[r0].poc;
			const int poc1 = pics1[0].poc;
			if (poc1 - poc0 == 0) {
				m0 = col_mv;
				m1 = zero;
			} else {
				const int dsf = H264_DistScaleFactor(poc0, poc1, cur_poc);
				m0.x = (int16)H264_Clip3i(-32768, (dsf * (int)col_mv.x + 128) >> 8, 32767);
				m0.y = (int16)H264_Clip3i(-32768, (dsf * (int)col_mv.y + 128) >> 8, 32767);
				m1.x = (int16)(m0.x - col_mv.x);
				m1.y = (int16)(m0.y - col_mv.y);
			}


			for (int yy = 0; yy < sq; ++yy) {
				for (int xx = 0; xx < sq; ++xx) {
					int idx = (int)cur_base + (sy + yy) * 4 + sx + xx;
					out_mv0[idx] = m0;
					out_mv1[idx] = m1;
					out_ref0[idx] = u0 ? (uint8)r0 : (uint8)255;
					out_ref1[idx] = u1 ? (uint8)r1 : (uint8)255;
				}
			}
		}
	}
}

// Emits the prediction partitions of one direct coded 8x8 sub-block out of the derived tables.
static inline void H264_EmitDirect8x8(int i8, int sq,
									  const H264MotionVector* tmv0, const uint8* tref0,
									  const H264MotionVector* tmv1, const uint8* tref1,
									  uint32 cur_base, H264BPredPart* parts, int* nparts) {
	const int sx0 = (i8 & 1) * 2;
	const int sy0 = (i8 >> 1) * 2;
	for (int sy = sy0; sy < sy0 + 2; sy += sq) {
		for (int sx = sx0; sx < sx0 + 2; sx += sq) {
			if (*nparts >= H264_MAX_B_PARTS) return;
			int idx = (int)cur_base + sy * 4 + sx;
			H264BPredPart& p = parts[*nparts];
			++(*nparts);
			p.x4 = sx; p.y4 = sy; p.w4 = sq; p.h4 = sq;
			// 255 in the reference table marks a list the direct prediction does not use, so
			// the partition has to be reconstructed from the other list alone.
			p.use0 = (tref0[idx] != 255);
			p.use1 = (tref1[idx] != 255);
			p.ref0 = p.use0 ? tref0[idx] : 0;
			p.ref1 = p.use1 ? tref1[idx] : 0;
			p.mv0 = tmv0[idx]; p.mv1 = tmv1[idx];
		}
	}
}

// Motion compensation of one plane region; a bi-predictive region blends both references with
// the implicit weights.
static void H264_PredRegion(
	uint8* dst, int dst_stride, int w, int h,
	const uint8* p0, int s0, const uint8* p1, int s1,
	int src_x, int src_y,
	int mv0x, int mv0y, int mv1x, int mv1y,
	int pw, int ph, bool chroma,
	bool use0, bool use1, int w0, int w1
) {
	if (use0 && !use1) {
		if (chroma) H264_MC_Chroma(dst, dst_stride, p0, s0, w, h, src_x, src_y, mv0x, mv0y, pw, ph);
		else H264_MC_Luma(dst, dst_stride, p0, s0, w, h, src_x, src_y, mv0x, mv0y, pw, ph);
		return;
	}
	if (!use0 && use1) {
		if (chroma) H264_MC_Chroma(dst, dst_stride, p1, s1, w, h, src_x, src_y, mv1x, mv1y, pw, ph);
		else H264_MC_Luma(dst, dst_stride, p1, s1, w, h, src_x, src_y, mv1x, mv1y, pw, ph);
		return;
	}
	uint8 ta[256], tb[256];
	if (chroma) {
		H264_MC_Chroma(ta, w, p0, s0, w, h, src_x, src_y, mv0x, mv0y, pw, ph);
		H264_MC_Chroma(tb, w, p1, s1, w, h, src_x, src_y, mv1x, mv1y, pw, ph);
	} else {
		H264_MC_Luma(ta, w, p0, s0, w, h, src_x, src_y, mv0x, mv0y, pw, ph);
		H264_MC_Luma(tb, w, p1, s1, w, h, src_x, src_y, mv1x, mv1y, pw, ph);
	}
	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			dst[y * dst_stride + x] = Clip8((ta[y * w + x] * w0 + tb[y * w + x] * w1 + 32) >> 6);
		}
	}
}

static bool H264_DecodeSlice_CABAC_B_skip(
	const byte* rbsp_data,
	size_t rbsp_len,
	const H264SPS* sps,
	const H264PPS* pps,
	uint8 nal_unit_type,
	uint8 nal_ref_idc,
	const H264RefPicDesc* list0,
	const H264RefPicDesc* list1,
	int n0_avail,
	int n1_avail,
	int cur_poc,
	uint8* dst_y,
	uint8* dst_u,
	uint8* dst_v,
	int dst_y_stride,
	int dst_uv_stride,
	uint8* mb_qp_out,
	// Per-macroblock deblocking export (H.264 8.7.2.1), see H264_DeblockFrame for the layout.
	uint8* db_flags_out,
	int16* db_mv_out,
	int16* db_ref_out,
	// Motion data of this picture, kept so that the temporal direct prediction of a later B
	// slice can read the collocated partitions (H.264 8.4.1.2.1).
	H264MotionVector* mv_out,
	uint8* refidx_out,
	H264MotionVector* mv1_out,
	uint8* refidx1_out
) {
	if (!rbsp_data || rbsp_len == 0 || !sps || !pps || !dst_y || !dst_u || !dst_v) return false;
	// A slice that has no weighted prediction of its own must not inherit the weights the previous
	// slice selected, and a B slice never scales its two predictions here: it uses the implicit
	// weights computed below instead.
	H264_WP_IDENTITY();
	if (!list0 || !list1 || n0_avail < 1 || n1_avail < 1) return false;
	// Explicit weighted bi-prediction (weighted_bipred_idc == 1) IS signalled in this stream, but
	// every luma_weight/chroma_weight flag in its pred_weight_table is 0, which per H.264 7.4.3.2
	// means "use the default weight", i.e. a plain unweighted prediction. Refusing such a slice
	// only threw the whole picture away: the caller then substituted a copy of a reference picture
	// (DecodeFrame: "MemCopyN(cur_y, ref_y, ...); repeated = true;"), which is why whole regions of
	// those B pictures came out with errors of 255. Decode them with default weights instead.
	// TODO: retain pred_weight_table in H264SliceHeader and apply it when a flag is set; decoding
	// unweighted is still closer than replacing the picture, so this is never a regression.

	H264BitReader br;
	H264_InitBitReader(&br, rbsp_data, rbsp_len);
	H264SliceHeader sh;
	if (!H264_ParseSliceHeader(&br, sps, pps, nal_unit_type, nal_ref_idc, &sh)) return false;
	if ((sh.slice_type % 5) != H264_SLICE_B) return false;

	const uint32 mb_width = sps->mb_width;
	const uint32 mb_height = sps->mb_height;
	const uint32 num_mbs = mb_width * mb_height;
	if (mb_width == 0 || mb_height == 0 || sh.first_mb_in_slice >= num_mbs) return false;

	// The number of active references is what the encoder used to decide whether ref_idx is
	// present at all; it is clamped to the references this decoder actually holds.
	const int n0_active = (int)sh.num_ref_idx_l0_active_minus1 + 1;
	const int n1_active = (int)sh.num_ref_idx_l1_active_minus1 + 1;
	const bool ref0_coded = (n0_active > 1);
	const bool ref1_coded = (n1_active > 1);
	const int n0_use = (n0_active < n0_avail) ? n0_active : n0_avail;
	const int n1_use = (n1_active < n1_avail) ? n1_active : n1_avail;
	const int sq = sps->direct_8x8_inference_flag ? 2 : 1;
	const bool spatial_direct = (sh.direct_spatial_mv_pred_flag != 0);

	const size_t n16 = (size_t)num_mbs * 16;
	uint8* cbp_table = (uint8*)malloc(num_mbs);
	uint8* cbp_chroma_table = (uint8*)malloc(num_mbs);
	int8* mode_table = (int8*)malloc(n16);
	uint8* intra_chroma_table = (uint8*)malloc(num_mbs);
	uint8* nnz_table = (uint8*)malloc((size_t)num_mbs * 24);
	uint8* transform_8x8_table = (uint8*)malloc(num_mbs);
	uint8* mb_skip_table = (uint8*)malloc(num_mbs);
	uint8* direct_table = (uint8*)malloc(num_mbs);
	uint8* nv_direct4 = (uint8*)malloc(n16);
	uint8* nz_dc_table = (uint8*)malloc((size_t)num_mbs * 3);
	H264MotionVector* mv0 = (H264MotionVector*)malloc(n16 * sizeof(H264MotionVector));
	H264MotionVector* mv1 = (H264MotionVector*)malloc(n16 * sizeof(H264MotionVector));
	int16* mvd0 = (int16*)malloc(n16 * 2 * sizeof(int16));
	int16* mvd1 = (int16*)malloc(n16 * 2 * sizeof(int16));
	uint8* ref0 = (uint8*)malloc(n16);
	uint8* ref1 = (uint8*)malloc(n16);

	if (!cbp_table || !cbp_chroma_table || !mode_table || !intra_chroma_table || !nnz_table ||
		!transform_8x8_table || !mb_skip_table || !direct_table || !nv_direct4 ||
		!nz_dc_table || !mv0 || !mv1 || !mvd0 || !mvd1 || !ref0 || !ref1) {
		if (cbp_table) free(cbp_table);
		if (cbp_chroma_table) free(cbp_chroma_table);
		if (mode_table) free(mode_table);
		if (intra_chroma_table) free(intra_chroma_table);
		if (nnz_table) free(nnz_table);
		if (transform_8x8_table) free(transform_8x8_table);
		if (mb_skip_table) free(mb_skip_table);
		if (direct_table) free(direct_table);
		if (nv_direct4) free(nv_direct4);
		if (nz_dc_table) free(nz_dc_table);
		if (mv0) free(mv0);
		if (mv1) free(mv1);
		if (mvd0) free(mvd0);
		if (mvd1) free(mvd1);
		if (ref0) free(ref0);
		if (ref1) free(ref1);
		return false;
	}

	MemSet(cbp_table, 0, num_mbs);
	MemSet(cbp_chroma_table, 0, num_mbs);
	MemSet(mode_table, -1, n16);
	MemSet(intra_chroma_table, 0, num_mbs);
	MemSet(nnz_table, 0, (size_t)num_mbs * 24);
	MemSet(transform_8x8_table, 0, num_mbs);
	MemSet(mb_skip_table, 0, num_mbs);
	MemSet(direct_table, 0, num_mbs);
	MemSet(nv_direct4, 0, n16);
	MemSet(nz_dc_table, 0, (size_t)num_mbs * 3);
	MemSet(mv0, 0, n16 * sizeof(H264MotionVector));
	MemSet(mv1, 0, n16 * sizeof(H264MotionVector));
	MemSet(mvd0, 0, n16 * 2 * sizeof(int16));
	MemSet(mvd1, 0, n16 * 2 * sizeof(int16));
	MemSet(ref0, 0, n16);
	MemSet(ref1, 0, n16);

	// The deblocking filter has to know which macroblocks are intra, and a B slice marks the
	// reference index of an intra macroblock as 255 (i.e. "list not used"), which is exactly what
	// an inter macroblock that predicts from neither list looks like.
	uint8* db_intra_tab = nullptr;
	if (db_flags_out) {
		db_intra_tab = (uint8*)malloc(num_mbs);
		if (db_intra_tab) MemSet(db_intra_tab, 0, num_mbs);
		else db_flags_out = nullptr;
	}

	const size_t bit_pos = (((br.byte_pos * 8) - br.bits_left) + 7) & ~(size_t)7;
	CABACDecoder cabac;
	CABAC_Init(&cabac, rbsp_data, rbsp_len, bit_pos, (int)H264_SLICE_B, sh.cabac_init_idc, sh.slice_qp);
	H264Probe_Reset((int)H264_SLICE_B, rbsp_len * 8);

	const int pic_w = (int)sps->width;
	const int pic_h = (int)sps->height;
	const int pic_w_c = pic_w / 2;
	const int pic_h_c = pic_h / 2;

	int current_qp = sh.slice_qp;
	bool prev_mb_has_qp_delta = false;
	bool failed = false;
	int fail_reason = 0;
	int terminated = 0;

	for (uint32 mb_idx = sh.first_mb_in_slice; mb_idx < num_mbs; ++mb_idx) {
		if (cabac.overrun) break;
		H264Probe_MB(mb_idx);
		const uint32 mb_x = mb_idx % mb_width;
		const uint32 mb_y = mb_idx / mb_width;
		const bool hasA = (mb_x > 0);
		const bool hasB = (mb_y > 0);
		const uint32 cur_base = mb_idx * 16;

		const int src_x = (int)mb_x * 16;
		const int src_y = (int)mb_y * 16;
		uint8* mb_y_dst = dst_y + src_y * dst_y_stride + src_x;
		uint8* mb_u_dst = dst_u + (src_y / 2) * dst_uv_stride + (src_x / 2);
		uint8* mb_v_dst = dst_v + (src_y / 2) * dst_uv_stride + (src_x / 2);

		uint8 pred_y[256], pred_u[64], pred_v[64];
		MemSet(pred_y, 0, sizeof(pred_y));
		MemSet(pred_u, 0, sizeof(pred_u));
		MemSet(pred_v, 0, sizeof(pred_v));

		// mb_skip_flag (ctxIdxOffset 24 for B slices): the context is 24 + the number of
		// neighbouring macroblocks that are available and were not coded as *skip*.  A
		// B_Direct_16x16 neighbour still contributes, because its mb_skip_flag is 0.
		const int ctx_skip = 24 + (hasA && !mb_skip_table[mb_idx - 1] ? 1 : 0)
							   + (hasB && !mb_skip_table[mb_idx - mb_width] ? 1 : 0);
		const int mb_skip = CABAC_DecodeBin(&cabac, ctx_skip);

		if (mb_skip) {
			mb_skip_table[mb_idx] = 1;
			direct_table[mb_idx] = 1;
			H264_BuildDirectMB(spatial_direct, &list1[0], cur_poc, list0, list1,
							   mb_idx, mb_width, mb_x, mb_y, hasA, hasB,
							   mv0, ref0, mv1, ref1, nv_direct4,
							   n0_use, n1_use, sq, mv0, ref0, mv1, ref1);
			for (int k = 0; k < 16; ++k) nv_direct4[cur_base + k] = 1;

			int nparts = 0;
			H264BPredPart parts[H264_MAX_B_PARTS];
			for (int i = 0; i < 4; ++i) {
				H264_EmitDirect8x8(i, sq, mv0, ref0, mv1, ref1, cur_base, parts, &nparts);
			}

			for (int p = 0; p < nparts; ++p) {
				int r0i = parts[p].ref0, r1i = parts[p].ref1;
				if (r0i >= n0_use) r0i = 0;
				if (r1i >= n1_use) r1i = 0;
				const H264RefPicDesc& r0 = list0[r0i];
				const H264RefPicDesc& r1 = list1[r1i];
				int w0 = 32, w1 = 32;
				H264_ImplicitWeights(pps->weighted_bipred_idc, cur_poc, r0.poc, r1.poc, &w0, &w1);
				int lx = parts[p].x4 * 4, ly = parts[p].y4 * 4;
				H264_PredRegion(pred_y + ly * 16 + lx, 16, parts[p].w4 * 4, parts[p].h4 * 4,
								r0.y, r0.y_stride, r1.y, r1.y_stride,
								src_x + lx, src_y + ly, parts[p].mv0.x, parts[p].mv0.y,
								parts[p].mv1.x, parts[p].mv1.y, pic_w, pic_h, false,
								parts[p].use0, parts[p].use1, w0, w1);
				int cx = parts[p].x4 * 2, cy = parts[p].y4 * 2;
				H264_PredRegion(pred_u + cy * 8 + cx, 8, parts[p].w4 * 2, parts[p].h4 * 2,
								r0.u, r0.uv_stride, r1.u, r1.uv_stride,
								src_x / 2 + cx, src_y / 2 + cy, parts[p].mv0.x, parts[p].mv0.y,
								parts[p].mv1.x, parts[p].mv1.y, pic_w_c, pic_h_c, true,
								parts[p].use0, parts[p].use1, w0, w1);
				H264_PredRegion(pred_v + cy * 8 + cx, 8, parts[p].w4 * 2, parts[p].h4 * 2,
								r0.v, r0.uv_stride, r1.v, r1.uv_stride,
								src_x / 2 + cx, src_y / 2 + cy, parts[p].mv0.x, parts[p].mv0.y,
								parts[p].mv1.x, parts[p].mv1.y, pic_w_c, pic_h_c, true,
								parts[p].use0, parts[p].use1, w0, w1);
			}

			// A direct/skipped macroblock carries no residual, so the prediction *is* the
			// reconstruction and has to be stored. Bounding it to the local prediction buffers
			// would leave the previous picture's samples in place.
			for (int yy = 0; yy < 16; ++yy) {
				for (int xx = 0; xx < 16; ++xx) {
					mb_y_dst[yy * dst_y_stride + xx] = pred_y[yy * 16 + xx];
				}
			}
			for (int yy = 0; yy < 8; ++yy) {
				for (int xx = 0; xx < 8; ++xx) {
					mb_u_dst[yy * dst_uv_stride + xx] = pred_u[yy * 8 + xx];
					mb_v_dst[yy * dst_uv_stride + xx] = pred_v[yy * 8 + xx];
				}
			}

			if (mb_qp_out) mb_qp_out[mb_idx] = (uint8)current_qp;
			prev_mb_has_qp_delta = false;
			if (CABAC_DecodeTerminate(&cabac)) { terminated = 1; break; }
			continue;
		}

		int intra_cbp_luma = 0, intra_cbp_chroma = 0, intra16_mode = 0;
		bool intra_is_16 = false;
		bool is_pcm = false;
		// A neighbour raises the mb_type context only when it is available and was coded as
		// something other than B_Skip / B_Direct_16x16.  An unavailable neighbour contributes
		// nothing at all (the availability flag is tested before the type test).
		const bool left_coded = hasA && !direct_table[mb_idx - 1] && !mb_skip_table[mb_idx - 1];
		const bool top_coded = hasB && !direct_table[mb_idx - mb_width] && !mb_skip_table[mb_idx - mb_width];
		int b_type = CABAC_DecodeBMbType(&cabac, left_coded, top_coded,
										 &intra_cbp_luma, &intra_cbp_chroma, &intra16_mode,
										 &intra_is_16, &is_pcm);
		if (is_pcm) {
			failed = true;
			fail_reason = 4;
			break;
		}

		if (b_type < 0) {
			if (db_intra_tab) db_intra_tab[mb_idx] = 1;
			// H.264 8.4.1.2.1 and 8.4.1.3.2: an intra macroblock uses neither list, and both
			// stored reference tables have to say so.  A zero-filled entry instead reads as an
			// inter partition that predicts reference 0 with a zero motion vector, which made
			// the neighbours of a spatial direct macroblock take that vector through the
			// 8.4.1.3.2 predictor and made a temporal collocated partition see refIdxCol = 0.
			CABAC_StoreRef(ref0, mb_idx, 0, 0, 4, 4, 255);
			CABAC_StoreRef(ref1, mb_idx, 0, 0, 4, 4, 255);
			// Intra macroblock inside a B slice: mb_type carried the intra type and, for
			// I_16x16, its coded block pattern and prediction mode. The remaining syntax
			// (prediction modes, chroma prediction mode, mb_qp_delta and the residual) is read
			// here and the macroblock is reconstructed in place from its neighbours.
			H264IntraMBArgs ia;
			MemSet(&ia, 0, sizeof(ia));
			ia.cabac = &cabac;
			ia.pps = pps;
			ia.mb_idx = mb_idx;
			ia.mb_width = mb_width;
			ia.mb_x = mb_x;
			ia.hasA = hasA;
			ia.hasB = hasB;
			ia.mode_table = mode_table;
			ia.transform_8x8_table = transform_8x8_table;
			ia.intra_chroma_table = intra_chroma_table;
			ia.cbp_table = cbp_table;
			ia.cbp_chroma_table = cbp_chroma_table;
			ia.nnz_table = nnz_table;
			ia.nz_dc_table = nz_dc_table;
			ia.mb_qp_out = mb_qp_out;
			ia.dst_y = mb_y_dst;
			ia.dst_u = mb_u_dst;
			ia.dst_v = mb_v_dst;
			ia.dst_y_stride = dst_y_stride;
			ia.dst_uv_stride = dst_uv_stride;
			ia.is_intra16 = intra_is_16;
			ia.cbp_luma_in = intra_cbp_luma;
			ia.cbp_chroma_in = intra_cbp_chroma;
			ia.intra16_mode_in = intra16_mode;
			ia.qp = &current_qp;
			ia.prev_mb_has_qp_delta = &prev_mb_has_qp_delta;
			CABAC_DecodeIntraMB(ia);

			if (CABAC_DecodeTerminate(&cabac)) {
				terminated = 1;
				break;
			}
			continue;
		}
		H264BPredPart parts[H264_MAX_B_PARTS];
		int nparts = 0;
		bool dct8x8_allowed = true;
		const bool is_direct_mb = (b_type == 0);

		if (is_direct_mb) {
			direct_table[mb_idx] = 1;
			H264_BuildDirectMB(spatial_direct, &list1[0], cur_poc, list0, list1,
							   mb_idx, mb_width, mb_x, mb_y, hasA, hasB,
							   mv0, ref0, mv1, ref1, nv_direct4,
							   n0_use, n1_use, sq, mv0, ref0, mv1, ref1);
			for (int i = 0; i < 4; ++i) {
				H264_EmitDirect8x8(i, sq, mv0, ref0, mv1, ref1, cur_base, parts, &nparts);
			}
			for (int k = 0; k < 16; ++k) nv_direct4[cur_base + k] = 1;
		} else if (b_type == 22) {
			// B_8x8: four independent sub-macroblock types.
			int sub[4];
			bool any_direct = false;
			for (int i = 0; i < 4; ++i) {
				sub[i] = CABAC_DecodeBSubMbType(&cabac);
				if (sub[i] == 0) any_direct = true;
				// H.264 7.3.5: transform_size_8x8_flag is present when no sub-macroblock partition
				// is smaller than 8x8.  A B_Direct_8x8 sub-macroblock has no explicit size, so it
				// counts as 8x8 only when direct_8x8_inference_flag is set; only otherwise does it
				// disqualify the flag.  Treating a direct sub-macroblock as "smaller than 8x8"
				// unconditionally drops the flag bin and desynchronises every macroblock after it.
				const bool sub_is_8x8 = (sub[i] == 1 || sub[i] == 2 || sub[i] == 3) ||
										(sub[i] == 0 && sps->direct_8x8_inference_flag);
				if (!sub_is_8x8) dct8x8_allowed = false;
			}
			if (any_direct) {
				H264_BuildDirectMB(spatial_direct, &list1[0], cur_poc, list0, list1,
								   mb_idx, mb_width, mb_x, mb_y, hasA, hasB,
								   mv0, ref0, mv1, ref1, nv_direct4,
								   n0_use, n1_use, sq, mv0, ref0, mv1, ref1);
			}
			// The direct sub-macroblocks have to be marked before the reference indices of the
			// remaining sub-macroblocks are read: they are neighbours for that context.
			for (int i = 0; i < 4; ++i) {
				if (sub[i] != 0) continue;
				int sx = (i & 1) * 2, sy = (i >> 1) * 2;
				for (int yy = 0; yy < 2; ++yy)
					for (int xx = 0; xx < 2; ++xx)
						nv_direct4[cur_base + (sy + yy) * 4 + sx + xx] = 1;
			}

			// Reference indices: every sub-macroblock of list0 first, then those of list1
			// (H.264 7.3.5 orders all reference indices before all motion vector differences).
			for (int list = 0; list < 2; ++list) {
				const bool ref_list_coded = (list == 0) ? ref0_coded : ref1_coded;
				uint8* rt = (list == 0) ? ref0 : ref1;
				for (int i = 0; i < 4; ++i) {
					if (sub[i] == 0) continue;
					int su0 = 0, su1 = 0, sw4 = 0, sh4 = 0;
					H264_BSubPartInfo(sub[i], 0, &su0, &su1, &sw4, &sh4);
					int sx = (i & 1) * 2, sy = (i >> 1) * 2;
					// A sub-macroblock that does not use this list is "list not used" (255) and has
					// to be marked as such NOW, not after the motion vector differences have been
					// read: the predictors of the other partitions of the same macroblock read this
					// table, and an untouched entry holds reference index 0, which makes them count
					// a neighbour that H.264 8.4.1.3.1 does not.  Measured on display frame 2
					// (mb 299, partition (0,2) of list 1): the predictor then saw three matching
					// neighbours instead of two, the median replaced the single-match vector and
					// the predictor fell to 0 instead of (17,-32).
					if (list == 0 ? !su0 : !su1) {
						CABAC_StoreRef(rt, mb_idx, sx, sy, 2, 2, 255);
						continue;
					}
					if (!ref_list_coded) {
						// Only one reference is active in this list, so ref_idx is not coded and
						// every partition that uses the list uses reference 0.
						CABAC_StoreRef(rt, mb_idx, sx, sy, 2, 2, 0);
						continue;
					}
					int r = CABAC_DecodeRefIdx(&cabac,
						CABAC_RefNeighbourDir(rt, nv_direct4, mb_idx, mb_width, sx, sy, hasA),
						CABAC_RefNeighbourTopDir(rt, nv_direct4, mb_idx, mb_width, sx, sy, hasB), (list == 0 ? sh.num_ref_idx_l0_active_minus1 : sh.num_ref_idx_l1_active_minus1) + 1);
					CABAC_StoreRef(rt, mb_idx, sx, sy, 2, 2, r);
				}
			}

			// Motion vector differences, list by list, sub-macroblock by sub-macroblock.
			for (int list = 0; list < 2; ++list) {
				H264MotionVector* mvt = (list == 0) ? mv0 : mv1;
				int16* mvdt = (list == 0) ? mvd0 : mvd1;
				const uint8* rt = (list == 0) ? ref0 : ref1;
				for (int i = 0; i < 4; ++i) {
					if (sub[i] == 0) continue;
					int u0 = 0, u1 = 0, w4 = 0, h4 = 0;
					H264_BSubPartInfo(sub[i], 0, &u0, &u1, &w4, &h4);
					if (list == 0 ? !u0 : !u1) {
						// The motion vector and motion vector difference cache of a list that a
						// (sub-)partition does not use are zeroed.  Leaving the slots alone would
						// let a stale value from another macroblock steer the amvd sum that
						// selects the mvd context.
						int sxz = (i & 1) * 2, syz = (i >> 1) * 2;
						for (int yy = 0; yy < 2; ++yy)
							for (int xx = 0; xx < 2; ++xx) {
								mvt[cur_base + (syz + yy) * 4 + sxz + xx] = H264MotionVector{ 0, 0 };
								int slz = (int)(mb_idx * 16 + (syz + yy) * 4 + sxz + xx) * 2;
								mvdt[slz] = 0; mvdt[slz + 1] = 0;
							}
						continue;
					}
					int sx = (i & 1) * 2, sy = (i >> 1) * 2;
					int nsp = (sub[i] <= 3) ? 1 : ((sub[i] <= 9) ? 2 : 4);
					for (int j = 0; j < nsp && nparts < H264_MAX_B_PARTS; ++j) {
						int pw4 = 0, ph4 = 0, uu0 = 0, uu1 = 0;
						H264_BSubPartInfo(sub[i], j, &uu0, &uu1, &pw4, &ph4);
						int bx4 = sx;
						int by4 = sy;
						if (sub[i] <= 3) { /* single 8x8 partition */ }
						else if (sub[i] <= 9) {
							// 16x8 stacks its two 8x4 halves, 8x16 places its two 4x8
							// halves side by side.
							if (sub[i] & 1) bx4 = sx + j;   // 4x8
							else            by4 = sy + j;   // 8x4
						}
						else { bx4 = sx + (j & 1); by4 = sy + (j >> 1); }   // 4x4

						int pref = rt[cur_base + by4 * 4 + bx4];
						// The reference table has to be handed over: H.264 8.4.1.3.2 gives a
						// neighbouring partition that does not use this list a zero vector and
						// reference index -1, and 8.4.1.3.1 only accepts a neighbour whose
						// reference index matches.  Without it every neighbour counts as a match,
						// which puts a wrong vector into both the single-match rule and the
						// median - the predictor of a partition that has not even been
						// reconstructed yet then leaks into this one.
						H264MotionVector mvp = H264_GetMVPForPartition(mvt, mb_idx, mb_width, mb_x, mb_y,
																	   bx4, by4, pw4, ph4, rt, pref);
						H264MotionVector mvd_abs;
						H264MotionVector mvd = CABAC_DecodeMVDAt(&cabac, mvdt, mb_idx, mb_width, hasA, hasB,
																 bx4, by4, &mvd_abs);
						CABAC_StoreMVD(mvdt, mb_idx, bx4, by4, pw4, ph4, mvd_abs);
						H264MotionVector mvv = { (int16)(mvp.x + mvd.x), (int16)(mvp.y + mvd.y) };
						for (int yy = 0; yy < ph4; ++yy)
							for (int xx = 0; xx < pw4; ++xx)
								mvt[cur_base + (by4 + yy) * 4 + bx4 + xx] = mvv;

						parts[nparts].x4 = bx4; parts[nparts].y4 = by4;
						parts[nparts].w4 = pw4; parts[nparts].h4 = ph4;
						parts[nparts].use0 = (list == 0);
						parts[nparts].use1 = (list == 1);
						parts[nparts].ref0 = (list == 0) ? pref : 0;
						parts[nparts].ref1 = (list == 1) ? pref : 0;
						H264MotionVector zero = { 0, 0 };
						parts[nparts].mv0 = (list == 0) ? mvv : zero;
						parts[nparts].mv1 = (list == 1) ? mvv : zero;
						++nparts;
					}
				}
			}

			// A sub-macroblock that uses both lists was emitted once per list; merge those
			// pairs so that it is predicted once from both references.
			for (int a = 0; a < nparts; ++a) {
				for (int b = a + 1; b < nparts; ++b) {
					if (parts[b].x4 == parts[a].x4 && parts[b].y4 == parts[a].y4 &&
						parts[b].w4 == parts[a].w4 && parts[b].h4 == parts[a].h4) {
						if (parts[b].use0) { parts[a].use0 = true; parts[a].ref0 = parts[b].ref0; parts[a].mv0 = parts[b].mv0; }
						if (parts[b].use1) { parts[a].use1 = true; parts[a].ref1 = parts[b].ref1; parts[a].mv1 = parts[b].mv1; }
						parts[b] = parts[nparts - 1];
						--nparts;
						--b;
					}
				}
			}

			// Partitions of the B_Direct_8x8 sub-macroblocks.
			for (int i = 0; i < 4; ++i) {
				if (sub[i] == 0) H264_EmitDirect8x8(i, sq, mv0, ref0, mv1, ref1, cur_base, parts, &nparts);
			}
		} else {
			// B_L0/B_L1/B_Bi 16x16, 16x8 and 8x16.
			const uint8 lists = h264_b_mb_lists[b_type];
			const int pc = (b_type <= 3) ? 1 : 2;
			const bool wide = (b_type >= 4) && ((b_type & 1) == 0); // 16x8 (two horizontal halves)
			for (int p = 0; p < pc; ++p) {
				parts[p].x4 = (pc == 1 || wide) ? 0 : p * 2;
				parts[p].y4 = (pc == 1 || !wide) ? 0 : p * 2;
				parts[p].w4 = (pc == 1 || wide) ? 4 : 2;
				parts[p].h4 = (pc == 1 || !wide) ? 4 : 2;
				parts[p].use0 = ((lists >> (2 * p)) & 1) != 0;
				parts[p].use1 = ((lists >> (2 * p + 1)) & 1) != 0;
				parts[p].ref0 = 0;
				parts[p].ref1 = 0;
				parts[p].mv0 = H264MotionVector{ 0, 0 };
				parts[p].mv1 = H264MotionVector{ 0, 0 };
			}
			nparts = pc;

			// All reference indices first, then all motion vector differences.
			for (int list = 0; list < 2; ++list) {
				const bool ref_list_coded = (list == 0) ? ref0_coded : ref1_coded;
				uint8* rt = (list == 0) ? ref0 : ref1;
				for (int p = 0; p < pc; ++p) {
					// Same as in the B_8x8 case above: the unused list has to be marked before the
					// motion vectors are predicted.
					if (list == 0 ? !parts[p].use0 : !parts[p].use1) {
						CABAC_StoreRef(rt, mb_idx, parts[p].x4, parts[p].y4, parts[p].w4, parts[p].h4, 255);
						continue;
					}
					if (!ref_list_coded) {
						CABAC_StoreRef(rt, mb_idx, parts[p].x4, parts[p].y4, parts[p].w4, parts[p].h4, 0);
						continue;
					}
					int r = CABAC_DecodeRefIdx(&cabac,
						CABAC_RefNeighbourDir(rt, nv_direct4, mb_idx, mb_width, parts[p].x4, parts[p].y4, hasA),
						CABAC_RefNeighbourTopDir(rt, nv_direct4, mb_idx, mb_width, parts[p].x4, parts[p].y4, hasB), (list == 0 ? sh.num_ref_idx_l0_active_minus1 : sh.num_ref_idx_l1_active_minus1) + 1);
					CABAC_StoreRef(rt, mb_idx, parts[p].x4, parts[p].y4, parts[p].w4, parts[p].h4, r);
					if (list == 0) parts[p].ref0 = r; else parts[p].ref1 = r;
				}
			}
			for (int list = 0; list < 2; ++list) {
				H264MotionVector* mvt = (list == 0) ? mv0 : mv1;
				int16* mvdt = (list == 0) ? mvd0 : mvd1;
				// The neighbouring partitions only predict the motion of this list when they
				// use the same reference index of it (H.264 8.4.1.3.1).
				const uint8* reft = (list == 0) ? ref0 : ref1;
				for (int p = 0; p < pc; ++p) {
					if (list == 0 ? !parts[p].use0 : !parts[p].use1) {
						// See the B_8x8 loop above: both caches of a list the
						// partition does not use are cleared.
						for (int yy = 0; yy < parts[p].h4; ++yy)
							for (int xx = 0; xx < parts[p].w4; ++xx) {
								mvt[cur_base + (parts[p].y4 + yy) * 4 + parts[p].x4 + xx] = H264MotionVector{ 0, 0 };
								int slz = (int)(mb_idx * 16 + (parts[p].y4 + yy) * 4 + parts[p].x4 + xx) * 2;
								mvdt[slz] = 0; mvdt[slz + 1] = 0;
							}
						continue;
					}
					const int cur_ref = (list == 0) ? parts[p].ref0 : parts[p].ref1;
					H264MotionVector mvp = H264_GetMVPForPartition(mvt, mb_idx, mb_width, mb_x, mb_y,
																   parts[p].x4, parts[p].y4,
																   parts[p].w4, parts[p].h4,
																   reft, cur_ref);
					H264MotionVector mvd_abs;
					H264MotionVector mvd = CABAC_DecodeMVDAt(&cabac, mvdt, mb_idx, mb_width, hasA, hasB,
															 parts[p].x4, parts[p].y4, &mvd_abs);
					CABAC_StoreMVD(mvdt, mb_idx, parts[p].x4, parts[p].y4, parts[p].w4, parts[p].h4, mvd_abs);
					H264MotionVector mvv = { (int16)(mvp.x + mvd.x), (int16)(mvp.y + mvd.y) };
					for (int yy = 0; yy < parts[p].h4; ++yy)
						for (int xx = 0; xx < parts[p].w4; ++xx)
							mvt[cur_base + (parts[p].y4 + yy) * 4 + parts[p].x4 + xx] = mvv;
					if (list == 0) parts[p].mv0 = mvv; else parts[p].mv1 = mvv;
				}
			}
		}

		// A partition that does not use a reference list has to be marked as such in that list's
		// reference table: an untouched entry holds reference index 0, which would make a later
		// spatial direct derivation believe that a neighbouring partition uses the list
		// (H.264 8.4.1.2.2 needs "no neighbour uses this list" to stay distinguishable).
		for (int p = 0; p < nparts; ++p) {
			if (!parts[p].use0)
				CABAC_StoreRef(ref0, mb_idx, parts[p].x4, parts[p].y4, parts[p].w4, parts[p].h4, 255);
			if (!parts[p].use1)
				CABAC_StoreRef(ref1, mb_idx, parts[p].x4, parts[p].y4, parts[p].w4, parts[p].h4, 255);
		}

		// Motion compensation of every partition.
		for (int p = 0; p < nparts; ++p) {
			int r0i = parts[p].ref0, r1i = parts[p].ref1;
			if (r0i >= n0_use) r0i = 0;
			if (r1i >= n1_use) r1i = 0;
			const H264RefPicDesc& r0 = list0[r0i];
			const H264RefPicDesc& r1 = list1[r1i];
			int w0 = 32, w1 = 32;
			H264_ImplicitWeights(pps->weighted_bipred_idc, cur_poc, r0.poc, r1.poc, &w0, &w1);
			int lx = parts[p].x4 * 4, ly = parts[p].y4 * 4;
			int cx = parts[p].x4 * 2, cy = parts[p].y4 * 2;
			H264_PredRegion(pred_y + ly * 16 + lx, 16, parts[p].w4 * 4, parts[p].h4 * 4,
							r0.y, r0.y_stride, r1.y, r1.y_stride,
							src_x + lx, src_y + ly, parts[p].mv0.x, parts[p].mv0.y,
							parts[p].mv1.x, parts[p].mv1.y, pic_w, pic_h, false,
							parts[p].use0, parts[p].use1, w0, w1);
			H264_PredRegion(pred_u + cy * 8 + cx, 8, parts[p].w4 * 2, parts[p].h4 * 2,
							r0.u, r0.uv_stride, r1.u, r1.uv_stride,
							src_x / 2 + cx, src_y / 2 + cy, parts[p].mv0.x, parts[p].mv0.y,
							parts[p].mv1.x, parts[p].mv1.y, pic_w_c, pic_h_c, true,
							parts[p].use0, parts[p].use1, w0, w1);
			H264_PredRegion(pred_v + cy * 8 + cx, 8, parts[p].w4 * 2, parts[p].h4 * 2,
							r0.v, r0.uv_stride, r1.v, r1.uv_stride,
							src_x / 2 + cx, src_y / 2 + cy, parts[p].mv0.x, parts[p].mv0.y,
							parts[p].mv1.x, parts[p].mv1.y, pic_w_c, pic_h_c, true,
							parts[p].use0, parts[p].use1, w0, w1);
		}

		// Residual: coded block pattern, mb_qp_delta, luma and chroma coefficients.
		{
			int cbp_luma = 0;
			int cA0 = (hasA && ((cbp_table[mb_idx - 1] & 2) == 0)) ? 1 : 0;
			int cB0 = (hasB && ((cbp_table[mb_idx - mb_width] & 4) == 0)) ? 1 : 0;
			if (CABAC_DecodeBin(&cabac, 73 + cA0 + 2 * cB0)) cbp_luma |= 1;
			int cA1 = ((cbp_luma & 1) == 0) ? 1 : 0;
			int cB1 = (hasB && ((cbp_table[mb_idx - mb_width] & 8) == 0)) ? 1 : 0;
			if (CABAC_DecodeBin(&cabac, 73 + cA1 + 2 * cB1)) cbp_luma |= 2;
			int cA2 = (hasA && ((cbp_table[mb_idx - 1] & 8) == 0)) ? 1 : 0;
			int cB2 = ((cbp_luma & 1) == 0) ? 1 : 0;
			if (CABAC_DecodeBin(&cabac, 73 + cA2 + 2 * cB2)) cbp_luma |= 4;
			int cA3 = ((cbp_luma & 4) == 0) ? 1 : 0;
			int cB3 = ((cbp_luma & 2) == 0) ? 1 : 0;
			if (CABAC_DecodeBin(&cabac, 73 + cA3 + 2 * cB3)) cbp_luma |= 8;

			int cbp_chroma = 0;
			int ca = hasA ? cbp_chroma_table[mb_idx - 1] : 0;
			int cbc = hasB ? cbp_chroma_table[mb_idx - mb_width] : 0;
			int c0 = (ca > 0 ? 1 : 0) + (cbc > 0 ? 2 : 0);
			if (CABAC_DecodeBin(&cabac, 77 + c0) == 0) {
				cbp_chroma = 0;
			} else {
				int c1 = 4 + (ca == 2 ? 1 : 0) + (cbc == 2 ? 2 : 0);
				cbp_chroma = 1 + CABAC_DecodeBin(&cabac, 77 + c1);
			}
			cbp_table[mb_idx] = (uint8)cbp_luma;
			cbp_chroma_table[mb_idx] = (uint8)cbp_chroma;

			bool is_8x8_dct = false;
			if (dct8x8_allowed && cbp_luma > 0 && pps->transform_8x8_mode_flag) {
				int ctx8 = 399 + (hasA && transform_8x8_table[mb_idx - 1] ? 1 : 0)
								+ (hasB && transform_8x8_table[mb_idx - mb_width] ? 1 : 0);
				is_8x8_dct = (CABAC_DecodeBin(&cabac, ctx8) != 0);
			}
			transform_8x8_table[mb_idx] = is_8x8_dct ? 1 : 0;

			const int qp_cbp = cbp_luma | (cbp_chroma << 4);
			if (qp_cbp > 0) {
				int ctx_qp = 60 + (prev_mb_has_qp_delta ? 1 : 0);
				if (CABAC_DecodeBin(&cabac, ctx_qp)) {
					int val = 1;
					while (val < 64 && CABAC_DecodeBin(&cabac, (val == 1 ? 62 : 63))) val++;
					int delta = (val & 1) ? ((val + 1) >> 1) : -((val + 1) >> 1);
					current_qp = (current_qp + delta + 52) % 52;
					prev_mb_has_qp_delta = true;
				} else {
					prev_mb_has_qp_delta = false;
				}
			} else {
				prev_mb_has_qp_delta = false;
			}

			if (is_8x8_dct) {
				for (int k8 = 0; k8 < 4; ++k8) {
					int bx8 = (k8 & 1) * 8;
					int by8 = ((k8 >> 1) & 1) * 8;
					if (cbp_luma & (1 << k8)) {
						// In 4:2:0 there is no coded_block_flag for an 8x8 luma block.
						int16 coeffs_8x8[64] = { 0 };
						int32 scaled_8x8[64] = { 0 };
						int32 res_8x8[64] = { 0 };
						CABAC_DecodeResidual(&cabac, 5, 64, coeffs_8x8);
						for (int sub = 0; sub < 4; ++sub) nnz_table[mb_idx * 24 + k8 * 4 + sub] = 1;
						H264_Dequant8x8(coeffs_8x8, current_qp, scaled_8x8);
						H264_IDCT8x8(scaled_8x8, res_8x8);
						uint8* blk_dst = mb_y_dst + by8 * dst_y_stride + bx8;
						for (int y = 0; y < 8; ++y)
							for (int x = 0; x < 8; ++x)
								blk_dst[y * dst_y_stride + x] =
									Clip8((int)pred_y[(by8 + y) * 16 + (bx8 + x)] + res_8x8[y * 8 + x]);
					} else {
						uint8* blk_dst = mb_y_dst + by8 * dst_y_stride + bx8;
						for (int y = 0; y < 8; ++y)
							for (int x = 0; x < 8; ++x)
								blk_dst[y * dst_y_stride + x] = pred_y[(by8 + y) * 16 + (bx8 + x)];
					}
				}
			} else {
				for (int k = 0; k < 16; ++k) {
					int bx = ((k & 1) ? 1 : 0) + ((k & 4) ? 2 : 0);
					int by = ((k & 2) ? 1 : 0) + ((k & 8) ? 2 : 0);
					int blk_8x8 = (by / 2) * 2 + (bx / 2);
					uint8* blk_dst = mb_y_dst + by * 4 * dst_y_stride + bx * 4;
					int32 residual[16] = { 0 };
					if (cbp_luma & (1 << blk_8x8)) {
						// Inter macroblock: an unavailable neighbour counts as empty (b_intra = 0).
						int condA_f = GetLuma4x4CondA(nnz_table, mb_idx, mb_width, k, hasA, 0);
						int condB_f = GetLuma4x4CondB(nnz_table, mb_idx, mb_width, k, hasB, 0);
						if (CABAC_DecodeBin(&cabac, 93 + condA_f + 2 * condB_f)) {
							int16 coeffs[16] = { 0 };
							int32 scaled[16] = { 0 };
							CABAC_DecodeResidual(&cabac, 2, 16, coeffs);
							nnz_table[mb_idx * 24 + k] = 1;
							H264_Dequant4x4(coeffs, current_qp, false, scaled);
							H264_IDCT4x4(scaled, residual);
						}
					}
					// The prediction has to reach the picture even when the block carries no
					// coefficients: leaving the destination untouched would keep the previous
					// picture's samples, which is exactly what a coded-but-empty macroblock
					// (or a skipped one) must not do.
					for (int y = 0; y < 4; ++y)
						for (int x = 0; x < 4; ++x)
							blk_dst[y * dst_y_stride + x] =
								Clip8((int)pred_y[(by * 4 + y) * 16 + (bx * 4 + x)] + residual[y * 4 + x]);
				}
			}

			// Chroma: both DC blocks first, then the AC blocks of both planes.
			uint8 qp_c = H264_GetChromaQP(current_qp, pps->chroma_qp_index_offset);
			int32 scaled_dc_plane[2][4];
			int32 residuals_c_plane[2][4][16];
			MemSet(scaled_dc_plane, 0, sizeof(scaled_dc_plane));
			MemSet(residuals_c_plane, 0, sizeof(residuals_c_plane));

			if (cbp_chroma > 0) {
				for (int plane = 0; plane < 2; ++plane) {
					int16 dc_coeffs[4] = { 0 };
					int condA_dc = hasA ? (nz_dc_table[(mb_idx - 1) * 3 + 1 + plane] ? 1 : 0) : 0;
					int condB_dc = hasB ? (nz_dc_table[(mb_idx - mb_width) * 3 + 1 + plane] ? 1 : 0) : 0;
					if (CABAC_DecodeBin(&cabac, 97 + condA_dc + 2 * condB_dc)) {
						CABAC_DecodeResidual(&cabac, 3, 4, dc_coeffs);
						nz_dc_table[mb_idx * 3 + 1 + plane] = 1;
					}
					stdsint dc_in[4] = { dc_coeffs[0], dc_coeffs[1], dc_coeffs[2], dc_coeffs[3] };
					stdsint dc_out[4] = { 0 };
					Hadamard_H264_2x2(dc_in, qp_c, dc_out);
					for (int i = 0; i < 4; ++i) scaled_dc_plane[plane][i] = (int32)dc_out[i];
				}
			}
			if (cbp_chroma == 2) {
				for (int plane = 0; plane < 2; ++plane) {
					int nnz_offset = 16 + plane * 4;
					for (int ck = 0; ck < 4; ++ck) {
						int16 ac_coeffs[16] = { 0 };
						// Inter macroblock: an unavailable neighbour counts as empty (b_intra = 0).
						int condA_ac = GetChromaACCondA(nnz_table, mb_idx, mb_width, plane, ck, hasA, 0);
						int condB_ac = GetChromaACCondB(nnz_table, mb_idx, mb_width, plane, ck, hasB, 0);
						if (CABAC_DecodeBin(&cabac, 101 + condA_ac + 2 * condB_ac)) {
							CABAC_DecodeResidual(&cabac, 4, 15, ac_coeffs);
							nnz_table[mb_idx * 24 + nnz_offset + ck] = 1;
						}
						stdsint scaled_block[16] = { 0 };
						Dequant_H264_4x4(ac_coeffs, qp_c, true, scaled_block);
						scaled_block[0] = scaled_dc_plane[plane][ck];
						stdsint res_s[16];
						IDCT_H264_4x4(scaled_block, res_s);
						for (int i = 0; i < 16; ++i) residuals_c_plane[plane][ck][i] = (int32)res_s[i];
					}
				}
			} else if (cbp_chroma == 1) {
				for (int plane = 0; plane < 2; ++plane) {
					for (int ck = 0; ck < 4; ++ck) {
						stdsint scaled_block[16] = { 0 };
						scaled_block[0] = scaled_dc_plane[plane][ck];
						stdsint res_s[16];
						IDCT_H264_4x4(scaled_block, res_s);
						for (int i = 0; i < 16; ++i) residuals_c_plane[plane][ck][i] = (int32)res_s[i];
					}
				}
			}

			for (int plane = 0; plane < 2; ++plane) {
				uint8* ch_dst = (plane == 0) ? mb_u_dst : mb_v_dst;
				uint8* ch_pred = (plane == 0) ? pred_u : pred_v;
				for (int ck = 0; ck < 4; ++ck) {
					int cbx = (ck & 1) * 4;
					int cby = ((ck >> 1) & 1) * 4;
					for (int y = 0; y < 4; ++y)
						for (int x = 0; x < 4; ++x)
							ch_dst[(cby + y) * dst_uv_stride + (cbx + x)] =
								Clip8((int)ch_pred[(cby + y) * 8 + (cbx + x)] + residuals_c_plane[plane][ck][y * 4 + x]);
				}
			}
		}

		if (mb_qp_out) mb_qp_out[mb_idx] = (uint8)current_qp;
		if (CABAC_DecodeTerminate(&cabac)) { terminated = 1; break; }
	}

	// Deblocking export (H.264 8.7.2.1).  A B slice predicts from both lists, so the reference
	// identity of every 4x4 block is exported as the picture order count of the picture the list
	// selects: JM compares the reference *pictures*, and list0[i] and list1[j] often name the same
	// picture.  255 means "this list is not used by the block", which JM keeps as a NULL reference.
	if (db_flags_out) {
		for (uint32 m = 0; m < num_mbs; ++m) {
			db_flags_out[(size_t)m * 4 + 0] = db_intra_tab ? db_intra_tab[m] : 0;
			db_flags_out[(size_t)m * 4 + 1] = transform_8x8_table[m];
			db_flags_out[(size_t)m * 4 + 2] = cbp_table[m];
			db_flags_out[(size_t)m * 4 + 3] = 0;
		}
	}
	if (db_mv_out) {
		for (uint32 m = 0; m < num_mbs; ++m) {
			for (int b = 0; b < 16; ++b) {
				db_mv_out[((size_t)m * 32 + b) * 2 + 0] = (int16)mv0[m * 16 + b].x;
				db_mv_out[((size_t)m * 32 + b) * 2 + 1] = (int16)mv0[m * 16 + b].y;
				db_mv_out[((size_t)m * 32 + 16 + b) * 2 + 0] = (int16)mv1[m * 16 + b].x;
				db_mv_out[((size_t)m * 32 + 16 + b) * 2 + 1] = (int16)mv1[m * 16 + b].y;
			}
		}
	}
	if (db_ref_out) {
		for (uint32 m = 0; m < num_mbs; ++m) {
			for (int b = 0; b < 16; ++b) {
				int ri = (int)ref0[m * 16 + b];
				int id = -1;
				if (ri != 0xFF && ri < n0_use) id = list0[ri].poc;
				db_ref_out[(size_t)m * 32 + b] = (int16)id;
				if (id < 0) {
					db_mv_out[((size_t)m * 32 + b) * 2 + 0] = 0;
					db_mv_out[((size_t)m * 32 + b) * 2 + 1] = 0;
				}
				ri = (int)ref1[m * 16 + b];
				id = -1;
				if (ri != 0xFF && ri < n1_use) id = list1[ri].poc;
				db_ref_out[(size_t)m * 32 + 16 + b] = (int16)id;
				if (id < 0 && db_mv_out) {
					db_mv_out[((size_t)m * 32 + 16 + b) * 2 + 0] = 0;
					db_mv_out[((size_t)m * 32 + 16 + b) * 2 + 1] = 0;
				}
			}
		}
	}
	if (db_intra_tab) free(db_intra_tab);

	if (mv_out) MemCopyN(mv_out, mv0, n16 * sizeof(H264MotionVector));
	if (refidx_out) MemCopyN(refidx_out, ref0, n16);
	if (mv1_out) MemCopyN(mv1_out, mv1, n16 * sizeof(H264MotionVector));
	if (refidx1_out) MemCopyN(refidx1_out, ref1, n16);

	free(cbp_table);
	free(cbp_chroma_table);
	free(mode_table);
	free(intra_chroma_table);
	free(nnz_table);
	free(transform_8x8_table);
	free(mb_skip_table);
	free(direct_table);
	free(nv_direct4);
	free(nz_dc_table);
	free(mv0);
	free(mv1);
	free(mvd0);
	free(mvd1);
	free(ref0);
	free(ref1);
	// A slice that ran out of CABAC data mid-macroblock is not usable: half of the picture
	// would still hold the previous frame's content. Report it as a failure so the caller
	// repeats the reference picture instead.
	bool ok = !failed && !cabac.overrun && cabac.bit_pos <= rbsp_len * 8 + 512;
	H264Probe_End(ok ? 1 : 0, &cabac, terminated, fail_reason);
	return ok;
}

static inline void DeblockEdge4(
	uint8* ptr,
	int stride_along,
	int stride_across,
	int bS,
	int indexA,
	int indexB,
	bool is_chroma
) {
	if (bS == 0) return;

	uint8 alpha = deblock_alpha_table[indexA];
	uint8 beta = deblock_beta_table[indexB];
	if (alpha == 0 && beta == 0) return;

	int tc0 = (bS < 4) ? (int)deblock_tc0_table[indexA][bS - 1] : 0;

	for (int k = 0; k < 4; ++k) {
		uint8* line = ptr + k * stride_along;
		int p0 = line[-1 * stride_across];
		int p1 = line[-2 * stride_across];
		int p2 = line[-3 * stride_across];
		int p3 = line[-4 * stride_across];
		int q0 = line[0 * stride_across];
		int q1 = line[1 * stride_across];
		int q2 = line[2 * stride_across];
		int q3 = line[3 * stride_across];

		int d_pq = (p0 > q0) ? (p0 - q0) : (q0 - p0);
		int d_p10 = (p1 > p0) ? (p1 - p0) : (p0 - p1);
		int d_q10 = (q1 > q0) ? (q1 - q0) : (q0 - q1);

		if (d_pq < (int)alpha && d_p10 < (int)beta && d_q10 < (int)beta) {
			if (bS == 4) {
				if (!is_chroma) {
					bool a_p = ((p2 > p0 ? p2 - p0 : p0 - p2) < (int)beta);
					bool a_q = ((q2 > q0 ? q2 - q0 : q0 - q2) < (int)beta);
					int alpha_thr = ((int)alpha >> 2) + 2;

					if (a_p && d_pq < alpha_thr) {
						int p0_new = (p2 + 2 * p1 + 2 * p0 + 2 * q0 + q1 + 4) >> 3;
						int p1_new = (p2 + p1 + p0 + q0 + 2) >> 2;
						int p2_new = (2 * p3 + 3 * p2 + p1 + p0 + q0 + 4) >> 3;
						line[-3 * stride_across] = Clip8(p2_new);
						line[-2 * stride_across] = Clip8(p1_new);
						line[-1 * stride_across] = Clip8(p0_new);
					} else {
						int p0_new = (2 * p1 + p0 + q1 + 2) >> 2;
						line[-1 * stride_across] = Clip8(p0_new);
					}

					if (a_q && d_pq < alpha_thr) {
						int q0_new = (q2 + 2 * q1 + 2 * q0 + 2 * p0 + p1 + 4) >> 3;
						int q1_new = (q2 + q1 + q0 + p0 + 2) >> 2;
						int q2_new = (2 * q3 + 3 * q2 + q1 + q0 + p0 + 4) >> 3;
						line[0 * stride_across] = Clip8(q0_new);
						line[1 * stride_across] = Clip8(q1_new);
						line[2 * stride_across] = Clip8(q2_new);
					} else {
						int q0_new = (2 * q1 + q0 + p1 + 2) >> 2;
						line[0 * stride_across] = Clip8(q0_new);
					}
				} else {
					int p0_new = (2 * p1 + p0 + q1 + 2) >> 2;
					int q0_new = (2 * q1 + q0 + p1 + 2) >> 2;
					line[-1 * stride_across] = Clip8(p0_new);
					line[0 * stride_across] = Clip8(q0_new);
				}
			} else {
				bool a_p = ((p2 > p0 ? p2 - p0 : p0 - p2) < (int)beta);
				bool a_q = ((q2 > q0 ? q2 - q0 : q0 - q2) < (int)beta);
				// H.264 8.7.2.3: equation (8-337) gives a luma edge tC = tC0 + ap + aq, but
				// equation (8-338) gives a chroma edge the flat value tC = tC0 + 1.
				int tc = is_chroma ? (tc0 + 1) : (tc0 + (a_p ? 1 : 0) + (a_q ? 1 : 0));
				int delta = Clip3(-tc, tc, (((q0 - p0) * 4) + (p1 - q1) + 4) >> 3);
				line[-1 * stride_across] = Clip8(p0 + delta);
				line[0 * stride_across] = Clip8(q0 - delta);

				if (!is_chroma && a_p) {
					int delta_p = Clip3(-tc0, tc0, (p2 + ((p0 + q0 + 1) >> 1) - (p1 * 2)) >> 1);
					line[-2 * stride_across] = Clip8(p1 + delta_p);
				}
				if (!is_chroma && a_q) {
					int delta_q = Clip3(-tc0, tc0, (q2 + ((p0 + q0 + 1) >> 1) - (q1 * 2)) >> 1);
					line[1 * stride_across] = Clip8(q1 + delta_q);
				}
			}
		}
	}
}

// Two motion vectors differ for the purpose of the boundary strength when either component differs
// by at least one whole luma sample, i.e. four quarter samples (H.264 8.7.2.1, JM's compare_mvs).
static inline int H264_CmpMV(const int16* a, const int16* b) {
	int dx = (int)a[0] - (int)b[0];
	int dy = (int)a[1] - (int)b[1];
	if (dx < 0) dx = -dx;
	if (dy < 0) dy = -dy;
	return (dx >= 4 || dy >= 4) ? 1 : 0;
}

// Boundary strength of one 4x4 edge (H.264 8.7.2.1).  blkP / blkQ are 4x4 luma block indices in the
// raster order by4 * 4 + bx4 that the slice decoders use for their own tables.
//   db_flags: 4 bytes per macroblock -- [0] intra, [1] transform_size_8x8_flag, [2] luma cbp bits.
//   db_mv:    two lists x 16 4x4 blocks x (x, y) per macroblock, quarter samples.
//   db_ref:   two lists x 16 4x4 blocks per macroblock, -1 meaning "this list is not used".
// The derivation and the comparisons below follow JM's get_strength_ver / get_strength_hor, whose
// special cases for P_Skip / P16x16 / P16x8 / B_Direct_16x16 are all equivalent to the generic
// rule once the motion vectors and the coded block pattern are available per 4x4 block.
static inline int H264_DeblockStrength(
	const uint8* db_flags, const int16* db_mv, const int16* db_ref,
	uint32 mbP, int blkP, uint32 mbQ, int blkQ, bool mb_edge
) {
	const uint8* fp = db_flags + (size_t)mbP * 4;
	const uint8* fq = db_flags + (size_t)mbQ * 4;
	if (fp[0] || fq[0]) return mb_edge ? 4 : 3;
	// A 4x4 luma block carries the cbp bit of the 8x8 block that contains it: bits 0..3 of the luma
	// coded_block_pattern are the 8x8 blocks in raster order (H.264 7.4.5, Figure 6-6).
	const int cbpP = (fp[2] >> (((blkP >> 2) >> 1) * 2 + ((blkP & 3) >> 1))) & 1;
	const int cbpQ = (fq[2] >> (((blkQ >> 2) >> 1) * 2 + ((blkQ & 3) >> 1))) & 1;
	if (cbpP || cbpQ) return 2;
	const int16* mp = db_mv + (size_t)mbP * 64;
	const int16* mq = db_mv + (size_t)mbQ * 64;
	const int16* rp = db_ref + (size_t)mbP * 32;
	const int16* rq = db_ref + (size_t)mbQ * 32;
	const int p0 = rp[blkP], p1 = rp[16 + blkP];
	const int q0 = rq[blkQ], q1 = rq[16 + blkQ];
	if ((p0 == q0 && p1 == q1) || (p0 == q1 && p1 == q0)) {
		const int16* pm0 = mp + blkP * 2;
		const int16* pm1 = mp + (16 + blkP) * 2;
		const int16* qm0 = mq + blkQ * 2;
		const int16* qm1 = mq + (16 + blkQ) * 2;
		int s;
		if (p0 != p1) {
			if (p0 == q0) s = H264_CmpMV(pm0, qm0) | H264_CmpMV(pm1, qm1);
			else s = H264_CmpMV(pm0, qm1) | H264_CmpMV(pm1, qm0);
		} else {
			s = (H264_CmpMV(pm0, qm0) | H264_CmpMV(pm1, qm1)) &&
				(H264_CmpMV(pm0, qm1) | H264_CmpMV(pm1, qm0));
		}
		return s ? 1 : 0;
	}
	return 1;
}

// One chroma edge of four samples with a per-sample boundary strength.  A chroma 4x4 block is as
// wide as two luma 4x4 blocks, so the two halves of a chroma edge can carry different strengths.
static inline void DeblockChromaEdge4(
	uint8* ptr, int stride_along, int stride_across,
	const int* bS, int indexA, int indexB
) {
	uint8 alpha = deblock_alpha_table[indexA];
	uint8 beta = deblock_beta_table[indexB];
	if (alpha == 0 && beta == 0) return;

	for (int k = 0; k < 4; ++k) {
		int s = bS[k];
		if (s == 0) continue;
		uint8* line = ptr + k * stride_along;
		int p0 = line[-1 * stride_across];
		int p1 = line[-2 * stride_across];
		int q0 = line[0 * stride_across];
		int q1 = line[1 * stride_across];

		int d_pq = (p0 > q0) ? (p0 - q0) : (q0 - p0);
		int d_p10 = (p1 > p0) ? (p1 - p0) : (p0 - p1);
		int d_q10 = (q1 > q0) ? (q1 - q0) : (q0 - q1);
		if (d_pq >= (int)alpha || d_p10 >= (int)beta || d_q10 >= (int)beta) continue;

		if (s == 4) {
			// 8.7.2.4 (8-351) and (8-358): a chroma edge never uses the long form.
			line[-1 * stride_across] = Clip8((2 * p1 + p0 + q1 + 2) >> 2);
			line[0 * stride_across] = Clip8((2 * q1 + q0 + p1 + 2) >> 2);
		} else {
			// 8.7.2.3 (8-334) with (8-338): a chroma edge takes the flat threshold tC = tC0 + 1.
			int tc = (int)deblock_tc0_table[indexA][s - 1] + 1;
			int delta = (((q0 - p0) * 4) + (p1 - q1) + 4) >> 3;
			if (delta > tc) delta = tc;
			else if (delta < -tc) delta = -tc;
			line[-1 * stride_across] = Clip8(p0 + delta);
			line[0 * stride_across] = Clip8(q0 - delta);
		}
	}
}

void H264_DeblockFrame(
	uint8* y_plane,
	uint8* u_plane,
	uint8* v_plane,
	int y_stride,
	int uv_stride,
	int mb_width,
	int mb_height,
	int slice_qp,
	int disable_deblocking_filter_idc,
	int slice_alpha_c_offset,
	int slice_beta_offset,
	const uint8* mb_qp,
	// Per-macroblock data of the picture being filtered, needed to derive the boundary strengths of
	// 8.7.2.1.  All three are NULL for a picture whose slice decoder does not export them; the
	// filter then keeps the old behaviour of treating every edge as intra-coded.
	const uint8* db_flags,
	const int16* db_mv,
	const int16* db_ref,
	// Chroma edges derive their thresholds from the CHROMA qp (H.264 8.7.2.2), not the luma qp.
	int chroma_qp_offset
) {
	if (!y_plane || !u_plane || !v_plane || mb_width <= 0 || mb_height <= 0) return;
	if (disable_deblocking_filter_idc == 1) return;

	// The thresholds of every edge come from qPav = ( qPp + qPq + 1 ) >> 1 of the two
	// macroblocks it separates (H.264 8.7.2.2); a single slice-wide QP is only correct when no
	// macroblock uses mb_qp_delta.
	int indexA = Clip3(0, 51, slice_qp + slice_alpha_c_offset);
	int indexB = Clip3(0, 51, slice_qp + slice_beta_offset);

	for (int mb_y = 0; mb_y < mb_height; ++mb_y) {
		for (int mb_x = 0; mb_x < mb_width; ++mb_x) {
			const uint32 mb_idx = (uint32)(mb_y * mb_width + mb_x);
			const bool db_here = (db_flags != nullptr);
			const bool db_t8x8 = db_here && db_flags[(size_t)mb_idx * 4 + 1] != 0;
			int qp_this = slice_qp;
			int indexA_i = indexA, indexB_i = indexB;
			int indexA_a = indexA, indexB_a = indexB;
			int indexA_b = indexA, indexB_b = indexB;
			// Chroma edges derive their thresholds from the CHROMA qp (H.264 8.7.2.2), not from
			// the luma qp.  The offset of this stream is passed in; when it is not available the
			// chroma indices stay equal to the luma ones, which is what the old code used.
			int indexA_ci = indexA, indexB_ci = indexB;
			int indexA_ca = indexA, indexB_ca = indexB;
			int indexA_cb = indexA, indexB_cb = indexB;
			if (chroma_qp_offset >= 0) {
				// qp_this is only updated further below, so read this macroblock's QP here.
				int ci = mb_y * mb_width + mb_x;
				int qp_luma_cur = (mb_qp && mb_qp[ci] != 0xFF) ? (int)mb_qp[ci] : slice_qp;
				int qc_t = H264_GetChromaQP(qp_luma_cur, chroma_qp_offset);
				int qc_a = qc_t, qc_b = qc_t;
				if (mb_qp) {
					if (mb_x > 0 && mb_qp[ci - 1] != 0xFF) qc_a = H264_GetChromaQP((int)mb_qp[ci - 1], chroma_qp_offset);
					if (mb_y > 0 && mb_qp[ci - mb_width] != 0xFF) qc_b = H264_GetChromaQP((int)mb_qp[ci - mb_width], chroma_qp_offset);
				}
				indexA_ci = Clip3(0, 51, qc_t + slice_alpha_c_offset);
				indexB_ci = Clip3(0, 51, qc_t + slice_beta_offset);
				indexA_ca = Clip3(0, 51, ((qc_a + qc_t + 1) >> 1) + slice_alpha_c_offset);
				indexB_ca = Clip3(0, 51, ((qc_a + qc_t + 1) >> 1) + slice_beta_offset);
				indexA_cb = Clip3(0, 51, ((qc_b + qc_t + 1) >> 1) + slice_alpha_c_offset);
				indexB_cb = Clip3(0, 51, ((qc_b + qc_t + 1) >> 1) + slice_beta_offset);
			}
			if (mb_qp) {
				int idx = mb_y * mb_width + mb_x;
				qp_this = (mb_qp[idx] != 0xFF) ? (int)mb_qp[idx] : slice_qp;
				int qp_a = qp_this, qp_b = qp_this;
				if (mb_x > 0 && mb_qp[idx - 1] != 0xFF) qp_a = (int)mb_qp[idx - 1];
				if (mb_y > 0 && mb_qp[idx - mb_width] != 0xFF) qp_b = (int)mb_qp[idx - mb_width];
				indexA_i = Clip3(0, 51, qp_this + slice_alpha_c_offset);
				indexB_i = Clip3(0, 51, qp_this + slice_beta_offset);
				indexA_a = Clip3(0, 51, ((qp_a + qp_this + 1) >> 1) + slice_alpha_c_offset);
				indexB_a = Clip3(0, 51, ((qp_a + qp_this + 1) >> 1) + slice_beta_offset);
				indexA_b = Clip3(0, 51, ((qp_b + qp_this + 1) >> 1) + slice_alpha_c_offset);
				indexB_b = Clip3(0, 51, ((qp_b + qp_this + 1) >> 1) + slice_beta_offset);
			}

			uint8* mb_y_ptr = y_plane + mb_y * 16 * y_stride + mb_x * 16;
			uint8* mb_u_ptr = u_plane + mb_y * 8 * uv_stride + mb_x * 8;
			uint8* mb_v_ptr = v_plane + mb_y * 8 * uv_stride + mb_x * 8;

			// 1. Luma Vertical Edges (across 4 columns inside MB)
			for (int edge = 0; edge < 4; ++edge) {
				if (edge == 0 && mb_x == 0) continue;
				// H.264 8.7.1: with transform_size_8x8_flag set there is no 4x4 transform
				// boundary inside an 8x8 block, so the edges at 4 and 12 samples are skipped -
				// but the edges at 0 and 8 are still block boundaries and have to be filtered.
				if (db_t8x8 && (edge == 1 || edge == 3)) continue;
				int iA = (edge == 0) ? indexA_a : indexA_i;
				int iB = (edge == 0) ? indexB_a : indexB_i;
				uint8* edge_ptr = mb_y_ptr + edge * 4;
				for (int k = 0; k < 4; ++k) {
					// p is the block before the edge (the left macroblock's rightmost column at a
					// macroblock edge), q the block after it; blk = by4 * 4 + bx4.
					int bS = (edge == 0) ? 4 : 3;
					if (db_here) {
						const uint32 mbP = (edge == 0) ? (mb_idx - 1) : mb_idx;
						const int blkP = (edge == 0) ? (k * 4 + 3) : (k * 4 + edge - 1);
						bS = H264_DeblockStrength(db_flags, db_mv, db_ref, mbP, blkP,
												  mb_idx, k * 4 + edge, edge == 0);
					}
					DeblockEdge4(edge_ptr + k * 4 * y_stride, y_stride, 1, bS, iA, iB, false);
				}
			}

			// 2. Luma Horizontal Edges (across 4 rows inside MB)
			for (int edge = 0; edge < 4; ++edge) {
				if (edge == 0 && mb_y == 0) continue;
				// H.264 8.7.1: with transform_size_8x8_flag set there is no 4x4 transform
				// boundary inside an 8x8 block, so the edges at 4 and 12 samples are skipped -
				// but the edges at 0 and 8 are still block boundaries and have to be filtered.
				if (db_t8x8 && (edge == 1 || edge == 3)) continue;
				int iA = (edge == 0) ? indexA_b : indexA_i;
				int iB = (edge == 0) ? indexB_b : indexB_i;
				uint8* edge_ptr = mb_y_ptr + edge * 4 * y_stride;
				for (int k = 0; k < 4; ++k) {
					int bS = (edge == 0) ? 4 : 3;
					if (db_here) {
						const uint32 mbP = (edge == 0) ? (mb_idx - mb_width) : mb_idx;
						const int blkP = (edge == 0) ? (3 * 4 + k) : ((edge - 1) * 4 + k);
						bS = H264_DeblockStrength(db_flags, db_mv, db_ref, mbP, blkP,
												  mb_idx, edge * 4 + k, edge == 0);
					}
					DeblockEdge4(edge_ptr + k * 4, 1, y_stride, bS, iA, iB, false);
				}
			}

			// 3. Chroma U & V Vertical Edges.  A chroma edge is filtered with the boundary strength
			// of the luma edge it is co-located with (chroma edge e sits at luma edge 2 * e), which
			// is also why each half of a chroma 4x4 edge can have its own strength: the four chroma
			// samples of a 4x4 block row come from two different luma 4x4 block rows.
			for (int edge = 0; edge < 2; ++edge) {
				if (edge == 0 && mb_x == 0) continue;
				int iA = (edge == 0) ? indexA_a : indexA_i;
				int iB = (edge == 0) ? indexB_a : indexB_i;
				for (int k = 0; k < 2; ++k) {
					int bS[4];
					if (db_here) {
						const uint32 mbP = (edge == 0) ? (mb_idx - 1) : mb_idx;
						for (int r = 0; r < 2; ++r) {
							const int by4 = 2 * k + r;
							const int blkP = (edge == 0) ? (by4 * 4 + 3) : (by4 * 4 + 2 * edge - 1);
							bS[2 * r] = bS[2 * r + 1] = H264_DeblockStrength(db_flags, db_mv, db_ref,
									mbP, blkP, mb_idx, by4 * 4 + 2 * edge, edge == 0);
						}
					} else {
						const int v = (edge == 0) ? 4 : 3;
						bS[0] = bS[1] = bS[2] = bS[3] = v;
					}
					DeblockChromaEdge4(mb_u_ptr + edge * 4 + k * 4 * uv_stride, uv_stride, 1, bS, iA, iB);
					DeblockChromaEdge4(mb_v_ptr + edge * 4 + k * 4 * uv_stride, uv_stride, 1, bS, iA, iB);
				}
			}

			// 4. Chroma U & V Horizontal Edges
			for (int edge = 0; edge < 2; ++edge) {
				if (edge == 0 && mb_y == 0) continue;
				int iA = (edge == 0) ? indexA_b : indexA_i;
				int iB = (edge == 0) ? indexB_b : indexB_i;
				for (int k = 0; k < 2; ++k) {
					int bS[4];
					if (db_here) {
						const uint32 mbP = (edge == 0) ? (mb_idx - mb_width) : mb_idx;
						for (int r = 0; r < 2; ++r) {
							const int bx4 = 2 * k + r;
							const int blkP = (edge == 0) ? (3 * 4 + bx4) : ((2 * edge - 1) * 4 + bx4);
							bS[2 * r] = bS[2 * r + 1] = H264_DeblockStrength(db_flags, db_mv, db_ref,
									mbP, blkP, mb_idx, 2 * edge * 4 + bx4, edge == 0);
						}
					} else {
						const int v = (edge == 0) ? 4 : 3;
						bS[0] = bS[1] = bS[2] = bS[3] = v;
					}
					DeblockChromaEdge4(mb_u_ptr + edge * 4 * uv_stride + k * 4, 1, uv_stride, bS, iA, iB);
					DeblockChromaEdge4(mb_v_ptr + edge * 4 * uv_stride + k * 4, 1, uv_stride, bS, iA, iB);
				}
			}
		}
	}
}

// The matrix and the sample range come from the active
// SPS's video_signal_type() (H.264 E.1.1): hardcoding BT.601 shifted every pixel of a BT.709
// stream by up to 9 levels - measured on wind.mp4, whose VUI carries matrix_coefficients 1,
// BT.601 gives an RGB MAE of 5.32 against the reference decoder and BT.709 gives 1.00.  An
// absent or unspecified matrix_coefficients keeps BT.601, which is what a stream without a
// colour description has always been interpreted with.  The coefficients are scaled by 256.
static inline uni::YCbCrMatrix H264YCbCrMatrix(int matrix) {
	if (matrix == 1) return uni::YCbCrMatrix::BT709;
	if (matrix == 9) return uni::YCbCrMatrix::BT2020_NCL;
	return uni::YCbCrMatrix::BT601;
}

class H264DecoderImpl {
public:
	int default_w;
	int default_h;

	H264SPS sps_list[32];
	H264PPS pps_list[256];
	int active_sps_id;
	int active_pps_id;

	uint8* cur_y;
	uint8* cur_u;
	uint8* cur_v;
	uint8* ref_y;
	uint8* ref_u;
	uint8* ref_v;
	int buf_w;
	int buf_h;
	int y_stride;
	int uv_stride;
	size_t pic_y_size;
	size_t pic_uv_size;
	// Per-macroblock quantiser of the picture being decoded. The deblocking filter derives its
	// thresholds from qPav = (qPp + qPq + 1) >> 1, so it needs each macroblock's QP, which only
	// the slice decoders know (mb_qp_delta). 0xFF means "not recorded" and falls back to the
	// slice QP.
	uint8* mb_qp_table;
	int    mb_qp_cap;

	// Per-macroblock deblocking input of the picture being decoded (H.264 8.7.2.1). The slice
	// decoders fill these tables, H264_DeblockFrame consumes them. db_flags is 4 bytes per
	// macroblock ([0] intra, [1] transform_size_8x8_flag, [2] luma coded block pattern), db_mv is
	// two lists x 16 4x4 blocks x (x, y) and db_ref two lists x 16 4x4 blocks of picture order
	// counts (-1 = "list not used") per macroblock.
	uint8* db_flags;
	int16* db_mv;
	int16* db_ref;
	int    db_cap;

	// --- minimal decoded picture buffer: reference pictures tagged with their picture order
	// count. B slices need the picture that is displayed *before* the current one (list0), which
	// is not necessarily the picture decoded last, so the references have to be ordered by POC. ---
	static const int H264_DPB_SLOTS = 16;
	struct H264RefSlot {
		uint8* y;
		uint8* u;
		uint8* v;
		// Motion data of this stored picture, used as the collocated picture by temporal
		// direct prediction of a B slice: one (x,y) pair per 4x4 luma block and one list0
		// reference index per 4x4 luma block. Both stay zero for an intra picture, which is
		// exactly what the collocated derivation needs for an intra collocated macroblock.
		H264MotionVector* mv;
		uint8* refidx;
		H264MotionVector* mv1;
		uint8* refidx1;
		int    l0_poc[H264_MAX_REF_LIST];
		int    n_l0_poc;
		int    poc;
		// frame_num of this picture. H.264 8.2.4.2.1 orders the reference picture lists by
		// PicNum (= frame_num), NOT by POC: with B pictures in the DPB the two orders differ,
		// and every reference index then points at the wrong picture.
		int    frame_num;
		bool   used;
	};
	H264RefSlot dpb[H264_DPB_SLOTS];
	int dpb_max_refs;   // max_num_ref_frames of the active SPS (8.2.5.3 sliding window)
	int  dpb_mv_mbs;
	int  poc_msb;
	int  poc_prev_lsb;
	int  poc_prev_msb;
	bool poc_have_prev;

	// --- output (display) order ------------------------------------------------------------------
	// A decoder reconstructs pictures in *coded* order, but H.264 8.2.4/C.5.2.2 hands them out in
	// increasing PicOrderCntVal: the reference picture of a B picture is coded before it and shown
	// after it.  Every reconstructed picture is therefore parked here and the one with the smallest
	// POC is released as soon as more than max_num_ref_frames pictures are waiting; a picture that
	// is still needed in front of a later one stays parked.  Without this the caller receives
	// I,P,B,B,... and the display jumps back and forth.
	static const int H264_OUT_QUEUE = 16;
	struct H264OutPic {
		uint8* y;
		uint8* u;
		uint8* v;
		int    poc;
		int    frame_type;
		// An IDR restarts PicOrderCntVal at 0, so POC alone would sort the pictures of an older
		// sequence after the new IDR.  Every IDR opens a new epoch and pictures of an older epoch
		// are always released first.
		uint32 epoch;
		bool   used;
	};
	H264OutPic out_q[H264_OUT_QUEUE];
	int  out_hold;   // pictures currently parked
	int  out_delay;  // how many must be parked before the first one is due
	uint32 out_epoch;
	int  pending_poc; // PicOrderCntVal of the picture that has just been reconstructed

	byte* rbsp_buf;
	size_t rbsp_buf_cap;

	H264DecoderImpl(int def_w, int def_h)
		: default_w(def_w > 0 ? def_w : 320),
		  default_h(def_h > 0 ? def_h : 240),
		  active_sps_id(-1), active_pps_id(-1),
		  cur_y(nullptr), cur_u(nullptr), cur_v(nullptr),
		  ref_y(nullptr), ref_u(nullptr), ref_v(nullptr),
		  buf_w(0), buf_h(0), y_stride(0), uv_stride(0),
		  pic_y_size(0), pic_uv_size(0),
		  mb_qp_table(nullptr), mb_qp_cap(0),
		  db_flags(nullptr), db_mv(nullptr), db_ref(nullptr), db_cap(0),
		  dpb_mv_mbs(0),
		  mv_export_buf(nullptr), ref_export_buf(nullptr),
		  mv1_export_buf(nullptr), ref1_export_buf(nullptr), mv_export_mbs(0),
		  poc_msb(0), poc_prev_lsb(0), poc_have_prev(false),
		  out_hold(0), out_delay(0), out_epoch(0), pending_poc(0),
		  rbsp_buf(nullptr), rbsp_buf_cap(0) {
		MemSet(sps_list, 0, sizeof(sps_list));
		MemSet(pps_list, 0, sizeof(pps_list));
		MemSet(dpb, 0, sizeof(dpb));
		MemSet(out_q, 0, sizeof(out_q));
	}

	~H264DecoderImpl() {
		FreeBuffers();
		OutputQueueClear();
		if (rbsp_buf) {
			free(rbsp_buf);
			rbsp_buf = nullptr;
			rbsp_buf_cap = 0;
		}
	}

	//=================================================================================
	// Reference pictures (minimal decoded picture buffer)
	//=================================================================================

	void ClearDPB() {
		for (int i = 0; i < H264_DPB_SLOTS; ++i) {
			if (dpb[i].y) { free(dpb[i].y); dpb[i].y = nullptr; }
			if (dpb[i].u) { free(dpb[i].u); dpb[i].u = nullptr; }
			if (dpb[i].v) { free(dpb[i].v); dpb[i].v = nullptr; }
			if (dpb[i].mv) { free(dpb[i].mv); dpb[i].mv = nullptr; }
			if (dpb[i].refidx) { free(dpb[i].refidx); dpb[i].refidx = nullptr; }
			if (dpb[i].mv1) { free(dpb[i].mv1); dpb[i].mv1 = nullptr; }
			if (dpb[i].refidx1) { free(dpb[i].refidx1); dpb[i].refidx1 = nullptr; }
			dpb[i].n_l0_poc = 0;
			dpb[i].poc = 0;
			dpb[i].frame_num = 0;
			dpb[i].used = false;
		}
	}

	//=================================================================================
	// Output (display) order
	//=================================================================================

	void OutputQueueClear() {
		for (int i = 0; i < H264_OUT_QUEUE; ++i) {
			if (out_q[i].y) { free(out_q[i].y); out_q[i].y = nullptr; }
			if (out_q[i].u) { free(out_q[i].u); out_q[i].u = nullptr; }
			if (out_q[i].v) { free(out_q[i].v); out_q[i].v = nullptr; }
			out_q[i].used = false;
		}
		out_hold = 0;
		out_epoch = 0;
	}

	// Parks the picture that was just reconstructed.  The parking depth is the decoder's reorder
	// buffer, which the spec sizes with max_num_reorder_frames; that field lives in the VUI, which
	// this parser does not read, so max_num_ref_frames (an upper bound on the reordering depth of
	// any conforming stream) is used instead.
	bool OutputQueuePush(int poc, int frame_type) {
		if (!cur_y || pic_y_size == 0) return false;
		int slot = -1;
		for (int i = 0; i < H264_OUT_QUEUE; ++i) if (!out_q[i].used) { slot = i; break; }
		if (slot < 0) return false;
		H264OutPic& q = out_q[slot];
		if (!q.y) {
			q.y = (uint8*)malloc(pic_y_size);
			q.u = (uint8*)malloc(pic_uv_size);
			q.v = (uint8*)malloc(pic_uv_size);
			if (!q.y || !q.u || !q.v) {
				if (q.y) { free(q.y); q.y = nullptr; }
				if (q.u) { free(q.u); q.u = nullptr; }
				if (q.v) { free(q.v); q.v = nullptr; }
				return false;
			}
		}
		MemCopyN(q.y, cur_y, pic_y_size);
		MemCopyN(q.u, cur_u, pic_uv_size);
		MemCopyN(q.v, cur_v, pic_uv_size);
		q.poc = poc;
		q.frame_type = frame_type;
		q.epoch = out_epoch;
		q.used = true;
		++out_hold;
		out_delay = (dpb_max_refs > 0) ? dpb_max_refs : 0;
		if (out_delay > H264_OUT_QUEUE - 1) out_delay = H264_OUT_QUEUE - 1;
		return true;
	}

	// Index of the picture that is due next, or -1 while the reorder buffer is not full yet.
	int OutputQueueNext(bool flush) {
		if (out_hold <= 0) return -1;
		if (!flush && out_hold <= out_delay) return -1;
		int best = -1;
		for (int i = 0; i < H264_OUT_QUEUE; ++i) {
			if (!out_q[i].used) continue;
			if (best < 0 || out_q[i].epoch < out_q[best].epoch ||
				(out_q[i].epoch == out_q[best].epoch && out_q[i].poc < out_q[best].poc)) best = i;
		}
		return best;
	}

	// Releases the next due picture as BGRA rows cropped to the display size.
	uni::Color* OutputQueueTake(bool flush, int* out_w, int* out_h, int* out_frame_type) {
		const int slot = OutputQueueNext(flush);
		if (slot < 0) return nullptr;
		EnsureDefaultParameters();
		const H264SPS* sps = &sps_list[active_sps_id >= 0 ? active_sps_id : 0];
		const int pic_w = (int)sps->width;
		const int pic_h = (int)sps->height;
		if (pic_w <= 0 || pic_h <= 0) return nullptr;

		H264OutPic& q = out_q[slot];
		const size_t pixel_cnt = (size_t)pic_w * (size_t)pic_h;
		uni::Color* pixels = (uni::Color*)malloc(pixel_cnt * sizeof(uni::Color));
		if (!pixels) return nullptr;

		for (int y = 0; y < pic_h; ++y) {
			const uint8* row_y = q.y + (size_t)y * y_stride;
			const uint8* row_u = q.u + (size_t)(y / 2) * uv_stride;
			const uint8* row_v = q.v + (size_t)(y / 2) * uv_stride;
			uni::Color* dst_row = pixels + (size_t)y * pic_w;
			for (int x = 0; x < pic_w; ++x) {
				dst_row[x] = uni::Color::FromYCbCr(row_y[x], row_u[x / 2], row_v[x / 2],
					H264YCbCrMatrix((int)sps->matrix_coefficients),
					sps->video_full_range_flag ? uni::YCbCrRange::Full : uni::YCbCrRange::Limited);
			}
		}

		if (out_w) *out_w = pic_w;
		if (out_h) *out_h = pic_h;
		if (out_frame_type) *out_frame_type = q.frame_type;

		q.used = false;
		--out_hold;
		return pixels;
	}

	// picture order count for pic_order_cnt_type == 0 (the type this decoder supports).
	// H.264 8.2.1.1: PicOrderCntMsb is derived from the *previous reference picture*, so a
	// non-reference B picture must not update prevPicOrderCntLsb/Msb.  Letting every picture
	// update them made the msb wrap condition fire on a B picture's lsb, which added
	// MaxPicOrderCntLsb (64 here) to the POC of the next P picture: the POCs ran
	// 8,16,...,64,132,140,...  A wrong POC then mis-sorts the reference lists and gives the
	// implicit B prediction weights the wrong picture distances.
	int AdvancePOC(bool is_idr, bool is_ref, int poc_lsb, const H264SPS* sps) {
		if (is_idr) {
			ClearDPB();
			// The pictures parked for output belong to the sequence that ends here: they keep
			// their (smaller) epoch and are therefore released before this IDR.
			++out_epoch;
			poc_msb = 0;
			poc_prev_msb = 0;
			poc_prev_lsb = poc_lsb;
			poc_have_prev = true;
			return poc_lsb;
		}
		const int log2_lsb = (int)sps->log2_max_pic_order_cnt_lsb_minus4 + 4;
		const int max_lsb = (log2_lsb > 0 && log2_lsb < 31) ? (1 << log2_lsb) : 256;
		if (poc_have_prev) {
			if (poc_lsb < poc_prev_lsb && (poc_prev_lsb - poc_lsb) >= max_lsb / 2) {
				poc_msb = poc_prev_msb + max_lsb;
			} else if (poc_lsb > poc_prev_lsb && (poc_lsb - poc_prev_lsb) > max_lsb / 2) {
				poc_msb = poc_prev_msb - max_lsb;
			} else {
				poc_msb = poc_prev_msb;
			}
		} else {
			poc_msb = 0;
		}
		const int full = poc_msb + poc_lsb;
		if (is_ref) {
			poc_prev_lsb = poc_lsb;
			poc_prev_msb = poc_msb;
			poc_have_prev = true;
		}
		return full;
	}

	// Copies the list0 reference (the stored picture with the largest POC below the current
	// one, i.e. the picture displayed just before) into the reference planes used by the
	// motion compensation paths. Falls back to the newest stored picture.
	bool SelectList0Reference(int cur_poc) {
		int best = -1;
		for (int i = 0; i < H264_DPB_SLOTS; ++i) {
			if (!dpb[i].used) continue;
			if (dpb[i].poc < cur_poc && (best < 0 || dpb[i].poc > dpb[best].poc)) best = i;
		}
		if (best < 0) {
			for (int i = 0; i < H264_DPB_SLOTS; ++i) {
				if (!dpb[i].used) continue;
				if (best < 0 || dpb[i].poc > dpb[best].poc) best = i;
			}
		}
		if (best < 0 || !dpb[best].y || !ref_y) return false;
		MemCopyN(ref_y, dpb[best].y, pic_y_size);
		MemCopyN(ref_u, dpb[best].u, pic_uv_size);
		MemCopyN(ref_v, dpb[best].v, pic_uv_size);
		return true;
	}

	// Stores the picture that was just reconstructed as a reference (sliding window: when all
	// slots are taken the one with the smallest POC is dropped).
	void InsertReferencePicture(int cur_poc, int cur_frame_num,
								const H264MotionVector* mv_src, const uint8* refidx_src,
								const H264MotionVector* mv1_src, const uint8* refidx1_src,
								const int* l0_poc_src, int n_l0_src,
								bool is_idr = false,
								const H264SliceHeader* marking = nullptr, int max_frame_num = 0) {
		if (!cur_y || pic_y_size == 0) return;
		// H.264 8.2.5.1 / 8.2.5.2: an instantaneous decoding refresh picture marks every
		// reference picture as "unused for reference" before it is stored, so the decoded
		// picture buffer is empty for the slices that follow it.  Without the flush the
		// pictures that preceded the IDR stayed in the buffer, and because frame_num repeats
		// every MaxFrameNum the RefPicListReordering commands of the next slices resolved
		// their short-term picture numbers against those stale entries: whole regions then
		// predicted from a picture tens of frames old (a P slice after the second IDR built
		// list0 = 64 124 .. instead of the pictures around it).
		if (is_idr) {
			for (int i = 0; i < H264_DPB_SLOTS; ++i) dpb[i].used = false;
		}
		const bool adaptive_marking = (marking != nullptr && marking->adaptive_ref_pic_marking != 0);
		// H.264 8.2.5.4: the adaptive marking operations run in order before the current picture
		// is stored, and they replace the sliding window of 8.2.5.3 entirely.  Only the two
		// operations that a short-term-only buffer can carry out are applied: 1 marks one
		// short-term picture unused, 5 marks every one of them unused.  Operations 2, 3, 4 and 6
		// address long-term reference indices, which this decoder does not keep, so a stream that
		// uses them falls back to the sliding window below.
		if (adaptive_marking && max_frame_num > 0 && !is_idr) {
			for (int i = 0; i < marking->n_mmco; ++i) {
				const uint32 op = marking->mmco[i] >> 16;
				const uint32 arg = marking->mmco[i] & 0xFFFF;
				if (op == 1) {
					// PicNumLX = CurrPicNum - ( difference_of_pic_nums_minus1 + 1 ), reduced
					// modulo MaxFrameNum.
					int pic_num = ((int)cur_frame_num - (int)(arg + 1)) % max_frame_num;
					if (pic_num < 0) pic_num += max_frame_num;
					for (int s = 0; s < H264_DPB_SLOTS; ++s) {
						if (!dpb[s].used || dpb[s].poc == cur_poc) continue;
						if ((dpb[s].frame_num % max_frame_num) == pic_num) { dpb[s].used = false; break; }
					}
				} else if (op == 5) {
					for (int s = 0; s < H264_DPB_SLOTS; ++s)
						if (dpb[s].used && dpb[s].poc != cur_poc) dpb[s].used = false;
				}
			}
		}
		int slot = -1;
		for (int i = 0; i < H264_DPB_SLOTS; ++i) {
			if (dpb[i].used && dpb[i].poc == cur_poc) { slot = i; break; }
		}
		if (slot < 0) {
			// H.264 8.2.5.3, sliding window: the decoded picture buffer keeps at most
			// max_num_ref_frames reference pictures, and the one with the smallest
			// PicOrderCntVal is marked "unused for reference" when a new one arrives.
			// Parking up to H264_DPB_SLOTS pictures instead left stale entries that share a
			// frame_num with a newer picture, so RefPicListReordering could resolve a
			// short-term command to a picture from a hundred frames ago.
			int limit = (dpb_max_refs > 0) ? dpb_max_refs : H264_DPB_SLOTS;
			if (limit > H264_DPB_SLOTS) limit = H264_DPB_SLOTS;
			int used = 0;
			for (int i = 0; i < H264_DPB_SLOTS; ++i) if (dpb[i].used) ++used;
			while (!adaptive_marking && used >= limit) {
				int victim = -1;
				for (int i = 0; i < H264_DPB_SLOTS; ++i) {
					if (!dpb[i].used || dpb[i].poc == cur_poc) continue;
					if (victim < 0 || dpb[i].poc < dpb[victim].poc) victim = i;
				}
				if (victim < 0) break;
				dpb[victim].used = false;
				--used;
			}
			for (int i = 0; i < H264_DPB_SLOTS; ++i) {
				if (!dpb[i].used) { slot = i; break; }
			}
		}
		if (slot < 0) {
			slot = 0;
			for (int i = 1; i < H264_DPB_SLOTS; ++i) {
				if (dpb[i].poc < dpb[slot].poc) slot = i;
			}
		}

		const int mbs = dpb_mv_mbs;
		H264RefSlot& s = dpb[slot];
		if (!s.y) {
			s.y = (uint8*)malloc(pic_y_size);
			s.u = (uint8*)malloc(pic_uv_size);
			s.v = (uint8*)malloc(pic_uv_size);
			s.mv = (H264MotionVector*)malloc((size_t)mbs * 16 * sizeof(H264MotionVector));
			s.refidx = (uint8*)malloc((size_t)mbs * 16);
			s.mv1 = (H264MotionVector*)malloc((size_t)mbs * 16 * sizeof(H264MotionVector));
			s.refidx1 = (uint8*)malloc((size_t)mbs * 16);
			if (!s.y || !s.u || !s.v || !s.mv || !s.refidx || !s.mv1 || !s.refidx1) {
				if (s.y) { free(s.y); s.y = nullptr; }
				if (s.u) { free(s.u); s.u = nullptr; }
				if (s.v) { free(s.v); s.v = nullptr; }
				if (s.mv) { free(s.mv); s.mv = nullptr; }
				if (s.refidx) { free(s.refidx); s.refidx = nullptr; }
				if (s.mv1) { free(s.mv1); s.mv1 = nullptr; }
				if (s.refidx1) { free(s.refidx1); s.refidx1 = nullptr; }
				s.used = false;
				return;
			}
		}
		MemCopyN(s.y, cur_y, pic_y_size);
		MemCopyN(s.u, cur_u, pic_uv_size);
		MemCopyN(s.v, cur_v, pic_uv_size);
		if (mv_src) {
			MemCopyN(s.mv, mv_src, (size_t)mbs * 16 * sizeof(H264MotionVector));
		} else {
			MemSet(s.mv, 0, (size_t)mbs * 16 * sizeof(H264MotionVector));
		}
		if (refidx_src) {
			MemCopyN(s.refidx, refidx_src, (size_t)mbs * 16);
		} else {
			// An unknown motion field has to read as "list not used" (255), not as "reference 0
			// with a zero motion vector": the difference decides whether a spatial direct
			// macroblock takes a zero vector and whether a temporal one sees refIdxCol = 0.
			MemSet(s.refidx, 255, (size_t)mbs * 16);
		}
		if (mv1_src) {
			MemCopyN(s.mv1, mv1_src, (size_t)mbs * 16 * sizeof(H264MotionVector));
		} else {
			MemSet(s.mv1, 0, (size_t)mbs * 16 * sizeof(H264MotionVector));
		}
		if (refidx1_src) {
			MemCopyN(s.refidx1, refidx1_src, (size_t)mbs * 16);
		} else {
			// A picture that never predicts from list1 (a P or I picture) leaves list1 unused
			// in every block.
			MemSet(s.refidx1, 255, (size_t)mbs * 16);
		}
		s.n_l0_poc = 0;
		if (l0_poc_src && n_l0_src > 0) {
			const int n = (n_l0_src < H264_MAX_REF_LIST) ? n_l0_src : H264_MAX_REF_LIST;
			for (int q = 0; q < n; ++q) s.l0_poc[q] = l0_poc_src[q];
			s.n_l0_poc = n;
		}
		s.poc = cur_poc;
		s.frame_num = cur_frame_num;
		s.used = true;
	}

	// H.264 8.2.4 reference picture list initialisation and 8.2.4.2.2/8.2.4.2.3 reordering.
	//
	// The initial lists are ordered by PicNum (= frame_num), NOT by POC.  With B pictures in the
	// DPB the two orders differ (decode order is not POC order), so ordering by POC makes every
	// reference index select the wrong picture.  The "distance" d = (CurrPicNum - PicNum) mod
	// MaxPicNum identifies the pictures that precede the current one: those with a small
	// non-zero d.  Encoders also emit ref_pic_list_reordering for most B pictures, which then
	// moves specific pictures to the front of the list.
	int BuildRefLists(int cur_poc, int cur_num, int max_num,
					  const uint32* rl0, int n_rl0, const uint32* rl1, int n_rl1,
					  H264RefPicDesc* l0, int* out_n0, int l0_cap,
					  H264RefPicDesc* l1, int* out_n1, int l1_cap) {
		(void)cur_poc;
		static const int MAXC = 32;
		int cnt = 0;
		int pool[MAXC];
		for (int i = 0; i < H264_DPB_SLOTS && cnt < MAXC; ++i) {
			if (dpb[i].used && dpb[i].y) {
				int d = (cur_num - dpb[i].frame_num) % max_num;
				if (d < 0) d += max_num;
				if (d == 0) continue;                 // the current picture itself
				pool[cnt++] = i;
			}
		}
		H264RefPicDesc* dst[2] = { l0, l1 };
		const uint32* cmds[2] = { rl0, rl1 };
		const int ncmds[2] = { n_rl0, n_rl1 };
		int cap[2] = { l0_cap, l1_cap };
		int got[2] = { 0, 0 };
		for (int list = 0; list < 2; ++list) {
			// order key: list0 walks the preceding pictures nearest-first (descending PicNum)
			// and then the following ones in ascending PicNum; list1 is the mirror image.
			int ord[MAXC];
			int nord = 0;
			for (int i = 0; i < cnt; ++i) ord[nord++] = pool[i];
			for (int i = 1; i < nord; ++i) {
				int key = ord[i];
				int dk = (cur_num - dpb[key].frame_num) % max_num;
				if (dk < 0) dk += max_num;
				const int pk_ = dpb[key].poc;
				int kk = (list == 0) ? ((pk_ < cur_poc) ? (cur_poc - pk_) : (max_num + (pk_ - cur_poc)))
									 : ((pk_ > cur_poc) ? (pk_ - cur_poc) : (max_num + (cur_poc - pk_)));
				int j = i - 1;
				while (j >= 0) {
					int dj = (cur_num - dpb[ord[j]].frame_num) % max_num;
					if (dj < 0) dj += max_num;
					const int pj_ = dpb[ord[j]].poc;
					int kj = (list == 0) ? ((pj_ < cur_poc) ? (cur_poc - pj_) : (max_num + (pj_ - cur_poc)))
										 : ((pj_ > cur_poc) ? (pj_ - cur_poc) : (max_num + (cur_poc - pj_)));
					if (kj <= kk) break;
					ord[j + 1] = ord[j];
					--j;
				}
				ord[j + 1] = key;
			}
			// H.264 8.2.4.2: the list holds num_ref_idx_lX_active entries, and 8.2.4.3.1 needs
			// one spare slot behind them that the insertion shifts into.  Fewer reference
			// pictures than active entries are padded by repeating the last one.
			const int active = cap[list];
			const int need = active + 1;
			if (nord > need) nord = need;
			while (nord > 0 && nord < need) {
				int keep = ord[nord - 1];
				ord[nord++] = keep;
			}
			// H.264 8.2.4.3.1 (equation 8-38): a command names a short-term reference picture by
			// its PicNum, relative to the PicNum the previous command named (the current picture's
			// frame_num for the first one).  The picture is inserted at refIdxLX, everything from
			// there on moves one index up, and the other copies of the SAME picture are removed
			// again - which is why a list shorter than num_ref_idx_lX_active ends up with the
			// named picture repeated.  The P slice at POC 16 carries the commands
			// (0,1) (0,15) (1,0) (0,1) and must give [POC 8, POC 8, POC 4, POC 0].
			int placed = 0;
			int pred = cur_num;
			for (int c = 0; c < ncmds[list] && placed < active; ++c) {
				uint32 idc = cmds[list][c] >> 16;
				int arg = (int)(cmds[list][c] & 0xFFFFu);
				if (idc > 1) break;                    // long-term / end of list: not used here
				int pn = pred - (arg + 1);             // (8-35)
				if (idc == 1) pn = pred + (arg + 1);   // (8-36)
				pn %= max_num;
				if (pn < 0) pn += max_num;
				pred = pn;
				// The picture is looked up among ALL reference pictures, not only among the ones
				// the initial list holds: a picture that is already placed may be named again.
				int v = -1;
				for (int i = 0; i < cnt; ++i) {
					if (dpb[pool[i]].frame_num % max_num == pn) { v = pool[i]; break; }
				}
				if (v < 0) break;
				for (int cIdx = active; cIdx > placed; --cIdx) ord[cIdx] = ord[cIdx - 1];
				ord[placed] = v;
				int nIdx = placed + 1;
				for (int cIdx = placed + 1; cIdx <= active; ++cIdx) {
					if (dpb[ord[cIdx]].frame_num % max_num != pn) ord[nIdx++] = ord[cIdx];
				}
				++placed;
			}
			for (int i = 0; i < nord && got[list] < cap[list]; ++i) {
				H264RefPicDesc& e = dst[list][got[list]];
				e.y = dpb[ord[i]].y; e.u = dpb[ord[i]].u; e.v = dpb[ord[i]].v;
				e.y_stride = y_stride; e.uv_stride = uv_stride;
				e.mv = dpb[ord[i]].mv; e.refidx = dpb[ord[i]].refidx;
				e.mv1 = dpb[ord[i]].mv1; e.refidx1 = dpb[ord[i]].refidx1;
				e.list0_poc = dpb[ord[i]].l0_poc; e.n_list0 = dpb[ord[i]].n_l0_poc;
				e.poc = dpb[ord[i]].poc;
				++got[list];
			}
			// H.264 8.2.4.2.1: when the list holds fewer pictures than the number of active
			// references, the last entry is repeated until RefPicListX is full.  Without it a
			// coded ref_idx that names one of the repeated positions selected nothing at all,
			// so the partition predicted from whichever reference happened to be current.
			while (got[list] > 0 && got[list] < cap[list]) {
				dst[list][got[list]] = dst[list][got[list] - 1];
				++got[list];
			}
		}
		*out_n0 = got[0];
		*out_n1 = got[1];
		return (got[0] > 0 && got[1] > 0) ? 1 : 0;
	}

	// Scratch buffers that receive the per-4x4 motion data of a reconstructed reference
	// picture, so that the DPB slot can keep it for temporal direct prediction.
	H264MotionVector* mv_export_buf;
	uint8* ref_export_buf;
	H264MotionVector* mv1_export_buf;
	uint8* ref1_export_buf;
	int    mv_export_mbs;

	// Each picture starts from "no motion data and neither list used" (255), so a picture whose
	// slice decoder exports nothing - an intra picture - stores an empty motion field instead of
	// whatever the previously decoded picture left in the buffer.
	H264MotionVector* ExportMVTemp() {
		if (mv_export_mbs <= 0) return nullptr;
		MemSet(mv_export_buf, 0, (size_t)mv_export_mbs * 16 * sizeof(H264MotionVector));
		return mv_export_buf;
	}
	uint8* ExportRefTemp() {
		if (mv_export_mbs <= 0) return nullptr;
		MemSet(ref_export_buf, 255, (size_t)mv_export_mbs * 16);
		return ref_export_buf;
	}
	H264MotionVector* ExportMVTemp1() {
		if (mv_export_mbs <= 0) return nullptr;
		MemSet(mv1_export_buf, 0, (size_t)mv_export_mbs * 16 * sizeof(H264MotionVector));
		return mv1_export_buf;
	}
	uint8* ExportRefTemp1() {
		if (mv_export_mbs <= 0) return nullptr;
		MemSet(ref1_export_buf, 255, (size_t)mv_export_mbs * 16);
		return ref1_export_buf;
	}

	void FreeBuffers() {
		if (cur_y) { free(cur_y); cur_y = nullptr; }
		if (mb_qp_table) { free(mb_qp_table); mb_qp_table = nullptr; mb_qp_cap = 0; }
		if (db_flags) { free(db_flags); db_flags = nullptr; }
		if (db_mv) { free(db_mv); db_mv = nullptr; }
		if (db_ref) { free(db_ref); db_ref = nullptr; }
		db_cap = 0;
		if (cur_u) { free(cur_u); cur_u = nullptr; }
		if (cur_v) { free(cur_v); cur_v = nullptr; }
		if (ref_y) { free(ref_y); ref_y = nullptr; }
		if (ref_u) { free(ref_u); ref_u = nullptr; }
		if (ref_v) { free(ref_v); ref_v = nullptr; }
		if (mv_export_buf) { free(mv_export_buf); mv_export_buf = nullptr; }
		if (ref_export_buf) { free(ref_export_buf); ref_export_buf = nullptr; }
		if (mv1_export_buf) { free(mv1_export_buf); mv1_export_buf = nullptr; }
		if (ref1_export_buf) { free(ref1_export_buf); ref1_export_buf = nullptr; }
		mv_export_mbs = 0;
		ClearDPB();
		// The parked pictures were copied with the strides that are being dropped here.
		OutputQueueClear();
		poc_msb = 0;
		poc_prev_lsb = 0;
		poc_have_prev = false;
		pic_y_size = 0;
		pic_uv_size = 0;
		buf_w = 0;
		buf_h = 0;
		y_stride = 0;
		uv_stride = 0;
	}

	bool AllocateBuffers(int w, int h) {
		if (buf_w == w && buf_h == h && cur_y && ref_y) return true;
		FreeBuffers();

		int mb_w = (w + 15) / 16;
		int mb_h = (h + 15) / 16;
		y_stride = mb_w * 16;
		uv_stride = mb_w * 8;
		size_t y_size = (size_t)y_stride * (mb_h * 16);
		size_t uv_size = (size_t)uv_stride * (mb_h * 8);

		cur_y = (uint8*)malloc(y_size);
		cur_u = (uint8*)malloc(uv_size);
		cur_v = (uint8*)malloc(uv_size);
		ref_y = (uint8*)malloc(y_size);
		ref_u = (uint8*)malloc(uv_size);
		ref_v = (uint8*)malloc(uv_size);

		if (!cur_y || !cur_u || !cur_v || !ref_y || !ref_u || !ref_v) {
			FreeBuffers();
			return false;
		}

		MemSet(cur_y, 0, y_size);
		MemSet(cur_u, 128, uv_size);
		MemSet(cur_v, 128, uv_size);
		MemSet(ref_y, 0, y_size);
		MemSet(ref_u, 128, uv_size);
		MemSet(ref_v, 128, uv_size);

		buf_w = w;
		buf_h = h;
		pic_y_size = y_size;
		pic_uv_size = uv_size;
		dpb_mv_mbs = mb_w * mb_h;
		mv_export_mbs = mb_w * mb_h;
		if (!mv_export_buf) mv_export_buf = (H264MotionVector*)malloc((size_t)mv_export_mbs * 16 * sizeof(H264MotionVector));
		if (!ref_export_buf) ref_export_buf = (uint8*)malloc((size_t)mv_export_mbs * 16);
		if (!mv1_export_buf) mv1_export_buf = (H264MotionVector*)malloc((size_t)mv_export_mbs * 16 * sizeof(H264MotionVector));
		if (!ref1_export_buf) ref1_export_buf = (uint8*)malloc((size_t)mv_export_mbs * 16);
		if (!mv_export_buf || !ref_export_buf || !mv1_export_buf || !ref1_export_buf) { mv_export_mbs = 0; }
		else {
			MemSet(mv_export_buf, 0, (size_t)mv_export_mbs * 16 * sizeof(H264MotionVector));
			MemSet(ref_export_buf, 255, (size_t)mv_export_mbs * 16);
			MemSet(mv1_export_buf, 0, (size_t)mv_export_mbs * 16 * sizeof(H264MotionVector));
			MemSet(ref1_export_buf, 255, (size_t)mv_export_mbs * 16);
		}
		{
			int mbs = mb_w * mb_h;
			if (mb_qp_cap < mbs) {
				if (mb_qp_table) free(mb_qp_table);
				mb_qp_table = (uint8*)malloc((size_t)mbs);
				mb_qp_cap = mb_qp_table ? mbs : 0;
			}
			if (mb_qp_table) MemSet(mb_qp_table, 0xFF, mbs);
			if (db_cap < mbs) {
				if (db_flags) free(db_flags);
				if (db_mv) free(db_mv);
				if (db_ref) free(db_ref);
				db_flags = (uint8*)malloc((size_t)mbs * 4);
				db_mv = (int16*)malloc((size_t)mbs * 64 * sizeof(int16));
				db_ref = (int16*)malloc((size_t)mbs * 32 * sizeof(int16));
				db_cap = (db_flags && db_mv && db_ref) ? mbs : 0;
			}
			if (db_cap >= mbs) {
				MemSet(db_flags, 0, (size_t)mbs * 4);
				MemSet(db_mv, 0, (size_t)mbs * 64 * sizeof(int16));
				MemSet(db_ref, 0, (size_t)mbs * 32 * sizeof(int16));
			}
		}
		return true;
	}

	byte* EnsureRBSPBuffer(size_t size) {
		if (size > rbsp_buf_cap) {
			size_t new_cap = size < 4096 ? 4096 : size * 2;
			byte* nb = (byte*)realloc(rbsp_buf, new_cap);
			if (!nb) return nullptr;
			rbsp_buf = nb;
			rbsp_buf_cap = new_cap;
		}
		return rbsp_buf;
	}

	void EnsureDefaultParameters() {
		const bool have_sps = (active_sps_id >= 0 && sps_list[active_sps_id].is_valid);
		const bool have_pps = (active_pps_id >= 0 && pps_list[active_pps_id].is_valid);
		if (have_sps && have_pps) return;

		// Synthesize only the half that is missing. A parameter set that was already parsed has
		// to survive: MP4 delivers SPS/PPS only once (via avcC), so clobbering it is permanent.
		if (!have_sps) {
			H264SPS def_sps{};
			def_sps.profile_idc = H264_PROFILE_BASELINE;
			def_sps.level_idc = 30;
			def_sps.seq_parameter_set_id = 0;
			def_sps.log2_max_frame_num_minus4 = 0;
			def_sps.pic_order_cnt_type = 0;
			def_sps.log2_max_pic_order_cnt_lsb_minus4 = 0;
			def_sps.max_num_ref_frames = 1;
			def_sps.pic_width_in_mbs_minus1 = (default_w + 15) / 16 - 1;
			def_sps.pic_height_in_map_units_minus1 = (default_h + 15) / 16 - 1;
			def_sps.frame_mbs_only_flag = 1;
			def_sps.direct_8x8_inference_flag = 1;
			def_sps.mb_width = def_sps.pic_width_in_mbs_minus1 + 1;
			def_sps.mb_height = def_sps.pic_height_in_map_units_minus1 + 1;
			def_sps.width = def_sps.mb_width * 16;
			def_sps.height = def_sps.mb_height * 16;
			def_sps.is_valid = true;
			const int slot = (active_sps_id >= 0) ? active_sps_id : 0;
			if (!sps_list[slot].is_valid) sps_list[slot] = def_sps;
			active_sps_id = slot;
		}

		if (!have_pps) {
			H264PPS def_pps{};
			def_pps.pic_parameter_set_id = 0;
			def_pps.seq_parameter_set_id = 0;
			def_pps.entropy_coding_mode_flag = 0; // CAVLC
			def_pps.pic_init_qp_minus26 = 0;
			def_pps.chroma_qp_index_offset = 0;
			def_pps.deblocking_filter_control_present_flag = 1;
			def_pps.is_valid = true;
			const int slot = (active_pps_id >= 0) ? active_pps_id : 0;
			if (!pps_list[slot].is_valid) pps_list[slot] = def_pps;
			active_pps_id = slot;
		}
	}

	uni::Color* DecodeFrame(
		const byte* data,
		size_t size,
		int* out_w,
		int* out_h,
		int* out_frame_type
	) {
		// A null buffer asks for the pictures that are still parked by the output reordering; the
		// caller drains them after the last access unit.
		if (!data) {
			return (size == 0) ? OutputQueueTake(true, out_w, out_h, out_frame_type) : nullptr;
		}
		if (size == 0) return nullptr;

		size_t pos = 0;
		bool has_reconstructed_frame = false;
		int frame_type = 0; // 0 = I, 1 = P

		// Detect if stream is length-prefixed (MP4 format) or Annex B start-code delimited
		bool is_annex_b = false;
		if (size >= 3 && data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x01) {
			is_annex_b = true;
		} else if (size >= 4 && data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x00 && data[3] == 0x01) {
			is_annex_b = true;
		}

		// A leading start code alone is not conclusive: a 4-byte length field of 0x000001xx
		// also begins with 00 00 01. Trust whichever framing consumes the buffer exactly.
		if (is_annex_b && size >= 4) {
			size_t probe = 0;
			while (probe + 4 <= size) {
				uint32 probe_len = ((uint32)data[probe] << 24) | ((uint32)data[probe + 1] << 16) |
								   ((uint32)data[probe + 2] << 8) | (uint32)data[probe + 3];
				probe += 4;
				if (probe_len == 0 || probe + probe_len > size) break;
				probe += probe_len;
				if (probe == size) { is_annex_b = false; break; }
			}
		}

		if (!is_annex_b) {
			// Length-prefixed MP4 NAL units (4-byte big-endian size)
			while (pos + 4 <= size) {
				uint32 nal_len = ((uint32)data[pos] << 24) | ((uint32)data[pos + 1] << 16) | ((uint32)data[pos + 2] << 8) | (uint32)data[pos + 3];
				pos += 4;
				if (nal_len == 0 || pos + nal_len > size) break;

				const byte* nal_bytes = data + pos;
				uint8 nal_type = nal_bytes[0] & 0x1F;
				uint8 nal_ref = (nal_bytes[0] >> 5) & 3;

				byte* rbsp = EnsureRBSPBuffer(nal_len);
				if (rbsp && nal_len > 1) {
					size_t rbsp_len = H264_UnescapeRBSP(nal_bytes + 1, nal_len - 1, rbsp, rbsp_buf_cap);

					if (nal_type == H264_NAL_SPS) {
						H264SPS parsed_sps{};
						if (H264_ParseSPS(rbsp, rbsp_len, &parsed_sps)) {
							sps_list[parsed_sps.seq_parameter_set_id % 32] = parsed_sps;
							active_sps_id = parsed_sps.seq_parameter_set_id % 32;
						}
					} else if (nal_type == H264_NAL_PPS) {
						H264PPS parsed_pps{};
						if (H264_ParsePPS(rbsp, rbsp_len, sps_list, &parsed_pps)) {
							pps_list[parsed_pps.pic_parameter_set_id % 256] = parsed_pps;
							active_pps_id = parsed_pps.pic_parameter_set_id % 256;
						}
					} else if (nal_type == H264_NAL_SLICE_IDR || nal_type == H264_NAL_SLICE_NON_IDR) {
						EnsureDefaultParameters();

						// 7.4.3 / 7.4.2.2: pic_parameter_set_id selects the PPS, that PPS names its SPS.
						{
							H264BitReader probe;
							H264_InitBitReader(&probe, rbsp, rbsp_len);
							H264_ReadUE(&probe);
							H264_ReadUE(&probe);
							const uint32 want_pps = H264_ReadUE(&probe);
							if (want_pps < 256 && pps_list[want_pps].is_valid) {
								active_pps_id = (int)want_pps;
								const uint32 want_sps = pps_list[want_pps].seq_parameter_set_id;
								if (want_sps < 32 && sps_list[want_sps].is_valid) active_sps_id = (int)want_sps;
							}
						}

						const H264SPS* sps = &sps_list[active_sps_id >= 0 ? active_sps_id : 0];
			dpb_max_refs = (int)sps->max_num_ref_frames;
						const H264PPS* pps = &pps_list[active_pps_id >= 0 ? active_pps_id : 0];

						// Buffers follow the coded macroblock grid, not the cropped display size.
						if (AllocateBuffers((int)sps->mb_width * 16, (int)sps->mb_height * 16)) {
							H264BitReader br;
							H264_InitBitReader(&br, rbsp, rbsp_len);
							H264SliceHeader sh{};
							H264_ParseSliceHeader(&br, sps, pps, nal_type, nal_ref, &sh);

							uint8 sl_type = sh.slice_type % 5;
							const bool is_idr = (nal_type == H264_NAL_SLICE_IDR);
							const bool is_b_slice = (sl_type == H264_SLICE_B);
							const int cur_poc = AdvancePOC(is_idr, nal_ref != 0, (int)sh.pic_order_cnt_lsb, sps);
							pending_poc = cur_poc;
							const bool have_ref = SelectList0Reference(cur_poc);
							H264MotionVector* mv_export = (nal_ref != 0) ? ExportMVTemp() : nullptr;
							uint8* ref_export = (nal_ref != 0) ? ExportRefTemp() : nullptr;
							H264MotionVector* mv1_export = (nal_ref != 0) ? ExportMVTemp1() : nullptr;
							uint8* ref1_export = (nal_ref != 0) ? ExportRefTemp1() : nullptr;
							int l0_poc_export[H264_MAX_REF_LIST];
							int n_l0_export = 0;
							bool ok = false;
							bool repeated = false;

							if (sl_type == H264_SLICE_I || sl_type == H264_SLICE_SI || is_idr) {
								if (pps->entropy_coding_mode_flag == 1) {
									ok = H264_DecodeSlice_CABAC_I(rbsp, rbsp_len, sps, pps, nal_type, nal_ref,
																  cur_y, cur_u, cur_v, y_stride, uv_stride, mb_qp_table,
																  db_flags);
								} else {
									ok = H264_DecodeSlice_I(rbsp, rbsp_len, sps, pps, nal_type, nal_ref,
															cur_y, cur_u, cur_v, y_stride, uv_stride);
								}
								frame_type = 0;
							} else if (sl_type == H264_SLICE_P || sl_type == H264_SLICE_SP) {
								// A P slice can activate more than one list0 picture
								// (num_ref_idx_l0_active_minus1 > 0), and the reference index of
								// each partition then selects the picture it predicts from
								// (H.264 8.2.4, 7.4.3.1).  The list has to be built here because
								// only the caller knows the slice's frame_num and reordering.
								H264RefPicDesc l0[H264_MAX_REF_LIST];
								H264RefPicDesc l1_unused[H264_MAX_REF_LIST];
								int cnt0 = 0, cnt1_unused = 0;
								BuildRefLists(cur_poc, (int)sh.frame_num,
											  1 << (sps->log2_max_frame_num_minus4 + 4),
											  sh.reorder_l0, sh.n_reorder_l0, nullptr, 0,
											  l0, &cnt0, (int)((sh.num_ref_idx_l0_active_minus1 + 1) < H264_MAX_REF_LIST ? (sh.num_ref_idx_l0_active_minus1 + 1) : H264_MAX_REF_LIST),
											  l1_unused, &cnt1_unused, (int)((sh.num_ref_idx_l1_active_minus1 + 1) < H264_MAX_REF_LIST ? (sh.num_ref_idx_l1_active_minus1 + 1) : H264_MAX_REF_LIST));
								n_l0_export = cnt0;
								for (int q = 0; q < cnt0 && q < H264_MAX_REF_LIST; ++q) l0_poc_export[q] = l0[q].poc;
								H264RefPicView list0_view[H264_MAX_REF_LIST];
								for (int q = 0; q < cnt0 && q < H264_MAX_REF_LIST; ++q) {
									list0_view[q].y = l0[q].y;
									list0_view[q].u = l0[q].u;
									list0_view[q].v = l0[q].v;
								}
								if (pps->entropy_coding_mode_flag == 1) {
									ok = H264_DecodeSlice_CABAC_P(rbsp, rbsp_len, sps, pps, nal_type, nal_ref,
																  ref_y, ref_u, ref_v, y_stride, uv_stride,
																  (cnt0 > 0) ? list0_view : nullptr, cnt0,
																  cur_y, cur_u, cur_v, y_stride, uv_stride, mb_qp_table,
																  mv_export, ref_export,
																  db_flags, db_mv, db_ref);
								} else {
									ok = H264_DecodeSlice_P(rbsp, rbsp_len, sps, pps, nal_type, nal_ref,
															ref_y, ref_u, ref_v, y_stride, uv_stride,
															cur_y, cur_u, cur_v, y_stride, uv_stride);
								}
								frame_type = 1;
							} else if (is_b_slice) {
								// A B picture is never used as a reference picture, so a B slice
								// that cannot be decoded cannot corrupt the reference chain: the
								// list0 reference picture is repeated instead.
								H264RefPicDesc l0[H264_MAX_REF_LIST], l1[H264_MAX_REF_LIST];
								int cnt0 = 0, cnt1 = 0;
								const int have_lists = BuildRefLists(cur_poc, (int)sh.frame_num, 1 << (sps->log2_max_frame_num_minus4 + 4),
										 sh.reorder_l0, sh.n_reorder_l0, sh.reorder_l1, sh.n_reorder_l1,
										 l0, &cnt0, (int)((sh.num_ref_idx_l0_active_minus1 + 1) < H264_MAX_REF_LIST ? (sh.num_ref_idx_l0_active_minus1 + 1) : H264_MAX_REF_LIST),
																	 l1, &cnt1, (int)((sh.num_ref_idx_l1_active_minus1 + 1) < H264_MAX_REF_LIST ? (sh.num_ref_idx_l1_active_minus1 + 1) : H264_MAX_REF_LIST));
								n_l0_export = cnt0;
								for (int q = 0; q < cnt0 && q < H264_MAX_REF_LIST; ++q) l0_poc_export[q] = l0[q].poc;
								if (have_ref && have_lists && pps->entropy_coding_mode_flag == 1) {
									ok = H264_DecodeSlice_CABAC_B_skip(rbsp, rbsp_len, sps, pps, nal_type, nal_ref,
																	   l0, l1, cnt0, cnt1, cur_poc,
																	   cur_y, cur_u, cur_v, y_stride, uv_stride, mb_qp_table,
																	   db_flags, db_mv, db_ref, mv_export, ref_export,
																	   mv1_export, ref1_export);
								}
								if (!ok) {
									MemCopyN(cur_y, ref_y, pic_y_size);
									MemCopyN(cur_u, ref_u, pic_uv_size);
									MemCopyN(cur_v, ref_v, pic_uv_size);
									ok = true;
									repeated = true;
								}
								frame_type = 2;
							}

							if (ok) {
								if (!repeated && sh.disable_deblocking_filter_idc != 1) {
									H264_DeblockFrame(cur_y, cur_u, cur_v, y_stride, uv_stride,
													  (int)sps->mb_width, (int)sps->mb_height,
													  sh.slice_qp, (int)sh.disable_deblocking_filter_idc,
													  (int)sh.slice_alpha_c_offset_div2 * 2,
													  (int)sh.slice_beta_offset_div2 * 2,
													  mb_qp_table, db_flags, db_mv, db_ref,
													  (int)pps->chroma_qp_index_offset);
								}

								if (!repeated && nal_ref != 0) {
									InsertReferencePicture(cur_poc, (int)sh.frame_num,
														   mv_export, ref_export,
														   mv1_export, ref1_export,
														   l0_poc_export, n_l0_export, is_idr,
														   &sh, 1 << ((int)sps->log2_max_frame_num_minus4 + 4));
								}

								has_reconstructed_frame = true;
							} else if (!is_b_slice) {
								// The slice could not be decoded (unsupported syntax or a broken
								// stream): keep showing the last reference picture instead of
								// dropping the frame, so the host never displays a blank buffer.
								if (have_ref) {
									MemCopyN(cur_y, ref_y, pic_y_size);
									MemCopyN(cur_u, ref_u, pic_uv_size);
									MemCopyN(cur_v, ref_v, pic_uv_size);
								}
								has_reconstructed_frame = true;
							}
						}
					}
				}

				pos += nal_len;
			}
		} else {
			// Annex B start-code delimited stream
			while (pos < size) {
				size_t nal_start = 0;
				size_t nal_end = 0;

				if (pos + 3 <= size && data[pos] == 0x00 && data[pos + 1] == 0x00 && data[pos + 2] == 0x01) {
					nal_start = pos + 3;
				} else if (pos + 4 <= size && data[pos] == 0x00 && data[pos + 1] == 0x00 && data[pos + 2] == 0x00 && data[pos + 3] == 0x01) {
					nal_start = pos + 4;
				} else {
					size_t sc = pos;
					while (sc + 3 <= size) {
						if (data[sc] == 0x00 && data[sc + 1] == 0x00 &&
							(data[sc + 2] == 0x01 || (sc + 4 <= size && data[sc + 2] == 0x00 && data[sc + 3] == 0x01))) {
							break;
						}
						sc++;
					}
					if (sc + 3 > size) break;
					pos = sc;
					continue;
				}

				size_t search = nal_start;
				while (search + 3 <= size) {
					if (data[search] == 0x00 && data[search + 1] == 0x00 &&
						(data[search + 2] == 0x01 || (search + 4 <= size && data[search + 2] == 0x00 && data[search + 3] == 0x01))) {
						break;
					}
					search++;
				}
				nal_end = (search + 3 <= size) ? search : size;

				size_t nal_len = (nal_end > nal_start) ? (nal_end - nal_start) : 0;
				if (nal_len == 0) {
					pos = nal_end;
					continue;
				}

				const byte* nal_bytes = data + nal_start;
				uint8 nal_type = nal_bytes[0] & 0x1F;
				uint8 nal_ref = (nal_bytes[0] >> 5) & 3;

				byte* rbsp = EnsureRBSPBuffer(nal_len);
				if (rbsp && nal_len > 1) {
					size_t rbsp_len = H264_UnescapeRBSP(nal_bytes + 1, nal_len - 1, rbsp, rbsp_buf_cap);

					if (nal_type == H264_NAL_SPS) {
						H264SPS parsed_sps{};
						if (H264_ParseSPS(rbsp, rbsp_len, &parsed_sps)) {
							sps_list[parsed_sps.seq_parameter_set_id % 32] = parsed_sps;
							active_sps_id = parsed_sps.seq_parameter_set_id % 32;
						}
					} else if (nal_type == H264_NAL_PPS) {
						H264PPS parsed_pps{};
						if (H264_ParsePPS(rbsp, rbsp_len, sps_list, &parsed_pps)) {
							pps_list[parsed_pps.pic_parameter_set_id % 256] = parsed_pps;
							active_pps_id = parsed_pps.pic_parameter_set_id % 256;
						}
					} else if (nal_type == H264_NAL_SLICE_IDR || nal_type == H264_NAL_SLICE_NON_IDR) {
						EnsureDefaultParameters();

						// 7.4.3 / 7.4.2.2: pic_parameter_set_id selects the PPS, that PPS names its SPS.
						{
							H264BitReader probe;
							H264_InitBitReader(&probe, rbsp, rbsp_len);
							H264_ReadUE(&probe);
							H264_ReadUE(&probe);
							const uint32 want_pps = H264_ReadUE(&probe);
							if (want_pps < 256 && pps_list[want_pps].is_valid) {
								active_pps_id = (int)want_pps;
								const uint32 want_sps = pps_list[want_pps].seq_parameter_set_id;
								if (want_sps < 32 && sps_list[want_sps].is_valid) active_sps_id = (int)want_sps;
							}
						}

						const H264SPS* sps = &sps_list[active_sps_id >= 0 ? active_sps_id : 0];
			dpb_max_refs = (int)sps->max_num_ref_frames;
						const H264PPS* pps = &pps_list[active_pps_id >= 0 ? active_pps_id : 0];

						// Buffers follow the coded macroblock grid, not the cropped display size.
						if (AllocateBuffers((int)sps->mb_width * 16, (int)sps->mb_height * 16)) {
							H264BitReader br;
							H264_InitBitReader(&br, rbsp, rbsp_len);
							H264SliceHeader sh{};
							H264_ParseSliceHeader(&br, sps, pps, nal_type, nal_ref, &sh);

							uint8 sl_type = sh.slice_type % 5;
							const bool is_idr = (nal_type == H264_NAL_SLICE_IDR);
							const bool is_b_slice = (sl_type == H264_SLICE_B);
							const int cur_poc = AdvancePOC(is_idr, nal_ref != 0, (int)sh.pic_order_cnt_lsb, sps);
							pending_poc = cur_poc;
							const bool have_ref = SelectList0Reference(cur_poc);
							H264MotionVector* mv_export = (nal_ref != 0) ? ExportMVTemp() : nullptr;
							uint8* ref_export = (nal_ref != 0) ? ExportRefTemp() : nullptr;
							H264MotionVector* mv1_export = (nal_ref != 0) ? ExportMVTemp1() : nullptr;
							uint8* ref1_export = (nal_ref != 0) ? ExportRefTemp1() : nullptr;
							int l0_poc_export[H264_MAX_REF_LIST];
							int n_l0_export = 0;
							bool ok = false;
							bool repeated = false;

							if (sl_type == H264_SLICE_I || sl_type == H264_SLICE_SI || is_idr) {
								if (pps->entropy_coding_mode_flag == 1) {
									ok = H264_DecodeSlice_CABAC_I(rbsp, rbsp_len, sps, pps, nal_type, nal_ref,
																  cur_y, cur_u, cur_v, y_stride, uv_stride, mb_qp_table,
																  db_flags);
								} else {
									ok = H264_DecodeSlice_I(rbsp, rbsp_len, sps, pps, nal_type, nal_ref,
															cur_y, cur_u, cur_v, y_stride, uv_stride);
								}
								frame_type = 0;
							} else if (sl_type == H264_SLICE_P || sl_type == H264_SLICE_SP) {
								// A P slice can activate more than one list0 picture
								// (num_ref_idx_l0_active_minus1 > 0), and the reference index of
								// each partition then selects the picture it predicts from
								// (H.264 8.2.4, 7.4.3.1).  The list has to be built here because
								// only the caller knows the slice's frame_num and reordering.
								H264RefPicDesc l0[H264_MAX_REF_LIST];
								H264RefPicDesc l1_unused[H264_MAX_REF_LIST];
								int cnt0 = 0, cnt1_unused = 0;
								BuildRefLists(cur_poc, (int)sh.frame_num,
											  1 << (sps->log2_max_frame_num_minus4 + 4),
											  sh.reorder_l0, sh.n_reorder_l0, nullptr, 0,
											  l0, &cnt0, (int)((sh.num_ref_idx_l0_active_minus1 + 1) < H264_MAX_REF_LIST ? (sh.num_ref_idx_l0_active_minus1 + 1) : H264_MAX_REF_LIST),
											  l1_unused, &cnt1_unused, (int)((sh.num_ref_idx_l1_active_minus1 + 1) < H264_MAX_REF_LIST ? (sh.num_ref_idx_l1_active_minus1 + 1) : H264_MAX_REF_LIST));
								n_l0_export = cnt0;
								for (int q = 0; q < cnt0 && q < H264_MAX_REF_LIST; ++q) l0_poc_export[q] = l0[q].poc;
								H264RefPicView list0_view[H264_MAX_REF_LIST];
								for (int q = 0; q < cnt0 && q < H264_MAX_REF_LIST; ++q) {
									list0_view[q].y = l0[q].y;
									list0_view[q].u = l0[q].u;
									list0_view[q].v = l0[q].v;
								}
								if (pps->entropy_coding_mode_flag == 1) {
									ok = H264_DecodeSlice_CABAC_P(rbsp, rbsp_len, sps, pps, nal_type, nal_ref,
																  ref_y, ref_u, ref_v, y_stride, uv_stride,
																  (cnt0 > 0) ? list0_view : nullptr, cnt0,
																  cur_y, cur_u, cur_v, y_stride, uv_stride, mb_qp_table,
																  mv_export, ref_export,
																  db_flags, db_mv, db_ref);
								} else {
									ok = H264_DecodeSlice_P(rbsp, rbsp_len, sps, pps, nal_type, nal_ref,
															ref_y, ref_u, ref_v, y_stride, uv_stride,
															cur_y, cur_u, cur_v, y_stride, uv_stride);
								}
								frame_type = 1;
							} else if (is_b_slice) {
								// A B picture is never used as a reference picture, so a B slice
								// that cannot be decoded cannot corrupt the reference chain: the
								// list0 reference picture is repeated instead.
								H264RefPicDesc l0[H264_MAX_REF_LIST], l1[H264_MAX_REF_LIST];
								int cnt0 = 0, cnt1 = 0;
								const int have_lists = BuildRefLists(cur_poc, (int)sh.frame_num, 1 << (sps->log2_max_frame_num_minus4 + 4),
										 sh.reorder_l0, sh.n_reorder_l0, sh.reorder_l1, sh.n_reorder_l1,
										 l0, &cnt0, (int)((sh.num_ref_idx_l0_active_minus1 + 1) < H264_MAX_REF_LIST ? (sh.num_ref_idx_l0_active_minus1 + 1) : H264_MAX_REF_LIST),
																	 l1, &cnt1, (int)((sh.num_ref_idx_l1_active_minus1 + 1) < H264_MAX_REF_LIST ? (sh.num_ref_idx_l1_active_minus1 + 1) : H264_MAX_REF_LIST));
								n_l0_export = cnt0;
								for (int q = 0; q < cnt0 && q < H264_MAX_REF_LIST; ++q) l0_poc_export[q] = l0[q].poc;
								if (have_ref && have_lists && pps->entropy_coding_mode_flag == 1) {
									ok = H264_DecodeSlice_CABAC_B_skip(rbsp, rbsp_len, sps, pps, nal_type, nal_ref,
																	   l0, l1, cnt0, cnt1, cur_poc,
																	   cur_y, cur_u, cur_v, y_stride, uv_stride, mb_qp_table,
																	   db_flags, db_mv, db_ref, mv_export, ref_export,
																	   mv1_export, ref1_export);
								}
								if (!ok) {
									MemCopyN(cur_y, ref_y, pic_y_size);
									MemCopyN(cur_u, ref_u, pic_uv_size);
									MemCopyN(cur_v, ref_v, pic_uv_size);
									ok = true;
									repeated = true;
								}
								frame_type = 2;
							}

							if (ok) {
								if (!repeated && sh.disable_deblocking_filter_idc != 1) {
									H264_DeblockFrame(cur_y, cur_u, cur_v, y_stride, uv_stride,
													  (int)sps->mb_width, (int)sps->mb_height,
													  sh.slice_qp, (int)sh.disable_deblocking_filter_idc,
													  (int)sh.slice_alpha_c_offset_div2 * 2,
													  (int)sh.slice_beta_offset_div2 * 2,
													  mb_qp_table, db_flags, db_mv, db_ref,
													  (int)pps->chroma_qp_index_offset);
								}

								if (!repeated && nal_ref != 0) {
									InsertReferencePicture(cur_poc, (int)sh.frame_num,
														   mv_export, ref_export,
														   mv1_export, ref1_export,
														   l0_poc_export, n_l0_export, is_idr,
														   &sh, 1 << ((int)sps->log2_max_frame_num_minus4 + 4));
								}

								has_reconstructed_frame = true;
							} else if (!is_b_slice) {
								// The slice could not be decoded (unsupported syntax or a broken
								// stream): keep showing the last reference picture instead of
								// dropping the frame, so the host never displays a blank buffer.
								if (have_ref) {
									MemCopyN(cur_y, ref_y, pic_y_size);
									MemCopyN(cur_u, ref_u, pic_uv_size);
									MemCopyN(cur_v, ref_v, pic_uv_size);
								}
								has_reconstructed_frame = true;
							}
						}
					}
				}

				pos = nal_end;
			}
		}

		if (!has_reconstructed_frame) return nullptr;

		// Park the picture that was just reconstructed and release the one that is due next
		// (H.264 C.5.2.2): pictures are shown in increasing PicOrderCntVal, and the reference
		// picture of a B picture is coded after it but displayed before it.
		if (OutputQueuePush(pending_poc, frame_type)) {
			return OutputQueueTake(false, out_w, out_h, out_frame_type);
		}

		EnsureDefaultParameters();
		const H264SPS* sps = &sps_list[active_sps_id >= 0 ? active_sps_id : 0];
			dpb_max_refs = (int)sps->max_num_ref_frames;
		int pic_w = (int)sps->width;
		int pic_h = (int)sps->height;
		if (pic_w <= 0 || pic_h <= 0) return nullptr;

		const size_t pixel_cnt = (size_t)pic_w * (size_t)pic_h;
		uni::Color* pixels = (uni::Color*)malloc(pixel_cnt * sizeof(uni::Color));
		if (!pixels) return nullptr;

		for (int y = 0; y < pic_h; ++y) {
			const uint8* row_y = cur_y + (size_t)y * y_stride;
			const uint8* row_u = cur_u + (size_t)(y / 2) * uv_stride;
			const uint8* row_v = cur_v + (size_t)(y / 2) * uv_stride;
			uni::Color* dst_row = pixels + (size_t)y * pic_w;
			for (int x = 0; x < pic_w; ++x) {
				dst_row[x] = uni::Color::FromYCbCr(row_y[x], row_u[x / 2], row_v[x / 2],
					H264YCbCrMatrix((int)sps->matrix_coefficients),
					sps->video_full_range_flag ? uni::YCbCrRange::Full : uni::YCbCrRange::Limited);
			}
		}

		if (out_w) *out_w = pic_w;
		if (out_h) *out_h = pic_h;
		if (out_frame_type) *out_frame_type = frame_type;

		return pixels;
	}
};

void* H264Decoder_Create(int default_w, int default_h) {
	return new H264DecoderImpl(default_w, default_h);
}

void H264Decoder_Destroy(void* ctx) {
	if (ctx) {
		delete (H264DecoderImpl*)ctx;
	}
}

uni::Color* H264Decoder_DecodeFrame(
	void* ctx,
	const byte* data,
	size_t size,
	int* out_w,
	int* out_h,
	int* out_frame_type
) {
	if (!ctx) return nullptr;
	return ((H264DecoderImpl*)ctx)->DecodeFrame(data, size, out_w, out_h, out_frame_type);
}

} // extern "C"

namespace uni {

	class H264VideoStream : public IVideoStream {
	private:
		StorageTrait* m_storage;
		trait::Malloc* m_allocator;
		VideoInfo m_info;
		H264DecoderImpl* m_decoder;
		uint32 m_curr_frame;
		uint64 m_file_pos;
		uint64 m_file_size;

	public:
		H264VideoStream(StorageTrait* storage, trait::Malloc* allocator)
			: m_storage(storage), m_allocator(allocator),
			  m_decoder(nullptr), m_curr_frame(0), m_file_pos(0), m_file_size(0) {
			m_info = {};
			m_info.containerFormat = VideoContainerFormat::Unknown;
			m_info.videoFormat.codec_type = VideoCodecType::H264;
			m_info.videoFormat.width = 320;
			m_info.videoFormat.height = 240;
			m_info.videoFormat.frame_rate_num = 30;
			m_info.videoFormat.frame_rate_den = 1;
			if (m_storage) {
				// getUnits() counts blocks, not bytes; m_file_pos below is a byte offset.
				m_file_size = m_storage->getUnits() * (m_storage->Block_Size ? m_storage->Block_Size : 1);
			}
			m_decoder = new H264DecoderImpl(320, 240);
		}

		virtual ~H264VideoStream() {
			if (m_decoder) {
				delete m_decoder;
				m_decoder = nullptr;
			}
		}

		virtual void Release() override {
			delete this;
		}

		virtual VideoResult GetInfo(VideoInfo& outInfo) const override {
			outInfo = m_info;
			return VideoResult::OK;
		}

		virtual VideoResult ReadVideoFrame(VideoFrame& outFrame, trait::Malloc& alloc) override {
			if (!m_storage || !m_decoder) {
				return VideoResult::EndOfStream;
			}

			int out_w = 0, out_h = 0, frame_type = 0;
			uni::Color* pixels = nullptr;

			// Pictures leave the decoder in display order, so a NAL unit can be consumed without a
			// picture being due, and pictures are still buffered when the last unit has been read;
			// the loop below keeps feeding units, and the null buffer drains the rest.
			while (!pixels) {
				if (m_file_pos >= m_file_size) {
					pixels = m_decoder->DecodeFrame(nullptr, 0, &out_w, &out_h, &frame_type);
					if (!pixels) return VideoResult::EndOfStream;
					break;
				}

				// Read next NAL unit / AU from Annex-B stream
				size_t chunk_cap = 65536;
				byte* chunk_buf = (byte*)malloc(chunk_cap);
				if (!chunk_buf) return VideoResult::OutOfMemory;

				size_t chunk_len = 0;
				bool found_start = false;

				while (m_file_pos < m_file_size) {
					int b = (*m_storage)[m_file_pos++];
					if (b < 0) break;

					if (chunk_len >= chunk_cap) {
						chunk_cap *= 2;
						byte* nb = (byte*)realloc(chunk_buf, chunk_cap);
						if (!nb) {
							free(chunk_buf);
							return VideoResult::OutOfMemory;
						}
						chunk_buf = nb;
					}
					chunk_buf[chunk_len++] = (byte)b;

					if (chunk_len >= 4 && chunk_buf[chunk_len - 4] == 0x00 && chunk_buf[chunk_len - 3] == 0x00 &&
						chunk_buf[chunk_len - 2] == 0x00 && chunk_buf[chunk_len - 1] == 0x01) {
						if (!found_start) {
							found_start = true;
						} else {
							// Hit next start code, rewind 4 bytes for next frame
							m_file_pos -= 4;
							chunk_len -= 4;
							break;
						}
					}
				}

				if (chunk_len == 0) {
					free(chunk_buf);
					// Nothing decodable is left: let the loop above drain the buffered pictures.
					m_file_pos = m_file_size;
					continue;
				}

				pixels = m_decoder->DecodeFrame(chunk_buf, chunk_len, &out_w, &out_h, &frame_type);
				free(chunk_buf);
			}

			if (!pixels) {
				return VideoResult::Failed;
			}

			m_info.videoFormat.width = (uint32)out_w;
			m_info.videoFormat.height = (uint32)out_h;

			VideoFrameClear(outFrame);
			outFrame.width = (uint32)out_w;
			outFrame.height = (uint32)out_h;
			outFrame.timestampMs = (uint32)(((uint64)m_curr_frame * 1000ULL) / 30);
			outFrame.frameIndex = m_curr_frame;
			outFrame.isKeyFrame = (frame_type == 0);

			outFrame.image.width = (uint32)out_w;
			outFrame.image.height = (uint32)out_h;
			outFrame.image.stride = (uint32)out_w * sizeof(uni::Color);
			outFrame.image.format = PixelFormat::BGRA8888;
			outFrame.image.colorSpace = ColorSpace::SRGB;
			outFrame.image.pixels = pixels;
			outFrame.image.size = (size_t)out_w * (size_t)out_h * sizeof(uni::Color);
			outFrame.image.allocator = nullptr;

			m_curr_frame++;
			return VideoResult::OK;
		}

		virtual VideoResult ReadAudioSamples(void* destBuffer, uint32 maxBytes, uint32& bytesRead) override {
			bytesRead = 0;
			return VideoResult::InvalidArgument;
		}

		virtual VideoResult SeekFrame(uint32 frameIndex) override {
			return VideoResult::Unsupported;
		}

		virtual VideoResult SeekTime(uint32 timestampMs) override {
			return VideoResult::Unsupported;
		}
	};

	const char* H264Codec::GetName() const {
		return "H.264 / AVC Video";
	}

	VideoContainerFormat H264Codec::GetFormat() const {
		return VideoContainerFormat::Unknown;
	}

	const char* const* H264Codec::GetExtensions() const {
		return H264_EXTENSIONS;
	}

	VideoResult H264Codec::Probe(StorageTrait& storage, bool& matched) const {
		matched = false;
		byte buf[64] = { 0 };
		for (size_t i = 0; i < sizeof(buf); ++i) {
			int b = storage[i];
			if (b < 0) return VideoResult::IoError;
			buf[i] = (byte)b;
		}

		// Look for Annex B Start Code (0x000001 or 0x00000001) followed by H.264 NAL header
		for (size_t i = 0; i + 4 < sizeof(buf); ++i) {
			if (buf[i] == 0x00 && buf[i + 1] == 0x00) {
				size_t nal_offset = 0;
				if (buf[i + 2] == 0x01) {
					nal_offset = i + 3;
				} else if (buf[i + 2] == 0x00 && buf[i + 3] == 0x01) {
					nal_offset = i + 4;
				}
				if (nal_offset > 0 && nal_offset < sizeof(buf)) {
					uint8 nal_type = buf[nal_offset] & 0x1F;
					uint8 forbidden_bit = (buf[nal_offset] >> 7) & 1;
					if (forbidden_bit == 0 && (nal_type == H264_NAL_SPS || nal_type == H264_NAL_PPS ||
											   nal_type == H264_NAL_SLICE_IDR || nal_type == H264_NAL_AUD ||
											   nal_type == H264_NAL_SEI)) {
						matched = true;
						return VideoResult::OK;
					}
				}
			}
		}
		return VideoResult::OK;
	}

	VideoResult H264Codec::ReadInfo(StorageTrait& storage, VideoInfo& outInfo) const {
		outInfo = {};
		outInfo.videoFormat.codec_type = VideoCodecType::H264;
		return VideoResult::OK;
	}

	VideoResult H264Codec::OpenStream(
		StorageTrait& storage,
		IVideoStream*& outStream,
		trait::Malloc& allocator
	) const {
		outStream = new H264VideoStream(&storage, &allocator);
		if (!outStream) return VideoResult::OutOfMemory;
		return VideoResult::OK;
	}

} // namespace uni

#endif // _INC_CPP
