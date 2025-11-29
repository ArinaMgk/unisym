// ASCII CPP TAB4 CRLF
// Docutitle: (Module) Video
// Codifiers: @dosconio: 20240513
// Attribute: Arn-Covenant Any-Architect Bit-32mode Non-Dependence
// Copyright: UNISYM, under Apache License 2.0; Dosconio Mecocoa, BSD 3-Clause License
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

// Screen = Video + Touch (Do not manage {Backlight} )

#ifndef _INC_DEVICE_Video_X
#define _INC_DEVICE_Video_X

#include "../unisym"
#include "../../c/graphic.h"
#include "../../c/consio.h"
#include "../../c/driver/Video/video.h"

namespace uni {

// VBE Video Modes (common selection, QEMU compatible)
// Reference: VESA VBE Core Functions 3.0
// Mode numbers are standard VBE IDs, but availability depends on BIOS/QEMU build.

	enum class VideoMode : uint16_t {
		// 640×480 modes
		INDEXED8_640x480 = 0x101, // VGA standard, 8bpp (256 colors, palette indexed)
		RGB555_640x480 = 0x110, // VGA standard, 15bpp (5:5:5 direct color)
		RGB565_640x480 = 0x111, // VGA standard, 16bpp (5:6:5 direct color)
		RGB888_640x480 = 0x112, // VGA standard, 24bpp, 0xRRGGBB (memory: B,G,R)
		RGBA8888_640x480 = 0x11A, // VGA standard, 32bpp, 0xAARRGGBB (memory: B,G,R,A)

		// 800×600 modes
		INDEXED8_800x600 = 0x103, // SVGA, 8bpp (256 colors, palette indexed)
		RGB555_800x600 = 0x113, // SVGA, 15bpp (5:5:5 direct color)
		RGB565_800x600 = 0x114, // SVGA, 16bpp (5:6:5 direct color)
		RGB888_800x600 = 0x115, // SVGA, 24bpp, 0xRRGGBB (memory: B,G,R)
		RGBA8888_800x600 = 0x118, // SVGA, 32bpp, 0xAARRGGBB (memory: B,G,R,A)

		// 1024×768 modes
		INDEXED8_1024x768 = 0x105, // SVGA, 8bpp
		RGB565_1024x768 = 0x117, // SVGA, 16bpp (5:6:5 direct color)
		RGB888_1024x768 = 0x116, // SVGA, 24bpp
		RGBA8888_1024x768 = 0x11B, // SVGA, 32bpp

		// 1280×1024 modes
		INDEXED8_1280x1024 = 0x107, // UXGA, 8bpp
		RGB565_1280x1024 = 0x119, // UXGA, 16bpp (5:6:5 direct color)
		RGB888_1280x1024 = 0x11A, // UXGA, 24bpp
		RGBA8888_1280x1024 = 0x11C  // UXGA, 32bpp
	};


	#define GrafColor32(c) Color::FromRGB888(c)

	static inline Rectangle GrafRect(stduint x, stduint y, stduint w, stduint h, Color color, bool hollow = false) {
		Rectangle res;
		res.x = x;
		res.y = y;
		res.width = w;
		res.height = h;
		res.color = color;
		res.filled = !hollow;
		return res;
	}
	static inline Rectangle GrafRectHollow(stduint x, stduint y, stduint w, stduint h, Color color) {
		return GrafRect(x, y, w, h, color);
	}

	struct DisplayFont {
		pureptr_t addr;
		pureptr_t family;
		stduint numbers;
		Color forecolor;
		Size2 size;
	};


	// Do not use pure virtual for PCU, must be entity
	class VideoControlInterface {
	public:
		virtual ~VideoControlInterface() { }
	public:
		virtual void SetCursor(const Point& disp) const = 0;
		virtual Point GetCursor() const = 0;
		virtual void DrawPoint(const Point& disp, Color color) const = 0;
		virtual void DrawRectangle(const Rectangle& rect) const = 0;
		virtual void DrawFont(const Point& disp, const DisplayFont& font) const = 0;
		virtual Color GetColor(Point p) const = 0;
	};
	
