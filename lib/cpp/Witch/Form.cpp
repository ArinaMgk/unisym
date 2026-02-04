// ASCII C++ TAB4 CRLF
// Attribute: 
// Codifiers: @ArinaMgk
// Docutitle: uni.Witch.Form
// Reference: (.C#)System.Windows.Forms
// Copyright: ArinaMgk UniSym, Apache License Version 2.0
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

#include "../../../inc/cpp/Witch/Form.hpp"

static const int CloseButtonWidth = 15;
static const int CloseButtonHeight = 15;
static const char close_button[CloseButtonHeight][CloseButtonWidth + 1] = {
	"..............@",
	".::::::::::::$@",
	".::::::::::::$@",
	".::@@::::@@::$@",
	".:::@@::@@:::$@",
	".::::@@@@::::$@",
	".:::::@@:::::$@",
	".::::@@@@::::$@",
	".:::@@::@@:::$@",
	".::@@::::@@::$@",
	".::::::::::::$@",
	".::::::::::::$@",
	".::::::::::::$@",
	".$$$$$$$$$$$$$@",
	"@@@@@@@@@@@@@@@",
};

uni::Color uni::Witch::Form_CloseButton::getPoint(Point p)
{
	auto dat = close_button[p.y][p.x];
	if (dat == '@') {
	return (0xFF000000);
	} else if (dat == '$') {
	return (0xFF848484);
	} else if (dat == ':') {
	return (0xFFC6C6C6);
	}
	return (0x00000000);
}

uni::Color uni::Witch::Form_TitleBar::getPoint(Point p)
{
	Color c = 0xFF000000;
	c.b = 0x7F + ((uint32)p.x * 0x40) / sheet_area.width;
	c.g = 0x00 + ((uint32)p.x * 0x80) / sheet_area.width;
	return c;
}

