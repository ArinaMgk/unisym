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
	void VideoControlInterface::RollUp(stduint height, const Rectangle& rect) const {
		MIN(height, rect.height);
		for (stduint y = 0; y < rect.height - height; y++) {
			for (stduint x = 0; x < rect.width; x++) {
				DrawPoint(
					Point(rect.x + x, rect.y + y),
					GetColor(Point(rect.x + x, rect.y + y + height))
				);
			}
		}
		DrawRectangle(Rectangle(
			Point(rect.x, rect.y + rect.height - height),
			Size2(rect.width, height),
			rect.color
		));
	}
	void VideoControlInterfaceMARGB8888::RollUp(stduint height, const Rectangle& rect) const {
		Point2 p_right_dn = rect.getVertex() + rect.getSize();
		stduint siz_w = rect.width;
		stduint siz_h = rect.height;
		if (p_right_dn.x > size.x) {
			if (rect.x >= size.x) return;
			siz_w = size.x - rect.x;
		}
		if (p_right_dn.y > size.y) {
			if (rect.y >= size.y) return;
			siz_h = size.y - rect.y;
		}
		stduint h_dif = minof(height, siz_h);
		if (h_dif == 0) return;
		Color* p0 = this->p + size.x * rect.y + rect.x;
		Color* p1 = this->p + size.x * (rect.y + h_dif) + rect.x;
		for0(i, siz_h - h_dif) {
			for0(j, siz_w) {
				p0[j] = p1[j];
			}
			p0 += size.x;
			p1 += size.x;
		}
		DrawRectangle(Rectangle(
			Point(rect.x, rect.y + siz_h - h_dif),
			Size2(siz_w, h_dif),
			rect.color
		));
	}
	/*
		stduint h_dif = minof(height, window.height);
		Color* p0 = buffer;
		Color* p = buffer + h_dif * window.width;
		for0(i, (window.height - h_dif) * window.width) {
			*p0++ = *p++;
		}
		for0(i, h_dif* window.width) {
			*p0++ = backcolor;
		}
	*/
}

