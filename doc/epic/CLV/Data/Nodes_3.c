typedef struct Dnode {
	struct Dnode* next;
	union { char* addr; pureptr_t offs; };
	struct Dnode* left;
	union { stduint type; stduint lens; };
} Dnode; // measures stdint[4]
