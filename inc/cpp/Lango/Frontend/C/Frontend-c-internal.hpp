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
	FunctionDeclaration,     // Function prototype without a body.
	TypedefDeclaration,      // Type alias declaration that emits no runtime object.
	Parameter,                // Typed function parameter normalized before expressions.
	VariadicParameter,        // Ellipsis terminating the fixed parameter list.
	Declaration,              // Function-local object declaration.
	GlobalDeclaration,        // Translation-unit object declaration.
	RecordDeclaration,        // Translation-unit struct or union forward declaration.
	RecordDefinition,         // Translation-unit struct or union type definition.
	EnumDeclaration,          // Enumeration tag declaration without an enumerator list.
	EnumDefinition,           // Translation-unit enumeration type definition.
	If,                       // If statement with its condition as children.
	Else,                     // Else marker paired by statement composition.
	While,                    // While loop with its condition as children.
	Do,                       // Post-tested loop whose trailing while follows its body.
	For,                      // For loop with init, condition, and step clauses.
	Switch,                   // Integer selection statement followed by its body.
	Case,                     // Constant selection label within the nearest switch.
	Default,                  // Fallback selection label within the nearest switch.
	Break,                    // Leave the nearest enclosing loop or switch.
	Continue,                 // Continue the nearest enclosing loop.
	Label,                    // Function-scope destination preceding one statement.
	Goto,                     // Unconditional jump to a function-scope label.
	Empty,                    // Empty statement represented by a semicolon.
	Return,                   // Return statement or inline return marker.
	ReturnVoid,               // Return statement without a value.
	CharacterLiteral,         // C ordinary character constant delimited by single quotes.
	CallExpression,           // Call through a parenthesized callee expression.
	DesignatedInitializer,    // C99 designation followed by one initializer value.
	MemberDesignator,         // A .member component in a C99 designation.
	IndexDesignator,          // An [index] component in a C99 designation.
	InitializerValue,         // Expression isolated from its designation components.
	End                       // First token value available after the C frontend range.
};

// Relative rank used by the C99 integer promotions and conversions.
enum class FrontendCIntegerRank {
	Bool,     // C99 _Bool.
	Char,     // char, signed char, or unsigned char.
	Short,    // short int or unsigned short int.
	Int,      // int or unsigned int.
	Long,     // long int or unsigned long int.
	LongLong  // long long int or unsigned long long int.
};

// Arithmetic category retained independently from integer rank and signedness.
enum class FrontendCScalarKind {
	Integer,
	Float,
	Double,
	LongDouble
};

// C storage-class information retained separately from the declared type.
enum class FrontendCStorageClass {
	None,
	Static,
	Extern,
	Typedef
};

// Type qualifiers attached to one level of a C declarator.
struct FrontendCTypeQualifier {
	bool is_const = false; // The object represented by this type level is not writable.
};

// One derived declarator layer, ordered from the declared identifier outward.
enum class FrontendCTypeLayerKind {
	Pointer, // Pointer to the type represented by the following layers.
	Array,   // Array of the type represented by the following layers.
	Function // Function returning the type represented by the following layers.
};

struct FrontendCTypeLayer {
	FrontendCTypeLayerKind kind = FrontendCTypeLayerKind::Pointer;
	FrontendCTypeQualifier qualifiers;
	stduint array_count = 0; // Zero denotes an incomplete array bound.
	stduint function_type_id = 0; // One-based FrontendCFunctionType identity.
};

// Semantic C type retained independently from the declarator syntax tree.
struct FrontendCType {
	static constexpr stduint MaxTypeLayers = 32;

	FrontendCIntegerRank integer_rank = FrontendCIntegerRank::Int;
	FrontendCScalarKind scalar_kind = FrontendCScalarKind::Integer;
	bool is_signed = true;
	bool is_void = false;
	bool is_incomplete = false; // The enum tag was incomplete at this use location.
	FrontendCTypeQualifier base_qualifiers;
	stduint record_id = 0; // One-based identity in FrontendCTypeInfo::records; zero is scalar.
	stduint enum_id = 0; // One-based identity in FrontendCTypeInfo::enums; zero is not enum.
	stduint layer_count = 0;
	FrontendCTypeLayer layers[MaxTypeLayers] = {};

