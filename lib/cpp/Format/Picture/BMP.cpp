// ASCII C/C++ TAB4 CRLF
// Docutitle: Windows BMP Decoder Implementation
// Attribute: Env-Freestanding Non-Dependence
// Copyright: UNISYM

#include "../../../../inc/c/format/picture/BMP.h"
#include "../../../../inc/c/ustring.h"

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
	byte* block_buf = (byte*)malloc(block_size);
	if (!block_buf) {
		return uni::ImageResult::OUT_OF_MEMORY;
	}

	stduint read_bytes = storage.Read(0, &bfType, 2, block_buf);
	free(block_buf);

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
	byte* block_buf = (byte*)malloc(block_size);
	if (!block_buf) {
		return uni::ImageResult::OUT_OF_MEMORY;
	}

	stduint read_bytes = storage.Read(0, &fileHeader, sizeof(fileHeader), block_buf);
	if (read_bytes == sizeof(fileHeader)) {
		read_bytes = storage.Read(sizeof(fileHeader), &infoHeader, sizeof(infoHeader), block_buf);
	}
	free(block_buf);

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

namespace {

	class BMPSurface : public uni::IImageSurface {
	private:
		uni::StorageTrait* storage;
		uni::ImageInfo     info;
		uint32             bfOffBits;
		uint16             bitCount;
		uint32             rowSize;
		int32              rawHeight; // > 0: bottom-up, < 0: top-down
		RGBQUAD            palette[256];
		uint32             numColors;
		uni::trait::Malloc* allocator;

	public:
		BMPSurface(uni::StorageTrait& stg, const uni::ImageInfo& inf, uint32 offBits, uint16 bpp, uint32 rSize, int32 rHeight, const RGBQUAD* pal, uint32 palCount, uni::trait::Malloc& alloc)
			: storage(&stg), info(inf), bfOffBits(offBits), bitCount(bpp), rowSize(rSize), rawHeight(rHeight), numColors(palCount), allocator(&alloc) {
			if (pal && palCount) {
				MemCopyN(palette, pal, (palCount > 256 ? 256 : palCount) * sizeof(RGBQUAD));
			}
		}

		virtual ~BMPSurface() = default;

		virtual void Release() override {
			uni::trait::Malloc* alloc = allocator;
			this->~BMPSurface();
			if (alloc) {
				alloc->deallocate(this);
			}
		}

		virtual uni::ImageResult GetInfo(uni::ImageInfo& outInfo) const override {
			outInfo = info;
			return uni::ImageResult::OK;
		}

		virtual uni::ImageSurfaceCapability GetCapabilities() const override {
			return uni::ImageSurfaceCapability::FULL_READ | uni::ImageSurfaceCapability::REGION_READ |
				   uni::ImageSurfaceCapability::SCANLINE_READ | uni::ImageSurfaceCapability::RANDOM_STORAGE;
		}

		virtual uni::ImageResult GetMetadata(uni::IImageMetadata*& outMetadata) override {
			outMetadata = nullptr;
			return uni::ImageResult::NOT_FOUND;
		}

