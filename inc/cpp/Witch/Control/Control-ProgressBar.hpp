// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Module) uni.Witch.Control.ProgressBar
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
// Parallels: (.C#)System.Windows.Forms.ProgressBar
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

#ifndef _INC_WITCH_CTRL_PROGRESSBAR
#define _INC_WITCH_CTRL_PROGRESSBAR

#include "../Control.hpp"
#include "../../Device/_Video.hpp"

namespace uni::witch::control {

	class ProgressBar : public SheetTrait {
	public:
		stdsint min_val = 0;
		stdsint max_val = 100;
		stdsint value = 0;
		Color bar_color = 0xFF000080;

		ProgressBar(stdsint _val = 0, stdsint _min = 0, stdsint _max = 100)
			: SheetTrait(), min_val(_min), max_val(_max), value(_val) {
			setValue(_val);
		}
		~ProgressBar() = default;

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

		virtual Color getPoint(Point p) override {
			if (p.x == 0 || p.y == 0) return 0xFF808080;
			if (p.x + 1 == sheet_area.width || p.y + 1 == sheet_area.height) return 0xFFFFFFFF;
			return 0xFFC6C6C6;
		}

		virtual void doshow(void*) override {
			stduint w = sheet_area.width;
			stduint h = sheet_area.height;
			if (!w || !h) return;
			if (!sheet_buffer) sheet_buffer = (Color*)calloc(sheet_area.getArea(), sizeof(Color));
			if (!sheet_buffer) return;

			for0(y, h) {
				for0(x, w) {
					sheet_buffer[y * w + x] = getPoint(Point(x, y));
				}
			}

			if (w > 4 && h > 4 && max_val > min_val && value > min_val) {
				stduint inner_w = w - 4;
				stduint fill_w = (stduint)(((int64)(value - min_val) * (int64)inner_w) / (int64)(max_val - min_val));
				if (fill_w > inner_w) fill_w = inner_w;
				for (stduint y = 2; y < h - 2; ++y) {
					for (stduint x = 0; x < fill_w; ++x) {
						// Classic segmented blocks (8px block + 2px gap)
						if ((x % 10) < 8) {
							sheet_buffer[y * w + (2 + x)] = bar_color;
						}
					}
				}
			}

			if (sheet_parent) sheet_parent->Update(this, Rectangle(Point(0, 0), sheet_area.getSize()));
		}

		virtual void onrupt(SheetEvent, Point, ...) override {}
	};

}

#endif
