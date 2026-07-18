// ASCII C++ TAB4 CRLF
// Attribute: 
// Codifiers: @ArinaMgk
// Docutitle: uni.Witch.Form
// Reference: (.C#)System.Windows.Forms
// Copyright: ArinaMgk UniSym, Apache License Version 2.0
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

#include "../../../inc/cpp/Witch/Form.hpp"
#include "../../../inc/c/data.h"
#include "../../../inc/cpp/reference"
#include "../../../inc/cpp/Device/_Video.hpp"
#include "../../../inc/c/driver/keyboard.h"

static const int CloseButtonWidth = 15;
static const int CloseButtonHeight = 15;
static const char close_button[CloseButtonHeight][CloseButtonWidth + 1] = {
	"..............@",
	".::::::::::::$@",
	".::::::::::::$@",
	".::@@::::@@::$@",
	".:::@@::@@:::$@",
	".::::@@@@::::$@",
	".:::::@@:::::$@",
	".::::@@@@::::$@",
	".:::@@::@@:::$@",
	".::@@::::@@::$@",
	".::::::::::::$@",
	".::::::::::::$@",
	".::::::::::::$@",
	".$$$$$$$$$$$$$@",
	"@@@@@@@@@@@@@@@",
};
static const char pressed_close_button[CloseButtonHeight][CloseButtonWidth + 1] = {
	"$$$$$$$$$$$$$$@",
	"$::::::::::::.@",
	"$::::::::::::.@",
	"$:::@@::::@@:.@",
	"$::::@@::@@::.@",
	"$:::::@@@@:::.@",
	"$::::::@@::::.@",
	"$:::::@@@@:::.@",
	"$::::@@::@@::.@",
	"$:::@@::::@@:.@",
	"$::::::::::::.@",
	"$::::::::::::.@",
	"$::::::::::::.@",
	"$.............@",
	"@@@@@@@@@@@@@@@",
};
static const char maximize_button[CloseButtonHeight][CloseButtonWidth + 1] = {
	"..............@",
	".::::::::::::$@",
	".::::::::::::$@",
	".::@@@@@@@@::$@",
	".::@@@@@@@@::$@",
	".::@::::::@::$@",
	".::@::::::@::$@",
	".::@::::::@::$@",
	".::@::::::@::$@",
	".::@::::::@::$@",
	".::@@@@@@@@::$@",
	".::::::::::::$@",
	".::::::::::::$@",
	".$$$$$$$$$$$$$@",
	"@@@@@@@@@@@@@@@",
};
static const char pressed_maximize_button[CloseButtonHeight][CloseButtonWidth + 1] = {
	"$$$$$$$$$$$$$$@",
	"$::::::::::::.@",
	"$::::::::::::.@",
	"$:::@@@@@@@@:.@",
	"$:::@@@@@@@@:.@",
	"$:::@::::::@:.@",
	"$:::@::::::@:.@",
	"$:::@::::::@:.@",
	"$:::@::::::@:.@",
	"$:::@::::::@:.@",
	"$:::@@@@@@@@:.@",
	"$::::::::::::.@",
	"$::::::::::::.@",
	"$.............@",
	"@@@@@@@@@@@@@@@",
};
static const char minimize_button[CloseButtonHeight][CloseButtonWidth + 1] = {
	"..............@",
	".::::::::::::$@",
	".::::::::::::$@",
	".::::::::::::$@",
	".::::::::::::$@",
	".::::::::::::$@",
	".::::::::::::$@",
	".::::::::::::$@",
	".::::::::::::$@",
	".::@@@@@@@:::$@",
	".::@@@@@@@:::$@",
	".::::::::::::$@",
	".::::::::::::$@",
	".$$$$$$$$$$$$$@",
	"@@@@@@@@@@@@@@@",
};
static const char pressed_minimize_button[CloseButtonHeight][CloseButtonWidth + 1] = {
	"$$$$$$$$$$$$$$@",
	"$::::::::::::.@",
	"$::::::::::::.@",
	"$::::::::::::.@",
	"$::::::::::::.@",
	"$::::::::::::.@",
	"$::::::::::::.@",
	"$::::::::::::.@",
	"$::::::::::::.@",
	"$:::@@@@@@@::.@",
	"$:::@@@@@@@::.@",
	"$::::::::::::.@",
	"$::::::::::::.@",
	"$.............@",
	"@@@@@@@@@@@@@@@",
};

