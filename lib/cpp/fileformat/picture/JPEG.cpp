// ASCII C/C++ TAB4 CRLF
// Docutitle: JPEG Decoder Implementation (Baseline & Progressive)
// Attribute: Env-Freestanding Non-Dependence
// Copyright: UNISYM

#include "../../../../inc/c/format/picture/JPEG.h"
#include "../../../../inc/cpp/Device/Graphic/GPE-JPEG.hpp"
#include "../../../../inc/cpp/Device/GPU"
#include "../../../../inc/cpp/endian"
// #include "../../../../inc/cpp/Device/UART"
// #include <stdlib.h>
#include "../../../../inc/c/ustring.h"
// #include <string.h>

namespace {

	// Zigzag order mapping for 8x8 DCT coefficients
	static const byte kZigzag[64] = {
		 0,  1,  8, 16,  9,  2,  3, 10,
		17, 24, 32, 25, 18, 11,  4,  5,
		12, 19, 26, 33, 40, 48, 41, 34,
		27, 20, 13,  6,  7, 14, 21, 28,
		35, 42, 49, 56, 57, 50, 43, 36,
		29, 22, 15, 23, 30, 37, 44, 51,
		58, 59, 52, 45, 38, 31, 39, 46,
		53, 60, 61, 54, 47, 55, 62, 63
	};

	// 10-bit fixed point cosine table: round(C(u) * cos((2x + 1) * u * pi / 16) * 1024)
	static const int kCosTable[8][8] = {
		{  724,  1004,   946,   851,   724,   569,   392,   200 },
		{  724,   851,   392,  -200,  -724, -1004,  -946,  -569 },
		{  724,   569,  -392, -1004,  -724,   200,   946,   851 },
		{  724,   200,  -946,  -569,   724,   851,  -392, -1004 },
		{  724,  -200,  -946,   569,   724,  -851,  -392,  1004 },
		{  724,  -569,  -392,  1004,  -724,  -200,   946,  -851 },
		{  724,  -851,   392,   200,  -724,  1004,  -946,   569 },
		{  724, -1004,   946,  -851,   724,  -569,   392,  -200 }
	};

	static inline byte ClampByte(int val) {
		if (val < 0) return 0;
		if (val > 255) return 255;
		return (byte)val;
	}

	static inline uint16 ReadBE16(const byte* p) {
		return ((const BigEndian<uint16, true>*)p)->get();
	}

	// Internal Huffman table structure with lookahead acceleration
	struct HuffmanTable {
		byte   lookahead_val[256];
		byte   lookahead_bits[256];
		uint16 mincode[17];
		uint16 maxcode[17];
		int16  valptr[17];
		byte   huffval[256];
		bool   valid;
	};

	static void BuildHuffmanTable(HuffmanTable& ht, const byte bits[16], const byte* huffval, int count) {
		int p = 0;
		uint16 code = 0;
		for (int l = 1; l <= 16; ++l) {
			ht.valptr[l] = (int16)p;
			if (bits[l - 1]) {
				ht.mincode[l] = code;
				ht.maxcode[l] = (uint16)(code + bits[l - 1] - 1);
				p += bits[l - 1];
				code += bits[l - 1];
			} else {
				ht.maxcode[l] = 0xFFFF;
				ht.mincode[l] = 0xFFFF;
			}
			code <<= 1;
		}

		for (int i = 0; i < count && i < 256; ++i) {
			ht.huffval[i] = huffval[i];
		}

		for (int i = 0; i < 256; ++i) {
			ht.lookahead_bits[i] = 0;
		}

		p = 0;
		code = 0;
		for (int l = 1; l <= 8; ++l) {
			for (int i = 0; i < bits[l - 1]; ++i) {
				byte val = huffval[p++];
				int shift = 8 - l;
				int numEntries = 1 << shift;
				int base = code << shift;
				for (int k = 0; k < numEntries; ++k) {
					ht.lookahead_val[base + k] = val;
					ht.lookahead_bits[base + k] = (byte)l;
				}
				code++;
			}
			code <<= 1;
		}
		ht.valid = true;
	}

	// Bitstream reader supporting byte-stuffing, restart markers, and multi-scan boundaries
	struct BitStream {
		const byte* data;
		size_t size;
		size_t pos;
		uint32 bitBuf;
		int    bitsCount;
		bool   markerEncountered;
		byte   marker;

		void Init(const byte* pData, size_t len, size_t startOffset = 0) {
			data = pData;
			size = len;
			pos = startOffset;
			bitBuf = 0;
			bitsCount = 0;
			markerEncountered = false;
			marker = 0;
		}

		void ResetForRestart() {
			bitBuf = 0;
			bitsCount = 0;
			markerEncountered = false;
			marker = 0;
		}

		void FillBits(int n) {
			while (bitsCount < n) {
				if (pos >= size) {
					bitBuf = (bitBuf << 8);
					bitsCount += 8;
					continue;
				}
				byte b = data[pos++];
				if (b == 0xFF) {
					while (pos < size && data[pos] == 0xFF) {
						pos++;
					}
					if (pos < size) {
						byte b2 = data[pos++];
						if (b2 == 0x00) {
							// Byte stuffing: literal 0xFF
							b = 0xFF;
						} else if (b2 >= 0xD0 && b2 <= 0xD7) {
							// Restart marker RST0..RST7
							markerEncountered = true;
							marker = b2;
							continue;
						} else {
							// Marker ending this scan: rewind and stop buffering
							pos -= 2;
							markerEncountered = true;
							marker = b2;
							bitBuf = (bitBuf << 8);
							bitsCount += 8;
							return;
						}
					}
				}
				bitBuf = (bitBuf << 8) | b;
				bitsCount += 8;
			}
		}

		int ReadBits(int n) {
			if (n == 0) return 0;
			FillBits(n);
			int val = (bitBuf >> (bitsCount - n)) & ((1 << n) - 1);
			bitsCount -= n;
			return val;
		}

		int DecodeHuffman(const HuffmanTable& ht) {
			if (!ht.valid) return -1;
			FillBits(8);
			int look = (bitBuf >> (bitsCount - 8)) & 0xFF;
			if (ht.lookahead_bits[look] > 0) {
				int len = ht.lookahead_bits[look];
				bitsCount -= len;
				return ht.lookahead_val[look];
			}
			int code = 0;
			for (int l = 1; l <= 16; ++l) {
				code = (code << 1) | ReadBits(1);
				if (ht.maxcode[l] != 0xFFFF && code <= ht.maxcode[l]) {
					int index = ht.valptr[l] + (code - ht.mincode[l]);
					return ht.huffval[index];
				}
			}
			return -1;
		}

		int ReceiveExtend(int s) {
			if (s == 0) return 0;
			int v = ReadBits(s);
			if (v < (1 << (s - 1))) {
				v += (-1 << s) + 1;
			}
			return v;
		}
	};

