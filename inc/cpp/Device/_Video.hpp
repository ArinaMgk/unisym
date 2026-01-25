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

// VBE Video Modes
// Reference: VESA VBE Core Functions 3.0
// Mode numbers are standard VBE IDs, but availability depends on BIOS/QEMU build.
	
	enum class VideoModeVBE3 : uint16_t {
		// text modes
		TEXT_80x60 = 0x108,
		TEXT_132x25 = 0x109,
		TEXT_132x43 = 0x10A,
		TEXT_132x50 = 0x10B,
		TEXT_132x60 = 0x10C,

		// 320x200 modes
		RGB555_320x200 = 0x10D,    // VBE3, 1:5:5:5
		RGB565_320x200 = 0x10E,    // VBE3,   5:6:5
		RGB888_320x200 = 0x10F,    // VBE3, 24bpp, 0xRRGGBB (memory: B,G,R)

		// 640x400 modes
		INDEXED8_640x400 = 0x100,  // VBE3, 8bpp (256 colors, palette indexed)

		// 640×480 modes
		INDEXED8_640x480 = 0x101,  // VBE3, 8bpp (256 colors, palette indexed)
		RGB555_640x480 = 0x110,    // VBE3, 15bpp (1:5:5:5 direct color)
		RGB565_640x480 = 0x111,    // VBE3, 16bpp (5:6:5 direct color)
		RGB888_640x480 = 0x112,    // VBE3, 24bpp, 0xRRGGBB (memory: B,G,R)
		ARGB8888_640x480 = 0x11A,  // VGA , 32bpp, 0xAARRGGBB (memory: B,G,R,A)

		// 800×600 modes
		INDEXED4_800x600 = 0x102,  // VBE3, 4bpp (16 colors, palette indexed)
		INDEXED8_800x600 = 0x103,  // VBE3, 8bpp (256 colors, palette indexed)
		RGB555_800x600 = 0x113,    // SVGA, 15bpp (1:5:5:5 direct color)
		RGB565_800x600 = 0x114,    // SVGA, 16bpp (5:6:5 direct color)
		RGB888_800x600 = 0x115,    // SVGA, 24bpp, 0xRRGGBB (memory: B,G,R)
		ARGB8888_800x600 = 0x118,  // SVGA, 32bpp, 0xAARRGGBB (memory: B,G,R,A)

		// 1024×768 modes
		INDEXED4_1024x768 = 0x104, // VBE3, 4bpp (16 colors, palette indexed)
		INDEXED8_1024x768 = 0x105, // VBE3, 8bpp
		RGB555_1024x768 = 0x116,   // SVGA, 15bpp (1:5:5:5 direct color)
		RGB565_1024x768 = 0x117,   // SVGA, 16bpp (5:6:5 direct color)
		RGB888_1024x768 = 0x118,   // SVGA, 24bpp
		ARGB8888_1024x768 = 0x11B, // SVGA, 32bpp (0x144)

		// ---- BELOW: Use dynamic checking the mode information ----

		// 1280×1024 modes
		INDEXED4_1280x1024 = 0x106, // VBE3, 4bpp (16 colors, palette indexed)
		INDEXED8_1280x1024 = 0x107, // VBE3, 8bpp
		RGB555_1280x1024 = 0x119,   // VBE3, 15bpp (1:5:5:5 direct color)
		RGB565_1280x1024 = 0x11A,   // VBE3, 16bpp (5:6:5 direct color)
		//               = 0x119,   // UXGA, 16bpp (5:6:5 direct color) (?)
		RGB888_1280x1024 = 0x11B,   // VBE3, 24bpp
		//               = 0x11A,   // UXGA, 24bpp (?)
		ARGB8888_1280x1024 = 0x11C, // UXGA, 32bpp

		// 1440x900 modes
		ARGB8888_1440x900 = 0x180, // SVGA, 32bpp

	};
	typedef VideoModeVBE3 VideoMode;
	// ...
	// 0x0143 1280x960 for vmware, 800x600 for qemu and bochs


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
	public:
		void RollUp(stduint height, const Rectangle& rect) const {
			MIN(height, rect.height);
			for (stduint y = 0; y < rect.height - height; y++) {
				for (stduint x = 0; x < rect.width; x++) {
					DrawPoint(Point(x, y), GetColor(Point(x, y + height)));
				}
			}
			DrawRectangle(Rectangle(
				Point(0, rect.height - height),
				Size2(rect.width, height),
				rect.color
			));
		}
	};

	/*
	* 请求上级刷新：sheet_parent->Update...(目标区域)
	*/
	class LayerManager {
	public:
		SheetTrait* subf = nullptr,// top
			* subl = nullptr;// bottom
		VideoControlInterface* pvci;
	protected:
		Rectangle window;
		// stduint sheet_count;//{} Dchain
	public:
		LayerManager(VideoControlInterface* p, const Rectangle rect) : pvci(p), window(rect) { }

		void Append(SheetTrait* sheet) {
			if (subf == nullptr) {
				subl = subf = sheet;
				sheet->sheet_pnext = nullptr;
				sheet->sheet_pleft = nullptr;
			}
			else {
				subl->sheet_pnext = sheet;
				sheet->sheet_pleft = subl;
				sheet->sheet_pnext = nullptr;
				subl = sheet;
			}
		}

		stduint Count() {
			stduint cnt = 0;
			SheetTrait* crt = subf;
			while (crt) {
				cnt++;
				crt = crt->sheet_pnext;
			}
			return cnt;
		}

		
		void Update(SheetTrait* who, const Rectangle& rect);
		
		void Domove(SheetTrait* who, Size2dif dif);

		inline constexpr Color& getPoint(SheetTrait* whom, const Point& p) {
			return whom->sheet_buffer[(p.y) * whom->sheet_area.width + p.x];
		}

		// The layers may use translucent color.
		Color EvaluateColor(const Point& p);

		SheetTrait* getTop(const Point& p, stduint skip = 0);

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

		inline VideoControlInterface& getVCI() const { return *(VideoControlInterface*)&vci; }

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


	};// Single Layer

	// inherit Console_t to make a console with self-defined font.
	class VideoConsole : public Console_t, public SheetTrait
	{
	public: const VideoControlInterface& vci;
	protected:
		Point cursor = { 0,0 };
		Color* buffer = nullptr;// pixels/words buffer, not sheet_buffer
		Size2 size;// char unit but pixel
		stduint typ;// 0: inner_8x5, 1:inner_16x8
	public:
		
		Color forecolor;
		Color backcolor;// for next font's, but background, while background use window.color.
		Rectangle window;//{TODO}
		byte update_method = 2;// 0: no update, 1: update all, 2: update line
	protected:
		static void (VideoConsole::* DrawCharPosition_f[])(uni::Point, uni::Color, char);
		static VideoConsole* crt_self;
		friend void _VideoConsoleOut(const char* str, stduint len);
		void DrawCharPosition_8x5(Point disp, Color color, char ch);
		void DrawCharPosition_16x8(Point disp, Color color, char ch);
		void FeedLine();

	public:
		VideoConsole(const VideoControlInterface& vci,
			const Rectangle& win,
			const Color& fore_color = Color::White,
			const Color& back_color = Color::Black) :
			SheetTrait(),
			vci(vci),
			buffer(nullptr), size(0), typ(1), forecolor(fore_color), backcolor(back_color),
			window(win)
		{
			size.x = window.width / (typ ? 8 : 5);
			size.y = window.height / (typ ? 16 : 8);
			window.color = fore_color;
		}

		inline void Clear() {
			if (buffer) {
				Color* p0 = buffer;
				for0(y, window.height * window.width) {
					*p0++ = backcolor;
				}
				sheet_parent->Update(this, window);
			}
			else vci.DrawRectangle(window);
		}

		void setModeBuffer(Color* buf) { buffer = buf; }
		void setBackcolor(Color bkcolor) {
			byte buf[4] = { (byte)'\xFE', bkcolor.b, bkcolor.g, bkcolor.r };
			out((rostr)buf, 4);
		}

	public:
		virtual int out(const char* str, stduint len);
		virtual int inn() _TODO;
		virtual void doshow(void*) override;
		virtual void onrupt(SheetEvent event, Point rel_p, ...) override {}

	public:

		void curinc();

	// Sheet:
	protected:
		void thisDrawPoint(const Point& disp, Color color);
		void thisDrawRectangle(const Rectangle& rect);
		void thisRollup(stduint height);
	public:
		void RefreshLine();
	};

}





#endif
