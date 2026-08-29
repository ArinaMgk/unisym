// ASCII C/C++ TAB4 CRLF
// Docutitle: PNG Decoder Implementation (Deflate, Filtering & Color Reconstruction)
// Attribute: Env-Freestanding Non-Dependence
// Copyright: UNISYM

#include "../../../../inc/c/format/picture/PNG.h"
#include <stdlib.h>
#include <string.h>

namespace {

	static inline byte ClampByte(int val) {
		if (val < 0) return 0;
		if (val > 255) return 255;
		return (byte)val;
	}

	static inline byte PaethPredictor(int a, int b, int c) {
		int p = a + b - c;
		int pa = p > a ? p - a : a - p;
		int pb = p > b ? p - b : b - p;
		int pc = p > c ? p - c : c - p;
		if (pa <= pb && pa <= pc) return (byte)a;
		if (pb <= pc) return (byte)b;
		return (byte)c;
	}

	// ===== Deflate Huffman Table (Canonical LSB-first) =====
	struct DeflateHuffman {
		uint16 table[1024]; // Fast lookup or tree: (length << 10) | symbol
		uint16 mincode[17];
		uint16 maxcode[17];
		int16  valptr[17];
		uint16 symbols[320];
		bool   valid;
	};

	static bool BuildDeflateHuffman(DeflateHuffman& dh, const byte* codeLengths, int numSymbols) {
		byte bl_count[17] = { 0 };
		for (int i = 0; i < numSymbols; ++i) {
			if (codeLengths[i] > 0 && codeLengths[i] <= 16) {
				bl_count[codeLengths[i]]++;
			}
		}

		uint16 next_code[17] = { 0 };
		uint16 code = 0;
		for (int bits = 1; bits <= 16; ++bits) {
			code = (code + bl_count[bits - 1]) << 1;
			next_code[bits] = code;
		}

		for (int l = 1; l <= 16; ++l) {
			if (bl_count[l]) {
				dh.mincode[l] = next_code[l];
				dh.maxcode[l] = next_code[l] + bl_count[l] - 1;
			} else {
				dh.mincode[l] = 0xFFFF;
				dh.maxcode[l] = 0xFFFF;
			}
		}

		int p = 0;
		for (int bits = 1; bits <= 16; ++bits) {
			dh.valptr[bits] = (int16)p;
			for (int i = 0; i < numSymbols; ++i) {
				if (codeLengths[i] == bits) {
					dh.symbols[p++] = (uint16)i;
				}
			}
		}

		// Fast lookup table for codes <= 9 bits
		for (int i = 0; i < 1024; ++i) {
			dh.table[i] = 0xFFFF;
		}

		for (int i = 0; i < numSymbols; ++i) {
			int len = codeLengths[i];
			if (len > 0 && len <= 9) {
				uint16 c = next_code[len]++;
				// Reverse bit order for LSB-first decoding
				uint16 rev = 0;
				for (int b = 0; b < len; ++b) {
					rev = (rev << 1) | ((c >> b) & 1);
				}
				int step = 1 << len;
				for (int idx = rev; idx < 1024; idx += step) {
					dh.table[idx] = (uint16)((len << 10) | i);
				}
			} else if (len > 9) {
				next_code[len]++;
			}
		}

		dh.valid = true;
		return true;
	}

	// ===== Deflate Bitstream (LSB-first) =====
	struct DeflateBitStream {
		const byte* data;
		size_t size;
		size_t pos;
		uint32 bitBuf;
		int    bitsCount;

		void Init(const byte* pData, size_t len) {
			data = pData;
			size = len;
			pos = 0;
			bitBuf = 0;
			bitsCount = 0;
		}

		void FillBits(int n) {
			while (bitsCount < n) {
				if (pos >= size) {
					bitBuf |= (0 << bitsCount);
					bitsCount += 8;
					continue;
				}
				bitBuf |= ((uint32)data[pos++]) << bitsCount;
				bitsCount += 8;
			}
		}

