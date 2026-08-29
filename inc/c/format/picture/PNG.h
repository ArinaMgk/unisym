// ASCII C/C++ TAB4 CRLF
// Docutitle: [Format.Picture] Portable Network Graphics
// Attribute: Env-Freestanding Non-Dependence
// Copyright: UNISYM

#ifndef _INC_FORMAT_PICTURE_PNG
#define _INC_FORMAT_PICTURE_PNG

#include "../../stdinc.h"

#if defined(_INC_CPP)

#include "../../graphic/color.h"
#include "../../../cpp/System/Picture.hpp"

// ===== PNG Chunk Types =====
#define PNG_CHUNK_IHDR 0x49484452 // "IHDR"
#define PNG_CHUNK_PLTE 0x504C5445 // "PLTE"
#define PNG_CHUNK_IDAT 0x49444154 // "IDAT"
#define PNG_CHUNK_IEND 0x49454E44 // "IEND"
#define PNG_CHUNK_tRNS 0x74524E53 // "tRNS"
#define PNG_CHUNK_gAMA 0x67414D41 // "gAMA"
#define PNG_CHUNK_cHRM 0x6348524D // "cHRM"
#define PNG_CHUNK_sRGB 0x73524742 // "sRGB"

// ===== PNG Color Types =====
enum class PNGColorType : byte {
	GRAYSCALE       = 0, // 1, 2, 4, 8, 16 bits
	RGB             = 2, // 8, 16 bits
	INDEXED         = 3, // 1, 2, 4, 8 bits (palette)
	GRAYSCALE_ALPHA = 4, // 8, 16 bits
	RGBA            = 6, // 8, 16 bits
};

// ===== PNG Filter Types =====
enum class PNGFilterType : byte {
	NONE    = 0,
	SUB     = 1,
	UP      = 2,
	AVERAGE = 3,
	PAETH   = 4,
};

// ===== PNG Header (IHDR) =====
#pragma pack(push, 1)
typedef struct {
	uint32 width;              // Width of image in pixels (big-endian)
	uint32 height;             // Height of image in pixels (big-endian)
	byte   bit_depth;          // Bits per sample or per palette index (1, 2, 4, 8, 16)
	byte   color_type;         // Color type (0, 2, 3, 4, 6)
	byte   compression_method; // Compression method (0 = Deflate)
	byte   filter_method;      // Filter method (0 = standard 5 adaptive filters)
	byte   interlace_method;   // Interlace method (0 = no interlace, 1 = Adam7)
} PNG_IHDR;
#pragma pack(pop)

// Decodes raw PNG file buffer into Color pixel array
uni::Color* DecodePNG(const byte* fileData, size_t fileSize, int* outWidth, int* outHeight);

namespace uni {
	class PNGCodec : public IImageCodec {
	public:
		virtual ~PNGCodec() = default;

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

#endif // _INC_FORMAT_PICTURE_PNG
