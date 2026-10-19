// ASCII CPP TAB4 CRLF
// Docutitle: (System) Terminal Backend
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

#ifndef _INCPP_System_Terminal_Backend
#define _INCPP_System_Terminal_Backend

#include "../../../c/stdinc.h"
#include "../../../c/driver/keyboard.h"

#if defined(_ACCM)
extern "C" int sysinnc(int blocked, byte* keycode, byte* properties);
#elif defined(_WinNT)
#include <conio.h>
#elif defined(_Linux)
#include <termios.h>
#include <unistd.h>
#endif

namespace uni {

	enum class TerminalRawProperties : byte {
		None = 0,
		Release = 1,
	};

	struct TerminalRaw {
		byte character;
		byte keycode;
		TerminalRawProperties properties;
	};

	// Hand out one raw event at a time, whatever platform this build targets.
	class TerminalBackend {
	public:
		// Block until one event is available, or return at once when block is false.
		bool Next(TerminalRaw& raw, bool block = true) {
		#if defined(_ACCM)
			byte keycode = 0;
			byte properties = 0;
			int character = sysinnc(block ? 1 : 0, &keycode, &properties);
			if (character == -1) return false;
			raw.character = (byte)character;
			raw.keycode = keycode;
			raw.properties = properties ? TerminalRawProperties::Release : TerminalRawProperties::None;
			return true;
		#elif defined(_WinNT)
			if (!block && !_kbhit()) return false;
			int character = _getch();
			if (character == -1) return false;
			if (character == 0 || character == 0xE0) {
				int code = _getch();
				raw.character = 0;
				raw.keycode = (code > 0 && code < 128) ? key_ps2set1_usb_E0[code] : _UKEY_NONE;
			}
			else {
				raw.character = (byte)character;
				raw.keycode = 0;
			}
			raw.properties = TerminalRawProperties::None;
			return true;
		#elif defined(_Linux)
			struct termios saved;
			if (tcgetattr(STDIN_FILENO, &saved) != 0) return false;
			struct termios mode = saved;
			mode.c_lflag &= ~(ICANON | ECHO);
			mode.c_cc[VTIME] = 0;
			mode.c_cc[VMIN] = block ? 1 : 0;
			if (tcsetattr(STDIN_FILENO, TCSANOW, &mode) != 0) return false;
			unsigned char character = 0;
			int got = (int)read(STDIN_FILENO, &character, 1);
			tcsetattr(STDIN_FILENO, TCSANOW, &saved);
			if (got != 1) return false;
			raw.character = character;
			raw.keycode = 0;
			raw.properties = TerminalRawProperties::None;
			return true;
		#else
			(void)raw;
			(void)block;
			return false;
		#endif
		}
	};

}

#endif // _INCPP_System_Terminal_Backend
