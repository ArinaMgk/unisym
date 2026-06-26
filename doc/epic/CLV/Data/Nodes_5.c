typedef struct Nnode
{
	// Dnode
	struct Nnode* next;
	union { char* addr; void* offs; };
	union { struct Nnode* left, * pare; };
	union { stduint type; stduint lens; };
	//
	struct Nnode* subf;// sub-first item
} Nnode, nnode;
