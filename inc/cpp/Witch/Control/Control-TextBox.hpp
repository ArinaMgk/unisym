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
		struct RichLine {
			String text = "";
			Vector<Color> colors;
			bool operator==(const RichLine& other) const {
				return text == other.text;
			}
		};
	public:
		String text = "";// compatibility field for current callers
	protected:
		struct DrawMetrics {
			stdsint origin_x = 4;
			stdsint origin_y = 4;
			stdsint line_height = 16;
			stdsint char_width = 8;
			stdsint margin_right = 8;
			stdsint margin_bottom = 16;
		};
		struct CachedPhysicalLine {
			Color* pixels = nullptr;
			stduint pixel_count = 0;
			stduint logical_row = (stduint)None;
			stduint segment_row = 0;
			stduint scroll_x_value = 0;
			stduint visible_cols = 0;
			bool wrap_mode = false;
			bool valid = false;
		};
		Vector<RichLine> lines;
		BitmapFontEngine default_font_engine = BitmapFontEngine(1);
		const FontEngine* font_engine = nullptr;
		CachedPhysicalLine* line_caches = nullptr;
		stduint line_cache_count = 0;
		stdsint cursor_row = 0;
		stdsint cursor_col = 0;
		stdsint scroll_y = 0;
		stdsint scroll_x = 0;
		bool word_wrap = true;
		Color current_input_color = Color::Black;
		bool cursor_visible = false;
	public:
		TextBox() : SheetTrait(), default_font_engine(1), font_engine(&default_font_engine) {}
		~TextBox() {
			ReleaseLineCaches();
		}
	protected:
		void ReleaseLineCaches() {
			if (line_caches) {
				for (stduint i = 0; i < line_cache_count; i++) {
					if (line_caches[i].pixels) {
						delete[] line_caches[i].pixels;
						line_caches[i].pixels = nullptr;
					}
				}
				delete[] line_caches;
				line_caches = nullptr;
			}
			line_cache_count = 0;
		}

		void InvalidateAllLineCaches() {
			for (stduint i = 0; i < line_cache_count; i++) {
				line_caches[i].valid = false;
				line_caches[i].logical_row = (stduint)None;
			}
		}

		void EnsureLineCacheSlots(stduint count, const DrawMetrics& dm) {
			stduint pixels_per_line = GetVisibleTextCols() * dm.char_width * dm.line_height;
			if (!pixels_per_line) pixels_per_line = 1;

			if (line_cache_count != count) {
				ReleaseLineCaches();
				line_cache_count = count;
				line_caches = new CachedPhysicalLine[line_cache_count];
			}

			for (stduint i = 0; i < line_cache_count; i++) {
				if (line_caches[i].pixel_count != pixels_per_line) {
					if (line_caches[i].pixels) delete[] line_caches[i].pixels;
					line_caches[i].pixels = new Color[pixels_per_line];
					line_caches[i].pixel_count = pixels_per_line;
					line_caches[i].valid = false;
					line_caches[i].logical_row = (stduint)None;
				}
			}
		}

		DrawMetrics GetDrawMetrics() {
			DrawMetrics dm;
			if (font_engine) {
				Size2 cell = font_engine->GetCellSize();
				dm.char_width = cell.x;
				dm.line_height = cell.y;
				dm.margin_right = dm.origin_x + dm.char_width;
				dm.margin_bottom = dm.line_height;
			}
			return dm;
		}

		void EnsureLineModel() {
			if (lines.Count() == 0) {
				RichLine line;
				lines.Append(line);
			}
		}

		void NormalizeLineColors(RichLine& line) {
			stduint len = line.text.getByteCount();
			while (line.colors.Count() > len) {
				line.colors.Remove(line.colors.Count() - 1);
			}
			while (line.colors.Count() < len) {
				line.colors.Append(current_input_color);
			}
		}

		void NormalizeAllLineColors() {
			for (stduint i = 0; i < lines.Count(); i++) {
				NormalizeLineColors(lines[i]);
			}
		}

		void ImportTextToLines() {
			lines.Clear();

			RichLine crt_line;
			rostr pstr = text.reference();
			if (!pstr || !*pstr) {
				lines.Append(crt_line);
				return;
			}

			while (*pstr) {
				char ch = *pstr++;
				if (ch == '\r' || ch == '\n') {
					lines.Append(crt_line);
					crt_line.text.Clear();
					if ((ch == '\r' && *pstr == '\n') || (ch == '\n' && *pstr == '\r')) {
						pstr++;
					}
				}
				else {
					crt_line.text << ch;
					crt_line.colors.Append(current_input_color);
				}
			}

			lines.Append(crt_line);
			EnsureLineModel();
			NormalizeAllLineColors();
			InvalidateAllLineCaches();
			SetCursorToDocumentEnd();
		}

		void ExportLinesToText() {
			String merged = "";
			for (stduint i = 0; i < lines.Count(); i++) {
				const char* p = lines[i].text.reference();
				if (p) while (*p) merged << *p++;
				if (i + 1 < lines.Count()) merged << '\n';
			}
			text = merged;
		}

		void SyncLinesFromText() {
			String merged = "";
			for (stduint i = 0; i < lines.Count(); i++) {
				const char* p = lines[i].text.reference();
				if (p) while (*p) merged << *p++;
				if (i + 1 < lines.Count()) merged << '\n';
			}
			if (lines.Count() == 0 || !(merged == text)) {
				ImportTextToLines();
			}
			EnsureLineModel();
			NormalizeAllLineColors();
			ClampCursor();
		}

		stduint GetLineLength(stduint row) {
			if (row >= lines.Count()) return 0;
			return lines[row].text.getByteCount();
		}

		void ClampCursor() {
			EnsureLineModel();
			if (cursor_row < 0) cursor_row = 0;
			if ((stduint)cursor_row >= lines.Count()) cursor_row = lines.Count() ? (stdsint)lines.Count() - 1 : 0;
			stduint line_len = GetLineLength((stduint)cursor_row);
			if (cursor_col < 0) cursor_col = 0;
			if ((stduint)cursor_col > line_len) cursor_col = (stdsint)line_len;
		}

		void ClampScroll() {
			EnsureLineModel();
			if (scroll_y < 0) scroll_y = 0;
			stduint total_rows = GetTotalPhysicalRows();
			if ((stduint)scroll_y >= total_rows) scroll_y = total_rows ? (stdsint)total_rows - 1 : 0;
			if (scroll_x < 0) scroll_x = 0;
		}

		stduint GetVisibleTextCols() {
			DrawMetrics dm = GetDrawMetrics();
			stdsint visible = ((stdsint)sheet_area.width - dm.margin_right) / dm.char_width;
			if (visible < 1) visible = 1;
			return (stduint)visible;
		}

		stduint GetVisibleTextRows() {
			DrawMetrics dm = GetDrawMetrics();
			stdsint visible = ((stdsint)sheet_area.height - dm.origin_y) / dm.line_height;
			if (visible < 1) visible = 1;
			return (stduint)visible;
		}

		stduint GetWrappedSegmentCount(stduint row) {
			stduint cols = GetVisibleTextCols();
			if (!cols) cols = 1;
			stduint len = GetLineLength(row);
			if (!word_wrap) return 1;
			if (len == 0) return 1;
			return (len + cols - 1) / cols;
		}

		stduint GetTotalPhysicalRows() {
			EnsureLineModel();
			if (!word_wrap) return lines.Count();
			stduint total = 0;
			for (stduint row = 0; row < lines.Count(); row++) {
				total += GetWrappedSegmentCount(row);
			}
			return total ? total : 1;
		}

		void GetPhysicalRowInfo(stduint phys_row, stdsint& logical_row, stdsint& segment_row) {
			EnsureLineModel();
			if (!word_wrap) {
				logical_row = (stdsint)phys_row;
				segment_row = 0;
				if ((stduint)logical_row >= lines.Count()) logical_row = lines.Count() ? (stdsint)lines.Count() - 1 : 0;
				return;
			}

			stduint remain = phys_row;
			stduint row = 0;
			for (; row < lines.Count(); row++) {
				stduint segs = GetWrappedSegmentCount(row);
				if (remain < segs) break;
				remain -= segs;
			}
			if (row >= lines.Count()) {
				row = lines.Count() ? lines.Count() - 1 : 0;
				remain = GetWrappedSegmentCount(row) ? GetWrappedSegmentCount(row) - 1 : 0;
			}
			logical_row = (stdsint)row;
			segment_row = (stdsint)remain;
		}

		stduint GetSegmentBeginCol(stduint logical_row, stduint segment_row) {
			if (!word_wrap) return 0;
			return segment_row * GetVisibleTextCols();
		}

		stduint GetSegmentEndCol(stduint logical_row, stduint segment_row) {
			stduint line_len = GetLineLength(logical_row);
			if (!word_wrap) return line_len;
			stduint cols = GetVisibleTextCols();
			stduint begin = GetSegmentBeginCol(logical_row, segment_row);
			return minof(line_len, begin + cols);
		}

		void GetCursorPhysicalPosition(stdsint& phys_row, stdsint& phys_col) {
			ClampCursor();
			if (!word_wrap) {
				phys_row = cursor_row;
				phys_col = cursor_col;
				return;
			}

			stduint cols = GetVisibleTextCols();
			if (!cols) cols = 1;
			phys_row = 0;
			for (stduint row = 0; row < (stduint)cursor_row; row++) {
				phys_row += (stdsint)GetWrappedSegmentCount(row);
			}

			stduint len = GetLineLength((stduint)cursor_row);
			if ((stduint)cursor_col >= len && len > 0 && ((stduint)cursor_col % cols) == 0) {
				phys_row += (cursor_col / (stdsint)cols) - 1;
				phys_col = cols;
			}
			else {
				phys_row += cursor_col / (stdsint)cols;
				phys_col = cursor_col % (stdsint)cols;
			}
		}

		void SetCursorFromPhysicalPosition(stdsint phys_row, stdsint phys_col) {
			EnsureLineModel();
			if (phys_row < 0) phys_row = 0;
			if (phys_col < 0) phys_col = 0;

			if (!word_wrap) {
				cursor_row = phys_row;
				cursor_col = phys_col;
				ClampCursor();
				return;
			}

			stduint cols = GetVisibleTextCols();
			if (!cols) cols = 1;
			stduint remain = (stduint)phys_row;
			stduint row = 0;
			for (; row < lines.Count(); row++) {
				stduint segs = GetWrappedSegmentCount(row);
				if (remain < segs) break;
				remain -= segs;
			}
			if (row >= lines.Count()) {
				row = lines.Count() ? lines.Count() - 1 : 0;
				remain = GetWrappedSegmentCount(row) - 1;
			}

			stduint line_len = GetLineLength(row);
			stduint base_col = remain * cols;
			stduint target_col = base_col + (stduint)phys_col;
			if (target_col > line_len) target_col = line_len;

			cursor_row = (stdsint)row;
			cursor_col = (stdsint)target_col;
			ClampCursor();
		}

		void EnsureCursorVisible() {
			ClampCursor();
			ClampScroll();

			stduint visible_rows = GetVisibleTextRows();
			if (word_wrap) {
				stdsint phys_row = 0;
				stdsint phys_col = 0;
				GetCursorPhysicalPosition(phys_row, phys_col);

				if (phys_row < scroll_y) scroll_y = phys_row;
				else if ((stduint)phys_row >= (stduint)scroll_y + visible_rows) {
					scroll_y = phys_row - (stdsint)visible_rows + 1;
				}
				scroll_x = 0;
			}
			else {
				stduint visible_cols = GetVisibleTextCols();
				if (cursor_row < scroll_y) scroll_y = cursor_row;
				else if ((stduint)cursor_row >= (stduint)scroll_y + visible_rows) {
					scroll_y = cursor_row - (stdsint)visible_rows + 1;
				}

				if (cursor_col < scroll_x) scroll_x = cursor_col;
				else if ((stduint)cursor_col >= (stduint)scroll_x + visible_cols) {
					scroll_x = cursor_col - (stdsint)visible_cols + 1;
				}
			}

			ClampScroll();
		}

		void SetCursorToDocumentEnd() {
			EnsureLineModel();
			cursor_row = lines.Count() ? (stdsint)lines.Count() - 1 : 0;
			cursor_col = (stdsint)GetLineLength((stduint)cursor_row);
			EnsureCursorVisible();
		}

		String MakeInsertedString(const String& src, stduint idx, char ch) {
			String dst = "";
			const char* p = src.reference();
			stduint pos = 0;
			while (p && *p) {
				if (pos == idx) dst << ch;
				dst << *p++;
				pos++;
			}
			if (idx >= pos) dst << ch;
			return dst;
		}

		String MakeRemovedString(const String& src, stduint idx) {
			String dst = "";
			const char* p = src.reference();
			stduint pos = 0;
			while (p && *p) {
				char ch = *p++;
				if (pos != idx) dst << ch;
				pos++;
			}
			return dst;
		}

		String MakeSliceString(const String& src, stduint begin, stduint endo) {
			String dst = "";
			const char* p = src.reference();
			stduint pos = 0;
			while (p && *p) {
				char ch = *p++;
				if (pos >= begin && pos < endo) dst << ch;
				pos++;
			}
			return dst;
		}

		Vector<Color> MakeInsertedColors(Vector<Color>& src, stduint idx, Color color) {
			Vector<Color> dst;
			stduint len = src.Count();
			for (stduint i = 0; i < len; i++) {
				if (i == idx) dst.Append(color);
				dst.Append(src[i]);
			}
			if (idx >= len) dst.Append(color);
			return dst;
		}

		Vector<Color> MakeRemovedColors(Vector<Color>& src, stduint idx) {
			Vector<Color> dst;
			for (stduint i = 0; i < src.Count(); i++) {
				if (i != idx) dst.Append(src[i]);
			}
			return dst;
		}

		Vector<Color> MakeSliceColors(Vector<Color>& src, stduint begin, stduint endo) {
			Vector<Color> dst;
			for (stduint i = begin; i < endo && i < src.Count(); i++) {
				dst.Append(src[i]);
			}
			return dst;
		}

		void InsertLineAt(stduint idx, const RichLine& line) {
			if (idx >= lines.Count()) {
				lines.Append(line);
				return;
			}
			lines.Append(line);
			for (stdsint i = (stdsint)lines.Count() - 1; i > (stdsint)idx; i--) {
				lines.Exchange((stduint)i, (stduint)i - 1);
			}
		}

		void InsertCharAtCursor(char ch) {
			EnsureLineModel();
			ClampCursor();
			if (ch == '\n') {
				SplitLineAtCursor();
				return;
			}
			RichLine& line = lines[(stduint)cursor_row];
			line.text = MakeInsertedString(line.text, (stduint)cursor_col, ch);
			line.colors = MakeInsertedColors(line.colors, (stduint)cursor_col, current_input_color);
			cursor_col++;
			NormalizeLineColors(line);
			InvalidateAllLineCaches();
			ExportLinesToText();
			EnsureCursorVisible();
		}

		void SplitLineAtCursor() {
			EnsureLineModel();
			ClampCursor();
			RichLine& line = lines[(stduint)cursor_row];
			stduint old_len = line.text.getByteCount();
			String left = MakeSliceString(line.text, 0, (stduint)cursor_col);
			String right = MakeSliceString(line.text, (stduint)cursor_col, old_len);
			Vector<Color> left_colors = MakeSliceColors(line.colors, 0, (stduint)cursor_col);
			Vector<Color> right_colors = MakeSliceColors(line.colors, (stduint)cursor_col, old_len);
			line.text = left;
			line.colors = left_colors;
			RichLine new_line;
			new_line.text = right;
			new_line.colors = right_colors;
			NormalizeLineColors(line);
			NormalizeLineColors(new_line);
			InsertLineAt((stduint)cursor_row + 1, new_line);
			cursor_row++;
			cursor_col = 0;
			InvalidateAllLineCaches();
			ExportLinesToText();
			EnsureCursorVisible();
		}

		bool BackspaceAtCursor() {
			EnsureLineModel();
			ClampCursor();
			if (cursor_col > 0) {
				RichLine& line = lines[(stduint)cursor_row];
				line.text = MakeRemovedString(line.text, (stduint)cursor_col - 1);
				line.colors = MakeRemovedColors(line.colors, (stduint)cursor_col - 1);
				cursor_col--;
				NormalizeLineColors(line);
			}
			else if (cursor_row > 0) {
				stduint prev_len = GetLineLength((stduint)cursor_row - 1);
				lines[(stduint)cursor_row - 1].text += lines[(stduint)cursor_row].text;
				for (stduint i = 0; i < lines[(stduint)cursor_row].colors.Count(); i++) {
					lines[(stduint)cursor_row - 1].colors.Append(lines[(stduint)cursor_row].colors[i]);
				}
				NormalizeLineColors(lines[(stduint)cursor_row - 1]);
				lines.Remove((stduint)cursor_row);
				cursor_row--;
				cursor_col = (stdsint)prev_len;
			}
			else return false;
			InvalidateAllLineCaches();
			ExportLinesToText();
			EnsureCursorVisible();
			return true;
		}

		void MoveCursorLeft() {
			EnsureLineModel();
			ClampCursor();
			if (cursor_col > 0) {
				cursor_col--;
			}
			else if (cursor_row > 0) {
				cursor_row--;
				cursor_col = (stdsint)GetLineLength((stduint)cursor_row);
			}
			EnsureCursorVisible();
		}

		void MoveCursorRight() {
			EnsureLineModel();
			ClampCursor();
			stduint line_len = GetLineLength((stduint)cursor_row);
			if ((stduint)cursor_col < line_len) {
				cursor_col++;
			}
			else if ((stduint)cursor_row + 1 < lines.Count()) {
				cursor_row++;
				cursor_col = 0;
			}
			EnsureCursorVisible();
		}

		void MoveCursorUp() {
			EnsureLineModel();
			ClampCursor();
			if (word_wrap) {
				stdsint phys_row = 0;
				stdsint phys_col = 0;
				GetCursorPhysicalPosition(phys_row, phys_col);
				if (phys_row > 0) SetCursorFromPhysicalPosition(phys_row - 1, phys_col);
			}
			else if (cursor_row > 0) {
				cursor_row--;
				ClampCursor();
			}
			EnsureCursorVisible();
		}

		void MoveCursorDown() {
			EnsureLineModel();
			ClampCursor();
			if (word_wrap) {
				stdsint phys_row = 0;
				stdsint phys_col = 0;
				GetCursorPhysicalPosition(phys_row, phys_col);
				SetCursorFromPhysicalPosition(phys_row + 1, phys_col);
			}
			else if ((stduint)cursor_row + 1 < lines.Count()) {
				cursor_row++;
				ClampCursor();
			}
			EnsureCursorVisible();
		}

		void PlaceCursorFromPoint(Point rel_p) {
			EnsureLineModel();
			DrawMetrics dm = GetDrawMetrics();

			stdsint raw_row = (rel_p.y - dm.origin_y) / dm.line_height;
			stdsint raw_col = (rel_p.x - dm.origin_x) / dm.char_width;
			if (rel_p.y < dm.origin_y) raw_row = 0;
			if (rel_p.x < dm.origin_x) raw_col = 0;
			if (raw_row < 0) raw_row = 0;
			if (raw_col < 0) raw_col = 0;

			if (word_wrap) {
				stdsint phys_row = raw_row + scroll_y;
				stdsint logical_row = 0;
				stdsint segment_row = 0;
				GetPhysicalRowInfo((stduint)maxof(phys_row, 0), logical_row, segment_row);

				cursor_row = logical_row;
				stduint segment_begin = GetSegmentBeginCol((stduint)logical_row, (stduint)segment_row);
				stduint segment_end = GetSegmentEndCol((stduint)logical_row, (stduint)segment_row);
				cursor_col = (stdsint)(segment_begin + (stduint)maxof(raw_col, 0));
				if ((stduint)cursor_col > segment_end) cursor_col = (stdsint)segment_end;
				ClampCursor();
			}
			else {
				cursor_row = raw_row + scroll_y;
				if ((stduint)cursor_row >= lines.Count()) cursor_row = lines.Count() ? (stdsint)lines.Count() - 1 : 0;

				stduint line_len = GetLineLength((stduint)cursor_row);
				cursor_col = raw_col + scroll_x;
				if ((stduint)cursor_col > line_len) cursor_col = (stdsint)line_len;
			}
			EnsureCursorVisible();
		}

		void DrawFrame(VideoControlInterfaceMARGB8888& vcim) {
			vcim.DrawRectangle(Rectangle(Point(0, 0), Size2(sheet_area.width, sheet_area.height), Color::White));
			vcim.DrawRectangle(Rectangle(Point(0, 0), Size2(3, sheet_area.height), 0xFF7F7F7F));
			vcim.DrawRectangle(Rectangle(Point(3, 0), Size2(sheet_area.width - 6, 3), 0xFF7F7F7F));
			vcim.DrawRectangle(Rectangle(Point(sheet_area.width - 3, 0), Size2(3, sheet_area.height), 0xFFDDDFE1));
			vcim.DrawRectangle(Rectangle(Point(3, sheet_area.height - 3), Size2(sheet_area.width - 6, 3), 0xFFDDDFE1));
		}

		bool CanDrawNextLine(stdsint draw_y, const DrawMetrics& dm) {
			return draw_y + dm.margin_bottom <= (stdsint)sheet_area.height;
		}

		void DrawGlyph(Point at, char ch, Color col) {
			if (!sheet_buffer || !font_engine) return;
			Size2 cell = font_engine->GetCellSize();
			for (stduint gy = 0; gy < (stduint)cell.y; gy++) {
				for (stduint gx = 0; gx < (stduint)cell.x; gx++) {
					Point p(at.x + (stdsint)gx, at.y + (stdsint)gy);
					if (p.x < 0 || p.y < 0 || p.x >= (stdsint)sheet_area.width || p.y >= (stdsint)sheet_area.height) continue;
					sheet_buffer[(stduint)p.y * sheet_area.width + (stduint)p.x] =
						font_engine->GetPixel((uint32)(byte)ch, gx, gy, col, Color::White);
				}
			}
		}

		void DrawGlyphToPixels(Color* pixels, stduint pitch_pixels, Point at, char ch, Color col) {
			if (!pixels || !font_engine) return;
			Size2 cell = font_engine->GetCellSize();
			for (stduint gy = 0; gy < (stduint)cell.y; gy++) {
				for (stduint gx = 0; gx < (stduint)cell.x; gx++) {
					Point p(at.x + (stdsint)gx, at.y + (stdsint)gy);
					if (p.x < 0 || p.y < 0) continue;
					pixels[(stduint)p.y * pitch_pixels + (stduint)p.x] =
						font_engine->GetPixel((uint32)(byte)ch, gx, gy, col, Color::White);
				}
			}
		}

		void RenderCachedLine(CachedPhysicalLine& cache, stduint logical_row, stduint segment_row, const DrawMetrics& dm) {
			stduint visible_cols = GetVisibleTextCols();
			stduint pitch_pixels = visible_cols * dm.char_width;
			for (stduint i = 0; i < cache.pixel_count; i++) cache.pixels[i] = Color::White;

			stduint start_col = word_wrap ? (segment_row * visible_cols) : (stduint)scroll_x;
			stduint line_len = GetLineLength(logical_row);
			stduint end_col = minof(line_len, start_col + visible_cols);
			stduint draw_x = 0;
			for (stduint char_idx = start_col; char_idx < end_col; char_idx++) {
				char ch = lines[logical_row].text[(stdsint)char_idx];
				if (ch >= 32 && ch <= 126) {
					Color col = (char_idx < lines[logical_row].colors.Count()) ? lines[logical_row].colors[char_idx] : Color::Black;
					DrawGlyphToPixels(cache.pixels, pitch_pixels, Point((stdsint)draw_x, 0), ch, col);
				}
				draw_x += dm.char_width;
			}

			cache.logical_row = logical_row;
			cache.segment_row = segment_row;
			cache.scroll_x_value = (stduint)scroll_x;
			cache.visible_cols = visible_cols;
			cache.wrap_mode = word_wrap;
			cache.valid = true;
		}

		void BlitCachedLine(const CachedPhysicalLine& cache, stduint draw_y, const DrawMetrics& dm) {
			if (!sheet_buffer || !cache.pixels) return;
			stduint visible_cols = GetVisibleTextCols();
			stduint pitch_pixels = visible_cols * dm.char_width;
			for (stduint sy = 0; sy < (stduint)dm.line_height; sy++) {
				Color* dst = sheet_buffer + (draw_y + sy) * sheet_area.width + dm.origin_x;
				Color* src = cache.pixels + sy * pitch_pixels;
				for (stduint sx = 0; sx < pitch_pixels; sx++) dst[sx] = src[sx];
			}
		}

		void DrawVisibleText(const DrawMetrics& dm, stdsint& cursor_draw_x, stdsint& cursor_draw_y, bool& cursor_draw_known) {
			stdsint cur_draw_x = dm.origin_x;
			stdsint cur_draw_y = dm.origin_y;
			bool stop_drawing = false;
			stduint visible_rows = GetVisibleTextRows();
			stduint visible_cols = GetVisibleTextCols();
			EnsureLineCacheSlots(visible_rows, dm);

			cursor_draw_x = cur_draw_x;
			cursor_draw_y = cur_draw_y;
			cursor_draw_known = false;

			if (!word_wrap) {
				stduint line_begin = (scroll_y < 0) ? 0 : (stduint)scroll_y;
				stduint line_endo = minof((stduint)lines.Count(), line_begin + visible_rows);
				stduint cache_idx = 0;

				for (stduint line_idx = line_begin; line_idx < line_endo; line_idx++) {
					if (!cursor_draw_known && line_idx == (stduint)cursor_row && cursor_col == 0) {
						cursor_draw_x = cur_draw_x;
						cursor_draw_y = cur_draw_y;
						cursor_draw_known = true;
					}

					rostr pstr = lines[line_idx].text.reference();
					stduint char_idx = 0;
					stduint draw_count = 0;
					CachedPhysicalLine& cache = line_caches[cache_idx];
					if (!cache.valid || cache.logical_row != line_idx || cache.segment_row != 0 ||
						cache.scroll_x_value != (stduint)scroll_x || cache.visible_cols != visible_cols ||
						cache.wrap_mode != word_wrap) {
						RenderCachedLine(cache, line_idx, 0, dm);
					}
					if (pstr) while (*pstr++) if (char_idx++ >= 0) {}
					char_idx = GetLineLength(line_idx);
					BlitCachedLine(cache, (stduint)cur_draw_y, dm);
					draw_count = minof(visible_cols, (char_idx > (stduint)scroll_x) ? (char_idx - (stduint)scroll_x) : 0);
					if (!cursor_draw_known && line_idx == (stduint)cursor_row &&
						(stduint)cursor_col >= (stduint)scroll_x &&
						(stduint)cursor_col <= (stduint)scroll_x + draw_count) {
						cursor_draw_x = cur_draw_x + (cursor_col - scroll_x) * dm.char_width;
						cursor_draw_y = cur_draw_y;
						cursor_draw_known = true;
					}

					if (!cursor_draw_known && line_idx == (stduint)cursor_row && char_idx == (stduint)cursor_col) {
						stdsint rel_col = cursor_col - scroll_x;
						if (rel_col < 0) rel_col = 0;
						cursor_draw_x = dm.origin_x + rel_col * dm.char_width;
						cursor_draw_y = cur_draw_y;
						cursor_draw_known = true;
					}

					if (line_idx + 1 < line_endo) {
						cur_draw_x = dm.origin_x;
						cur_draw_y += dm.line_height;
						if (!CanDrawNextLine(cur_draw_y, dm)) {
							stop_drawing = true;
							break;
						}
					}
					cache_idx++;
				}
			}
			else {
				stduint phys_row = 0;
				stduint cache_idx = 0;
				for (stduint line_idx = 0; line_idx < lines.Count(); line_idx++) {
					stduint line_len = GetLineLength(line_idx);
					stduint seg_count = GetWrappedSegmentCount(line_idx);
					for (stduint seg = 0; seg < seg_count; seg++, phys_row++) {
						if ((stdsint)phys_row < scroll_y) continue;
						if ((stduint)((stdsint)phys_row - scroll_y) >= visible_rows) {
							stop_drawing = true;
							break;
						}

						cur_draw_x = dm.origin_x;
						cur_draw_y = dm.origin_y + ((stdsint)phys_row - scroll_y) * dm.line_height;
						if (!cursor_draw_known && line_idx == (stduint)cursor_row && cursor_col == 0 && seg == 0) {
							cursor_draw_x = cur_draw_x;
							cursor_draw_y = cur_draw_y;
							cursor_draw_known = true;
						}

						stduint start_col = seg * visible_cols;
						stduint end_col = minof(line_len, start_col + visible_cols);
						CachedPhysicalLine& cache = line_caches[cache_idx];
						if (!cache.valid || cache.logical_row != line_idx || cache.segment_row != seg ||
							cache.visible_cols != visible_cols || cache.wrap_mode != word_wrap) {
							RenderCachedLine(cache, line_idx, seg, dm);
						}
						BlitCachedLine(cache, (stduint)cur_draw_y, dm);
						if (!cursor_draw_known && line_idx == (stduint)cursor_row &&
							(stduint)cursor_col >= start_col && (stduint)cursor_col <= end_col) {
							cursor_draw_x = cur_draw_x + ((cursor_col - (stdsint)start_col) * dm.char_width);
							cursor_draw_y = cur_draw_y;
							cursor_draw_known = true;
						}
						cur_draw_x += (end_col - start_col) * dm.char_width;

						if (!cursor_draw_known && line_idx == (stduint)cursor_row) {
							stduint seg_end_cursor = minof(line_len, start_col + visible_cols);
							bool at_segment_end = ((stduint)cursor_col == seg_end_cursor);
							bool last_segment_end = (seg + 1 == seg_count) && ((stduint)cursor_col == line_len);
							if (at_segment_end || last_segment_end) {
								cursor_draw_x = cur_draw_x;
								cursor_draw_y = cur_draw_y;
								cursor_draw_known = true;
							}
						}
						cache_idx++;
					}
					if (stop_drawing) break;
				}
			}

			if (!cursor_draw_known) {
				cursor_draw_x = cur_draw_x;
				cursor_draw_y = cur_draw_y;
			}
			(void)stop_drawing;
		}

		void DrawCursor(VideoControlInterfaceMARGB8888& vcim, stdsint cursor_draw_x, stdsint cursor_draw_y) {
			if (cursor_draw_x < 4) cursor_draw_x = 4;
			if (cursor_visible && cursor_draw_y + 16 <= (stdsint)sheet_area.height) {
				vcim.DrawRectangle(Rectangle(Point(cursor_draw_x, cursor_draw_y), Size2(2, 16), Color::Black));
			}
		}

	public:
		virtual Color getPoint(Point p) override {
			_TEMP (void)p; return Color::Black;
		}

		// Update the buffer
		virtual void doshow(void*) override {
			if (!sheet_buffer) sheet_buffer = (Color*)calloc(sheet_area.getArea(), sizeof(Color));
			SyncLinesFromText();
			EnsureCursorVisible();
			
			VideoControlInterfaceMARGB8888 vcim(sheet_buffer, sheet_area.getSize());
			DrawMetrics dm = GetDrawMetrics();
			DrawFrame(vcim);

			stdsint cursor_draw_x = dm.origin_x;
			stdsint cursor_draw_y = dm.origin_y;
			bool cursor_draw_known = false;
			DrawVisibleText(dm, cursor_draw_x, cursor_draw_y, cursor_draw_known);
			DrawCursor(vcim, cursor_draw_x, cursor_draw_y);

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
					SyncLinesFromText();
					PlaceCursorFromPoint(rel_p);
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
				SyncLinesFromText();
				para_list args;
				para_ento(args, rel_p);
				keyboard_event_t* pkey = para_next(args, keyboard_event_t*);
				para_endo(args);
					if (pkey && (pkey->method == keyboard_event_t::method_t::keydown || pkey->method == keyboard_event_t::method_t::keyrepeat)) {
						bool shift = pkey->mod.l_shift || pkey->mod.r_shift;
						char ch = (shift ? key_map_shift : key_map)[pkey->keycode];

					if (pkey->keycode == 0x50) {
						MoveCursorLeft();
						doshow(nullptr);
						if (sheet_parent) {
							sheet_parent->AddDirty(sheet_area);
						}
					}
					else if (pkey->keycode == 0x4F) {
						MoveCursorRight();
						doshow(nullptr);
						if (sheet_parent) {
							sheet_parent->AddDirty(sheet_area);
						}
					}
					else if (pkey->keycode == 0x52) {
						MoveCursorUp();
						doshow(nullptr);
						if (sheet_parent) {
							sheet_parent->AddDirty(sheet_area);
						}
					}
					else if (pkey->keycode == 0x51) {
						MoveCursorDown();
						doshow(nullptr);
						if (sheet_parent) {
							sheet_parent->AddDirty(sheet_area);
						}
					}
					else if (ch == '\b') {
						if (BackspaceAtCursor()) {
							doshow(nullptr);
							if (sheet_parent) {
								sheet_parent->AddDirty(sheet_area);
							}
						}
					}
					else if ((ch >= 32 && ch <= 126) || ch == '\n' || ch == '\r') {
						if (ch == '\r') ch = '\n';
						InsertCharAtCursor(ch);
						doshow(nullptr);
						if (sheet_parent) {
							sheet_parent->AddDirty(sheet_area);
						}
					}
				}
			}
		}

		void setFontEngine(const FontEngine* fe) {
			font_engine = fe ? fe : &default_font_engine;
			InvalidateAllLineCaches();
		}

		void Start() {
			if (sheet_parent) sheet_parent->RegisterTimer(this);
		}
	};

}
#endif
