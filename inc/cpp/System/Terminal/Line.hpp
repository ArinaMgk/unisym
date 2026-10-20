// ASCII CPP TAB4 CRLF
// Docutitle: (System) Terminal Line
// Codifiers: @ArinaMgk
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

#ifndef _INCPP_System_Terminal_Line
#define _INCPP_System_Terminal_Line

#include "../../../c/consio.h"
#include "../../../c/multichar.h"
#include "Decoder.hpp"
#if defined(_WinNT) || defined(_Linux)
#include <stdio.h>
#endif

namespace uni {

	// Byte length of the code point ending at pos, 1 when the bytes do not form one.
	static inline stduint TerminalPrevLength(const char* buf, stduint pos) {
		stduint start = pos - 1;
		while (start && ((byte)buf[start] & 0xC0) == 0x80) start--;
		stduint len = ScanUTF8((byte*)(buf + start));
		if (len < 1 || len > 6 || start + len != pos) return 1;
		return len;
	}

	// Byte length of the code point starting at pos, 1 when the bytes do not form one.
	static inline stduint TerminalNextLength(const char* buf, stduint pos) {
		stduint len = ScanUTF8((byte*)(buf + pos));
		if (len < 1 || len > 6) return 1;
		return len;
	}

	// Columns the bytes from..to take on a terminal, wide East Asian runes counting twice.
	static inline stduint TerminalColumns(const char* buf, stduint from, stduint to) {
		stduint columns = 0;
		stduint i = from;
		while (i < to) {
			stduint width = ScanUTF8Width((byte*)(buf + i));
			if (width < 1 || width > 2) width = 1;
			columns += width;
			stduint len = ScanUTF8((byte*)(buf + i));
			if (len < 1 || len > 4) len = 1;
			i += len;
		}
		return columns;
	}

	// Edit one line on the terminal; history memory is optional, pass null to disable the arrow keys.
	class TerminalLine {
	public:
		// Rewrite the line in place when Tab is pressed, return true to repaint it.
		typedef bool (*CompleteHandler)(char* buffer, stduint capacity, stduint& length, stduint& position);
	protected:
		TerminalDecoder* decoder;
		char* buffer;
		stduint capacity;

		char* history;
		stduint history_count;
		stduint history_length;
		stduint history_position;

		CompleteHandler complete;

		stduint length;
		stduint position;
		stduint shown;

		byte pending[4];
		stduint pending_length;
		stduint pending_target;

		void LoadHistory();
	public:
		TerminalLine(TerminalDecoder* decoder, char* buffer, stduint capacity,
			char* history = nullptr, stduint history_count = 0, CompleteHandler complete = nullptr)
			: decoder(decoder), buffer(buffer), capacity(capacity),
			history(history), history_count(history_count),
			history_length(0), history_position(0),
			complete(complete), length(0), position(0), shown(0),
			pending_length(0), pending_target(0) {}
		// Block until Enter, return false when the stream is over.
		bool Edit(const char* prefix = nullptr);
		// Keep one finished line so the arrow keys can bring it back.
		void PushHistory(const char* text);
	};

	inline void TerminalLine::LoadHistory() {
		length = 0;
		position = 0;
		if (history_position) {
			const char* slot = history + ((history_length - history_position) % history_count) * capacity;
			while (slot[length] && length + 1 < capacity) {
				buffer[length] = slot[length];
				length++;
			}
			position = length;
		}
	}

	inline void TerminalLine::PushHistory(const char* text) {
		if (!history || !history_count || !text || !text[0]) return;
		char* slot = history + (history_length % history_count) * capacity;
		stduint i = 0;
		while (text[i] && i + 1 < capacity) { slot[i] = text[i]; i++; }
		slot[i] = 0;
		history_length++;
		history_position = 0;
	}