		int ReadBits(int n) {
			if (n == 0) return 0;
			FillBits(n);
			int val = bitBuf & ((1 << n) - 1);
			bitBuf >>= n;
			bitsCount -= n;
			return val;
		}

		int PeekBits(int n) {
			FillBits(n);
			return bitBuf & ((1 << n) - 1);
		}

		void DropBits(int n) {
			bitBuf >>= n;
			bitsCount -= n;
		}

		void AlignToByte() {
			int drop = bitsCount % 8;
			if (drop > 0) {
				DropBits(drop);
			}
		}

		int DecodeSymbol(const DeflateHuffman& dh) {
			FillBits(9);
			int look = bitBuf & 0x01FF;
			uint16 entry = dh.table[look];
			if (entry != 0xFFFF) {
				int len = entry >> 10;
				DropBits(len);
				return entry & 0x03FF;
			}

			// Code > 9 bits: fallback bit-by-bit
			uint16 code = 0;
			for (int l = 1; l <= 16; ++l) {
				code = (code << 1) | ReadBits(1);
				if (dh.maxcode[l] != 0xFFFF && code <= dh.maxcode[l]) {
					int idx = dh.valptr[l] + (code - dh.mincode[l]);
					return dh.symbols[idx];
				}
			}
			return -1;
		}
	};

	// Deflate constant tables (RFC 1951)
	static const uint16 kBaseLength[29] = {
		3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258
	};
	static const byte kExtraLengthBits[29] = {
		0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0
	};
	static const uint16 kBaseDist[30] = {
		1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577
	};
	static const byte kExtraDistBits[30] = {
		0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13
	};
	static const byte kClenOrder[19] = {
		16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
	};

