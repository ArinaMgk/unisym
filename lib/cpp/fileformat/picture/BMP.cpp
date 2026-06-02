// ASCII C/C++ TAB4 CRLF
// Docutitle: Windows BMP Decoder Implementation
// Codifiers: @Antigravity
// Attribute: Env-Freestanding Non-Dependence
// Copyright: UNISYM

#include "../../../../inc/c/format/picture/BMP.h"
#include <stdlib.h>

#if defined(_INC_CPP) || defined(__cplusplus)
extern "C" {
#endif

// Decodes a raw BMP file buffer.
// Returns a pointer to an array of Color structs (allocated via malloc), or nullptr on failure.
// The caller is responsible for freeing the returned pixel buffer via free().
uni::Color* DecodeBMP(const byte* fileData, size_t fileSize, int* outWidth, int* outHeight) {
	if (!fileData || fileSize < sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)) {
		return nullptr;
	}

	const BITMAPFILEHEADER* fileHeader = (const BITMAPFILEHEADER*)fileData;
	
	// Verify 'BM' magic number
	if (fileHeader->bfType != 0x4D42) {
		return nullptr;
	}

	const BITMAPINFOHEADER* infoHeader = (const BITMAPINFOHEADER*)(fileData + sizeof(BITMAPFILEHEADER));
	
	// Verify minimum header size and compression type (must be BI_RGB)
	if (infoHeader->biSize < 40 || infoHeader->biCompression != BMP_BI_RGB) {
		return nullptr;
	}

	uint16 bitCount = infoHeader->biBitCount;
	
	// Verify bits per pixel (must be 1, 4, 8, 24, or 32)
	if (bitCount != 1 && bitCount != 4 && bitCount != 8 && bitCount != 24 && bitCount != 32) {
		return nullptr;
	}

	int32 width = infoHeader->biWidth;
	int32 height = infoHeader->biHeight;
	
	// Width must be positive; height cannot be 0
	if (width <= 0 || height == 0) {
		return nullptr;
	}

	int32 absHeight = height < 0 ? -height : height;

	// Extract palette for palettized formats (<= 8bpp)
	const RGBQUAD* palette = nullptr;
	uint32 numColors = 0;
	if (bitCount <= 8) {
		palette = (const RGBQUAD*)(fileData + sizeof(BITMAPFILEHEADER) + infoHeader->biSize);
		if (infoHeader->biClrUsed > 0) {
			numColors = infoHeader->biClrUsed;
		} else {
			numColors = 1 << bitCount;
		}
		
		// Ensure the palette does not exceed the file buffer boundary
		if ((const byte*)(palette + numColors) > fileData + fileSize) {
			return nullptr;
		}
	}

	// Calculate row size in bytes (padded to 4-byte boundary)
	uint32 rowSize;
	switch (bitCount) {
	case 1:
		rowSize = ((width + 31) / 32) * 4;
		break;
	case 4:
		rowSize = ((width + 7) / 8) * 4;
		break;
	case 8:
		rowSize = ((width + 3) / 4) * 4;
		break;
	case 24:
		rowSize = ((width * 3 + 3) / 4) * 4;
		break;
	case 32:
		rowSize = width * 4;
		break;
	default:
		return nullptr;
	}

	// Check file size boundaries to avoid buffer overflow
	if (fileHeader->bfOffBits + rowSize * absHeight > fileSize) {
		return nullptr;
	}

	// Allocate buffer for output pixels
	uni::Color* pixels = (uni::Color*)malloc(width * absHeight * sizeof(uni::Color));
	if (!pixels) {
		return nullptr;
	}

	const byte* pixelDataStart = fileData + fileHeader->bfOffBits;

	for (int32 y = 0; y < absHeight; ++y) {
		// If height > 0, BMP is bottom-up (first row in file is bottom-most row of image)
		// If height < 0, BMP is top-down (first row in file is top-most row of image)
		int32 srcY = (height > 0) ? (absHeight - 1 - y) : y;
		const byte* srcRow = pixelDataStart + srcY * rowSize;
		uni::Color* destRow = pixels + y * width;

		switch (bitCount) {
		case 1:
			for (int32 x = 0; x < width; ++x) {
				int byteIdx = x / 8;
				int bitShift = 7 - (x % 8);
				byte index = (srcRow[byteIdx] >> bitShift) & 1;
				
				if (index < numColors) {
					destRow[x].b = palette[index].rgbBlue;
					destRow[x].g = palette[index].rgbGreen;
					destRow[x].r = palette[index].rgbRed;
				} else {
					destRow[x].b = destRow[x].g = destRow[x].r = 0;
				}
				destRow[x].a = 0xFF; // Opaque
			}
			break;

		case 4:
			for (int32 x = 0; x < width; ++x) {
				int byteIdx = x / 2;
				byte index = (x % 2 == 0) ? ((srcRow[byteIdx] >> 4) & 0x0F) : (srcRow[byteIdx] & 0x0F);
				
				if (index < numColors) {
					destRow[x].b = palette[index].rgbBlue;
					destRow[x].g = palette[index].rgbGreen;
					destRow[x].r = palette[index].rgbRed;
				} else {
					destRow[x].b = destRow[x].g = destRow[x].r = 0;
				}
				destRow[x].a = 0xFF; // Opaque
			}
			break;

		case 8:
			for (int32 x = 0; x < width; ++x) {
				byte index = srcRow[x];
				
				if (index < numColors) {
					destRow[x].b = palette[index].rgbBlue;
					destRow[x].g = palette[index].rgbGreen;
					destRow[x].r = palette[index].rgbRed;
				} else {
					destRow[x].b = destRow[x].g = destRow[x].r = 0;
				}
				destRow[x].a = 0xFF; // Opaque
			}
			break;

		case 24:
			for (int32 x = 0; x < width; ++x) {
				byte b = srcRow[x * 3 + 0];
				byte g = srcRow[x * 3 + 1];
				byte r = srcRow[x * 3 + 2];
				destRow[x].b = b;
				destRow[x].g = g;
				destRow[x].r = r;
				destRow[x].a = 0xFF; // Opaque by default for 24-bit
			}
			break;

		case 32:
			for (int32 x = 0; x < width; ++x) {
				byte b = srcRow[x * 4 + 0];
				byte g = srcRow[x * 4 + 1];
				byte r = srcRow[x * 4 + 2];
				byte a = srcRow[x * 4 + 3];
				destRow[x].b = b;
				destRow[x].g = g;
				destRow[x].r = r;
				destRow[x].a = a;
			}
			break;

		default:
			break;
		}
	}

	if (outWidth) *outWidth = width;
	if (outHeight) *outHeight = absHeight;

	return pixels;
}

#if defined(_INC_CPP) || defined(__cplusplus)
}
#endif
