// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Module) uni.Witch.Control.TextChrome
// Codifiers: @dosconio: 2024
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#ifndef _INC_WITCH_CTRL_TEXTCHROME
#define _INC_WITCH_CTRL_TEXTCHROME

#include "Control.hpp"
#include "../Device/_Video.hpp"

namespace uni {

	// Abstract font rendering engine interface
	class FontEngine {
	public:
		virtual ~FontEngine() {}
		virtual Size2 GetCellSize() const = 0;
		virtual void DrawChar(Color* pixel_buffer, stduint pitch_pixels, stduint px_x, stduint px_y, uint32 unicode_char, Color fg, Color bg) const = 0;
		virtual Color GetPixel(uint32 unicode_char, stduint gx, stduint gy, Color fg, Color bg) const = 0;
	};

	// Default bitmap font engine supporting 8x5 and 16x8 formats
	class BitmapFontEngine : public FontEngine {
	protected:
		stduint typ; // 0: 8x5, 1: 16x8
	public:
		BitmapFontEngine(stduint type = 1) : typ(type) {}
		virtual ~BitmapFontEngine() {}
		virtual Size2 GetCellSize() const override;
		virtual void DrawChar(Color* pixel_buffer, stduint pitch_pixels, stduint px_x, stduint px_y, uint32 unicode_char, Color fg, Color bg) const override;
		virtual Color GetPixel(uint32 unicode_char, stduint gx, stduint gy, Color fg, Color bg) const override;
	};

namespace witch::control {

	class TextChrome {
	protected:
		BitmapFontEngine default_font_engine;
		const FontEngine* font_engine = nullptr;

	public:
		TextChrome();
		virtual ~TextChrome() {}

	public:
		static void RasterCellToPixels(const FontEngine* font_engine, Color* pixels, stduint pitch_pixels, stduint cell_x_pixels, uint32 unicode_char, Color fg, Color bg);
		static void BlitPixelLine(Color* dst_pixels, stduint dst_pitch_pixels, stduint dst_x_pixels, stduint dst_y_pixels, const Color* src_pixels, stduint src_pitch_pixels, stduint copy_width_pixels, stduint copy_height_pixels);
		void setFontEngine(const FontEngine* fe);
	};

}
}
#endif
