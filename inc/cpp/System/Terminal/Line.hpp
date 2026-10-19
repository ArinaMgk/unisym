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
#include "Decoder.hpp"
#if defined(_WinNT) || defined(_Linux)
#include <stdio.h>
#endif

namespace uni {

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
		void LoadHistory();
	public:
		TerminalLine(TerminalDecoder* decoder, char* buffer, stduint capacity,
			char* history = nullptr, stduint history_count = 0, CompleteHandler complete = nullptr)
			: decoder(decoder), buffer(buffer), capacity(capacity),
			history(history), history_count(history_count),
			history_length(0), history_position(0),
			complete(complete), length(0), position(0), shown(0) {}
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
		stduint cursor = 0;
		char character = 0;
		_USB_IF_KEYENUM key = _UKEY_NONE;
		for (;;) {
			if (!decoder->Poll(key, character)) return false;
			switch (key) {
			case _UKEY_NONE:
				if (length + 1 >= capacity) break;
				for (stduint i = length; i > position; i--) buffer[i] = buffer[i - 1];
				buffer[position] = character;
				length++;
				position++;
				break;
			case _UKEY_BACKSPACE:
				if (!position) break;
				for (stduint i = position - 1; i + 1 < length; i++) buffer[i] = buffer[i + 1];
				length--;
				position--;
				break;
			case _UKEY_DELETE:
				if (position >= length) break;
				for (stduint i = position; i + 1 < length; i++) buffer[i] = buffer[i + 1];
				length--;
				break;
			case _UKEY_LEFT:
				if (!position) break;
				position--;
				break;
			case _UKEY_RIGHT:
				if (position >= length) break;
				position++;
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
			// Repaint: step back to the input start, draw, blank the rest, then return to the cursor.
			for (stduint i = 0; i < cursor; i++) Console.OutChar('\b');
			for (stduint i = 0; i < length; i++) Console.OutChar(buffer[i]);
			stduint drawn = length;
			while (drawn < shown) { Console.OutChar(' '); drawn++; }
			shown = length;
			for (stduint i = drawn; i > position; i--) Console.OutChar('\b');
			cursor = position;
			// A hosted stdout is line buffered, so a repaint without a newline stays invisible until flushed.
		#if defined(_WinNT) || defined(_Linux)
			fflush(stdout);
		#endif
		}
	}

}

#endif // _INCPP_System_Terminal_Line
