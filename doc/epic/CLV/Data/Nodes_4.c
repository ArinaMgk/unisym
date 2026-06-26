#define _MACRO_DCHAIN_MEMBER \
Dnode* root_node;\
Dnode* last_node;\
struct {\
	Dnode* midl_node;\
} fastab;\
stduint node_count;\
stduint extn_field;\
struct {\
	bool been_sorted /* `need_sort` as para of Append */;\
} state;
	
// C...
typedef struct DnodeChain_t {
	_MACRO_DCHAIN_MEMBER
	_tofree_ft func_free;
	_tocomp_ft func_comp;
} dchain_t;
typedef struct Tnode {
	Dnode;
	TnodeField;
} Tnode;// Magice Style
	
// C++...
class Dchain : public ArrayTrait, public IterateTrait {
protected: _MACRO_DCHAIN_MEMBER /*...*/
public:
	_tofree_ft func_free;
	Dchain(); ~Dchain();
	/*...*/
}
