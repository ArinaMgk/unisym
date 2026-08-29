// ASCII C/C++ TAB4 CRLF
// Docutitle: GIF Decoder Implementation (LZW, Multi-Frame & Animation Compositing)
// Attribute: Env-Freestanding Non-Dependence
// Copyright: UNISYM

#include "../../../../inc/c/format/picture/GIF.h"
#include <stdlib.h>
#include <string.h>

namespace {

	// LZW bitstream reader over GIF data sub-blocks (LSB-first)
	struct GIFBitStream {
		const byte* data;
		size_t size;
		size_t pos;
		uint32 bitBuf;
		int    bitsCount;
		byte   subBlockRemain;

		void Init(const byte* pData, size_t len, size_t startOffset) {
			data = pData;
			size = len;
			pos = startOffset;
			bitBuf = 0;
			bitsCount = 0;
			subBlockRemain = 0;
		}

		bool FillBits(int n) {
			while (bitsCount < n) {
				if (subBlockRemain == 0) {
					if (pos >= size) return false;
					subBlockRemain = data[pos++];
					if (subBlockRemain == 0) {
						// Block terminator (0x00)
						bitBuf |= (0 << bitsCount);
						bitsCount += 8;
						return true;
					}
				}
				if (pos >= size) return false;
				byte b = data[pos++];
				subBlockRemain--;
				bitBuf |= ((uint32)b) << bitsCount;
				bitsCount += 8;
			}
			return true;
		}

		int ReadBits(int n) {
			if (n == 0) return 0;
			if (!FillBits(n)) return -1;
			int val = bitBuf & ((1 << n) - 1);
			bitBuf >>= n;
			bitsCount -= n;
			return val;
		}

		void SkipRemainingSubBlocks() {
			while (subBlockRemain > 0 && pos < size) {
				pos += subBlockRemain;
				subBlockRemain = 0;
				if (pos < size) {
					subBlockRemain = data[pos++];
				}
			}
		}
	};

	// Decodes GIF LZW compressed image pixel stream
	static bool DecodeGIF_LZW(const byte* fileData, size_t fileSize, size_t& pos,
							  byte* outPixels, size_t expectedPixelCount) {
		if (pos >= fileSize) return false;
		byte minCodeSize = fileData[pos++];
		if (minCodeSize < 2 || minCodeSize > 11) return false;

		int clearCode = 1 << minCodeSize;
		int eoiCode = clearCode + 1;
		int codeSize = minCodeSize + 1;
		int maxCode = 1 << codeSize;
		int nextCode = eoiCode + 1;

		int16 prefix[4096];
		byte  suffix[4096];
		byte  stack[4096];
		int   stackPtr = 0;

		for (int i = 0; i < clearCode; ++i) {
			prefix[i] = -1;
			suffix[i] = (byte)i;
		}

		GIFBitStream bs;
		bs.Init(fileData, fileSize, pos);

		size_t outIndex = 0;
		int prevCode = -1;
		int firstChar = 0;

		while (outIndex < expectedPixelCount) {
			int code = bs.ReadBits(codeSize);
			if (code < 0 || code == eoiCode) {
				break;
			}

			if (code == clearCode) {
				codeSize = minCodeSize + 1;
				maxCode = 1 << codeSize;
				nextCode = eoiCode + 1;
				prevCode = -1;
				continue;
			}

			int inCode = code;
			if (code >= nextCode) {
				if (code > nextCode || prevCode < 0) {
					bs.SkipRemainingSubBlocks();
					pos = bs.pos;
					return false;
				}
				stack[stackPtr++] = (byte)firstChar;
				code = prevCode;
			}

			while (code >= clearCode && code < 4096) {
				stack[stackPtr++] = suffix[code];
				code = prefix[code];
			}
			firstChar = suffix[code];
			stack[stackPtr++] = (byte)firstChar;

			if (prevCode >= 0 && nextCode < 4096) {
				prefix[nextCode] = (int16)prevCode;
				suffix[nextCode] = (byte)firstChar;
				nextCode++;
				if (nextCode >= maxCode && codeSize < 12) {
					codeSize++;
					maxCode = 1 << codeSize;
				}
			}
			prevCode = inCode;

			while (stackPtr > 0 && outIndex < expectedPixelCount) {
				outPixels[outIndex++] = stack[--stackPtr];
			}
		}

		bs.SkipRemainingSubBlocks();
		pos = bs.pos;
		return true;
	}

}

