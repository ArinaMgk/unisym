// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Module) uni.Witch.Control.TextBox
// Codifiers: @dosconio: 20240422 ~ <Last-check> 
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
// Parallels: (.C#)System.Windows.Forms.TextBox
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

#ifndef _INC_WITCH_CTRL_EDITBOX
#define _INC_WITCH_CTRL_EDITBOX

#include "../Control.hpp"
#include "../../string"
#include "../../Device/_Video.hpp"

extern const char key_map[256], key_map_shift[256];

namespace uni::witch::control {

	class TextBox : public SheetTrait {
	public:
		String text = "";
		bool cursor_visible = false;
		TextBox() : SheetTrait() {}
		~TextBox() = default;

	public:
		virtual Color getPoint(Point p) override {
			_TEMP (void)p; return Color::Black;
		}

		// Update the buffer
		virtual void doshow(void*) override {
			if (!sheet_buffer) sheet_buffer = (Color*)calloc(sheet_area.getArea(), sizeof(Color));
			
			VideoControlInterfaceMARGB8888 vcim(sheet_buffer, sheet_area.getSize());
			vcim.DrawRectangle(Rectangle(Point(0, 0), Size2(sheet_area.width, sheet_area.height), Color::White));
			
			// Draw classic inner borders
			vcim.DrawRectangle(Rectangle(Point(0, 0), Size2(3, sheet_area.height), 0xFF7F7F7F));
			vcim.DrawRectangle(Rectangle(Point(3, 0), Size2(sheet_area.width - 6, 3), 0xFF7F7F7F));
			vcim.DrawRectangle(Rectangle(Point(sheet_area.width - 3, 0), Size2(3, sheet_area.height), 0xFFDDDFE1));
			vcim.DrawRectangle(Rectangle(Point(3, sheet_area.height - 3), Size2(sheet_area.width - 6, 3), 0xFFDDDFE1));

			stdsint cur_draw_x = 4;
			stdsint cur_draw_y = 4;
			stdsint line_height = 16;
			stdsint char_width = 8;
			stdsint margin_right = 8;
			stdsint margin_bottom = 16;

			rostr pstr = text.reference();
			if (pstr) {
				while (*pstr) {
					char ch = *pstr++;
					if (ch == '\n' || ch == '\r') {
						if (ch == '\n' && *pstr == '\n');
						else if (*pstr == '\n' || *pstr == '\r') {
							pstr++; // Skip LF of CRLF
						}
						cur_draw_x = 4;
						cur_draw_y += line_height;
						if (cur_draw_y + margin_bottom > (stdsint)sheet_area.height) {
							break;
						}
					}
					else if (ch >= 32 && ch <= 126) {
						if (cur_draw_x + char_width > (stdsint)sheet_area.width - margin_right) {
							cur_draw_x = 4;
							cur_draw_y += line_height;
							if (cur_draw_y + margin_bottom > (stdsint)sheet_area.height) {
								break;
							}
						}
						char temp_str[2] = { ch, 0 };
						DrawString_16(self, Point(cur_draw_x, cur_draw_y), temp_str, Color::Black);
						cur_draw_x += char_width;
					}
				}
			}

			// Render blinking cursor at the final tracking position
			if (cursor_visible && cur_draw_y + 16 <= (stdsint)sheet_area.height) {
				vcim.DrawRectangle(Rectangle(Point(cur_draw_x, cur_draw_y), Size2(8, 16), Color::Black));
			}

			if (sheet_parent) {
				sheet_parent->Update(this, Rectangle(Point(0, 0), sheet_area.getSize()));
			}
		}

		virtual void onrupt(SheetEvent event, Point rel_p, ...) override {
			if (event == SheetEvent::onClick) {
				para_list args;
				para_ento(args, rel_p);
				unsigned state = para_next(args, unsigned);
				para_endo(args);
				// To only trigger the focus change on left click down:
				if (state & 0b00010000) {
					cursor_visible = true;
					timer_timeout_period = 50;// 0.5s
					doshow(nullptr);
				}
			}
			else if (event == SheetEvent::onEnter) {
				cursor_visible = true;
				timer_timeout_period = 50;// 0.5s
				doshow(nullptr);
			}
			else if (event == SheetEvent::onLeave) {
				para_list args;
				para_ento(args, rel_p);
				int type = para_next(args, int);
				para_endo(args);
				if (type == 1) {
					if (sheet_parent) timer_timeout_period = 0;// stop timer
					cursor_visible = false;
					doshow(nullptr);
				}
			}
			else if (event == SheetEvent::onTimer) {
				para_list args;
				para_ento(args, rel_p);
				stduint current_tick = para_next(args, stduint);
				int type = para_next(args, int);
				para_endo(args);
				(void)current_tick; (void)type; // unused for now but parsed correctly
				
				cursor_visible = !cursor_visible;
				doshow(nullptr); // this will redraw with updated cursor_visible state, resulting in blink
			}
			else if (event == SheetEvent::onKeybd) {
				para_list args;
				para_ento(args, rel_p);
				keyboard_event_t* pkey = para_next(args, keyboard_event_t*);
				para_endo(args);
				if (pkey && (pkey->method == keyboard_event_t::method_t::keydown || pkey->method == keyboard_event_t::method_t::keyrepeat)) {
					bool shift = pkey->mod.l_shift || pkey->mod.r_shift;
					char ch = (shift ? key_map_shift : key_map)[pkey->keycode];

					if (ch == '\b') {
						char* p = text.reflect();
						if (p) {
							stduint len = text.getByteCount();
							if (len > 0) {
								p[len - 1] = '\0';
								text.Refresh();
								doshow(nullptr);
								if (sheet_parent) {
									sheet_parent->AddDirty(sheet_area);
								}
							}
						}
					}
					else if ((ch >= 32 && ch <= 126) || ch == '\n' || ch == '\r') {
						if (ch == '\r') ch = '\n';
						text << ch;
						doshow(nullptr);
						if (sheet_parent) {
							sheet_parent->AddDirty(sheet_area);
						}
					}
				}
			}
		}

		void Start() {
			if (sheet_parent) sheet_parent->RegisterTimer(this);
		}
	};

}
#endif