		virtual uni::ImageResult ReadPixels(
			const uni::Rectangle& rect,
			uni::ImageBuffer& outBuffer,
			uni::trait::Malloc& alloc
		) override {
			int rx = rect.x;
			int ry = rect.y;
			int rw = rect.width;
			int rh = rect.height;

			if (rx < 0 || ry < 0 || rx + rw > (int)info.width || ry + rh > (int)info.height || rw <= 0 || rh <= 0) {
				return uni::ImageResult::INVALID_ARGUMENT;
			}

			size_t neededSize = (size_t)rw * (size_t)rh * sizeof(uni::Color);
			void* pixelsMem = outBuffer.pixels;
			bool ownsMem = false;
			if (!pixelsMem) {
				pixelsMem = alloc.allocate(neededSize);
				if (!pixelsMem) return uni::ImageResult::OUT_OF_MEMORY;
				ownsMem = true;
			}

			uni::Color* outPixels = (uni::Color*)pixelsMem;
			byte* rowBuf = (byte*)malloc(rowSize);
			if (!rowBuf) {
				if (ownsMem) alloc.deallocate(pixelsMem);
				return uni::ImageResult::OUT_OF_MEMORY;
			}

			stduint block_size = storage->Block_Size ? storage->Block_Size : 512;
			byte* block_buf = (byte*)malloc(block_size);
			if (!block_buf) {
				free(rowBuf);
				if (ownsMem) alloc.deallocate(pixelsMem);
				return uni::ImageResult::OUT_OF_MEMORY;
			}

			for (int line = 0; line < rh; ++line) {
				int y = ry + line;
				int diskRow = (rawHeight > 0) ? ((int)info.height - 1 - y) : y;
				stduint fileOffset = bfOffBits + (stduint)diskRow * rowSize;

				stduint readBytes = storage->Read(fileOffset, rowBuf, rowSize, block_buf);
				if (readBytes != rowSize) {
					free(block_buf);
					free(rowBuf);
					if (ownsMem) alloc.deallocate(pixelsMem);
					return uni::ImageResult::FAILED;
				}

				uni::Color* destRow = outPixels + line * rw;
				for (int col = 0; col < rw; ++col) {
					int x = rx + col;
					uni::Color c(0);
					switch (bitCount) {
					case 1: {
						byte b = rowBuf[x / 8];
						byte idx = (b >> (7 - (x % 8))) & 0x01;
						if (idx < numColors) {
							c.r = palette[idx].rgbRed;
							c.g = palette[idx].rgbGreen;
							c.b = palette[idx].rgbBlue;
							c.a = 0xFF;
						}
						break;
					}
					case 4: {
						byte b = rowBuf[x / 2];
						byte idx = (x % 2 == 0) ? ((b >> 4) & 0x0F) : (b & 0x0F);
						if (idx < numColors) {
							c.r = palette[idx].rgbRed;
							c.g = palette[idx].rgbGreen;
							c.b = palette[idx].rgbBlue;
							c.a = 0xFF;
						}
						break;
					}
					case 8: {
						byte idx = rowBuf[x];
						if (idx < numColors) {
							c.r = palette[idx].rgbRed;
							c.g = palette[idx].rgbGreen;
							c.b = palette[idx].rgbBlue;
							c.a = 0xFF;
						}
						break;
					}
					case 24: {
						const byte* p = rowBuf + x * 3;
						c.b = p[0];
						c.g = p[1];
						c.r = p[2];
						c.a = 0xFF;
						break;
					}
					case 32: {
						const byte* p = rowBuf + x * 4;
						c.b = p[0];
						c.g = p[1];
						c.r = p[2];
						c.a = p[3];
						break;
					}
					}
					destRow[col] = c;
				}
			}

			free(block_buf);
			free(rowBuf);

			outBuffer.width = (uint32)rw;
			outBuffer.height = (uint32)rh;
			outBuffer.stride = (uint32)(rw * sizeof(uni::Color));
			outBuffer.format = uni::PixelFormat::ARGB8888;
			outBuffer.colorSpace = uni::ColorSpace::SRGB;
			outBuffer.alphaMode = (bitCount == 32) ? uni::ImageAlphaMode::STRAIGHT : uni::ImageAlphaMode::NONE;
			outBuffer.pixels = pixelsMem;
			outBuffer.size = neededSize;
			outBuffer.allocator = ownsMem ? &alloc : nullptr;

			return uni::ImageResult::OK;
		}

		virtual uni::ImageResult WritePixels(const uni::Rectangle& rect, const uni::ImageBuffer& srcBuffer) override {
			return uni::ImageResult::UNSUPPORTED;
		}

		virtual uni::ImageResult Flush() override {
			return uni::ImageResult::OK;
		}
	};

}

