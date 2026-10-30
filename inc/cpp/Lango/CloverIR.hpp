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
	I1,  // One-bit condition value.
	I8,  // Eight-bit integer value.
	I16, // Sixteen-bit integer value.
	I32, // Thirty-two-bit integer value.
	I64, // Signed or unsigned 64-bit integer value.
	Ptr, // Target-independent address value.
};

// Operations represented by the in-memory Clover IR.
enum class Opcode {
	Const, // Create an integer constant.
	Local, // Allocate a function-local storage object.
	Load,  // Read a value from a Ptr operand.
	Store, // Write a value to a Ptr operand.
	Mov,   // Copy an SSA value without accessing memory.
	Add,   // Integer addition.
	Sub,   // Integer subtraction.
	Mul,   // Integer multiplication.
	SDiv,  // Signed integer division.
	SRem,  // Signed integer remainder.
	And,   // Bitwise AND.
	Or,    // Bitwise OR.
	Xor,   // Bitwise XOR.
	Shl,   // Logical left shift.
	AShr,  // Arithmetic right shift.
	PtrAdd, // Add an integer index scaled by the pointee type to a pointer.
	ICmp,  // Integer comparison producing I1.
	ZExt,  // Zero-extension to a wider integer type.
	Jmp,   // Unconditional or condition-selected basic-block transfer.
	Ret,   // Return from the current function.
};

// Predicate carried by an ICmp instruction.
enum class ICmpPredicate {
	EQ,  // Equal.
	NE,  // Not equal.
	SLT, // Signed less than.
	SLE, // Signed less than or equal.
	SGT, // Signed greater than.
	SGE, // Signed greater than or equal.
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
	uni::String name;               // Optional source name retained by Local.
	uni::String debug_source;       // Optional source statement printed before this instruction.
	uni::String target;             // Jump target, or true target for a conditional jump.
	uni::String target_false;       // False target for a conditional jump.
	stdsint immediate = 0;          // Integer literal retained by Const.
	ICmpPredicate predicate = ICmpPredicate::EQ; // Comparison mode for ICmp.
};

// A named straight-line instruction sequence.
struct BasicBlock {
	uni::String name;
	uni::Vector<Instruction> instructions;
};

// Function-level Clover IR container.
struct Function {
	uni::String name;
	Type return_type = Type::I64;
	uni::Vector<BasicBlock> blocks;
};

// Compilation unit exchanged between a Clover frontend and backend.
struct Module {
	uni::Vector<Function> functions;
};

// Parse the textual Clover IR representation from input.
bool Parse(uni::IstreamTrait& input, Module& module);

// Serialize an in-memory Clover module using the textual Clover syntax.
bool Print(const Module& module, uni::OstreamTrait& output);

}

#endif
