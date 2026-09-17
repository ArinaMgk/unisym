// ASCII C/C++ TAB4 CRLF
// Docutitle: RIFF AVI Format Definition
// Attribute: Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0; Dosconio Mecocoa, BSD 3-Clause License

#ifndef _INC_FORMAT_VIDEO_AVI
#define _INC_FORMAT_VIDEO_AVI

#include "../../stdinc.h"
#include "../audio/WAV.h"
#include "../picture/BMP.h"

#if defined(_INC_CPP) || defined(__cplusplus)
extern "C" {
#endif

#define AVI_FOURCC(a, b, c, d) \
	((uint32)(uint8)(a) | ((uint32)(uint8)(b) << 8) | ((uint32)(uint8)(c) << 16) | ((uint32)(uint8)(d) << 24))

#define AVI_FOURCC_RIFF AVI_FOURCC('R', 'I', 'F', 'F')
#define AVI_FOURCC_AVI  AVI_FOURCC('A', 'V', 'I', ' ')
#define AVI_FOURCC_AVIX AVI_FOURCC('A', 'V', 'I', 'X')
#define AVI_FOURCC_LIST AVI_FOURCC('L', 'I', 'S', 'T')
#define AVI_FOURCC_HDRL AVI_FOURCC('h', 'd', 'r', 'l')
#define AVI_FOURCC_AVIH AVI_FOURCC('a', 'v', 'i', 'h')
#define AVI_FOURCC_STRL AVI_FOURCC('s', 't', 'r', 'l')
#define AVI_FOURCC_STRH AVI_FOURCC('s', 't', 'r', 'h')
#define AVI_FOURCC_STRF AVI_FOURCC('s', 't', 'r', 'f')
#define AVI_FOURCC_STRD AVI_FOURCC('s', 't', 'r', 'd')
#define AVI_FOURCC_STRN AVI_FOURCC('s', 't', 'r', 'n')
#define AVI_FOURCC_MOVI AVI_FOURCC('m', 'o', 'v', 'i')
#define AVI_FOURCC_IDX1 AVI_FOURCC('i', 'd', 'x', '1')
#define AVI_FOURCC_JUNK AVI_FOURCC('J', 'U', 'N', 'K')

#define AVI_STREAM_TYPE_VIDS AVI_FOURCC('v', 'i', 'd', 's')
#define AVI_STREAM_TYPE_AUDS AVI_FOURCC('a', 'u', 'd', 's')
#define AVI_STREAM_TYPE_TXTS AVI_FOURCC('t', 'x', 't', 's')

#define AVI_FOURCC_MJPG AVI_FOURCC('M', 'J', 'P', 'G')
#define AVI_FOURCC_mjpg AVI_FOURCC('m', 'j', 'p', 'g')
#define AVI_FOURCC_JPEG AVI_FOURCC('J', 'P', 'E', 'G')
#define AVI_FOURCC_jpeg AVI_FOURCC('j', 'p', 'e', 'g')
#define AVI_FOURCC_DIB  AVI_FOURCC('D', 'I', 'B', ' ')
#define AVI_FOURCC_RGB  AVI_FOURCC('R', 'G', 'B', ' ')
#define AVI_FOURCC_RAW  AVI_FOURCC('R', 'A', 'W', ' ')
#define AVI_FOURCC_MPG1 AVI_FOURCC('M', 'P', 'G', '1')
#define AVI_FOURCC_MPG2 AVI_FOURCC('M', 'P', 'G', '2')
#define AVI_FOURCC_MP4V AVI_FOURCC('M', 'P', '4', 'V')
#define AVI_FOURCC_H264 AVI_FOURCC('H', '2', '6', '4')
#define AVI_FOURCC_h264 AVI_FOURCC('h', '2', '6', '4')
#define AVI_FOURCC_XVID AVI_FOURCC('X', 'V', 'I', 'D')
#define AVI_FOURCC_xvid AVI_FOURCC('x', 'v', 'i', 'd')
#define AVI_FOURCC_DIVX AVI_FOURCC('D', 'I', 'V', 'X')
#define AVI_FOURCC_divx AVI_FOURCC('d', 'i', 'v', 'x')

#define AVIF_HASINDEX       0x00000010u
#define AVIF_MUSTUSEINDEX   0x00000020u
#define AVIF_ISINTERLEAVED  0x00000100u
#define AVIF_WASCAPTUREFILE 0x00010000u
#define AVIF_COPYRIGHTED    0x00020000u

#define AVIIF_KEYFRAME      0x00000010u
#define AVIIF_NO_TIME       0x00000100u

#pragma pack(push, 1)

typedef struct {
	uint32 riff_tag;  // 'RIFF'
	uint32 riff_size; // file size - 8
	uint32 avi_tag;   // 'AVI ' or 'AVIX'
} AVIRIFFHEADER;

typedef struct {
	uint32 list_tag;  // 'LIST'
	uint32 list_size; // chunk size
	uint32 list_type; // 'hdrl', 'strl', 'movi', etc.
} AVILISTHEADER;

typedef struct {
	uint32 chunk_tag;  // e.g. 'avih', 'strh', '00dc', etc.
	uint32 chunk_size; // chunk data size (excluding 8-byte header)
} AVICHUNKHEADER;

typedef struct {
	uint32 dwMicroSecPerFrame;    // frame display rate (in microseconds)
	uint32 dwMaxBytesPerSec;      // max transfer rate
	uint32 dwPaddingGranularity;  // pad to multiples of this size
	uint32 dwFlags;               // AVIF_* flags
	uint32 dwTotalFrames;         // total video frames in file
	uint32 dwInitialFrames;
	uint32 dwStreams;             // number of streams (video + audio)
	uint32 dwSuggestedBufferSize;
	uint32 dwWidth;               // video width in pixels
	uint32 dwHeight;              // video height in pixels
	uint32 dwReserved[4];
} AVIMAINHEADER;

typedef struct {
	uint32 fccType;               // 'vids', 'auds', etc.
	uint32 fccHandler;            // 'MJPG', etc.
	uint32 dwFlags;
	uint16 wPriority;
	uint16 wLanguage;
	uint32 dwInitialFrames;
	uint32 dwScale;
	uint32 dwRate;                // frame rate / sample rate = dwRate / dwScale
	uint32 dwStart;
	uint32 dwLength;              // total samples or frames in stream
	uint32 dwSuggestedBufferSize;
	uint32 dwQuality;
	uint32 dwSampleSize;          // 0 for variable size (compressed video)
	struct {
		int16 left;
		int16 top;
		int16 right;
		int16 bottom;
	} rcFrame;
} AVISTREAMHEADER;

typedef struct {
	uint32 ckid;                  // chunk ID (e.g. '00dc', '01wb')
	uint32 dwFlags;               // AVIIF_KEYFRAME (0x10), etc.
	uint32 dwChunkOffset;         // position of chunk in file or relative to 'movi'
	uint32 dwChunkLength;         // chunk data length
} AVIINDEXENTRY;

#pragma pack(pop)

#if defined(_INC_CPP) || defined(__cplusplus)
}
#endif

#if defined(_INC_CPP)

#include "../../../cpp/System/Videosys.hpp"

namespace uni {

	class AVICodec : public IVideoCodec {
	public:
		virtual ~AVICodec() = default;

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

#endif // _INC_FORMAT_VIDEO_AVI
