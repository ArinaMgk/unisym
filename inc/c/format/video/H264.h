// ASCII C/C++ TAB4 CRLF
// Docutitle: [Format.Video] Advanced Video Coding (H.264 / AVC / MPEG-4 Part 10)
// Attribute: Env-Freestanding Non-Dependence
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

#ifndef _INC_FORMAT_VIDEO_H264
#define _INC_FORMAT_VIDEO_H264

#include "../../stdinc.h"
#include "../../graphic/color.h"

#if defined(_INC_CPP) || defined(__cplusplus)
extern "C" {
#endif

// H.264 NAL Unit Types (nal_unit_type: 5 bits)
#define H264_NAL_UNSPECIFIED       0u
#define H264_NAL_SLICE_NON_IDR     1u // Coded slice of a non-IDR picture
#define H264_NAL_SLICE_DPA         2u // Coded slice data partition A
#define H264_NAL_SLICE_DPB         3u // Coded slice data partition B
#define H264_NAL_SLICE_DPC         4u // Coded slice data partition C
#define H264_NAL_SLICE_IDR         5u // Coded slice of an IDR picture
#define H264_NAL_SEI               6u // Supplemental enhancement information
#define H264_NAL_SPS               7u // Sequence parameter set
#define H264_NAL_PPS               8u // Picture parameter set
#define H264_NAL_AUD               9u // Access unit delimiter
#define H264_NAL_END_OF_SEQ        10u// End of sequence
#define H264_NAL_END_OF_STREAM     11u// End of stream
#define H264_NAL_FILLER            12u// Filler data
#define H264_NAL_SPS_EXT           13u// Sequence parameter set extension
#define H264_NAL_PREFIX            14u// Prefix NAL unit
#define H264_NAL_SUBSET_SPS        15u// Subset sequence parameter set

// H.264 Slice Types (slice_type % 5)
#define H264_SLICE_P               0u // Predictive slice
#define H264_SLICE_B               1u // Bi-predictive slice
#define H264_SLICE_I               2u // Intra slice
#define H264_SLICE_SP              3u // Switching P slice
#define H264_SLICE_SI              4u // Switching I slice

// H.264 Profiles
#define H264_PROFILE_BASELINE      66u
#define H264_PROFILE_MAIN          77u
#define H264_PROFILE_EXTENDED      88u
#define H264_PROFILE_HIGH          100u
#define H264_PROFILE_HIGH_10       110u
#define H264_PROFILE_HIGH_422      122u
#define H264_PROFILE_HIGH_444      244u

#pragma pack(push, 1)

typedef struct {
	uint8  forbidden_zero_bit; // 1 bit (must be 0)
	uint8  nal_ref_idc;        // 2 bits (0 = not reference, >0 = reference)
	uint8  nal_unit_type;      // 5 bits (H264_NAL_*)
} H264NALHeader;

typedef struct {
	uint8  profile_idc;
	uint8  constraint_set0_flag;
	uint8  constraint_set1_flag;
	uint8  constraint_set2_flag;
	uint8  constraint_set3_flag;
	uint8  constraint_set4_flag;
	uint8  constraint_set5_flag;
	uint8  level_idc;
	uint8  seq_parameter_set_id;

	// High profile extensions
	uint8  chroma_format_idc;
	uint8  separate_colour_plane_flag;
	uint8  bit_depth_luma_minus8;
	uint8  bit_depth_chroma_minus8;
	uint8  qpprime_y_zero_transform_bypass_flag;
	uint8  seq_scaling_matrix_present_flag;
	uint8  seq_scaling_list_present_flag[8];
	uint8  scaling_list_4x4[6][16];
	uint8  scaling_list_8x8[2][64];

	uint8  log2_max_frame_num_minus4;
	uint8  pic_order_cnt_type;
	uint8  log2_max_pic_order_cnt_lsb_minus4;
	uint8  delta_pic_order_always_zero_flag;
	int32  offset_for_non_ref_pic;
	int32  offset_for_top_to_bottom_field;
	uint8  num_ref_frames_in_pic_order_cnt_cycle;
	int32  offset_for_ref_frame[256];

	uint8  max_num_ref_frames;
	uint8  gaps_in_frame_num_value_allowed_flag;
	uint32 pic_width_in_mbs_minus1;
	uint32 pic_height_in_map_units_minus1;
	uint8  frame_mbs_only_flag;
	uint8  mb_adaptive_frame_field_flag;
	uint8  direct_8x8_inference_flag;

	// Frame Cropping
	uint8  frame_cropping_flag;
	uint32 frame_crop_left_offset;
	uint32 frame_crop_right_offset;
	uint32 frame_crop_top_offset;
	uint32 frame_crop_bottom_offset;

	// VUI Parameters
	uint8  vui_parameters_present_flag;
	uint8  aspect_ratio_info_present_flag;
	uint8  aspect_ratio_idc;
	uint16 sar_width;
	uint16 sar_height;
	uint8  timing_info_present_flag;
	uint32 num_units_in_tick;
	uint32 time_scale;
	uint8  fixed_frame_rate_flag;
	// video_signal_type() (H.264 E.1.1). The colour matrix and the sample range decide how the
	// decoded YCbCr planes become RGB; a decoder that hardcodes BT.601 shifts every pixel of a
	// BT.709 stream.  matrix_coefficients 1 is BT.709, 5 and 6 are BT.601, 9 is BT.2020 NCL.
	uint8  video_signal_type_present_flag;
	uint8  video_full_range_flag;
	uint8  colour_description_present_flag;
	uint8  matrix_coefficients;

	// Calculated helpers
	uint32 width;
	uint32 height;
	uint32 mb_width;
	uint32 mb_height;
	uint32 fps_num;
	uint32 fps_den;
	bool   is_valid;
} H264SPS;

typedef struct {
	uint8  pic_parameter_set_id;
	uint8  seq_parameter_set_id;
	uint8  entropy_coding_mode_flag; // 0 = CAVLC, 1 = CABAC
	uint8  bottom_field_pic_order_in_frame_present_flag;
	uint8  num_slice_groups_minus1;
	uint8  slice_group_map_type;

	uint8  num_ref_idx_l0_default_active_minus1;
	uint8  num_ref_idx_l1_default_active_minus1;
	uint8  weighted_pred_flag;
	uint8  weighted_bipred_idc;
	int8   pic_init_qp_minus26;
	int8   pic_init_qs_minus26;
	int8   chroma_qp_index_offset;

	uint8  deblocking_filter_control_present_flag;
	uint8  constrained_intra_pred_flag;
	uint8  redundant_pic_cnt_present_flag;
	uint8  transform_8x8_mode_flag;
	uint8  pic_scaling_matrix_present_flag;
	int8   second_chroma_qp_index_offset;

	bool   is_valid;
} H264PPS;

#pragma pack(pop)

// Phase 1 Parser C APIs

// Remove emulation prevention bytes (0x000003) from NAL packet into clean RBSP buffer
size_t H264_UnescapeRBSP(const byte* src, size_t src_len, byte* dst, size_t dst_len);

// Parse Sequence Parameter Set (SPS) from unescaped RBSP data
bool H264_ParseSPS(const byte* rbsp_data, size_t rbsp_len, H264SPS* out_sps);

// Parse Picture Parameter Set (PPS) from unescaped RBSP data
bool H264_ParsePPS(const byte* rbsp_data, size_t rbsp_len, const H264SPS* sps_list, H264PPS* out_pps);

// Phase 2 CAVLC Entropy Decoder, Dequantization & Transforms

typedef struct {
	const byte* data;
	size_t      size;
	size_t      byte_pos;
	uint64      bit_buf;
	int         bits_left;
} H264BitReader;

void H264_InitBitReader(H264BitReader* reader, const byte* data, size_t size);
uint32 H264_PeekBits(H264BitReader* reader, int n);
void H264_SkipBits(H264BitReader* reader, int n);
uint32 H264_ReadBits(H264BitReader* reader, int n);
bool H264_ReadBool(H264BitReader* reader);
uint32 H264_ReadUE(H264BitReader* reader);
int32 H264_ReadSE(H264BitReader* reader);
uint32 H264_ReadTE(H264BitReader* reader, uint32 max_val);
void H264_ByteAlign(H264BitReader* reader);
bool H264_HasData(const H264BitReader* reader);
int H264_GetBitsLeft(const H264BitReader* reader);

// Decode CAVLC residual block (4x4 or 2x2 chroma DC)
// nC: context parameter (-1 for Chroma DC, 0..16+ for normal blocks)
// max_num_coeff: 16 (for 4x4 luma/chroma full), 15 (for 4x4 AC), 4 (for 2x2 chroma DC)
// out_coeffs: output array of max_num_coeff coefficients in zigzag / raster order
// out_total_coeff: output number of non-zero coefficients decoded
// Returns true on success, false on bitstream error
bool H264_DecodeCAVLC_Block(
	H264BitReader* reader,
	int32 nC,
	int32 max_num_coeff,
	int16* out_coeffs,
	int32* out_total_coeff
);

// 4x4 Core Inverse Integer Transform
void H264_IDCT4x4(const int32 in_scaled[16], int32 out_residual[16]);

// 4x4 Luma DC Inverse Hadamard Transform and Scaling
void H264_Hadamard4x4(const int16 in_dc[16], int32 qp, int32 out_scaled_dc[16]);

// 2x2 Chroma DC Inverse Hadamard Transform and Scaling
void H264_Hadamard2x2(const int16 in_dc[4], int32 qp, int32 out_scaled_dc[4]);

// Dequantize a 4x4 block
void H264_Dequant4x4(const int16 in_coeffs[16], int32 qp, bool is_dc_present, int32 out_scaled[16]);

// Chroma QP table lookup
uint8 H264_GetChromaQP(int32 qp_luma, int8 chroma_qp_index_offset);

// Phase 3: Intra Macroblock Prediction & I-Slice Decoding

typedef struct {
	uint32 first_mb_in_slice;
	uint8  slice_type;
	uint8  pic_parameter_set_id;
	uint32 frame_num;
	uint8  field_pic_flag;
	uint8  bottom_field_flag;
	uint16 idr_pic_id;
	uint32 pic_order_cnt_lsb;
	int32  delta_pic_order_cnt_bottom;
	int32  delta_pic_order_cnt[2];
	uint8  redundant_pic_cnt;
	uint8  direct_spatial_mv_pred_flag;
	uint8  num_ref_idx_active_override_flag;
	uint8  num_ref_idx_l0_active_minus1;
	uint8  num_ref_idx_l1_active_minus1;
	// ref_pic_list_reordering() (H.264 7.3.3.1). Each entry is one command packed as
	// (reordering_of_pic_nums_idc << 16) | argument, where the argument is
	// abs_diff_pic_num_minus1 for idc 0/1 and long_term_pic_num for idc 2. The commands must be
	// applied to the initialised reference picture lists (8.2.4.2.2/8.2.4.2.3) - recording them
	// is not optional, because encoders emit them for most B pictures.
	uint32 reorder_l0[32];
	uint32 reorder_l1[32];
	uint8  n_reorder_l0;
	uint8  n_reorder_l1;
	// dec_ref_pic_marking() (H.264 7.3.3.3), packed like the reordering commands above with
	// memory_management_control_operation in the high half. Adaptive marking replaces the sliding
	// window of 8.2.5.3 completely (8.2.5.4), so the operations have to be applied in order before
	// the picture is stored. Recording them is not optional: a stream that keeps only a few of its
	// references alive is otherwise forced through the sliding window, which drops the oldest entry
	// of the decoded picture buffer (measured on wind.mp4, poc 60: the IDR was evicted, so
	// ref_idx_l0 == 2 resolved to a different picture than the reference decoder selected).
	uint32 mmco[16];
	uint8  n_mmco;
	uint8  adaptive_ref_pic_marking;
	// pred_weight_table() (H.264 7.3.3.2). The table is present when weighted_pred_flag is set for
	// a P/SP slice, or weighted_bipred_idc is 1 for a B slice. The arrays hold the *effective*
	// weights: a cleared flag means "default weight", i.e. w = 1 << log2_weight_denom and o = 0, and
	// that is what is stored, so the reconstruction can apply them unconditionally.
	uint8  weighted_pred_present;
	uint8  luma_log2_weight_denom;
	uint8  chroma_log2_weight_denom;
	int16  luma_weight_l0[32];
	int16  luma_offset_l0[32];
	int16  chroma_weight_l0[32][2];
	int16  chroma_offset_l0[32][2];
	int16  luma_weight_l1[32];
	int16  luma_offset_l1[32];
	int16  chroma_weight_l1[32][2];
	int16  chroma_offset_l1[32][2];
	uint8  cabac_init_idc;
	int32  slice_qp_delta;
	uint8  disable_deblocking_filter_idc;
	int8   slice_alpha_c_offset_div2;
	int8   slice_beta_offset_div2;
	int32  slice_qp;
	uint8  sp_for_switch_flag;
	int32  slice_qs_delta;
} H264SliceHeader;

bool H264_ParseSliceHeader(
	H264BitReader* reader,
	const H264SPS* sps,
	const H264PPS* pps,
	uint8 nal_unit_type,
	uint8 nal_ref_idc,
	H264SliceHeader* out_sh
);

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
);

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
);

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
);

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
);