	// 8x8 Fixed-Point Integer Inverse Discrete Cosine Transform (IDCT)
	static void IDCT8x8(const int* in, byte* out, int outStride) {
		int work[64];
		// Pass 1: rows (scale by 1024 / 256 = 4)
		for (int y = 0; y < 8; ++y) {
			int row = y * 8;
			for (int x = 0; x < 8; ++x) {
				int sum = 0;
				for (int u = 0; u < 8; ++u) {
					sum += in[row + u] * kCosTable[x][u];
				}
				work[row + x] = (sum + 128) >> 8;
			}
		}
		// Pass 2: columns (scale by 4 * 1024 * 4 = 16384)
		for (int x = 0; x < 8; ++x) {
			for (int y = 0; y < 8; ++y) {
				int sum = 0;
				for (int v = 0; v < 8; ++v) {
					sum += work[v * 8 + x] * kCosTable[y][v];
				}
				int val = ((sum + 8192) >> 14) + 128;
				out[y * outStride + x] = ClampByte(val);
			}
		}
	}

	static inline uni::Color YCbCrToColor(int y, int cb, int cr) {
		int r = y + ((359 * (cr - 128) + 128) >> 8);
		int g = y - ((88 * (cb - 128) + 183 * (cr - 128) + 128) >> 8);
		int b = y + ((454 * (cb - 128) + 128) >> 8);
		uni::Color c;
		c.b = ClampByte(b);
		c.g = ClampByte(g);
		c.r = ClampByte(r);
		c.a = 0xFF;
		return c;
	}

	static inline uni::Color GrayToColor(int y) {
		byte b = ClampByte(y);
		uni::Color c;
		c.b = b;
		c.g = b;
		c.r = b;
		c.a = 0xFF;
		return c;
	}

	// Decodes one block for spectral selection and successive approximation
	static bool DecodeBlockSpectral(BitStream& bs, const HuffmanTable& dcHt, const HuffmanTable& acHt,
									int& dcPredictor, int& eobRun, int16* block,
									byte ss, byte se, byte ah, byte al) {
		// DC first scan
		if (ss == 0 && ah == 0) {
			int s = bs.DecodeHuffman(dcHt);
			if (s < 0) return false;
			int diff = bs.ReceiveExtend(s);
			dcPredictor += diff;
			block[0] = (int16)(dcPredictor << al);
			return true;
		}

		// DC refinement scan
		if (ss == 0 && ah > 0) {
			int bit = bs.ReadBits(1);
			block[0] |= (int16)(bit << al);
			return true;
		}

		// AC first scan
		if (ss > 0 && ah == 0) {
			if (eobRun > 0) {
				eobRun--;
				return true;
			}
			int k = ss;
			while (k <= se) {
				int rs = bs.DecodeHuffman(acHt);
				if (rs < 0) return false;
				int r = rs >> 4;
				int s = rs & 0x0F;
				if (s == 0) {
					if (r < 15) {
						eobRun = (1 << r);
						if (r > 0) eobRun += bs.ReadBits(r);
						eobRun--;
						break;
					} else {
						// ZRL: 16 zeros
						k += 16;
					}
				} else {
					k += r;
					if (k > se) break;
					int val = bs.ReceiveExtend(s);
					int zPos = kZigzag[k];
					block[zPos] = (int16)(val << al);
					k++;
				}
			}
			return true;
		}

		// AC refinement scan
		if (ss > 0 && ah > 0) {
			if (eobRun > 0) {
				for (int k = ss; k <= se; ++k) {
					int zPos = kZigzag[k];
					if (block[zPos] != 0) {
						int bit = bs.ReadBits(1);
						if (bit) {
							if (block[zPos] > 0) block[zPos] += (int16)(1 << al);
							else                 block[zPos] -= (int16)(1 << al);
						}
					}
				}
				eobRun--;
				return true;
			}

			int k = ss;
			while (k <= se) {
				int rs = bs.DecodeHuffman(acHt);
				if (rs < 0) return false;
				int r = rs >> 4;
				int s = rs & 0x0F;
				if (s == 0) {
					if (r < 15) {
						eobRun = (1 << r);
						if (r > 0) eobRun += bs.ReadBits(r);
						while (k <= se) {
							int zPos = kZigzag[k];
							if (block[zPos] != 0) {
								int bit = bs.ReadBits(1);
								if (bit) {
									if (block[zPos] > 0) block[zPos] += (int16)(1 << al);
									else                 block[zPos] -= (int16)(1 << al);
								}
							}
							k++;
						}
						eobRun--;
						break;
					} else {
						// ZRL: 16 zero-coefficients
						int zeros = 16;
						while (k <= se && zeros > 0) {
							int zPos = kZigzag[k];
							if (block[zPos] != 0) {
								int bit = bs.ReadBits(1);
								if (bit) {
									if (block[zPos] > 0) block[zPos] += (int16)(1 << al);
									else                 block[zPos] -= (int16)(1 << al);
								}
							} else {
								zeros--;
							}
							k++;
						}
					}
				} else {
					int zeros = r;
					while (k <= se && zeros > 0) {
						int zPos = kZigzag[k];
						if (block[zPos] != 0) {
							int bit = bs.ReadBits(1);
							if (bit) {
								if (block[zPos] > 0) block[zPos] += (int16)(1 << al);
								else                 block[zPos] -= (int16)(1 << al);
							}
						} else {
							zeros--;
						}
						k++;
					}

					while (k <= se) {
						int zPos = kZigzag[k];
						if (block[zPos] != 0) {
							int bit = bs.ReadBits(1);
							if (bit) {
								if (block[zPos] > 0) block[zPos] += (int16)(1 << al);
								else                 block[zPos] -= (int16)(1 << al);
							}
							k++;
						} else {
							break;
						}
					}

					if (k <= se) {
						int bit = bs.ReadBits(1);
						int zPos = kZigzag[k];
						block[zPos] = (int16)((bit ? 1 : -1) << al);
						k++;
					}
				}
			}
			return true;
		}

		return true;
	}

}

