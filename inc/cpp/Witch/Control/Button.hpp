// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Module) uni.Witch.Control.Button
// Codifiers: @dosconio: 20240422 ~ <Last-check> 
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
// Parallels: (.C#)System.Windows.Forms.Button
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

#ifndef _INC_WITCH_CTRL_Button_X
#define _INC_WITCH_CTRL_Button_X

#include "../Control.hpp"
#include "../../string"
#include "../../Device/_Video.hpp"

namespace uni::witch::control {

	class Button : public SheetTrait {
	public:
		String text;
		bool pressed = false;

		Button(const String& _str) : SheetTrait(), text(_str), pressed(false) {}
		~Button() = default;

	public:
		virtual Color getPoint(Point p) override {
			if (pressed) {
				if (p.x == 0 || p.y == 0) return 0xFF000000;
				if (p.x == 1 || p.y == 1) return 0xFF848484;
				if (p.x == sheet_area.width - 1 || p.y == sheet_area.height - 1) return 0xFFFFFFFF;
			} else {
				if (p.x == 0 || p.y == 0) return 0xFFFFFFFF;
				if (p.x == sheet_area.width - 1 || p.y == sheet_area.height - 1) return 0xFF000000;
				if (p.x == sheet_area.width - 2 || p.y == sheet_area.height - 2) return 0xFF848484;
			}
			return 0xFFC6C6C6; // Light gray background
		}

		virtual void doshow(void*) override {
			if (!sheet_buffer) sheet_buffer = (Color*)calloc(sheet_area.getArea(), sizeof(Color));
			
			for0(y, sheet_area.height) {
				for0(x, sheet_area.width) {
					sheet_buffer[y * sheet_area.width + x] = getPoint(Point(x, y));
				}
			}

			if (text.reference()) {
				// Estimate text size (8x16 font)
				stduint text_width = text.getByteCount() * 8;
				stduint text_height = 16;
				
				stdsint draw_x = (sheet_area.width - text_width) / 2;
				stdsint draw_y = (sheet_area.height - text_height) / 2;
				
				if (pressed) {
					draw_x += 1;
					draw_y += 1;
				}
				
				if (draw_x < 0) draw_x = 2;
				if (draw_y < 0) draw_y = 2;

				DrawString_16(self, Point(draw_x, draw_y), text, Color::Black);
			}

			if (sheet_parent) sheet_parent->Update(this, Rectangle(Point(0,0), sheet_area.getSize()));
		}

		virtual void onrupt(SheetEvent event, Point rel_p, ...) override {
			Letpara(args, rel_p);
			stduint para1 = para_next(args, stduint);
			
			bool redraw = false;

			if (event == SheetEvent::onLeave && para1 == 1) {
				if (pressed) {
					pressed = false;
					redraw = true;
				}
			}
			else if (event == SheetEvent::onClick) {
				bool lbtn_down = (para1 & 0x10) != 0;
				if (pressed != lbtn_down) {
					pressed = lbtn_down;
					redraw = true;
				}
			}

			if (redraw) {
				doshow(nullptr);
			}
		}
	};

}
#endif