// Phase 4: Inter Prediction, Motion Compensation & P-Slice Decoding

typedef struct {
	int16 x; // 1/4-pel units
	int16 y; // 1/4-pel units
} H264MotionVector;

// 1/4-pel Luma Motion Compensation (6-tap Wiener filter + bilinear)
void H264_MC_Luma(
	uint8* dst, int dst_stride,
	const uint8* ref, int ref_stride,
	int width, int height,
	int src_x, int src_y,
	int mv_x, int mv_y,
	int pic_width, int pic_height
);

// 1/8-pel Chroma Motion Compensation (bilinear interpolation)
void H264_MC_Chroma(
	uint8* dst, int dst_stride,
	const uint8* ref, int ref_stride,
	int width, int height,
	int src_x, int src_y,
	int mv_x, int mv_y,
	int pic_width, int pic_height,
	// 1 selects Cb and 2 selects Cr: weighted prediction keeps a separate weight and offset per
	// chroma component (H.264 8.4.2.2.2), so the plane has to be named.  0 applies nothing.
	int wp_plane = 0
);

// Decode a P-Slice into YUV420 buffers using reference frame
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
);

// Phase 5: In-Loop Deblocking Filter and Video Decoder C APIs

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
	// Per-macroblock data used to derive the boundary strengths of H.264 8.7.2.1: 4 bytes per
	// macroblock ([0] intra, [1] transform_size_8x8_flag, [2] luma coded block pattern), two lists
	// x 16 4x4 blocks of motion vectors, and two lists x 16 4x4 blocks of reference indices (-1 for
	// "list not used").  All NULL keeps the pre-8.7.2.1 behaviour of filtering every edge.
	const uint8* db_flags,
	const int16* db_mv,
	const int16* db_ref,
	int chroma_qp_offset
);

