// ASCII C/C++ TAB4 CRLF
// Docutitle: MPEG Audio Layer III (MP3) Format Definition
// Attribute: Env-Freestanding Non-Dependence
// Copyright: UNISYM

#ifndef _INC_FORMAT_AUDIO_MP3
#define _INC_FORMAT_AUDIO_MP3

#include "../../stdinc.h"

#if defined(_INC_CPP)
extern "C" {
#endif

#pragma pack(push, 1)

// ID3v2 Tag Header (10 bytes)
typedef struct {
	uint8  tag[3];       // 'ID3'
	uint8  version_major;// e.g. 3 for ID3v2.3.0
	uint8  version_minor;// e.g. 0
	uint8  flags;        // Flags (e.g. unsynchronization, extended header)
	uint8  size[4];      // Synchsafe integer (7 bits per byte, MSB 0)
} ID3V2HEADER;

// ID3v1 Tag (128 bytes at the end of file)
typedef struct {
	char   tag[3];       // 'TAG'
	char   title[30];
	char   artist[30];
	char   album[30];
	char   year[4];
	char   comment[30];
	uint8  genre;
} ID3V1TAG;

#pragma pack(pop)

// Decode Synchsafe integer (ID3v2 size: 4 bytes, 7 bits each = 28 bits total)
static inline uint32 ID3v2_DecodeSynchsafe(const uint8 size[4]) {
	return ((uint32)(size[0] & 0x7F) << 21) |
	       ((uint32)(size[1] & 0x7F) << 14) |
	       ((uint32)(size[2] & 0x7F) << 7)  |
	       ((uint32)(size[3] & 0x7F));
}

// Check if buffer starts with ID3v2 header and return total tag size (including 10-byte header)
static inline uint32 ID3v2_GetTagSize(const void* data, uint32 len) {
	if (!data || len < sizeof(ID3V2HEADER)) return 0;
	const ID3V2HEADER* hdr = (const ID3V2HEADER*)data;
	if (hdr->tag[0] == 'I' && hdr->tag[1] == 'D' && hdr->tag[2] == '3') {
		uint32 body_size = ID3v2_DecodeSynchsafe(hdr->size);
		uint32 total_size = (uint32)sizeof(ID3V2HEADER) + body_size;
		if (hdr->flags & 0x10) { // Footer present (ID3v2.4)
			total_size += 10;
		}
		return total_size;
	}
	return 0;
}

#if defined(_INC_CPP)
}
#endif

#if defined(_INC_CPP)
#include "../../../cpp/System/Audiosys.hpp"

namespace uni {
	class MP3Codec : public IAudioCodec {
	public:
		virtual ~MP3Codec() = default;

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

#endif // _INC_FORMAT_AUDIO_MP3