	// Deflate / zlib Inflate implementation
	static bool InflateData(const byte* inData, size_t inSize, byte* outData, size_t expectedOutSize) {
		if (!inData || inSize < 2 || !outData || expectedOutSize == 0) {
			return false;
		}

		// Verify zlib header (CMF/FLG)
		byte cmf = inData[0];
		byte flg = inData[1];
		if ((cmf & 0x0F) != 8 || ((cmf * 256 + flg) % 31) != 0) {
			return false;
		}

		size_t inPos = 2;
		if (flg & 0x20) {
			inPos += 4; // Skip preset dictionary ID
		}
		if (inPos >= inSize) return false;

		DeflateBitStream bs;
		bs.Init(inData + inPos, inSize - inPos);

		size_t outPos = 0;
		int bfinal = 0;

		DeflateHuffman fixedLitTree;
		DeflateHuffman fixedDistTree;
		bool fixedTreesBuilt = false;

		while (!bfinal) {
			bfinal = bs.ReadBits(1);
			int btype = bs.ReadBits(2);

			if (btype == 0) {
				// Stored / Uncompressed block
				bs.AlignToByte();
				uint16 len = (uint16)bs.ReadBits(16);
				uint16 nlen = (uint16)bs.ReadBits(16);
				if ((uint16)(len ^ 0xFFFF) != nlen) {
					return false;
				}
				if (outPos + len > expectedOutSize) {
					return false;
				}
				for (int i = 0; i < len; ++i) {
					outData[outPos++] = (byte)bs.ReadBits(8);
				}
			} else if (btype == 1 || btype == 2) {
				DeflateHuffman dynamicLitTree;
				DeflateHuffman dynamicDistTree;
				DeflateHuffman* pLitTree = nullptr;
				DeflateHuffman* pDistTree = nullptr;

				if (btype == 1) {
					// Fixed Huffman codes
					if (!fixedTreesBuilt) {
						byte litLengths[288];
						for (int i = 0; i <= 143; ++i) litLengths[i] = 8;
						for (int i = 144; i <= 255; ++i) litLengths[i] = 9;
						for (int i = 256; i <= 279; ++i) litLengths[i] = 7;
						for (int i = 280; i <= 287; ++i) litLengths[i] = 8;
						BuildDeflateHuffman(fixedLitTree, litLengths, 288);

						byte distLengths[32];
						for (int i = 0; i < 32; ++i) distLengths[i] = 5;
						BuildDeflateHuffman(fixedDistTree, distLengths, 32);
						fixedTreesBuilt = true;
					}
					pLitTree = &fixedLitTree;
					pDistTree = &fixedDistTree;
				} else {
					// Dynamic Huffman codes
					int hlit = bs.ReadBits(5) + 257;
					int hdist = bs.ReadBits(5) + 1;
					int hclen = bs.ReadBits(4) + 4;

					byte clenCodeLengths[19] = { 0 };
					for (int i = 0; i < hclen; ++i) {
						clenCodeLengths[kClenOrder[i]] = (byte)bs.ReadBits(3);
					}

					DeflateHuffman clenTree;
					BuildDeflateHuffman(clenTree, clenCodeLengths, 19);

					byte totalLengths[320] = { 0 };
					int totalSymbols = hlit + hdist;
					int cur = 0;
					while (cur < totalSymbols) {
						int sym = bs.DecodeSymbol(clenTree);
						if (sym < 0) return false;
						if (sym <= 15) {
							totalLengths[cur++] = (byte)sym;
						} else if (sym == 16) {
							if (cur == 0) return false;
							byte prev = totalLengths[cur - 1];
							int repeat = bs.ReadBits(2) + 3;
							while (repeat-- > 0 && cur < totalSymbols) {
								totalLengths[cur++] = prev;
							}
						} else if (sym == 17) {
							int repeat = bs.ReadBits(3) + 3;
							while (repeat-- > 0 && cur < totalSymbols) {
								totalLengths[cur++] = 0;
							}
						} else if (sym == 18) {
							int repeat = bs.ReadBits(7) + 11;
							while (repeat-- > 0 && cur < totalSymbols) {
								totalLengths[cur++] = 0;
							}
						}
					}

					BuildDeflateHuffman(dynamicLitTree, totalLengths, hlit);
					BuildDeflateHuffman(dynamicDistTree, totalLengths + hlit, hdist);
					pLitTree = &dynamicLitTree;
					pDistTree = &dynamicDistTree;
				}

				// Decompress block data
				while (true) {
					int sym = bs.DecodeSymbol(*pLitTree);
					if (sym < 0) return false;
					if (sym < 256) {
						if (outPos >= expectedOutSize) return false;
						outData[outPos++] = (byte)sym;
					} else if (sym == 256) {
						// End of Block
						break;
					} else {
						int lenIndex = sym - 257;
						if (lenIndex >= 29) return false;
						int matchLen = kBaseLength[lenIndex] + bs.ReadBits(kExtraLengthBits[lenIndex]);

						int distSym = bs.DecodeSymbol(*pDistTree);
						if (distSym < 0 || distSym >= 30) return false;
						int matchDist = kBaseDist[distSym] + bs.ReadBits(kExtraDistBits[distSym]);

						if (matchDist > (int)outPos || outPos + matchLen > expectedOutSize) {
							return false;
						}

						size_t srcPos = outPos - matchDist;
						for (int i = 0; i < matchLen; ++i) {
							outData[outPos++] = outData[srcPos + i];
						}
					}
				}
			} else {
				// Invalid block type
				return false;
			}
		}

		return outPos == expectedOutSize;
	}

}