// Decodes a single frame (frame 0) of a raw GIF buffer into a Color pixel array
uni::Color* DecodeGIF(const byte* fileData, size_t fileSize, int* outWidth, int* outHeight) {
	GIFAnimation anim;
	if (!DecodeGIFAnimation(fileData, fileSize, anim) || anim.frameCount == 0 || !anim.frames) {
		return nullptr;
	}

	uni::Color* firstFramePixels = anim.frames[0].pixels;
	if (outWidth)  *outWidth = (int)anim.width;
	if (outHeight) *outHeight = (int)anim.height;

	// Detach frame 0 pixels so FreeGIFAnimation won't free it
	anim.frames[0].pixels = nullptr;
	FreeGIFAnimation(anim);

	return firstFramePixels;
}

// Decodes all frames of a GIF image for animation playback
bool DecodeGIFAnimation(const byte* fileData, size_t fileSize, GIFAnimation& outAnim) {
	outAnim.width = 0;
	outAnim.height = 0;
	outAnim.frameCount = 0;
	outAnim.frames = nullptr;

	if (!fileData || fileSize < 13) {
		return false;
	}

	// Verify GIF header: "GIF87a" or "GIF89a"
	if (memcmp(fileData, "GIF87a", 6) != 0 && memcmp(fileData, "GIF89a", 6) != 0) {
		return false;
	}

	const GIF_SCREEN_DESCRIPTOR* screen = (const GIF_SCREEN_DESCRIPTOR*)(fileData + 6);
	uint16 screenWidth = screen->width;
	uint16 screenHeight = screen->height;
	byte screenPacked = screen->packed_fields;
	byte bgIndex = screen->bg_color_index;

	if (screenWidth == 0 || screenHeight == 0) {
		return false;
	}

	bool hasGct = (screenPacked & 0x80) != 0;
	int gctSize = 1 << ((screenPacked & 0x07) + 1);

	byte globalPalette[256 * 3];
	memset(globalPalette, 0, sizeof(globalPalette));

	size_t pos = 13;
	if (hasGct) {
		size_t gctBytes = (size_t)gctSize * 3;
		if (pos + gctBytes > fileSize) return false;
		memcpy(globalPalette, fileData + pos, gctBytes);
		pos += gctBytes;
	}

	// Dynamic frame array
	size_t frameCapacity = 8;
	GIFFrame* frameList = (GIFFrame*)malloc(frameCapacity * sizeof(GIFFrame));
	if (!frameList) return false;
	size_t frameCount = 0;

	// Master canvas for animation compositing
	size_t canvasPixelCount = (size_t)screenWidth * (size_t)screenHeight;
	uni::Color* canvas = (uni::Color*)malloc(canvasPixelCount * sizeof(uni::Color));
	uni::Color* prevBackup = (uni::Color*)malloc(canvasPixelCount * sizeof(uni::Color));

	if (!canvas || !prevBackup) {
		if (canvas) free(canvas);
		if (prevBackup) free(prevBackup);
		free(frameList);
		return false;
	}

	// Initialize canvas background
	uni::Color bgColor(0);
	if (hasGct && bgIndex < gctSize) {
		bgColor.r = globalPalette[bgIndex * 3 + 0];
		bgColor.g = globalPalette[bgIndex * 3 + 1];
		bgColor.b = globalPalette[bgIndex * 3 + 2];
		bgColor.a = 0xFF;
	}
	for (size_t i = 0; i < canvasPixelCount; ++i) {
		canvas[i] = bgColor;
	}

	// Graphic Control Extension state
	GIFDisposalMethod disposalMethod = GIFDisposalMethod::UNSPECIFIED;
	GIFDisposalMethod prevDisposal = GIFDisposalMethod::UNSPECIFIED;
	uint16 prevLeft = 0, prevTop = 0, prevWidth = 0, prevHeight = 0;
	bool   hasTrans = false;
	byte   transIndex = 0;
	uint32 delayMs = 100; // Default 100ms

	while (pos < fileSize) {
		byte intro = fileData[pos++];
		if (intro == GIF_TRAILER) {
			break;
		}

		if (intro == GIF_INTRO_EXTENSION) {
			if (pos >= fileSize) break;
			byte label = fileData[pos++];
			if (label == GIF_EXT_GRAPHIC_CONTROL) {
				if (pos >= fileSize) break;
				byte blockSize = fileData[pos++];
				if (pos + blockSize > fileSize) break;
				if (blockSize >= 4) {
					byte packed = fileData[pos + 0];
					LitEndian<uint16, true> delayUnits = *(const LitEndian<uint16, true>*)(fileData + pos + 1);
					transIndex = fileData[pos + 3];
					disposalMethod = (GIFDisposalMethod)((packed >> 2) & 0x07);
					hasTrans = (packed & 0x01) != 0;
					delayMs = ((uint16)delayUnits > 0) ? ((uint32)(uint16)delayUnits * 10) : 100;
				}
				pos += blockSize;
				if (pos < fileSize && fileData[pos] == 0x00) pos++; // Terminator
			} else {
				// Skip other extensions (Application, Comment, etc.)
				while (pos < fileSize) {
					byte subLen = fileData[pos++];
					if (subLen == 0) break;
					pos += subLen;
				}
			}
		} else if (intro == GIF_INTRO_IMAGE) {
			if (pos + 9 > fileSize) break;
			const GIF_IMAGE_DESCRIPTOR* imgDesc = (const GIF_IMAGE_DESCRIPTOR*)(fileData + pos);
			uint16 imgLeft = imgDesc->left;
			uint16 imgTop = imgDesc->top;
			uint16 imgWidth = imgDesc->width;
			uint16 imgHeight = imgDesc->height;
			byte imgPacked = imgDesc->packed_fields;
			pos += 9;

			bool hasLct = (imgPacked & 0x80) != 0;
			bool interlace = (imgPacked & 0x40) != 0;
			int lctSize = 1 << ((imgPacked & 0x07) + 1);

			byte localPalette[256 * 3];
			const byte* activePalette = globalPalette;
			int activePaletteSize = gctSize;

			if (hasLct) {
				size_t lctBytes = (size_t)lctSize * 3;
				if (pos + lctBytes > fileSize) break;
				memcpy(localPalette, fileData + pos, lctBytes);
				pos += lctBytes;
				activePalette = localPalette;
				activePaletteSize = lctSize;
			}

			// Apply previous frame's disposal method
			if (prevDisposal == GIFDisposalMethod::RESTORE_BG) {
				for (uint16 y = 0; y < prevHeight; ++y) {
					for (uint16 x = 0; x < prevWidth; ++x) {
						uint16 cy = prevTop + y;
						uint16 cx = prevLeft + x;
						if (cy < screenHeight && cx < screenWidth) {
							canvas[cy * screenWidth + cx] = bgColor;
						}
					}
				}
			} else if (prevDisposal == GIFDisposalMethod::RESTORE_PREV) {
				memcpy(canvas, prevBackup, canvasPixelCount * sizeof(uni::Color));
			}

			// If current frame requests RESTORE_PREV, save canvas state before drawing
			if (disposalMethod == GIFDisposalMethod::RESTORE_PREV) {
				memcpy(prevBackup, canvas, canvasPixelCount * sizeof(uni::Color));
			}

			// Decode LZW pixel indices for this image
			size_t imgPixelCount = (size_t)imgWidth * (size_t)imgHeight;
			byte* decodedIndices = (byte*)malloc(imgPixelCount);
			if (!decodedIndices) break;

			bool lzwOk = DecodeGIF_LZW(fileData, fileSize, pos, decodedIndices, imgPixelCount);
			if (!lzwOk) {
				free(decodedIndices);
				break;
			}

			// Render decoded image onto canvas
			static const int kPassStarts[4] = { 0, 4, 2, 1 };
			static const int kPassSteps[4]  = { 8, 8, 4, 2 };

			if (interlace) {
				size_t srcIdx = 0;
				for (int pass = 0; pass < 4; ++pass) {
					for (int y = kPassStarts[pass]; y < imgHeight; y += kPassSteps[pass]) {
						for (int x = 0; x < imgWidth; ++x) {
							byte idx = decodedIndices[srcIdx++];
							if (!hasTrans || idx != transIndex) {
								uint16 cy = imgTop + y;
								uint16 cx = imgLeft + x;
								if (cy < screenHeight && cx < screenWidth) {
									uni::Color c;
									c.r = activePalette[idx * 3 + 0];
									c.g = activePalette[idx * 3 + 1];
									c.b = activePalette[idx * 3 + 2];
									c.a = 0xFF;
									canvas[cy * screenWidth + cx] = c;
								}
							}
						}
					}
				}
			} else {
				for (uint16 y = 0; y < imgHeight; ++y) {
					for (uint16 x = 0; x < imgWidth; ++x) {
						byte idx = decodedIndices[y * imgWidth + x];
						if (!hasTrans || idx != transIndex) {
							uint16 cy = imgTop + y;
							uint16 cx = imgLeft + x;
							if (cy < screenHeight && cx < screenWidth) {
								uni::Color c;
								c.r = activePalette[idx * 3 + 0];
								c.g = activePalette[idx * 3 + 1];
								c.b = activePalette[idx * 3 + 2];
								c.a = 0xFF;
								canvas[cy * screenWidth + cx] = c;
							}
						}
					}
				}
			}
			free(decodedIndices);

			// Copy current canvas snapshot into frame array
			uni::Color* framePixels = (uni::Color*)malloc(canvasPixelCount * sizeof(uni::Color));
			if (framePixels) {
				memcpy(framePixels, canvas, canvasPixelCount * sizeof(uni::Color));
				if (frameCount >= frameCapacity) {
					frameCapacity *= 2;
					frameList = (GIFFrame*)realloc(frameList, frameCapacity * sizeof(GIFFrame));
				}
				frameList[frameCount].pixels = framePixels;
				frameList[frameCount].delayMs = delayMs;
				frameCount++;
			}

			// Update state for next frame's disposal
			prevDisposal = disposalMethod;
			prevLeft = imgLeft;
			prevTop = imgTop;
			prevWidth = imgWidth;
			prevHeight = imgHeight;

			// Reset control state for next image
			disposalMethod = GIFDisposalMethod::UNSPECIFIED;
			hasTrans = false;
			delayMs = 100;
		}
	}

	free(canvas);
	free(prevBackup);

	if (frameCount == 0) {
		free(frameList);
		return false;
	}

	outAnim.width = screenWidth;
	outAnim.height = screenHeight;
	outAnim.frameCount = (uint32)frameCount;
	outAnim.frames = frameList;
	return true;
}

