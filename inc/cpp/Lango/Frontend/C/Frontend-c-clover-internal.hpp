// ASCII C++11 TAB4 LF
// Docutitle: Lango C Frontend Clover Lowering Internals
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

#ifndef UNISYM_LANGO_FRONTEND_C_CLOVER_INTERNAL_HPP
#define UNISYM_LANGO_FRONTEND_C_CLOVER_INTERNAL_HPP

#include "Frontend-c-internal.hpp"

namespace clover::frontend_c_internal {

using namespace uni::lango;

// Result returned while recursively lowering one AST expression.
struct LoweredValue {
	Value value;
	bool valid = false;
	FrontendCType type;
	bool is_lvalue = false;
	stduint alignment = 0; // Known byte alignment of an lvalue or pointer value.
};

// Associates a source identifier with the Ptr value produced by Local.
struct LocalValue {
	uni::String name;
	Value address;
	FrontendCType type;
	stduint alignment = 0;
	const uni::Nnode* declaration = nullptr;
	const uni::Nnode* scope = nullptr;
	bool active = false;
};

// One C statement atom after transparent tok_statement wrappers are removed.
struct StatementAtom {
	uni::Nnode* node = nullptr;
	const uni::Nnode* source = nullptr;
};

// Function declaration visible at the current C translation-unit position.
struct FrontendCFunction {
	uni::String name;
	FrontendCType return_type;
	uni::Vector<FrontendCType> parameter_types;
};

// Module-scope object visible at the current translation-unit position.
struct FrontendCGlobal {
	uni::String name;
	FrontendCType type;
	stduint alignment = 0;
};

// One block-scope enumerator activated in source order while lowering a function.
struct FrontendCLocalConstant {
	const FrontendCEnumerator* constant = nullptr;
	const uni::Nnode* scope = nullptr;
	bool active = false;
};

stduint GreatestCommonDivisor(stduint left, stduint right);
bool ParseIntegerLiteral(const char* text, stdsint& value);
bool ParseIntegerLiteral(const char* text, const FrontendCTargetInfo& target,
	stdsint& value, FrontendCType& type);
bool ParseCharacterLiteral(const char* text, const FrontendCTargetInfo& target,
	stdsint& value);
bool DecodeCString(const char* text, uni::Vector<stdsint>& bytes);
Type CloverTypeFor(const FrontendCType& type, const FrontendCTargetInfo& target);

// Lowers one C function body into one Clover function.
class FunctionBuilder {
private:
	Module* module = nullptr;
	Function* function = nullptr;
	FrontendCType return_type;
	stduint current_block = 0;
	unsigned next_block = 0;
	unsigned next_value = 0;
	uni::Vector<LocalValue> locals;
	uni::Vector<FrontendCLocalConstant> local_constants;
	uni::Vector<const uni::Nnode*> emitted_sources;
	const uni::Vector<FrontendCFunction>* visible_functions = nullptr;
	const uni::Vector<FrontendCGlobal>* visible_globals = nullptr;
	const uni::Vector<FrontendCEnumerator>* visible_constants = nullptr;
	const FrontendCTargetInfo* target = nullptr;
	const FrontendCTypeInfo* type_info = nullptr;
	const FrontendCDebugInfo* debug_info = nullptr;
	const FrontendCDiagnostics* diagnostics = nullptr;
	unsigned* next_string_literal = nullptr;
	uint32 debug_file_id = 0;
	bool failed = false;

