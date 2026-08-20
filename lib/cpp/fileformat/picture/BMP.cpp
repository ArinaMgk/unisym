// ASCII C/C++ TAB4 CRLF
// Docutitle: Windows BMP Decoder Implementation
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

#if defined(_INC_CPP) || defined(__cplusplus)
#include <string.h>

const char* uni::BMPCodec::GetName() const {
	return "BMP";
}

uni::ImageFormat uni::BMPCodec::GetFormat() const {
	return uni::ImageFormat::BMP;
}

const char* const* uni::BMPCodec::GetExtensions() const {
	static const char* const extensions[] = { "bmp", "dib", nullptr };
	return extensions;
}

uni::ImageResult uni::BMPCodec::Probe(StorageTrait& storage, bool& matched) const {
	matched = false;

	uint16 bfType = 0;
	stduint block_size = storage.Block_Size ? storage.Block_Size : 512;

	byte* block_buf = nullptr;
	bool is_dyn = false;
	byte stack_buf[2048];
	if (block_size <= 2048) {
		block_buf = stack_buf;
	} else {
		block_buf = (byte*)malloc(block_size);
		if (!block_buf) {
			return uni::ImageResult::OUT_OF_MEMORY;
		}
		is_dyn = true;
	}

	stduint read_bytes = storage.Read(0, &bfType, 2, block_buf);

	if (is_dyn) {
		free(block_buf);
	}

	// Verify 'BM' magic number (0x4D42)
	if (read_bytes == 2 && bfType == 0x4D42) {
		matched = true;
	}

	return uni::ImageResult::OK;
}

uni::ImageResult uni::BMPCodec::ReadInfo(StorageTrait& storage, ImageInfo& outInfo) const {
	bool matched = false;
	uni::ImageResult res = Probe(storage, matched);
	if (res != uni::ImageResult::OK) {
		return res;
	}
	if (!matched) {
		return uni::ImageResult::INVALID_FORMAT;
	}

	BITMAPFILEHEADER fileHeader;
	BITMAPINFOHEADER infoHeader;

	stduint block_size = storage.Block_Size ? storage.Block_Size : 512;
	byte* block_buf = nullptr;
	bool is_dyn = false;
	byte stack_buf[2048];
	if (block_size <= 2048) {
		block_buf = stack_buf;
	} else {
		block_buf = (byte*)malloc(block_size);
		if (!block_buf) {
			return uni::ImageResult::OUT_OF_MEMORY;
		}
		is_dyn = true;
	}

	stduint read_bytes = storage.Read(0, &fileHeader, sizeof(fileHeader), block_buf);
	if (read_bytes == sizeof(fileHeader)) {
		read_bytes = storage.Read(sizeof(fileHeader), &infoHeader, sizeof(infoHeader), block_buf);
	}

	if (is_dyn) {
		free(block_buf);
	}

	if (read_bytes != sizeof(infoHeader)) {
		return uni::ImageResult::INVALID_FORMAT;
	}

	if (infoHeader.biSize < 40 || infoHeader.biCompression != BMP_BI_RGB) {
		return uni::ImageResult::UNSUPPORTED;
	}

	uint16 bitCount = infoHeader.biBitCount;
	if (bitCount != 1 && bitCount != 4 && bitCount != 8 && bitCount != 24 && bitCount != 32) {
		return uni::ImageResult::UNSUPPORTED;
	}

	int32 width = infoHeader.biWidth;
	int32 height = infoHeader.biHeight;
	if (width <= 0 || height == 0) {
		return uni::ImageResult::INVALID_FORMAT;
	}

	int32 absHeight = height < 0 ? -height : height;

	outInfo.width = (uint32)width;
	outInfo.height = (uint32)absHeight;
	outInfo.format = PixelFormat::ARGB8888;
	outInfo.colorSpace = ColorSpace::SRGB;
	outInfo.alphaMode = (bitCount == 32) ? ImageAlphaMode::STRAIGHT : ImageAlphaMode::NONE;
	outInfo.fileFormat = ImageFormat::BMP;
	outInfo.bitsPerPixel = bitCount;
	outInfo.frameCount = 1;
	outInfo.hasAlpha = (bitCount == 32);
	outInfo.hasAnimation = false;

	return uni::ImageResult::OK;
}

uni::ImageResult uni::BMPCodec::OpenSurface(
	StorageTrait& storage,
	IImageSurface*& outSurface,
	trait::Malloc& allocator,
	const ImageDecodeOptions& options,
	ImageAccessMode access
) const {
	return uni::ImageResult::UNSUPPORTED;
}

