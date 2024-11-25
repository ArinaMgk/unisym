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
#include "../cinc"
#include "../../c/graphic.h"
#include "../cinc"

namespace uni {

	#define GrafColor32(c) Color::From32(c)

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

	class VideoControlInterface {
	public:
		virtual void SetCursor(const Point& disp) = 0;
		virtual Point GetCursor() = 0;
		virtual void DrawPoint(const Point& disp, Color color) = 0;
		virtual void DrawRectangle(const Rectangle& rect) = 0;
		virtual void DrawFont(const Point& disp, const DisplayFont& font) = 0;
		virtual Color GetColor(Point p) = 0;
	};
	
	// general
	typedef void (*onPressed_t)(Point disp, stduint pressure, stduint range_radius);
	class VideoControlBlock {
	protected:
		pureptr_t buffer_addr;
		stduint pix_size;
		stduint cols, rows;
		onPressed_t onPressed;// callback event
		VideoControlInterface* vci;
		// color can be past
	public:
		VideoControlBlock(pureptr_t addr, VideoControlInterface* vci) : buffer_addr(addr), vci(vci) { }
		~VideoControlBlock() { }
		// here: public objects
		inline void setMode(stduint psiz, stduint cols, stduint rows, onPressed_t onpress = 0) {
			pix_size = psiz;
			this->cols = cols;
			this->rows = rows;
			onPressed = onpress;
		}

		// basic
		inline void setCursor(Point disp) { vci->SetCursor(disp); }
		inline Point getCursor() { return vci->GetCursor(); { return Point(); } }
		inline void Draw(Rectangle rect) {
			if (rect.filled) vci->DrawRectangle(rect);
			else {
				self.DrawLine(rect.getVertex(), 
					Size2(rect.width, 1), rect.color);
				self.DrawLine(Point(rect.x, rect.y + rect.height - 1),
					Size2(rect.width, 1), rect.color);
				self.DrawLine(rect.getVertex(),
					Size2(1, rect.height), rect.color);
				self.DrawLine(Point(rect.x + rect.width - 1, rect.y),
					Size2(1, rect.height), rect.color);
			}
		}
		inline void Draw(const Circle& circ) {
			stduint x = 0, y = circ.radius;
			stdsint delta, tmp;
			delta = 3 - (circ.radius << 1);
			const Point& cen = circ.center;
			while(y > x) {
				if(circ.filled)
				{
					vci->DrawRectangle(Rectangle(Point(cen.x - x, cen.y + y), Size2(2 * x, 1), circ.color));
					vci->DrawRectangle(Rectangle(Point(cen.x - x, cen.y - y), Size2(2 * x, 1), circ.color));
					vci->DrawRectangle(Rectangle(Point(cen.x - y, cen.y + x), Size2(2 * y, 1), circ.color));
					vci->DrawRectangle(Rectangle(Point(cen.x - y, cen.y - x), Size2(2 * y, 1), circ.color));
				}
				else
				{
					Draw(Point(cen.x + x, cen.y + y), circ.color);
					Draw(Point(cen.x - x, cen.y + y), circ.color);
					Draw(Point(cen.x + x, cen.y - y), circ.color);
					Draw(Point(cen.x - x, cen.y - y), circ.color);
					Draw(Point(cen.x + y, cen.y + x), circ.color);
					Draw(Point(cen.x - y, cen.y + x), circ.color);
					Draw(Point(cen.x + y, cen.y - x), circ.color);
					Draw(Point(cen.x - y, cen.y - x), circ.color);
				}
				x++;
				if (delta >= 0)
				{
					y--;
					tmp = (x << 2) - (y << 2);
					delta += (tmp + 10);
				}
				else
					delta += ((x << 2) + 6);
			}
		}

		//
		inline void Draw(Point disp, Color color) { vci->DrawPoint(disp, color); }
		inline void Draw(Point disp, DisplayFont font) { vci->DrawFont(disp, font); }
		inline void Draw(Point disp, const char* addr, Color col = 0, Size2 siz = Size2(8, 16)) {
			DisplayFont df;
			df.addr = (pureptr_t)addr;
			df.size = siz;
			df.forecolor = col;
			vci->DrawFont(disp, df);
		}
		inline void Draw(stdint x, stdint y, const char* str) {
			if (x < 0) x = cols + x;
			if (y < 0) y = rows + y;
			if (x < 0 || y < 0) return;
			Point p(x, y);
			DisplayFont f = { (pureptr_t)str };
			Draw(p, f);
		}
		inline stduint getLimitX() { return cols; }
		inline stduint getLimitY() { return rows; }
		// more
		//{unchk} negSizy
		void DrawLine(Point disp, Size2 size, Color color, bool negSizy = false) {
			if (!size.x || !size.y) return;
			if (1 == size.x) for0(i, size.y) {
				Draw(disp, color);// can also use "DrawRect"
				disp.y++;// vertical
			}
			else if (1 == size.y) for0(i, size.x) {
				Draw(disp, color);// can also use "DrawRect"
				disp.x++;// horizontal
			}
			else if (size.y >= size.x) {
				Point orig = disp;
				for0 (i, size.x) {
					for0 (j, _IMM((double)orig.y + i * (size.y - 1) / (size.x - 1) + 0.5) - disp.y) {
						Draw(disp, color);
						if (negSizy) disp.y--; else disp.y++;
					}
					disp.x++;
				}
			}
			else {
				Point orig = disp;
				for0(i, size.y) {
					for0(j, _IMM((double)orig.x + i * (size.x - 1) / (size.y - 1) + 0.5) - disp.x) {
						Draw(disp, color);
						disp.x++;
					}
					if (negSizy) disp.y--; else disp.y++;
				}
			}
		}
	};// Single Layer
	/* e.g. 80x25
		0xB8000 and pix_size(2)
	*/

}





#endif
