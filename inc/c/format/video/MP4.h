// ASCII C/C++ TAB4 CRLF
// Docutitle: [Format.Video.Container] ISO Base Media File Format (ISOBMFF / MP4 / MOV) Definition
// Attribute: Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0; Dosconio Mecocoa, BSD 3-Clause License

#ifndef _INC_FORMAT_VIDEO_MP4
#define _INC_FORMAT_VIDEO_MP4

#include "../../stdinc.h"
#include "../../graphic/color.h"

#if defined(_INC_CPP) || defined(__cplusplus)
extern "C" {
#endif

#define ISOBMFF_FOURCC(a, b, c, d) \
	((uint32)(uint8)(d) | ((uint32)(uint8)(c) << 8) | ((uint32)(uint8)(b) << 16) | ((uint32)(uint8)(a) << 24))

// Major Box Types (32-bit big-endian)
#define MP4_BOX_FTYP ISOBMFF_FOURCC('f', 't', 'y', 'p')
#define MP4_BOX_MOOV ISOBMFF_FOURCC('m', 'o', 'o', 'v')
#define MP4_BOX_MVHD ISOBMFF_FOURCC('m', 'v', 'h', 'd')
#define MP4_BOX_TRAK ISOBMFF_FOURCC('t', 'r', 'a', 'k')
#define MP4_BOX_TKHD ISOBMFF_FOURCC('t', 'k', 'h', 'd')
#define MP4_BOX_MDIA ISOBMFF_FOURCC('m', 'd', 'i', 'a')
#define MP4_BOX_MDHD ISOBMFF_FOURCC('m', 'd', 'h', 'd')
#define MP4_BOX_HDLR ISOBMFF_FOURCC('h', 'd', 'l', 'r')
#define MP4_BOX_MINF ISOBMFF_FOURCC('m', 'i', 'n', 'f')
#define MP4_BOX_VMHD ISOBMFF_FOURCC('v', 'm', 'h', 'd')
#define MP4_BOX_SMHD ISOBMFF_FOURCC('s', 'm', 'h', 'd')
#define MP4_BOX_DINF ISOBMFF_FOURCC('d', 'i', 'n', 'f')
#define MP4_BOX_STBL ISOBMFF_FOURCC('s', 't', 'b', 'l')
#define MP4_BOX_STSD ISOBMFF_FOURCC('s', 't', 's', 'd')
#define MP4_BOX_STTS ISOBMFF_FOURCC('s', 't', 't', 's')
#define MP4_BOX_STSS ISOBMFF_FOURCC('s', 't', 's', 's')
#define MP4_BOX_STSC ISOBMFF_FOURCC('s', 't', 's', 'c')
#define MP4_BOX_STSZ ISOBMFF_FOURCC('s', 't', 's', 'z')
#define MP4_BOX_STCO ISOBMFF_FOURCC('s', 't', 'c', 'o')
#define MP4_BOX_CO64 ISOBMFF_FOURCC('c', 'o', '6', '4')
#define MP4_BOX_MDAT ISOBMFF_FOURCC('m', 'd', 'a', 't')
#define MP4_BOX_FREE ISOBMFF_FOURCC('f', 'r', 'e', 'e')
#define MP4_BOX_SKIP ISOBMFF_FOURCC('s', 'k', 'i', 'p')
#define MP4_BOX_WIDE ISOBMFF_FOURCC('w', 'i', 'd', 'e')

// Handler Types
#define MP4_HDLR_VIDE ISOBMFF_FOURCC('v', 'i', 'd', 'e')
#define MP4_HDLR_SOUN ISOBMFF_FOURCC('s', 'o', 'u', 'n')
#define MP4_HDLR_HINT ISOBMFF_FOURCC('h', 'i', 'n', 't')

// Codec / Sample Entry FourCCs
#define MP4_CODEC_AVC1 ISOBMFF_FOURCC('a', 'v', 'c', '1')
#define MP4_CODEC_AVC3 ISOBMFF_FOURCC('a', 'v', 'c', '3')
#define MP4_CODEC_AVCC ISOBMFF_FOURCC('a', 'v', 'c', 'C')
#define MP4_CODEC_MP4V ISOBMFF_FOURCC('m', 'p', '4', 'v')
#define MP4_CODEC_ESDS ISOBMFF_FOURCC('e', 's', 'd', 's')
#define MP4_CODEC_MP4A ISOBMFF_FOURCC('m', 'p', '4', 'a')

#pragma pack(push, 1)

typedef struct {
	uint32 size; // 32-bit big endian (if 1, 64-bit size follows)
	uint32 type; // FourCC
} MP4BoxHeader;

#pragma pack(pop)

#if defined(_INC_CPP) || defined(__cplusplus)
}
#endif

#if defined(_INC_CPP)

#include "../../../cpp/System/Videosys.hpp"

namespace uni {

	class MP4Codec : public IVideoCodec {
	public:
		virtual ~MP4Codec() = default;

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

#endif // _INC_FORMAT_VIDEO_MP4
