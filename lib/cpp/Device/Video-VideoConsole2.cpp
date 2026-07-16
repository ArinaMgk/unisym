// ASCII CPP TAB4 CRLF
// Docutitle: (Module) Video - VideoConsole2
// Codifiers: @dosconio: 20260410
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

// VideoConsole2 -- character-unit console with lazy on-demand rendering.
//
// ANSI/VT100 SGR color support (\x1b[...m):
//   The parser is a persistent state machine stored in the VideoConsole2
//   object itself.  Sequences like "\x1b[3" + "2m" spread across two
//   separate out() calls are handled correctly.
//
//   esc_state == 0  : idle (normal character output)
//   esc_state == 1  : ESC (0x1b) received, waiting for '[' to confirm CSI
//   esc_state == 2  : inside CSI parameter string, collecting digits / ';'
//
// Supported SGR codes:
//   0           Reset to console defaults
//   30-37       Standard foreground colors
//   38;5;n      256-color foreground
//   38;2;r;g;b  24-bit foreground
//   39          Default foreground
//   40-47       Standard background colors
//   48;5;n      256-color background
//   48;2;r;g;b  24-bit background
//   49          Default background
//   90-97       Bright foreground colors
//   100-107     Bright background colors
//   (Others consumed, attr reserved for future bold/underline/blink.)

#include "../../../inc/cpp/Device/_Video.hpp"
#include "../../../inc/cpp/Witch/TextChrome.hpp"
#include "../../../inc/cpp/reference"
#include "../../../inc/c/data.h"
#include "../../../inc/c/multichar.h"
#if defined(_MCCA) && _MCCA==0x8632
#include "../../../inc/cpp/Device/Buzzer.hpp"
#endif

namespace uni {

	// -------------------------------------------------------------------------
	// ANSI 16-color palette (standard 8 + bright 8, classic xterm/VGA values).
	// -------------------------------------------------------------------------
	static Color vc2AnsiColor(int idx) {
		static const uint8 tbl[16][3] = {
			{   0,   0,   0 }, // 0  Black
			{ 170,   0,   0 }, // 1  Red
			{   0, 170,   0 }, // 2  Green
			{ 170, 170,   0 }, // 3  Yellow (dark)
			{   0,   0, 170 }, // 4  Blue
			{ 170,   0, 170 }, // 5  Magenta
			{   0, 170, 170 }, // 6  Cyan
			{ 170, 170, 170 }, // 7  White (light gray)
			{  85,  85,  85 }, // 8  Bright Black (dark gray)
			{ 255,  85,  85 }, // 9  Bright Red
			{  85, 255,  85 }, // 10 Bright Green
			{ 255, 255,  85 }, // 11 Bright Yellow
			{  85,  85, 255 }, // 12 Bright Blue
			{ 255,  85, 255 }, // 13 Bright Magenta
			{  85, 255, 255 }, // 14 Bright Cyan
			{ 255, 255, 255 }, // 15 Bright White
		};
		if (idx < 0 || idx > 15) idx = 7;
		Color c; c.a = 0xFF;
		c.r = tbl[idx][0]; c.g = tbl[idx][1]; c.b = tbl[idx][2];
		return c;
	}

	// -------------------------------------------------------------------------
	// 256-color palette lookup.
	// -------------------------------------------------------------------------
	static Color vc2AnsiColor256(int n) {
		if (n < 0) n = 0; if (n > 255) n = 255;
		if (n < 16) return vc2AnsiColor(n);
		if (n >= 232) {
			uint8 v = (uint8)(8 + (n - 232) * 10);
			Color c; c.a = 0xFF; c.r = c.g = c.b = v; return c;
		}
		n -= 16;
		static const uint8 cube[6] = { 0, 95, 135, 175, 215, 255 };
		uint8 b = cube[n % 6]; n /= 6;
		uint8 g = cube[n % 6]; n /= 6;
		uint8 r = cube[n];
		Color c; c.a = 0xFF; c.r = r; c.g = g; c.b = b; return c;
	}

