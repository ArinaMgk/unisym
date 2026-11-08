// ASCII C++ TAB4 CRLF
// Docutitle: Pool Node System
// Codifiers: @ArinaMgk
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

#include "../../../inc/cpp/pnode"
#include "../../../inc/c/ustring.h"

namespace uni {

	static stduint AlignPnodeSize(stduint size)
	{
		const stduint alignment = alignof(Pnode);
		return (size + alignment - 1) & ~(alignment - 1);
	}

	Pchain::Pchain(stduint extension_size, stduint capacity_per_block,
		_tofree_ft free_function)
	{
		root_block = nullptr;
		last_block = nullptr;
		free_node = nullptr;
		root_node = nullptr;
		last_node = nullptr;
		extn_field = extension_size;
		node_stride = AlignPnodeSize(sizeof(Pnode) + extension_size);
		block_capacity = capacity_per_block ? capacity_per_block : 1;
		node_count = 0;
		acquired_count = 0;
		node_capacity = 0;
		func_free = free_function;
	}

	Pchain::~Pchain()
	{
		Clear();
	}

	bool Pchain::AllocateBlock()
	{
		const stduint block_header = AlignPnodeSize(sizeof(Block));
		const stduint maximum_size = ~(stduint)0;
		if (node_stride > (maximum_size - block_header) / block_capacity)
			return false;

		Block* block = (Block*)malc(block_header + node_stride * block_capacity);
		if (!block)
			return false;

		block->next = nullptr;
		if (last_block)
			last_block->next = block;
		else
			root_block = block;
		last_block = block;

		byte* storage = (byte*)block + block_header;
		for (stduint index = 0; index < block_capacity; index++) {
			Pnode* node = (Pnode*)(storage + index * node_stride);
			node->next = free_node;
			free_node = node;
		}
		node_capacity += block_capacity;
		return true;
	}

	Pnode* Pchain::Acquire()
	{
		if (!free_node && !AllocateBlock())
			return nullptr;

		Pnode* node = free_node;
		free_node = free_node->next;
		MemSet(node, 0, node_stride);
		acquired_count++;
		return node;
	}

	void Pchain::Release(Pnode* node)
	{
		if (!node)
			return;

		MemSet(node, 0, node_stride);
		node->next = free_node;
		free_node = node;
		if (acquired_count)
			acquired_count--;
	}

	void Pchain::ReleaseList(Pnode* first)
	{
		while (first) {
			Pnode* next = first->next;
			Release(first);
			first = next;
		}
	}

	void Pchain::Clear()
	{
		Pnode* node = root_node;
		while (node) {
			Pnode* next = node->next;
			if (func_free)
				func_free(node);
			node = next;
		}

		Block* block = root_block;
		while (block) {
			Block* next = block->next;
			memf(block);
			block = next;
		}

		root_block = nullptr;
		last_block = nullptr;
		free_node = nullptr;
		root_node = nullptr;
		last_node = nullptr;
		node_count = 0;
		acquired_count = 0;
		node_capacity = 0;
	}

	void Pchain::LinkNode(Pnode* node, Pnode* left, Pnode* right)
	{
		node->left = left;
		node->next = right;
		if (left)
			left->next = node;
		else
			root_node = node;
		if (right)
			right->left = node;
		else
			last_node = node;
		node_count++;
	}

	void Pchain::UnlinkNode(Pnode* node)
	{
		if (node->left)
			node->left->next = node->next;
		else
			root_node = node->next;
		if (node->next)
			node->next->left = node->left;
		else
			last_node = node->left;
		node->left = nullptr;
		node->next = nullptr;
		if (node_count)
			node_count--;
	}

	Pnode* Pchain::Append(pureptr_t addr, bool onleft, Pnode* node)
	{
		Pnode* inserted = Acquire();
		if (!inserted)
			return nullptr;
		inserted->offs = addr;

		if (!root_node) {
			LinkNode(inserted, nullptr, nullptr);
		}
		else if (!node) {
			if (onleft)
				LinkNode(inserted, nullptr, root_node);
			else
				LinkNode(inserted, last_node, nullptr);
		}
		else if (onleft) {
			LinkNode(inserted, node->left, node);
		}
		else {
			LinkNode(inserted, node, node->next);
		}
		return inserted;
	}