	bool IsPointer() const {
		return layer_count && layers[0].kind == FrontendCTypeLayerKind::Pointer;
	}
	bool IsArray() const {
		return layer_count && layers[0].kind == FrontendCTypeLayerKind::Array;
	}
	bool IsFunction() const {
		return layer_count && layers[0].kind == FrontendCTypeLayerKind::Function;
	}
	bool IsFunctionPointer() const {
		return layer_count > 1 &&
			layers[0].kind == FrontendCTypeLayerKind::Pointer &&
			layers[1].kind == FrontendCTypeLayerKind::Function;
	}
	bool IsDerived() const { return layer_count != 0; }
	bool IsVoid() const { return is_void && !layer_count; }
	bool IsRecord() const {
		return record_id != 0 && !layer_count;
	}
	bool IsEnum() const {
		return enum_id != 0 && !layer_count;
	}
	bool IsInteger() const {
		return scalar_kind == FrontendCScalarKind::Integer && !is_void &&
			!record_id && !layer_count;
	}
	bool IsFloating() const {
		return (scalar_kind == FrontendCScalarKind::Float ||
			scalar_kind == FrontendCScalarKind::Double ||
			scalar_kind == FrontendCScalarKind::LongDouble) && !is_void &&
			!record_id && !layer_count;
	}
	bool IsArithmetic() const { return IsInteger() || IsFloating(); }
	bool IsScalar() const { return IsArithmetic() || IsPointer(); }
	bool AppendLayer(const FrontendCTypeLayer& layer) {
		if (layer_count >= MaxTypeLayers) return false;
		layers[layer_count++] = layer;
		return true;
	}
	bool PrependLayer(const FrontendCTypeLayer& layer) {
		if (layer_count >= MaxTypeLayers) return false;
		for (stduint index = layer_count; index; index--)
			layers[index] = layers[index - 1];
		layers[0] = layer;
		layer_count++;
		return true;
	}
	bool AppendPointer() {
		FrontendCTypeLayer layer;
		layer.kind = FrontendCTypeLayerKind::Pointer;
		return AppendLayer(layer);
	}
	bool AppendArrayDimension(stduint count) {
		FrontendCTypeLayer layer;
		layer.kind = FrontendCTypeLayerKind::Array;
		layer.array_count = count;
		return AppendLayer(layer);
	}
	bool HasSameShape(const FrontendCType& other) const {
		if (integer_rank != other.integer_rank || scalar_kind != other.scalar_kind ||
			is_signed != other.is_signed ||
			is_void != other.is_void || record_id != other.record_id ||
			enum_id != other.enum_id || layer_count != other.layer_count) return false;
		for (stduint index = 0; index < layer_count; index++)
			if (layers[index].kind != other.layers[index].kind ||
				layers[index].array_count != other.layers[index].array_count ||
				layers[index].function_type_id !=
				other.layers[index].function_type_id) return false;
		return true;
	}
	bool IsSame(const FrontendCType& other) const {
		if (!HasSameShape(other) ||
			base_qualifiers.is_const != other.base_qualifiers.is_const) return false;
		for (stduint index = 0; index < layer_count; index++)
			if (layers[index].qualifiers.is_const !=
				other.layers[index].qualifiers.is_const) return false;
		return true;
	}
	bool CanAssignFrom(const FrontendCType& source) const {
		if (!HasSameShape(source)) return false;
		if (source.base_qualifiers.is_const && !base_qualifiers.is_const) return false;
		for (stduint index = 0; index < layer_count; index++)
			if (source.layers[index].qualifiers.is_const &&
				!layers[index].qualifiers.is_const) return false;
		return true;
	}
	bool IsConstObject() const {
		return layer_count ? layers[0].qualifiers.is_const : base_qualifiers.is_const;
	}
	bool HasIncompleteArray() const {
		for (stduint index = 0; index < layer_count &&
			layers[index].kind == FrontendCTypeLayerKind::Array; index++)
			if (!layers[index].array_count) return true;
		return false;
	}
	stduint ArrayDimensionCount() const {
		stduint count = 0;
		while (count < layer_count &&
			layers[count].kind == FrontendCTypeLayerKind::Array) count++;
		return count;
	}
	stduint ArrayDimension(stduint index) const {
		return index < ArrayDimensionCount() ? layers[index].array_count : 0;
	}
	bool SetArrayDimension(stduint index, stduint count) {
		if (index >= ArrayDimensionCount()) return false;
		layers[index].array_count = count;
		return true;
	}
	stduint StorageElementCount() const {
		stduint count = 1;
		for (stduint index = 0; index < layer_count &&
			layers[index].kind == FrontendCTypeLayerKind::Array; index++)
			count *= layers[index].array_count;
		return count;
	}
	bool StorageElementIsPointer() const {
		stduint index = 0;
		while (index < layer_count &&
			layers[index].kind == FrontendCTypeLayerKind::Array) index++;
		return index < layer_count &&
			layers[index].kind == FrontendCTypeLayerKind::Pointer;
	}
	FrontendCType RemoveOuterLayer() const {
		FrontendCType result = *this;
		if (result.layer_count) {
			for (stduint index = 1; index < result.layer_count; index++)
				result.layers[index - 1] = result.layers[index];
			result.layers[--result.layer_count] = {};
		}
		return result;
	}
	FrontendCType AddressType() const {
		FrontendCType result = *this;
		FrontendCTypeLayer layer;
		layer.kind = FrontendCTypeLayerKind::Pointer;
		result.PrependLayer(layer);
		return result;
	}
	FrontendCType DecayType() const {
		FrontendCType result = IsArray() ? RemoveOuterLayer() : *this;
		if (!IsArray() && !IsFunction()) return result;
		FrontendCTypeLayer layer;
		layer.kind = FrontendCTypeLayerKind::Pointer;
		result.PrependLayer(layer);
		return result;
	}
	FrontendCType DereferenceType() const {
		return IsPointer() ? RemoveOuterLayer() : *this;
	}
};

