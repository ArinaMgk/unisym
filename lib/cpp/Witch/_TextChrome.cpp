// ASCII CPP TAB4 CRLF
// Docutitle: (Module) uni.Witch.Control.TextChrome
// Codifiers: @dosconio: 20260702
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#include "../../../inc/cpp/Witch/TextChrome.hpp"
#include "../../../inc/cpp/reference"

#include "../../../inc/c/data.h"

namespace uni {
	
	// -------------------------------------------------------------------------
	// BitmapFontEngine Implementation
	// -------------------------------------------------------------------------
	void BitmapFontEngine::DrawChar(
		Color* pixel_buffer,
		stduint pitch_pixels,
		stduint px_x,
		stduint px_y,
		uint32 unicode_char,
		Color fg,
		Color bg
	) const {
		stduint font_w = typ ? 8 : 5;
		stduint font_h = typ ? 16 : 8;

		if (bg.a != 0) {
			for0(scan_y, font_h) {
				Color* dst = pixel_buffer + (px_y + scan_y) * pitch_pixels + px_x;
				for0(px, font_w) dst[px] = bg;
			}
		}

		char ch = (char)(unicode_char & 0xFF);
		if (!ascii_isprint(ch)) return;
		ch -= 0x20;

		if (typ == 1) {
			const uint16(*datptr) = (const uint16(*)) &_BITFONT_ASCII_16x8[(byte)ch];
			uint16 dat = 0;
			Reference_T<uint16> dat_bmap _IMM(&dat);
			for0(i, 8) {
				dat = datptr[i];
				for0r(j, 16) {
					if (dat_bmap.bitof(j)) {
						pixel_buffer[(px_y + (j ^ 0b111)) * pitch_pixels + px_x + i] = fg;
					}
				}
			}
		}
		else {
			const uint8(*datptr) = (const uint8(*)) &_BITFONT_ASCII_8x5[(byte)ch];
			uint8 dat = 0;
			Reference_T<uint8> dat_bmap _IMM(&dat);
			for0(i, 5) {
				dat = datptr[i];
				for0r(j, 8) {
					if (dat_bmap.bitof(j)) {
						pixel_buffer[(px_y + j) * pitch_pixels + px_x + i] = fg;
					}
				}
			}
		}
	}

	Color BitmapFontEngine::GetPixel(
		uint32 unicode_char,
		stduint gx,
		stduint gy,
		Color fg,
		Color bg
	) const {
		char ch = (char)(unicode_char & 0xFF);
		if (!ascii_isprint(ch)) return bg;
		ch -= 0x20;

		if (typ == 1) {
			uint16 dat = ((const uint16(*)) &_BITFONT_ASCII_16x8[(byte)ch])[gx];
			return ((dat >> (gy ^ 0b111)) & 1) ? fg : bg;
		}
		else {
			uint8 dat = ((const uint8(*)) &_BITFONT_ASCII_8x5[(byte)ch])[gx];
			return ((dat >> gy) & 1) ? fg : bg;
		}
	}

	Size2 BitmapFontEngine::GetCellSize() const {
		return typ ? Size2(8, 16) : Size2(5, 8);
	}

namespace witch::control {

	TextChrome::TextChrome() : default_font_engine(1), font_engine(&default_font_engine) {}

	void TextChrome::RasterCellToPixels(const FontEngine* font_engine, Color* pixels, stduint pitch_pixels, stduint cell_x_pixels, uint32 unicode_char, Color fg, Color bg) {
		if (!pixels || !font_engine) return;
		font_engine->DrawChar(pixels, pitch_pixels, cell_x_pixels, 0, unicode_char, fg, bg);
	}

	void TextChrome::BlitPixelLine(
		Color* dst_pixels,
		stduint dst_pitch_pixels,
		stduint dst_x_pixels,
		stduint dst_y_pixels,
		const Color* src_pixels,
		stduint src_pitch_pixels,
		stduint copy_width_pixels,
		stduint copy_height_pixels
	) {
		if (!dst_pixels || !src_pixels) return;
		for (stduint sy = 0; sy < copy_height_pixels; sy++) {
			Color* dst = dst_pixels + (dst_y_pixels + sy) * dst_pitch_pixels + dst_x_pixels;
			const Color* src = src_pixels + sy * src_pitch_pixels;
			for (stduint sx = 0; sx < copy_width_pixels; sx++) dst[sx] = src[sx];
		}
	}

	void TextChrome::setFontEngine(const FontEngine* fe) {
		font_engine = fe ? fe : &default_font_engine;
	}

}
}
