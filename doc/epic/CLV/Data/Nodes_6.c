// C...
typedef struct NnodeChain_t {
	Nnode* root_node;
	stduint extn_field;
	_tofree_ft func_free;
	_tocomp_ft func_comp;
} nchain_t, Nchain;

// C++...
class Nchain {
protected:
	Nnode* root_node;
public:
	_tofree_ft func_free;
	stduint extn_field;
	//
	Nchain(bool defa_free = false); ~Nchain();
};
