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
	void VideoConsole::doshow(void*_) {
		_TEMP 0;
	}


	
	VideoConsole* VideoConsole::crt_self;
	void _VideoConsoleOut(const char* str, stduint len) {
		#define crt_self VideoConsole::crt_self
		void (VideoConsole:: * draw_f)(uni::Point, uni::Color, char) = nullptr;
		if (crt_self->typ < numsof(VideoConsole::DrawCharPosition_f)) {
			draw_f = VideoConsole::DrawCharPosition_f[crt_self->typ];
		}
		else return;
		for0(i, len) if (false);
		else if (str[i] == '\n') {
			crt_self->cursor.y++;
			crt_self->FeedLine();
			Rectangle rect = crt_self->window;
			rect.y = (crt_self->cursor.y - 1) * FontSizeHeight[crt_self->typ];
			rect.height = FontSizeHeight[crt_self->typ];
			// cannot change rect.width because \n and \r are separated
			if (crt_self->sheet_parent && crt_self->buffer) crt_self->sheet_parent->Update(crt_self, rect);
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
			crt_self->curinc();
		}
		// crt_self->sheet_parent->Update(crt_self, crt_self->window);
		#undef crt_self
	}
	int VideoConsole::out(const char* str, stduint len) {
		crt_self = this;
		_VideoConsoleOut(str, len);
		return 0 _TEMP;
	}


	void VideoConsole::thisDrawPoint(const Point& disp, Color color) {
		if (buffer) {
			Color* p0 = buffer + window.width * disp.y + disp.x;
			*p0 = color;
		}
		else vci.DrawPoint(disp, color);
	}
	void VideoConsole::thisDrawRectangle(const Rectangle& rect) {
		if (buffer) {
			for0(y, rect.height) {
				Color* p0 = buffer + window.width * (rect.y + y) + rect.x;
				for0(x, rect.width) *p0++ = rect.color;
			}
		}
		else vci.DrawRectangle(rect);
	}
	void VideoConsole::thisRollup(stduint height) {
		const Size2 scr(size.x * FontSizeWidth[crt_self->typ], size.y * FontSizeHeight[crt_self->typ]);
		if (buffer) {
			stduint h_dif = minof(height, window.height);
			Color* p0 = buffer;
			Color* p = buffer + h_dif * window.width;
			for0(i, (window.height - h_dif) * window.width) {
				*p0++ = *p++;
			}
			for0(i, h_dif* window.width) {
				*p0++ = backcolor;
			}
			sheet_parent->Update(this, window);
		}// quick method
		else vci.RollUp(FontSizeHeight[crt_self->typ], window);
	}
	void VideoConsole::FeedLine() {
		while (cursor.y >= size.y) {
			cursor.y--;
			thisRollup(FontSizeHeight[crt_self->typ]);
		}
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