void* H264Decoder_Create(int default_w, int default_h);
void  H264Decoder_Destroy(void* ctx);
// Decode one access unit.  Pictures come out in display order (increasing PicOrderCntVal), so a
// call can return NULL while the picture it decoded is still held back by the reorder buffer;
// call it once with data = NULL and size = 0 after the last access unit to drain the rest.
uni::Color* H264Decoder_DecodeFrame(
	void* ctx,
	const byte* data,
	size_t size,
	int* out_w,
	int* out_h,
	int* out_frame_type
);

#if defined(_INC_CPP) || defined(__cplusplus)
}
#endif

#if defined(_INC_CPP)

#include "../../../cpp/System/Videosys.hpp"

namespace uni {

	class H264Codec : public IVideoCodec {
	public:
		virtual ~H264Codec() = default;

		virtual const char* GetName() const override;
		virtual VideoContainerFormat GetFormat() const override;
		virtual const char* const* GetExtensions() const override;

		virtual VideoResult Probe(StorageTrait& storage, bool& matched) const override;
		virtual VideoResult ReadInfo(StorageTrait& storage, VideoInfo& outInfo) const override;
		virtual VideoResult OpenStream(
			StorageTrait& storage,
			IVideoStream*& outStream,
			trait::Malloc& allocator
		) const override;
	};

} // namespace uni

#endif // _INC_CPP

#endif // _INC_FORMAT_VIDEO_H264
