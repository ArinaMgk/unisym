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
	Declaration,              // Function-local object declaration.
	GlobalDeclaration,        // Translation-unit object declaration.
	If,                       // If statement with its condition as children.
	Else,                     // Else marker paired by statement composition.
	While,                    // While loop with its condition as children.
	For,                      // For loop with init, condition, and step clauses.
	Empty,                    // Empty statement represented by a semicolon.
	Return,                   // Return statement or inline return marker.
	ReturnVoid,               // Return statement without a value.
	End                       // First token value available after the C frontend range.
};

// Relative rank used by the C99 integer promotions and conversions.
enum class FrontendCIntegerRank {
	Char,     // char, signed char, or unsigned char.
	Short,    // short int or unsigned short int.
	Int,      // int or unsigned int.
	Long,     // long int or unsigned long int.
	LongLong  // long long int or unsigned long long int.
};

// Type qualifiers attached to one level of a C declarator.
struct FrontendCTypeQualifier {
	bool is_const = false; // The object represented by this type level is not writable.
};

// Semantic integer, pointer, and array type retained by the C frontend.
struct FrontendCType {
	static constexpr stduint MaxArrayDimensions = 16;
	static constexpr stduint MaxPointerDepth = 16;

	FrontendCIntegerRank integer_rank = FrontendCIntegerRank::Int;
	bool is_signed = true;
	bool is_void = false;
	FrontendCTypeQualifier base_qualifiers;
	unsigned pointer_depth = 0;
	FrontendCTypeQualifier pointer_qualifiers[MaxPointerDepth] = {};
	stduint array_dimension_count = 0;
	stduint array_dimensions[MaxArrayDimensions] = {};

	bool IsPointer() const { return pointer_depth != 0; }
	bool IsVoid() const { return is_void && pointer_depth == 0; }
	bool IsArray() const { return pointer_depth == 0 && array_dimension_count; }
	bool IsInteger() const {
		return !is_void && pointer_depth == 0 && !array_dimension_count;
	}
	bool HasSameShape(const FrontendCType& other) const {
		if (integer_rank != other.integer_rank || is_signed != other.is_signed ||
			is_void != other.is_void ||
			pointer_depth != other.pointer_depth ||
			array_dimension_count != other.array_dimension_count) return false;
		for (stduint index = 0; index < array_dimension_count; index++)
			if (array_dimensions[index] != other.array_dimensions[index]) return false;
		return true;
	}
	bool IsSame(const FrontendCType& other) const {
		if (!HasSameShape(other) ||
			base_qualifiers.is_const != other.base_qualifiers.is_const) return false;
		for (stduint index = 0; index < pointer_depth; index++)
			if (pointer_qualifiers[index].is_const !=
				other.pointer_qualifiers[index].is_const) return false;
		return true;
	}
	bool CanAssignFrom(const FrontendCType& source) const {
		if (!HasSameShape(source)) return false;
		if (source.base_qualifiers.is_const && !base_qualifiers.is_const) return false;
		for (stduint index = 0; index < pointer_depth; index++)
			if (source.pointer_qualifiers[index].is_const &&
				!pointer_qualifiers[index].is_const) return false;
		return true;
	}
	bool IsConstObject() const {
		return pointer_depth ? pointer_qualifiers[pointer_depth - 1].is_const :
			base_qualifiers.is_const;
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
		if (result.pointer_depth < MaxPointerDepth)
			result.pointer_qualifiers[result.pointer_depth++] = {};
		return result;
	}
	FrontendCType DecayType() const {
		FrontendCType result = *this;
		if (result.array_dimension_count) {
			for (stduint index = 1; index < result.array_dimension_count; index++)
				result.array_dimensions[index - 1] = result.array_dimensions[index];
			result.array_dimensions[--result.array_dimension_count] = 0;
		}
		if (result.pointer_depth < MaxPointerDepth)
			result.pointer_qualifiers[result.pointer_depth++] = {};
		return result;
	}
	FrontendCType DereferenceType() const {
		FrontendCType result = *this;
		if (result.pointer_depth) {
			result.pointer_qualifiers[--result.pointer_depth] = {};
		}
		return result;
	}
};

// Associates normalized syntax nodes with their C types without extending Nnode.
struct FrontendCTypedNode {
	const uni::Nnode* node = nullptr;
	FrontendCType type;
};

// Type information collected before generic operator parsing rearranges nodes.
struct FrontendCTypeInfo {
	uni::Vector<FrontendCTypedNode> nodes;

	void Add(const uni::Nnode* node, const FrontendCType& type);
	const FrontendCType* Find(const uni::Nnode* node) const;
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
		const FrontendCTargetInfo& target, FrontendCTypeInfo& type_info,
		FrontendCDebugInfo* debug_info = nullptr);
	bool Validate(uni::Nchain& syntax_tree, const FrontendCDiagnostics& diagnostics);

	// Lower an analyzed C syntax tree to an in-memory Clover module.
	bool Lower(uni::Nchain& syntax_tree, clover::Module& module,
		const FrontendCDiagnostics& diagnostics, const FrontendCTargetInfo& target,
		const FrontendCTypeInfo& type_info,
		const FrontendCDebugInfo* debug_info = nullptr);

}

}

#endif
