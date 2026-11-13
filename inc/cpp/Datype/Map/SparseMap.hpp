// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: Sparse Integer Map
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
/*
	Copyright 2023 ArinaMgk

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
// Upgraded from `strpage`

#ifndef _INCPP_DATYPE_MAP_SPARSEMAP
#define _INCPP_DATYPE_MAP_SPARSEMAP

#include "../../unisym"

namespace uni {

	class SparseMap {
	public:
		using Key = stduint;
		using Value = stduint;

		SparseMap();
		~SparseMap();

		SparseMap(const SparseMap&) = delete;
		SparseMap& operator=(const SparseMap&) = delete;

		SparseMap(SparseMap&& other) noexcept;
		SparseMap& operator=(SparseMap&& other) noexcept;

		bool Set(Key key, Value value);
		bool Get(Key key, Value& value) const;
		Value Get(Key key) const;

		Value* Find(Key key);
		const Value* Find(Key key) const;
		bool Contains(Key key) const;
		bool Remove(Key key);

		void Clear();
		stduint Count() const { return count; }
		bool IsEmpty() const { return count == 0; }

	private:
		struct Node;

		Node* root;
		stduint count;

		static Node* AllocateNode(byte level);
		static Node* AllocatePath(byte level, Key key);
		static void FreeNode(Node* node);
		static byte RequiredLevel(Key key);
		static stduint IndexAt(Key key, byte level);
		static bool IsOccupied(const Node* node, stduint index);
		static void SetOccupied(Node* node, stduint index, bool occupied);

		const Node* FindLeaf(Key key) const;
		void ReduceRoot();
	};

}

#endif
