// ASCII C++11 TAB4 LF
// Docutitle: Clover RISC-V Backend
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

#ifndef UNISYM_LANGO_BACKEND_RISCV_HPP
#define UNISYM_LANGO_BACKEND_RISCV_HPP

#include "../Backend.hpp"

namespace uni::lango {

// Target features selected for one RISC-V backend instance.
struct RISCVTargetInfo {
	unsigned bits = 64;     // General-purpose register width.
	bool extension_i = true; // Base integer instruction set.
	bool extension_m = true; // Integer multiplication and division extension.
};

// RISC-V backend producing GNU assembler source.
class BackendRISCV : public BackendInterface {
private:
	RISCVTargetInfo target;

public:
	// Construct a backend for the requested register width and extensions.
	BackendRISCV(const RISCVTargetInfo& target_info = RISCVTargetInfo()) : target(target_info) {}

	// Lower all functions in module to RISC-V assembly.
	virtual bool Generate(const clover::Module& module, uni::OstreamTrait& output) override;
};

}

#endif
