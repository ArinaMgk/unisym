// ASCII C++11 TAB4 LF
// Docutitle: Clover Intermediate Representation
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

#ifndef UNISYM_LANGO_CLOVER_IR_HPP
#define UNISYM_LANGO_CLOVER_IR_HPP

#include "../string"
#include "../vector"
#include "../trait/XstreamTrait.hpp"

namespace clover {

enum class IRClass {
	Soft,
	Hard,
	Exec
};

// Scalar and address types currently represented by Clover IR.
enum class Type {
	Void, // No value; valid for function returns and side-effecting calls.
	I1,  // One-bit condition value.
	I8,  // Eight-bit integer value.
	I16, // Sixteen-bit integer value.
	I32, // Thirty-two-bit integer value.
	I64, // Signed or unsigned 64-bit integer value.
	F32, // IEEE 754 binary32 floating-point value.
	F64, // IEEE 754 binary64 floating-point value.
	F128, // IEEE 754 binary128 floating-point value.
	Ptr, // Target-independent address value.
};

// A scalar, exact-width bit vector, named type, or array type used for objects.
struct TypeReference {
	static constexpr unsigned MaxArrayDepth = 32;

	Type scalar = Type::Void; // Built-in scalar type when this is not Bits or Defined.
	stduint bit_width = 0; // Exact width for bN; zero when this is not a bit vector.
	uni::String defined_name; // Named type without the leading @.
	unsigned array_depth = 0; // Number of C-style array suffixes in array_counts.
	stduint array_counts[MaxArrayDepth] = {}; // Outer-to-inner element counts.