uni::Color uni::Witch::Form_CloseButton::getPoint(Point p)
{
	auto dat = pressed ? pressed_close_button[p.y][p.x] : close_button[p.y][p.x];
	if (dat == '@') {
	return (0xFF000000);
	} else if (dat == '$') {
	return (0xFF848484);
	} else if (dat == ':') {
	return (0xFFC6C6C6);
	}
	else return (0xFFFFFFFF);
}

uni::Color uni::Witch::Form_MaximizeButton::getPoint(Point p)
{
	if (!visible) return 0xFFFFFFFF;
	auto dat = pressed && enabled ? pressed_maximize_button[p.y][p.x] : maximize_button[p.y][p.x];
	if (dat == '@') {
		return enabled ? (0xFF000000) : (0xFF848484);
	} else if (dat == '$') {
		return (0xFF848484);
	} else if (dat == ':') {
		return (0xFFC6C6C6);
	}
	else return (0xFFFFFFFF);
}

uni::Color uni::Witch::Form_MinimizeButton::getPoint(Point p)
{
	if (!visible) return 0xFFFFFFFF;
	auto dat = pressed && enabled ? pressed_minimize_button[p.y][p.x] : minimize_button[p.y][p.x];
	if (dat == '@') {
		return enabled ? (0xFF000000) : (0xFF848484);
	} else if (dat == '$') {
		return (0xFF848484);
	} else if (dat == ':') {
		return (0xFFC6C6C6);
	}
	else return (0xFFFFFFFF);
}

uni::Color uni::Witch::Form_TitleBar::getPoint(Point p)
{
	Color c = 0xFF000000;
	if (!active) {
		c.r = c.b = c.g = 0x80 + ((uint32)p.x * 0x30) / sheet_area.width;
	}
	else {
		c.b = 0x7F + ((uint32)p.x * 0x40) / sheet_area.width;
		c.g = 0x00 + ((uint32)p.x * 0x80) / sheet_area.width;
	}
	return c;
}

void uni::Witch::Form::doshow(void* _)
{
	
}