	// -------------------------------------------------------------------------
	// Apply one collected SGR parameter sequence.
	// params[0..nparams-1] hold the decoded values.
	// -------------------------------------------------------------------------
	static void vc2ApplySGR(VideoConsole2* con, int* params, int nparams) {
		for (int pi = 0; pi < nparams; pi++) {
			int p = params[pi];
			if (p == 0) {
				con->backcolor = con->window.color;
				con->forecolor = ~con->window.color;
				con->forecolor.a = ~con->forecolor.a;
			}
			else if (p >= 30 && p <= 37) {
				con->forecolor = vc2AnsiColor(p - 30);
			}
			else if (p == 38 && pi + 1 < nparams) {
				if (params[pi + 1] == 5 && pi + 2 < nparams) {
					con->forecolor = vc2AnsiColor256(params[pi + 2]);
					pi += 2;
				}
				else if (params[pi + 1] == 2 && pi + 4 < nparams) {
					Color col; col.a = 0xFF;
					col.r = (uint8)params[pi + 2];
					col.g = (uint8)params[pi + 3];
					col.b = (uint8)params[pi + 4];
					con->forecolor = col;
					pi += 4;
				}
			}
			else if (p == 39) {
				con->forecolor = ~con->window.color;
				con->forecolor.a = ~con->forecolor.a;
			}
			else if (p >= 40 && p <= 47) {
				con->backcolor = vc2AnsiColor(p - 40);
			}
			else if (p == 48 && pi + 1 < nparams) {
				if (params[pi + 1] == 5 && pi + 2 < nparams) {
					con->backcolor = vc2AnsiColor256(params[pi + 2]);
					pi += 2;
				}
				else if (params[pi + 1] == 2 && pi + 4 < nparams) {
					Color col; col.a = 0xFF;
					col.r = (uint8)params[pi + 2];
					col.g = (uint8)params[pi + 3];
					col.b = (uint8)params[pi + 4];
					con->backcolor = col;
					pi += 4;
				}
			}
			else if (p == 49) {
				con->backcolor = con->window.color;
			}
			else if (p >= 90 && p <= 97) {
				con->forecolor = vc2AnsiColor((p - 90) + 8);
			}
			else if (p >= 100 && p <= 107) {
				con->backcolor = vc2AnsiColor((p - 100) + 8);
			}
			// 1(bold),2(dim),3(italic),4(underline),5(blink),7(reverse) →
			// TODO: store in attr for future rendering.
		}
	}

	// -------------------------------------------------------------------------
	// Build a blank BufferChar (space) with the given colors.
	// -------------------------------------------------------------------------
	static inline BufferChar MakeBlankCell(Color fg, Color bg) {
		BufferChar c;
		c.back_color   = bg;
		c.fore_color   = fg;
		c.attr         = 0;
		c.unicode_char = (uint32)' ';
		return c;
	}

	// -------------------------------------------------------------------------
	// BitmapFontEngine Implementation
	// -------------------------------------------------------------------------
	stduint VideoConsole2::getLineBufferSize() const {
		if (!font_engine) return 0;
		Size2 cell_size = font_engine->GetCellSize();
		return cols * cell_size.x * cell_size.y;
	}

	// -------------------------------------------------------------------------
	// Static member
	// -------------------------------------------------------------------------
	VideoConsole2* VideoConsole2::crt_self2 = nullptr;

	// -------------------------------------------------------------------------
	// Constructor
	// -------------------------------------------------------------------------
	VideoConsole2::VideoConsole2(
		const VideoControlInterface* vci_,
		const Rectangle& win,
		const Color& fore_color,
		const Color& back_color)
		: Console_t(), SheetTrait(),
		vci(vci_),
		buffer(nullptr), size(0), font_engine(nullptr),
		forecolor(fore_color), backcolor(back_color),
		window(win)
	{
		cols = 0;
		rows = 0;
		window.color = back_color;
	}

