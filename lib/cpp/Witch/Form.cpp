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

uni::Color uni::Witch::Form_CloseButton::getPoint(Point p)
{
	auto dat = close_button[p.y][p.x];
	if (dat == '@') {
	return (0xFF000000);
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
	}
	else if (event == SheetEvent::onMoved || event == SheetEvent::onClick) {
		if (event == SheetEvent::onClick) {
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
		
		// Handle message queueing with merging and limiting
		if (event == SheetEvent::onMoved && !msg_queue.isEmpty() && msg_queue.refTail().event == SheetEvent::onMoved) {
			// Merge sequential moving events
			SheetMessage& last = msg_queue.refTail();
			last.args[0] = rel_p.x;
			last.args[1] = rel_p.y;
			last.args[2] = para1;
		}
		else {
			// Limit congestion: 32 for onMoved, 510 for all events
			if (event == SheetEvent::onMoved && msg_queue.Count() >= 32) return;
			if (msg_queue.Count() >= 510) return;

			// Dispatch new message
			SheetMessage smsg;
			smsg.event = event;
			smsg.args[0] = rel_p.x;
			smsg.args[1] = rel_p.y;
			smsg.args[2] = para1;
			// Determine Component ID
			if (close_btn.sheet_area.ifContain(rel_p)) smsg.args[3] = 1; // Close Button
			else if (title_bar.sheet_area.ifContain(rel_p)) smsg.args[3] = 2; // Title Bar
			else smsg.args[3] = 0; // Client Area (Default)
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
	if (redraw) {
		if (Title.reference()) DrawString_16(self, Point2(3, 3), Title, Color::Black);
		if (Title.reference()) DrawString_16(self, Point2(2, 2), Title, Color::White);
		if (sheet_parent) sheet_parent->Update(this, title_bar.sheet_area);
	}
}

void uni::Witch::Form::setSheet(LayerManager& layman, const Rectangle& rect, Color* buffer) {
	InitializeSheet(layman, rect.getVertex(), rect.getSize());
	window = rect;
	close_btn.sheet_area = Rectangle(Point(rect.width - 17, 2), Size2(CloseButtonWidth, CloseButtonHeight));
	close_btn.sheet_node.offs = dynamic_cast<SheetTrait*>(&close_btn);
	sheet_node.subf = &close_btn.sheet_node;
	title_bar.sheet_area = Rectangle(Point(1, 1), Size2(rect.width - 2, 17));
	title_bar.sheet_node.offs = dynamic_cast<SheetTrait*>(&title_bar);
	sheet_node.subf->Tail()->next = &title_bar.sheet_node;
	client_area.refSheetParent() = this;
	client_area.sheet_area = Rectangle(Point(1, 18), Size2(rect.width - 2, rect.height - 19));
	client_area.window = client_area.sheet_area;
	client_area.refSheetNode().offs = dynamic_cast<SheetTrait*>(&client_area);
	sheet_node.subf->Tail()->next = &client_area.refSheetNode();

	client_area.window.color = 0xFFC6C6C6;

	if (buffer) {
		Color* p = buffer;
		for0(j, rect.height) for0(i, rect.width) {
			*p++ = getPoint(Point(i, j));
		}
		sheet_buffer = buffer;
		if (Title.reference()) DrawString_16(self, Point2(3, 3), Title, Color::Black);
		if (Title.reference()) DrawString_16(self, Point2(2, 2), Title, Color::White);

	}

}