// Decodes raw PNG buffer into a Color pixel array
uni::Color* DecodePNG(const byte* fileData, size_t fileSize, int* outWidth, int* outHeight) {
	if (!fileData || fileSize < 8) {
		return nullptr;
	}

	// Verify 8-byte PNG signature: \x89PNG\r\n\x1a\n
	static const byte kPngSig[8] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
	if (memcmp(fileData, kPngSig, 8) != 0) {
		return nullptr;
	}

	PNG_IHDR ihdr;
	memset(&ihdr, 0, sizeof(ihdr));
	bool hasIhdr = false;

	// Palette for indexed color
	byte palette[256 * 3];
	int paletteSize = 0;

	// Transparency info (tRNS)
	byte trnsData[256];
	size_t trnsSize = 0;
	bool hasTrns = false;

	// Dynamic accumulator buffer for concatenating IDAT chunk payloads
	byte* idatBuffer = nullptr;
	size_t idatCapacity = 0;
	size_t idatSize = 0;

	size_t pos = 8;
	while (pos + 8 <= fileSize) {
		uint32 chunkLen = *(const BigEndian<uint32, true>*)(fileData + pos);
		uint32 chunkType = *(const BigEndian<uint32, true>*)(fileData + pos + 4);
		pos += 8;

		if (pos + chunkLen + 4 > fileSize) {
			break;
		}

		const byte* chunkData = fileData + pos;

		if (chunkType == PNG_CHUNK_IHDR) {
			if (chunkLen >= 13) {
				const PNG_IHDR* ihdrPtr = (const PNG_IHDR*)chunkData;
				ihdr.width = ihdrPtr->width;
				ihdr.height = ihdrPtr->height;
				ihdr.bit_depth = ihdrPtr->bit_depth;
				ihdr.color_type = ihdrPtr->color_type;
				ihdr.compression_method = ihdrPtr->compression_method;
				ihdr.filter_method = ihdrPtr->filter_method;
				ihdr.interlace_method = ihdrPtr->interlace_method;
				if (ihdr.width > 0 && ihdr.height > 0 && ihdr.compression_method == 0 && ihdr.filter_method == 0) {
					hasIhdr = true;
				}
			}
		} else if (chunkType == PNG_CHUNK_PLTE) {
			paletteSize = (int)(chunkLen / 3);
			if (paletteSize > 256) paletteSize = 256;
			memcpy(palette, chunkData, paletteSize * 3);
		} else if (chunkType == PNG_CHUNK_tRNS) {
			trnsSize = chunkLen > 256 ? 256 : chunkLen;
			memcpy(trnsData, chunkData, trnsSize);
			hasTrns = true;
		} else if (chunkType == PNG_CHUNK_IDAT) {
			if (chunkLen > 0) {
				if (idatSize + chunkLen > idatCapacity) {
					size_t newCap = idatCapacity ? idatCapacity * 2 : 65536;
					while (newCap < idatSize + chunkLen) newCap *= 2;
					byte* newBuf = (byte*)realloc(idatBuffer, newCap);
					if (!newBuf) {
						if (idatBuffer) free(idatBuffer);
						return nullptr;
					}
					idatBuffer = newBuf;
					idatCapacity = newCap;
				}
				memcpy(idatBuffer + idatSize, chunkData, chunkLen);
				idatSize += chunkLen;
			}
		} else if (chunkType == PNG_CHUNK_IEND) {
			break;
		}

		pos += chunkLen + 4; // Skip chunk data and 4-byte CRC
	}

	if (!hasIhdr || idatSize == 0 || !idatBuffer) {
		if (idatBuffer) free(idatBuffer);
		return nullptr;
	}

	// Calculate channels and bytes per pixel
	int channels = 1;
	switch ((PNGColorType)ihdr.color_type) {
	case PNGColorType::GRAYSCALE:       channels = 1; break;
	case PNGColorType::RGB:             channels = 3; break;
	case PNGColorType::INDEXED:         channels = 1; break;
	case PNGColorType::GRAYSCALE_ALPHA: channels = 2; break;
	case PNGColorType::RGBA:            channels = 4; break;
	default:
		free(idatBuffer);
		return nullptr;
	}

	// Line bytes excluding filter type byte
	size_t rowBytes = ((size_t)ihdr.width * channels * ihdr.bit_depth + 7) / 8;
	size_t rawLineStride = 1 + rowBytes;
	size_t rawTotalBytes = rawLineStride * ihdr.height;

	byte* rawInflated = (byte*)malloc(rawTotalBytes);
	if (!rawInflated) {
		free(idatBuffer);
		return nullptr;
	}

	// Decompress IDAT stream with Inflate
	bool inflateOk = InflateData(idatBuffer, idatSize, rawInflated, rawTotalBytes);
	free(idatBuffer);

	if (!inflateOk) {
		free(rawInflated);
		return nullptr;
	}

	// Allocate unfiltered rows buffer
	byte* unfiltered = (byte*)malloc(rowBytes * ihdr.height);
	if (!unfiltered) {
		free(rawInflated);
		return nullptr;
	}

	int bpp = (channels * ihdr.bit_depth + 7) / 8;
	if (bpp < 1) bpp = 1;

	// Unfilter each scanline (None, Sub, Up, Average, Paeth)
	for (uint32 y = 0; y < ihdr.height; ++y) {
		const byte* rawRow = rawInflated + y * rawLineStride;
		byte filterType = rawRow[0];
		const byte* src = rawRow + 1;
		byte* dst = unfiltered + y * rowBytes;
		const byte* prev = (y > 0) ? (unfiltered + (y - 1) * rowBytes) : nullptr;

		switch ((PNGFilterType)filterType) {
		case PNGFilterType::NONE:
			memcpy(dst, src, rowBytes);
			break;

		case PNGFilterType::SUB:
			for (size_t i = 0; i < rowBytes; ++i) {
				byte a = (i >= (size_t)bpp) ? dst[i - bpp] : 0;
				dst[i] = (byte)(src[i] + a);
			}
			break;

		case PNGFilterType::UP:
			for (size_t i = 0; i < rowBytes; ++i) {
				byte bVal = prev ? prev[i] : 0;
				dst[i] = (byte)(src[i] + bVal);
			}
			break;

		case PNGFilterType::AVERAGE:
			for (size_t i = 0; i < rowBytes; ++i) {
				int a = (i >= (size_t)bpp) ? dst[i - bpp] : 0;
				int bVal = prev ? prev[i] : 0;
				dst[i] = (byte)(src[i] + ((a + bVal) >> 1));
			}
			break;

		case PNGFilterType::PAETH:
			for (size_t i = 0; i < rowBytes; ++i) {
				int a = (i >= (size_t)bpp) ? dst[i - bpp] : 0;
				int bVal = prev ? prev[i] : 0;
				int cVal = (prev && i >= (size_t)bpp) ? prev[i - bpp] : 0;
				dst[i] = (byte)(src[i] + PaethPredictor(a, bVal, cVal));
			}
			break;

		default:
			// Invalid filter type fallback
			memcpy(dst, src, rowBytes);
			break;
		}
	}
	free(rawInflated);

	// Output pixels array allocation
	uni::Color* pixels = (uni::Color*)malloc((size_t)ihdr.width * (size_t)ihdr.height * sizeof(uni::Color));
	if (!pixels) {
		free(unfiltered);
		return nullptr;
	}

	// Color conversion to uni::Color (b, g, r, a)
	for (uint32 y = 0; y < ihdr.height; ++y) {
		const byte* row = unfiltered + y * rowBytes;
		uni::Color* outRow = pixels + y * ihdr.width;

		if (ihdr.color_type == (byte)PNGColorType::RGBA && ihdr.bit_depth == 8) {
			for (uint32 x = 0; x < ihdr.width; ++x) {
				outRow[x].r = row[x * 4 + 0];
				outRow[x].g = row[x * 4 + 1];
				outRow[x].b = row[x * 4 + 2];
				outRow[x].a = row[x * 4 + 3];
			}
		} else if (ihdr.color_type == (byte)PNGColorType::RGB && ihdr.bit_depth == 8) {
			for (uint32 x = 0; x < ihdr.width; ++x) {
				outRow[x].r = row[x * 3 + 0];
				outRow[x].g = row[x * 3 + 1];
				outRow[x].b = row[x * 3 + 2];
				outRow[x].a = 0xFF;
				if (hasTrns && trnsSize >= 6) {
					if (row[x * 3 + 0] == trnsData[1] &&
						row[x * 3 + 1] == trnsData[3] &&
						row[x * 3 + 2] == trnsData[5]) {
						outRow[x].a = 0x00;
					}
				}
			}
		} else if (ihdr.color_type == (byte)PNGColorType::INDEXED) {
			for (uint32 x = 0; x < ihdr.width; ++x) {
				byte idx = 0;
				if (ihdr.bit_depth == 8) {
					idx = row[x];
				} else if (ihdr.bit_depth == 4) {
					idx = (row[x / 2] >> ((1 - (x % 2)) * 4)) & 0x0F;
				} else if (ihdr.bit_depth == 2) {
					idx = (row[x / 4] >> ((3 - (x % 4)) * 2)) & 0x03;
				} else if (ihdr.bit_depth == 1) {
					idx = (row[x / 8] >> (7 - (x % 8))) & 0x01;
				}
				if (idx < paletteSize) {
					outRow[x].r = palette[idx * 3 + 0];
					outRow[x].g = palette[idx * 3 + 1];
					outRow[x].b = palette[idx * 3 + 2];
					outRow[x].a = (hasTrns && idx < trnsSize) ? trnsData[idx] : 0xFF;
				} else {
					outRow[x].r = outRow[x].g = outRow[x].b = 0;
					outRow[x].a = 0xFF;
				}
			}
		} else if (ihdr.color_type == (byte)PNGColorType::GRAYSCALE && ihdr.bit_depth == 8) {
			for (uint32 x = 0; x < ihdr.width; ++x) {
				byte gVal = row[x];
				outRow[x].r = outRow[x].g = outRow[x].b = gVal;
				outRow[x].a = (hasTrns && trnsSize >= 2 && gVal == trnsData[1]) ? 0x00 : 0xFF;
			}
		} else if (ihdr.color_type == (byte)PNGColorType::GRAYSCALE_ALPHA && ihdr.bit_depth == 8) {
			for (uint32 x = 0; x < ihdr.width; ++x) {
				byte gVal = row[x * 2 + 0];
				outRow[x].r = outRow[x].g = outRow[x].b = gVal;
				outRow[x].a = row[x * 2 + 1];
			}
		} else if (ihdr.bit_depth == 16) {
			// 16-bit depth downscaled to 8-bit
			for (uint32 x = 0; x < ihdr.width; ++x) {
				if (ihdr.color_type == (byte)PNGColorType::RGBA) {
					outRow[x].r = row[x * 8 + 0];
					outRow[x].g = row[x * 8 + 2];
					outRow[x].b = row[x * 8 + 4];
					outRow[x].a = row[x * 8 + 6];
				} else if (ihdr.color_type == (byte)PNGColorType::RGB) {
					outRow[x].r = row[x * 6 + 0];
					outRow[x].g = row[x * 6 + 2];
					outRow[x].b = row[x * 6 + 4];
					outRow[x].a = 0xFF;
				} else if (ihdr.color_type == (byte)PNGColorType::GRAYSCALE) {
					byte gVal = row[x * 2 + 0];
					outRow[x].r = outRow[x].g = outRow[x].b = gVal;
					outRow[x].a = 0xFF;
				}
			}
		}
	}

	free(unfiltered);

	if (outWidth) *outWidth = (int)ihdr.width;
	if (outHeight) *outHeight = (int)ihdr.height;

	return pixels;
}