// Decodes raw JPEG data into a Color pixel array (supports Baseline & Progressive)
uni::Color* DecodeJPEG(const byte* fileData, size_t fileSize, int* outWidth, int* outHeight) {
	if (!fileData || fileSize < 4) {
		return nullptr;
	}

	// Verify JPEG SOI marker (0xFFD8)
	if (fileData[0] != 0xFF || fileData[1] != (byte)JPEGMarker::SOI) {
		return nullptr;
	}

	struct JPEGContext {
		byte         quantTables[4][64];
		bool         quantTableValid[4];
		HuffmanTable dcTables[4];
		HuffmanTable acTables[4];
	};

	JPEGContext* ctx = (JPEGContext*)malloc(sizeof(JPEGContext));
	if (!ctx) return nullptr;
	MemSet(ctx, 0, sizeof(JPEGContext));

	JPEG_FRAME_HEADER frame;
	MemSet(&frame, 0, sizeof(frame));
	bool hasFrame = false;
	bool isProgressive = false;

	uint16 restartInterval = 0;
	size_t pos = 2;

	// First pass: locate SOF and compute image/block dimensions
	while (pos < fileSize) {
		if (fileData[pos] != 0xFF) {
			pos++;
			continue;
		}
		while (pos < fileSize && fileData[pos] == 0xFF) {
			pos++;
		}
		if (pos >= fileSize) break;

		byte marker = fileData[pos++];
		if (marker == (byte)JPEGMarker::SOI) continue;
		if (marker == (byte)JPEGMarker::EOI) break;

		if (pos + 2 > fileSize) break;
		uint16 len = ReadBE16(fileData + pos);
		if (len < 2 || pos + len > fileSize) break;

		if (marker == (byte)JPEGMarker::SOF0 || marker == (byte)JPEGMarker::SOF1 || marker == (byte)JPEGMarker::SOF2) {
			const byte* segment = fileData + pos + 2;
			size_t payloadLen = len - 2;
			if (payloadLen >= 6) {
				frame.precision = segment[0];
				frame.height = ReadBE16(segment + 1);
				frame.width = ReadBE16(segment + 3);
				frame.ncomp = segment[5];
				if (frame.precision == 8 && frame.width > 0 && frame.height > 0 &&
					(frame.ncomp == 1 || frame.ncomp == 3) &&
					payloadLen >= 6 + (size_t)frame.ncomp * 3) {
					for (int i = 0; i < frame.ncomp; ++i) {
						frame.comp_id[i] = segment[6 + i * 3 + 0];
						frame.comp_h[i] = (segment[6 + i * 3 + 1] >> 4) & 0x0F;
						frame.comp_v[i] = segment[6 + i * 3 + 1] & 0x0F;
						frame.comp_q[i] = segment[6 + i * 3 + 2];
					}
					hasFrame = true;
					isProgressive = (marker == (byte)JPEGMarker::SOF2);
				}
			}
			break;
		}
		pos += len;
	}

	if (!hasFrame) {
		free(ctx);
		return nullptr;
	}

	int maxH = 1;
	int maxV = 1;
	for (int i = 0; i < frame.ncomp; ++i) {
		if (frame.comp_h[i] > maxH) maxH = frame.comp_h[i];
		if (frame.comp_v[i] > maxV) maxV = frame.comp_v[i];
	}

	int mcuWidth = maxH * 8;
	int mcuHeight = maxV * 8;
	int mcusX = (frame.width + mcuWidth - 1) / mcuWidth;
	int mcusY = (frame.height + mcuHeight - 1) / mcuHeight;

	int blocksX[JPEG_MAX_COMPONENTS];
	int blocksY[JPEG_MAX_COMPONENTS];
	int totalBlocks[JPEG_MAX_COMPONENTS];
	int16* coeffBuffer[JPEG_MAX_COMPONENTS];

	for (int c = 0; c < frame.ncomp; ++c) {
		blocksX[c] = mcusX * frame.comp_h[c];
		blocksY[c] = mcusY * frame.comp_v[c];
		totalBlocks[c] = blocksX[c] * blocksY[c];
		coeffBuffer[c] = (int16*)calloc(totalBlocks[c] * 64, sizeof(int16));
		if (!coeffBuffer[c]) {
			for (int k = 0; k < c; ++k) free(coeffBuffer[k]);
			free(ctx);
			return nullptr;
		}
	}

	// Second pass: parse DQT, DHT, DRI and execute all SOS scans
	pos = 2;
	BitStream bs;

	while (pos < fileSize) {
		if (fileData[pos] != 0xFF) {
			pos++;
			continue;
		}
		while (pos < fileSize && fileData[pos] == 0xFF) {
			pos++;
		}
		if (pos >= fileSize) break;

		byte marker = fileData[pos++];
		if (marker == (byte)JPEGMarker::SOI) continue;
		if (marker == (byte)JPEGMarker::EOI) break;

		if (pos + 2 > fileSize) break;
		uint16 len = ReadBE16(fileData + pos);
		if (len < 2 || pos + len > fileSize) break;

		const byte* segment = fileData + pos + 2;
		size_t payloadLen = len - 2;

		if (marker == (byte)JPEGMarker::DQT) {
			size_t dqtPos = 0;
			while (dqtPos < payloadLen) {
				byte info = segment[dqtPos++];
				byte tableId = info & 0x0F;
				byte precision = (info >> 4) & 0x0F;
				if (tableId >= 4) break;
				if (precision == 0) {
					if (dqtPos + 64 > payloadLen) break;
					for (int i = 0; i < 64; ++i) {
						ctx->quantTables[tableId][kZigzag[i]] = segment[dqtPos + i];
					}
					ctx->quantTableValid[tableId] = true;
					dqtPos += 64;
				} else {
					if (dqtPos + 128 > payloadLen) break;
					for (int i = 0; i < 64; ++i) {
						ctx->quantTables[tableId][kZigzag[i]] = segment[dqtPos + i * 2 + 1];
					}
					ctx->quantTableValid[tableId] = true;
					dqtPos += 128;
				}
			}
		} else if (marker == (byte)JPEGMarker::DHT) {
			size_t dhtPos = 0;
			while (dhtPos < payloadLen) {
				byte info = segment[dhtPos++];
				byte tableId = info & 0x0F;
				byte tableClass = (info >> 4) & 0x0F;
				if (tableId >= 4 || dhtPos + 16 > payloadLen) break;

				byte bits[16];
				MemCopyN(bits, segment + dhtPos, 16);
				dhtPos += 16;

				int count = 0;
				for (int i = 0; i < 16; ++i) count += bits[i];
				if (dhtPos + count > payloadLen) break;

				if (tableClass == 0) {
					BuildHuffmanTable(ctx->dcTables[tableId], bits, segment + dhtPos, count);
				} else {
					BuildHuffmanTable(ctx->acTables[tableId], bits, segment + dhtPos, count);
				}
				dhtPos += count;
			}
		} else if (marker == (byte)JPEGMarker::DRI) {
			if (payloadLen >= 2) {
				restartInterval = ReadBE16(segment);
			}
		} else if (marker == (byte)JPEGMarker::SOS) {
			if (payloadLen < 1) break;
			byte scanNcomp = segment[0];
			if (payloadLen < 1 + (size_t)scanNcomp * 2 + 3) break;

			byte scanCompIndex[JPEG_MAX_COMPONENTS];
			byte scanDcTable[JPEG_MAX_COMPONENTS];
			byte scanAcTable[JPEG_MAX_COMPONENTS];

			for (int i = 0; i < scanNcomp; ++i) {
				byte compId = segment[1 + i * 2];
				byte tables = segment[1 + i * 2 + 1];
				scanCompIndex[i] = 0;
				for (int c = 0; c < frame.ncomp; ++c) {
					if (frame.comp_id[c] == compId) {
						scanCompIndex[i] = (byte)c;
						break;
					}
				}
				scanDcTable[i] = (tables >> 4) & 0x0F;
				scanAcTable[i] = tables & 0x0F;
			}

			byte ss = segment[1 + scanNcomp * 2 + 0];
			byte se = segment[1 + scanNcomp * 2 + 1];
			byte ahal = segment[1 + scanNcomp * 2 + 2];
			byte ah = (ahal >> 4) & 0x0F;
			byte al = ahal & 0x0F;

			pos += len;
			bs.Init(fileData, fileSize, pos);

			int dcPredictors[JPEG_MAX_COMPONENTS] = { 0 };
			int eobRun = 0;
			int mcuIndex = 0;

			if (scanNcomp > 1) {
				// Interleaved scan
				for (int my = 0; my < mcusY; ++my) {
					for (int mx = 0; mx < mcusX; ++mx) {
						if (restartInterval > 0 && (mcuIndex % restartInterval) == 0 && mcuIndex > 0) {
							for (int i = 0; i < scanNcomp; ++i) dcPredictors[i] = 0;
							eobRun = 0;
							bs.ResetForRestart();
						}

						for (int i = 0; i < scanNcomp; ++i) {
							int c = scanCompIndex[i];
							byte dcId = scanDcTable[i];
							byte acId = scanAcTable[i];

							for (int vy = 0; vy < frame.comp_v[c]; ++vy) {
								for (int hx = 0; hx < frame.comp_h[c]; ++hx) {
									int bx = mx * frame.comp_h[c] + hx;
									int by = my * frame.comp_v[c] + vy;
									int16* block = &coeffBuffer[c][(by * blocksX[c] + bx) * 64];

									if (!isProgressive && ss == 0 && se == 63 && ah == 0 && al == 0) {
										// Baseline full block decode
										int s = bs.DecodeHuffman(ctx->dcTables[dcId]);
										if (s < 0) goto scan_done;
										int diff = bs.ReceiveExtend(s);
										dcPredictors[i] += diff;
										block[0] = (int16)dcPredictors[i];

										int k = 1;
										while (k <= 63) {
											int rs = bs.DecodeHuffman(ctx->acTables[acId]);
											if (rs < 0) goto scan_done;
											int r = rs >> 4;
											int sVal = rs & 0x0F;
											if (sVal == 0) {
												if (r == 0) break;
												if (r == 15) { k += 16; continue; }
												break;
											}
											k += r;
											if (k > 63) break;
											int val = bs.ReceiveExtend(sVal);
											block[kZigzag[k]] = (int16)val;
											k++;
										}
									} else {
										if (!DecodeBlockSpectral(bs, ctx->dcTables[dcId], ctx->acTables[acId],
																dcPredictors[i], eobRun, block, ss, se, ah, al)) {
											goto scan_done;
										}
									}
								}
							}
						}
						mcuIndex++;
					}
				}
			} else {
				// Non-interleaved scan
				int c = scanCompIndex[0];
				byte dcId = scanDcTable[0];
				byte acId = scanAcTable[0];

				for (int by = 0; by < blocksY[c]; ++by) {
					for (int bx = 0; bx < blocksX[c]; ++bx) {
						if (restartInterval > 0 && (mcuIndex % restartInterval) == 0 && mcuIndex > 0) {
							dcPredictors[0] = 0;
							eobRun = 0;
							bs.ResetForRestart();
						}
						int16* block = &coeffBuffer[c][(by * blocksX[c] + bx) * 64];
						if (!DecodeBlockSpectral(bs, ctx->dcTables[dcId], ctx->acTables[acId],
												dcPredictors[0], eobRun, block, ss, se, ah, al)) {
							goto scan_done;
						}
						mcuIndex++;
					}
				}
			}

scan_done:
			// Step back unused whole bytes in bit buffer to locate next marker
			size_t realPos = bs.pos > (size_t)(bs.bitsCount / 8) ? (bs.pos - (bs.bitsCount / 8)) : 0;
			while (realPos < fileSize && fileData[realPos] != 0xFF) {
				realPos++;
			}
			pos = realPos;
			continue;
		}

		pos += len;
	}

	// Final Step: IDCT and color reconstruction
	byte* compSamples[JPEG_MAX_COMPONENTS] = { nullptr };
	for (int c = 0; c < frame.ncomp; ++c) {
		size_t sampleCount = (size_t)(blocksY[c] * 8) * (blocksX[c] * 8);
		compSamples[c] = (byte*)malloc(sampleCount);
		if (!compSamples[c]) {
			for (int k = 0; k < frame.ncomp; ++k) free(coeffBuffer[k]);
			for (int k = 0; k < c; ++k) free(compSamples[k]);
			free(ctx);
			return nullptr;
		}

		byte qId = frame.comp_q[c];
		const byte* qTable = ctx->quantTables[qId < 4 ? qId : 0];
		int stride = blocksX[c] * 8;

		for (int by = 0; by < blocksY[c]; ++by) {
			for (int bx = 0; bx < blocksX[c]; ++bx) {
				const int16* block = &coeffBuffer[c][(by * blocksX[c] + bx) * 64];
				int dequant[64];
				for (int k = 0; k < 64; ++k) {
					dequant[k] = block[k] * qTable[k];
				}
				byte* dest = &compSamples[c][by * 8 * stride + bx * 8];
				IDCT8x8(dequant, dest, stride);
			}
		}
		free(coeffBuffer[c]);
		coeffBuffer[c] = nullptr;
	}

	uni::Color* pixels = (uni::Color*)malloc(frame.width * frame.height * sizeof(uni::Color));
	if (!pixels) {
		for (int c = 0; c < frame.ncomp; ++c) free(compSamples[c]);
		free(ctx);
		return nullptr;
	}

	for (int py = 0; py < frame.height; ++py) {
		for (int px = 0; px < frame.width; ++px) {
			if (frame.ncomp == 3) {
				int ySampleX = px * frame.comp_h[0] / maxH;
				int ySampleY = py * frame.comp_v[0] / maxV;
				int y = compSamples[0][ySampleY * (blocksX[0] * 8) + ySampleX];

				int cbSampleX = px * frame.comp_h[1] / maxH;
				int cbSampleY = py * frame.comp_v[1] / maxV;
				int cb = compSamples[1][cbSampleY * (blocksX[1] * 8) + cbSampleX];

				int crSampleX = px * frame.comp_h[2] / maxH;
				int crSampleY = py * frame.comp_v[2] / maxV;
				int cr = compSamples[2][crSampleY * (blocksX[2] * 8) + crSampleX];

				pixels[py * frame.width + px] = YCbCrToColor(y, cb, cr);
			} else {
				int y = compSamples[0][py * (blocksX[0] * 8) + px];
				pixels[py * frame.width + px] = GrayToColor(y);
			}
		}
	}

	for (int c = 0; c < frame.ncomp; ++c) {
		free(compSamples[c]);
	}

	free(ctx);
	if (outWidth) *outWidth = frame.width;
	if (outHeight) *outHeight = frame.height;

	return pixels;
}