	// general
	typedef void (*onPressed_t)(Point disp, stduint pressure, stduint range_radius);
	class VideoControlBlock {
	protected:
		pureptr_t buffer_addr;
		PixelFormat pixel_fmt = PixelFormat::ARGB8888;// decide the size of the element
		stduint cols = 0, rows = 0;
		onPressed_t onPressed;// callback event
		const VideoControlInterface& vci;
		Color back_color;
		// Curret Model:
		// 1. The Video buffer is organized in one memory area;
		// 2. The address of next line equals: addr + cols * pixel_fmt.getSize();
		//{MAYBE TODO} padding after row-end (?)
	public:
		~VideoControlBlock() { }
		VideoControlBlock(pureptr_t addr, const VideoControlInterface& vci) : 
			buffer_addr(addr), vci(vci) { }
		VideoControlBlock(const VideoControlInterface& vci, const Size2& siz, const Color& bcolor) : 
			buffer_addr(nullptr), vci(vci), cols(siz.x), rows(siz.y), back_color(bcolor) { }
		// here: public objects
		inline void setMode(PixelFormat pfmt, stduint cols, stduint rows, onPressed_t onpress = 0) {
			pixel_fmt = pfmt;
			this->cols = cols;
			this->rows = rows;
			onPressed = onpress;
		}
		inline stduint getLimitX() { return cols; }
		inline stduint getLimitY() { return rows; }

		// basic
		inline void setCursor(Point disp) { vci.SetCursor(disp); }
		inline Point getCursor() { return vci.GetCursor(); { return Point(); } }

		// draw
		void DrawLine(Point disp, Size2 size, Color color, bool negSizy = false);
		void Draw(Rectangle rect);
		void Draw(const Circle& circ);

		//
		inline void Draw(Point disp, Color color) { vci.DrawPoint(disp, color); }
		inline void Draw(Point disp, DisplayFont font) { vci.DrawFont(disp, font); }
		inline void Draw(Point disp, const char* addr, Color col = 0, Size2 siz = Size2(8, 16)) {
			vci.DrawFont(disp, { (pureptr_t)addr , nullptr, StrLength(addr), col, siz });
		}
		inline void Draw(stdint x, stdint y, const char* str) {
			while (x < 0) x += cols;
			while (y < 0) y += rows;
			DisplayFont f = { (pureptr_t)str };
			Draw(Point(x, y), f);
		}
		// override this if needed
		virtual void Draw_2Points(Point disp, Color colors[4]);
		virtual void Draw_4Points(Point disp, Color colors[4]);

		void RollUp(stduint height) {
			for (stduint y = 0; y < rows - height; y++) {
				for (stduint x = 0; x < cols; x++) {
					Draw(Point(x, y), vci.GetColor(Point(x, y + height)));
				}
			}
			for (stduint y = rows - height; y < rows; y++) {
				for (stduint x = 0; x < cols; x++) {
					Draw(Point(x, y), back_color);
				}
			}
		}

	};// Single Layer

	// inherit Console_t to make a console with self-defined font.
	class VideoConsole : public Console_t, public SheetTrait
	{
	public: const VideoControlInterface& vci;
	protected:
		Point cursor = { 0,0 };
		byte* buffer = nullptr;// pixels buffer
		Size2 size;// char unit but pixel
		stduint typ;// 0: inner_8x5, 1:inner_16x8
	public:
		
		Color forecolor;
		Color backcolor;// for next font's, but background, while background use window.color.
		Rectangle window;//{TODO}
	protected:
		static void (VideoConsole::* DrawCharPosition_f[])(uni::Point, uni::Color, char);
		static VideoConsole* crt_self;
		friend void _VideoConsoleOut(const char* str, stduint len);
		void DrawCharPosition_8x5(Point disp, Color color, char ch);
		void DrawCharPosition_16x8(Point disp, Color color, char ch);
		void FeedLine();

		void Refresh();//{TODO} according to (buffer)
	public:
		VideoConsole(const VideoControlInterface& vci,
			const Rectangle& win,
			const Color& fore_color = Color::White,
			const Color& back_color = Color::Black) :
			vci(vci), size(0), typ(1), forecolor(fore_color), backcolor(back_color),
			window(win)
		{
			size.x = window.width / (typ ? 8 : 5);
			size.y = window.height / (typ ? 16 : 8);
		}

		inline void Clear() {
			vci.DrawRectangle(window);
		}
	public:
		virtual int out(const char* str, stduint len);
		virtual int inn() _TODO;
		virtual void doshow() override;
	public:

		void curinc();


	};

}





#endif