	// -------------------------------------------------------------------------
	// setFontEngine
	// -------------------------------------------------------------------------
	void VideoConsole2::setFontEngine(const FontEngine* fe) {
		font_engine = fe;
		if (font_engine) {
			Size2 cell_size = font_engine->GetCellSize();
			size.x = window.width  / cell_size.x;
			size.y = window.height / cell_size.y;
			cols   = size.x;
			rows   = size.y;
		}
	}

	// -------------------------------------------------------------------------
	// setBuffers
	// -------------------------------------------------------------------------
	void VideoConsole2::setBuffers(Color* pixel_buf, BufferChar* text_storage, Color* line_storage) {
		buffer         = pixel_buf;
		text_buf       = text_storage;
		line_buf       = line_storage;
		line_buf_row   = -1;
		line_buf_valid = false;
		esc_state  = 0;
		esc_nparams = 0;
		for0(k, 8) esc_params[k] = 0;
		if (text_buf) {
			BufferChar blank = MakeBlankCell(forecolor, backcolor);
			for0(i, cols * rows) text_buf[i] = blank;
		}
	}

	// -------------------------------------------------------------------------
	// Clear
	// -------------------------------------------------------------------------
	void VideoConsole2::Clear() {
		cursor.x       = 0;
		cursor.y       = 0;
		line_buf_valid = false;
		line_buf_row   = -1;
		esc_state  = 0;
		esc_nparams = 0;
		for0(k, 8) esc_params[k] = 0;
		if (text_buf) {
			BufferChar blank = MakeBlankCell(forecolor, backcolor);
			for0(i, cols * rows) text_buf[i] = blank;
		}
		if (buffer) {
			stduint total = window.width * window.height;
			Color* p = buffer;
			for0(i, total) *p++ = backcolor;
			if (sheet_parent) sheet_parent->Update(this, window);
			else if (vci) { Rectangle r = window; r.color = backcolor; vci->DrawRectangle(r); }
		}
		else if (sheet_parent) {
			sheet_parent->Update(this, window);
		}
		else if (vci) {
			Rectangle r = window; r.color = backcolor;
			vci->DrawRectangle(r);
		}
	}

	void VideoConsole2::Reconfigure(const VideoControlInterface* vci_, LayerManager& parent, const Rectangle& win) {
		Stop();
		vci = vci_;
		window = win;
		window.color = backcolor;
		InitializeSheet(parent, win.getVertex(), win.getSize());
		if (font_engine) {
			Size2 cell_size = font_engine->GetCellSize();
			size.x = window.width / cell_size.x;
			size.y = window.height / cell_size.y;
			cols = size.x;
			rows = size.y;
		}
		else {
			size = Size2(0, 0);
			cols = 0;
			rows = 0;
		}

		BufferChar* text_storage = cols && rows ? new BufferChar[cols * rows] : nullptr;
		Color* line_storage = getLineBufferSize() ? new Color[getLineBufferSize()] : nullptr;
		setBuffers(nullptr, text_storage, line_storage);
		Clear();
		Start();
	}

	// -------------------------------------------------------------------------
	// Start
	// -------------------------------------------------------------------------
	void VideoConsole2::Start() {
		if (sheet_parent) sheet_parent->RegisterTimer(this);
	}

	// -------------------------------------------------------------------------
	// Stop
	// -------------------------------------------------------------------------
	void VideoConsole2::Stop() {
		if (this->timer_root_manager) {
			this->timer_root_manager->UnregisterTimer(this);
			this->timer_root_manager = nullptr;
		}
		// Use delete[] to match new[] allocation in console initialization
		if (text_buf) { delete[] text_buf; text_buf = nullptr; }
		if (line_buf) { delete[] line_buf; line_buf = nullptr; }
	}

