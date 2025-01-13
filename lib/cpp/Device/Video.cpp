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

#include "../../../inc/cpp/Device/_Video.hpp"
#include "../../../inc/cpp/reference"
#include "../../../inc/c/data.h"

namespace uni {
	void VideoControlBlock::Draw(Rectangle rect) {
		if (rect.filled) vci.DrawRectangle(rect);
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
	void VideoControlBlock::Draw(const Circle& circ) {
		stduint x = 0, y = circ.radius;
		stdsint delta, tmp;
		delta = 3 - (circ.radius << 1);
		const Point& cen = circ.center;
		while (y >= x) {
			if (circ.filled)
			{
				Rectangle rect(Point(cen.x - x, cen.y + y), Size2(2 * x, 1), circ.color);
				vci.DrawRectangle(rect);
				// vci.DrawRectangle(Rectangle(Point(cen.x - x, cen.y - y), Size2(2 * x, 1), circ.color));
				rect.y -= 2 * y;
				vci.DrawRectangle(rect);
				// vci.DrawRectangle(Rectangle(Point(cen.x - y, cen.y + x), Size2(2 * y, 1), circ.color));
				rect.width = 2 * y;
				rect.x = cen.x - y, rect.y = cen.y + x;
				vci.DrawRectangle(rect);
				// vci.DrawRectangle(Rectangle(Point(cen.x - y, cen.y - x), Size2(2 * y, 1), circ.color));
				rect.y -= 2 * x;
				vci.DrawRectangle(rect);
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

	//{unchk} negSizy
	void VideoControlBlock::DrawLine(Point disp, Size2 size, Color color, bool negSizy) {
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
			for0(i, size.x) {
				for0(j, _IMM((double)orig.y + i * (size.y - 1) / (size.x - 1) + 0.5) - disp.y) {
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

	void VideoControlBlock::Draw_2Points(Point disp, Color colors[4]) {
		vci.DrawPoint(disp, colors[0]);
		disp.x++;
		if (disp.x < cols) vci.DrawPoint(disp, colors[1]);
	}
	void VideoControlBlock::Draw_4Points(Point disp, Color colors[4]) {
		for0(i, 4) {
			vci.DrawPoint(disp, colors[i]);
			disp.x++; if (disp.x >= cols) { disp.x = 0; disp.y++; }
		}
	}

	// ----
	void (VideoConsole::* (VideoConsole::DrawCharPosition_f)[])(uni::Point, uni::Color, char) {
		&VideoConsole::DrawCharPosition_8x5, 
		&VideoConsole::DrawCharPosition_16x8
	};
	static uint16 FontSizeHeight[]{ 8, 16 };
	static uint16 FontSizeWidth[]{ 5,8 };

	void VideoConsole::DrawCharPosition_8x5(Point disp, Color color, char ch) {
		if (ascii_isprint(ch)) {
			ch -= 0x20;
			const uint8(*datptr) = (const uint8(*)) & _BITFONT_ASCII_8x5[ch];
			uint8 dat = 0; Reference_T<uint8> dat_bmap _IMM(&dat);
			for0(i, 5) {
				dat = datptr[i];
				for0r(j, 8) {
					if (dat_bmap.bitof(j))// else if (fill) ...
						vci.DrawPoint(Point(disp.x, disp.y + j), color);
				}
				disp.x++;
			}
		}
		else return;
		// no curinc nor scrcol
	}
	
	void VideoConsole::DrawCharPosition_16x8(Point disp, Color color, char ch) {
		if (ascii_isprint(ch)) {
			ch -= 0x20;
			const uint16(*datptr) = (const uint16(*))&_BITFONT_ASCII_16x8[ch];
			uint16 dat = 0; Reference_T<uint16> dat_bmap _IMM(&dat);
			for0(i, 8) {
				dat = datptr[i];
				for0r(j, 16) {
					if (dat_bmap.bitof(j))// else if (fill) ...
						vci.DrawPoint(Point(disp.x, disp.y + (j ^ 0b111)), color);
				}
				disp.x++;
			}
		}
		else return;
		// no curinc nor scrcol
	}

	

	int VideoConsole::inn() {
		return _TEMP 0;
	}


	
	VideoConsole* VideoConsole::crt_self;
	void _VideoConsoleOut(const char* str, stduint len) {
		#define crt_self VideoConsole::crt_self
		void (VideoConsole:: * draw_f)(uni::Point, uni::Color, char) = nullptr;
		if (crt_self->typ < numsof(VideoConsole::DrawCharPosition_f)) {
			draw_f = VideoConsole::DrawCharPosition_f[crt_self->typ];
		}
		else return;
		for0(i, len) {
			(crt_self->*draw_f) (
				Point(
					crt_self->cursor.x * FontSizeWidth[crt_self->typ],
					crt_self->cursor.y * FontSizeHeight[crt_self->typ]),
				crt_self->forecolor, str[i]);
			crt_self->curinc();
		}
		#undef crt_self
	}
	int VideoConsole::out(const char* str, dword len) {
		_VideoConsoleOut(str, len);
		return 0 _TEMP;
	}
	int VideoConsole::FormatShow(const char* fmt, ...) {
		crt_self = this;
		Letpara(args, fmt);
		outbyte_t last = outredirect(_VideoConsoleOut);
		int ret = outsfmtlst(fmt, args);
		outredirect(last);
		return ret;
	}

}
