// ASCII C/C++ TAB4 CRLF
// Docutitle: MIDI Audio Format Definition
// Attribute: Env-Freestanding Non-Dependence
// Copyright: UNISYM

#ifndef _INC_FORMAT_AUDIO_MIDI
#define _INC_FORMAT_AUDIO_MIDI

#include "../../stdinc.h"

#if defined(_INC_CPP)
extern "C" {
#endif

#pragma pack(push, 1)

#define MIDI_MAGIC_MTHD 0x6468544Du // 'MThd' in little-endian
#define MIDI_MAGIC_MTRK 0x6B72544Du // 'MTrk' in little-endian

typedef struct {
	uint32 chunk_type;    // 'MThd'
	uint32 length;        // 6 (Big Endian)
	uint16 format;        // 0: single track, 1: multi track sync, 2: multi track async
	uint16 track_count;   // Number of tracks
	uint16 time_division; // Ticks per quarter-note or SMPTE
} MIDI_HEADER;

typedef struct {
	uint32 chunk_type;    // 'MTrk'
	uint32 length;        // Track length in bytes (Big Endian)
} MIDI_TRACK_HEADER;

#pragma pack(pop)

#if defined(_INC_CPP)
}
#endif

#if defined(_INC_CPP)
#include "../../../cpp/System/Audiosys.hpp"

namespace uni {
	class MIDICodec : public IAudioCodec {
	public:
		virtual ~MIDICodec() = default;

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

#endif // _INC_FORMAT_AUDIO_MIDI