// Frees all resources associated with a GIFAnimation structure
void FreeGIFAnimation(GIFAnimation& anim) {
	if (anim.frames) {
		for (uint32 i = 0; i < anim.frameCount; ++i) {
			if (anim.frames[i].pixels) {
				free(anim.frames[i].pixels);
			}
		}
		free(anim.frames);
		anim.frames = nullptr;
	}
	anim.frameCount = 0;
	anim.width = 0;
	anim.height = 0;
}

const char* uni::GIFCodec::GetName() const {
	return "GIF";
}

uni::ImageFormat uni::GIFCodec::GetFormat() const {
	return uni::ImageFormat::GIF;
}

const char* const* uni::GIFCodec::GetExtensions() const {
	static const char* const extensions[] = { "gif", nullptr };
	return extensions;
}

uni::ImageResult uni::GIFCodec::Probe(StorageTrait& storage, bool& matched) const {
	matched = false;
	byte sig[6];
	stduint blockSize = storage.Block_Size ? storage.Block_Size : 512;

	byte* blockBuf = nullptr;
	bool isDyn = false;
	byte stackBuf[2048];
	if (blockSize <= 2048) {
		blockBuf = stackBuf;
	} else {
		blockBuf = (byte*)malloc(blockSize);
		if (!blockBuf) {
			return uni::ImageResult::OUT_OF_MEMORY;
		}
		isDyn = true;
	}

	stduint readBytes = storage.Read(0, sig, 6, blockBuf);
	if (isDyn) free(blockBuf);

	if (readBytes == 6 && (memcmp(sig, "GIF87a", 6) == 0 || memcmp(sig, "GIF89a", 6) == 0)) {
		matched = true;
	}

	return uni::ImageResult::OK;
}