	// Clover block construction, debug attachment, and primitive IR emission.
	BasicBlock& CurrentBlock();
	BasicBlock& EntryBlock();
	bool IsKeyword(const uni::Nnode* node, const char* keyword) const;
	void AppendUnsigned(uni::String& text, unsigned value);
	uni::String NewBlockName(const char* prefix);
	stduint AppendBlock(const uni::String& name);
	stduint NewBlock(const char* prefix);
	bool BlockTerminated() const;
	void EmitJump(stduint target);
	void EmitJump(const uni::String& target);
	void EmitJump(Value condition, stduint target_true, stduint target_false);
	void EmitJump(Value condition, const uni::String& target_true,
		const uni::String& target_false);
	bool SourceEmitted(const uni::Nnode* statement) const;
	void AttachSource(Instruction& instruction, const uni::Nnode* statement);
	void AttachSource(stduint instruction_index, const uni::Nnode* statement);
	Value NewValue(Type type);
	Value EmitResult(Opcode opcode, Type type, Value operand);
	Value EmitResult(Opcode opcode, Type type, Value left, Value right);
	Value EmitPtrAdd(Value pointer, Value index, const FrontendCType& pointer_type);
	Value EmitByteOffset(Value pointer, stduint offset);
	Value EmitLoad(Type type, Value address, stduint alignment);
	void EmitStore(Value value, Value address, stduint alignment);

	// Target-aware C type queries, integer conversions, and common-type rules.
	unsigned IntegerBits(const FrontendCType& type) const;
	Type CloverType(const FrontendCType& type) const;
	stduint TypeAlignment(const FrontendCType& type) const;
	stduint TypeSize(const FrontendCType& type) const;
	stduint OffsetAlignment(stduint base_alignment, stduint offset) const;
	stduint PointerStepAlignment(stduint pointer_alignment,
		const FrontendCType& pointer_type) const;
	FrontendCType IntType() const;
	FrontendCType SizeType() const;
	FrontendCType PointerIntegerType(bool is_signed) const;
	Value EmitConversion(Opcode opcode, Type destination, Value operand);
	LoweredValue ConvertInteger(LoweredValue lowered,
		const FrontendCType& destination, const uni::Nnode* node);
	FrontendCType PromoteType(FrontendCType type) const;
	FrontendCType CommonIntegerType(FrontendCType left,
		FrontendCType right) const;
	bool CommonConditionalType(const FrontendCType& true_type,
		const FrontendCType& false_type, FrontendCType& result) const;

	// Value categories, temporary storage, and visible symbol lookup.
	LoweredValue ToRValue(LoweredValue lowered, const uni::Nnode* node);
	Value AsCondition(LoweredValue lowered, const uni::Nnode* node);
	Value EmitConst(stdsint immediate,
		const FrontendCType& type = FrontendCType());
	Value EmitTypedConst(Type type, stdsint immediate);
	LoweredValue CreateTemporaryStorage(const FrontendCType& type);
	LocalValue* FindLocal(const char* name);
	const FrontendCFunction* FindFunction(const char* name) const;
	const FrontendCGlobal* FindGlobal(const char* name) const;
	const FrontendCEnumerator* FindConstant(const char* name) const;
	bool ActivateEnum(const uni::Nnode* declaration, const uni::Nnode* scope);
	Value EmitGlobalAddress(const uni::String& name);
	Value EmitGlobalAddress(const FrontendCGlobal& global);
	LoweredValue LowerStringLiteral(uni::Nnode* node);

	// Local declarator collection, activation, initialization, and scope exit.
	bool PredeclareLocal(const char* name, const FrontendCType& type,
		const uni::Nnode* declaration, const uni::Nnode* scope);
	bool ParseDeclaratorNode(uni::Nnode* node, uni::Nnode*& identifier,
		FrontendCType& type);
	bool ParseDeclarator(uni::Nnode* node, uni::Nnode*& identifier,
		FrontendCType& type, const FrontendCType& base_type);
	bool PredeclareDeclarator(uni::Nnode* node, const uni::Nnode* declaration,
		const uni::Nnode* scope, const FrontendCType& base_type);
	bool PredeclareBlock(uni::Nnode* block);
	LocalValue* ActivateLocal(const char* name, const uni::Nnode* declaration);
	void AppendInitializerValues(uni::Nnode* node,
		uni::Vector<uni::Nnode*>& values);
	bool InitializeArray(LocalValue& local, uni::Nnode* initializer);
	bool LowerDeclarator(uni::Nnode* node, const uni::Nnode* declaration);
	void LeaveScope(const uni::Nnode* block);

