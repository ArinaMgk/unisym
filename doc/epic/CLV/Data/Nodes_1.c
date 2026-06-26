	typedef struct Node {
		struct Node* next;
		union {
			const char* addr;
			pureptr_t offs;
		};
	} Node; // measures stdint[2]