	bool Pchain::Insert(stduint index, pureptr_t addr)
	{
		if (index > node_count)
			return false;
		if (index == node_count)
			return Append(addr) != nullptr;
		return Append(addr, true, LocateNode(index)) != nullptr;
	}

	Pnode* Pchain::Remove(Pnode* node)
	{
		if (!node)
			return nullptr;
		if (node != root_node && node != last_node && !node->left && !node->next)
			return nullptr;

		Pnode* next = node->next;
		UnlinkNode(node);
		if (func_free)
			func_free(node);
		Release(node);
		return next;
	}

	Pnode* Pchain::Remove(pureptr_t content)
	{
		return Remove(FindNode(content));
	}

	bool Pchain::Remove(stduint index, stduint times)
	{
		if (!times)
			return true;
		if (index >= node_count || times > node_count - index)
			return false;

		Pnode* node = LocateNode(index);
		while (times--)
			node = Remove(node);
		return true;
	}

	pureptr_t Pchain::Locate(stduint index) const
	{
		return (pureptr_t)LocateNode(index);
	}

	Pnode* Pchain::LocateNode(stduint index) const
	{
		if (index >= node_count)
			return nullptr;

		if (index <= node_count / 2) {
			Pnode* node = root_node;
			while (index--)
				node = node->next;
			return node;
		}

		Pnode* node = last_node;
		stduint distance = node_count - index - 1;
		while (distance--)
			node = node->left;
		return node;
	}

	stduint Pchain::Locate(pureptr_t content, bool from_right) const
	{
		if (from_right) {
			Pnode* node = last_node;
			stduint index = node_count;
			while (node) {
				index--;
				if (node->offs == content)
					return index;
				node = node->left;
			}
		}
		else {
			Pnode* node = root_node;
			stduint index = 0;
			while (node) {
				if (node->offs == content)
					return index;
				node = node->next;
				index++;
			}
		}
		return ~(stduint)0;
	}

	Pnode* Pchain::FindNode(pureptr_t content, _tocomp_ft compare) const
	{
		Pnode* node = root_node;
		while (node) {
			if ((!compare && node->offs == content) ||
				(compare && !compare(node->offs, content)))
				return node;
			node = node->next;
		}
		return nullptr;
	}

	bool Pchain::Exchange(stduint index1, stduint index2)
	{
		Pnode* node1 = LocateNode(index1);
		Pnode* node2 = LocateNode(index2);
		if (!node1 || !node2)
			return false;
		if (node1 != node2) {
			pureptr_t offs = node1->offs;
			node1->offs = node2->offs;
			node2->offs = offs;
			stduint type = node1->type;
			node1->type = node2->type;
			node2->type = type;
		}
		return true;
	}

	void Pchain::Iterate()
	{
		if (_iterate_datas)
			delete[] _iterate_datas;
		_iterate_lim = node_count;
		_iterate_crt = 0;
		_iterate_datas = node_count ? new pureptr_t[node_count] : nullptr;

		Pnode* node = root_node;
		for (stduint index = 0; index < node_count; index++) {
			_iterate_datas[index] = node;
			node = node->next;
		}
	}

	void Pchain::Reverse()
	{
		Pnode* node = root_node;
		while (node) {
			Pnode* next = node->next;
			node->next = node->left;
			node->left = next;
			node = next;
		}
		Pnode* root = root_node;
		root_node = last_node;
		last_node = root;
	}

	bool Pchain::Match(pureptr_t content, stduint type) const
	{
		Pnode* node = root_node;
		while (node) {
			if (node->offs == content && node->type == type)
				return true;
			node = node->next;
		}
		return false;
	}

	byte* Pchain::GetExtnField(Pnode* node) const
	{
		return node ? (byte*)node + sizeof(Pnode) : nullptr;
	}

	const byte* Pchain::GetExtnField(const Pnode* node) const
	{
		return node ? (const byte*)node + sizeof(Pnode) : nullptr;
	}

}
