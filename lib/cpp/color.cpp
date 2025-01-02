// ASCII C TAB4 CRLF
// Docutitle: (Module) Color
// Codifiers: @dosconio: 20240513
// Attribute: Arn-Covenant Any-Architect Bit-32mode Non-Dependence
// Copyright: UNISYM, under Apache License 2.0; Dosconio Mecocoa, BSD 3-Clause License
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

#include "../../inc/c/graphic/color.h"

namespace uni {
	Color Color::FromRGB888(uint32 argb) {
		Color color = *(Color*)&argb;
		return color;
	}

	Color Color::FromBGR565(uint16 col) {
		Color color;
		color.b = (col) & 0x1F;
		color.g = (col >> 5) & 0x3F;
		color.r = (col >> 11) & 0x1F;
		return color;
	}
}