namespace uni {
	void LayerManager::Draw(Rectangle rect) {
		if (rect.filled) pvci->DrawRectangle(rect);
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
	void LayerManager::Draw(const Circle& circ) {
		auto& vci = *pvci;
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
	#if defined(_MCCA) && ((_MCCA & 0xFF00) == 0x8600)
	__attribute__((target("sse2")))
		#endif
		void LayerManager::DrawLine(Point disp, Size2 size, Color color, bool negSizy) {
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

	#if defined(_MCCA) && ((_MCCA & 0xFF00) == 0x8600)
	__attribute__((target("general-regs-only")))
		#endif
		void LayerManager::Update(SheetTrait* who, const Rectangle& rect) {
			// auto p = who->sheet_buffer; if (!p) return;
		if (!this) return;
		VideoControlInterfaceMARGB8888 vcim(sheet_buffer, window.getSize());
		Rectangle abs_rect = who ? who->sheet_area : window;
		abs_rect.x += rect.x;
		abs_rect.y += rect.y;
		//{TEMP} | then —
		for0(i, rect.height) {
			// auto pp = p + (rect.y + i) * who->sheet_area.width + rect.x;
			Point point(abs_rect.x + 0, abs_rect.y + i);
			if (point.y >= window.height) break;
			for0(j, rect.width) {
				if (point.x >= window.width) break;
				if (sheet_buffer) {
					vcim.DrawPoint(point, EvaluateColor(point));
				}
				else if (pvci) pvci->DrawPoint(point, EvaluateColor(point));
				point.x++;
			}
		}
		if (sheet_parent) sheet_parent->Update(this, Rectangle(
			Point(abs_rect.x, abs_rect.y),
			Size2(rect.width, rect.height)
		));
		else {
			// Ensure dirty area spans the actual affected absolute rect
			// this->AddDirty(window);
			this->AddDirty(Rectangle(Point(abs_rect.x, abs_rect.y), Size2(rect.width, rect.height)));
		}
	}

	void LayerManager::Domove(SheetTrait* who, Size2dif dif) {
		stdsint _x = who->sheet_area.x; _x += dif.x;
		stdsint _y = who->sheet_area.y; _y += dif.y;
		MIN(_x, (stdsint)window.width - 1); MAX(_x, 0);
		MIN(_y, (stdsint)window.height - 1); MAX(_y, 0);
		Rectangle old_rect = who->sheet_area;
		who->sheet_area.x = _x;
		who->sheet_area.y = _y;
		asserv(old_rect.x)--; if (old_rect.x + old_rect.width + 2 < window.width) old_rect.width += 2;
		asserv(old_rect.y)--; if (old_rect.y + old_rect.height + 2 < window.height) old_rect.height += 2;
		Update(NULL, old_rect);
		Update(who, Rectangle(Point(0, 0), who->sheet_area.getSize()));
	}

	auto LayerManager::getPoint(Point p) -> Color {
		return EvaluateColor(p);
	}

	// [trait::sheet]
	void LayerManager::doshow(void*) {
		_TODO
	}

	// [trait::sheet]
	void LayerManager::onrupt(SheetEvent event, Point rel_p, ...) {
		Letpara(args, rel_p);
		auto top = getTop(rel_p);
		stduint para1 = para_next(args, stduint);
		if (event == SheetEvent::onLeave && para1 == 1) {
			for (auto n = subf; n; n = n->next) {
				auto& sheet = treat<SheetTrait>(n->offs);
				sheet.onrupt(event, Point(0, 0), para1);
			}
			return;
		}
		asserv(top)->onrupt(event, rel_p - top->sheet_area.getVertex(), para1);
	}
	
	#if defined(_MCCA) && ((_MCCA & 0xFF00) == 0x8600)
	__attribute__((target("general-regs-only")))
		#endif
		Color LayerManager::EvaluateColor(const Point& glb_p) {
		Nnode* crt = subf;
		Color col = 0;
		if (crt && crt->offs) do {
			auto& crt_sheet = treat<SheetTrait>(crt->offs);
			auto& rect = crt_sheet.sheet_area;
			if (!rect.ifContain(glb_p)) continue;
			const Point pt{ glb_p - rect.getVertex() };
			auto pp = !crt_sheet.sheet_buffer ? crt_sheet.getPoint(pt) :
				crt_sheet.sheet_buffer[pt.y * crt_sheet.sheet_area.width + pt.x];

			if (pp.a == 0xFF) {
				if (col.a == 0) return pp;
				else {
					stduint inv_a = 0xFF - col.a;
					col.r += (sint32)pp.r * inv_a / 0xFF;
					col.g += (sint32)pp.g * inv_a / 0xFF;
					col.b += (sint32)pp.b * inv_a / 0xFF;
					col.a = 0xFF;
					break;
				}
			}
			else if (pp.a > 0) {
				stduint factor = pp.a * (0xFF - col.a);
				constexpr const sint32 dv = 0xFF * 0xFF;
				col.r += (sint32)pp.r * factor / dv;
				col.g += (sint32)pp.g * factor / dv;
				col.b += (sint32)pp.b * factor / dv;
				col.a += (sint32)pp.a * (0xFF - col.a) / 0xFF;
			}
		} while (col.a <= 0xFEu && (crt = crt->next) && crt->offs);
		if (col.a != 0xFFu) {
			col.r += (sint32)window.color.r * (0xFF - col.a) / 0xFF;
			col.g += (sint32)window.color.g * (0xFF - col.a) / 0xFF;
			col.b += (sint32)window.color.b * (0xFF - col.a) / 0xFF;
		}
		col.a = 0xFFu;
		return col;
	}

	//{TODO} Skip Irregular Shape
	SheetTrait* LayerManager::getTop(const Point& p, stduint skip) {
		auto crt = subf;
		if (crt) do {
			if (skip) { skip--; continue; }
			auto& area = treat<SheetTrait>(crt->offs).sheet_area;
			if (area.ifContain(p)) return cast<SheetTrait*>(crt->offs);
		} while (crt = crt->next);
		return nullptr;
	}

	void LayerManager::RegisterTimer(SheetTrait* sheet) {
		if (sheet_parent) {
			sheet_parent->RegisterTimer(sheet);
		} else {
			sheet->timer_timeout_period = 0;
			timer_sheets.Append(sheet);
		}
	}

	void LayerManager::CheckTimers(stduint crt_tick) {
		sys_tick = crt_tick;
		for (stduint i = 0; i < timer_sheets.Count(); i++) {
			SheetTrait* sheet = timer_sheets[i];
			if (sheet->timer_timeout_period && sys_tick >= sheet->timer_timeout_tick) {
				sheet->timer_timeout_tick = sys_tick + sheet->timer_timeout_period;
				sheet->onrupt(SheetEvent::onTimer, Point(0, 0), 0, sys_tick);
			}
		}
	}

}

namespace uni {
	void DrawString_16(SheetTrait& st, const Point2& p, const String& str, Color col) {
	//{} ASCII only
	Point2 pp = p;
	if (!st.sheet_buffer) return;
	for (const char* pstr = str.reference(); *pstr; pstr++) {
		byte ch = *pstr;
		if (!ascii_isprint(*pstr)) continue;
		const uint16(*datptr) = (const uint16(*)) & _BITFONT_ASCII_16x8[ch - 0x20];
		uint16 dat = 0; Reference_T<uint16> dat_bmap _IMM(&dat);
		for0(i, 8) {
			dat = datptr[i];
			for0r(j, 16) {
				if (dat_bmap.bitof(j))
					st.sheet_buffer[pp.x + (pp.y + (j ^ 0b111)) * st.sheet_area.width] = col;
			}
			pp.x++;
		}
		if (pp.x >= st.sheet_area.width) break;
	}
}
}