const char* uni::PNGCodec::GetName() const {
	return "PNG";
}

uni::ImageFormat uni::PNGCodec::GetFormat() const {
	return uni::ImageFormat::PNG;
}

const char* const* uni::PNGCodec::GetExtensions() const {
	static const char* const extensions[] = { "png", nullptr };
	return extensions;
}

uni::ImageResult uni::PNGCodec::Probe(StorageTrait& storage, bool& matched) const {
	matched = false;
	byte sig[8];
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

	stduint readBytes = storage.Read(0, sig, 8, blockBuf);
	if (isDyn) free(blockBuf);

	static const byte kPngSig[8] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
	if (readBytes == 8 && memcmp(sig, kPngSig, 8) == 0) {
		matched = true;
	}

	return uni::ImageResult::OK;
}

uni::ImageResult uni::PNGCodec::ReadInfo(StorageTrait& storage, ImageInfo& outInfo) const {
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

	byte headerBuf[32];
	stduint readBytes = storage.Read(0, headerBuf, 32, blockBuf);
	if (isDyn) free(blockBuf);

	if (readBytes < 29) {
		return uni::ImageResult::INVALID_FORMAT;
	}

	uint32 chunkType = *(const BigEndian<uint32, true>*)(headerBuf + 12);
	if (chunkType != PNG_CHUNK_IHDR) {
		return uni::ImageResult::INVALID_FORMAT;
	}

	const PNG_IHDR* ihdr = (const PNG_IHDR*)(headerBuf + 16);
	uint32 width = ihdr->width;
	uint32 height = ihdr->height;
	byte bitDepth = ihdr->bit_depth;
	byte colorType = ihdr->color_type;

	outInfo.width = width;
	outInfo.height = height;
	outInfo.format = PixelFormat::ARGB8888;
	outInfo.colorSpace = ColorSpace::SRGB;
	outInfo.alphaMode = (colorType == 4 || colorType == 6) ? ImageAlphaMode::STRAIGHT : ImageAlphaMode::NONE;
	outInfo.fileFormat = ImageFormat::PNG;
	outInfo.bitsPerPixel = (colorType == 6 ? 4 : (colorType == 2 ? 3 : 1)) * bitDepth;
	outInfo.frameCount = 1;
	outInfo.hasAlpha = (colorType == 4 || colorType == 6);
	outInfo.hasAnimation = false;

	return uni::ImageResult::OK;
}