	// Recursive expression lowering and expression-only type resolution.
	LoweredValue Error(const char* message, const uni::Nnode* node);
	void AppendCallArguments(uni::Nnode* node,
		uni::Vector<uni::Nnode*>& arguments);
	LoweredValue LowerCall(uni::Nnode* node);
	LoweredValue LowerSubscriptOperands(LoweredValue left,
		uni::Nnode* left_node, uni::Nnode* right_node, uni::Nnode* node);
	LoweredValue LowerSubscript(uni::Nnode* node);
	const FrontendCRecordMember* FindRecordMember(
		const FrontendCType& type, const char* name) const;
	FrontendCType MemberType(const FrontendCType& record_type,
		const FrontendCRecordMember& member) const;
	LoweredValue LowerRecordMember(Value record_address,
		const FrontendCType& record_type, stduint record_alignment,
		uni::Nnode* designator);
	LoweredValue LowerMember(uni::Nnode* node);
	LoweredValue LowerPointerMember(uni::Nnode* node);
	bool ResolveRecordMemberType(const FrontendCType& record_type,
		uni::Nnode* designator, FrontendCType& type);
	LoweredValue LowerIncrement(uni::Nnode* operand, bool increment,
		bool return_updated);
	bool ResolveExpressionType(uni::Nnode* node, FrontendCType& type);
	stdsint SizeOf(const FrontendCType& type) const;
	LoweredValue LowerSizeof(uni::Nnode* operand);
	LoweredValue LowerCast(uni::Nnode* node, uni::Nnode* operand);
	bool IsCompoundAssignment(const char* operation) const;
	LoweredValue LowerCompoundAssignment(uni::Nnode* node,
		const char* operation, uni::Nnode* left_node, uni::Nnode* right_node);
	LoweredValue LowerLogical(uni::Nnode* node, bool is_and,
		uni::Nnode* left_node, uni::Nnode* right_node);
	LoweredValue ConvertConditionalBranch(LoweredValue branch,
		const FrontendCType& result_type, uni::Nnode* branch_node);
	LoweredValue LowerConditional(uni::Nnode* node, uni::Nnode* condition_node,
		uni::Nnode* true_node, uni::Nnode* false_node);
	LoweredValue LowerOperator(uni::Nnode* node);
	LoweredValue LowerExpression(uni::Nnode* node);

	// Structured statement and function-body lowering.
	bool EmitReturnExpression(uni::Nnode* expression);
	bool BeginIf(uni::Nnode* condition_node, const uni::Nnode* source_statement,
		stduint& then_block, stduint& else_block, uni::String& end_block);
	void EndIfBranch(const uni::String& end_block);
	void AppendStatementAtoms(uni::Nnode* node,
		uni::Vector<StatementAtom>& atoms);
	uni::Nnode* ForClause(uni::Nnode* clause);
	bool LowerWhile(uni::Nnode* node, const uni::Nnode* source_statement,
		uni::Vector<StatementAtom>& atoms, stduint& index);
	bool LowerFor(uni::Nnode* node, const uni::Nnode* source_statement,
		uni::Vector<StatementAtom>& atoms, stduint& index);
	bool LowerStatement(uni::Vector<StatementAtom>& atoms, stduint& index);
	bool LowerBlock(uni::Nnode* block);
	bool InitializeParameters(uni::Nnode* parameter_node,
		const uni::Nnode* body);

public:
	FunctionBuilder(Module& output_module, Function& function,
		const uni::Vector<FrontendCFunction>& functions,
		const uni::Vector<FrontendCGlobal>& globals,
		const uni::Vector<FrontendCEnumerator>& constants,
		const FrontendCType& function_return_type,
		const FrontendCTargetInfo& target_info,
		const FrontendCTypeInfo& semantic_types,
		const FrontendCDiagnostics& diagnostic_output,
		const FrontendCDebugInfo* debug_information,
		unsigned& string_literal_index, uint32 source_file_id);
	bool Lower(uni::Nnode* body, uni::Nnode* parameters);
};

}

#endif
