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

#ifndef _INC_WITCH_Form_X
#define _INC_WITCH_Form_X

#include "../string"
#include "../node"
#include "../trait/SheetTrait.hpp"
#include "../Device/_Video.hpp"

namespace uni::Witch {
	//
	class Control;
	class Form;

	struct Form_CloseButton : public SheetTrait {
		friend class Form;
		Form_CloseButton() = default;
		virtual void doshow(void*) override {}
		virtual void onrupt(SheetEvent event, Point rel_p, ...) override {}
		virtual Color getPoint(Point p) override;
	};
	struct Form_TitleBar : public SheetTrait {
		bool active = false;
		friend class Form;
		Form_TitleBar() = default;
		virtual void doshow(void*) override {}
		virtual void onrupt(SheetEvent event, Point rel_p, ...) override {}
		virtual Color getPoint(Point p) override;
	};

	//{TODO} Form Style: 0 default, 1 win98(default)
	//{TODO} Min Size, Max Size (0 for no limit)
	//{TODO} (uni.String) UTF-8 Title
	//{} title bar buffer
	//{} doshow fill its buffer

	class Form : public LayerManager
	{
	private:
		// subsheet0: Close Button
		Form_CloseButton close_btn;
		// subsheet1: Title Bar
		Form_TitleBar title_bar;
		// subsheetN: Client Area
		LayerManager client_area;// controls
		SheetTrait* focus_sheet = nullptr;

	protected:
		bool active = false;

	public:
		String Title = nullptr;
		// NodeChain Controls = (nullptr);
		Form() : LayerManager() {
		}

		virtual Color getPoint(Point p) override
		{
			for (auto crt = sheet_node.subf; crt; crt = crt->next) {
				if (auto crt_sheet = (SheetTrait*)crt->offs) {
					auto& rect = crt_sheet->sheet_area;
					if (!rect.ifContain(p)) continue;
					const Point pt{ (p.x - rect.x), (p.y - rect.y) };
					auto pp = !crt_sheet->sheet_buffer ? crt_sheet->getPoint(pt) :
						crt_sheet->sheet_buffer[pt.y * crt_sheet->sheet_area.width + pt.x];
					return pp;
				}
			}
			return Color::AliceBlue;
		}

		// [optional] if using buffer
		virtual void doshow(void*) override;

		virtual void onrupt(SheetEvent event, Point rel_p, ...) override;

		//

		void setSheet(LayerManager& layman, const Rectangle& rect, Color* buffer = nullptr);

		bool setFocus(SheetTrait* sheet) {
			//{TODO} check if in client area
			if (sheet == focus_sheet) return true;
			focus_sheet = sheet;
			return true;
		}

		Rectangle getClientArea(void); //_TODO

		bool AppendControl(SheetTrait* sheet) {
			client_area.Append(sheet);
			return true;
		}

		virtual void Update(SheetTrait* who, const Rectangle& rect) override {
			if (sheet_buffer) {
				Color* p = sheet_buffer;
				for0(j, sheet_area.height) for0(i, sheet_area.width) {
					*p++ = getPoint(Point(i, j));
				}
				if (Title.reference()) DrawString_16(self, Point2(3, 3), Title, Color::Black);
				if (Title.reference()) DrawString_16(self, Point2(2, 2), Title, Color::White);
			}
			if (sheet_parent) sheet_parent->Update(this, who->sheet_area);
		}

	};// SheetT


	//
}

#endif