void uni::Witch::Form::onrupt(SheetEvent event, Point rel_p, ...)
{
	bool redraw = false;
	auto update_title_bar = [this]() {
		if (!this->sheet_buffer) return;
		for0(j, title_bar.sheet_area.height) {
			Color* p = this->sheet_buffer + (title_bar.sheet_area.y + j) * this->sheet_area.width + title_bar.sheet_area.x;
			for0(i, title_bar.sheet_area.width) {
				*p++ = this->getPoint(Point(title_bar.sheet_area.x + i, title_bar.sheet_area.y + j));
			}
		}
	};
	auto update_close_btn = [this]() {
		if (!this->sheet_buffer) return;
		for0(j, close_btn.sheet_area.height) {
			Color* p = this->sheet_buffer + (close_btn.sheet_area.y + j) * this->sheet_area.width + close_btn.sheet_area.x;
			for0(i, close_btn.sheet_area.width) {
				*p++ = this->getPoint(Point(close_btn.sheet_area.x + i, close_btn.sheet_area.y + j));
			}
		}
	};
	auto update_max_btn = [this]() {
		if (!this->sheet_buffer || !max_btn.visible) return;
		for0(j, max_btn.sheet_area.height) {
			Color* p = this->sheet_buffer + (max_btn.sheet_area.y + j) * this->sheet_area.width + max_btn.sheet_area.x;
			for0(i, max_btn.sheet_area.width) {
				*p++ = this->getPoint(Point(max_btn.sheet_area.x + i, max_btn.sheet_area.y + j));
			}
		}
	};
	auto update_min_btn = [this]() {
		if (!this->sheet_buffer || !min_btn.visible) return;
		for0(j, min_btn.sheet_area.height) {
			Color* p = this->sheet_buffer + (min_btn.sheet_area.y + j) * this->sheet_area.width + min_btn.sheet_area.x;
			for0(i, min_btn.sheet_area.width) {
				*p++ = this->getPoint(Point(min_btn.sheet_area.x + i, min_btn.sheet_area.y + j));
			}
		}
	};
	Letpara(args, rel_p);
	stduint para1 = para_next(args, stduint);
	// notice layman if button_dn the title bar
	if (event == SheetEvent::onLeave && para1 == 1) {
		active = false, title_bar.active = false;
		update_title_bar();
		redraw = true;
		if (client_area.sheet_area.ifContain(rel_p)) {
			client_area.onrupt(event, rel_p - client_area.sheet_area.getVertex(), para1);
		}
		else if (focus_sheet) {
			focus_sheet->onrupt(event, Point(0, 0), para1);
		}
		
		if (msg_queue.Count() < 510) {
			SheetMessage smsg;
			smsg.event = event;
			smsg.args[0] = rel_p.x;
			smsg.args[1] = rel_p.y;
			smsg.args[2] = para1;
			smsg.args[3] = 0;
			this->PushMessage(smsg);
		}
	}
	else if (event == SheetEvent::onMoved || event == SheetEvent::onClick) {
		if (event == SheetEvent::onClick) {
			bool lbtn_down = (para1 & 0x10) != 0;
			if (close_btn.sheet_area.ifContain(rel_p)) {
				if (close_btn.pressed != lbtn_down) {
					close_btn.pressed = lbtn_down;
					update_close_btn();
					redraw = true;
				}
			} else if (close_btn.pressed) {
				close_btn.pressed = false;
				update_close_btn();
				redraw = true;
			}
			if (max_btn.visible && max_btn.enabled) {
				if (max_btn.sheet_area.ifContain(rel_p)) {
					if (max_btn.pressed != lbtn_down) {
						max_btn.pressed = lbtn_down;
						update_max_btn();
						redraw = true;
					}
				} else if (max_btn.pressed) {
					max_btn.pressed = false;
					update_max_btn();
					redraw = true;
				}
			}
			if (min_btn.visible && min_btn.enabled) {
				if (min_btn.sheet_area.ifContain(rel_p)) {
					if (min_btn.pressed != lbtn_down) {
						min_btn.pressed = lbtn_down;
						update_min_btn();
						redraw = true;
					}
				} else if (min_btn.pressed) {
					min_btn.pressed = false;
					update_min_btn();
					redraw = true;
				}
			}
			
			if (!active) {
				active = true, title_bar.active = true;
				update_title_bar();
				redraw = true;
			}
			if (sheet_parent && title_bar.sheet_area.ifContain(rel_p) &&
				!close_btn.sheet_area.ifContain(rel_p)) {
				para_ento(args, rel_p);
				sheet_parent->Dorupt(this, event, rel_p, args);
			}
		}
		if (client_area.sheet_area.ifContain(rel_p)) {
			client_area.onrupt(event, rel_p - client_area.sheet_area.getVertex(), para1);
		}
		else if (focus_sheet) {
			focus_sheet->onrupt(SheetEvent::onEnter, Point(0, 0));
		}
		
		// Only dispatch messages to the application queue if the event occurs within the client area,
		// or if it's a click on the close button (to allow the application to handle window closing).
		bool is_client_area = client_area.sheet_area.ifContain(rel_p);
		bool is_close_click = (event == SheetEvent::onClick) && close_btn.sheet_area.ifContain(rel_p);
		bool is_max_click = (event == SheetEvent::onClick) && max_btn.visible && max_btn.enabled && max_btn.sheet_area.ifContain(rel_p);
		bool is_min_click = (event == SheetEvent::onClick) && min_btn.visible && min_btn.enabled && min_btn.sheet_area.ifContain(rel_p);

		if (is_client_area || is_close_click || is_max_click || is_min_click) {
			// Limit congestion: 32 for onMoved, 510 for all events
			if (event == SheetEvent::onMoved && msg_queue.Count() >= 64) return; // Increased limit for smoother drawing
			if (msg_queue.Count() >= 510) return;

			// Dispatch new message
			SheetMessage smsg;
			smsg.event = event;
			smsg.args[0] = rel_p.x;
			smsg.args[1] = rel_p.y;
			smsg.args[2] = para1;
			// Component ID: 1 for Close Button, 2 for Max, 3 for Min, 0 for Client Area (Default)
			if (is_close_click) smsg.args[3] = 1;
			else if (is_max_click) smsg.args[3] = 2;
			else if (is_min_click) smsg.args[3] = 3;
			else smsg.args[3] = 0; 
			this->PushMessage(smsg);
		}
	}
	else if (event == SheetEvent::onKeybd) {
		// Dispatch keyboard message to queue
		if (msg_queue.Count() < 510) {
			SheetMessage smsg;
			smsg.event = event;
			// para1 has already consumed the first argument, which is the pointer to event
			*(keyboard_event_t*)smsg.args = *(keyboard_event_t*)para1;
			this->PushMessage(smsg);
		}
	}
	else if (event == SheetEvent::onTimer) {
		if (msg_queue.Count() < 510) {
			SheetMessage smsg;
			smsg.event = event;
			smsg.args[3] = para_next(args, stduint); // Get sys_tick
			this->PushMessage(smsg);
			// ploginfo("Form onTimer pushed: %u", smsg.args[3]);
		}
	}
	if (redraw) {
		if (title_visable && Title.reference()) DrawString_16(self, Point2(3, 3), Title, Color::Black);
		if (title_visable && Title.reference()) DrawString_16(self, Point2(2, 2), Title, Color::White);
		if (sheet_parent) sheet_parent->Update(this, title_bar.sheet_area);
	}
}

