// ASCII C/C++ TAB4 CRLF
// Docutitle: PNG Decoder Implementation (Deflate, Filtering & Color Reconstruction)
// Attribute: Env-Freestanding Non-Dependence
// Copyright: UNISYM

#include "../../../../inc/c/format/picture/PNG.h"
#include "../../../../inc/cpp/endian"

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

		struct InflateWorkingTrees {
			DeflateHuffman fixedLitTree;
			DeflateHuffman fixedDistTree;
			DeflateHuffman dynamicLitTree;
			DeflateHuffman dynamicDistTree;
			DeflateHuffman clenTree;
		};
		InflateWorkingTrees* trees = (InflateWorkingTrees*)malloc(sizeof(InflateWorkingTrees));
		if (!trees) return false;
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
					free(trees);
					return false;
				}
				if (outPos + len > expectedOutSize) {
					free(trees);
					return false;
				}
				for (int i = 0; i < len; ++i) {
					outData[outPos++] = (byte)bs.ReadBits(8);
				}
			} else if (btype == 1 || btype == 2) {
				
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
						BuildDeflateHuffman(trees->fixedLitTree, litLengths, 288);

						byte distLengths[32];
						for (int i = 0; i < 32; ++i) distLengths[i] = 5;
						BuildDeflateHuffman(trees->fixedDistTree, distLengths, 32);
						fixedTreesBuilt = true;
					}
					pLitTree = &trees->fixedLitTree;
					pDistTree = &trees->fixedDistTree;
				} else {
					// Dynamic Huffman codes
					int hlit = bs.ReadBits(5) + 257;
					int hdist = bs.ReadBits(5) + 1;
					int hclen = bs.ReadBits(4) + 4;
					if (hlit > 286 || hdist > 30) {
						free(trees);
						return false;
					}

					byte clenCodeLengths[19] = { 0 };
					for (int i = 0; i < hclen; ++i) {
						clenCodeLengths[kClenOrder[i]] = (byte)bs.ReadBits(3);
					}

					
					if (!BuildDeflateHuffman(trees->clenTree, clenCodeLengths, 19)) {
						free(trees);
						return false;
					}

					byte totalLengths[320] = { 0 };
					int totalSymbols = hlit + hdist;
					int cur = 0;
					while (cur < totalSymbols) {
						int sym = bs.DecodeSymbol(trees->clenTree);
						if (sym < 0) { free(trees); return false; }
						if (sym <= 15) {
							totalLengths[cur++] = (byte)sym;
						} else if (sym == 16) {
							if (cur == 0) { free(trees); return false; }
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

					BuildDeflateHuffman(trees->dynamicLitTree, totalLengths, hlit);
					BuildDeflateHuffman(trees->dynamicDistTree, totalLengths + hlit, hdist);
					pLitTree = &trees->dynamicLitTree;
					pDistTree = &trees->dynamicDistTree;
				}

				// Decompress block data
				while (true) {
					int sym = bs.DecodeSymbol(*pLitTree);
					if (sym < 0) { free(trees); return false; }
					if (sym < 256) {
						if (outPos >= expectedOutSize) { free(trees); return false; }
						outData[outPos++] = (byte)sym;
					} else if (sym == 256) {
						// End of Block
						break;
					} else if (sym <= 285) {
						int len = kBaseLength[sym - 257] + bs.ReadBits(kExtraLengthBits[sym - 257]);
						int distSym = bs.DecodeSymbol(*pDistTree);
						if (distSym < 0 || distSym >= 30) { free(trees); return false; }
						int dist = kBaseDist[distSym] + bs.ReadBits(kExtraDistBits[distSym]);

						if (dist > (int)outPos) { free(trees); return false; }
						if (outPos + len > expectedOutSize) { free(trees); return false; }

						for (int i = 0; i < len; ++i) {
							outData[outPos] = outData[outPos - dist];
							outPos++;
						}
					} else {
						free(trees);
						return false;
					}
				}
			} else {
				// Invalid block type
				free(trees);
				return false;
			}
		}

		free(trees);
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
	if (MemCompare((const char*)fileData, (const char*)kPngSig, 8) != 0) {
		return nullptr;
	}

	PNG_IHDR ihdr;
	MemSet(&ihdr, 0, sizeof(ihdr));
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
			MemCopyN(palette, chunkData, paletteSize * 3);
		} else if (chunkType == PNG_CHUNK_tRNS) {
			trnsSize = chunkLen > 256 ? 256 : chunkLen;
			MemCopyN(trnsData, chunkData, trnsSize);
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
				MemCopyN(idatBuffer + idatSize, chunkData, chunkLen);
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
			MemCopyN(dst, src, rowBytes);
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
			MemCopyN(dst, src, rowBytes);
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
	byte* blockBuf = (byte*)malloc(blockSize);
	if (!blockBuf) {
		return uni::ImageResult::OUT_OF_MEMORY;
	}

	stduint readBytes = storage.Read(0, sig, 8, blockBuf);
	free(blockBuf);

	static const byte kPngSig[8] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
	if (readBytes == 8 && MemCompare((const char*)sig, (const char*)kPngSig, 8) == 0) {
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
	byte* blockBuf = (byte*)malloc(blockSize);
	if (!blockBuf) return uni::ImageResult::OUT_OF_MEMORY;

	byte headerBuf[32];
	stduint readBytes = storage.Read(0, headerBuf, 32, blockBuf);
	free(blockBuf);

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

namespace {

	struct PNGIDATSource {
		uni::StorageTrait* storage;
		stduint firstIDATOffset;
		stduint firstIDATLength;
		stduint currentFileOffset;
		stduint remainingInCurrentChunk;
		stduint totalFileSize;
		byte*   chunkBuf;
		byte*   sectorBuf;
		stduint blockSize;
		stduint bufPos;
		stduint bufAvail;

		PNGIDATSource() : storage(nullptr), firstIDATOffset(0), firstIDATLength(0),
			currentFileOffset(0), remainingInCurrentChunk(0), totalFileSize(0),
			chunkBuf(nullptr), sectorBuf(nullptr), blockSize(0), bufPos(0), bufAvail(0) {}

		~PNGIDATSource() {
			if (chunkBuf) free(chunkBuf);
			if (sectorBuf) free(sectorBuf);
		}

		void Init(uni::StorageTrait* stg, stduint firstOffset, stduint firstLen, stduint fileSize) {
			storage = stg;
			firstIDATOffset = firstOffset;
			firstIDATLength = firstLen;
			totalFileSize = fileSize;
			blockSize = storage->Block_Size ? storage->Block_Size : 512;
			if (!chunkBuf) {
				chunkBuf = (byte*)malloc(blockSize);
			}
			if (!sectorBuf) {
				sectorBuf = (byte*)malloc(blockSize);
			}
			Reset();
		}

		void Reset() {
			currentFileOffset = firstIDATOffset;
			remainingInCurrentChunk = firstIDATLength;
			bufPos = 0;
			bufAvail = 0;
		}

		int ReadByte() {
			if (bufPos < bufAvail) {
				return (int)chunkBuf[bufPos++];
			}

			while (remainingInCurrentChunk == 0) {
				currentFileOffset += 4; // Skip CRC of previous chunk
				if (currentFileOffset + 8 > totalFileSize) return -1;

				byte hdr[8];
				stduint rd = storage->Read(currentFileOffset, hdr, 8, sectorBuf);
				if (rd != 8) return -1;

				uint32 chunkLen = *(const BigEndian<uint32, true>*)hdr;
				uint32 chunkType = *(const BigEndian<uint32, true>*)(hdr + 4);
				currentFileOffset += 8;

				if (chunkType == PNG_CHUNK_IDAT) {
					remainingInCurrentChunk = chunkLen;
				} else {
					return -1;
				}
			}

			stduint toRead = blockSize;
			if (toRead > remainingInCurrentChunk) toRead = remainingInCurrentChunk;
			stduint rd = storage->Read(currentFileOffset, chunkBuf, toRead, sectorBuf);
			if (rd == 0) return -1;

			currentFileOffset += rd;
			remainingInCurrentChunk -= rd;
			bufPos = 1;
			bufAvail = rd;
			return (int)chunkBuf[0];
		}

		bool ReadBytes(byte* dest, size_t count) {
			size_t out = 0;
			while (out < count) {
				if (bufPos < bufAvail) {
					size_t n = bufAvail - bufPos;
					if (n > count - out) n = count - out;
					MemCopyN(dest + out, chunkBuf + bufPos, n);
					bufPos += n;
					out += n;
					continue;
				}

				int b = ReadByte();
				if (b < 0) return false;
				dest[out++] = (byte)b;
			}
			return true;
		}
	};

	struct StreamInflate {
		PNGIDATSource* src;
		uint32 bitBuf;
		int    bitsCount;
		bool   zlibHeaderParsed;
		bool   bfinal;
		int    btype;
		uint16 storedLen;

		DeflateHuffman litTree;
		DeflateHuffman distTree;
		DeflateHuffman clenTree;

		int    matchRemain;
		uint16 matchDist;

		byte   window[32768];
		uint32 winHead;

		void Init(PNGIDATSource* pSrc) {
			src = pSrc;
			Reset();
		}

		void Reset() {
			bitBuf = 0;
			bitsCount = 0;
			zlibHeaderParsed = false;
			bfinal = false;
			btype = -1;
			storedLen = 0;
			matchRemain = 0;
			matchDist = 0;
			winHead = 0;
			MemSet(window, 0, sizeof(window));
		}

		bool FillBits(int n) {
			while (bitsCount < n) {
				int b = src->ReadByte();
				if (b < 0) {
					bitBuf |= (0 << bitsCount);
					bitsCount += 8;
					return false;
				}
				bitBuf |= ((uint32)(byte)b) << bitsCount;
				bitsCount += 8;
			}
			return true;
		}

		int ReadBits(int n) {
			if (n == 0) return 0;
			FillBits(n);
			int val = bitBuf & ((1 << n) - 1);
			bitBuf >>= n;
			bitsCount -= n;
			return val;
		}

		void DropBits(int n) {
			bitBuf >>= n;
			bitsCount -= n;
		}

		void AlignToByte() {
			int drop = bitsCount % 8;
			if (drop > 0) DropBits(drop);
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

		bool StartNewBlock() {
			if (bfinal) return false;
			bfinal = ReadBits(1) != 0;
			btype = ReadBits(2);
			if (btype == 0) {
				AlignToByte();
				uint16 len = (uint16)ReadBits(16);
				uint16 nlen = (uint16)ReadBits(16);
				if ((uint16)(len ^ 0xFFFF) != nlen) return false;
				storedLen = len;
			} else if (btype == 1) {
				byte litLengths[288];
				for (int i = 0; i <= 143; ++i) litLengths[i] = 8;
				for (int i = 144; i <= 255; ++i) litLengths[i] = 9;
				for (int i = 256; i <= 279; ++i) litLengths[i] = 7;
				for (int i = 280; i <= 287; ++i) litLengths[i] = 8;
				BuildDeflateHuffman(litTree, litLengths, 288);

				byte distLengths[32];
				for (int i = 0; i < 32; ++i) distLengths[i] = 5;
				BuildDeflateHuffman(distTree, distLengths, 32);
			} else if (btype == 2) {
				int hlit = ReadBits(5) + 257;
				int hdist = ReadBits(5) + 1;
				int hclen = ReadBits(4) + 4;
				if (hlit > 286 || hdist > 30) return false;

				byte clenLengths[19] = { 0 };
				for (int i = 0; i < hclen; ++i) {
					clenLengths[kClenOrder[i]] = (byte)ReadBits(3);
				}

				if (!BuildDeflateHuffman(clenTree, clenLengths, 19)) return false;

				byte codeLengths[320] = { 0 };
				int numCodes = hlit + hdist;
				int idx = 0;
				while (idx < numCodes) {
					int sym = DecodeSymbol(clenTree);
					if (sym < 0) return false;
					if (sym < 16) {
						codeLengths[idx++] = (byte)sym;
					} else if (sym == 16) {
						if (idx == 0) return false;
						int rep = ReadBits(2) + 3;
						byte prev = codeLengths[idx - 1];
						while (rep-- > 0 && idx < numCodes) codeLengths[idx++] = prev;
					} else if (sym == 17) {
						int rep = ReadBits(3) + 3;
						while (rep-- > 0 && idx < numCodes) codeLengths[idx++] = 0;
					} else if (sym == 18) {
						int rep = ReadBits(7) + 11;
						while (rep-- > 0 && idx < numCodes) codeLengths[idx++] = 0;
					}
				}
				BuildDeflateHuffman(litTree, codeLengths, hlit);
				BuildDeflateHuffman(distTree, codeLengths + hlit, hdist);
			} else {
				return false;
			}
			return true;
		}

		bool EnsureZlibHeader() {
			if (!zlibHeaderParsed) {
				int cmf = src->ReadByte();
				int flg = src->ReadByte();
				if (cmf < 0 || flg < 0) return false;
				if ((cmf & 0x0F) != 8 || ((cmf * 256 + flg) % 31) != 0) return false;
				if (flg & 0x20) {
					for (int i = 0; i < 4; ++i) src->ReadByte();
				}
				zlibHeaderParsed = true;
			}
			return true;
		}

		void AppendWindow(const byte* data, size_t count) {
			while (count > 0) {
				size_t pos = winHead & 32767;
				size_t n = 32768 - pos;
				if (n > count) n = count;
				MemCopyN(window + pos, data, n);
				winHead += (uint32)n;
				data += n;
				count -= n;
			}
		}

		void RepeatWindowByte(byte value, size_t count) {
			while (count > 0) {
				size_t pos = winHead & 32767;
				size_t n = 32768 - pos;
				if (n > count) n = count;
				MemSet(window + pos, value, n);
				winHead += (uint32)n;
				count -= n;
			}
		}

		void CopyMatchNoOverlap(byte* dest, size_t count) {
			size_t out = 0;
			while (out < count) {
				size_t pos = (winHead - matchDist + out) & 32767;
				size_t n = 32768 - pos;
				if (n > count - out) n = count - out;
				MemCopyN(dest + out, window + pos, n);
				out += n;
			}
			AppendWindow(dest, count);
		}

		int PullByte() {
			if (!EnsureZlibHeader()) return -1;

			if (matchRemain > 0) {
				byte b = window[(winHead - matchDist) & 32767];
				window[winHead & 32767] = b;
				winHead++;
				matchRemain--;
				return (int)b;
			}

			while (true) {
				if (btype < 0) {
					if (!StartNewBlock()) return -1;
				}

				if (btype == 0) {
					if (storedLen > 0) {
						int b = ReadBits(8);
						storedLen--;
						if (storedLen == 0) btype = -1;
						window[winHead & 32767] = (byte)b;
						winHead++;
						return b;
					}
					btype = -1;
				} else if (btype == 1 || btype == 2) {
					int sym = DecodeSymbol(litTree);
					if (sym < 0) return -1;
					if (sym < 256) {
						byte b = (byte)sym;
						window[winHead & 32767] = b;
						winHead++;
						return (int)b;
					} else if (sym == 256) {
						btype = -1;
					} else if (sym <= 285) {
						int len = kBaseLength[sym - 257] + ReadBits(kExtraLengthBits[sym - 257]);
						int distSym = DecodeSymbol(distTree);
						if (distSym < 0 || distSym >= 30) return -1;
						int dist = kBaseDist[distSym] + ReadBits(kExtraDistBits[distSym]);
						matchDist = (uint16)dist;
						matchRemain = len;

						byte b = window[(winHead - matchDist) & 32767];
						window[winHead & 32767] = b;
						winHead++;
						matchRemain--;
						return (int)b;
					} else {
						return -1;
					}
				} else {
					return -1;
				}
			}
		}

		bool Pull(byte* dest, size_t count) {
			if (!EnsureZlibHeader()) return false;

			size_t out = 0;
			while (out < count) {
				if (matchRemain > 0) {
					size_t n = count - out;
					if (n > (size_t)matchRemain) n = matchRemain;
					if (matchDist == 1) {
						byte b = window[(winHead - 1) & 32767];
						MemSet(dest + out, b, n);
						RepeatWindowByte(b, n);
						out += n;
					} else if ((size_t)matchDist >= n) {
						CopyMatchNoOverlap(dest + out, n);
						out += n;
					} else {
						for (size_t i = 0; i < n; ++i) {
							byte b = window[(winHead - matchDist) & 32767];
							window[winHead & 32767] = b;
							winHead++;
							dest[out++] = b;
						}
					}
					matchRemain -= (int)n;
					continue;
				}

				if (btype < 0) {
					if (!StartNewBlock()) return false;
				}

				if (btype == 0) {
					if (storedLen > 0) {
						size_t n = count - out;
						if (n > (size_t)storedLen) n = storedLen;
						if (bitsCount == 0) {
							if (!src->ReadBytes(dest + out, n)) return false;
							AppendWindow(dest + out, n);
							out += n;
						} else {
							for (size_t i = 0; i < n; ++i) {
								int b = ReadBits(8);
								byte outByte = (byte)b;
								window[winHead & 32767] = outByte;
								winHead++;
								dest[out++] = outByte;
							}
						}
						storedLen -= (uint16)n;
						if (storedLen == 0) btype = -1;
						continue;
					}
					btype = -1;
				} else if (btype == 1 || btype == 2) {
					int sym = DecodeSymbol(litTree);
					if (sym < 0) return false;
					if (sym < 256) {
						byte b = (byte)sym;
						window[winHead & 32767] = b;
						winHead++;
						dest[out++] = b;
					} else if (sym == 256) {
						btype = -1;
					} else if (sym <= 285) {
						int len = kBaseLength[sym - 257] + ReadBits(kExtraLengthBits[sym - 257]);
						int distSym = DecodeSymbol(distTree);
						if (distSym < 0 || distSym >= 30) return false;
						int dist = kBaseDist[distSym] + ReadBits(kExtraDistBits[distSym]);
						matchDist = (uint16)dist;
						matchRemain = len;
					} else {
						return false;
					}
				} else {
					return false;
				}
			}
			return true;
		}
	};

	static void ReconstructScanline(
		const byte* src,
		uni::Color* dst,
		uint32 startX,
		uint32 count,
		byte colorType,
		byte bitDepth,
		const byte* palette,
		int paletteSize,
		const byte* trnsData,
		int trnsSize,
		bool hasTrns
	) {
		for (uint32 col = 0; col < count; ++col) {
			uint32 x = startX + col;
			uni::Color c;
			c.r = c.g = c.b = 0;
			c.a = 0xFF;

			switch ((PNGColorType)colorType) {
			case PNGColorType::GRAYSCALE: {
				byte g = 0;
				if (bitDepth == 8) {
					g = src[x];
				} else if (bitDepth == 16) {
					g = src[x * 2];
				} else if (bitDepth == 4) {
					byte b = src[x / 2];
					g = (x % 2 == 0) ? ((b >> 4) & 0x0F) : (b & 0x0F);
					g = (byte)(g * 255 / 15);
				} else if (bitDepth == 2) {
					byte b = src[x / 4];
					g = (b >> (2 * (3 - (x % 4)))) & 0x03;
					g = (byte)(g * 255 / 3);
				} else if (bitDepth == 1) {
					byte b = src[x / 8];
					g = ((b >> (7 - (x % 8))) & 0x01) ? 255 : 0;
				}
				c.r = c.g = c.b = g;
				if (hasTrns && trnsSize >= 2) {
					uint16 key = ((uint16)trnsData[0] << 8) | trnsData[1];
					if (g == (byte)key) c.a = 0;
				}
				break;
			}
			case PNGColorType::RGB: {
				if (bitDepth == 8) {
					const byte* p = src + x * 3;
					c.r = p[0]; c.g = p[1]; c.b = p[2];
				} else if (bitDepth == 16) {
					const byte* p = src + x * 6;
					c.r = p[0]; c.g = p[2]; c.b = p[4];
				}
				if (hasTrns && trnsSize >= 6) {
					uint16 rk = ((uint16)trnsData[0] << 8) | trnsData[1];
					uint16 gk = ((uint16)trnsData[2] << 8) | trnsData[3];
					uint16 bk = ((uint16)trnsData[4] << 8) | trnsData[5];
					if (c.r == (byte)rk && c.g == (byte)gk && c.b == (byte)bk) {
						c.a = 0;
					}
				}
				break;
			}
			case PNGColorType::INDEXED: {
				byte idx = 0;
				if (bitDepth == 8) {
					idx = src[x];
				} else if (bitDepth == 4) {
					byte b = src[x / 2];
					idx = (x % 2 == 0) ? ((b >> 4) & 0x0F) : (b & 0x0F);
				} else if (bitDepth == 2) {
					byte b = src[x / 4];
					idx = (b >> (2 * (3 - (x % 4)))) & 0x03;
				} else if (bitDepth == 1) {
					byte b = src[x / 8];
					idx = (b >> (7 - (x % 8))) & 0x01;
				}
				if (idx < paletteSize) {
					c.r = palette[idx * 3 + 0];
					c.g = palette[idx * 3 + 1];
					c.b = palette[idx * 3 + 2];
				}
				if (hasTrns && idx < trnsSize) {
					c.a = trnsData[idx];
				}
				break;
			}
			case PNGColorType::GRAYSCALE_ALPHA: {
				if (bitDepth == 8) {
					const byte* p = src + x * 2;
					c.r = c.g = c.b = p[0];
					c.a = p[1];
				} else if (bitDepth == 16) {
					const byte* p = src + x * 4;
					c.r = c.g = c.b = p[0];
					c.a = p[2];
				}
				break;
			}
			case PNGColorType::RGBA: {
				if (bitDepth == 8) {
					const byte* p = src + x * 4;
					c.r = p[0]; c.g = p[1]; c.b = p[2]; c.a = p[3];
				} else if (bitDepth == 16) {
					const byte* p = src + x * 8;
					c.r = p[0]; c.g = p[2]; c.b = p[4]; c.a = p[6];
				}
				break;
			}
			}
			dst[col] = c;
		}
	}

	static void ReconstructScanlineRGB565(
		const byte* src,
		uint16* dst,
		uint32 startX,
		uint32 count,
		byte colorType,
		byte bitDepth,
		const byte* palette,
		int paletteSize,
		const byte* trnsData,
		int trnsSize,
		bool hasTrns
	) {
		for (uint32 col = 0; col < count; ++col) {
			uint32 x = startX + col;
			uni::Color c;
			c.r = c.g = c.b = 0;
			c.a = 0xFF;

			switch ((PNGColorType)colorType) {
			case PNGColorType::GRAYSCALE: {
				byte g = 0;
				if (bitDepth == 8) {
					g = src[x];
				} else if (bitDepth == 16) {
					g = src[x * 2];
				} else if (bitDepth == 4) {
					byte b = src[x / 2];
					g = (x % 2 == 0) ? ((b >> 4) & 0x0F) : (b & 0x0F);
					g = (byte)(g * 255 / 15);
				} else if (bitDepth == 2) {
					byte b = src[x / 4];
					g = (b >> (2 * (3 - (x % 4)))) & 0x03;
					g = (byte)(g * 255 / 3);
				} else if (bitDepth == 1) {
					byte b = src[x / 8];
					g = ((b >> (7 - (x % 8))) & 0x01) ? 255 : 0;
				}
				c.r = c.g = c.b = g;
				if (hasTrns && trnsSize >= 2) {
					uint16 key = ((uint16)trnsData[0] << 8) | trnsData[1];
					if (g == (byte)key) c.a = 0;
				}
				break;
			}
			case PNGColorType::RGB: {
				if (bitDepth == 8) {
					const byte* p = src + x * 3;
					c.r = p[0]; c.g = p[1]; c.b = p[2];
				} else if (bitDepth == 16) {
					const byte* p = src + x * 6;
					c.r = p[0]; c.g = p[2]; c.b = p[4];
				}
				if (hasTrns && trnsSize >= 6) {
					uint16 rk = ((uint16)trnsData[0] << 8) | trnsData[1];
					uint16 gk = ((uint16)trnsData[2] << 8) | trnsData[3];
					uint16 bk = ((uint16)trnsData[4] << 8) | trnsData[5];
					if (c.r == (byte)rk && c.g == (byte)gk && c.b == (byte)bk) {
						c.a = 0;
					}
				}
				break;
			}
			case PNGColorType::INDEXED: {
				byte idx = 0;
				if (bitDepth == 8) {
					idx = src[x];
				} else if (bitDepth == 4) {
					byte b = src[x / 2];
					idx = (x % 2 == 0) ? ((b >> 4) & 0x0F) : (b & 0x0F);
				} else if (bitDepth == 2) {
					byte b = src[x / 4];
					idx = (b >> (2 * (3 - (x % 4)))) & 0x03;
				} else if (bitDepth == 1) {
					byte b = src[x / 8];
					idx = (b >> (7 - (x % 8))) & 0x01;
				}
				if (idx < paletteSize) {
					c.r = palette[idx * 3 + 0];
					c.g = palette[idx * 3 + 1];
					c.b = palette[idx * 3 + 2];
				}
				if (hasTrns && idx < trnsSize) {
					c.a = trnsData[idx];
				}
				break;
			}
			case PNGColorType::GRAYSCALE_ALPHA: {
				if (bitDepth == 8) {
					const byte* p = src + x * 2;
					c.r = c.g = c.b = p[0];
					c.a = p[1];
				} else if (bitDepth == 16) {
					const byte* p = src + x * 4;
					c.r = c.g = c.b = p[0];
					c.a = p[2];
				}
				break;
			}
			case PNGColorType::RGBA: {
				if (bitDepth == 8) {
					const byte* p = src + x * 4;
					c.r = p[0]; c.g = p[1]; c.b = p[2]; c.a = p[3];
				} else if (bitDepth == 16) {
					const byte* p = src + x * 8;
					c.r = p[0]; c.g = p[2]; c.b = p[4]; c.a = p[6];
				}
				break;
			}
			}

			dst[col] = c.ToRGB565();
		}
	}

	class PNGSurface : public uni::IImageSurface {
	private:
		uni::StorageTrait* storage;
		uni::ImageInfo     info;
		uni::trait::Malloc* allocator;
		uni::PixelFormat   outputFormat;

		PNGIDATSource      idatSrc;
		StreamInflate      inflate;

		byte   colorType;
		byte   bitDepth;
		byte   palette[256 * 3];
		int    paletteSize;
		byte   trnsData[256];
		int    trnsSize;
		bool   hasTrns;

		size_t rowBytes;
		int    bpp;
		byte*  prevRow;
		byte*  currRow;
		byte*  rawRow;
		uint32 currentY;

	public:
		PNGSurface(
			uni::StorageTrait& stg,
			const uni::ImageInfo& inf,
			stduint firstIDATOff,
			stduint firstIDATLen,
			stduint fileSize,
			byte colType,
			byte depth,
			const byte* pal,
			int palSize,
			const byte* trns,
			int trSize,
			bool trnsFlag,
			size_t rBytes,
			int bytesPerPx,
			uni::PixelFormat outFmt,
			uni::trait::Malloc& alloc
		) : storage(&stg), info(inf), allocator(&alloc),
			outputFormat(outFmt),
			colorType(colType), bitDepth(depth),
			paletteSize(palSize), trnsSize(trSize), hasTrns(trnsFlag),
			rowBytes(rBytes), bpp(bytesPerPx), currentY(0) {

			info.format = outputFormat;
			if (outputFormat == uni::PixelFormat::RGB565) {
				info.bitsPerPixel = 16;
				info.alphaMode = uni::ImageAlphaMode::NONE;
				info.hasAlpha = false;
			}

			if (pal && palSize) {
				MemCopyN(palette, pal, palSize * 3);
			}
			if (trns && trSize) {
				MemCopyN(trnsData, trns, trSize);
			}

			idatSrc.Init(storage, firstIDATOff, firstIDATLen, fileSize);
			inflate.Init(&idatSrc);

			prevRow = (byte*)malloc(rowBytes);
			currRow = (byte*)malloc(rowBytes);
			rawRow  = (byte*)malloc(rowBytes);

			if (prevRow) MemSet(prevRow, 0, rowBytes);
			if (currRow) MemSet(currRow, 0, rowBytes);
			if (rawRow)  MemSet(rawRow, 0, rowBytes);
		}

		virtual ~PNGSurface() {
			if (prevRow) free(prevRow);
			if (currRow) free(currRow);
			if (rawRow)  free(rawRow);
		}

		virtual void Release() override {
			uni::trait::Malloc* alloc = allocator;
			this->~PNGSurface();
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
				   uni::ImageSurfaceCapability::SCANLINE_READ;
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

			if (!prevRow || !currRow || !rawRow) {
				return uni::ImageResult::OUT_OF_MEMORY;
			}

			// If requested region starts before current scanline position, restart stream
			if ((uint32)ry < currentY) {
				idatSrc.Reset();
				inflate.Reset();
				MemSet(prevRow, 0, rowBytes);
				currentY = 0;
			}

			size_t pixelSize = outputFormat == uni::PixelFormat::RGB565 ? sizeof(uint16) : sizeof(uni::Color);
			size_t neededSize = (size_t)rw * (size_t)rh * pixelSize;
			void* pixelsMem = outBuffer.pixels;
			bool ownsMem = false;
			if (!pixelsMem) {
				pixelsMem = alloc.allocate(neededSize);
				if (!pixelsMem) return uni::ImageResult::OUT_OF_MEMORY;
				ownsMem = true;
			}

			uni::Color* outPixels = (uni::Color*)pixelsMem;
			uint16* outPixels565 = (uint16*)pixelsMem;
			uint32 targetEndY = (uint32)(ry + rh);

			while (currentY < targetEndY) {
				// Pull 1 byte filter type
				int filterType = inflate.PullByte();
				if (filterType < 0) {
					if (ownsMem) alloc.deallocate(pixelsMem);
					return uni::ImageResult::FAILED;
				}

				// Pull 1 scanline raw data
				if (!inflate.Pull(rawRow, rowBytes)) {
					if (ownsMem) alloc.deallocate(pixelsMem);
					return uni::ImageResult::FAILED;
				}

				// Apply inverse filter
				switch ((PNGFilterType)filterType) {
				case PNGFilterType::NONE:
					MemCopyN(currRow, rawRow, rowBytes);
					break;
				case PNGFilterType::SUB:
					for (size_t i = 0; i < rowBytes; ++i) {
						byte a = (i >= (size_t)bpp) ? currRow[i - bpp] : 0;
						currRow[i] = (byte)(rawRow[i] + a);
					}
					break;
				case PNGFilterType::UP:
					for (size_t i = 0; i < rowBytes; ++i) {
						byte bVal = prevRow[i];
						currRow[i] = (byte)(rawRow[i] + bVal);
					}
					break;
				case PNGFilterType::AVERAGE:
					for (size_t i = 0; i < rowBytes; ++i) {
						int a = (i >= (size_t)bpp) ? currRow[i - bpp] : 0;
						int bVal = prevRow[i];
						currRow[i] = (byte)(rawRow[i] + ((a + bVal) >> 1));
					}
					break;
				case PNGFilterType::PAETH:
					for (size_t i = 0; i < rowBytes; ++i) {
						int a = (i >= (size_t)bpp) ? currRow[i - bpp] : 0;
						int bVal = prevRow[i];
						int cVal = (i >= (size_t)bpp) ? prevRow[i - bpp] : 0;
						currRow[i] = (byte)(rawRow[i] + PaethPredictor(a, bVal, cVal));
					}
					break;
				default:
					if (ownsMem) alloc.deallocate(pixelsMem);
					return uni::ImageResult::FAILED;
				}

				// If current scanline is within the requested rectangle, reconstruct pixels
				if (currentY >= (uint32)ry && currentY < targetEndY) {
					uint32 lineOffset = (currentY - (uint32)ry) * (uint32)rw;
					if (outputFormat == uni::PixelFormat::RGB565) {
						ReconstructScanlineRGB565(
							currRow,
							outPixels565 + lineOffset,
							(uint32)rx,
							(uint32)rw,
							colorType,
							bitDepth,
							palette,
							paletteSize,
							trnsData,
							trnsSize,
							hasTrns
						);
					} else {
						ReconstructScanline(
							currRow,
							outPixels + lineOffset,
							(uint32)rx,
							(uint32)rw,
							colorType,
							bitDepth,
							palette,
							paletteSize,
							trnsData,
							trnsSize,
							hasTrns
						);
					}
				}

				// Update previous row
				MemCopyN(prevRow, currRow, rowBytes);
				currentY++;
			}

			outBuffer.width = (uint32)rw;
			outBuffer.height = (uint32)rh;
			outBuffer.stride = (uint32)(rw * pixelSize);
			outBuffer.format = outputFormat;
			outBuffer.colorSpace = uni::ColorSpace::SRGB;
			outBuffer.alphaMode = outputFormat == uni::PixelFormat::RGB565 ? uni::ImageAlphaMode::NONE : info.alphaMode;
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

uni::ImageResult uni::PNGCodec::OpenSurface(
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

	stduint fileSize = storage.getUnits() * storage.Block_Size;
	stduint blockSize = storage.Block_Size ? storage.Block_Size : 512;
	byte* blockBuf = (byte*)malloc(blockSize);
	if (!blockBuf) return uni::ImageResult::OUT_OF_MEMORY;

	byte* palette = (byte*)malloc(256 * 3);
	byte* trnsData = (byte*)malloc(256);
	if (!palette || !trnsData) {
		if (blockBuf) free(blockBuf);
		if (palette) free(palette);
		if (trnsData) free(trnsData);
		return uni::ImageResult::OUT_OF_MEMORY;
	}

	byte headerBuf[32];
	if (storage.Read(0, headerBuf, 32, blockBuf) < 29) {
		free(blockBuf); free(palette); free(trnsData);
		return uni::ImageResult::INVALID_FORMAT;
	}

	const PNG_IHDR* ihdr = (const PNG_IHDR*)(headerBuf + 16);
	if (ihdr->interlace_method != 0) {
		free(blockBuf); free(palette); free(trnsData);
		return uni::ImageResult::UNSUPPORTED; // Non-interlaced streaming only
	}

	int paletteSize = 0;
	int trnsSize = 0;
	bool hasTrns = false;

	stduint firstIDATOffset = 0;
	stduint firstIDATLength = 0;

	// Traverse chunks to extract PLTE, tRNS, and locate first IDAT
	stduint pos = 8;
	while (pos + 8 <= fileSize) {
		byte chunkHdr[8];
		if (storage.Read(pos, chunkHdr, 8, blockBuf) != 8) break;

		uint32 chunkLen = *(const BigEndian<uint32, true>*)chunkHdr;
		uint32 chunkType = *(const BigEndian<uint32, true>*)(chunkHdr + 4);
		pos += 8;

		if (chunkType == PNG_CHUNK_PLTE) {
			paletteSize = (int)(chunkLen / 3);
			if (paletteSize > 256) paletteSize = 256;
			storage.Read(pos, palette, paletteSize * 3, blockBuf);
		} else if (chunkType == PNG_CHUNK_tRNS) {
			trnsSize = chunkLen > 256 ? 256 : chunkLen;
			storage.Read(pos, trnsData, trnsSize, blockBuf);
			hasTrns = true;
		} else if (chunkType == PNG_CHUNK_IDAT) {
			firstIDATOffset = pos;
			firstIDATLength = chunkLen;
			break;
		} else if (chunkType == PNG_CHUNK_IEND) {
			break;
		}

		pos += chunkLen + 4; // Skip data + CRC
	}

	if (firstIDATOffset == 0 || firstIDATLength == 0) {
		free(blockBuf); free(palette); free(trnsData);
		return uni::ImageResult::INVALID_FORMAT;
	}

	int channels = 1;
	switch ((PNGColorType)ihdr->color_type) {
	case PNGColorType::GRAYSCALE:       channels = 1; break;
	case PNGColorType::RGB:             channels = 3; break;
	case PNGColorType::INDEXED:         channels = 1; break;
	case PNGColorType::GRAYSCALE_ALPHA: channels = 2; break;
	case PNGColorType::RGBA:            channels = 4; break;
	default:
		free(blockBuf); free(palette); free(trnsData);
		return uni::ImageResult::INVALID_FORMAT;
	}

	size_t rowBytes = ((size_t)ihdr->width * channels * ihdr->bit_depth + 7) / 8;
	int bpp = (channels * ihdr->bit_depth + 7) / 8;
	if (bpp < 1) bpp = 1;

	PixelFormat outputFormat = options.preferredFormat == PixelFormat::RGB565 ?
		PixelFormat::RGB565 : PixelFormat::ARGB8888;

	void* mem = allocator.allocate(sizeof(PNGSurface));
	if (!mem) {
		free(blockBuf); free(palette); free(trnsData);
		return uni::ImageResult::OUT_OF_MEMORY;
	}

	PNGSurface* surf = new (mem) PNGSurface(
		storage, info, firstIDATOffset, firstIDATLength, fileSize,
		ihdr->color_type, ihdr->bit_depth,
		palette, paletteSize, trnsData, trnsSize, hasTrns,
		rowBytes, bpp, outputFormat, allocator
	);

	free(blockBuf);
	free(palette);
	free(trnsData);

	outSurface = surf;
	return uni::ImageResult::OK;
}

uni::ImageResult uni::PNGCodec::Decode(
	StorageTrait& storage,
	ImageBuffer& outBuffer,
	trait::Malloc& allocator,
	const ImageDecodeOptions& options
) const {
	IImageSurface* surface = nullptr;
	uni::ImageResult res = OpenSurface(storage, surface, allocator, options, ImageAccessMode::READ_ONLY);
	if (res != uni::ImageResult::OK) {
		return res;
	}

	ImageInfo info;
	surface->GetInfo(info);

	Rectangle fullRect{ Point(0, 0), Size2(info.width, info.height) };
	res = surface->ReadPixels(fullRect, outBuffer, allocator);
	surface->Release();

	return res;
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