	// -------------------------------------------------------------------------
	// doshow
	// -------------------------------------------------------------------------
	void VideoConsole2::doshow(void*) {
		if (!font_engine) return;
		stduint font_w = font_engine->GetCellSize().x;
		stduint font_h = font_engine->GetCellSize().y;
		Size2 fontsize((stdsint)font_w, (stdsint)font_h);
		Rectangle cursor_rect(cursor * fontsize, fontsize, cursor_visible ? forecolor : backcolor);

		if (sheet_buffer) {
			VideoControlInterfaceMARGB8888 vcim(sheet_buffer, window.getSize());
			vcim.DrawRectangle(cursor_rect);
			if (sheet_parent) sheet_parent->Update(this, cursor_rect);
		}
		else if (buffer) {
			Color fill = cursor_visible ? forecolor : backcolor;
			stduint px = (stduint)cursor.x * font_w;
			stduint py = (stduint)cursor.y * font_h;
			for (stduint sy = 0; sy < font_h; sy++) {
				Color* row = buffer + (py + sy) * window.width + px;
				for (stduint sx = 0; sx < font_w; sx++) row[sx] = fill;
			}
			if (sheet_parent) sheet_parent->Update(this, cursor_rect);
		}
		else if (vci) {
			vci->DrawRectangle(cursor_rect);
		}
		else if (sheet_parent) {
			sheet_parent->Update(this, cursor_rect);
		}
	}

	// -------------------------------------------------------------------------
	// onrupt
	// -------------------------------------------------------------------------
	void VideoConsole2::onrupt(SheetEvent event, Point rel_p, ...) {
		if (event == SheetEvent::onClick) {
			para_list args;
			para_ento(args, rel_p);
			unsigned state = para_next(args, unsigned);
			para_endo(args);
			if (state & 0b00010000) {
				cursor_visible = true;
				timer_timeout_period = 50;
				doshow(nullptr);
			}
		}
		else if (event == SheetEvent::onEnter) {
			cursor_visible = true;
			timer_timeout_period = 50;
			doshow(nullptr);
		}
		else if (event == SheetEvent::onLeave) {
			para_list args;
			para_ento(args, rel_p);
			int type = para_next(args, int);
			para_endo(args);
			if (type == 1) {
				if (sheet_parent) timer_timeout_period = 0;
				cursor_visible = false;
				doshow(nullptr);
			}
		}
		else if (event == SheetEvent::onTimer) {
			para_list args;
			para_ento(args, rel_p);
			stduint current_tick = para_next(args, stduint);
			int type             = para_next(args, int);
			para_endo(args);
			(void)current_tick; (void)type;
			cursor_visible = !cursor_visible;
			doshow(nullptr);
		}
	}

	// -------------------------------------------------------------------------
	// getPoint -- per-pixel color query for buffer-free (getPoint) mode.
	// -------------------------------------------------------------------------
	Color VideoConsole2::getPoint(Point p) {
		if (!text_buf || !font_engine) return backcolor;

		stduint font_w = font_engine->GetCellSize().x;
		stduint font_h = font_engine->GetCellSize().y;
		stduint cx = (stduint)p.x / font_w;
		stduint cy = (stduint)p.y / font_h;

		if (cx >= cols || cy >= rows) return backcolor;

		stduint gx = (stduint)p.x % font_w;
		stduint gy = (stduint)p.y % font_h;

		Color result;
		const BufferChar& cell = text_buf[cy * cols + cx];

		if (line_buf && line_buf_valid && (stdsint)cy == line_buf_row) {
			result = line_buf[gy * (cols * font_w) + cx * font_w + gx];
		}
		else if (line_buf) {
			EnsureLineBuffer(cy);
			result = line_buf[gy * (cols * font_w) + cx * font_w + gx];
		}
		else {
			result = font_engine->GetPixel(cell.unicode_char, gx, gy, cell.fore_color, cell.back_color);
		}

		if (cursor_visible && cx == cursor.x && cy == cursor.y)
			return forecolor;

		return result;
	}

	// -------------------------------------------------------------------------
	// Putchar
	// -------------------------------------------------------------------------
	void VideoConsole2::Putchar(stduint cx, stduint cy, BufferChar cell) {
		if (!text_buf || cx >= cols || cy >= rows) return;
		text_buf[cy * cols + cx] = cell;
		if ((stdsint)cy == line_buf_row) line_buf_valid = false;
	}

