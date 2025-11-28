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
//{TODO} ! Below are included by HEREPIC ! //

#ifndef _INCPP_TRAIT_SHEET
#define _INCPP_TRAIT_SHEET
namespace uni {

	class SheetTrait {
	public:
		bool EnableChrome;
		byte* BufferChrome;
		virtual void doshow() = 0;
	};




}
#endif
