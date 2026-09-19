// ASCII C/C++ TAB4 CRLF
// Docutitle: MPEG-4 Part 2 (Visual) Format Definition
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

#ifndef _INC_FORMAT_VIDEO_MPEG4
#define _INC_FORMAT_VIDEO_MPEG4

#include "../../stdinc.h"
#include "../../graphic/color.h"

#if defined(_INC_CPP) || defined(__cplusplus)
extern "C" {
#endif

// MPEG-4 Start Codes (32-bit big-endian with 0x000001 prefix)
#define MPEG4_START_CODE_VO_MIN         0x00000100u
#define MPEG4_START_CODE_VO_MAX         0x0000011Fu
#define MPEG4_START_CODE_VOL_MIN        0x00000120u
#define MPEG4_START_CODE_VOL_MAX        0x0000012Fu
#define MPEG4_START_CODE_VOS            0x000001B0u // Visual Object Sequence
#define MPEG4_START_CODE_VOS_END        0x000001B1u
#define MPEG4_START_CODE_USER_DATA      0x000001B2u
#define MPEG4_START_CODE_GOV            0x000001B3u // Group of VOPs
#define MPEG4_START_CODE_VISUAL_OBJ     0x000001B5u
#define MPEG4_START_CODE_VOP            0x000001B6u // Video Object Plane

// VOP Coding Types
#define MPEG4_VOP_TYPE_I                0u // Intra-coded
#define MPEG4_VOP_TYPE_P                1u // Predictive-coded
#define MPEG4_VOP_TYPE_B                2u // Bidirectionally predictive-coded
#define MPEG4_VOP_TYPE_S                3u // Sprite-coded

// Video Object Layer Shape
#define MPEG4_SHAPE_RECTANGULAR         0u
#define MPEG4_SHAPE_BINARY              1u
#define MPEG4_SHAPE_BINARY_ONLY         2u
#define MPEG4_SHAPE_GRAYSCALE           3u

// Aspect Ratio Types
#define MPEG4_ASPECT_RATIO_FORBIDDEN    0x00u
#define MPEG4_ASPECT_RATIO_1_1          0x01u
#define MPEG4_ASPECT_RATIO_4_3_PAL      0x02u
#define MPEG4_ASPECT_RATIO_4_3_NTSC     0x03u
#define MPEG4_ASPECT_RATIO_16_9_PAL     0x04u
#define MPEG4_ASPECT_RATIO_16_9_NTSC    0x05u
#define MPEG4_ASPECT_RATIO_EXTENDED     0x0Fu

#pragma pack(push, 1)

typedef struct {
	uint32 start_code;                  // 0x00000120..0x0000012F
	uint8  random_accessible_vol;       // 1 bit
	uint8  video_object_type_indication;// 8 bits
	uint8  is_object_layer_identifier;  // 1 bit
	uint8  video_object_layer_verid;    // 4 bits
	uint8  video_object_layer_priority; // 3 bits
	uint8  aspect_ratio_info;           // 4 bits
	uint16 par_width;                   // 8 bits if extended
	uint16 par_height;                  // 8 bits if extended
	uint8  vol_control_parameters;      // 1 bit
	uint8  video_object_layer_shape;    // 2 bits (0 = rectangular)
	uint16 vop_time_increment_resolution;// 16 bits
	uint8  fixed_vop_rate;              // 1 bit
	uint16 fixed_vop_time_increment;    // variable bits
	uint16 video_object_layer_width;    // 13 bits
	uint16 video_object_layer_height;   // 13 bits
	uint8  interlaced;                  // 1 bit
	uint8  obmc_disable;                // 1 bit
	uint8  sprite_enable;               // variable
	uint8  not_8_bit;                   // 1 bit
	uint8  quant_type;                  // 1 bit (0 = H.263, 1 = MPEG)
	uint8  load_intra_quant_mat;        // 1 bit
	uint8  intra_quant_mat[64];
	uint8  load_nonintra_quant_mat;     // 1 bit
	uint8  nonintra_quant_mat[64];
	uint8  quarter_sample;              // 1 bit
	uint8  complexity_estimation_disable;// 1 bit
	uint8  resync_marker_disable;       // 1 bit
	uint8  data_partitioned;            // 1 bit
	uint8  reversible_vlc;              // 1 bit
	uint8  newpred_enable;              // 1 bit
	uint8  reduced_resolution_vop_enable;// 1 bit
	uint8  scalability;                 // 1 bit
} MPEG4VOLHeader;

typedef struct {
	uint32 start_code;                  // 0x000001B6
	uint8  vop_coding_type;             // 2 bits (I=0, P=1, B=2, S=3)
	uint32 modulo_time_base;            // variable 1-bits terminated by 0
	uint32 vop_time_increment;          // variable bits (based on resolution)
	uint8  vop_coded;                   // 1 bit
	uint8  vop_rounding_type;           // 1 bit (for P-VOP)
	uint8  intra_dc_vlc_thr;            // 3 bits
	uint8  vop_quant;                   // 5 bits
	uint8  vop_fcode_forward;           // 3 bits
	uint8  vop_fcode_backward;          // 3 bits
} MPEG4VOPHeader;

#pragma pack(pop)

// C API for MPEG-4 Decoder context
void* MPEG4Decoder_Create(int default_w, int default_h);
void  MPEG4Decoder_Destroy(void* ctx);
uni::Color* MPEG4Decoder_DecodeFrame(
	void* ctx,
	const byte* data,
	size_t size,
	int* out_w,
	int* out_h,
	int* out_vop_type
);

#if defined(_INC_CPP) || defined(__cplusplus)
}
#endif

#if defined(_INC_CPP)

#include "../../../cpp/System/Videosys.hpp"

namespace uni {

	class MPEG4Codec : public IVideoCodec {
	public:
		virtual ~MPEG4Codec() = default;

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

#endif // _INC_FORMAT_VIDEO_MPEG4