	// -------------------------------------------------------------------------
	// EnsureLineBuffer
	// -------------------------------------------------------------------------
	void VideoConsole2::EnsureLineBuffer(stduint row) {
		if (!line_buf || !text_buf || !font_engine) return;
		if (line_buf_valid && (stdsint)row == line_buf_row) return;

		stduint font_w   = font_engine->GetCellSize().x;
		stduint font_h   = font_engine->GetCellSize().y;
		stduint row_px_w = cols * font_w;

		for0(cx, cols) {
			const BufferChar& cell = text_buf[row * cols + cx];
			witch::control::TextChrome::RasterCellToPixels(font_engine, line_buf, row_px_w, cx * font_w,
				cell.unicode_char, cell.fore_color, cell.back_color);
		}
		line_buf_row   = (stdsint)row;
		line_buf_valid = true;
	}

	// -------------------------------------------------------------------------
	// BlitLineBuffer
	// -------------------------------------------------------------------------
	void VideoConsole2::BlitLineBuffer(stduint row) {
		if (!line_buf || !buffer || !font_engine) return;

		stduint font_w   = font_engine->GetCellSize().x;
		stduint font_h   = font_engine->GetCellSize().y;
		stduint row_px_w = cols * font_w;
		stduint py_base  = row * font_h;

		witch::control::TextChrome::BlitPixelLine(
			buffer, window.width, 0, py_base,
			line_buf, row_px_w,
			row_px_w, font_h
		);
		if (sheet_parent) {
			Rectangle strip;
			strip.x = 0; strip.y = py_base;
			strip.width = row_px_w; strip.height = font_h;
			sheet_parent->Update(this, strip);
		}
	}

	// -------------------------------------------------------------------------
	// thisRollup
	// -------------------------------------------------------------------------
	void VideoConsole2::thisRollup(stduint /*height_px*/) {
		if (!font_engine) return;
		stduint font_w = font_engine->GetCellSize().x;
		stduint font_h = font_engine->GetCellSize().y;

		if (text_buf) {
			if (rows > 1) {
				for0(r, rows - 1) {
					BufferChar* dst = text_buf + r * cols;
					BufferChar* src = text_buf + (r + 1) * cols;
					for0(c, cols) dst[c] = src[c];
				}
			}
			BufferChar blank = MakeBlankCell(forecolor, backcolor);
			BufferChar* last = text_buf + (rows - 1) * cols;
			for0(c, cols) last[c] = blank;
			InvalidateLineBuffer();

			if (buffer) {
				stduint row_px_w = cols * font_w;
				for0(r, rows) {
					EnsureLineBuffer(r);
					stduint py_base = r * font_h;
					witch::control::TextChrome::BlitPixelLine(
						buffer, window.width, 0, py_base,
						line_buf, row_px_w,
						row_px_w, font_h
					);
					line_buf_valid = false;
				}
				Rectangle full;
				full.x = 0; full.y = 0;
				full.width = window.width; full.height = (stduint)(rows * font_h);
				if (sheet_parent) sheet_parent->Update(this, full);
			}
			else if (sheet_parent) {
				Rectangle full;
				full.x = 0; full.y = 0;
				full.width = window.width; full.height = (stduint)(rows * font_h);
				sheet_parent->Update(this, full);
			}
			else if (vci) {
				if (line_buf) {
					for0(r, rows) {
						EnsureLineBuffer(r);
						Rectangle strip;
						strip.x = 0; strip.y = r * font_h;
						strip.width = cols * font_w; strip.height = font_h;
						vci->DrawPoints(strip, line_buf);
						line_buf_valid = false;
					}
				}
				else {
					vci->RollUp(font_h, window);
					Rectangle clr;
					clr.x = 0; clr.y = (rows - 1) * font_h;
					clr.width = window.width; clr.height = font_h;
					clr.color = backcolor;
					vci->DrawRectangle(clr);
				}
			}
		}
		else {
			if (buffer) {
				VideoControlInterfaceMARGB8888 bvim(buffer, window.getSize());
				Rectangle zero_rect = window; zero_rect.x = zero_rect.y = 0;
				bvim.RollUp(font_h, zero_rect);
				if (sheet_parent) sheet_parent->Update(this, zero_rect);
			}
			else if (vci) {
				vci->RollUp(font_h, window);
			}
		}
	}

