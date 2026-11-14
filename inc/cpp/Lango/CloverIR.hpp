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

// Link visibility retained independently from mutability and storage section.
enum class Linkage {
	External, // Symbol may be referenced by another translation unit.
	Internal  // Symbol is private to this Clover module.
};

// Operations represented by the in-memory Clover IR.
enum class Opcode {
	Const, // Create an integer constant.
	FConst, // Create a floating-point constant from its exact IEEE bit pattern.
	Local, // Allocate a function-local storage object.
	Load,  // Read a value from a Ptr operand.
	Store, // Write a value to a Ptr operand.
	MemCopy, // Copy a fixed byte count between non-overlapping Ptr operands.
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

// One Clover IR instruction. Fields unused by an opcode retain defaults.
struct Instruction {
	Opcode opcode = Opcode::Const;
	Type type = Type::I64;          // Result type, or accessed integer type.
	bool has_result = false;        // Whether this instruction defines result.
	Value result;                   // SSA result when has_result is true.
	uni::Vector<Value> operands;    // Ordered input values.
	uni::String name;               // Local source name or referenced global symbol.
	uni::String debug_source;       // Optional source statement printed before this instruction.
	uint32 debug_file = 0;          // One-based source file identifier; zero means unavailable.
	uint32 debug_line = 0;          // One-based source line; zero means unavailable.
	uni::String callee;             // Named function target used by Call.
	Value callee_value;             // Pointer target used by an indirect Call.
	bool has_callee_value = false;  // Select callee_value instead of the named callee.
	uni::String target;             // Jump target, or true target for a conditional jump.
	uni::String target_false;       // False target for a conditional jump.
	stdsint immediate = 0;          // Integer literal, float bits, or Local element count.
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
	Type type = Type::I8;
	stdsint initializer = 0;
	stdsint initializer_high = 0; // High 64 bits for an f128 initializer.
	uni::String initializer_symbol; // Optional relocated address initializer for Ptr.
	stdsint initializer_addend = 0; // Signed byte displacement from initializer_symbol.
	stdsint element_count = 1;
	stduint alignment = 0; // Required object alignment in bytes; zero selects natural alignment.
	bool is_declaration = false; // Refer to externally provided storage without defining it.
	bool is_read_only = false; // Emit the object in target read-only storage.
	Linkage linkage = Linkage::External;
	uni::Vector<stdsint> initializer_values; // Explicit elements for aggregate data.
	uni::Vector<GlobalInitializerFragment> initializer_fragments; // Heterogeneous aggregate data.
};

// Function-level Clover IR container.
struct Function {
	uni::String name;
	Type return_type = Type::I64;
	uni::Vector<Parameter> parameters;
	bool is_variadic = false; // Accept call operands after the fixed parameter list.
	bool is_declaration = false; // Refer to a function supplied by this or another module.
	Linkage linkage = Linkage::External;
	uni::Vector<BasicBlock> blocks;
};

// Compilation unit exchanged between a Clover frontend and backend.
struct Module {
	unsigned pointer_bits = 0; // Address width required by Ptr; zero means unspecified.
	uni::Vector<SourceFile> source_files;
	uni::Vector<Global> globals;
	uni::Vector<Function> functions;
};

// Parse the textual Clover IR representation from input.
bool Parse(uni::IstreamTrait& input, Module& module);

// Serialize an in-memory Clover module using the textual Clover syntax.
bool Print(const Module& module, uni::OstreamTrait& output);

}

#endif