void uni::Witch::Form::setSheet(LayerManager& layman, const Rectangle& rect, Color* buffer) {
	InitializeSheet(layman, rect.getVertex(), rect.getSize());
	window = rect;
	
	if (title_visable) {
		int btn_x = rect.width - 17;
		close_btn.refSheetParent() = this;//
		close_btn.sheet_area = Rectangle(Point(btn_x, 2), Size2(CloseButtonWidth, CloseButtonHeight));
		close_btn.sheet_node.offs = dynamic_cast<SheetTrait*>(&close_btn);
		sheet_node.subf = &close_btn.sheet_node;
		
		if (max_btn.visible || min_btn.visible) btn_x -= 2;
		if (max_btn.visible) {
			btn_x -= 15;
			max_btn.sheet_area = Rectangle(Point(btn_x, 2), Size2(CloseButtonWidth, CloseButtonHeight));
		} else {
			max_btn.sheet_area = Rectangle(Point(0, 0), Size2(0, 0));
		}
		max_btn.refSheetParent() = this;
		max_btn.sheet_node.offs = dynamic_cast<SheetTrait*>(&max_btn);
		sheet_node.subf->Tail()->next = &max_btn.sheet_node;
		
		if (min_btn.visible) {
			btn_x -= 15;
			min_btn.sheet_area = Rectangle(Point(btn_x, 2), Size2(CloseButtonWidth, CloseButtonHeight));
		} else {
			min_btn.sheet_area = Rectangle(Point(0, 0), Size2(0, 0));
		}
		min_btn.refSheetParent() = this;
		min_btn.sheet_node.offs = dynamic_cast<SheetTrait*>(&min_btn);
		sheet_node.subf->Tail()->next = &min_btn.sheet_node;
		
		title_bar.refSheetParent() = this;//
		title_bar.sheet_area = Rectangle(Point(1, 1), Size2(rect.width - 2, 17));
		title_bar.sheet_node.offs = dynamic_cast<SheetTrait*>(&title_bar);
		sheet_node.subf->Tail()->next = &title_bar.sheet_node;
		client_area.refSheetParent() = this;
		client_area.sheet_area = Rectangle(Point(1, 18), Size2(rect.width - 2, rect.height - 19));
		client_area.window = client_area.sheet_area;
		client_area.refSheetNode().offs = dynamic_cast<SheetTrait*>(&client_area);
		sheet_node.subf->Tail()->next = &client_area.refSheetNode();
	} else {
		close_btn.sheet_area = Rectangle(Point(0, 0), Size2(0, 0));
		max_btn.sheet_area = Rectangle(Point(0, 0), Size2(0, 0));
		min_btn.sheet_area = Rectangle(Point(0, 0), Size2(0, 0));
		title_bar.sheet_area = Rectangle(Point(0, 0), Size2(0, 0));

		client_area.refSheetParent() = this;
		client_area.sheet_area = Rectangle(Point(0, 0), Size2(rect.width, rect.height));
		client_area.window = client_area.sheet_area;
		client_area.refSheetNode().offs = dynamic_cast<SheetTrait*>(&client_area);
		sheet_node.subf = &client_area.refSheetNode();
	}

	client_area.window.color = 0xFFC6C6C6;

	if (buffer) {
		Color* p = buffer;
		for0(j, rect.height) for0(i, rect.width) {
			*p++ = getPoint(Point(i, j));
		}
		sheet_buffer = buffer;
		if (title_visable && Title.reference()) DrawString_16(self, Point2(3, 3), Title, Color::Black);
		if (title_visable && Title.reference()) DrawString_16(self, Point2(2, 2), Title, Color::White);

	}

}