uni::ImageResult uni::GIFCodec::ReadInfo(StorageTrait& storage, ImageInfo& outInfo) const {
	bool matched = false;
	uni::ImageResult res = Probe(storage, matched);
	if (res != uni::ImageResult::OK) return res;
	if (!matched) return uni::ImageResult::INVALID_FORMAT;

	stduint blockSize = storage.Block_Size ? storage.Block_Size : 512;
	byte* blockBuf = nullptr;
	bool isDyn = false;
	byte stackBuf[2048];
	if (blockSize <= 2048) {
		blockBuf = stackBuf;
	} else {
		blockBuf = (byte*)malloc(blockSize);
		if (!blockBuf) return uni::ImageResult::OUT_OF_MEMORY;
		isDyn = true;
	}

	byte headerBuf[13];
	stduint readBytes = storage.Read(0, headerBuf, 13, blockBuf);
	if (isDyn) free(blockBuf);

	if (readBytes < 13) {
		return uni::ImageResult::INVALID_FORMAT;
	}

	const GIF_SCREEN_DESCRIPTOR* screen = (const GIF_SCREEN_DESCRIPTOR*)(headerBuf + 6);
	uint16 width = screen->width;
	uint16 height = screen->height;

	// Quickly count total frames across entire file
	stduint maxStorageSize = storage.getUnits() * storage.Block_Size;
	byte* scanBuf = (byte*)malloc(maxStorageSize);
	uint32 totalFrames = 1;
	if (scanBuf) {
		stduint totalRead = storage.Read(0, scanBuf, maxStorageSize, blockBuf);
		uint32 frameCounter = 0;
		for (stduint i = 13; i + 9 <= totalRead; ++i) {
			if (scanBuf[i] == GIF_INTRO_IMAGE) {
				frameCounter++;
			}
		}
		if (frameCounter > 0) totalFrames = frameCounter;
		free(scanBuf);
	}

	outInfo.width = width;
	outInfo.height = height;
	outInfo.format = PixelFormat::ARGB8888;
	outInfo.colorSpace = ColorSpace::SRGB;
	outInfo.alphaMode = ImageAlphaMode::STRAIGHT;
	outInfo.fileFormat = ImageFormat::GIF;
	outInfo.bitsPerPixel = 8;
	outInfo.frameCount = totalFrames;
	outInfo.hasAlpha = true;
	outInfo.hasAnimation = (totalFrames > 1);

	return uni::ImageResult::OK;
}

