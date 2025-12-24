
#ifndef _AASM_RBTREE
#define _AASM_RBTREE

#include "../../inc/cpp/unisym"

/* This structure should be embedded in a larger data structure;
   the final output from rb_search() can then be converted back
   to the larger data structure via container_of(). */
struct rbtree {
	uint64_t key;
	struct rbtree* left, * right;
	bool red;
	//
	rbtree* rb_insert(rbtree* node)
	{
		rbtree* tree = this;
		if (!tree) {
			node->red = true;
			return node;
		}
		if (tree->left->is_red() && tree->right->is_red())
			color_flip();
		if (node->key < tree->key)
			tree->left = tree->left->rb_insert(node);
		else
			tree->right = tree->right->rb_insert(node);
		if (tree->right->is_red())
			tree = rotate_left();
		if (tree->left->is_red() && tree->left->left->is_red())
			tree = rotate_right();
		return tree;
	}
	rbtree* rb_search(uint64_t key) {
		rbtree* tree = this;
		struct rbtree* best = NULL;
		while (tree) {
			if (tree->key == key)
				return tree;
			else if (tree->key > key)
				tree = tree->left;
			else {
				best = tree;
				tree = tree->right;
			}
		}
		return best;
	}
	//

	bool is_red()
	{
		return this->red;
	}

	rbtree* rotate_left()
	{
		rbtree* h = this;
		rbtree* x = h->right;
		h->right = x->left;
		x->left = h;
		x->red = x->left->red;
		x->left->red = true;
		return x;
	}
	rbtree* rotate_right()
	{
		rbtree* h = this;
		rbtree* x = h->left;
		h->left = x->right;
		x->right = h;
		x->red = x->right->red;
		x->right->red = true;
		return x;
	}

	void color_flip()
	{
		rbtree* h = this;
		h->red = !h->red;
		h->left->red = !h->left->red;
		h->right->red = !h->right->red;
	}





};




#endif