// One named member in a C structure or union definition.
struct FrontendCRecordMember {
	uni::String name;
	FrontendCType type;
	stduint offset_bits = 0;
};

// Layout policy selected for one Magice record definition.
enum class FrontendCRecordLayout {
	Default, // Use the target C ABI without an explicit layout keyword.
	Padded,  // Explicitly request the target C ABI layout.
	Packed   // Omit implicit member padding.
};

// One tagged C structure or union retained independently from syntax nodes.
struct FrontendCRecord {
	uni::String tag;
	uni::Vector<FrontendCRecordMember> members;
	stduint size_bits = 0;
	stduint align_bits = 0;
	bool is_union = false;
	FrontendCRecordLayout layout = FrontendCRecordLayout::Default;
	stduint explicit_align_bits = 0; // Alignment requested by alignb or aligne.
	bool complete = false;
};

// One enumeration constant in the ordinary C identifier namespace.
struct FrontendCEnumerator {
	uni::String name;
	stdsint value = 0;
	FrontendCType type;
	const uni::Nnode* declaration = nullptr;
};

// One tagged C enumeration definition.
struct FrontendCEnum {
	uni::String tag;
	uni::Vector<FrontendCEnumerator> enumerators;
	const uni::Nnode* declaration = nullptr;
	const uni::Nnode* tag_declaration = nullptr;
	const uni::Nnode* body = nullptr;
	const uni::Nnode* scope = nullptr;
	FrontendCIntegerRank integer_rank = FrontendCIntegerRank::Int;
	bool is_signed = true;
	bool complete = false;
};

// Associates normalized syntax nodes with their C types without extending Nnode.
struct FrontendCTypedNode {
	const uni::Nnode* node = nullptr;
	FrontendCType type;
};

// Parameter information shared by declarations containing the same function type.
struct FrontendCFunctionType {
	uni::Vector<FrontendCType> parameter_types;
	bool is_variadic = false;
	bool has_prototype = false;
};

// Associates one declarator function suffix with its interned function type.
struct FrontendCFunctionTypeNode {
	const uni::Nnode* node = nullptr;
	stduint function_type_id = 0;
};

// One function declaration or definition after parameter type normalization.
struct FrontendCFunctionDeclaration {
	const uni::Nnode* node = nullptr;
	uni::String name;
	FrontendCType return_type;
	uni::Vector<FrontendCType> parameter_types;
	stduint function_type_id = 0;
	bool is_variadic = false;
	bool has_prototype = false;
	bool is_definition = false;
};

// Associates one declaration node with its C storage-class specifier.
struct FrontendCStorageNode {
	const uni::Nnode* node = nullptr;
	FrontendCStorageClass storage_class = FrontendCStorageClass::None;
};

// One typedef name in the ordinary identifier namespace.
struct FrontendCTypedef {
	uni::String name;
	FrontendCType type;
	const uni::Nnode* declaration = nullptr;
	const uni::Nnode* scope = nullptr;
};

// One non-typedef declaration in C's ordinary identifier namespace.
struct FrontendCOrdinaryName {
	uni::String name;
	const uni::Nnode* declaration = nullptr;
	const uni::Nnode* scope = nullptr;
};

// One object declarator retained before generic operator parsing rearranges it.
struct FrontendCObjectDeclarator {
	const uni::Nnode* declaration = nullptr;
	uni::Nnode* identifier = nullptr;
	uni::Nnode* initializer = nullptr;
	FrontendCType type;
};

// Type information collected before generic operator parsing rearranges nodes.
struct FrontendCTypeInfo {
	uni::Vector<FrontendCTypedNode> nodes;
	uni::Vector<FrontendCStorageNode> storage_nodes;
	uni::Vector<FrontendCFunctionType> function_types;
	uni::Vector<FrontendCFunctionTypeNode> function_type_nodes;
	uni::Vector<FrontendCFunctionDeclaration> functions;
	uni::Vector<FrontendCTypedef> typedefs;
	uni::Vector<FrontendCOrdinaryName> ordinary_names;
	uni::Vector<FrontendCObjectDeclarator> object_declarators;
	uni::Vector<FrontendCRecord> records;
	uni::Vector<FrontendCEnum> enums;