uni::ImageResult uni::GIFCodec::OpenSurface(
	StorageTrait& storage,
	IImageSurface*& outSurface,
	trait::Malloc& allocator,
	const ImageDecodeOptions& options,
	ImageAccessMode access
) const {
	return uni::ImageResult::UNSUPPORTED;
}

uni::ImageResult uni::GIFCodec::Decode(
	StorageTrait& storage,
	ImageBuffer& outBuffer,
	trait::Malloc& allocator,
	const ImageDecodeOptions& options
) const {
	bool matched = false;
	uni::ImageResult res = Probe(storage, matched);
	if (res != uni::ImageResult::OK) return res;
	if (!matched) return uni::ImageResult::INVALID_FORMAT;

	stduint maxStorageSize = storage.getUnits() * storage.Block_Size;
	if (maxStorageSize < 13) return uni::ImageResult::INVALID_FORMAT;

	stduint blockSize = storage.Block_Size ? storage.Block_Size : 512;
	byte* blockBuf = nullptr;
	bool isDyn = false;
	byte stackBuf[2048];
	if (blockSize <= 2048) {
		blockBuf = stackBuf;
	} else {
		blockBuf = (byte*)malloc(blockSize);
		if (!blockBuf) return uni::ImageResult::OUT_OF_MEMORY;
		isDyn = true;
	}

	byte* fileData = (byte*)malloc(maxStorageSize);
	if (!fileData) {
		if (isDyn) free(blockBuf);
		return uni::ImageResult::OUT_OF_MEMORY;
	}

	stduint totalRead = storage.Read(0, fileData, maxStorageSize, blockBuf);
	if (isDyn) free(blockBuf);

	GIFAnimation anim;
	if (!DecodeGIFAnimation(fileData, totalRead, anim) || anim.frameCount == 0 || !anim.frames) {
		free(fileData);
		return uni::ImageResult::FAILED;
	}
	free(fileData);

	uint32 targetIdx = options.targetFrame < anim.frameCount ? options.targetFrame : 0;
	uni::Color* targetPixelsSrc = anim.frames[targetIdx].pixels;

	size_t pixelBufferSize = (size_t)anim.width * (size_t)anim.height * sizeof(uni::Color);
	void* targetPixels = allocator.allocate(pixelBufferSize);
	if (!targetPixels) {
		FreeGIFAnimation(anim);
		return uni::ImageResult::OUT_OF_MEMORY;
	}

	memcpy(targetPixels, targetPixelsSrc, pixelBufferSize);

	outBuffer.width = anim.width;
	outBuffer.height = anim.height;
	outBuffer.stride = anim.width * sizeof(uni::Color);
	outBuffer.format = PixelFormat::ARGB8888;
	outBuffer.colorSpace = ColorSpace::SRGB;
	outBuffer.alphaMode = ImageAlphaMode::STRAIGHT;
	outBuffer.pixels = targetPixels;
	outBuffer.size = pixelBufferSize;
	outBuffer.allocator = &allocator;

	FreeGIFAnimation(anim);
	return uni::ImageResult::OK;
}

uni::ImageResult uni::GIFCodec::Encode(
	const ImageBuffer& image,
	StorageTrait& storage,
	trait::Malloc& allocator,
	const ImageEncodeOptions& options
) const {
	return uni::ImageResult::UNSUPPORTED;
}

bool uni::GIFCodec::CanEncode(PixelFormat format) const {
	return false;
}
