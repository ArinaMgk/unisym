// ASCII C++11 TAB4 LF
// Docutitle: Code Output Interface
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

#ifndef UNISYM_FORMAT_CODE_HPP
#define UNISYM_FORMAT_CODE_HPP

#include "../../c/stdinc.h"
#include "../Lango/Target.hpp"

namespace uni {

enum class CodeendFlag : uint32_t {
	None = 0,
	Text = 1u << 0
};

constexpr CodeendFlag operator|(CodeendFlag left, CodeendFlag right)
{
	return static_cast<CodeendFlag>(
		static_cast<uint32_t>(left) | static_cast<uint32_t>(right));
}

constexpr CodeendFlag operator&(CodeendFlag left, CodeendFlag right)
{
	return static_cast<CodeendFlag>(
		static_cast<uint32_t>(left) & static_cast<uint32_t>(right));
}

struct CodeendInfo {
	const char* fullname;
	const char* shortname;
	CodeendFlag flags;
};

enum class CodeendOutputType : uint8_t {
	RawData,
	Reserve
};

struct CodeendOutput {
	int32_t section;
	const void* data;
	CodeendOutputType type;
	uint64_t size;
};

enum class CodeFixupKind : uint8_t {
	Absolute,
	PcRelative,
	Branch,
	Call,
	Got,
	Plt,
	Tls,
	PageRelative,
	PageOffset,
	High,
	Low,
	ArchitectureSpecific
};

struct CodeFixupEncoding {
	uint16_t storage_bits;
	uint16_t value_bits;
	uint16_t bit_offset;
	uint8_t right_shift;
	bool signed_value;
};

struct CodeFixup {
	int32_t section;
	uint64_t offset;
	CodeFixupKind kind;
	CodeFixupEncoding encoding;
	int32_t target_section;
	int64_t addend;
	int64_t pc_bias;
	int32_t legacy_wrt;
	uint32_t architecture_kind;
};

class CodeendInterface {
public:
	virtual ~CodeendInterface() = default;

	virtual const CodeendInfo& DumpCodeend() const = 0;
	virtual bool SupportsArchitecture(
		uni::lango::ArchitectureKind architecture) const = 0;
	virtual const unsigned char* StandardMacros() const = 0;
	virtual void PrepareFilename(char* input_name, char* output_name) = 0;
	virtual void Initialize(FILE* output) = 0;
	virtual void InitializeDebug(FILE* output) = 0;
	virtual void Cleanup(bool debug_info) = 0;
	virtual int32_t SelectSection(
		const char* name, int pass, int& bits) = 0;
	virtual void DefineSymbol(char* name, int32_t section,
		int64_t value, int binding, char* attributes) = 0;
	virtual void DefineDebugSymbol(char* name, int32_t section,
		int64_t value, int binding, char* attributes) = 0;
	virtual bool ProcessDirective(
		char* directive, char* value, int pass) = 0;
	virtual bool ProcessOption(char** option) = 0;
	virtual bool SelectDebugFormat(const char* name) = 0;
	virtual void ListDebugFormats(FILE* output) const = 0;
	virtual void DisableDebug() = 0;
	virtual void ProcessDebugDirective(
		const char* directive, const char* parameters) = 0;
	virtual void SetDebugType(int32_t type) = 0;
	virtual void SetDebugLine(
		const char* filename, int32_t line, int32_t section) = 0;
	virtual void Emit(const CodeendOutput& output) = 0;
	virtual void EmitFixup(const CodeFixup& fixup) = 0;
	virtual int32_t ResolveSegmentBase(int32_t section) = 0;
	virtual void* NativeHandle() const = 0;
};

CodeendInterface& DefaultCodeend();
CodeendInterface* FindCodeend(const char* name);
void SelectCodeend(CodeendInterface& codeend);
void ListCodeends(const CodeendInterface& selected, FILE* output);

}

#endif
