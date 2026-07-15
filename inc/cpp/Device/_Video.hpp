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
#include "../vector"

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
	
	// ...
	// 0x0143 1280x960 for vmware, 800x600 for qemu and bochs

	// [Methods]
	// STM32H7  : LTDC + DMA2D
	// STM32MP13: LTDC
	struct VideoManager {
		stduint id = 0;
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
		pureptr_t family = nullptr;
		Color forecolor = {};
		Size2 size = { 0, 0 };
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
		virtual void DrawFont(const Point& disp, const DisplayFont& font, const String& str) const = 0;
		virtual Color GetColor(Point p) const = 0;
	public:
		virtual void RollUp(stduint height, const Rectangle& rect) const;
		virtual void DrawPoints(const Rectangle& rect, const Color* base) const { }
	};

	/*
	* 请求上级刷新：sheet_parent->Update...(目标区域)
	* 点击事件分发：getTop
	*/
	class LayerManager
		: public SheetTrait
		// Add LayerManagerTrait Here ?
	{
	public:
		Nnode* subf = nullptr,// top
			* subl = nullptr;// bottom
		VideoControlInterface* pvci = nullptr;
		Rectangle window = {};//[duplicate] sheet_area
		volatile bool is_dirty = true;
		Rectangle dirty_area = {};
		stduint sys_tick = 0;
		
		uni::Vector<SheetTrait*> timer_sheets;

		void RegisterTimer(SheetTrait* sheet);
		void UnregisterTimer(SheetTrait* sheet);

		void CheckTimers(stduint crt_tick);
		
		void AddDirty(const Rectangle& rect) {
			stduint x0 = maxof(rect.x, 0);
			stduint y0 = maxof(rect.y, 0);
			stduint x1 = rect.x + rect.width;
			stduint y1 = rect.y + rect.height;
			
			if (window.width) { x1 = minof((stduint)x1, window.width); }
			if (window.height) { y1 = minof((stduint)y1, window.height); }
			if (x0 >= x1 || y0 >= y1) return;

			if (!is_dirty || dirty_area.width == 0 || dirty_area.height == 0) {
				dirty_area.x = x0;
				dirty_area.y = y0;
				dirty_area.width = x1 - x0;
				dirty_area.height = y1 - y0;
				is_dirty = true;
			} else {
				stdsint min_x = minof(dirty_area.x, x0);
				stdsint min_y = minof(dirty_area.y, y0);
				stdsint max_x = maxof(dirty_area.x + dirty_area.width, x1);
				stdsint max_y = maxof(dirty_area.y + dirty_area.height, y1);
				dirty_area.x = min_x;
				dirty_area.y = min_y;
				dirty_area.width = max_x - min_x;
				dirty_area.height = max_y - min_y;
				is_dirty = true;///
			}
		}

	public:// User Using
		PixelFormat pixel_fmt = PixelFormat::ARGB8888;// decide the size of the element
		stduint video_memory = 0;
		stduint video_mode = 0;
		// stduint sheet_count;//{} Dchain
	public:
		LayerManager() : SheetTrait(), subf(nullptr), subl(nullptr) {}
		LayerManager(VideoControlInterface* p, const Rectangle& rect) : SheetTrait(), subf(nullptr), subl(nullptr), pvci(p), window(rect) {}
		inline void Reset(VideoControlInterface* p, const Rectangle& rect) {  pvci = p; window = rect; }

		void Append(SheetTrait* sheet) {
			sheet->sheet_node.offs = sheet;
			sheet->sheet_parent = this;
			if (subf == nullptr) {
				sheet->sheet_node.next = nullptr;
				sheet->sheet_node.left = nullptr;
				subl = subf = &sheet->sheet_node;
			}
			else {
				subl->next = &sheet->sheet_node;
				sheet->sheet_node.left = subl;
				sheet->sheet_node.next = nullptr;
				subl = &sheet->sheet_node;
			}
		}

		bool Remove(SheetTrait* sheet) {
			if (!sheet || sheet->sheet_parent != this) return false;
			Nnode* node = &sheet->sheet_node;
			if (node->left) node->left->next = node->next;
			else subf = node->next;
			if (node->next) node->next->left = node->left;
			else subl = node->left;
			node->next = node->left = nullptr;
			sheet->sheet_parent = nullptr;
			return true;
		}

		// Parallel Sheets
		stduint Count() {
			stduint cnt = 0;
			auto crt = subf;
			while (crt) {
				cnt++;
				crt = crt->next;
			}
			return cnt;
		}

		virtual void Update(SheetTrait* who, const Rectangle& rect);
		
		void Domove(SheetTrait* who, Size2dif dif);

		static void Dorupt(SheetTrait* who, SheetEvent event, Point rel_p, para_list args);

		virtual Color getPoint(Point p) override;

		// [trait::sheet]
		virtual void doshow(void*) override;

		// [trait::sheet]
		virtual void onrupt(SheetEvent event, Point rel_p, ...) override;

		inline VideoControlInterface& getVCI() const { return *pvci; }
		inline constexpr Color& getPoint(SheetTrait* whom, const Point& p) {
			return whom->sheet_buffer[(p.y) * whom->sheet_area.width + p.x];
		}
		inline void setCursor(Point disp) { pvci->SetCursor(disp); }
		inline Point getCursor() { return pvci->GetCursor(); }

		// The layers may use translucent color.
		Color EvaluateColor(const Point& p);

		SheetTrait* getTop(const Point& p, stduint skip = 0);

		//

		// draw
		void DrawLine(Point disp, Size2dif size, Color color);
		inline void Draw(Point disp, Color color) { pvci->DrawPoint(disp, color); }
		inline void Draw(Point disp, DisplayFont font, const String& str) { pvci->DrawFont(disp, font, str); }
		inline void Draw(Point disp, const char* addr, Color col = 0, Size2 siz = Size2(8, 16)) {
			pvci->DrawFont(disp,  { nullptr, col, siz }, String(addr));
		}
		void Draw(Rectangle rect);
		void Draw(const Circle& circ);		

	};
	using VideoControlBlock = LayerManager;


	void DrawString_16(SheetTrait& st, const Point2& p, const String& str, Color col);


}

