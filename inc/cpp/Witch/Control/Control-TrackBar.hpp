// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Module) uni.Witch.Control.TrackBar
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
// Parallels: (.C#)System.Windows.Forms.TrackBar
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

#ifndef _INC_WITCH_CTRL_TRACKBAR
#define _INC_WITCH_CTRL_TRACKBAR

#include "../Control.hpp"
#include "../../Device/_Video.hpp"

namespace uni::witch::control {

	class TrackBar : public SheetTrait {
	public:
		static constexpr stduint kThumbBreadth = 10;

		stdsint min_val = 0;
		stdsint max_val = 100;
		stdsint value = 0;
		bool vertical = false;
		bool dragging = false;
		bool committed = false;

		TrackBar(stdsint _val = 0, stdsint _min = 0, stdsint _max = 100, bool _vertical = false)
			: SheetTrait(), min_val(_min), max_val(_max), value(_val), vertical(_vertical) {
			setValue(_val);
		}
		~TrackBar() = default;

		void setRange(stdsint _min, stdsint _max) {
			min_val = _min;
			max_val = (_max > _min) ? _max : _min;
			setValue(value);
		}

		void setValue(stdsint _val) {
			if (_val < min_val) _val = min_val;
			if (_val > max_val) _val = max_val;
			value = _val;
		}

		stdsint getValue() const { return value; }

		bool TakeCommitted() {
			bool res = committed;
			committed = false;
			return res;
		}

		stdsint ValueFromPoint(Point rel_p) const {
			if (max_val <= min_val) return min_val;
			stdsint span = vertical ? (stdsint)sheet_area.height : (stdsint)sheet_area.width;
			stdsint half = (stdsint)(kThumbBreadth / 2);
			stdsint travel = span - (stdsint)kThumbBreadth;
			if (travel <= 0) return min_val;

			stdsint pos = (vertical ? (stdsint)rel_p.y : (stdsint)rel_p.x) - half;
			if (pos < 0) pos = 0;
			if (pos > travel) pos = travel;

			// Vertical TrackBar puts max_val at the top and min_val at the bottom
			stdsint ratio_num = vertical ? (travel - pos) : pos;
			stdsint mapped = min_val + (stdsint)(((int64)ratio_num * (int64)(max_val - min_val) + travel / 2) / travel);
			if (mapped < min_val) mapped = min_val;
			if (mapped > max_val) mapped = max_val;
			return mapped;
		}

		Rectangle getThumbRect() const {
			stduint w = sheet_area.width;
			stduint h = sheet_area.height;
			if (!w || !h) return Rectangle();

			if (!vertical) {
				stduint thumb_w = w > kThumbBreadth ? kThumbBreadth : w;
				stduint thumb_h = h > 4 ? h - 4 : h;
				stduint travel = w > thumb_w ? w - thumb_w : 0;
				stduint tx = 0;
				if (travel > 0 && max_val > min_val) {
					tx = (stduint)(((int64)(value - min_val) * (int64)travel) / (int64)(max_val - min_val));
				}
				stduint ty = (h - thumb_h) / 2;
				return Rectangle(Point(tx, ty), Size2(thumb_w, thumb_h));
			} else {
				stduint thumb_w = w > 4 ? w - 4 : w;
				stduint thumb_h = h > kThumbBreadth ? kThumbBreadth : h;
				stduint travel = h > thumb_h ? h - thumb_h : 0;
				stduint ty = 0;
				if (travel > 0 && max_val > min_val) {
					ty = travel - (stduint)(((int64)(value - min_val) * (int64)travel) / (int64)(max_val - min_val));
				}
				stduint tx = (w - thumb_w) / 2;
				return Rectangle(Point(tx, ty), Size2(thumb_w, thumb_h));
			}
		}

		virtual Color getPoint(Point) override {
			return 0xFFC6C6C6;
		}

