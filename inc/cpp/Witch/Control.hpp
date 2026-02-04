// ASCII C++ TAB4 CRLF
// Attribute: 
// AllAuthor: @ArinaMgk
// ModuTitle: uni.Witch.Form
// Reference: (.C#)System.Windows.Controls.Control
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
	class Control {
	private:

	protected:
		stduint left;
		stduint top;
		stduint width;
		stduint height;
		// Form* pare;
	public:
		friend class Form;
		Control(stduint left, stduint top, stduint width, stduint height) : 
			left(left), top(top), width(width), height(height) {
			// if(pare) pare->Controls.Append(this);
		}
		~Control() {
			// if (pare) pare->Controls.Remove((void*)this);
		}
		virtual void Show() = 0;
		virtual void Event(void* event) = 0;
		virtual void Render() = 0;
	};// SheetT


	//
}

#endif
