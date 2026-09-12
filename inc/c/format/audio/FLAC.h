// ASCII C/C++ TAB4 CRLF
// Docutitle: Free Lossless Audio Codec (FLAC) Format Definition
// Attribute: Env-Freestanding Non-Dependence
// Copyright: UNISYM

#ifndef _INC_FORMAT_AUDIO_FLAC
#define _INC_FORMAT_AUDIO_FLAC

#include "../../stdinc.h"

#if defined(_INC_CPP)
extern "C" {
#endif

#pragma pack(push, 1)

#define FLAC_MAGIC 0x43614C66u // 'fLaC' in little-endian

typedef struct {
	uint16 min_blocksize;
	uint16 max_blocksize;
	uint32 min_framesize; // 24-bit in spec
	uint32 max_framesize; // 24-bit in spec
	uint32 sample_rate;   // 20-bit
	uint8  channels;      // 3-bit (1..8)
	uint8  bits_per_sample;// 5-bit (4..32)
	uint64 total_samples; // 36-bit
	uint8  md5[16];
} FLAC_STREAMINFO;

#pragma pack(pop)

#if defined(_INC_CPP)
}
#endif

#if defined(_INC_CPP)
#include "../../../cpp/System/Audiosys.hpp"

namespace uni {
	class FLACCodec : public IAudioCodec {
	public:
		virtual ~FLACCodec() = default;

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

#endif // _INC_FORMAT_AUDIO_FLAC
