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

#include "../unisym"
#include "../node"
#include "../trait/SheetTrait.hpp"

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
		friend class Form;
		Form_TitleBar() = default;
		virtual void doshow(void*) override {}
		virtual void onrupt(SheetEvent event, Point rel_p, ...) override {}
		virtual Color getPoint(Point p) override;
	};

	//{TODO} Form Style: 0 default, 1 win98(default)
	//{TODO} Min Size, Max Size (0 for no limit)
	//{TODO} (uni.String) UTF-8 Title
	class Form : public SheetTrait
	{
	private:
		// subsheet0: Close Button
		Form_CloseButton close_btn;
		// subsheet1: Title Bar
		Form_TitleBar title_bar;
		// subsheet2: Client Area

	protected:

	public:
		// NodeChain Controls = (nullptr);
		Form() : SheetTrait() {
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
		virtual void doshow(void*) override
		{
			
		}

		virtual void onrupt(SheetEvent event, Point rel_p, ...) override
		{
			
		}

		//

		void setSheet(LayerManager& layman, const Rectangle& rect) {
			InitializeSheet(layman, rect.getVertex(), rect.getSize());
			close_btn.sheet_area = Rectangle(Point(rect.width - 17, 2), Size2(15, 15));
			close_btn.sheet_node.offs = dynamic_cast<SheetTrait*>(&close_btn);
			sheet_node.subf = &close_btn.sheet_node;
			title_bar.sheet_area = Rectangle(Point(1, 1), Size2(rect.width - 2, 17));
			title_bar.sheet_node.offs = dynamic_cast<SheetTrait*>(&title_bar);
			sheet_node.subf->Tail()->next = &title_bar.sheet_node;
		}


	};// SheetT


	//
}

#endif