const char* uni::JPEGCodec::GetName() const {
	return "JPEG";
}

uni::ImageFormat uni::JPEGCodec::GetFormat() const {
	return uni::ImageFormat::JPEG;
}

const char* const* uni::JPEGCodec::GetExtensions() const {
	static const char* const extensions[] = { "jpg", "jpeg", "jpe", "jfif", nullptr };
	return extensions;
}

uni::ImageResult uni::JPEGCodec::Probe(StorageTrait& storage, bool& matched) const {
	matched = false;
	byte magic[2] = { 0, 0 };
	stduint blockSize = storage.Block_Size ? storage.Block_Size : 512;
	byte* blockBuf = (byte*)malloc(blockSize);
	if (!blockBuf) {
		return uni::ImageResult::OUT_OF_MEMORY;
	}

	stduint readBytes = storage.Read(0, magic, 2, blockBuf);
	free(blockBuf);

	// Verify JPEG SOI marker (0xFFD8)
	if (readBytes == 2 && magic[0] == 0xFF && magic[1] == (byte)JPEGMarker::SOI) {
		matched = true;
	}

	return uni::ImageResult::OK;
}

uni::ImageResult uni::JPEGCodec::ReadInfo(StorageTrait& storage, ImageInfo& outInfo) const {
	bool matched = false;
	uni::ImageResult res = Probe(storage, matched);
	if (res != uni::ImageResult::OK) {
		return res;
	}
	if (!matched) {
		return uni::ImageResult::INVALID_FORMAT;
	}

	stduint maxStorageSize = storage.getUnits() * storage.Block_Size;
	if (maxStorageSize < 4) {
		return uni::ImageResult::INVALID_FORMAT;
	}

	stduint blockSize = storage.Block_Size ? storage.Block_Size : 512;
	byte* blockBuf = (byte*)malloc(blockSize);
	if (!blockBuf) {
		return uni::ImageResult::OUT_OF_MEMORY;
	}

	stduint scanLimit = maxStorageSize > 65536 ? 65536 : maxStorageSize;
	byte* headerBuf = (byte*)malloc(scanLimit);
	if (!headerBuf) {
		free(blockBuf);
		return uni::ImageResult::OUT_OF_MEMORY;
	}

	stduint readBytes = storage.Read(0, headerBuf, scanLimit, blockBuf);
	free(blockBuf);

	if (readBytes < 4) {
		free(headerBuf);
		return uni::ImageResult::INVALID_FORMAT;
	}

	size_t pos = 2;
	while (pos < readBytes) {
		if (headerBuf[pos] != 0xFF) {
			pos++;
			continue;
		}
		while (pos < readBytes && headerBuf[pos] == 0xFF) {
			pos++;
		}
		if (pos >= readBytes) break;

		byte marker = headerBuf[pos++];
		if (marker == (byte)JPEGMarker::SOI) continue;
		if (marker == (byte)JPEGMarker::EOI) break;

		if (pos + 2 > readBytes) break;
		uint16 len = ReadBE16(headerBuf + pos);
		if (len < 2) break;

		if (marker == (byte)JPEGMarker::SOF0 || marker == (byte)JPEGMarker::SOF1 || marker == (byte)JPEGMarker::SOF2) {
			if (pos + 2 + 6 <= readBytes) {
				const byte* segment = headerBuf + pos + 2;
				uint16 height = ReadBE16(segment + 1);
				uint16 width = ReadBE16(segment + 3);
				byte ncomp = segment[5];

				outInfo.width = width;
				outInfo.height = height;
				outInfo.format = PixelFormat::ARGB8888;
				outInfo.colorSpace = (ncomp == 1) ? ColorSpace::GRAY : ColorSpace::SRGB;
				outInfo.alphaMode = ImageAlphaMode::NONE;
				outInfo.fileFormat = ImageFormat::JPEG;
				outInfo.bitsPerPixel = (ncomp == 1) ? 8 : 24;
				outInfo.frameCount = 1;
				outInfo.hasAlpha = false;
				outInfo.hasAnimation = false;

				free(headerBuf);
				return uni::ImageResult::OK;
			}
			break;
		}

		pos += len;
	}

	free(headerBuf);
	return uni::ImageResult::INVALID_FORMAT;
}

