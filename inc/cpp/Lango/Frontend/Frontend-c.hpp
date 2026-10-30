// ASCII C++11 TAB4 LF
// Docutitle: Lango C Frontend
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

#ifndef UNISYM_LANGO_FRONTEND_C_HPP
#define UNISYM_LANGO_FRONTEND_C_HPP

#include "../Frontend.hpp"

namespace uni::lango {

// Conventional C integer and pointer width families selected by an ABI.
enum class CDataModel {
	ILP32, // int, long, and pointers are 32-bit.
	LP64,  // int is 32-bit; long and pointers are 64-bit.
	LLP64  // int and long are 32-bit; long long and pointers are 64-bit.
};

// C-specific type sizes derived from a target and its ABI data model.
struct FrontendCTargetInfo {
	CDataModel data_model = CDataModel::LP64;
	unsigned char_bits = 8;
	unsigned short_bits = 16;
	unsigned int_bits = 32;
	unsigned long_bits = 64;
	unsigned long_long_bits = 64;
	bool char_is_signed = true;

	static FrontendCTargetInfo FromTarget(const TargetInfo& target);
};

inline FrontendCTargetInfo FrontendCTargetInfo::FromTarget(const TargetInfo& target) {
	FrontendCTargetInfo info;
	info.char_bits = target.byte_bits;
	if (target.pointer_bits == 32) {
		info.data_model = CDataModel::ILP32;
		info.long_bits = 32;
	}
	return info;
}

// C source frontend producing Clover IR.
class FrontendC : public FrontendInterface {
public:
	// Parse C source and lower the complete translation unit to Clover IR.
	virtual bool Generate(uni::IstreamTrait& input, clover::Module& module,
		const FrontendOptions& options = FrontendOptions()) override;

	// Parse C source and print its frontend-specific syntax tree.
	bool PrintAST(uni::IstreamTrait& input, uni::OstreamTrait& output,
		const FrontendOptions& options = FrontendOptions());
};

}

#endif