		virtual void doshow(void*) override {
			stduint w = sheet_area.width;
			stduint h = sheet_area.height;
			if (!w || !h) return;
			if (!sheet_buffer) sheet_buffer = (Color*)calloc(sheet_area.getArea(), sizeof(Color));
			if (!sheet_buffer) return;

			for0(i, w * h) sheet_buffer[i] = 0xFFC6C6C6;

			// Draw sunken 4px groove along center
			if (!vertical) {
				stduint gx0 = kThumbBreadth / 2;
				stduint gx1 = w > kThumbBreadth / 2 ? w - kThumbBreadth / 2 : w;
				stduint gy = h >= 4 ? (h - 4) / 2 : 0;
				if (gx1 > gx0 && gy + 3 < h) {
					for (stduint x = gx0; x < gx1; ++x) {
						sheet_buffer[(gy + 0) * w + x] = 0xFF808080;
						sheet_buffer[(gy + 1) * w + x] = 0xFF000000;
						sheet_buffer[(gy + 2) * w + x] = 0xFFDFDFDF;
						sheet_buffer[(gy + 3) * w + x] = 0xFFFFFFFF;
					}
					sheet_buffer[(gy + 1) * w + gx0] = 0xFF808080;
					sheet_buffer[(gy + 2) * w + gx0] = 0xFF808080;
				}
			} else {
				stduint gy0 = kThumbBreadth / 2;
				stduint gy1 = h > kThumbBreadth / 2 ? h - kThumbBreadth / 2 : h;
				stduint gx = w >= 4 ? (w - 4) / 2 : 0;
				if (gy1 > gy0 && gx + 3 < w) {
					for (stduint y = gy0; y < gy1; ++y) {
						sheet_buffer[y * w + (gx + 0)] = 0xFF808080;
						sheet_buffer[y * w + (gx + 1)] = 0xFF000000;
						sheet_buffer[y * w + (gx + 2)] = 0xFFDFDFDF;
						sheet_buffer[y * w + (gx + 3)] = 0xFFFFFFFF;
					}
				}
			}

			// Draw 3D thumb
			Rectangle tr = getThumbRect();
			if (tr.width >= 3 && tr.height >= 3) {
				Color face = dragging ? 0xFFB8B8B8 : 0xFFC6C6C6;
				for0(ty, tr.height) {
					for0(tx, tr.width) {
						Color c = face;
						if (tx == 0 || ty == 0) c = 0xFFFFFFFF;
						else if (tx + 1 == tr.width || ty + 1 == tr.height) c = 0xFF000000;
						else if (tx + 2 == tr.width || ty + 2 == tr.height) c = 0xFF848484;
						sheet_buffer[(tr.y + ty) * w + (tr.x + tx)] = c;
					}
				}
			}

			if (sheet_parent) sheet_parent->Update(this, Rectangle(Point(0, 0), sheet_area.getSize()));
		}

		virtual void onrupt(SheetEvent event, Point rel_p, ...) override {
			Letpara(args, rel_p);
			stduint para1 = para_next(args, stduint);

			bool redraw = false;
			if (event == SheetEvent::onLeave && para1 == 1) {
				if (dragging) {
					dragging = false;
					committed = true;
					redraw = true;
				}
			} else if (event == SheetEvent::onClick) {
				bool lbtn_down = (para1 & 0x10) != 0;
				if (lbtn_down) {
					stdsint next_val = ValueFromPoint(rel_p);
					if (!dragging || next_val != value) {
						dragging = true;
						value = next_val;
						redraw = true;
					}
				} else if (dragging) {
					stdsint next_val = ValueFromPoint(rel_p);
					dragging = false;
					value = next_val;
					committed = true;
					redraw = true;
				}
			} else if (event == SheetEvent::onMoved) {
				if (dragging) {
					if ((para1 & 0x10) != 0) {
						stdsint next_val = ValueFromPoint(rel_p);
						if (next_val != value) {
							value = next_val;
							redraw = true;
						}
					} else {
						dragging = false;
						committed = true;
						redraw = true;
					}
				}
			}

			if (redraw) {
				doshow(nullptr);
			}
		}
	};

}

#endif