uni::ImageResult uni::BMPCodec::Decode(
	StorageTrait& storage,
	ImageBuffer& outBuffer,
	trait::Malloc& allocator,
	const ImageDecodeOptions& options
) const {
	BITMAPFILEHEADER fileHeader;
	BITMAPINFOHEADER infoHeader;

	stduint block_size = storage.Block_Size ? storage.Block_Size : 512;
	byte* block_buf = nullptr;
	bool is_dyn = false;
	byte stack_buf[2048];
	if (block_size <= 2048) {
		block_buf = stack_buf;
	} else {
		block_buf = (byte*)malloc(block_size);
		if (!block_buf) {
			return uni::ImageResult::OUT_OF_MEMORY;
		}
		is_dyn = true;
	}

	stduint read_bytes = storage.Read(0, &fileHeader, sizeof(fileHeader), block_buf);
	if (read_bytes == sizeof(fileHeader)) {
		read_bytes = storage.Read(sizeof(fileHeader), &infoHeader, sizeof(infoHeader), block_buf);
	}

	if (read_bytes != sizeof(infoHeader)) {
		if (is_dyn) free(block_buf);
		return uni::ImageResult::INVALID_FORMAT;
	}

	if (fileHeader.bfType != 0x4D42 || infoHeader.biSize < 40 || infoHeader.biCompression != BMP_BI_RGB) {
		if (is_dyn) free(block_buf);
		return uni::ImageResult::UNSUPPORTED;
	}

	uint16 bitCount = infoHeader.biBitCount;
	if (bitCount != 1 && bitCount != 4 && bitCount != 8 && bitCount != 24 && bitCount != 32) {
		if (is_dyn) free(block_buf);
		return uni::ImageResult::UNSUPPORTED;
	}

	int32 width = infoHeader.biWidth;
	int32 height = infoHeader.biHeight;
	if (width <= 0 || height == 0) {
		if (is_dyn) free(block_buf);
		return uni::ImageResult::INVALID_FORMAT;
	}

	stduint fileSize = fileHeader.bfSize;
	stduint maxStorageSize = storage.getUnits() * storage.Block_Size;
	if (fileSize == 0 || fileSize > maxStorageSize) {
		fileSize = maxStorageSize;
	}

	if (fileSize < sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)) {
		if (is_dyn) free(block_buf);
		return uni::ImageResult::INVALID_FORMAT;
	}

	// Allocate temporary file buffer using malloc since it is transient data
	byte* fileData = (byte*)malloc(fileSize);
	if (!fileData) {
		if (is_dyn) free(block_buf);
		return uni::ImageResult::OUT_OF_MEMORY;
	}

	stduint totalRead = storage.Read(0, fileData, fileSize, block_buf);
	if (is_dyn) {
		free(block_buf);
	}

	int outWidth = 0;
	int outHeight = 0;
	uni::Color* stdPixels = DecodeBMP(fileData, totalRead, &outWidth, &outHeight);
	free(fileData);

	if (!stdPixels) {
		return uni::ImageResult::FAILED;
	}

	// Allocate pixel buffer using the custom allocator required by the IImageCodec framework
	size_t pixelBufferSize = outWidth * outHeight * sizeof(uni::Color);
	void* targetPixels = allocator.allocate(pixelBufferSize);
	if (!targetPixels) {
		free(stdPixels);
		return uni::ImageResult::OUT_OF_MEMORY;
	}

	memcpy(targetPixels, stdPixels, pixelBufferSize);
	free(stdPixels);

	outBuffer.width = (uint32)outWidth;
	outBuffer.height = (uint32)outHeight;
	outBuffer.stride = outWidth * sizeof(uni::Color);
	outBuffer.format = PixelFormat::ARGB8888;
	outBuffer.colorSpace = ColorSpace::SRGB;
	outBuffer.alphaMode = (bitCount == 32) ? ImageAlphaMode::STRAIGHT : ImageAlphaMode::NONE;
	outBuffer.pixels = targetPixels;
	outBuffer.size = pixelBufferSize;
	outBuffer.allocator = &allocator;

	return uni::ImageResult::OK;
}

uni::ImageResult uni::BMPCodec::Encode(
	const ImageBuffer& image,
	StorageTrait& storage,
	trait::Malloc& allocator,
	const ImageEncodeOptions& options
) const {
	return uni::ImageResult::UNSUPPORTED;
}

bool uni::BMPCodec::CanEncode(PixelFormat format) const {
	return false;
}
#endif
