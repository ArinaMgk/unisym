// ASCII CPP TAB4 CRLF
// Docutitle: (System) Terminal Decoder
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

#ifndef _INCPP_System_Terminal_Decoder
#define _INCPP_System_Terminal_Decoder

#include "../../../c/driver/keyboard.h"
#include "Backend.hpp"

namespace uni {

	// Turn raw backend events into _USB_IF_KEYENUM keys, _UKEY_NONE marks a plain character.
	class TerminalDecoder {
	protected:
		TerminalBackend* backend;
		bool Decode(_USB_IF_KEYENUM& key, char& character, bool block);
	public:
		TerminalDecoder(TerminalBackend* backend) : backend(backend) {}
		// Block until one key is available, return false when the stream is over.
		bool Poll(_USB_IF_KEYENUM& key, char& character);
		// Take whatever is already waiting, return false when nothing is.
		bool TryPoll(_USB_IF_KEYENUM& key, char& character);
	};

	inline bool TerminalDecoder::Poll(_USB_IF_KEYENUM& key, char& character) {
		return Decode(key, character, true);
	}

	inline bool TerminalDecoder::TryPoll(_USB_IF_KEYENUM& key, char& character) {
		return Decode(key, character, false);
	}

	inline bool TerminalDecoder::Decode(_USB_IF_KEYENUM& key, char& character, bool block) {
		TerminalRaw raw;
		TerminalRaw ahead;
		for (;;) {
			if (!backend->Next(raw, block)) return false;
			if (raw.properties == TerminalRawProperties::Release) continue;
			// A real keyboard delivers keycodes directly, a serial terminal does not.
			if (raw.character == 0) {
				if (raw.keycode == 0) continue;
				key = (_USB_IF_KEYENUM)raw.keycode;
				return true;
			}
			if (raw.character == '\n' || raw.character == '\r') {
				key = _UKEY_ENTER;
				return true;
			}
			if (raw.character == '\b' || raw.character == 0x7F) {
				key = _UKEY_BACKSPACE;
				return true;
			}
			if (raw.character == '\t') {
				key = _UKEY_TAB;
				return true;
			}
			if (raw.character != 0x1B) {
				key = _UKEY_NONE;
				character = (char)raw.character;
				return true;
			}
			// Escape: either a lone ESC key or the start of a CSI sequence.
			if (!backend->Next(ahead, block)) return false;
			if (ahead.character != '[') {
				key = _UKEY_ESC;
				return true;
			}
			if (!backend->Next(ahead, block)) return false;
			switch (ahead.character) {
			case 'A': key = _UKEY_UP; return true;
			case 'B': key = _UKEY_DOWN; return true;
			case 'C': key = _UKEY_RIGHT; return true;
			case 'D': key = _UKEY_LEFT; return true;
			case 'H': key = _UKEY_HOME; return true;
			case 'F': key = _UKEY_END; return true;
			default: break;
			}
		}
	}

}

#endif // _INCPP_System_Terminal_Decoder