uni::ImageResult uni::PNGCodec::OpenSurface(
	StorageTrait& storage,
	IImageSurface*& outSurface,
	trait::Malloc& allocator,
	const ImageDecodeOptions& options,
	ImageAccessMode access
) const {
	return uni::ImageResult::UNSUPPORTED;
}

uni::ImageResult uni::PNGCodec::Decode(
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
	if (maxStorageSize < 8) return uni::ImageResult::INVALID_FORMAT;

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

	int outWidth = 0;
	int outHeight = 0;
	uni::Color* stdPixels = DecodePNG(fileData, totalRead, &outWidth, &outHeight);
	free(fileData);

	if (!stdPixels) {
		return uni::ImageResult::FAILED;
	}

	size_t pixelBufferSize = (size_t)outWidth * (size_t)outHeight * sizeof(uni::Color);
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
	outBuffer.alphaMode = ImageAlphaMode::STRAIGHT;
	outBuffer.pixels = targetPixels;
	outBuffer.size = pixelBufferSize;
	outBuffer.allocator = &allocator;

	return uni::ImageResult::OK;
}

uni::ImageResult uni::PNGCodec::Encode(
	const ImageBuffer& image,
	StorageTrait& storage,
	trait::Malloc& allocator,
	const ImageEncodeOptions& options
) const {
	return uni::ImageResult::UNSUPPORTED;
}

bool uni::PNGCodec::CanEncode(PixelFormat format) const {
	return false;
}
