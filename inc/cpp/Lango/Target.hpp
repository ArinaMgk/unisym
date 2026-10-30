// ASCII C++11 TAB4 LF
// Docutitle: Lango Target Information
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0
/*
	Copyright 2026 ArinaMgk

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

#ifndef UNISYM_LANGO_TARGET_HPP
#define UNISYM_LANGO_TARGET_HPP

#include "../unisym"

namespace uni::lango {

// Language-independent machine properties shared by frontends and backends.
struct TargetInfo {
	unsigned register_bits = 64;
	unsigned pointer_bits = 64;
	unsigned byte_bits = 8;
};

}

#endif
