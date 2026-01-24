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

	// ---- LayerManager ---- //
	void LayerManager::Update(SheetTrait* who, const Rectangle& rect) {
		// auto p = who->sheet_buffer; if (!p) return;
		Rectangle abs_rect = who ? who->sheet_area : window;
		abs_rect.x += rect.x;
		abs_rect.y += rect.y;
		for0(i, rect.height) {
			// auto pp = p + (rect.y + i) * who->sheet_area.width + rect.x;
			Point point(abs_rect.x + 0, abs_rect.y + i);
			for0(j, rect.width) {
				pvci->DrawPoint(point, EvaluateColor(point));
				point.x++;
				if (point.x >= window.width) break;
			}
		}
	}

	void LayerManager::Domove(SheetTrait* who, Size2dif dif) {
		stdsint _x = who->sheet_area.x; _x += dif.x;
		stdsint _y = who->sheet_area.y; _y += dif.y;
		if (dif.x >= 0) { MIN(_x, window.width - 1); }
		else { MAX(_x, 0); }
		if (dif.y >= 0) { MIN(_y, window.height - 1); }
		else { MAX(_y, 0); }
		Rectangle old_rect = who->sheet_area;
		who->sheet_area.x = _x;
		who->sheet_area.y = _y;
		Update(NULL, old_rect);
		Update(who, Rectangle(Point(0,0), who->sheet_area.getSize()));
	}

	Color LayerManager::EvaluateColor(const Point& glb_p) {
		SheetTrait* crt = subl;
		Color col = window.color;
		if (crt) do {
			auto& rect = crt->sheet_area;
			if (!crt->sheet_buffer || !rect.ifContain(glb_p)) continue;
			auto pp = crt->sheet_buffer + (glb_p.y - rect.y) * crt->sheet_area.width + (glb_p.x - rect.x);
			
			// return *pp;
			if (pp->a == 0xFFu) col = *pp;
			else if (!pp->a);
			else {
				double rate = (double)pp->a / 255.0;
				double r = pp->r * rate + col.r * (1 - rate);
				double g = pp->g * rate + col.g * (1 - rate);
				double b = pp->b * rate + col.b * (1 - rate);
				col.r = (byte)r; col.g = (byte)g; col.b = (byte)b;
			}
		} while (crt = crt->sheet_pleft);
		col.a = 0xFF;
		return col;
	}

	SheetTrait* LayerManager::getTop(const Point& p, stduint skip) {
		SheetTrait* crt = subf;
		if (crt) do {
			if (skip) { skip--; continue; }
			auto& area = crt->sheet_area;
			if (area.ifContain(p)) return crt;
		} while (crt = crt->sheet_pnext);
		return nullptr;
	}

}
