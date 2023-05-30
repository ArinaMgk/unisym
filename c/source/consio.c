// GBK RFR30+
// Open-source ArinaMgk
/*
	Copyright 2023 ArinaMgk

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/

#ifdef _WinNT
#include <windows.h>

static HANDLE ConHandle = 0;
void ConInitialize()
{
	ConHandle = GetStdHandle(STD_OUTPUT_HANDLE);
}

void ConCursor(unsigned short col, unsigned short row)
{
	if (!ConHandle) ConInitialize();
	if (!ConHandle) return;
	COORD pos = { x,y };
	SetConsoleCursorPosition(ConHandle, pos);
}

#endif
