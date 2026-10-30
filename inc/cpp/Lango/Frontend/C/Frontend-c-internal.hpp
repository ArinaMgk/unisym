// ASCII C++11 TAB4 LF
// Docutitle: Lango C Frontend Internals
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

#ifndef UNISYM_LANGO_FRONTEND_C_INTERNAL_HPP
#define UNISYM_LANGO_FRONTEND_C_INTERNAL_HPP

#include "../Frontend-c.hpp"
#include "../../../parse.hpp"

namespace uni::lango {

// C-specific syntax node types stored above the generic nested-parser range.
enum class FrontendCToken : stduint {
	Function = tok__continue, // Function definition followed by its body block.
	Parameter,                // Typed function parameter normalized before expressions.
	Declaration,              // Reserved for a C declaration statement.
	If,                       // If statement with its condition as children.
	Else,                     // Else marker paired by statement composition.
	While,                    // While loop with its condition as children.
	For,                      // For loop with init, condition, and step clauses.
	Empty,                    // Empty statement represented by a semicolon.
	Return,                   // Return statement or inline return marker.
	End                       // First token value available after the C frontend range.
};

// C99 type categories currently recognized by the frontend.
enum class FrontendCTypeKind {
	Int,    // Target ABI signed int type.
	Pointer // Pointer to another C type.
};

// Minimal semantic C type retained before conversion to fixed-width Clover types.
struct FrontendCType {
	static constexpr stduint MaxArrayDimensions = 16;

	FrontendCTypeKind kind = FrontendCTypeKind::Int;
	unsigned pointer_depth = 0;
	stduint array_dimension_count = 0;
	stduint array_dimensions[MaxArrayDimensions] = {};

	bool IsPointer() const { return pointer_depth != 0; }
	bool IsArray() const { return pointer_depth == 0 && array_dimension_count; }
	bool IsInteger() const {
		return pointer_depth == 0 && !array_dimension_count &&
			kind == FrontendCTypeKind::Int;
	}
	bool IsSame(const FrontendCType& other) const {
		if (kind != other.kind || pointer_depth != other.pointer_depth ||
			array_dimension_count != other.array_dimension_count) return false;
		for (stduint index = 0; index < array_dimension_count; index++)
			if (array_dimensions[index] != other.array_dimensions[index]) return false;
		return true;
	}
	bool AppendArrayDimension(stduint count) {
		if (array_dimension_count >= MaxArrayDimensions) return false;
		array_dimensions[array_dimension_count++] = count;
		return true;
	}
	stduint StorageElementCount() const {
		stduint count = 1;
		for (stduint index = 0; index < array_dimension_count; index++)
			count *= array_dimensions[index];
		return count;
	}
	FrontendCType AddressType() const {
		FrontendCType result = *this;
		result.kind = FrontendCTypeKind::Pointer;
		result.pointer_depth++;
		return result;
	}
	FrontendCType DecayType() const {
		FrontendCType result = *this;
		if (result.array_dimension_count) {
			for (stduint index = 1; index < result.array_dimension_count; index++)
				result.array_dimensions[index - 1] = result.array_dimensions[index];
			result.array_dimensions[--result.array_dimension_count] = 0;
		}
		result.kind = FrontendCTypeKind::Pointer;
		result.pointer_depth++;
		return result;
	}
	FrontendCType DereferenceType() const {
		FrontendCType result = *this;
		if (result.pointer_depth) result.pointer_depth--;
		if (!result.pointer_depth) result.kind = FrontendCTypeKind::Int;
		return result;
	}
};

// Convert a scoped frontend token to the integer representation stored by Nnode.
constexpr stduint ToToken(FrontendCToken token) {
	return (stduint)token;
}

// Source text retained for one C frontend node when debug output is enabled.
struct FrontendCDebugSource {
	const uni::Nnode* node = nullptr;
	uni::String text;
};

// Source mapping consumed while lowering the C syntax tree to Clover IR.
struct FrontendCDebugInfo {
	uni::Vector<FrontendCDebugSource> sources;

	void Add(const uni::Nnode* node, const uni::String& text);
	const uni::String* Find(const uni::Nnode* node) const;
};

// Diagnostic destination and source identity shared by C frontend phases.
struct FrontendCDiagnostics {
	DiagnosticInterface* output = nullptr;
	const char* source_name = nullptr;

	void Report(DiagnosticStage stage, const char* message,
		const uni::Nnode* node = nullptr) const;
};

namespace frontend_c_detail {

	// Populate the operator-precedence groups used by the C frontend.
	void ListOperators(uni::Chain& operators);

	// Classify C statements before and validate them after operator parsing.
	bool Prepare(uni::Nchain& syntax_tree, const FrontendCDiagnostics& diagnostics,
		FrontendCDebugInfo* debug_info = nullptr);
	bool Validate(uni::Nchain& syntax_tree, const FrontendCDiagnostics& diagnostics);

	// Lower an analyzed C syntax tree to an in-memory Clover module.
	bool Lower(uni::Nchain& syntax_tree, clover::Module& module,
		const FrontendCDiagnostics& diagnostics,
		const FrontendCDebugInfo* debug_info = nullptr);

}

}

#endif
