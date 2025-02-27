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


namespace uni {

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
		stduint pix_size;
		stduint cols, rows;
		onPressed_t onPressed;// callback event
		const VideoControlInterface& vci;
		// color can be past
	public:
		~VideoControlBlock() { }
		VideoControlBlock(pureptr_t addr, const VideoControlInterface& vci) : buffer_addr(addr), vci(vci) { }
		// here: public objects
		inline void setMode(stduint psiz, stduint cols, stduint rows, onPressed_t onpress = 0) {
			pix_size = psiz;
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

	};// Single Layer

	// inherit Console_t to make a console with self-defined font.
	class VideoConsole : public Console_t {
		const VideoControlInterface& vci;
		Point cursor;
		Size2 size;// char size but pixel size
		stduint typ;// 0: inner_8x5, 1:inner_16x8
	public:
		
		Color forecolor;
	protected:
		static void (VideoConsole::* DrawCharPosition_f[])(uni::Point, uni::Color, char);
		static VideoConsole* crt_self;
		friend void _VideoConsoleOut(const char* str, stduint len);
		void DrawCharPosition_8x5(Point disp, Color color, char ch);
		void DrawCharPosition_16x8(Point disp, Color color, char ch);
	public:
		VideoConsole(const VideoControlInterface& vci, Size2 siz) : 
			vci(vci), cursor({ 0,0 }), size(siz), typ(1), forecolor(Color::Black) { }
	public:
		virtual int out(const char* str, stduint len);
		virtual int inn() _TODO;
	public:

		void curinc() {
			cursor.x++;
			if (cursor.x >= size.x) {
				cursor.x -= size.x;
				cursor.y++;
			}
		}


	};

}





#endif
