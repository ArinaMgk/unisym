// ASCII C/C++ TAB4 CRLF
// Docutitle: [Format.Picture] Graphics Interchange Format
// Attribute: Env-Freestanding Non-Dependence
// Copyright: UNISYM

#ifndef _INC_FORMAT_PICTURE_GIF
#define _INC_FORMAT_PICTURE_GIF

#include "../../stdinc.h"

#if defined(_INC_CPP)

#include "../../graphic/color.h"
#include "../../../cpp/System/Picture.hpp"
#include "../../../cpp/endian"

// ===== GIF Block & Extension Markers =====
#define GIF_INTRO_IMAGE         0x2C // ',' Image Descriptor
#define GIF_INTRO_EXTENSION     0x21 // '!' Extension Block
#define GIF_TRAILER             0x3B // ';' Trailer (End of GIF)

#define GIF_EXT_GRAPHIC_CONTROL 0xF9 // Graphic Control Extension
#define GIF_EXT_COMMENT          0xFE // Comment Extension
#define GIF_EXT_PLAIN_TEXT       0x01 // Plain Text Extension
#define GIF_EXT_APPLICATION      0xFF // Application Extension (e.g. NETSCAPE2.0 loop)

// ===== GIF Disposal Methods =====
enum class GIFDisposalMethod : byte {
	UNSPECIFIED    = 0, // No disposal specified
	DO_NOT_DISPOSE = 1, // Leave canvas as is
	RESTORE_BG     = 2, // Restore to background color
	RESTORE_PREV   = 3, // Restore to previous frame state
};

// ===== GIF Logical Screen Descriptor =====
#pragma pack(push, 1)
typedef struct {
	LitEndian<uint16, true> width;              // Logical screen width (little-endian)
	LitEndian<uint16, true> height;             // Logical screen height (little-endian)
	byte   packed_fields;      // GCT flag (1 bit), Color resolution (3 bits), Sort flag (1 bit), GCT size (3 bits)
	byte   bg_color_index;     // Background color index
	byte   pixel_aspect_ratio; // Pixel aspect ratio
} GIF_SCREEN_DESCRIPTOR;

// ===== GIF Image Descriptor =====
typedef struct {
	LitEndian<uint16, true> left;               // Image left position
	LitEndian<uint16, true> top;                // Image top position
	LitEndian<uint16, true> width;              // Image width
	LitEndian<uint16, true> height;             // Image height
	byte   packed_fields;      // LCT flag (1 bit), Interlace flag (1 bit), Sort flag (1 bit), Reserved (2 bits), LCT size (3 bits)
} GIF_IMAGE_DESCRIPTOR;
#pragma pack(pop)

// Single animation frame structure for GIF playback
struct GIFFrame {
	uni::Color* pixels;        // Canvas pixel buffer for this frame (allocated via malloc)
	uint32      delayMs;       // Frame delay time in milliseconds
};

// Full decoded GIF animation structure
struct GIFAnimation {
	uint32     width;
	uint32     height;
	uint32     frameCount;
	GIFFrame*  frames;         // Array of frames (allocated via malloc)
};

// Decodes a single frame (frame 0) of a raw GIF buffer into a uni::Color pixel array
uni::Color* DecodeGIF(const byte* fileData, size_t fileSize, int* outWidth, int* outHeight);

// Decodes all frames of a GIF image for animation playback
bool DecodeGIFAnimation(const byte* fileData, size_t fileSize, GIFAnimation& outAnim);

// Frees all resources associated with a GIFAnimation structure
void FreeGIFAnimation(GIFAnimation& anim);

namespace uni {
	class GIFCodec : public IImageCodec {
	public:
		virtual ~GIFCodec() = default;

		virtual const char* GetName() const override;
		virtual ImageFormat GetFormat() const override;
		virtual const char* const* GetExtensions() const override;

		virtual ImageResult Probe(StorageTrait& storage, bool& matched) const override;
		virtual ImageResult ReadInfo(StorageTrait& storage, ImageInfo& outInfo) const override;

		virtual ImageResult OpenSurface(
			StorageTrait& storage,
			IImageSurface*& outSurface,
			trait::Malloc& allocator,
			const ImageDecodeOptions& options,
			ImageAccessMode access
		) const override;

		virtual ImageResult Decode(
			StorageTrait& storage,
			ImageBuffer& outBuffer,
			trait::Malloc& allocator,
			const ImageDecodeOptions& options
		) const override;

		virtual ImageResult Encode(
			const ImageBuffer& image,
			StorageTrait& storage,
			trait::Malloc& allocator,
			const ImageEncodeOptions& options
		) const override;

		virtual bool CanEncode(PixelFormat format) const override;
	};
}

#endif // _INC_CPP

#endif // _INC_FORMAT_PICTURE_GIF
