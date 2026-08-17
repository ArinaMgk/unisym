// ASCII C/C++ TAB4 CRLF
// Docutitle: RIFF WAVE Format Definition
// Attribute: Env-Freestanding Non-Dependence
// Copyright: UNISYM

#ifndef _INC_FORMAT_AUDIO_WAV
#define _INC_FORMAT_AUDIO_WAV

#include "../../stdinc.h"

#if defined(_INC_CPP) || defined(__cplusplus)
extern "C" {
#endif

#pragma pack(push, 1)

typedef struct {
	uint32 riff_tag;      // 'RIFF'
	uint32 riff_size;     // file size - 8
	uint32 wave_tag;      // 'WAVE'
} WAVRIFFHEADER;

typedef struct {
	uint32 chunk_tag;
	uint32 chunk_size;
} WAVCHUNKHEADER;

typedef struct {
	uint16 audio_format;      // PCM = 1
	uint16 channel_count;
	uint32 sample_rate;
	uint32 byte_rate;
	uint16 block_align;
	uint16 bits_per_sample;
} WAVFMTPCM;

typedef struct {
	uint16 audio_format;
	uint16 channel_count;
	uint32 sample_rate;
	uint16 bits_per_sample;
	const byte* pcm_data;
	uint32 pcm_size;
} WAVPCMVIEW;

#pragma pack(pop)

#define WAV_FOURCC_RIFF 0x46464952u
#define WAV_FOURCC_WAVE 0x45564157u
#define WAV_FOURCC_FMT  0x20746D66u
#define WAV_FOURCC_DATA 0x61746164u

#define WAV_FORMAT_PCM  0x0001u

static inline const WAVCHUNKHEADER* WAV_FindChunk(
	const void* file_data, uint32 file_size, uint32 chunk_tag) {
	if (!file_data || file_size < sizeof(WAVRIFFHEADER)) return 0;
	const byte* bytes = (const byte*)file_data;
	const WAVRIFFHEADER* riff = (const WAVRIFFHEADER*)bytes;
	if (riff->riff_tag != WAV_FOURCC_RIFF || riff->wave_tag != WAV_FOURCC_WAVE) {
		return 0;
	}

	uint32 offset = sizeof(WAVRIFFHEADER);
	while (offset + sizeof(WAVCHUNKHEADER) <= file_size) {
		const WAVCHUNKHEADER* chunk = (const WAVCHUNKHEADER*)(bytes + offset);
		const uint32 chunk_data_offset = offset + sizeof(WAVCHUNKHEADER);
		const uint32 padded_size = chunk->chunk_size + (chunk->chunk_size & 1u);
		if (chunk_data_offset > file_size ||
			chunk->chunk_size > file_size - chunk_data_offset) {
			return 0;
		}
		if (chunk->chunk_tag == chunk_tag) return chunk;
		if (padded_size > file_size - chunk_data_offset) return 0;
		offset = chunk_data_offset + padded_size;
	}
	return 0;
}

static inline bool WAV_ParsePCM(
	const void* file_data, uint32 file_size, WAVPCMVIEW* out_view) {
	if (!out_view) return false;
	out_view->audio_format = 0;
	out_view->channel_count = 0;
	out_view->sample_rate = 0;
	out_view->bits_per_sample = 0;
	out_view->pcm_data = 0;
	out_view->pcm_size = 0;

	const WAVCHUNKHEADER* fmt_chunk =
		WAV_FindChunk(file_data, file_size, WAV_FOURCC_FMT);
	const WAVCHUNKHEADER* data_chunk =
		WAV_FindChunk(file_data, file_size, WAV_FOURCC_DATA);
	if (!fmt_chunk || !data_chunk ||
		fmt_chunk->chunk_size < sizeof(WAVFMTPCM)) return false;

	const byte* bytes = (const byte*)file_data;
	const WAVFMTPCM* fmt = (const WAVFMTPCM*)(bytes +
		((const byte*)fmt_chunk - bytes) + sizeof(WAVCHUNKHEADER));
	const byte* pcm = (const byte*)data_chunk + sizeof(WAVCHUNKHEADER);

	out_view->audio_format = fmt->audio_format;
	out_view->channel_count = fmt->channel_count;
	out_view->sample_rate = fmt->sample_rate;
	out_view->bits_per_sample = fmt->bits_per_sample;
	out_view->pcm_data = pcm;
	out_view->pcm_size = data_chunk->chunk_size;
	return true;
}

#if defined(_INC_CPP) || defined(__cplusplus)
}
#endif

#endif // _INC_FORMAT_AUDIO_WAV
