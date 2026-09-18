// ASCII C/C++ TAB4 CRLF
// Docutitle: MPEG-1 / MPEG-2 Format Definition
// Attribute: Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0; Dosconio Mecocoa, BSD 3-Clause License

#ifndef _INC_FORMAT_VIDEO_MPEG
#define _INC_FORMAT_VIDEO_MPEG

#include "../../stdinc.h"
#include "../audio/MP3.h"

#if defined(_INC_CPP) || defined(__cplusplus)
extern "C" {
#endif

// MPEG Start Codes (32-bit big-endian with 0x000001 prefix)
#define MPEG_PICTURE_START_CODE       0x00000100u
#define MPEG_SLICE_START_CODE_MIN     0x00000101u
#define MPEG_SLICE_START_CODE_MAX     0x000001AFu
#define MPEG_USER_DATA_START_CODE     0x000001B2u
#define MPEG_SEQUENCE_HEADER_CODE     0x000001B3u
#define MPEG_SEQUENCE_ERROR_CODE      0x000001B4u
#define MPEG_EXTENSION_START_CODE     0x000001B5u
#define MPEG_SEQUENCE_END_CODE        0x000001B7u
#define MPEG_GROUP_START_CODE         0x000001B8u
#define MPEG_PROGRAM_END_CODE         0x000001B9u
#define MPEG_PACK_START_CODE          0x000001BAu
#define MPEG_SYSTEM_HEADER_START_CODE 0x000001BBu
#define MPEG_PES_PROGRAM_STREAM_MAP   0x000001BCu
#define MPEG_PES_PRIVATE_STREAM_1     0x000001BDu
#define MPEG_PES_PADDING_STREAM       0x000001BEu
#define MPEG_PES_PRIVATE_STREAM_2     0x000001BFu
#define MPEG_PES_AUDIO_STREAM_MIN     0x000001C0u
#define MPEG_PES_AUDIO_STREAM_MAX     0x000001DFu
#define MPEG_PES_VIDEO_STREAM_MIN     0x000001E0u
#define MPEG_PES_VIDEO_STREAM_MAX     0x000001EFu

// Picture Coding Types
#define MPEG_PICTURE_TYPE_I           1u
#define MPEG_PICTURE_TYPE_P           2u
#define MPEG_PICTURE_TYPE_B           3u
#define MPEG_PICTURE_TYPE_D           4u

// Macroblock Modes
#define MPEG_MB_INTRA                 0x01u
#define MPEG_MB_PATTERN               0x02u
#define MPEG_MB_BACKWARD              0x04u
#define MPEG_MB_FORWARD               0x08u
#define MPEG_MB_QUANT                 0x10u

#pragma pack(push, 1)

typedef struct {
	uint32 start_code;             // 0x000001B3
	uint16 horizontal_size_value;  // 12 bits
	uint16 vertical_size_value;    // 12 bits
	uint8  aspect_ratio_information;// 4 bits
	uint8  frame_rate_code;        // 4 bits
	uint32 bit_rate_value;         // 18 bits
	uint16 vbv_buffer_size_value;  // 10 bits
	uint8  constrained_parameters_flag;
	uint8  load_intra_quantizer_matrix;
	uint8  intra_quantizer_matrix[64];
	uint8  load_non_intra_quantizer_matrix;
	uint8  non_intra_quantizer_matrix[64];
} MPEGSequenceHeader;

typedef struct {
	uint32 start_code;             // 0x000001B8
	uint32 time_code;              // 25 bits: drop_frame(1), hours(5), mins(6), marker(1), secs(6), pictures(6)
	uint8  closed_gop;             // 1 bit
	uint8  broken_link;            // 1 bit
} MPEGGOPHeader;

typedef struct {
	uint32 start_code;             // 0x00000100
	uint16 temporal_reference;     // 10 bits
	uint8  picture_coding_type;    // 3 bits (I=1, P=2, B=3)
	uint16 vbv_delay;              // 16 bits
	uint8  full_pel_forward_vector;// 1 bit
	uint8  forward_f_code;         // 3 bits
	uint8  full_pel_backward_vector;// 1 bit
	uint8  backward_f_code;        // 3 bits
} MPEGPictureHeader;

#pragma pack(pop)

#if defined(_INC_CPP) || defined(__cplusplus)
}
#endif

#if defined(_INC_CPP)

#include "../../../cpp/System/Videosys.hpp"

namespace uni {

	class MPEGCodec : public IVideoCodec {
	public:
		virtual ~MPEGCodec() = default;

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

#endif // _INC_FORMAT_VIDEO_MPEG
