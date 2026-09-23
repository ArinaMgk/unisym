// ASCII C/C++ TAB4 CRLF
// Docutitle: [Filefmt.Audio] MPEG-4 Advanced Audio Coding (AAC-LC) Format Definition
// Attribute: Env-Freestanding Non-Dependence
// Copyright: UNISYM
//
// License: Apache-2.0 (see LICENSE); copyright only. AAC is covered by third-party patents -- commercial use needs its own patent consideration.

#ifndef _INC_FORMAT_AUDIO_AAC
#define _INC_FORMAT_AUDIO_AAC

#include "../../stdinc.h"

#if defined(_INC_CPP)
#include "../../../cpp/System/Audiosys.hpp"

namespace uni {

	// One raw AAC access unit exactly as a container recorded it.  Unlike ADTS, raw AAC frames
	// carry no length or sync word, so a stream can only be walked through the sample table of
	// whatever container holds it: MP4 supplies the one it parsed from stbl.
	struct AACRawFrame {
		uint64 offset;
		uint32 size;
	};

	IAudioStream* AAC_OpenRawStream(
		StorageTrait& storage,
		const AACRawFrame* frames,
		uint32 frame_count,
		uint32 sample_rate,
		uint32 channels,
		trait::Malloc& allocator
	);

	// ADTS frames are self-delimiting, so that form fits the generic codec interface and can be
	// probed like any other audio file.
	class AACCodec : public IAudioCodec {
	public:
		virtual ~AACCodec() = default;

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

#endif // _INC_FORMAT_AUDIO_AAC
