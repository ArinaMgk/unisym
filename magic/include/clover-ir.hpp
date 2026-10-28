// ASCII C++11 TAB4 LF
// Docutitle: Magice Clover IR Lowering Interface
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

#ifndef MAGIC_CLOVER_IR_HPP
#define MAGIC_CLOVER_IR_HPP

#include "../../inc/c/nnode.h"
#include "../../inc/cpp/Lango/CloverIR.hpp"

namespace clover {

// Lower a parsed Magice syntax tree into a Clover IR module.
bool Generate(uni::Nchain& syntax_tree, Module& module);

}

#endif
