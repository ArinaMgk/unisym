// ASCII C/C++ TAB4 CRLF
// Docutitle: Ogg Vorbis Audio Format Definition
// Attribute: Env-Freestanding Non-Dependence
// Copyright: UNISYM

#ifndef _INC_FORMAT_AUDIO_OGG
#define _INC_FORMAT_AUDIO_OGG

#include "../../stdinc.h"

#if defined(_INC_CPP)
extern "C" {
#endif

#pragma pack(push, 1)

#define OGG_MAGIC 0x5367674Fu // 'OggS' in little-endian

typedef struct {
	uint32 capture_pattern; // 'OggS'
	uint8  version;         // 0
	uint8  header_type;     // 0x01: continuation, 0x02: BOS, 0x04: EOS
	uint64 granule_position;// Audio sample position
	uint32 bitstream_serial;// Serial number
	uint32 page_seq_no;     // Page sequence number
	uint32 checksum;        // CRC32 checksum
	uint8  page_segments;   // Segment count (0..255)
} OGG_PAGE_HEADER;

typedef struct {
	uint32 vorbis_version;
	uint8  audio_channels;
	uint32 audio_sample_rate;
	int32  bitrate_maximum;
	int32  bitrate_nominal;
	int32  bitrate_minimum;
	uint8  blocksize_0;     // Log2 size of short block
	uint8  blocksize_1;     // Log2 size of long block
	uint8  framing_flag;
} VORBIS_INFO;

#pragma pack(pop)

#if defined(_INC_CPP)
}
#endif

#if defined(_INC_CPP)
#include "../../../cpp/System/Audiosys.hpp"

namespace uni {
	class OGGCodec : public IAudioCodec {
	public:
		virtual ~OGGCodec() = default;

		virtual const char* GetName() const override;
		virtual AudioContainerFormat GetFormat() const override;
		virtual const char* const* GetExtensions() const override;

		virtual AudioResult Probe(StorageTrait& storage, bool& matched) const override;
		virtual AudioResult ReadInfo(StorageTrait& storage, AudioInfo& outInfo) const override;
		virtual AudioResult OpenStream(
			StorageTrait& storage,
			IAudioStream*& outStream,
			trait::Malloc& allocator
		) const override;
	};
}
#endif

#endif // _INC_FORMAT_AUDIO_OGG
