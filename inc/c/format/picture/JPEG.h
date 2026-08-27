// ASCII C/C++ TAB4 CRLF
// Docutitle: [Format.Picture] Joint Photographic Experts Group
// Attribute: Env-Freestanding Non-Dependence
// Copyright: UNISYM

#ifndef _INC_FORMAT_PICTURE_JPEG
#define _INC_FORMAT_PICTURE_JPEG

#include "../../stdinc.h"

// C++ only: enum class and uni::ColorSpace are C++ features.
#if defined(_INC_CPP)

#include "../../graphic/color.h"
#include "../../../cpp/System/Picture.hpp"

// ===== JPEG file markers =====
// Each marker is 1 byte 0xFF followed by 1 byte type
enum class JPEGMarker : byte {
	SOI  = 0xD8, // Start Of Image
	EOI  = 0xD9, // End Of Image
	SOF0 = 0xC0, // Baseline DCT (most common)
	SOF1 = 0xC1, // Extended sequential DCT
	SOF2 = 0xC2, // Progressive DCT
	SOF3 = 0xC3, // Lossless
	DHT  = 0xC4, // Define Huffman Table
	DQT  = 0xDB, // Define Quantization Table
	DRI  = 0xDD, // Define Restart Interval
	SOS  = 0xDA, // Start Of Scan
	APP0 = 0xE0, // Application segment 0 (JFIF)
	APP1 = 0xE1, // Application segment 1 (EXIF)
	COM  = 0xFE, // Comment
	// Restart markers RST0..RST7 = 0xD0..0xD7
};

// ===== Chroma subsampling =====
enum class JPEGSubsampling : byte {
	_444 = 0, // 8x8 MCU
	_420 = 1, // 16x16 MCU
	_422 = 2, // 16x8 MCU
};

// ===== Table sizes =====
#define JPEG_QUANT_TABLE_SIZE 64 // Quantization table: 8x8
#define JPEG_HUFF_BITS_SIZE   16  // Huffman table: number of codes per length
#define JPEG_HUFF_VAL_SIZE    256 // Huffman table: code values
#define JPEG_MAX_COMPONENTS   4   // YCbCr=3, CMYK=4

// ===== Quantization table (DQT) =====
typedef struct {
	byte precision; // 0: 8bit, 1: 16bit
	byte table[JPEG_QUANT_TABLE_SIZE]; // 8x8 coefficients (JPEG standard order)
} JPEG_QUANT_TABLE;

// ===== Huffman table (DHT) =====
typedef struct {
	byte bits[JPEG_HUFF_BITS_SIZE];     // number of codes of each length
	byte huffval[JPEG_HUFF_VAL_SIZE];   // code values
} JPEG_HUFF_TABLE;

// ===== SOF frame header (shared by SOF0/1/2/3) =====
typedef struct {
	byte precision;        // sample precision (8 or 12)
	uint16 height;         // number of lines
	uint16 width;          // number of pixels per line
	byte ncomp;            // number of components
	byte comp_id[JPEG_MAX_COMPONENTS]; // component identifiers
	byte comp_h[JPEG_MAX_COMPONENTS];  // horizontal sampling factors
	byte comp_v[JPEG_MAX_COMPONENTS];  // vertical sampling factors
	byte comp_q[JPEG_MAX_COMPONENTS];  // quantization table index used
} JPEG_FRAME_HEADER;

// ===== SOS scan header =====
typedef struct {
	byte ncomp;            // number of components in this scan
	byte comp_id[JPEG_MAX_COMPONENTS]; // component identifiers
	byte comp_dc[JPEG_MAX_COMPONENTS]; // DC Huffman table indices
	byte comp_ac[JPEG_MAX_COMPONENTS]; // AC Huffman table indices
} JPEG_SCAN_HEADER;

// ===== Decoding info =====
// Parsed from the JPEG file header, for mapping into uni::ImageInfo:
//   width/height      -> ImageInfo.width/height
//   colorSpace        -> ImageInfo.colorSpace (GRAY / SRGB / CMYK)
//   subsampling       -> decides MCU layout (444:8x8, 420:16x16, 422:16x8)
//   quality           -> encoding quality estimate (1..100)
// Output pixel layout is decided by the upper codec via uni::PixelFormat,
// not re-defined here.
typedef struct {
	uint32 width;
	uint32 height;
	byte precision;   // sample precision
	byte ncomp;       // number of components
	uni::ColorSpace colorSpace; // GRAY / SRGB / CMYK
	JPEGSubsampling subsampling;
	byte quality;     // 1..100, 0 if unknown
} JPEG_INFO;

uni::Color* DecodeJPEG(const byte* fileData, size_t fileSize, int* outWidth, int* outHeight);

namespace uni {
	class JPEGCodec : public IImageCodec {
	public:
		virtual ~JPEGCodec() = default;

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

#endif // _INC_FORMAT_PICTURE_JPEG
