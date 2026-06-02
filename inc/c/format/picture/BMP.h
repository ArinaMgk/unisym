// ASCII C/C++ TAB4 CRLF
// Docutitle: Windows BMP Format Definition
// Codifiers: @Antigravity
// Attribute: Env-Freestanding Non-Dependence
// Copyright: UNISYM

#ifndef _INC_FORMAT_PICTURE_BMP
#define _INC_FORMAT_PICTURE_BMP

#include "../../stdinc.h"
#include "../../graphic/color.h"

#if defined(_INC_CPP) || defined(__cplusplus)
extern "C" {
#endif

#pragma pack(push, 1)

typedef struct {
	uint16 bfType;      // 'BM' (0x4D42)
	uint32 bfSize;      // File size in bytes
	uint16 bfReserved1; // 0
	uint16 bfReserved2; // 0
	uint32 bfOffBits;   // Offset to start of pixel data
} BITMAPFILEHEADER;

typedef struct {
	uint32 biSize;          // Size of this header (should be 40)
	int32  biWidth;         // Width of image
	int32  biHeight;        // Height of image (positive = bottom-up, negative = top-down)
	uint16 biPlanes;        // Number of color planes (must be 1)
	uint16 biBitCount;      // Bits per pixel (1, 4, 8, 24, or 32)
	uint32 biCompression;   // Compression type (0 = BI_RGB)
	uint32 biSizeImage;     // Size of image data in bytes
	int32  biXPelsPerMeter; // Horizontal resolution
	int32  biYPelsPerMeter; // Vertical resolution
	uint32 biClrUsed;       // Number of colors in palette
	uint32 biClrImportant;  // Number of important colors
} BITMAPINFOHEADER;

typedef struct {
	byte rgbBlue;
	byte rgbGreen;
	byte rgbRed;
	byte rgbReserved;
} RGBQUAD;

#pragma pack(pop)

// Compression constants
#define BMP_BI_RGB 0

#if defined(_INC_CPP) || defined(__cplusplus)
uni::Color* DecodeBMP(const byte* fileData, size_t fileSize, int* outWidth, int* outHeight);
#else
struct Color* DecodeBMP(const byte* fileData, size_t fileSize, int* outWidth, int* outHeight);
#endif

#if defined(_INC_CPP) || defined(__cplusplus)
}
#endif

#endif // _INC_FORMAT_PICTURE_BMP
