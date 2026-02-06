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
#include "../../../inc/c/data.h"
#include "../../../inc/cpp/reference"
#include "../../../inc/cpp/Device/_Video.hpp"

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
	else return (0xFFFFFFFF);
}

uni::Color uni::Witch::Form_TitleBar::getPoint(Point p)
{
	Color c = 0xFF000000;
	c.b = 0x7F + ((uint32)p.x * 0x40) / sheet_area.width;
	c.g = 0x00 + ((uint32)p.x * 0x80) / sheet_area.width;
	return c;
}

void uni::Witch::Form::doshow(void* _)
{
	
}

void uni::Witch::Form::onrupt(SheetEvent event, Point rel_p, ...)
{
	Letpara(args, rel_p);
	// notice layman if button_dn the title bar
	if (sheet_parent && title_bar.sheet_area.ifContain(rel_p) &&
		!close_btn.sheet_area.ifContain(rel_p)) {
		sheet_parent->Dorupt(this, event, rel_p, args);
	}
}

void uni::Witch::Form::setSheet(LayerManager& layman, const Rectangle& rect, Color* buffer) {
	InitializeSheet(layman, rect.getVertex(), rect.getSize());
	close_btn.sheet_area = Rectangle(Point(rect.width - 17, 2), Size2(CloseButtonWidth, CloseButtonHeight));
	close_btn.sheet_node.offs = dynamic_cast<SheetTrait*>(&close_btn);
	sheet_node.subf = &close_btn.sheet_node;
	title_bar.sheet_area = Rectangle(Point(1, 1), Size2(rect.width - 2, 17));
	title_bar.sheet_node.offs = dynamic_cast<SheetTrait*>(&title_bar);
	sheet_node.subf->Tail()->next = &title_bar.sheet_node;

	if (buffer) {
		Color* p = buffer;
		for0(j, rect.height) for0(i, rect.width) {
			*p++ = getPoint(Point(i, j));
		}
		sheet_buffer = buffer;
		if (Title.reference()) DrawString_16(Point2(3, 3), Title, Color::Black);
		if (Title.reference()) DrawString_16(Point2(2, 2), Title, Color::White);

	}

}

void uni::Witch::Form::DrawString_16(const Point2& p, const String& str, Color col) {
	//{} ASCII only
	Point2 pp = p;
	if (!sheet_buffer) return;
	for (const char* pstr = str.reference(); *pstr; pstr++) {
		byte ch = *pstr;
		if (!ascii_isprint(*pstr)) continue;
		const uint16(*datptr) = (const uint16(*)) & _BITFONT_ASCII_16x8[ch - 0x20];
		uint16 dat = 0; Reference_T<uint16> dat_bmap _IMM(&dat);
		for0(i, 8) {
			dat = datptr[i];
			for0r(j, 16) {
				if (dat_bmap.bitof(j)) sheet_buffer[pp.x + (pp.y + (j ^ 0b111)) * sheet_area.width] = col;
			}
			pp.x++;
		}
		if (pp.x >= sheet_area.width) break;
	}
}

