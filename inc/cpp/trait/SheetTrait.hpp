// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Trait) SheetTrait
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
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
#include "../unisym"
#include "../../c/graphic.h"
//{TODO} ! Below are included by HEREPIC ! //

#ifndef _INCPP_TRAIT_SHEET
#define _INCPP_TRAIT_SHEET
// - The superior diverts the subordinate, the subordinate triggers the superior
// - the page, form, and control are all sheets
namespace uni {
	
	enum class SheetEvent {
		onFocus,// Activated
		onEnter,// Mouse Move In
		onLeave,// Mouse Move Out
		onMoved,// Mouse Move or Finger Move
		onClick,// Mouse Click or Finger Touch
	};

	class LayerManager;
	class SheetTrait {
	public:
		LayerManager* sheet_parent = nullptr;
		SheetTrait* sheet_pnext = nullptr;
		Rectangle sheet_area;
		Color* sheet_buffer = nullptr;// if set, the LayMan will fetch the uni.Color from the buffer
	public:
		SheetTrait() = default;

		void InitializeSheet(LayerManager& _parent, Point vertex, Size2 size, Color* buf) {
			sheet_parent = &_parent;
			sheet_pnext = nullptr;
			sheet_area = Rectangle{ vertex, size, Color::White, false };
			sheet_buffer = buf;
		}

		virtual Color getPoint(Point p) { return Color::Black; }

		// [optional] if using buffer
		virtual void doshow(void*) = 0;

		virtual bool isEntity(Point rel_p) {
			return rel_p.x < sheet_area.width && rel_p.y < sheet_area.height;
		}// for shape which is not rect

		virtual void onrupt(SheetEvent event, Point rel_p, ...) = 0;

	};


}
#endif
