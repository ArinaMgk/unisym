#define _MACRO_CHAIN_MEMBER \
Node* root_node;\
Node* last_node;\
struct {\
	Node* midl_node;\
} fastab;\
stduint node_count;\
stduint extn_field;\
struct {\
	bool been_sorted /* `need_sort` as para of Append */;\
} state;

// C...
typedef struct NodeChain_t {
	_MACRO_CHAIN_MEMBER
	_tofree_ft func_free;
	_tocomp_ft func_comp;
} chain_t;

// C++...
class Chain : public ArrayTrait, public IterateTrait {
protected: _MACRO_CHAIN_MEMBER /*...*/
public:
	_tofree_ft func_free;
	Chain(bool defa_free = false);
	~Chain();
	/*...*/
}