namespace uni {
	struct FramebufferInfo {
		uni::Slice physical_range;
		uni::Size2 screen_size;
		uint32 pitch;
		uint32 bpp;
		uni::PixelFormat format;
	};

	struct VideoMode {
		uni::Size2 resolution;
		uni::PixelFormat format;
		uint32 refresh_rate;
		uint32 output_id;
	};

	class VideoDevice : public uni::VideoControlInterface {
	public:
		virtual ~VideoDevice() = default;
		virtual const FramebufferInfo& GetFramebuffer() const = 0;
		virtual bool SetMode(const VideoMode& mode) { return false; }
		virtual void Flush(const Rectangle& rect) {}
	};
}

namespace uni {
	class VideoControlInterfaceMARGB8888 : public VideoControlInterface {
		Color* p;
		Size2 size;
	public:
		VideoControlInterfaceMARGB8888(Color* buf, Size2 siz) : p(buf), size(siz) {}
	public:
		virtual void SetCursor(const Point&) const override {}
		virtual Point GetCursor() const override { return Point(0, 0); }
		virtual void DrawPoint(const Point& disp, Color color) const override {
			if (disp < size) p[disp.y * size.x + disp.x] = color;
		}
		virtual void DrawRectangle(const Rectangle& rect) const override {
			if (size.y < rect.y || size.x < rect.x) return;
			for0(y, minof(rect.height, size.y - rect.y)) {
				Color* p0 = p + size.x * (rect.y + y) + rect.x;
				for0(x, minof(rect.width, size.x - rect.x))* p0++ = rect.color;
			}
		}
		virtual void DrawFont(const Point& disp, const DisplayFont& font, const String& str) const override {}
		virtual Color GetColor(Point disp) const override {
			return (disp < size) ? p[disp.y * size.x + disp.x] : Color::Black;
		}
	public:
		virtual void RollUp(stduint height, const Rectangle& rect) const override;
		virtual void DrawPoints(const Rectangle& rect, const Color* base) const override {
			//{TODO}	
		}
		// void DrawLine(Point disp, Size2 size, Color color, bool negSizy = false);
	};

	//

	class ScreenBridge : public VideoControlInterface {
		mutable FramebufferInfo* fbi;
	public:
		uint32& Locate32(const Point& disp) const;
		virtual ~ScreenBridge() = default;
		ScreenBridge(FramebufferInfo& _fbi) : fbi(&_fbi) {}
		//
		virtual void SetCursor(const Point& disp) const override;
		virtual Point GetCursor() const override;
		virtual void DrawPoint(const Point& disp, Color color) const override;
		virtual void DrawRectangle(const Rectangle& rect) const override;
		virtual void DrawFont(const Point& disp, const DisplayFont& font, const String& str) const override;
		virtual Color GetColor(Point p) const override;
		virtual void DrawPoints(const Rectangle& rect, const Color* base) const override;
	};
}

#include "./_Video_Console.hpp"



#endif