	inline bool TerminalLine::Edit(const char* prefix) {
		if (prefix) {
			stduint n = 0;
			while (prefix[n]) n++;
			Console.out(prefix, n);
		}
		length = 0;
		position = 0;
		shown = 0;
		pending_length = 0;
		pending_target = 0;
		stduint cursor = 0;
		char character = 0;
		_USB_IF_KEYENUM key = _UKEY_NONE;
		for (;;) {
			if (!decoder->Poll(key, character)) return false;

			stduint pos_before = position;
			stduint len_before = length;
			stduint erased_columns = 0;
			
			switch (key) {
			case _UKEY_NONE:
				{
					byte value = (byte)character;
					if (!pending_target) {
						if (value < 0x80) {
							if (length + 1 >= capacity) break;
							for (stduint i = length; i > position; i--) buffer[i] = buffer[i - 1];
							buffer[position] = character;
							length++;
							position++;
							break;
						}
						stduint need = ScanUTF8(&value);
						if (need < 2 || need > 4) break;
						pending[0] = value;
						pending_length = 1;
						pending_target = need;
						continue;
					}
					if ((value & 0xC0) != 0x80U) {
						pending_target = 0;
						pending_length = 0;
						continue;
					}
					pending[pending_length++] = value;
					if (pending_length < pending_target) continue;
					{
						stduint count = pending_target;
						pending_target = 0;
						pending_length = 0;
						if (length + count >= capacity) break;
						for (stduint i = length; i > position; i--) buffer[i + count - 1] = buffer[i - 1];
						for (stduint i = 0; i < count; i++) buffer[position + i] = (char)pending[i];
						length += count;
						position += count;
					}
				}
				break;
			case _UKEY_BACKSPACE:
				if (!position) break;
				{
					stduint bytes = TerminalPrevLength(buffer, position);

					erased_columns = TerminalColumns(buffer, position - bytes, position);

					for (stduint i = position - bytes; i + bytes < length; i++) buffer[i] = buffer[i + bytes];
					length -= bytes;
					position -= bytes;
				}
				break;
			case _UKEY_DELETE:
				if (position >= length) break;
				{
					stduint bytes = TerminalNextLength(buffer, position);
					for (stduint i = position; i + bytes < length; i++) buffer[i] = buffer[i + bytes];
					length -= bytes;
				}
				break;
			case _UKEY_LEFT:
				if (!position) break;
				position -= TerminalPrevLength(buffer, position);
				break;
			case _UKEY_RIGHT:
				if (position >= length) break;
				position += TerminalNextLength(buffer, position);
				break;
			case _UKEY_HOME:
				position = 0;
				break;
			case _UKEY_END:
				position = length;
				break;
			case _UKEY_TAB:
				if (!complete || complete(buffer, capacity, length, position)) break;
				continue;
			case _UKEY_UP:
				if (!history || !history_count || !history_length) break;
				if (history_position >= history_length) break;
				history_position++;
				LoadHistory();
				break;
			case _UKEY_DOWN:
				if (!history || !history_count || !history_position) break;
				history_position--;
				LoadHistory();
				break;
			case _UKEY_ENTER:
				buffer[length] = 0;
				Console.OutChar('\n');
				return true;
			default:
				continue;
			}

			// No repaint needed when the change happened at the tail of the line.
			if (key == _UKEY_NONE && pos_before == len_before && length > len_before) {
				for (stduint i = len_before; i < length; i++) Console.OutChar(buffer[i]);
				shown += TerminalColumns(buffer, len_before, length);
				cursor = shown;
				continue;
			}
			if (key == _UKEY_BACKSPACE && pos_before == len_before && erased_columns) {
				for (stduint i = 0; i < erased_columns; i++) Console.OutChar('\b');
				for (stduint i = 0; i < erased_columns; i++) Console.OutChar(' ');
				for (stduint i = 0; i < erased_columns; i++) Console.OutChar('\b');
				shown -= erased_columns;
				cursor = shown;
				continue;
			}

			// Repaint: step back to the input start, draw, blank the rest, then return to the cursor.
			stduint drawn_columns = TerminalColumns(buffer, 0, length);
			stduint pos_columns = TerminalColumns(buffer, 0, position);
			for (stduint i = 0; i < cursor; i++) Console.OutChar('\b');
			for (stduint i = 0; i < length; i++) Console.OutChar(buffer[i]);
			stduint blanks = shown > drawn_columns ? shown - drawn_columns : 0;
			for (stduint i = 0; i < blanks; i++) Console.OutChar(' ');
			shown = drawn_columns;
			stduint end_columns = drawn_columns + blanks;
			for (stduint i = end_columns; i > pos_columns; i--) Console.OutChar('\b');
			cursor = pos_columns;
			// A hosted stdout is line buffered, so a repaint without a newline stays invisible until flushed.
		#if defined(_WinNT) || defined(_Linux)
			fflush(stdout);
		#endif
		}
	}

}

#endif // _INCPP_System_Terminal_Line
