// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: Picture
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
/*
	Copyright 2023 ArinaMgk

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0
	http://unisym.org/license.html

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/

#include "../../../inc/cpp/System/Picture.hpp"

namespace uni {
namespace PictureOperation {

	void FitAspect(stduint src_w, stduint src_h, stduint box_w, stduint box_h, stduint& dst_w, stduint& dst_h) {
		if (!src_w || !src_h || !box_w || !box_h) {
			dst_w = 1;
			dst_h = 1;
			return;
		}
		if (src_w * box_h > box_w * src_h) {
			// width-constrained
			dst_w = box_w;
			dst_h = src_h * box_w / src_w;
			if (dst_h == 0) dst_h = 1;
		} else {
			// height-constrained
			dst_h = box_h;
			dst_w = src_w * box_h / src_h;
			if (dst_w == 0) dst_w = 1;
		}
	}

	namespace {

		// Bytes per pixel of a supported format; returns 0 for unsupported.
		stduint PixelSize(PixelFormat fmt) {
			switch (fmt) {
			case PixelFormat::RGB565:   return 2;
			case PixelFormat::RGB888:   return 3;
			case PixelFormat::RGBA8888: return 4;
			case PixelFormat::ARGB8888: return 4;
			case PixelFormat::L8:       return 1;
			default:                    return 0;
			}
		}

		// Read one pixel from a row pointer and convert it to a common Color value.
		Color ReadPixel(const byte* row, stduint x, PixelFormat fmt) {
			switch (fmt) {
			case PixelFormat::RGB565: {
				uint16 v = ((const uint16*)row)[x];
				// 5:6:5 -> 8:8:8
				byte r = (byte)((v >> 11) & 0x1F); r = (byte)((r << 3) | (r >> 2));
				byte g = (byte)((v >> 5) & 0x3F);  g = (byte)((g << 2) | (g >> 4));
				byte b = (byte)(v & 0x1F);         b = (byte)((b << 3) | (b >> 2));
				Color c; c.r = r; c.g = g; c.b = b; c.a = 0xFF;
				return c;
			}
			case PixelFormat::RGB888: {
				const byte* p = row + x * 3;
				Color c; c.r = p[0]; c.g = p[1]; c.b = p[2]; c.a = 0xFF;
				return c;
			}
			case PixelFormat::RGBA8888: {
				const byte* p = row + x * 4;
				Color c; c.r = p[0]; c.g = p[1]; c.b = p[2]; c.a = p[3];
				return c;
			}
			case PixelFormat::ARGB8888: {
				// Memory layout is B, G, R, A (matches Color field order b, g, r, a).
				const Color* p = (const Color*)(row + x * 4);
				return *p;
			}
			case PixelFormat::L8: {
				byte v = row[x];
				Color c; c.r = v; c.g = v; c.b = v; c.a = 0xFF;
				return c;
			}
			default:
				return Color();
			}
		}

		// Write one Color into a row pointer in the given format.
		void WritePixel(byte* row, stduint x, PixelFormat fmt, Color c) {
			switch (fmt) {
			case PixelFormat::RGB565: {
				((uint16*)row)[x] = c.ToRGB565();
				break;
			}
			case PixelFormat::RGB888: {
				byte* p = row + x * 3;
				p[0] = c.r; p[1] = c.g; p[2] = c.b;
				break;
			}
			case PixelFormat::RGBA8888: {
				byte* p = row + x * 4;
				p[0] = c.r; p[1] = c.g; p[2] = c.b; p[3] = c.a;
				break;
			}
			case PixelFormat::ARGB8888: {
				Color* p = (Color*)(row + x * 4);
				*p = c;
				break;
			}
			case PixelFormat::L8: {
				// Luminance (BT.601 grayscale).
				stduint y = (stduint)((66 * (int)c.r + 129 * (int)c.g + 25 * (int)c.b + 128) >> 8);
				row[x] = (byte)(y > 255 ? 255 : y);
				break;
			}
			default:
				break;
			}
		}

	} // anonymous namespace

	ImageResult ScaleNearest(const ImageBuffer& src, ImageBuffer& dst, stduint dst_w, stduint dst_h, PixelFormat dst_format, trait::Malloc& allocator) {
		ImageBufferClear(dst);
		if (!src.pixels || !src.width || !src.height || dst_w == 0 || dst_h == 0) {
			return ImageResult::INVALID_ARGUMENT;
		}
		if (PixelSize(src.format) == 0 || PixelSize(dst_format) == 0) {
			return ImageResult::INVALID_ARGUMENT;
		}

		stduint src_bpp = PixelSize(src.format);
		stduint dst_bpp = PixelSize(dst_format);

		size_t dst_size = (size_t)dst_w * dst_h * dst_bpp;
		void* dst_pixels = allocator.allocate(dst_size, 3, 0);
		if (!dst_pixels) {
			return ImageResult::OUT_OF_MEMORY;
		}

		const byte* src_rows = (const byte*)src.pixels;
		stduint src_stride = src.stride ? src.stride : src.width * src_bpp;
		byte* dst_rows = (byte*)dst_pixels;
		stduint dst_stride = dst_w * dst_bpp;

		for (stduint dy = 0; dy < dst_h; ++dy) {
			stduint sy = dy * src.height / dst_h;
			if (sy >= src.height) sy = src.height - 1;
			const byte* src_row = src_rows + sy * src_stride;
			byte* dst_row = dst_rows + dy * dst_stride;
			for (stduint dx = 0; dx < dst_w; ++dx) {
				stduint sx = dx * src.width / dst_w;
				if (sx >= src.width) sx = src.width - 1;
				WritePixel(dst_row, dx, dst_format, ReadPixel(src_row, sx, src.format));
			}
		}

		dst.width = (uint32)dst_w;
		dst.height = (uint32)dst_h;
		dst.stride = (uint32)dst_stride;
		dst.format = dst_format;
		dst.colorSpace = src.colorSpace;
		dst.alphaMode = src.alphaMode;
		dst.pixels = dst_pixels;
		dst.size = dst_size;
		dst.allocator = &allocator;

		return ImageResult::OK;
	}

} // namespace PictureOperation
} // namespace uni