	void Add(const uni::Nnode* node, const FrontendCType& type);
	const FrontendCType* Find(const uni::Nnode* node) const;
	stduint InternFunctionType(const FrontendCFunctionType& type);
	void AddFunctionTypeNode(const uni::Nnode* node, stduint function_type_id);
	stduint FindFunctionTypeNode(const uni::Nnode* node) const;
	const FrontendCFunctionType* GetFunctionType(stduint function_type_id) const;
	void AddStorage(const uni::Nnode* node, FrontendCStorageClass storage_class);
	FrontendCStorageClass FindStorage(const uni::Nnode* node) const;
	const FrontendCFunctionDeclaration* FindFunction(
		const uni::Nnode* node) const;
	const FrontendCTypedef* FindTypedef(const char* name,
		const uni::Nnode* use = nullptr) const;
	stduint FindRecord(const char* tag) const;
	const FrontendCRecord* GetRecord(stduint record_id) const;
	FrontendCRecord* GetRecord(stduint record_id);
	stduint FindEnum(const char* tag, const uni::Nnode* use = nullptr) const;
	stduint FindEnumInScope(const char* tag, const uni::Nnode* scope) const;
	const FrontendCEnum* GetEnum(stduint enum_id) const;
	FrontendCEnum* GetEnum(stduint enum_id);
	const FrontendCEnumerator* FindEnumerator(
		const char* name, const uni::Nnode* use) const;
	stduint SizeBits(const FrontendCType& type,
		const FrontendCTargetInfo& target) const;
	stduint AlignBits(const FrontendCType& type,
		const FrontendCTargetInfo& target) const;
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

	// Parse one C99 integer constant and select its target-dependent C type.
	bool ParseIntegerLiteral(const char* text, const FrontendCTargetInfo& target,
		stdsint& value, FrontendCType& type);
	bool ParseCharacterLiteral(const char* text, const FrontendCTargetInfo& target,
		stdsint& value);
	bool EvaluateIntegerConstant(uni::Nnode* node,
		const FrontendCTargetInfo& target, const FrontendCTypeInfo& type_info,
		stdsint& value);

	// Append each C operator stage only when its operands are ready to fold.
	void ListPostfixOperators(uni::Chain& operators);
	void ListPrefixOperators(uni::Chain& operators);
	void ListBinaryOperators(uni::Chain& operators);
	void ListTrailingOperators(uni::Chain& operators);
	// Distinguish parenthesized type names from ordinary grouped expressions.
	bool ParseCastOperators(uni::Nchain& syntax_tree,
		const FrontendCDiagnostics& diagnostics, const FrontendCTargetInfo& target,
		FrontendCTypeInfo& type_info);
	bool ParseConditionalOperators(uni::Nchain& syntax_tree,
		const FrontendCDiagnostics& diagnostics);
	bool ParseCallExpressions(uni::Nchain& syntax_tree);
	bool FinalizeEnums(uni::Nchain& syntax_tree,
		const FrontendCDiagnostics& diagnostics, const FrontendCTargetInfo& target,
		FrontendCTypeInfo& type_info);
	// Parse one complete C type-name without modifying its syntax nodes.
	bool ParseTypeName(const uni::Nnode* node, FrontendCType& type,
		const FrontendCTargetInfo& target, const FrontendCTypeInfo& type_info,
		const uni::Nnode* end = nullptr);
	// Parse one normalized declarator using function suffixes prepared earlier.
	bool ParseDeclarator(uni::Nnode* node, FrontendCType& type,
		uni::Nnode*& identifier, const FrontendCTargetInfo& target,
		const FrontendCTypeInfo& type_info, const uni::Nnode* end = nullptr);
	// Prepare function suffixes while parsing one declaration for the first time.
	bool PrepareDeclarator(uni::Nchain& syntax_tree, uni::Nnode* node,
		FrontendCType& type, uni::Nnode*& identifier,
		const FrontendCDiagnostics& diagnostics,
		const FrontendCTargetInfo& target, FrontendCTypeInfo& type_info,
		bool require_function_parameter_names = false,
		const uni::Nnode* end = nullptr);
	// Normalize one function suffix parameter list and return its semantic signature.
	bool PrepareFunctionType(uni::Nchain& syntax_tree, uni::Nnode*& parameters,
		const FrontendCDiagnostics& diagnostics,
		const FrontendCTargetInfo& target, FrontendCTypeInfo& type_info,
		FrontendCFunctionType& function_type, bool require_names);
	// Isolate C99 designators before generic operator parsing handles their values.
	bool PrepareInitializer(uni::Nchain& syntax_tree, uni::Nnode* initializer,
		const FrontendCDiagnostics& diagnostics);

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