	void VideoConsole2::FeedLine() {
		if (!font_engine) return;
		stduint font_h = font_engine->GetCellSize().y;
		while (cursor.y >= rows) {
			cursor.y--;
			thisRollup(font_h);
		}
	}

	void VideoConsole2::curinc() {
		if (!cols || !rows) return;
		cursor.x++;
		if (cursor.x >= cols) { cursor.x = 0; cursor.y++; }
		if (cursor.y >= rows) FeedLine();
	}

	// -------------------------------------------------------------------------
	// _VideoConsole2Out -- inner output loop.
	// -------------------------------------------------------------------------
	void _VideoConsole2Out(VideoConsole2* crt_self, const char* str, stduint len) {
		if (!crt_self || !crt_self->font_engine) return;

		stduint font_w = crt_self->font_engine->GetCellSize().x;
		stduint font_h = crt_self->font_engine->GetCellSize().y;

		// Convert UTF-8 to UTF-32 (Unicode code points) using Unisym's CscUTF
		uint32* u32_str = nullptr;
		stduint u32_bytes = CscUTF(8, 32, (const pureptr_t)str, len, (pureptr_t*)&u32_str);

		stduint loop_len = len;
		const uint32* src_chars = nullptr;
		uint32* allocated_buf = nullptr;
		bool allocated_by_csc = false;

		if (u32_bytes != NONE && u32_str != nullptr) {
			loop_len = u32_bytes / sizeof(uint32);
			src_chars = u32_str;
			allocated_buf = u32_str;
			allocated_by_csc = true;
		} else {
			// Fallback: treat as raw Latin-1 bytes if conversion failed
			uint32* fallback_buf = new uint32[len];
			for0(k, len) {
				fallback_buf[k] = (uint32)(byte)str[k];
			}
			loop_len = len;
			src_chars = fallback_buf;
			allocated_buf = fallback_buf;
			allocated_by_csc = false;
		}

		for0(i, loop_len) {
			uint32 u_c = src_chars[i];
			char c = (u_c < 0x100) ? (char)u_c : '\0';

			bool consumed = false;

			if (crt_self->esc_state == 1) {
				if (c == '[') {
					crt_self->esc_state   = 2;
					crt_self->esc_nparams = 0;
					for (int k = 0; k < 8; k++) crt_self->esc_params[k] = 0;
					consumed = true;
				}
				else if (c == 'c') {
					// RIS - Reset to Initial State
					crt_self->backcolor = crt_self->window.color;
					crt_self->forecolor = ~crt_self->window.color;
					crt_self->forecolor.a = ~crt_self->forecolor.a;
					crt_self->Clear();
					crt_self->esc_state = 0;
					consumed = true;
				}
				else {
					crt_self->esc_state = 0;
				}
			}
			else if (crt_self->esc_state == 2) {
				consumed = true;
				if (c >= '0' && c <= '9') {
					if (crt_self->esc_nparams < 8)
						crt_self->esc_params[crt_self->esc_nparams] =
							crt_self->esc_params[crt_self->esc_nparams] * 10 + (c - '0');
				}
				else if (c == ';') {
					if (crt_self->esc_nparams < 7) crt_self->esc_nparams++;
				}
				else {
					crt_self->esc_nparams++;
					if (c == 'm')
						vc2ApplySGR(crt_self, crt_self->esc_params, crt_self->esc_nparams);
					crt_self->esc_state = 0;
				}
			}

			if (consumed) continue;

			if (u_c == 0) {
				if (crt_self->cursor.x > 0 && crt_self->refSheetParent()) {
					Rectangle rect;
					rect.x = (crt_self->cursor.x - 1) * font_w;
					rect.y = crt_self->cursor.y * font_h;
					rect.width = font_w; rect.height = font_h;
					crt_self->refSheetParent()->Update(crt_self, rect);
				}
			}
			else if (c == '\x1b') {
				crt_self->esc_state = 1;
			}
			else if (c == '\n') {
				if (crt_self->buffer && crt_self->line_buf) {
					stduint cur_row = (stduint)crt_self->cursor.y;
					crt_self->EnsureLineBuffer(cur_row);
					crt_self->BlitLineBuffer(cur_row);
				}
				crt_self->cursor.x = 0;
				crt_self->cursor.y++;
				crt_self->FeedLine();

				if (crt_self->refSheetParent() && crt_self->update_method == 2) {
					Rectangle rect;
					rect.x = 0;
					rect.y = (stduint)(crt_self->cursor.y - 1) * font_h;
					rect.width = crt_self->window.width; rect.height = font_h;
					crt_self->refSheetParent()->Update(crt_self, rect);
				}
			}
			else if (c == '\r') {
				crt_self->cursor.x = 0;
			}
			else if (c == '\a') {
				#if defined(_MCCA) && _MCCA==0x8632
				Buzzer::Buzz(true);
				for (volatile size_t ii = 0, _L = 0x1000; ii < _L; ii++)
					for (volatile size_t jj = 0, _L2 = 0x1000; jj < _L2; jj++);
				Buzzer::Buzz(false);
				#endif
			}
			else if (c == '\b') {
				if (crt_self->cursor.x > 0)
					crt_self->cursor.x--;
				else if (crt_self->cursor.y > 0) {
					crt_self->cursor.y--;
					crt_self->cursor.x = (stdsint)crt_self->cols - 1;
				}
			}
			else if ((byte)c == (byte)'\xFF' && _LIMIT - i > 1) {
				byte color = str[++i];
				if (color == (byte)0xFF) {
					crt_self->backcolor = crt_self->window.color;
					crt_self->forecolor = ~crt_self->window.color;
					crt_self->forecolor.a = ~crt_self->forecolor.a;
					continue;
				}
				Color col; col.a = 0xFF;
				col.b = (color & 0b00010000) ? 0xFA : 0;
				col.g = (color & 0b00100000) ? 0xFA : 0;
				col.r = (color & 0b01000000) ? 0xFA : 0;
				crt_self->forecolor = col;
				col.b = (color & 0b00000001) ? 0xFA : 0;
				col.g = (color & 0b00000010) ? 0xFA : 0;
				col.r = (color & 0b00000100) ? 0xFA : 0;
				crt_self->backcolor = col;
			}
			else if ((byte)c == (byte)'\xFE' && _LIMIT - i > 3) {
				Color col; col.a = 0xFF;
				col.b = str[++i]; col.g = str[++i]; col.r = str[++i];
				crt_self->backcolor = col;
			}
			else {
				stduint cx = (stduint)crt_self->cursor.x;
				stduint cy = (stduint)crt_self->cursor.y;

				BufferChar cell;
				cell.back_color   = crt_self->backcolor;
				cell.fore_color   = crt_self->forecolor;
				cell.attr         = 0;
				cell.unicode_char = (uint32)u_c;
				crt_self->Putchar(cx, cy, cell);

				if (crt_self->line_buf && (stdsint)cy == crt_self->line_buf_row) {
					stduint row_px_w = crt_self->cols * font_w;
					crt_self->font_engine->DrawChar(crt_self->line_buf, row_px_w,
						cx * font_w, 0, cell.unicode_char,
						cell.fore_color, cell.back_color);
					crt_self->line_buf_valid = true;
				}

				if (crt_self->buffer) {
					if (crt_self->line_buf) {
						crt_self->EnsureLineBuffer(cy);
						stduint row_px_w = crt_self->cols * font_w;
						stduint py_base  = cy * font_h;
						Color* glyph_base = crt_self->line_buf + cx * font_w;
						for0(scan_y, font_h) {
							Color* dst = crt_self->buffer
								+ (py_base + scan_y) * crt_self->window.width
								+ cx * font_w;
							Color* src = glyph_base + scan_y * row_px_w;
							for0(px, font_w) dst[px] = src[px];
						}
					}
					else {
						crt_self->font_engine->DrawChar(crt_self->buffer, crt_self->window.width,
							cx * font_w, cy * font_h, cell.unicode_char,
							cell.fore_color, cell.back_color);
					}
				}

				if (u_c >= 0x100) {
					// [Double-wide CJK character]
					if (crt_self->refSheetParent() && crt_self->update_method >= 1) {
						Rectangle rect;
						rect.x = cx * font_w; rect.y = cy * font_h;
						rect.width = font_w * 2; rect.height = font_h;
						crt_self->refSheetParent()->Update(crt_self, rect);
					}

					// Move to next cell to place the dummy placeholder
					crt_self->curinc();
					stduint dummy_cx = (stduint)crt_self->cursor.x;
					stduint dummy_cy = (stduint)crt_self->cursor.y;

					BufferChar dummy_cell;
					dummy_cell.back_color   = crt_self->backcolor;
					dummy_cell.fore_color   = crt_self->forecolor;
					dummy_cell.attr         = 0;
					dummy_cell.unicode_char = 0xFFFFFFFF; // Double-wide dummy placeholder
					crt_self->Putchar(dummy_cx, dummy_cy, dummy_cell);

					if (crt_self->line_buf && (stdsint)dummy_cy == crt_self->line_buf_row) {
						stduint row_px_w = crt_self->cols * font_w;
						crt_self->font_engine->DrawChar(crt_self->line_buf, row_px_w,
							dummy_cx * font_w, 0, dummy_cell.unicode_char,
							dummy_cell.fore_color, dummy_cell.back_color);
						crt_self->line_buf_valid = true;
					}

					if (crt_self->buffer) {
						if (crt_self->line_buf) {
							crt_self->EnsureLineBuffer(dummy_cy);
							stduint row_px_w = crt_self->cols * font_w;
							stduint py_base  = dummy_cy * font_h;
							Color* glyph_base = crt_self->line_buf + dummy_cx * font_w;
							for0(scan_y, font_h) {
								Color* dst = crt_self->buffer
									+ (py_base + scan_y) * crt_self->window.width
									+ dummy_cx * font_w;
								Color* src = glyph_base + scan_y * row_px_w;
								for0(px, font_w) dst[px] = src[px];
							}
						}
						else {
							crt_self->font_engine->DrawChar(crt_self->buffer, crt_self->window.width,
								dummy_cx * font_w, dummy_cy * font_h, dummy_cell.unicode_char,
								dummy_cell.fore_color, dummy_cell.back_color);
						}
					}
					// Finally, advance cursor past the dummy cell
					crt_self->curinc();
				} else {
					// [Regular single-wide character]
					if (crt_self->refSheetParent() && crt_self->update_method >= 1) {
						Rectangle rect;
						rect.x = cx * font_w; rect.y = cy * font_h;
						rect.width = font_w; rect.height = font_h;
						crt_self->refSheetParent()->Update(crt_self, rect);
					}
					crt_self->curinc();
				}
			}
		}
		if (allocated_buf) {
			if (allocated_by_csc) {
				memfree(allocated_buf);
			} else {
				delete[] allocated_buf;
			}
		}
	}

	// -------------------------------------------------------------------------
	// out / inn
	// -------------------------------------------------------------------------
	int VideoConsole2::out(const char* str, stduint len) {
		if (!font_engine) {
			plogerro("VideoConsole2: Attempted to print characters without a bound FontEngine!");
			return -1;
		}
		bool old_visible = cursor_visible;
		cursor_visible = false;
		doshow(nullptr);

		_VideoConsole2Out(this, str, len);

		cursor_visible = old_visible;
		doshow(nullptr);
		return 0;
	}
	int VideoConsole2::inn() {
		return _TEMP 0;
	}

} // namespace uni
