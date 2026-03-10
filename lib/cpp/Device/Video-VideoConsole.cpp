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
#if defined(_MCCA) && _MCCA==0x8632
#include "../../../inc/cpp/Device/Buzzer.hpp"
#endif

namespace uni {
	void (VideoConsole::* (VideoConsole::DrawCharPosition_f)[])(uni::Point, uni::Color, char) {
		&VideoConsole::DrawCharPosition_8x5,
			& VideoConsole::DrawCharPosition_16x8
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
						thisDrawPoint(Point(disp.x, disp.y + j), color);
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
						thisDrawPoint(Point(disp.x, disp.y + (j ^ 0b111)), color);
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
	void VideoConsole::doshow(void* _) {
		if (!sheet_buffer && !vci) return;
		VideoControlInterfaceMARGB8888 vcim(sheet_buffer, window.getSize());
		Size2 fontsize(FontSizeWidth[typ], FontSizeHeight[typ]);
		Point posi = cursor;
		// if (cursor.x++ > size.x) {
		// 	cursor.x = 0;
		// 	cursor.y++;
		// }
		Rectangle cursor_rect(posi * fontsize, fontsize, cursor_visible ? forecolor : backcolor);
		(sheet_buffer ? vcim : *vci).DrawRectangle(cursor_rect);
		if (sheet_parent) sheet_parent->Update(this, cursor_rect);
	}


	
	VideoConsole* VideoConsole::crt_self;
	void _VideoConsoleOut(const char* str, stduint len) {
		#define crt_self VideoConsole::crt_self
		void (VideoConsole:: * draw_f)(uni::Point, uni::Color, char) = nullptr;
		if (crt_self->typ < numsof(VideoConsole::DrawCharPosition_f)) {
			draw_f = VideoConsole::DrawCharPosition_f[crt_self->typ];
		}
		else return;
		for0(i, len) if (!str[i]) {
			Rectangle rect;
			rect.x = (crt_self->cursor.x - 1) * FontSizeWidth[crt_self->typ];
			rect.y = crt_self->cursor.y * FontSizeHeight[crt_self->typ];
			rect.width = FontSizeWidth[crt_self->typ];
			rect.height = FontSizeHeight[crt_self->typ];
			crt_self->sheet_parent->Update(crt_self, rect);
		}
		else if (str[i] == '\n') {
			crt_self->cursor.y++;
			crt_self->FeedLine();
			Rectangle rect;
			rect.x = 0;
			rect.y = (crt_self->cursor.y - 1) * FontSizeHeight[crt_self->typ];
			rect.width = crt_self->window.width;
			rect.height = FontSizeHeight[crt_self->typ];
			// cannot change rect.width because \n and \r are separated
			if (crt_self->sheet_parent && crt_self->update_method == 2)
				crt_self->sheet_parent->Update(crt_self, rect);
		}
		else if (str[i] == '\r') {
			crt_self->cursor.x = nil;
		}
		else if (str[i] == '\a') {
			#if defined(_MCCA) && _MCCA==0x8632
			Buzzer::Buzz(true);
			for (volatile size_t i = 0, _LIMIT = (0x1000);i < (_LIMIT);i++)
				for (volatile size_t j = 0, _LIMIT2 = (0x1000);j < (_LIMIT2);j++);
			Buzzer::Buzz(false);
			#endif
		}
		else if (str[i] == '\b') {
			if (crt_self->cursor.x > 0) crt_self->cursor.x--;
			else if (crt_self->cursor.y > 0) {
				crt_self->cursor.y--;
				crt_self->cursor.x = crt_self->size.x - 1;
			}
		}
		else if ((byte)str[i] == (byte)'\xFF' && _LIMIT - i > 1) {// Mecocoa Style II since 20250929
			byte color = str[++i];
			if (color == (byte)0xFF) {
				crt_self->backcolor = crt_self->window.color;
				crt_self->forecolor = ~crt_self->window.color;
				crt_self->forecolor.a = ~crt_self->forecolor.a;
				continue;
			}
			Color col;
			col.a = 0xFF;
			col.b = (color & 0b00010000) ? 0xFA : 0;// stands for PHinA
			col.g = (color & 0b00100000) ? 0xFA : 0;
			col.r = (color & 0b01000000) ? 0xFA : 0;
			crt_self->forecolor = col;
			col.b = (color & 0b00000001) ? 0xFA : 0;
			col.g = (color & 0b00000010) ? 0xFA : 0;
			col.r = (color & 0b00000100) ? 0xFA : 0;
			crt_self->backcolor = col;
		}
		else if ((byte)str[i] == (byte)'\xFE' && _LIMIT - i > 3) {// set backcolor 32bit
			Color col; col.a = 0xFF;
			col.b = str[++i];
			col.g = str[++i];
			col.r = str[++i];
			crt_self->backcolor = col;
		}
		else {
			Size2 fontsize(FontSizeWidth[crt_self->typ], FontSizeHeight[crt_self->typ]);
			Point point(
				crt_self->cursor.x * fontsize.x,
				crt_self->cursor.y * fontsize.y);
			Rectangle fontblock(point, fontsize, crt_self->backcolor);
			crt_self->thisDrawRectangle(fontblock);
			(crt_self->*draw_f) (point, crt_self->forecolor, str[i]);
			// if (crt_self->update_method == 1) {
			// 	Rectangle rect;
			// 	rect.x = crt_self->cursor.x * FontSizeWidth[crt_self->typ];
			// 	rect.y = crt_self->cursor.y * FontSizeHeight[crt_self->typ];
			// 	rect.width = FontSizeWidth[crt_self->typ];
			// 	rect.height = FontSizeHeight[crt_self->typ];
			// 	crt_self->sheet_parent->Update(crt_self, crt_self->window);
			// }
			crt_self->curinc();
		}
		#undef crt_self
	}
	int VideoConsole::out(const char* str, stduint len) {
		crt_self = this;
		_VideoConsoleOut(str, len);
		return 0 _TEMP;
	}
	void VideoConsole::onrupt(SheetEvent event, Point rel_p, ...) {
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
	}


	void VideoConsole::thisDrawPoint(const Point& disp, Color color) {
		if (buffer) {
			Color* p0 = buffer + window.width * disp.y + disp.x;
			*p0 = color;
		}
		else vci->DrawPoint(disp, color);
	}
	void VideoConsole::thisDrawRectangle(const Rectangle& rect) {
		if (buffer) {
			for0(y, rect.height) {
				Color* p0 = buffer + window.width * (rect.y + y) + rect.x;
				for0(x, rect.width) *p0++ = rect.color;
			}
		}
		else vci->DrawRectangle(rect);
	}
	void VideoConsole::thisRollup(stduint height) {
		const Size2 scr(size.x * FontSizeWidth[typ], size.y * FontSizeHeight[typ]);
		if (buffer) {
			VideoControlInterfaceMARGB8888 bvim(buffer, window.getSize());
			Rectangle zero_rect = window;
			zero_rect.x = zero_rect.y = 0;
			bvim.RollUp(height, zero_rect);
			sheet_parent->Update(this, zero_rect);
		}// quick method
		else vci->RollUp(FontSizeHeight[typ], window);
	}
	void VideoConsole::RefreshLine() {
		if (buffer) {
			Rectangle rect;
			rect.x = 0;
			rect.y = cursor.y * FontSizeHeight[typ];
			rect.width = window.width;
			rect.height = FontSizeHeight[typ];
			sheet_parent->Update(this, rect);
		}
	}
		
	void VideoConsole::FeedLine() {
		while (cursor.y >= size.y) {
			cursor.y--;
			thisRollup(FontSizeHeight[typ]);
		}
	}

	void VideoConsole::Start() {
		if (sheet_parent) sheet_parent->RegisterTimer(this);
	}

	void VideoConsole::curinc() {
		if (!size.x || !size.y) return;
		cursor.x++;
		if (cursor.x >= size.x) {
			cursor.x -= size.x;
			cursor.y++;
		}
		if (cursor.y >= size.y) FeedLine();
	}
}
