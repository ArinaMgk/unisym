#ifndef _AASM_HASHTBL
#define _AASM_HASHTBL

#include "../../inc/cpp/string"

#define HASH_MAX_LOAD		2 /* Higher = more memory-efficient, slower */


struct hash_tbl_node {
    uint64_t hash;
    const char *key;
    void *data;
};

struct hash_table;

struct hash_insert {
    uint64_t hash;
    struct hash_table *head;
	struct hash_tbl_node* where;
};

_ESYM_C uint64_t crc64(uint64_t crc, const char *string);
_ESYM_C uint64_t crc64i(uint64_t crc, const char *string);
#define CRC64_INIT UINT64_C(0xffffffffffffffff)

/* Some reasonable initial sizes... */
#define HASH_SMALL	4
#define HASH_MEDIUM	16
#define HASH_LARGE	256

inline static struct hash_tbl_node *alloc_table(size_t newsize)
{
	size_t bytes = newsize*sizeof(struct hash_tbl_node);
	hash_tbl_node *newtbl = (hash_tbl_node*)zalc(bytes);
	return newtbl;
}

struct hash_table {
    struct hash_tbl_node *table;
    size_t load;
    size_t size;
	size_t max_load;
	//
	void hash_init(size_t _size) {
		table    = alloc_table(_size);
		load     = 0;
		size     = _size;
		max_load = _size * (HASH_MAX_LOAD - 1) / HASH_MAX_LOAD;
	}
	/*
	* Find an entry in a hash table.
	*
	* On failure, if "insert" is non-NULL, store data in that structure
	* which can be used to insert that node using hash_add().
	*
	* WARNING: this data is only valid until the very next call of
	* hash_add(); it cannot be "saved" to a later date.
	*
	* On success, return a pointer to the "data" element of the hash
	* structure.
	*/
	void** hash_find(const char* key, struct hash_insert* insert) {
		struct hash_table* head = this;
		//if (!head->size) return NULL;
		struct hash_tbl_node* np;
		uint64_t hash = crc64(CRC64_INIT, key);
		struct hash_tbl_node* tbl = head->table;
		size_t mask = head->size - 1;
		size_t pos = hash & mask;
		size_t inc = ((hash >> 32) & mask) | 1;	/* Always odd */
		while ((np = &tbl[pos])->key) {
			if (hash == np->hash && !StrCompare(key, np->key))
				return &np->data;
			pos = (pos + inc) & mask;
		}
		/* Not found.  Store info for insert if requested. */
		if (insert) {
			insert->head = head;
			insert->hash = hash;
			insert->where = np;
		}
		return NULL;
	}
	void** hash_findi(const char* key, struct hash_insert* insert) {
		struct hash_table* head = this;
		struct hash_tbl_node* np;
		uint64_t hash = crc64i(CRC64_INIT, key);
		struct hash_tbl_node* tbl = head->table;
		size_t mask = head->size - 1;
		size_t pos = hash & mask;
		size_t inc = ((hash >> 32) & mask) | 1;	/* Always odd */
		while ((np = &tbl[pos])->key) {
			if (hash == np->hash && !StrCompareInsensitive(key, np->key))
				return &np->data;
			pos = (pos + inc) & mask;
		}
		/* Not found.  Store info for insert if requested. */
		if (insert) {
			insert->head = head;
			insert->hash = hash;
			insert->where = np;
		}
		return NULL;
	}
	/*
	* Iterate over all members of a hash set.  For the first call,
	* iterator should be initialized to NULL.  Returns the data pointer,
	* or NULL on failure.
	*/
	void* hash_iterate(struct hash_tbl_node** iterator, const char** key) const {
		const struct hash_table* head = this;
		struct hash_tbl_node* np = *iterator;
		struct hash_tbl_node* ep = head->table + head->size;

		if (!np) {
			np = head->table;
			if (!np)
				return NULL;	/* Uninitialized table */
		}

		while (np < ep) {
			if (np->key) {
				*iterator = np + 1;
				if (key)
					*key = np->key;
				return np->data;
			}
			np++;
		}

		*iterator = NULL;
		if (key)
			*key = NULL;
		return NULL;
	}
	/*
	* Free the hash itself.  Doesn't free the data elements; use
	* hash_iterate() to do that first, if needed.
	*/
	void hash_free() {
		void* p = this->table;
		this->table = NULL;
		memf(p);
	}
};

inline static void** hash_add(struct hash_insert* insert, const char* key, void* data) {
	struct hash_table* head = insert->head;
	struct hash_tbl_node* np = insert->where;

	/* Insert node.  We can always do this, even if we need to
	   rebalance immediately after. */
	np->hash = insert->hash;
	np->key = key;
	np->data = data;
	if (++head->load > head->max_load) {
	/* Need to expand the table */
		size_t newsize = head->size << 1;
		struct hash_tbl_node* newtbl = alloc_table(newsize);
		size_t mask = newsize - 1;

		if (head->table) {
			struct hash_tbl_node* op, * xp;
			size_t i;

			/* Rebalance all the entries */
			for (i = 0, op = head->table; i < head->size; i++, op++) {
				if (op->key) {
					size_t pos = op->hash & mask;
					size_t inc = ((op->hash >> 32) & mask) | 1;

					while ((xp = &newtbl[pos])->key)
						pos = (pos + inc) & mask;

					*xp = *op;
					if (op == np)
						np = xp;
				}
			}
			memf(head->table);
		}
		head->table = newtbl;
		head->size = newsize;
		head->max_load = newsize * (HASH_MAX_LOAD - 1) / HASH_MAX_LOAD;
	}
	return &np->data;
}






#endif