namespace {

	class JPEGSurface : public uni::IImageSurface {
	private:
		uni::StorageTrait* storage;
		uni::ImageInfo     info;
		uni::trait::Malloc* allocator;

	public:
		JPEGSurface(uni::StorageTrait& stg, const uni::ImageInfo& inf, uni::trait::Malloc& alloc)
			: storage(&stg), info(inf), allocator(&alloc) {
		}

		virtual ~JPEGSurface() = default;

		virtual void Release() override {
			uni::trait::Malloc* alloc = allocator;
			this->~JPEGSurface();
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

			stduint maxStorageSize = storage->getUnits() * storage->Block_Size;
			if (maxStorageSize < 4) return uni::ImageResult::INVALID_FORMAT;

			stduint blockSize = storage->Block_Size ? storage->Block_Size : 512;
			byte* blockBuf = (byte*)malloc(blockSize);
			if (!blockBuf) return uni::ImageResult::OUT_OF_MEMORY;

			byte* fileData = (byte*)malloc(maxStorageSize);
			if (!fileData) {
				free(blockBuf);
				return uni::ImageResult::OUT_OF_MEMORY;
			}

			stduint totalRead = storage->Read(0, fileData, maxStorageSize, blockBuf);
			free(blockBuf);

			int decWidth = 0, decHeight = 0;
			uni::Color* fullPixels = DecodeJPEG(fileData, totalRead, &decWidth, &decHeight);
			free(fileData);

			if (!fullPixels) {
				return uni::ImageResult::FAILED;
			}

			size_t neededSize = (size_t)rw * (size_t)rh * sizeof(uni::Color);
			void* pixelsMem = outBuffer.pixels;
			bool ownsMem = false;
			if (!pixelsMem) {
				pixelsMem = alloc.allocate(neededSize);
				if (!pixelsMem) {
					free(fullPixels);
					return uni::ImageResult::OUT_OF_MEMORY;
				}
				ownsMem = true;
			}

			uni::Color* outPixels = (uni::Color*)pixelsMem;
			for (int line = 0; line < rh; ++line) {
				int srcY = ry + line;
				MemCopyN(outPixels + line * rw, fullPixels + srcY * decWidth + rx, rw * sizeof(uni::Color));
			}

			free(fullPixels);

			outBuffer.width = (uint32)rw;
			outBuffer.height = (uint32)rh;
			outBuffer.stride = (uint32)(rw * sizeof(uni::Color));
			outBuffer.format = uni::PixelFormat::ARGB8888;
			outBuffer.colorSpace = uni::ColorSpace::SRGB;
			outBuffer.alphaMode = uni::ImageAlphaMode::NONE;
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

uni::ImageResult uni::JPEGCodec::OpenSurface(
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

	void* mem = allocator.allocate(sizeof(JPEGSurface));
	if (!mem) return uni::ImageResult::OUT_OF_MEMORY;

	JPEGSurface* surf = new (mem) JPEGSurface(storage, info, allocator);
	outSurface = surf;
	return uni::ImageResult::OK;
}

uni::ImageResult uni::JPEGCodec::Decode(
	StorageTrait& storage,
	ImageBuffer& outBuffer,
	trait::Malloc& allocator,
	const ImageDecodeOptions& options
) const {
	bool matched = false;
	uni::ImageResult res = Probe(storage, matched);
	if (res != uni::ImageResult::OK) {
		return res;
	}
	if (!matched) {
		return uni::ImageResult::INVALID_FORMAT;
	}

	stduint maxStorageSize = storage.getUnits() * storage.Block_Size;
	if (maxStorageSize < 4) {
		return uni::ImageResult::INVALID_FORMAT;
	}

	stduint blockSize = storage.Block_Size ? storage.Block_Size : 512;
	byte* blockBuf = (byte*)malloc(blockSize);
	if (!blockBuf) {
		return uni::ImageResult::OUT_OF_MEMORY;
	}

	byte* fileData = (byte*)malloc(maxStorageSize);
	if (!fileData) {
		free(blockBuf);
		return uni::ImageResult::OUT_OF_MEMORY;
	}

	stduint totalRead = storage.Read(0, fileData, maxStorageSize, blockBuf);
	free(blockBuf);

	int outWidth = 0;
	int outHeight = 0;
	uni::Color* stdPixels = DecodeJPEG(fileData, totalRead, &outWidth, &outHeight);
	free(fileData);

	if (!stdPixels) {
		return uni::ImageResult::FAILED;
	}

	size_t pixelBufferSize = (size_t)outWidth * (size_t)outHeight * sizeof(uni::Color);
	void* targetPixels = allocator.allocate(pixelBufferSize, 3, 0);
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
	outBuffer.alphaMode = ImageAlphaMode::NONE;
	outBuffer.pixels = targetPixels;
	outBuffer.size = pixelBufferSize;
	outBuffer.allocator = &allocator;

	return uni::ImageResult::OK;
}

uni::ImageResult uni::JPEGCodec::Encode(
	const ImageBuffer& image,
	StorageTrait& storage,
	trait::Malloc& allocator,
	const ImageEncodeOptions& options
) const {
	return uni::ImageResult::UNSUPPORTED;
}

bool uni::JPEGCodec::CanEncode(PixelFormat format) const {
	return false;
}

// ==================== JPEGCodecHard (hardware, H7 only) ====================

// On non-H7 targets there is no hardware JPEG; every operation is UNSUPPORTED.
const char* uni::JPEGCodecHard::GetName() const {
	return "jpeg-hard";
}

uni::ImageFormat uni::JPEGCodecHard::GetFormat() const {
	return uni::ImageFormat::JPEG;
}

const char* const* uni::JPEGCodecHard::GetExtensions() const {
	static const char* const exts[] = { "jpg", "jpeg", nullptr };
	return exts;
}

uni::ImageResult uni::JPEGCodecHard::Probe(StorageTrait& storage, bool& matched) const {
	// JPEG file starts with SOI marker 0xFF 0xD8.
	matched = false;
	byte b[2];
	byte block[512];
	if (storage.Read(0, b, 2, block) != 2) return uni::ImageResult::IO_ERROR;
	if (b[0] == 0xFF && b[1] == 0xD8) matched = true;
	return uni::ImageResult::OK;
}

uni::ImageResult uni::JPEGCodecHard::ReadInfo(StorageTrait& storage, ImageInfo& outInfo) const {
	// Read the whole file into memory, decode with hardware, then report dims.
	// (Simplified: reuse Decode's path but only fill ImageInfo.)
	ImageBuffer buf;
	uni::ImageBufferClear(buf);
	// need an allocator; use a local heap via the injected jpeg driver not needed.
	// Simplest: full decode then copy metadata.
	// Use a temporary Malloc wrapper over a static buffer is messy; instead decode into
	// a caller-provided buffer is not available here, so return UNSUPPORTED for ReadInfo
	// unless we implement a light header parse. We parse the JPEG header (SOF) directly.
	byte hdr[256];
	byte block[512];
	stduint maxStorageSize = storage.getUnits() * storage.Block_Size;
	if (maxStorageSize < 4) return uni::ImageResult::INVALID_FORMAT;
	stduint toRead = maxStorageSize < sizeof(hdr) ? maxStorageSize : sizeof(hdr);
	stduint got = storage.Read(0, hdr, toRead, block);
	// scan for SOF0/SOF2 marker (0xFFC0 / 0xFFC2) to read height/width/ns
	stduint i = 2;
	while (i + 8 < got) {
		if (hdr[i] != 0xFF) { i++; continue; }
		byte m = hdr[i + 1];
		if (m == 0xC0 || m == 0xC1 || m == 0xC2 || m == 0xC3) {
			outInfo.height = ((const BigEndian<uint16, true>*)&hdr[i + 5])->get();
			outInfo.width  = ((const BigEndian<uint16, true>*)&hdr[i + 7])->get();
			byte ns = hdr[i + 9];
			outInfo.format = PixelFormat::ARGB8888;
			outInfo.colorSpace = (ns == 1) ? ColorSpace::GRAY : (ns == 4 ? ColorSpace::CMYK : ColorSpace::SRGB);
			outInfo.alphaMode = ImageAlphaMode::NONE;
			outInfo.fileFormat = ImageFormat::JPEG;
			outInfo.bitsPerPixel = 24;
			outInfo.frameCount = 1;
			outInfo.hasAlpha = false;
			outInfo.hasAnimation = false;
			return uni::ImageResult::OK;
		}
		// skip segment
		if (m == 0xD8 || m == 0xD9) { i += 2; continue; }
		stduint segLen = ((const BigEndian<uint16, true>*)&hdr[i + 2])->get();
		i += 2 + segLen;
	}
	return uni::ImageResult::INVALID_FORMAT;
}

uni::ImageResult uni::JPEGCodecHard::OpenSurface(
	StorageTrait& storage,
	IImageSurface*& outSurface,
	trait::Malloc& allocator,
	const ImageDecodeOptions& options,
	ImageAccessMode access
) const {
	(void)storage; (void)outSurface; (void)allocator; (void)options; (void)access;
	return uni::ImageResult::UNSUPPORTED;
}

// YCbCr -> RGB888 (single pixel), integer BT.601 coefficients (scaled <<10).
static inline void YCbCrToRGB(int y, int cb, int cr, byte& r, byte& g, byte& b) {
	int yy = y << 10;
	int rr = yy + 1436 * (cr - 128);
	int gg = yy - 352 * (cb - 128) - 731 * (cr - 128);
	int bb = yy + 1815 * (cb - 128);
	rr >>= 10; gg >>= 10; bb >>= 10;
	if (rr < 0) rr = 0; else if (rr > 255) rr = 255;
	if (gg < 0) gg = 0; else if (gg > 255) gg = 255;
	if (bb < 0) bb = 0; else if (bb > 255) bb = 255;
	r = (byte)rr; g = (byte)gg; b = (byte)bb;
}

uni::ImageResult uni::JPEGCodecHard::Decode(
	StorageTrait& storage,
	ImageBuffer& outBuffer,
	trait::Malloc& allocator,
	const ImageDecodeOptions& options
) const {
#if !defined(_MCU_STM32H7x)
	(void)storage; (void)outBuffer; (void)allocator; (void)options;
	return uni::ImageResult::UNSUPPORTED;
#else
	// Read whole JPEG stream.
	stduint maxStorageSize = storage.getUnits() * storage.Block_Size;
	if (maxStorageSize < 4) return uni::ImageResult::INVALID_FORMAT;
	byte* stream = (byte*)allocator.allocate(maxStorageSize, 3, 0);
	if (!stream) return uni::ImageResult::OUT_OF_MEMORY;
	byte block[512];
	stduint got = storage.Read(0, stream, maxStorageSize, block);
	if (got < 4) { allocator.deallocate(stream, maxStorageSize); return uni::ImageResult::INVALID_FORMAT; }

	// Initialize hardware + header parsing.
	if (!jpeg.setMode()) { allocator.deallocate(stream, maxStorageSize); return uni::ImageResult::FAILED; }

	// Parse dimensions + subsampling from the JPEG stream SOF segment.
	stduint width = 0, height = 0;
	JPEGSubsampling subsampling = JPEGSubsampling::_444;
	{
		stduint i = 2;
		while (i + 12 < got) {
			if (stream[i] != 0xFF) { i++; continue; }
			byte m = stream[i + 1];
			if (m == 0xC0) {
				// Big-endian fields; packed=true reads byte-by-byte (no unaligned LDRH).
				height = ((const BigEndian<uint16, true>*)&stream[i + 5])->get();
				width  = ((const BigEndian<uint16, true>*)&stream[i + 7])->get();
				byte ns = stream[i + 9];
				// subsampling from first component's H/V sampling factors (byte 11/12)
				if (ns >= 1) {
					byte hv = stream[i + 11];
					byte h = (hv >> 4) & 0xF, v = hv & 0xF;
					if (h == 2 && v == 2) subsampling = JPEGSubsampling::_420;
					else if (h == 2 && v == 1) subsampling = JPEGSubsampling::_422;
					else subsampling = JPEGSubsampling::_444;
				}
				break;
			}
			// SOF1(0xC1)/SOF2(0xC2)/SOF3(0xC3) are extended/progressive/lossless:
			// H7 hardware JPEG only decodes baseline (SOF0). Reject them.
			if (m == 0xC1 || m == 0xC2 || m == 0xC3 || m == 0xC5 || m == 0xC6 ||
				m == 0xC7 || m == 0xC9 || m == 0xCA || m == 0xCB || m == 0xCD ||
				m == 0xCE || m == 0xCF) {
				allocator.deallocate(stream, maxStorageSize);
				return uni::ImageResult::INVALID_FORMAT;
			}
			if (m == 0xD8 || m == 0xD9) { i += 2; continue; }
			stduint segLen = ((const BigEndian<uint16, true>*)&stream[i + 2])->get();
			if (segLen < 2) break;// malformed segment length, stop parsing
			i += 2 + segLen;
		}
	}
	if (!width || !height) { allocator.deallocate(stream, maxStorageSize); return uni::ImageResult::INVALID_FORMAT; }

	// XART1.OutFormat("W=%u H=%u\n", (unsigned)width, (unsigned)height);

	// Hardware decode into a YCbCr buffer (JPEG outputs interleaved YCbCr blocks).
	// Output buffer size: worst case ~3 bytes/pixel.
	stduint ycbcrSize = width * height * 3 + 64;
	// XART1.OutFormat("ycbcrSize=%u\n", (unsigned)ycbcrSize);
	byte* ycbcr = (byte*)allocator.allocate(ycbcrSize, 3, 0);// 8-byte aligned (MDMA/DMA2D)
	if (!ycbcr) { allocator.deallocate(stream, maxStorageSize); return uni::ImageResult::OUT_OF_MEMORY; }

	jpeg.inn_buffer = { (stduint)stream, got };
	jpeg.out_buffer = { (stduint)ycbcr, ycbcrSize };
	if (!jpeg.Decode(stream, got, ycbcr, ycbcrSize, IOMethod::Loop)) {
		// XART1.OutFormat("jpeg.Decode FAIL: err=0x%X state=%d\n",
		// 	(unsigned)jpeg.getError(), (int)jpeg.getState());
		// XART1.OutFormat("SR=0x%X CR=0x%X CONFR0=0x%X CONFR1=0x%X CONFR2=0x%X CONFR3=0x%X\n",
		// 	(unsigned)(stduint)jpeg[JPEGReg::SR], (unsigned)(stduint)jpeg[JPEGReg::CR],
		// 	(unsigned)(stduint)jpeg[JPEGReg::CONFR0], (unsigned)(stduint)jpeg[JPEGReg::CONFR1],
		// 	(unsigned)(stduint)jpeg[JPEGReg::CONFR2], (unsigned)(stduint)jpeg[JPEGReg::CONFR3]);
		// XART1.OutFormat("tail: got=%u [%02X %02X %02X %02X]\n", (unsigned)got,
		// 	(unsigned)stream[got - 4], (unsigned)stream[got - 3],
		// 	(unsigned)stream[got - 2], (unsigned)stream[got - 1]);
		// XART1.OutFormat("CONFR4=0x%X CONFR5=0x%X CONFR6=0x%X\n",
		// 	(unsigned)(stduint)jpeg[JPEGReg::CONFR4],
		// 	(unsigned)(stduint)jpeg[JPEGReg::CONFR5],
		// 	(unsigned)(stduint)jpeg[JPEGReg::CONFR6]);
		allocator.deallocate(stream, maxStorageSize); allocator.deallocate(ycbcr, ycbcrSize);
		return uni::ImageResult::FAILED;
	}

	// Output pixel format: follow user's preference; default to RGBA8888 (ARGB8888).
	PixelFormat outFmt = options.preferredFormat;
	if (outFmt != PixelFormat::RGB888 && outFmt != PixelFormat::RGB565 &&
		outFmt != PixelFormat::RGBA8888 && outFmt != PixelFormat::ARGB8888)
		outFmt = PixelFormat::ARGB8888;

	// DMA2D: YCbCr interleaved -> RGB (hardware color conversion).
	// Map JPEGSubsampling -> DMA2D ChromaSubSampling.
	uni::ChromaSubSampling css;
	switch (subsampling) {
	case JPEGSubsampling::_420: css = uni::ChromaSubSampling::_420; break;
	case JPEGSubsampling::_422: css = uni::ChromaSubSampling::_422; break;
	default:                    css = uni::ChromaSubSampling::None; break;
	}

	uni::DMA2D_LAYER_t::LayerPara lp;
	lp.pixel_format = PixelFormat::YCbCr;
	lp.chroma_sub_sampling = css;
	lp.alpha_mode = uni::AlphaMode::NoModification;
	lp.input_alpha = 0xFF;

	size_t outBytesPerPx = 4;
	switch (outFmt) {
	case PixelFormat::RGB888: outBytesPerPx = 3; break;
	case PixelFormat::RGB565: outBytesPerPx = 2; break;
	default: outBytesPerPx = 4; break;
	}
	size_t rgbSize = (size_t)width * height * outBytesPerPx;
	void* rgb = allocator.allocate(rgbSize, 3, 0);// 8-byte aligned (DMA2D)
	if (!rgb) { allocator.deallocate(stream, maxStorageSize); allocator.deallocate(ycbcr, ycbcrSize); return uni::ImageResult::OUT_OF_MEMORY; }

	// Configure DMA2D: M2M with pixel format conversion, foreground = YCbCr source.
	if (!DMA2D.setMode(uni::DMA2DMode::M2MPFC, outFmt)) {
		allocator.deallocate(rgb, rgbSize); allocator.deallocate(stream, maxStorageSize); allocator.deallocate(ycbcr, ycbcrSize);
		return uni::ImageResult::FAILED;
	}
	DMA2D[1].setMode(lp);
	// Transfer whole image: source YCbCr (input_offset 0), dst rgb.
	if (!DMA2D.Transfer((pureptr_t)ycbcr, (pureptr_t)rgb, width, height, 0, IOMethod::Loop)) {
		allocator.deallocate(rgb, rgbSize); allocator.deallocate(stream, maxStorageSize); allocator.deallocate(ycbcr, ycbcrSize);
		return uni::ImageResult::FAILED;
	}

	allocator.deallocate(stream, maxStorageSize);
	allocator.deallocate(ycbcr, ycbcrSize);

	outBuffer.width = (uint32)width;
	outBuffer.height = (uint32)height;
	outBuffer.stride = width * outBytesPerPx;
	outBuffer.format = outFmt;
	outBuffer.colorSpace = ColorSpace::SRGB;
	outBuffer.alphaMode = ImageAlphaMode::NONE;
	outBuffer.pixels = rgb;
	outBuffer.size = rgbSize;
	outBuffer.allocator = &allocator;

	return uni::ImageResult::OK;
#endif
}

// RGB -> YCbCr (single pixel), integer BT.601 (scaled <<10). Returns packed YCbCr (24-bit).
static inline uint32 RGBToYCbCr(byte r, byte g, byte b) {
	int yy = (66 * (int)r + 129 * (int)g + 25 * (int)b + 128) >> 8;
	int cb = (-38 * (int)r - 74 * (int)g + 112 * (int)b + 128) >> 8;
	int cr = (112 * (int)r - 94 * (int)g - 18 * (int)b + 128) >> 8;
	yy += 16; cb += 128; cr += 128;
	return ((uint32)(byte)yy) | ((uint32)(byte)cb << 8) | ((uint32)(byte)cr << 16);
}

uni::ImageResult uni::JPEGCodecHard::Encode(
	const ImageBuffer& image,
	StorageTrait& storage,
	trait::Malloc& allocator,
	const ImageEncodeOptions& options
) const {
#if !defined(_MCU_STM32H7x)
	(void)image; (void)storage; (void)allocator; (void)options;
	return uni::ImageResult::UNSUPPORTED;
#else
	(void)allocator;
	if (!image.pixels || !image.width || !image.height) return uni::ImageResult::INVALID_ARGUMENT;
	// Configure hardware encoder from the image.
	jpeg.image_width = image.width;
	jpeg.image_height = image.height;
	jpeg.image_quality = options.quality ? (byte)options.quality : 90;
	// Choose color space / subsampling from image format.
	bool gray = false;
	switch (image.format) {
	case PixelFormat::L8:
		jpeg.color_space = JPEGColorSpace::Gray;
		jpeg.subsampling = JPEGSubsampling::_444;
		gray = true;
		break;
	case PixelFormat::RGB888:
	case PixelFormat::RGBA8888:
	case PixelFormat::ARGB8888:
		jpeg.color_space = JPEGColorSpace::YCbCr;
		jpeg.subsampling = JPEGSubsampling::_444;
		break;
	default:
		return uni::ImageResult::INVALID_ARGUMENT;
	}
	if (!jpeg.setMode()) return uni::ImageResult::FAILED;
	if (!jpeg.ConfigEncoding()) return uni::ImageResult::FAILED;

	// Convert RGB(A) pixels to YCbCr interleaved (Y,Cb,Cr per pixel, 444 here).
	// For 4:4:4 the hardware encoder consumes one YCbCr triple per pixel.
	size_t pxCount = (size_t)image.width * image.height;
	size_t ycbcrSize = pxCount * 3;
	byte* ycbcr = (byte*)allocator.allocate(ycbcrSize, 3, 0);
	if (!ycbcr) return uni::ImageResult::OUT_OF_MEMORY;

	const byte* src = (const byte*)image.pixels;
	size_t stride = image.stride ? image.stride : (size_t)image.width * (image.format == PixelFormat::RGB888 ? 3 : 4);
	for (size_t y = 0; y < image.height; y++) {
		const byte* row = src + y * stride;
		for (size_t x = 0; x < image.width; x++) {
			size_t dstIdx = (y * image.width + x) * 3;
			if (gray) {
				byte g0 = row[x];
				ycbcr[dstIdx] = g0;
				ycbcr[dstIdx + 1] = 128;
				ycbcr[dstIdx + 2] = 128;
			} else {
				// little-endian Color layout: b,g,r,a (see color.h union)
				byte b = row[x * 4 + 0];
				byte g = row[x * 4 + 1];
				byte r = row[x * 4 + 2];
				uint32 p = RGBToYCbCr(r, g, b);
				ycbcr[dstIdx] = (byte)(p & 0xFF);
				ycbcr[dstIdx + 1] = (byte)((p >> 8) & 0xFF);
				ycbcr[dstIdx + 2] = (byte)((p >> 16) & 0xFF);
			}
		}
	}

	// Hardware encode: YCbCr input -> JPEG stream.
	byte* jpegOut = (byte*)allocator.allocate(pxCount + 1024, 3, 0);
	if (!jpegOut) { allocator.deallocate(ycbcr, ycbcrSize); return uni::ImageResult::OUT_OF_MEMORY; }
	bool ok = jpeg.Encode(ycbcr, ycbcrSize, jpegOut, pxCount + 1024, IOMethod::Loop);
	allocator.deallocate(ycbcr, ycbcrSize);
	if (!ok) { allocator.deallocate(jpegOut, pxCount + 1024); return uni::ImageResult::FAILED; }

	// Write JPEG stream into storage.
	// Determine written length: out_buffer.length reflects consumed output.
	stduint outLen = jpeg.out_buffer.length;
	byte block[512];
	stduint wlen = storage.Write(0, jpegOut, outLen, block);
	allocator.deallocate(jpegOut, pxCount + 1024);
	if (wlen != outLen) return uni::ImageResult::IO_ERROR;

	return uni::ImageResult::OK;
#endif
}

bool uni::JPEGCodecHard::CanEncode(PixelFormat format) const {
	switch (format) {
	case PixelFormat::L8:
	case PixelFormat::RGB888:
	case PixelFormat::RGBA8888:
	case PixelFormat::ARGB8888:
		return true;
	default:
		return false;
	}
}