	TypeReference() = default;
	TypeReference(Type type) : scalar(type) {}
	TypeReference& operator=(Type type) {
		scalar = type;
		bit_width = 0;
		defined_name.Clear();
		array_depth = 0;
		return *this;
	}
	bool IsBits() const { return bit_width != 0; }
	bool IsDefined() const { return defined_name.getByteCount() != 0; }
	bool IsArray() const { return array_depth != 0; }
	bool IsScalar() const { return !IsBits() && !IsDefined() && !IsArray(); }
	bool operator==(Type type) const {
		return !IsBits() && !IsDefined() && !IsArray() && scalar == type;
	}
	bool operator!=(Type type) const { return !(*this == type); }
};

enum class TypeDefinitionKind {
	Structure, // Members occupy consecutive bits in textual order.
	Union // Every member begins at bit zero.
};

// One named or anonymous member of a struct or union definition.
struct TypeMember {
	uni::String name; // Empty for an anonymous layout member.
	TypeReference type;
	stduint offset_bits = 0; // Computed from the explicit member sequence.
};

// One named aggregate type shared by Soft, Hard, and Exec IR.
struct TypeDefinition {
	uni::String name; // Type name without the leading @.
	TypeDefinitionKind kind = TypeDefinitionKind::Structure;
	stduint size_bits = 0; // Explicit total size; Clover never inserts padding.
	stduint alignment_bits = 0; // Zero when the type has no memory alignment.
	uni::Vector<TypeMember> members; // Textual member order defines bit placement.
};

// Link visibility retained independently from mutability and storage section.
enum class Linkage {
	External, // Symbol may be referenced by another translation unit.
	Internal  // Symbol is private to this Clover module.
};

enum class RelocationModel {
	Static, // Default relocation model.
	PIC, // Position-independent code for shared-library use.
	PIE // Position-independent executable code.
};

enum class SymbolVisibility {
	Default, // Visible to dynamic symbol resolution.
	Hidden, // Not visible outside the defining module.
	Protected // Externally visible but locally bound within the module.
};

enum class SymbolBinding {
	Strong, // Regular symbol definition or reference.
	Weak // Weak symbol definition or reference.
};

// Operations represented by the in-memory Clover IR.
enum class Opcode {
	Const, // Create an integer constant.
	FConst, // Create a floating-point constant from its exact IEEE bit pattern.
	Local, // Allocate a fixed or runtime-sized function-local storage object.
	StackSave, // Capture the current dynamic local-storage position.
	StackRestore, // Restore a previously captured dynamic local-storage position.
	Load,  // Read a value from a Ptr operand.
	Store, // Write a value to a Ptr operand.
	MemCopy, // Copy fixed bytes; exact source and destination aliasing is allowed.
	Mov,   // Copy an SSA value without accessing memory.
	Add,   // Integer addition.
	Sub,   // Integer subtraction.
	Mul,   // Integer multiplication.
	SDiv,  // Signed integer division.
	UDiv,  // Unsigned integer division.
	SRem,  // Signed integer remainder.
	URem,  // Unsigned integer remainder.
	And,   // Bitwise AND.
	Or,    // Bitwise OR.
	Xor,   // Bitwise XOR.
	Shl,   // Logical left shift.
	AShr,  // Arithmetic right shift.
	LShr,  // Logical right shift.
	PtrAdd, // Add an integer index scaled by the pointee type to a pointer.
	ICmp,  // Integer comparison producing I1.
	FAdd,  // Floating-point addition.
	FSub,  // Floating-point subtraction.
	FMul,  // Floating-point multiplication.
	FDiv,  // Floating-point division.
	FCmp,  // Floating-point comparison producing I1.
	ZExt,  // Zero-extension to a wider integer type.
	SExt,  // Sign-extension to a wider integer type.
	Trunc, // Truncate an integer to a narrower integer type.
	IntToPtr, // Convert an integer bit pattern to a pointer value.
	PtrToInt, // Convert a pointer bit pattern to an integer value.
	SIToFP, // Convert a signed integer to floating point.
	UIToFP, // Convert an unsigned integer to floating point.
	FPToSI, // Convert floating point to a signed integer, rounding toward zero.
	FPToUI, // Convert floating point to an unsigned integer, rounding toward zero.
	FPExt, // Extend f32 to f64.
	FPTrunc, // Round f64 to f32.
	GlobalAddr, // Produce the address of a named module global object.
	FunctionAddr, // Produce the address of a named module function.
	Call,  // Call a named function or function pointer and produce its return value.
	Jmp,   // Unconditional or condition-selected basic-block transfer.
	Ret,   // Return from the current function.
};

// C-compatible floating comparison carried by an FCmp instruction.
enum class FCmpPredicate {
	EQ, // Equal; false when either operand is NaN.
	NE, // Not equal; true when either operand is NaN.
	LT, // Less than; false when either operand is NaN.
	LE, // Less than or equal; false when either operand is NaN.
	GT, // Greater than; false when either operand is NaN.
	GE  // Greater than or equal; false when either operand is NaN.
};

// Predicate carried by an ICmp instruction.
enum class ICmpPredicate {
	EQ,  // Equal.
	NE,  // Not equal.
	SLT, // Signed less than.
	SLE, // Signed less than or equal.
	SGT, // Signed greater than.
	SGE, // Signed greater than or equal.
	ULT, // Unsigned less than.
	ULE, // Unsigned less than or equal.
	UGT, // Unsigned greater than.
	UGE, // Unsigned greater than or equal.
};

// Reference to a typed SSA result. IDs are unique within one function.
struct Value {
	Type type = Type::I64;
	unsigned id = 0;
};

// One function-call argument, either a scalar SSA value or an aggregate address.
struct CallArgument {
	Value value;
	bool by_value = false;
	TypeReference object_type;
};

// One Clover IR instruction. Fields unused by an opcode retain defaults.
struct Instruction {
	Opcode opcode = Opcode::Const;
	Type type = Type::I64;          // Result type, or accessed integer type.
	TypeReference object_type;      // Local or PtrAdd object type when explicitly present.
	bool has_object_type = false;
	bool by_value_return = false;    // Call or Ret carries an aggregate object address.
	bool has_result = false;        // Whether this instruction defines result.
	Value result;                   // SSA result when has_result is true.
	uni::Vector<Value> operands;    // Ordered input values.
	uni::Vector<CallArgument> call_arguments; // Typed call arguments, including by-value objects.
	uni::String name;               // Local source name or referenced global symbol.
	uni::String debug_source;       // Optional source statement printed before this instruction.
	uint32 debug_file = 0;          // One-based source file identifier; zero means unavailable.
	uint32 debug_line = 0;          // One-based source line; zero means unavailable.
	uni::String callee;             // Named function target used by Call.
	Value callee_value;             // Pointer target used by an indirect Call.
	bool has_callee_value = false;  // Select callee_value instead of the named callee.
	uni::String target;             // Jump target, or true target for a conditional jump.
	uni::String target_false;       // False target for a conditional jump.
	stdsint immediate = 0;          // Integer literal, float bits, or fixed Local element count.
	stdsint immediate_high = 0;     // High 64 bits for an f128 constant.
	stduint alignment = 0;          // Optional Local or memory-access alignment in bytes.
	stduint source_alignment = 0;   // Optional source alignment used by MemCopy.
	ICmpPredicate predicate = ICmpPredicate::EQ; // Comparison mode for ICmp.
	FCmpPredicate float_predicate = FCmpPredicate::EQ; // Comparison mode for FCmp.
};

// One source path referenced by instruction debug_file fields.
struct SourceFile {
	uint32 id = 0;
	uni::String path;
};

// A named straight-line instruction sequence.
struct BasicBlock {
	uni::String name;
	uni::Vector<Instruction> instructions;
};

// One typed SSA value supplied by the caller at function entry.
struct Parameter {
	uni::String name;
	Value value;
	unsigned aggregate_group = 0; // Nonzero for a flattened aggregate ABI member.
	bool by_value = false; // Value is an address to a copied object of object_type.
	TypeReference object_type; // Source-level object type for a by-value aggregate.
};

// One typed constant placed at a byte offset within a global object.
struct GlobalInitializerFragment {
	stduint offset = 0; // Byte offset from the beginning of the global object.
	Type type = Type::I8; // Stored scalar type; target byte order is chosen by the backend.
	stdsint initializer = 0;
	stdsint initializer_high = 0; // High 64 bits for an f128 initializer.
	uni::String initializer_symbol; // Optional symbol relocation for a Ptr fragment.
	stdsint initializer_addend = 0; // Signed byte displacement from initializer_symbol.
};

// One module-scope object with static storage duration.
struct Global {
	uni::String name;
	TypeReference type = Type::I8;
	stdsint initializer = 0;
	stdsint initializer_high = 0; // High 64 bits for an f128 initializer.
	uni::String initializer_symbol; // Optional relocated address initializer for Ptr.
	stdsint initializer_addend = 0; // Signed byte displacement from initializer_symbol.
	stdsint element_count = 1;
	stduint alignment = 0; // Required object alignment in bytes; zero selects natural alignment.
	bool is_declaration = false; // Refer to externally provided storage without defining it.
	bool is_read_only = false; // Emit the object in target read-only storage.
	Linkage linkage = Linkage::External;
	SymbolVisibility visibility = SymbolVisibility::Default;
	SymbolBinding binding = SymbolBinding::Strong;
	uni::Vector<stdsint> initializer_values; // Explicit elements for aggregate data.
	uni::Vector<GlobalInitializerFragment> initializer_fragments; // Heterogeneous aggregate data.
};

// Function-level Clover IR container.
struct Function {
	uni::String name;
	Type return_type = Type::I64;
	bool returns_by_value = false; // Return type is the aggregate object_type, not a scalar.
	TypeReference return_object_type;
	uni::Vector<Parameter> parameters;
	bool is_variadic = false; // Accept call operands after the fixed parameter list.
	bool is_declaration = false; // Refer to a function supplied by this or another module.
	Linkage linkage = Linkage::External;
	SymbolVisibility visibility = SymbolVisibility::Default;
	SymbolBinding binding = SymbolBinding::Strong;
	uni::Vector<BasicBlock> blocks;
};

enum class PortDirection {
	Input,
	Output,
	InOut
};

struct HardwarePort {
	uni::String name;
	TypeReference type;
	PortDirection direction = PortDirection::Input;
};

enum class HardwareOpcode {
	Read,
	Const,
	Add,
	Assign
};

struct HardwareValue {
	TypeReference type;
	unsigned id = 0;
};

struct HardwareOperation {
	HardwareOpcode opcode = HardwareOpcode::Const;
	bool has_result = false;
	HardwareValue result;
	uni::Vector<HardwareValue> operands;
	uni::String symbol;
	uint64 immediate = 0;
};

struct HardwareBlock {
	uni::String name;
	uni::Vector<HardwareOperation> operations;
};

struct HardwareModule {
	uni::String name;
	uni::Vector<HardwarePort> ports;
	uni::Vector<HardwareBlock> blocks;
};

// Compilation unit exchanged between a Clover frontend and backend.
struct Module {
	IRClass ir_class = IRClass::Soft;
	unsigned pointer_bits = 0; // Address width required by Ptr; zero means unspecified.
	RelocationModel relocation_model = RelocationModel::Static;
	uni::Vector<SourceFile> source_files;
	uni::Vector<TypeDefinition> type_definitions;
	uni::Vector<Global> globals;
	uni::Vector<Function> functions;
	uni::Vector<HardwareModule> hardware_modules;
};

// Parse the textual Clover IR representation from input.
bool Parse(uni::IstreamTrait& input, Module& module);

// Serialize an in-memory Clover module using the textual Clover syntax.
bool Print(const Module& module, uni::OstreamTrait& output);

const TypeDefinition* FindTypeDefinition(const Module& module, const char* name);
TypeDefinition* FindTypeDefinition(Module& module, const char* name);
// Compute the complete size or natural alignment of one object type in bits.
bool TypeSizeBits(const Module& module, const TypeReference& type, stduint& bits);
bool TypeAlignmentBits(const Module& module, const TypeReference& type, stduint& bits);
// Compare the base kind, named identity, bit width, and every array dimension.
bool SameType(const TypeReference& left, const TypeReference& right);

}

#endif
