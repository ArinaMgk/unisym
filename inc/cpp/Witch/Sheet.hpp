// ASCII C++ TAB4 CRLF
// Attribute: 
// LastCheck: 20241214
// AllAuthor: @dosconio
// ModuTitle: Sheet Tree
// Reference: 
// Parallel : Electron Engine, Flutter
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

#ifndef _INC_SHEET
#define _INC_SHEET

// - The superior diverts the subordinate, the subordinate triggers the superior
// - the page, form, and control are all sheets
#include "../unisym"

namespace uni {

	class Sheet {
	private:
		Sheet* renderring_sheet;
	public:
		_tocall_ft onFocus;// Activated
		_tocall_ft onEnter;// Mouse Move In
		_tocall_ft onLeave;// Mouse Move Out
		_tocall_ft onMoved;// Mouse Move or Finger Move
		_tocall_ft onClick;// Mouse Click or Finger Touch

		virtual void reRender();// Stop this and subs and re-render
		

	};


}


#endif
