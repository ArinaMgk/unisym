// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: Sparse Integer Map
// Codifiers: @dosconio: 20261113 ~ <Last-check>
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

#include "../../../../inc/cpp/Datype/Map/SparseMap.hpp"

namespace uni {

	static constexpr byte SparseMapRadixBits = 8;
	static constexpr stduint SparseMapRadixSize = _IMM1 << SparseMapRadixBits;
	static constexpr stduint SparseMapRadixMask = SparseMapRadixSize - 1;
	static constexpr stduint SparseMapOccupancyBits = bitsof(stduint);
	static constexpr stduint SparseMapOccupancyWords =
		(SparseMapRadixSize + SparseMapOccupancyBits - 1) /
		SparseMapOccupancyBits;

	struct SparseMap::Node {
		struct Leaf {
			Value values[SparseMapRadixSize];
			stduint occupied[SparseMapOccupancyWords];
		};

		byte level;
		stduint used;
		union {
			Node* branches[SparseMapRadixSize];
			Leaf leaf;
		};
	};

	SparseMap::SparseMap()
		: root(nullptr), count(0) {
	}

	SparseMap::~SparseMap() {
		Clear();
	}

	SparseMap::SparseMap(SparseMap&& other) noexcept
		: root(other.root), count(other.count) {
		other.root = nullptr;
		other.count = 0;
	}

	SparseMap& SparseMap::operator=(SparseMap&& other) noexcept {
		if (this == &other) return *this;
		Clear();
		root = other.root;
		count = other.count;
		other.root = nullptr;
		other.count = 0;
		return *this;
	}

	SparseMap::Node* SparseMap::AllocateNode(byte level) {
		Node* node = malcof(Node);
		if (node) {
			byte* content = (byte*)node;
			for (stduint index = 0; index < sizeof(Node); ++index)
				content[index] = 0;
			node->level = level;
		}
		return node;
	}

	SparseMap::Node* SparseMap::AllocatePath(byte level, Key key) {
		Node* node = AllocateNode(level);
		if (!node || !level) return node;

		Node* child = AllocatePath(level - 1, key);
		if (!child) {
			memfree(node);
			return nullptr;
		}

		node->branches[IndexAt(key, level)] = child;
		node->used = 1;
		return node;
	}

	void SparseMap::FreeNode(Node* node) {
		if (!node) return;
		if (node->level) {
			for (stduint index = 0; index < SparseMapRadixSize; ++index) {
				FreeNode(node->branches[index]);
			}
		}
		memfree(node);
	}

	byte SparseMap::RequiredLevel(Key key) {
		byte level = 0;
		while (key >>= SparseMapRadixBits) ++level;
		return level;
	}

	stduint SparseMap::IndexAt(Key key, byte level) {
		return (key >> (level * SparseMapRadixBits)) & SparseMapRadixMask;
	}

	bool SparseMap::IsOccupied(const Node* node, stduint index) {
		const stduint word = index / SparseMapOccupancyBits;
		const stduint bit = index % SparseMapOccupancyBits;
		return (node->leaf.occupied[word] & (_IMM1 << bit)) != 0;
	}

	void SparseMap::SetOccupied(Node* node, stduint index, bool occupied) {
		const stduint word = index / SparseMapOccupancyBits;
		const stduint bit = index % SparseMapOccupancyBits;
		const stduint mask = _IMM1 << bit;
		if (occupied) node->leaf.occupied[word] |= mask;
		else node->leaf.occupied[word] &= ~mask;
	}

	const SparseMap::Node* SparseMap::FindLeaf(Key key) const {
		if (!root || RequiredLevel(key) > root->level) return nullptr;

		const Node* node = root;
		while (node->level) {
			node = node->branches[IndexAt(key, node->level)];
			if (!node) return nullptr;
		}
		return node;
	}

	bool SparseMap::Set(Key key, Value value) {
		const byte required_level = RequiredLevel(key);
		if (!root) {
			root = AllocatePath(required_level, key);
			if (!root) return false;
		}

		while (root->level < required_level) {
			Node* replacement = AllocateNode(root->level + 1);
			if (!replacement) return false;
			replacement->branches[0] = root;
			replacement->used = 1;
			root = replacement;
		}

		Node* node = root;
		while (node->level) {
			const stduint index = IndexAt(key, node->level);
			Node* child = node->branches[index];
			if (!child) {
				child = AllocatePath(node->level - 1, key);
				if (!child) return false;
				node->branches[index] = child;
				++node->used;
			}
			node = child;
		}

		const stduint index = IndexAt(key, 0);
		if (!IsOccupied(node, index)) {
			SetOccupied(node, index, true);
			++node->used;
			++count;
		}
		node->leaf.values[index] = value;
		return true;
	}

	bool SparseMap::Get(Key key, Value& value) const {
		const Value* found = Find(key);
		if (!found) return false;
		value = *found;
		return true;
	}

	SparseMap::Value SparseMap::Get(Key key) const {
		const Value* found = Find(key);
		return found ? *found : 0;
	}

	SparseMap::Value* SparseMap::Find(Key key) {
		return const_cast<Value*>(static_cast<const SparseMap*>(this)->Find(key));
	}

	const SparseMap::Value* SparseMap::Find(Key key) const {
		const Node* leaf = FindLeaf(key);
		const stduint index = IndexAt(key, 0);
		if (!leaf || !IsOccupied(leaf, index)) return nullptr;
		return &leaf->leaf.values[index];
	}

	bool SparseMap::Contains(Key key) const {
		return Find(key) != nullptr;
	}

	void SparseMap::ReduceRoot() {
		while (root && root->level && root->used == 1 && root->branches[0]) {
			Node* replacement = root->branches[0];
			root->branches[0] = nullptr;
			memfree(root);
			root = replacement;
		}
	}

	bool SparseMap::Remove(Key key) {
		if (!root || RequiredLevel(key) > root->level) return false;

		Node* parents[sizeof(Key)] = {};
		byte indexes[sizeof(Key)] = {};
		stduint depth = 0;
		Node* node = root;

		while (node->level) {
			const stduint index = IndexAt(key, node->level);
			parents[depth] = node;
			indexes[depth] = (byte)index;
			++depth;
			node = node->branches[index];
			if (!node) return false;
		}

		const stduint index = IndexAt(key, 0);
		if (!IsOccupied(node, index)) return false;

		SetOccupied(node, index, false);
		node->leaf.values[index] = 0;
		--node->used;
		--count;
		if (node->used) return true;

		while (depth) {
			Node* parent = parents[depth - 1];
			const stduint branch_index = indexes[depth - 1];
			memfree(node);
			parent->branches[branch_index] = nullptr;
			--parent->used;
			node = parent;
			--depth;
			if (node->used) {
				ReduceRoot();
				return true;
			}
		}

		memfree(node);
		root = nullptr;
		return true;
	}

	void SparseMap::Clear() {
		FreeNode(root);
		root = nullptr;
		count = 0;
	}

}
