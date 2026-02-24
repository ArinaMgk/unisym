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

namespace uni::witch::control {

	class TextBox : public SheetTrait {
	public:
		String text = "";
		TextBox() : SheetTrait() {}
		~TextBox() = default;

	public:
		virtual Color getPoint(Point p) override {
			_TEMP (void)p; return Color::Black;
		}

		// Update the buffer
		virtual void doshow(void*) override {
			if (!sheet_buffer) sheet_buffer = (Color*)calloc(sheet_area.getArea(), sizeof(Color));
			//{now} only consider single line
			VideoControlInterfaceMARGB8888 vcim(sheet_buffer, sheet_area.getSize());
			vcim.DrawRectangle(Rectangle(Point(0, 0), Size2(sheet_area.width, sheet_area.height), Color::White));
			vcim.DrawRectangle(Rectangle(Point(0, 0), Size2(3, sheet_area.height), 0xFF7F7F7F));
			vcim.DrawRectangle(Rectangle(Point(3, 0), Size2(sheet_area.width - 6, 3), 0xFF7F7F7F));
			vcim.DrawRectangle(Rectangle(Point(sheet_area.width - 3, 0), Size2(3, sheet_area.height), 0xFFDDDFE1));
			vcim.DrawRectangle(Rectangle(Point(3, sheet_area.height - 3), Size2(sheet_area.width - 6, 3), 0xFFDDDFE1));
			DrawString_16(self, Point(3, 3), text, Color::Black);
			if (sheet_parent) sheet_parent->Update(this, Rectangle(Point(0,0), sheet_area.getSize()));
		}

		virtual void onrupt(SheetEvent event, Point rel_p, ...) override { }
	};

}
#endif
