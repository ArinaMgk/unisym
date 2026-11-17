// ASCII C++11 TAB4 LF
// Docutitle: Clover Intermediate Representation Passes
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

#include "../../../inc/cpp/Lango/CloverPass.hpp"

namespace clover {

	namespace {

		bool SameName(const uni::String& left, const uni::String& right) {
			return !StrCompare(left.reference(), right.reference());
		}

		bool HasBlock(const Function& function, const uni::String& name) {
			for (const BasicBlock& block : function.blocks)
				if (SameName(block.name, name)) return true;
			return false;
		}

		void RewriteJumpTargets(Function& function, const uni::String& removed_name,
			const uni::String& target_name) {
			for (BasicBlock& block : function.blocks) {
				for (Instruction& instruction : block.instructions) {
					if (instruction.opcode != Opcode::Jmp) continue;
					if (SameName(instruction.target, removed_name))
						instruction.target = target_name;
					if (instruction.target_false.getByteCount() &&
						SameName(instruction.target_false, removed_name))
						instruction.target_false = target_name;
				}
			}
		}

		bool ForwardJumpBlocks(Function& function, stduint& changes) {
			stduint index = 1;
			while (index < function.blocks.Count()) {
				BasicBlock& block = function.blocks[index];
				if (block.instructions.Count() != 1) {
					index++;
					continue;
				}
				const Instruction& jump = block.instructions[0];
				if (jump.opcode != Opcode::Jmp || jump.operands.Count() ||
					jump.target_false.getByteCount() || !jump.target.getByteCount() ||
					SameName(block.name, jump.target) || !HasBlock(function, jump.target)) {
					index++;
					continue;
				}
				uni::String removed_name = block.name;
				uni::String target_name = jump.target;
				if (!function.blocks.Remove(index)) return false;
				RewriteJumpTargets(function, removed_name, target_name);
				changes++;
			}
			return true;
		}

	}

	PassResult ForwardJumpBlocks(Module& module) {
		PassResult result;
		if (module.ir_class != IRClass::Soft) {
			result.success = false;
			return result;
		}
		for (Function& function : module.functions) {
			if (!ForwardJumpBlocks(function, result.changes)) {
				result.success = false;
				return result;
			}
		}
		return result;
	}

}