uni::ImageResult uni::BMPCodec::OpenSurface(
	StorageTrait& storage,
	IImageSurface*& outSurface,
	trait::Malloc& allocator,
	const ImageDecodeOptions& options,
	ImageAccessMode access
) const {
	if (access == ImageAccessMode::READ_WRITE) {
		return uni::ImageResult::UNSUPPORTED;
	}

	ImageInfo info;
	uni::ImageResult res = ReadInfo(storage, info);
	if (res != uni::ImageResult::OK) return res;

	BITMAPFILEHEADER fileHeader;
	BITMAPINFOHEADER infoHeader;

	stduint block_size = storage.Block_Size ? storage.Block_Size : 512;
	byte* block_buf = (byte*)malloc(block_size);
	if (!block_buf) return uni::ImageResult::OUT_OF_MEMORY;

	stduint read_bytes = storage.Read(0, &fileHeader, sizeof(fileHeader), block_buf);
	if (read_bytes == sizeof(fileHeader)) {
		read_bytes = storage.Read(sizeof(fileHeader), &infoHeader, sizeof(infoHeader), block_buf);
	}

	RGBQUAD palette[256];
	uint32 numColors = 0;
	if (infoHeader.biBitCount <= 8) {
		numColors = infoHeader.biClrUsed > 0 ? infoHeader.biClrUsed : (1 << infoHeader.biBitCount);
		if (numColors > 256) numColors = 256;
		storage.Read(sizeof(BITMAPFILEHEADER) + infoHeader.biSize, palette, numColors * sizeof(RGBQUAD), block_buf);
	}

	free(block_buf);

	uint32 rowSize = 0;
	switch (infoHeader.biBitCount) {
	case 1:  rowSize = ((info.width + 31) / 32) * 4; break;
	case 4:  rowSize = ((info.width + 7) / 8) * 4;   break;
	case 8:  rowSize = ((info.width + 3) / 4) * 4;   break;
	case 24: rowSize = ((info.width * 3 + 3) / 4) * 4; break;
	case 32: rowSize = info.width * 4; break;
	default: return uni::ImageResult::UNSUPPORTED;
	}

	void* mem = allocator.allocate(sizeof(BMPSurface));
	if (!mem) return uni::ImageResult::OUT_OF_MEMORY;

	BMPSurface* surf = new (mem) BMPSurface(
		storage, info, fileHeader.bfOffBits, infoHeader.biBitCount,
		rowSize, infoHeader.biHeight, palette, numColors, allocator
	);
	outSurface = surf;
	return uni::ImageResult::OK;
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
	byte* block_buf = (byte*)malloc(block_size);
	if (!block_buf) {
		return uni::ImageResult::OUT_OF_MEMORY;
	}

	stduint read_bytes = storage.Read(0, &fileHeader, sizeof(fileHeader), block_buf);
	if (read_bytes == sizeof(fileHeader)) {
		read_bytes = storage.Read(sizeof(fileHeader), &infoHeader, sizeof(infoHeader), block_buf);
	}

	if (read_bytes != sizeof(infoHeader)) {
		free(block_buf);
		return uni::ImageResult::INVALID_FORMAT;
	}

	if (fileHeader.bfType != 0x4D42 || infoHeader.biSize < 40 || infoHeader.biCompression != BMP_BI_RGB) {
		free(block_buf);
		return uni::ImageResult::UNSUPPORTED;
	}

	uint16 bitCount = infoHeader.biBitCount;
	if (bitCount != 1 && bitCount != 4 && bitCount != 8 && bitCount != 24 && bitCount != 32) {
		free(block_buf);
		return uni::ImageResult::UNSUPPORTED;
	}

	int32 width = infoHeader.biWidth;
	int32 height = infoHeader.biHeight;
	if (width <= 0 || height == 0) {
		free(block_buf);
		return uni::ImageResult::INVALID_FORMAT;
	}

	stduint fileSize = fileHeader.bfSize;
	stduint maxStorageSize = storage.getUnits() * storage.Block_Size;
	if (fileSize == 0 || fileSize > maxStorageSize) {
		fileSize = maxStorageSize;
	}

	if (fileSize < sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)) {
		free(block_buf);
		return uni::ImageResult::INVALID_FORMAT;
	}

	// Allocate temporary file buffer using malloc since it is transient data
	byte* fileData = (byte*)malloc(fileSize);
	if (!fileData) {
		free(block_buf);
		return uni::ImageResult::OUT_OF_MEMORY;
	}

	stduint totalRead = storage.Read(0, fileData, fileSize, block_buf);
	free(block_buf);

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

	MemCopyN(targetPixels, stdPixels, pixelBufferSize);
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
